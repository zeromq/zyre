/*  =========================================================================
    zyre_node - node on a ZRE network

    -------------------------------------------------------------------------
    Copyright (c) the Contributors as noted in the AUTHORS file.

    This file is part of Zyre, an open-source framework for proximity-based
    peer-to-peer applications -- See http://zyre.org.

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
    =========================================================================
*/

#include "zyre_classes.h"

//  --------------------------------------------------------------------------
//  Structure of our class

struct _zyre_node_t {
    //  We send command replies and signals to the pipe
    zsock_t *pipe;              //  Pipe back to application
    //  We send all Zyre messages to the outbox
    zsock_t *outbox;            //  Outbox back to application
    bool terminated;            //  API shut us down
    bool verbose;               //  Log all traffic
    int beacon_port;            //  Beacon UDP port number
    char *ephemeral_port;         //  Beacon TCP ephemeral port number
    byte beacon_version;        //  Beacon version
    uint64_t evasive_timeout;   //  Time since a message is received before a peer is considered evasive
    uint64_t expired_timeout;   //  Time since a message is received before a peer is considered gone
    size_t interval;            //  Beacon interval
    zpoller_t *poller;          //  Socket poller
    zactor_t *beacon;           //  Beacon actor
    zuuid_t *uuid;              //  Our UUID as object
    zsock_t *inbox;             //  Our inbox socket (ROUTER)
    char *name;                 //  Our public name
    char *endpoint;             //  Our public endpoint
    char *advertised_endpoint;  //  Our advertised public endpoint - NAT workaround?
    int port;                   //  Our inbox port, if any
    byte status;                //  Our own change counter
    zhash_t *peers;             //  Hash of known peers, fast lookup
    zhash_t *peer_groups;       //  Groups that our peers are in
    zlist_t *own_groups;        //  Groups that we are in
    zhash_t *headers;           //  Our header values
    zactor_t *gossip;           //  Gossip discovery service, if any
    char *gossip_bind;          //  Gossip bind endpoint, if any
    char *gossip_connect;       //  Gossip connect endpoint, if any
    char *public_key;           // Our curve public key
    char *secret_key;           // Our curve private key
    char *zap_domain;           // ZAP domain if any
};

//  Beacon frame has this format:
//
//  Z R E       3 bytes
//  version     1 byte 0x01 | 0x03
//  UUID        16 bytes
//  port        2 bytes in network order
//  curve key   32 bytes if version == 0x03

#define BEACON_VERSION_V2 0x01
#define BEACON_VERSION_V3 0x03
#define BEACON_VERSION BEACON_VERSION_V2
#define BEACON_SIZE_V2 22
#define BEACON_SIZE_V3 54
#define BEACON_SIZE(b) \
   b.version == BEACON_VERSION_V2 ? BEACON_SIZE_V2 : BEACON_SIZE_V3


typedef struct {
    byte protocol [3];
    byte version;
    byte uuid [ZUUID_LEN];
    uint16_t port;
    uint8_t public_key [32];
} beacon_t;

//  --------------------------------------------------------------------------
//  Local helper

static zyre_group_t *
zyre_node_require_peer_group (zyre_node_t *self, const char *name);

static int
s_string_compare (void *item1, void *item2)
{
    const char *str1 = (const char *) item1;
    const char *str2 = (const char *) item2;
    return strcmp (str1, str2);
}

//  --------------------------------------------------------------------------
//  Constructor

static zyre_node_t *
zyre_node_new (zsock_t *pipe, void *args)
{
    zyre_node_t *self = (zyre_node_t *) zmalloc (sizeof (zyre_node_t));
    self->inbox = zsock_new (ZMQ_ROUTER);
    if (self->inbox == NULL) {
        free (self);
        return NULL;            //  Could not create new socket
    }
    //  Use ZMQ_ROUTER_HANDOVER so that when a peer disconnects and
    //  then reconnects, the new client connection is treated as the
    //  canonical one, and any old trailing commands are discarded.
    zsock_set_router_handover (self->inbox, 1);

    self->pipe = pipe;
    self->outbox = (zsock_t *) args;
    self->poller = zpoller_new (self->pipe, NULL);
    self->beacon_port = ZRE_DISCOVERY_PORT;
    self->ephemeral_port = NULL; // random system assigned port
    self->evasive_timeout = 5000;
    self->expired_timeout = 30000;
    self->interval = 0;         //  Use default
    self->uuid = zuuid_new ();
    self->peers = zhash_new ();
    self->peer_groups = zhash_new ();
    self->own_groups = zlist_new ();
    zlist_autofree (self->own_groups);
    zlist_comparefn (self->own_groups, s_string_compare);
    self->headers = zhash_new ();
    zhash_autofree (self->headers);

    self->beacon_version = BEACON_VERSION_V2;
    self->zap_domain = strdup(ZAP_DOMAIN_DEFAULT);

    //  Default name for node is first 6 characters of UUID:
    //  the shorter string is more readable in logs
    self->name = (char *) zmalloc (7);
    memcpy (self->name, zuuid_str (self->uuid), 6);
    return self;
}


//  --------------------------------------------------------------------------
//  Destructor

static void
zyre_node_destroy (zyre_node_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zyre_node_t *self = *self_p;
        zpoller_destroy (&self->poller);
        zuuid_destroy (&self->uuid);
        zhash_destroy (&self->peers);
        zhash_destroy (&self->peer_groups);
        zlist_destroy (&self->own_groups);
        zhash_destroy (&self->headers);
        zsock_destroy (&self->inbox);
        zsock_destroy (&self->outbox);
        zactor_destroy (&self->beacon);
        zactor_destroy (&self->gossip);
        zstr_free (&self->endpoint);
        zstr_free (&self->gossip_bind);
        zstr_free (&self->gossip_connect);
        zstr_free (&self->secret_key);
        zstr_free (&self->public_key);
        zstr_free (&self->zap_domain);
        zstr_free (&self->advertised_endpoint);
        zstr_free (&self->ephemeral_port);
        free (self->name);
        free (self);
        *self_p = NULL;
    }
}


//  If we haven't already set-up the gossip network, do so
static void
zyre_node_gossip_start (zyre_node_t *self)
{
    if (!self->gossip) {
        self->beacon_port = 0;      //  Disable UDP beaconing
        self->gossip = zactor_new (zgossip, self->name);
        if (self->verbose)
            zstr_send (self->gossip, "VERBOSE");
        assert (self->gossip);
    }
}

//  Start node, return 0 if OK, 1 if not possible

static int
zyre_node_start (zyre_node_t *self)
{
    if (self->secret_key) {
        // apply the cert to the socket
        if (self->verbose)
            zsys_debug ("applying zcert to ->inbox");

        uint8_t pub[32] = { 0 }, sec[32] = { 0 };
        assert (zmq_z85_decode (pub, self->public_key));
        assert (zmq_z85_decode (sec, self->secret_key));
        zcert_t *cert = zcert_new_from(pub, sec);
        zcert_apply(cert, self->inbox);
        zsock_set_curve_server (self->inbox, 1);
        zsock_set_zap_domain (self->inbox, self->zap_domain);
        zcert_destroy(&cert);
    }

    if (self->beacon_port) {
        //  Start beacon discovery
        //  ------------------------------------------------------------------
        assert (!self->beacon);

        if (self->secret_key) {
            // upgrade the beacon version
            if (self->verbose)
                zsys_debug ("switching to beacon v3");
            self->beacon_version = BEACON_VERSION_V3;
        }

        self->beacon = zactor_new (zbeacon, NULL);
        if (!self->beacon)
            return 1;               //  Not possible to start beacon

        if (self->verbose)
            zsock_send (self->beacon, "s", "VERBOSE");
    }
    else {
        //  Start gossip discovery
        //  ------------------------------------------------------------------
        //  If application didn't set an endpoint explicitly, grab ephemeral
        //  port on all available network interfaces.
        if (!self->endpoint) {
            const char *iface = zsys_interface ();
            if (streq (iface, ""))
                iface = "*";
            self->port = zsock_bind (self->inbox, "tcp://%s:*", iface);
            assert (self->port > 0);    //  Die on bad interface or port exhaustion

            char *hostname = zsys_hostname ();
            self->endpoint = zsys_sprintf ("tcp://%s:%d", hostname, self->port);
            zstr_free (&hostname);
        }
        assert (self->gossip);

        char *published_endpoint = strdup(self->endpoint);

        // if the advertised endpoint is set and different than our bound endpoint
        if (self->advertised_endpoint) {
            zstr_free (&published_endpoint);
            published_endpoint = zsys_sprintf("%s", self->advertised_endpoint);
        }

        // further arrange if we have a public key associated
        if (self->public_key) {
            // there has got to be a better way to do this. valgrind gets cranky
            char *t1 = strdup(published_endpoint);
            zstr_free (&published_endpoint);
            published_endpoint = zsys_sprintf("%s|%s", t1, self->public_key);
            zstr_free (&t1);
        }

        zstr_sendx (self->gossip, "PUBLISH", zuuid_str (self->uuid), published_endpoint, NULL);
        zstr_free (&published_endpoint);

        //  Start polling on zgossip
        zpoller_add (self->poller, self->gossip);
        //  Start polling on inbox
        zpoller_add(self->poller, self->inbox);
    }

    // this needs to be tested after bind
#ifndef ZMQ_CURVE
        // legacy ZMQ support
        // inline incase the underlying assert is removed
        bool ZMQ_CURVE = false;
#endif
    if (self->secret_key)
        assert (zsock_mechanism (self->inbox) == ZMQ_CURVE);

    return 0;
}

//  Stop node discovery and interconnection
//  TODO: clear peer tables; test stop/start cycles; how will this work
//  with gossip network? Do we leave that running in the meantime?

static int
zyre_node_stop (zyre_node_t *self)
{
    if (self->beacon) {
        //  Stop broadcast/listen beacon
        beacon_t beacon;
        beacon.protocol [0] = 'Z';
        beacon.protocol [1] = 'R';
        beacon.protocol [2] = 'E';
        beacon.version = self->beacon_version;
        if (self->public_key)
            zmq_z85_decode(beacon.public_key, self->public_key);
        beacon.port = 0;            //  Zero means we're stopping
        zuuid_export (self->uuid, beacon.uuid);
        zsock_send (self->beacon, "sbi", "PUBLISH",
            (byte *) &beacon, BEACON_SIZE(beacon), self->interval);
        zclock_sleep (1);           //  Allow 1 msec for beacon to go out
        zpoller_remove (self->poller, self->beacon);
        zactor_destroy (&self->beacon);
    }
    //  Stop polling on inbox
    zpoller_remove (self->poller, self->inbox);
    zstr_sendm (self->outbox, "STOP");
    zstr_sendm (self->outbox, zuuid_str (self->uuid));
    zstr_send (self->outbox, self->name);
    return 0;
}


//  Send message to one peer; called while looping over zhash

static int
zyre_node_send_peer (const char *key, void *item, void *argument)
{
    zyre_peer_t *peer = (zyre_peer_t *) item;
    zre_msg_t *msg = zre_msg_dup ((zre_msg_t *) argument);
    zyre_peer_send (peer, &msg);
    return 0;
}

//  Print hash key to log

static int
zyre_node_log_peer (zyre_peer_t *peer)
{
    zsys_info ("   - uuid=%s name=%s endpoint=%s connected=%s ready=%s sent_seq=%" PRIu16 " want_seq=%" PRIu16,
        zyre_peer_identity(peer),
        zyre_peer_name(peer),
        zyre_peer_endpoint(peer),
        zyre_peer_connected(peer) ? "yes" : "no",
        zyre_peer_ready(peer) ? "yes" : "no",
        zyre_peer_sent_sequence(peer),
        zyre_peer_want_sequence(peer));
    return 0;
}

static int
zyre_node_log_pair (const char *key, void *item, void *argument)
{
    zsys_info ("   - %s: %s", key, (const char *)item);
    return 0;
}

//  Prints node information

static void
zyre_node_dump (zyre_node_t *self)
{
    void *item;

    zsys_info ("zyre_node: dump state");
    zsys_info (" - name=%s uuid=%s", self->name, zuuid_str (self->uuid));

    zsys_info (" - endpoint=%s", self->endpoint);
#ifdef ZYRE_BUILD_DRAFT_API
    //  DRAFT-API: Security
    if (self->public_key)
        zsys_info (" - public-key: %s", self->public_key);
#endif
    if (self->beacon_port)
        zsys_info (" - discovery=beacon port=%d interval=%zu",
                   self->beacon_port, self->interval);
    else {
        zsys_info (" - discovery=gossip");
        if (self->gossip_bind)
            zsys_info ("   - bind endpoint=%s", self->gossip_bind);
        if (self->gossip_connect)
            zsys_info ("   - connect endpoint=%s", self->gossip_connect);
    }
    zsys_info (" - headers=%zu:", zhash_size (self->headers));
    for (item = zhash_first (self->headers); item != NULL;
            item = zhash_next (self->headers))
        zyre_node_log_pair (zhash_cursor (self->headers), item, self);

    zsys_info (" - peers=%zu:", zhash_size (self->peers));
    for (item = zhash_first (self->peers); item != NULL;
            item = zhash_next (self->peers))
        zyre_node_log_peer((zyre_peer_t *)item);

    zsys_info (" - own groups=%zu:", zlist_size (self->own_groups));
    const char *group = (const char *) zlist_first (self->own_groups);
    while (group) {
        zsys_info ("   - %s", group);
        group = (const char *) zlist_next (self->own_groups);
    }

    zsys_info (" - peer groups=%zu:", zhash_size (self->peer_groups));
    zlist_t *groups = zhash_keys (self->peer_groups);
    group = (const char *) zlist_first (groups);
    while (group) {
        zsys_info ("   - %s", group);
        zyre_group_t *rgroup = (zyre_group_t *) zhash_lookup (self->peer_groups, group);
        zlist_t *neighbors = zyre_group_peers (rgroup);
        char *neighbor = (char *) zlist_first (neighbors);
        while (neighbor) {
            zsys_info ("     - %s", neighbor);
            neighbor = (char *) zlist_next (neighbors);
        }
        zlist_destroy (&neighbors);
        group = (const char *) zlist_next (groups);
    }
    zlist_destroy (&groups);

}


//  Here we handle the different control messages from the front-end

// Forward declaration so that REQUIRE PEER works
static zyre_peer_t *
zyre_node_require_peer (zyre_node_t *self, zuuid_t *uuid, const char *endpoint, const char *public_key);

static void
zyre_node_recv_api (zyre_node_t *self)
{
    //  Get the whole message off the pipe in one go
    zmsg_t *request = zmsg_recv (self->pipe);
    if (!request)
        return;                 //  Interrupted

    char *command = zmsg_popstr (request);

    if (self->verbose)
        zsys_debug ("%s:     API command=%s", self->name, command);

    if (streq (command, "UUID"))
        zstr_send (self->pipe, zuuid_str (self->uuid));
    else
    if (streq (command, "NAME"))
        zstr_send (self->pipe, self->name);
    else
    if (streq (command, "SET NAME")) {
        free (self->name);
        self->name = zmsg_popstr (request);
        assert (self->name);
    }
    else
    if (streq (command, "SET HEADER")) {
        char *name = zmsg_popstr (request);
        char *value = zmsg_popstr (request);
        zhash_update (self->headers, name, value);
        zstr_free (&name);
        zstr_free (&value);
    }
    else
    if (streq (command, "SET VERBOSE"))
        self->verbose = true;
    else
    if (streq (command, "SET PORT")) {
        char *value = zmsg_popstr (request);
        self->beacon_port = atoi (value);
        zstr_free (&value);
    }
    else
    if (streq (command, "SET EPHEMERAL PORT")) {
        zstr_free (&self->ephemeral_port);
        self->ephemeral_port = zmsg_popstr(request);
    }
    else
    if (streq (command, "SET EVASIVE TIMEOUT")) {
        char *value = zmsg_popstr (request);
        self->evasive_timeout = atoi (value);
        zstr_free (&value);
    }
    else
    if (streq (command, "SET EXPIRED TIMEOUT")) {
        char *value = zmsg_popstr (request);
        self->expired_timeout = atoi (value);
        zstr_free (&value);
    }
    else
    if (streq (command, "SET INTERVAL")) {
        char *value = zmsg_popstr (request);
        self->interval = atol (value);
        zstr_free (&value);
    }
    else
#ifdef ZYRE_BUILD_DRAFT_API
//  DRAFT-API: Election
    if (streq (command, "SET CONTEST")) {
        char *groupname = zmsg_popstr (request);
        zyre_group_t *group = zyre_node_require_peer_group (self, groupname);
        zyre_group_set_contest (group);
        zstr_free (&groupname);
    }
    else
#endif
#ifdef ZYRE_BUILD_DRAFT_API
        //  DRAFT-API: Public IP
    if (streq (command, "SET ADVERTISED ENDPOINT")) {
        self->advertised_endpoint = zmsg_popstr (request);
    }
    else
#endif
    if (streq (command, "SET ENDPOINT")) {
        zyre_node_gossip_start (self);
        char *endpoint = zmsg_popstr (request);
        // when SET ENDPOINT is called, it calls zsock_bind
        // CURVE needs to be set before that happens
        // this happens when connecting nodes define the endpoint
        if (self->secret_key) {
            // apply the cert to the socket
            if (self->verbose)
                zsys_debug ("applying zcert to ->inbox");

            uint8_t pub[32] = { 0 }, sec[32] = { 0 };
            assert (zmq_z85_decode (pub, self->public_key));
            assert (zmq_z85_decode (sec, self->secret_key));
            zcert_t *cert = zcert_new_from(pub, sec);
            zcert_apply(cert, self->inbox);
            zsock_set_curve_server (self->inbox, 1);
            zsock_set_zap_domain (self->inbox, self->zap_domain);
            zcert_destroy(&cert);
        }
        if (zsock_bind (self->inbox, "%s", endpoint) != -1) {
            zstr_free(&self->endpoint);
#ifndef ZMQ_CURVE
            // legacy ZMQ support
            // inline incase the underlying assert is removed
            bool ZMQ_CURVE = false;
#endif
            // if we set a secret key- make sure the bind ZMQ_CURVE'd properly
            if (self->secret_key)
                assert (zsock_mechanism (self->inbox) == ZMQ_CURVE);

        self->endpoint = strdup(zsock_endpoint(self->inbox));
		zstr_free(&endpoint);
            zsock_signal (self->pipe, 0);
        }
        else {
            zstr_free (&endpoint);
            zsock_signal (self->pipe, 1);
        }
    }
    else
#ifdef ZYRE_BUILD_DRAFT_API
    //  DRAFT-API: Security
    if (streq (command, "SET PUBLICKEY")) {
        self->public_key = zmsg_popstr (request);
        zhash_update (self->headers, "X-PUBLICKEY", self->public_key);
        assert (self->public_key);
    }
    else
    if (streq (command, "SET SECRETKEY")) {
        self->secret_key = zmsg_popstr (request);
        assert (self->secret_key);
    }
    else
    if (streq (command, "ZAP DOMAIN")) {
        free (self->zap_domain);
        self->zap_domain = zmsg_popstr (request);
        assert (self->zap_domain);
    }
    else
#endif
    if (streq (command, "GOSSIP BIND")) {
        zyre_node_gossip_start (self);
        zstr_free (&self->gossip_bind);
        self->gossip_bind = zmsg_popstr (request);
        if (self->secret_key) {
            zstr_sendx(self->gossip, "SET SECRETKEY", self->secret_key, NULL);
            zstr_sendx(self->gossip, "SET PUBLICKEY", self->public_key, NULL);
        }
        zstr_sendx (self->gossip, "BIND", self->gossip_bind, NULL);
    }
    else
    if (streq (command, "GOSSIP CONNECT")) {
        zyre_node_gossip_start (self);
        zstr_free (&self->gossip_connect);
        self->gossip_connect = zmsg_popstr (request);
        if (self->secret_key) {
            zstr_sendx(self->gossip, "SET SECRETKEY", self->secret_key, NULL);
            zstr_sendx(self->gossip, "SET PUBLICKEY", self->public_key, NULL);
        }
        char *server_public_key = zmsg_popstr (request);
        zstr_sendx (self->gossip, "CONNECT", self->gossip_connect, server_public_key, NULL);
        zstr_free (&server_public_key);
    }
#ifdef ZYRE_BUILD_DRAFT_API
    // DRAFT-API: GOSSIP TTL
    else
    if (streq (command, "GOSSIP UNPUBLISH")) {
        char *msg = zmsg_popstr (request);
        zstr_sendx (self->gossip, "UNPUBLISH", msg, NULL);
        zstr_free (&msg);
    }
#endif
    else
    if (streq (command, "START"))
        zsock_signal (self->pipe, zyre_node_start (self));
    else
    if (streq (command, "STOP"))
        zsock_signal (self->pipe, zyre_node_stop (self));
    else
    if (streq (command, "WHISPER")) {
        //  Get peer to send message to
        char *identity = zmsg_popstr (request);
        zyre_peer_t *peer = (zyre_peer_t *) zhash_lookup (self->peers, identity);

        //  Send frame on out to peer's mailbox, drop message
        //  if peer doesn't exist (may have been destroyed)
        if (peer) {
            zre_msg_t *msg = zre_msg_new ();
            zre_msg_set_id (msg, ZRE_MSG_WHISPER);
            zre_msg_set_content (msg, &request);
            zyre_peer_send (peer, &msg);
        }
        zstr_free (&identity);
    }
    else
    if (streq (command, "SHOUT")) {
        //  Get group to send message to
        char *name = zmsg_popstr (request);
        zyre_group_t *group = (zyre_group_t *) zhash_lookup (self->peer_groups, name);
        if (group) {
            zre_msg_t *msg = zre_msg_new ();
            zre_msg_set_id (msg, ZRE_MSG_SHOUT);
            zre_msg_set_group (msg, name);
            zre_msg_set_content (msg, &request);
            zyre_group_send (group, &msg);
        }
        zstr_free (&name);
    }
    else
    if (streq (command, "JOIN")) {
        char *name = zmsg_popstr (request);
        if (!zlist_exists (self->own_groups, name)) {
            void *item;
            //  Only send if we're not already in group
            zlist_append (self->own_groups, name);
            zre_msg_t *msg = zre_msg_new ();
            zre_msg_set_id (msg, ZRE_MSG_JOIN);
            zre_msg_set_group (msg, name);
            //  Update status before sending command
            zre_msg_set_status (msg, ++(self->status));
            for (item = zhash_first (self->peers); item != NULL;
                    item = zhash_next (self->peers))
                zyre_node_send_peer (zhash_cursor (self->peers), item, msg);

            zre_msg_destroy (&msg);
            if (self->verbose)
                zsys_info ("(%s) JOIN group=%s", self->name, name);
        }
        zstr_free (&name);
    }
    else
    if (streq (command, "LEAVE")) {
        char *name = zmsg_popstr (request);
        if (zlist_exists (self->own_groups, name)) {
            void *item;
            //  Only send if we are actually in group
            zre_msg_t *msg = zre_msg_new ();
            zre_msg_set_id (msg, ZRE_MSG_LEAVE);
            zre_msg_set_group (msg, name);
            //  Update status before sending command
            zre_msg_set_status (msg, ++(self->status));
            for (item = zhash_first (self->peers); item != NULL;
                    item = zhash_next (self->peers))
                zyre_node_send_peer (zhash_cursor (self->peers), item, msg);

            zre_msg_destroy (&msg);
            zlist_remove (self->own_groups, name);
            if (self->verbose)
                zsys_info ("(%s) LEAVE group=%s", self->name, name);
        }
        zstr_free (&name);
    }
    else
    if (streq (command, "PEERS"))
        zsock_send (self->pipe, "p", zhash_keys (self->peers));
    #ifdef ZYRE_BUILD_DRAFT_API
    //  DRAFT-API: Security
    else
    if (streq (command, "REQUIRE PEER")) {
        char *uuidstr = zmsg_popstr (request);
        char *endpoint = zmsg_popstr (request);
        char *public_key = zmsg_popstr (request);
        if (strneq (endpoint, self->endpoint)) {
            zuuid_t *uuid = zuuid_new ();
            zuuid_set_str (uuid, uuidstr);
            zyre_node_require_peer (self, uuid, endpoint, public_key);
            zuuid_destroy (&uuid);
        }
        zstr_free (&uuidstr);
        zstr_free (&endpoint);
        zstr_free (&public_key);
    }
    #endif
    else
    if (streq (command, "GROUP PEERS")) {
        char *name = zmsg_popstr (request);
        zyre_group_t *group = (zyre_group_t *) zhash_lookup (self->peer_groups, name);
        if (group)
            zsock_send (self->pipe, "p", zyre_group_peers (group));
        else
            zsock_send (self->pipe, "p", NULL);

        zstr_free (&name);
    }
    else
    if (streq (command, "PEER ENDPOINT")) {
        char *uuid = zmsg_popstr (request);
        zyre_peer_t *peer = (zyre_peer_t *) zhash_lookup (self->peers, uuid);
        if (peer)
            zsock_send (self->pipe, "s", zyre_peer_endpoint (peer));
        else
            zsock_send (self->pipe, "s", "");
        zstr_free (&uuid);
    }
    else
    if (streq (command, "PEER NAME")) {
        char *uuid = zmsg_popstr (request);
        zyre_peer_t *peer = (zyre_peer_t *) zhash_lookup (self->peers, uuid);
        assert (peer);
        zsock_send (self->pipe, "s", zyre_peer_name (peer));
        zstr_free (&uuid);
    }
    else
    if (streq (command, "PEER HEADER")) {
        char *uuid = zmsg_popstr (request);
        char *key = zmsg_popstr (request);
        zyre_peer_t *peer = (zyre_peer_t *) zhash_lookup (self->peers, uuid);
        if (!peer)
            zstr_send (self->pipe, "");
        else
            zstr_send (self->pipe, zyre_peer_header (peer, key, NULL));
        zstr_free (&uuid);
        zstr_free (&key);
    }
    else
    if (streq (command, "PEER GROUPS"))
        zsock_send (self->pipe, "p", zhash_keys (self->peer_groups));
    else
    if (streq (command, "OWN GROUPS"))
        zsock_send (self->pipe, "p", zlist_dup (self->own_groups));
    else
    if (streq (command, "DUMP"))
        zyre_node_dump (self);
    else
    if (streq (command, "$TERM"))
        self->terminated = true;
    else {
        zsys_error ("invalid command '%s'", command);
        assert (false);
    }
    zstr_free (&command);
    zmsg_destroy (&request);
}

//  Delete peer for a given endpoint

static int
zyre_node_purge_peer (const char *key, void *item, void *argument)
{
    zyre_peer_t *peer = (zyre_peer_t *) item;
    char *endpoint = (char *) argument;
    if (streq (zyre_peer_endpoint (peer), endpoint))
        zyre_peer_disconnect (peer);
    return 0;
}

//  Find or create peer via its UUID

static zyre_peer_t *
zyre_node_require_peer (zyre_node_t *self, zuuid_t *uuid, const char *endpoint, const char *public_key)
{
    assert (self);
    assert (endpoint);

    zyre_peer_t *peer = (zyre_peer_t *) zhash_lookup (self->peers, zuuid_str (uuid));
    if (!peer) {
        //  Purge any previous peer on same endpoint
        void *item;
        for (item = zhash_first (self->peers); item != NULL;
                item = zhash_next (self->peers))
            zyre_node_purge_peer (zhash_cursor (self->peers), item, (char *) endpoint);

        peer = zyre_peer_new (self->peers, uuid);
        assert (peer);

        if (self->public_key && self->secret_key) {
            assert (public_key != NULL);
            // set my local keys
            zyre_peer_set_public_key(peer, self->public_key);
            zyre_peer_set_secret_key(peer, self->secret_key);
            // set the public key of the peer we're connecting to
            // peer is acting as the 'server' curve role
            zyre_peer_set_server_key(peer, public_key);
        }

        zyre_peer_set_origin (peer, self->name);
        zyre_peer_set_verbose (peer, self->verbose);
        int rc = zyre_peer_connect (peer, self->uuid, endpoint,
                self->expired_timeout);
        if (rc != 0) {
            // TBD: removing the peer means it will keep retrying. Should
            // it be kept in the hash table instead perhaps?
            zhash_delete (self->peers, zyre_peer_identity (peer));
            return NULL;
        }

        //  Handshake discovery by sending HELLO as first message
        zlist_t *groups = zlist_dup (self->own_groups);
        zhash_t *headers = zhash_dup (self->headers);
        zre_msg_t *msg = zre_msg_new ();
        zre_msg_set_id (msg, ZRE_MSG_HELLO);

        //  If the endpoint is a link-local IPv6 address we must not send the
        //  interface name to the peer, as it is relevant only on the local node
        char endpoint_iface [NI_MAXHOST] = {0};
        if (zsys_ipv6 () && strchr (self->endpoint, '%')) {
            strcat (endpoint_iface, self->endpoint);
            memmove (strchr (endpoint_iface, '%'),
                    strrchr (endpoint_iface, ':'),
                    strlen (strrchr (endpoint_iface, ':')) + 1);
            zre_msg_set_endpoint (msg, endpoint_iface);
        }
        else
        if (self->advertised_endpoint)
            zre_msg_set_endpoint (msg, self->advertised_endpoint);
        else
            zre_msg_set_endpoint (msg, self->endpoint);

        zre_msg_set_groups (msg, &groups);
        zre_msg_set_status (msg, self->status);
        zre_msg_set_name (msg, self->name);
        zre_msg_set_headers (msg, &headers);
        zyre_peer_send (peer, &msg);
        zre_msg_destroy (&msg);

        zyre_peer_refresh (peer, self->evasive_timeout, self->expired_timeout);
    }
    return peer;
}


//  Remove peer from group, if it's a member

static int
zyre_node_delete_peer (const char *key, void *item, void *argument)
{
    zyre_group_t *group = (zyre_group_t *) item;
    zyre_peer_t *peer = (zyre_peer_t *) argument;
    zyre_group_leave (group, peer);
    return 0;
}

//  Remove a peer from our data structures

static void
zyre_node_remove_peer (zyre_node_t *self, zyre_peer_t *peer)
{
    void *item;
    //  Tell the calling application the peer has gone
    zstr_sendm (self->outbox, "EXIT");
    zstr_sendm (self->outbox, zyre_peer_identity (peer));
    zstr_send (self->outbox, zyre_peer_name (peer));

    if (self->verbose)
        zsys_info ("(%s) EXIT name=%s endpoint=%s",
                self->name, zyre_peer_name (peer), zyre_peer_endpoint (peer));

    //  Remove peer from any groups we've got it in
    for (item = zhash_first (self->peer_groups); item != NULL;
            item = zhash_next (self->peer_groups))
        zyre_node_delete_peer (zhash_cursor (self->peer_groups), item, peer);
    //  To destroy peer, we remove from peers hash table
    zhash_delete (self->peers, zyre_peer_identity (peer));


}


//  Find or create group via its name

static zyre_group_t *
zyre_node_require_peer_group (zyre_node_t *self, const char *name)
{
    zyre_group_t *group = (zyre_group_t *) zhash_lookup (self->peer_groups, name);
    if (!group)
        group = zyre_group_new (name, self->peer_groups);

    return group;
}

static zyre_group_t *
zyre_node_join_peer_group (zyre_node_t *self, zyre_peer_t *peer, const char *name)
{
    zyre_group_t *group = zyre_node_require_peer_group (self, name);
    zyre_group_join (group, peer);

    //  Now tell the caller about the peer joined group
    zstr_sendm (self->outbox, "JOIN");
    zstr_sendm (self->outbox, zyre_peer_identity (peer));
    zstr_sendm (self->outbox, zyre_peer_name (peer));
    zstr_send (self->outbox, name);

    if (self->verbose)
        zsys_info ("(%s) JOIN name=%s group=%s",
                self->name, zyre_peer_name (peer), name);

    return group;
}

static zyre_group_t *
zyre_node_leave_peer_group (zyre_node_t *self, zyre_peer_t *peer, const char *name)
{
    zyre_group_t *group = zyre_node_require_peer_group (self, name);
    zyre_group_leave (group, peer);

    //  Now tell the caller about the peer left group
    zstr_sendm (self->outbox, "LEAVE");
    zstr_sendm (self->outbox, zyre_peer_identity (peer));
    zstr_sendm (self->outbox, zyre_peer_name (peer));
    zstr_send (self->outbox, name);

    if (self->verbose)
        zsys_info ("(%s) LEAVE name=%s group=%s",
                self->name, zyre_peer_name (peer), name);

    return group;
}

static void
zyre_node_leader_peer_group (zyre_node_t *self, const char *identity,
                             const char *name, const char *group)
{
    //  Now tell the caller about the elected leader peer
    zstr_sendm (self->outbox, "LEADER");
    zstr_sendm (self->outbox, identity);
    zstr_sendm (self->outbox, name);
    zstr_send (self->outbox, group);

    if (self->verbose)
        zsys_info ("(%s) LEADER name=%s group=%s identity=%s",
                   self->name,
                   name,
                   group,
                   identity);
}

//  Here we handle messages coming from other peers

static void
zyre_node_recv_peer (zyre_node_t *self)
{
    //  Router socket tells us the identity of this peer
    zre_msg_t *msg = zre_msg_new ();
    int rc = zre_msg_recv (msg, self->inbox);
    if (rc == -1)
        return;                 //  Interrupted
    if (rc == -2) {
        zre_msg_destroy (&msg);
        return;                 //  Malformed
    }

    //  First frame is sender identity
    byte *peerid_data = zframe_data (zre_msg_routing_id (msg));
    size_t peerid_size = zframe_size (zre_msg_routing_id (msg));

    //  Identity must be [1] followed by 16-byte UUID
    if (peerid_size != ZUUID_LEN + 1) {
        zre_msg_destroy (&msg);
        return;
    }
    zuuid_t *uuid = zuuid_new ();
    zuuid_set (uuid, peerid_data + 1);

    //  On HELLO we may create the peer if it's unknown
    //  On other commands the peer must already exist
    zyre_peer_t *peer = (zyre_peer_t *) zhash_lookup (self->peers, zuuid_str (uuid));
    if (zre_msg_id (msg) == ZRE_MSG_HELLO) {
        if (peer) {
            //  Remove fake peers
            if (zyre_peer_ready (peer)) {
                zyre_node_remove_peer (self, peer);
                assert (!(zyre_peer_t *) zhash_lookup (self->peers, zuuid_str (uuid)));
            }
            else
            if (streq (zyre_peer_endpoint (peer), self->endpoint)) {
                //  We ignore HELLO, if peer has same endpoint as current node
                zre_msg_destroy (&msg);
                zuuid_destroy (&uuid);
                return;
            }
        }
        if (!self->secret_key) {
            peer = zyre_node_require_peer (self, uuid, zre_msg_endpoint (msg), NULL);
        } else {
            zhash_t *headers = zre_msg_headers(msg);
            char *public_key = (char *) zhash_lookup (headers, "X-PUBLICKEY");
            if(public_key) {
                assert (public_key[0] != 0);
                peer = zyre_node_require_peer (self, uuid, zre_msg_endpoint (msg), public_key);
            } else {
                if (self->verbose)
                    zsys_debug ("ignoring HELLO to avoid security downgrade, does not contain public key");
                peer = NULL;
            }
        }
        if (peer)
            zyre_peer_set_ready (peer, true);
    }
    //  Ignore command if peer isn't ready
    if (peer == NULL || !zyre_peer_ready (peer)) {
        zre_msg_destroy (&msg);
        zuuid_destroy (&uuid);
        return;
    }
    if (zyre_peer_messages_lost (peer, msg)) {
        zsys_warning ("(%s) messages lost from %s", self->name, zyre_peer_name (peer));
        zyre_node_remove_peer (self, peer);
        zre_msg_destroy (&msg);
        zuuid_destroy (&uuid);
        return;
    }
    //  Now process each command
    if (zre_msg_id (msg) == ZRE_MSG_HELLO) {
        //  Store properties from HELLO command into peer
        zyre_peer_set_name (peer, zre_msg_name (msg));
        zyre_peer_set_headers (peer, zre_msg_headers (msg));

        //  Tell the caller about the peer
        zstr_sendm (self->outbox, "ENTER");
        zstr_sendm (self->outbox, zyre_peer_identity (peer));
        zstr_sendm (self->outbox, zyre_peer_name (peer));
        if (zyre_peer_headers (peer)) {
            zframe_t *headers = zhash_pack (zyre_peer_headers (peer));
            zframe_send (&headers, self->outbox, ZFRAME_MORE);
        }
        zstr_send (self->outbox, zre_msg_endpoint (msg));

        if (self->verbose)
            zsys_info ("(%s) ENTER name=%s endpoint=%s",
                self->name, zyre_peer_name (peer), zyre_peer_endpoint (peer));

        //  Join peer to listed groups
        zlist_t *groups = zre_msg_groups (msg);
        const char *name = (const char *) zlist_first (groups);
        while (name) {
            zyre_node_join_peer_group (self, peer, name);
            name = (const char *) zlist_next (groups);
        }
        //  Now take peer's status from HELLO, after joining groups
        zyre_peer_set_status (peer, zre_msg_status (msg));
    }
    else
    if (zre_msg_id (msg) == ZRE_MSG_WHISPER) {
        //  Pass up to caller API as WHISPER event
        zstr_sendm (self->outbox, "WHISPER");
        zstr_sendm (self->outbox, zuuid_str (uuid));
        zstr_sendm (self->outbox, zyre_peer_name (peer));
        zmsg_t *content = zmsg_dup (zre_msg_content (msg));
        zmsg_send (&content, self->outbox);
    }
    else
    if (zre_msg_id (msg) == ZRE_MSG_SHOUT) {
        //  Pass up to caller as SHOUT event
        zstr_sendm (self->outbox, "SHOUT");
        zstr_sendm (self->outbox, zuuid_str (uuid));
        zstr_sendm (self->outbox, zyre_peer_name (peer));
        zstr_sendm (self->outbox, zre_msg_group (msg));
        zmsg_t *content = zmsg_dup (zre_msg_content (msg));
        zmsg_send (&content, self->outbox);
    }
    else
    if (zre_msg_id (msg) == ZRE_MSG_PING) {
        zre_msg_t *msg = zre_msg_new ();
        zre_msg_set_id (msg, ZRE_MSG_PING_OK);
        zyre_peer_send (peer, &msg);
    }
    else
    if (zre_msg_id (msg) == ZRE_MSG_JOIN) {
        zyre_group_t *group = zyre_node_join_peer_group (self, peer, zre_msg_group (msg));
        assert (zre_msg_status (msg) == zyre_peer_status (peer));
        if (zlist_exists (self->own_groups, (char *) zre_msg_group (msg))) {
            if (zyre_group_contest (zyre_node_require_peer_group (self, zre_msg_group (msg)))) {
                //  Start election if there's an active election abort it
                zyre_election_t *election = zyre_group_election (group);
                if (election) {
                    //  Discard a running election because the number of peers change
                    zyre_election_destroy (&election);
                }
                election = zyre_election_new ();
                zyre_group_set_election (group, election);

                //  Start challenge for leadership
                zyre_election_set_caw (election, strdup (zuuid_str (self->uuid)));
                zre_msg_t *election_msg = zyre_election_build_elect_msg (election);
                zre_msg_set_group (election_msg, zre_msg_group (msg));
                if (self->verbose)
                    zsys_info ("(%s) [%s] send ELECT message - %s",
                        self->name, zre_msg_group (msg), zuuid_str (self->uuid));

                zyre_group_send (group, &election_msg);
            }
        }
    }
    else
    if (zre_msg_id (msg) == ZRE_MSG_LEAVE) {
        zyre_group_t *group = zyre_node_leave_peer_group (self, peer, zre_msg_group (msg));
        assert (zre_msg_status (msg) == zyre_peer_status (peer));
        if (zlist_exists (self->own_groups, (char *) zre_msg_group (msg))) {
            zyre_peer_t *group_leader = zyre_group_leader (group);
            if (group_leader) {
                if (streq (zyre_peer_identity (group_leader), zyre_peer_identity (peer))) {
                    // If leader left do election
                    zyre_election_t *election = zyre_group_election (group);
                    if (election) {
                        //  Discard a running election because the number of peers change
                        zyre_election_destroy (&election);
                    }
                    if (zyre_group_contest (zyre_node_require_peer_group (self, zre_msg_group (msg)))) {
                        election = zyre_election_new ();
                        zyre_group_set_election (group, election);
                        //  Start challenge for leadership
                        zyre_election_set_caw (election, strdup (zuuid_str (self->uuid)));
                        zre_msg_t *election_msg = zyre_election_build_elect_msg (election);
                        zre_msg_set_group (election_msg, zre_msg_group (msg));

                        zyre_group_send (group, &election_msg);
                    }
                }
            }
        }
    }
    else
    if (zre_msg_id (msg) == ZRE_MSG_ELECT) {
        zyre_group_t *group = zyre_node_require_peer_group (self, zre_msg_group (msg));
        zyre_election_t *election = zyre_group_require_election (group);
        const char *r = zre_msg_challenger_id (msg);

        if (zyre_election_challenger_superior (election, r)) {
            //  Initiate or re-initiate leader election
            zyre_election_reset (election);
            zyre_election_set_caw (election, strdup (r));
            zyre_election_set_father (election, peer);

            zre_msg_t *election_msg = zyre_election_build_elect_msg (election);
            zre_msg_set_group (election_msg, zre_msg_group (msg));

            //  Send election message to all neighbors but father but father
            zlist_t *group_peers = zyre_group_peers (group);
            char *group_peer = (char *) zlist_first (group_peers);
            while (group_peer) {
                if (strneq (group_peer, zyre_peer_identity (peer))) {
                    zyre_peer_t *receiver = (zyre_peer_t *) zhash_lookup (self->peers, group_peer);
                    zre_msg_t *election_msg_dup = zre_msg_dup (election_msg);
                    zyre_peer_send (receiver, &election_msg_dup);
                }
                group_peer = (char *) zlist_next (group_peers);
            }
            zlist_destroy (&group_peers);
            zre_msg_destroy (&election_msg);
            if (self->verbose)
                zsys_info ("(%s) [%s] support challenger - %s",
                           self->name, zre_msg_group (msg), r);
        }

        //  Support the challenger by participating in its current active wave
        if (zyre_election_supporting_challenger (election, r)) {
            zyre_election_increment_erec (election);
            if (zyre_election_erec_complete (election, group)) {
                if (streq (zyre_election_caw (election), zuuid_str (self->uuid))) {
                    zre_msg_t *leader_msg = zyre_election_build_leader_msg (election);
                    zre_msg_set_group (leader_msg, zre_msg_group (msg));

                    //  Send leader message to all neighbors
                    zyre_group_send (group, &leader_msg);
                    if (self->verbose)
                        zsys_info ("(%s) [%s] LEADER decision - %s",
                                   self->name, zre_msg_group (msg), zuuid_str (self->uuid));
                }
                else {
                    zre_msg_t *election_msg = zyre_election_build_elect_msg (election);
                    zre_msg_set_group (election_msg, zre_msg_group (msg));

                    //  Send election message to father
                    zyre_peer_send (zyre_election_father (election), &election_msg);
                    if (self->verbose)
                        zsys_info ("(%s) [%s] Echo wave to father - %s",
                                   self->name, zre_msg_group (msg), r);
                }
            }
        }
        //  If challenger is unworthy the message is ignored!
    }
    else
    if (zre_msg_id (msg) == ZRE_MSG_LEADER) {
        zyre_group_t *group = zyre_node_require_peer_group (self, zre_msg_group (msg));
        zyre_election_t *election = zyre_group_require_election (group);
        assert (election);
        const char *r = zre_msg_leader_id (msg);

        // Only propagate if not leader
        if (strneq (zuuid_str (self->uuid), r) && !zyre_election_lrec_started (election)) {
            zre_msg_t *leader_msg = zyre_election_build_leader_msg (election);
            zre_msg_set_group (leader_msg, zre_msg_group (msg));

            //  Send leader message to all neighbors
            zyre_group_send (group, &leader_msg);
            if (self->verbose)
                zsys_info ("(%s) [%s] Propagate LEADER - %s\n",
                           self->name, zre_msg_group (msg), zuuid_str (self->uuid));
        }
        zyre_election_increment_lrec (election);
        zyre_election_set_leader (election, strdup (r));
        if (self->verbose)
            zsys_info ("(%s) [%s] Received LEADER - %s\n",
                       self->name, zre_msg_group (msg), zuuid_str (self->uuid));

        // Check if election is finished
        if (zyre_election_lrec_complete (election, group)) {
            if (streq (zyre_election_leader (election), zuuid_str (self->uuid))) {
                //  This node is leader
                zyre_node_leader_peer_group (self,
                                             zuuid_str (self->uuid),
                                             self->name,
                                             zre_msg_group (msg));
            }
            else {
                //  Peer is leader
                zyre_peer_t *leader_peer = (zyre_peer_t *) zhash_lookup (self->peers, zyre_election_leader (election));
                zyre_group_set_leader (group, leader_peer);
                assert (leader_peer);
                zyre_node_leader_peer_group (self,
                                             zyre_peer_identity (leader_peer),
                                             zyre_peer_name (leader_peer),
                                             zre_msg_group (msg));
            }

            if (self->verbose)
                zsys_info ("(%s) [%s] Election finished %s, %s!\n",
                           self->name, zre_msg_group (msg), zuuid_str (self->uuid),
                           streq (zyre_election_leader (election), zuuid_str (self->uuid))? "LEADER": "FOLLOWER");

            zyre_election_destroy (&election);
            zyre_group_set_election (group, NULL);
        }
    }
    zuuid_destroy (&uuid);
    zre_msg_destroy (&msg);

    //  Activity from peer resets peer timers
    zyre_peer_refresh (peer, self->evasive_timeout, self->expired_timeout);
}

//  Handle beacon data

static void
zyre_node_recv_beacon (zyre_node_t *self)
{
    //  Get IP address and beacon of peer
    char *ipaddress = zstr_recv (self->beacon);
    zframe_t *frame = zframe_recv (self->beacon);
    if (ipaddress == NULL) {
        if(frame != NULL)
            zframe_destroy (&frame);
        return;                 //  Interrupted
    }

    //  Ignore anything that isn't a valid beacon
    beacon_t beacon;
    memset (&beacon, 0, sizeof (beacon_t));
    if (zframe_size (frame) == BEACON_SIZE_V2 ||
            zframe_size (frame) == BEACON_SIZE_V3)
        memcpy (&beacon, zframe_data (frame), zframe_size (frame));
    zframe_destroy (&frame);
    if (beacon.version != self->beacon_version) {
        zstr_free (&ipaddress);
        if (self->verbose)
            zsys_debug ("tossing beacon, version mis-match");
        return;
    }

//     beacon missing public key when we're in secure mode
    if (self->secret_key && (beacon.public_key[0] == 0)) {
        zstr_free (&ipaddress);
        //         toss it to avoid down-grade attacks
        if (self->verbose)
            zsys_debug ("tossing beacon to avoid security downgrade, does not contain public key...");

        return;
    }

    zuuid_t *uuid = zuuid_new ();
    zuuid_set (uuid, beacon.uuid);
    if (beacon.port) {
        char endpoint [NI_MAXHOST];
        sprintf (endpoint, "tcp://%s:%d", ipaddress, ntohs (beacon.port));

        if (beacon.version == BEACON_VERSION_V3) {
            char public_key [41];
            zmq_z85_encode(public_key, beacon.public_key, 32);
            zyre_node_require_peer(self, uuid, endpoint, public_key);
        }
        else
            zyre_node_require_peer(self, uuid, endpoint, NULL);
    }
    else {
        //  Zero port means peer is going away; remove it if
        //  we had any knowledge of it already
        zyre_peer_t *peer = (zyre_peer_t *) zhash_lookup (
            self->peers, zuuid_str (uuid));
        if (peer)
            zyre_node_remove_peer (self, peer);
    }
    zuuid_destroy (&uuid);
    zstr_free (&ipaddress);
}


//  Handle gossip data

static void
zyre_node_recv_gossip (zyre_node_t *self)
{
    //  Get IP address and beacon of peer
    char *command = NULL, *uuidstr, *endpoint;
    zstr_recvx (self->gossip, &command, &uuidstr, &endpoint, NULL);
    if (command == NULL)
        return;                 //  Interrupted

    //  Any replies except DELIVER would signify an internal error; these
    //  messages come from zgossip, not an external source
    assert (streq (command, "DELIVER"));

    // extract public key from published service
    // tcp://endpoint:NNNN|asdfasdfasdfasdfasdf
    // do this before we check the endpoint o/w it will not match..
    char *pipe = strchr(endpoint, '|');
    const char *public_key = NULL;
    if(pipe != NULL) {
        *pipe = '\0';
        public_key = pipe+1;
    }

    //  Require peer, if it's not us
    // check to see if the endpoint and advertised endpoint are the same (NAT checking)
    if ((strneq (endpoint, self->endpoint))
        && (!self->advertised_endpoint || (strneq (endpoint, self->advertised_endpoint)))) {
        zuuid_t *uuid = zuuid_new ();
        zuuid_set_str (uuid, uuidstr);
        zyre_node_require_peer (self, uuid, endpoint, public_key);
        zuuid_destroy (&uuid);
    }
    zstr_free (&command);
    zstr_free (&uuidstr);
    zstr_free (&endpoint);
}


//  We do this once a second:
//  - if peer has gone quiet, send TCP ping and emit EVASIVE event
//  - if peer has disappeared, expire it

static int
zyre_node_ping_peer (const char *key, void *item, void *argument)
{
    zyre_peer_t *peer = (zyre_peer_t *) item;
    zyre_node_t *self = (zyre_node_t *) argument;
    if (!peer)
        return 0;
    if (zclock_mono () >= zyre_peer_expired_at (peer)) {
        if (self->verbose)
            zsys_info ("(%s) peer expired name=%s endpoint=%s",
                self->name, zyre_peer_name (peer), zyre_peer_endpoint (peer));
        zyre_node_remove_peer (self, peer);
    }
    else
    if (zclock_mono () >= zyre_peer_evasive_at (peer)) {
        //  If peer is being evasive, force a TCP ping.
        //  TODO: do this only once for a peer in this state;
        //  it would be nicer to use a proper state machine
        //  for peer management.
        if (self->verbose)
            zsys_info ("(%s) peer seems dead/slow name=%s endpoint=%s",
                self->name, zyre_peer_name (peer), zyre_peer_endpoint (peer));
        zre_msg_t *msg = zre_msg_new ();
        zre_msg_set_id (msg, ZRE_MSG_PING);
        zyre_peer_send (peer, &msg);
        zre_msg_destroy (&msg);
        // Inform the calling application this peer is being evasive
        zstr_sendm (self->outbox, "EVASIVE");
        zstr_sendm (self->outbox, zyre_peer_identity (peer));
        zstr_send (self->outbox, zyre_peer_name (peer));
    }
    return 0;
}


//  --------------------------------------------------------------------------
//  This is the actor that runs a single node; it uses one thread, creates
//  a zyre_node object at start and destroys that when finishing.

void
zyre_node_actor (zsock_t *pipe, void *args)
{
    //  Create node instance to pass around
    zyre_node_t *self = zyre_node_new (pipe, args);
    if (!self)                  //  Interrupted
        return;

    //  Signal actor successfully initialized
    zsock_signal (self->pipe, 0);

    //  Loop until the agent is terminated one way or another
    int64_t reap_at = zclock_mono () + REAP_INTERVAL;
    while (!self->terminated) {

        // Start beacon as soon as we can
        if (self->beacon && self->port <= 0) {
            //  Our hostname is provided by zbeacon
            zsock_send(self->beacon, "si", "CONFIGURE", self->beacon_port);
            char *hostname = zstr_recv(self->beacon);

            // Is UDP broadcast interface available?
            if (!streq(hostname, "")) {
                const char *iface = zsys_interface ();
                if (zsys_ipv6() && iface && !streq (iface, "") && !streq (iface, "*") && !streq (zsys_ipv6_address (), "")) {
                    self->port = zsock_bind(self->inbox, "tcp://%s%%%s:%s", zsys_ipv6_address (),
                        iface, self->ephemeral_port ? self->ephemeral_port : "*");
                } else
                    self->port = zsock_bind(self->inbox, "tcp://%s:%s", hostname,
                        self->ephemeral_port ? self->ephemeral_port : "*");

                if (self->port > 0) {
                    assert(!self->endpoint);   //  If caller set this, we'd be using gossip
                    if (streq(zsys_interface(), "*")) {
                        char *hostname = zsys_hostname();
                        self->endpoint = zsys_sprintf("tcp://%s:%d", hostname, self->port);
                        zstr_free(&hostname);
                    }
                    else {
                        self->endpoint = strdup(zsock_endpoint(self->inbox));
                    }

                    //  Set broadcast/listen beacon
                    beacon_t beacon;
                    beacon.protocol[0] = 'Z';
                    beacon.protocol[1] = 'R';
                    beacon.protocol[2] = 'E';
                    beacon.version = self->beacon_version;
                    beacon.port = htons(self->port);
                    zuuid_export(self->uuid, beacon.uuid);
                    // SEND
                    if (self->public_key) {
                        zmq_z85_decode(beacon.public_key, self->public_key);
                    }
                    zsock_send(self->beacon, "sbi", "PUBLISH",
                        (byte *)&beacon, BEACON_SIZE(beacon), self->interval);
                    zsock_send(self->beacon, "sb", "SUBSCRIBE", (byte *) "ZRE", 3);
                    zpoller_add(self->poller, self->beacon);

                    //  Start polling on inbox
                    zpoller_add(self->poller, self->inbox);
                }
            }
            zstr_free(&hostname);
        }

        int timeout = (int) (reap_at - zclock_mono ());
        if (timeout > REAP_INTERVAL)
            timeout = REAP_INTERVAL;
        else
        if (timeout < 0)
            timeout = 0;

        zsock_t *which = (zsock_t *) zpoller_wait (self->poller, timeout);
        if (which == self->pipe)
            zyre_node_recv_api (self);
        else
        if (which == self->inbox)
            zyre_node_recv_peer (self);
        else
        if (self->beacon
        && (void *) which == self->beacon)
            zyre_node_recv_beacon (self);
        else
        if (self->gossip
        && (zactor_t *) which == self->gossip)
            zyre_node_recv_gossip (self);
        else
        if (zpoller_terminated (self->poller))
            break;          //  Interrupted, check before expired
        else
        if (zpoller_expired (self->poller)) {
            if (zclock_mono () >= reap_at) {
                void *item;
                reap_at = zclock_mono () + REAP_INTERVAL;
                //  Ping all peers and reap any expired ones
                for (item = zhash_first (self->peers); item != NULL;
                        item = zhash_next (self->peers))
                    zyre_node_ping_peer (zhash_cursor (self->peers), item, self);
            }
        }
    }
    zyre_node_destroy (&self);
}


//  --------------------------------------------------------------------------
//  Self test of this class

void
zyre_node_test (bool verbose)
{
    printf (" * zyre_node: ");
    zsock_t *pipe = zsock_new (ZMQ_PAIR);
    zsock_t *outbox = zsock_new (ZMQ_PAIR);
    zyre_node_t *node = zyre_node_new (pipe, outbox);
    zyre_node_destroy (&node);
    zsock_destroy (&pipe);
    //  Node takes ownership of outbox and destroys it
    printf ("OK\n");
}
