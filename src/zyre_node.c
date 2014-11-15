/*  =========================================================================
    zyre_node - node on a ZRE network

    -------------------------------------------------------------------------
    Copyright (c) 1991-2014 iMatix Corporation <www.imatix.com>
    Copyright other contributors as noted in the AUTHORS file.

    This file is part of Zyre, an open-source framework for proximity-based
    peer-to-peer applications -- See http://zyre.org.

    This is free software; you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 3 of the License, or (at
    your option) any later version.

    This software is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this program. If not, see
    <http://www.gnu.org/licenses/>.
    =========================================================================
*/

#include "../include/zyre.h"
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
    int beacon_port;            //  Beacon port number
    size_t interval;            //  Beacon interval
    zpoller_t *poller;          //  Socket poller
    zactor_t *beacon;           //  Beacon actor
    zuuid_t *uuid;              //  Our UUID as object
    zsock_t *inbox;             //  Our inbox socket (ROUTER)
    char *name;                 //  Our public name
    char *endpoint;             //  Our public endpoint
    int port;                   //  Our inbox port, if any
    byte status;                //  Our own change counter
    zhash_t *peers;             //  Hash of known peers, fast lookup
    zhash_t *peer_groups;       //  Groups that our peers are in
    zhash_t *own_groups;        //  Groups that we are in
    zhash_t *headers;           //  Our header values
    zactor_t *gossip;           //  Gossip discovery service, if any
    char *gossip_bind;          //  Gossip bind endpoint, if any
    char *gossip_connect;       //  Gossip connect endpoint, if any
};

//  Beacon frame has this format:
//
//  Z R E       3 bytes
//  version     1 byte, %x01
//  UUID        16 bytes
//  port        2 bytes in network order

#define BEACON_VERSION 0x01

typedef struct {
    byte protocol [3];
    byte version;
    byte uuid [ZUUID_LEN];
    uint16_t port;
} beacon_t;


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
    self->interval = 0;         //  Use default
    self->uuid = zuuid_new ();
    self->peers = zhash_new ();
    self->peer_groups = zhash_new ();
    self->own_groups = zhash_new ();
    self->headers = zhash_new ();
    zhash_autofree (self->headers);

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
        zhash_destroy (&self->own_groups);
        zhash_destroy (&self->headers);
        zsock_destroy (&self->inbox);
        zsock_destroy (&self->outbox);
        zactor_destroy (&self->beacon);
        zactor_destroy (&self->gossip);
        zstr_free (&self->endpoint);
        zstr_free (&self->gossip_bind);
        zstr_free (&self->gossip_connect);
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
        assert (self->gossip);
    }
}


//  Start node, return 0 if OK, 1 if not possible

static int
zyre_node_start (zyre_node_t *self)
{
    if (self->beacon_port) {
        //  Start beacon discovery
        //  ------------------------------------------------------------------
        assert (!self->beacon);
        self->beacon = zactor_new (zbeacon, NULL);
        if (!self->beacon)
            return 1;               //  Not possible to start beacon
        zsock_send (self->beacon, "si", "CONFIGURE", self->beacon_port);

        //  Our hostname is provided by zbeacon
        char *hostname = zstr_recv (self->beacon);
        self->port = zsock_bind (self->inbox, "tcp://%s:*", hostname);
        zstr_free (&hostname);
        assert (self->port > 0);    //  Die on bad interface or port exhaustion
        assert (!self->endpoint);   //  If caller set this, we'd be using gossip
        self->endpoint = strdup (zsock_endpoint (self->inbox));

        //  Set broadcast/listen beacon
        beacon_t beacon;
        beacon.protocol [0] = 'Z';
        beacon.protocol [1] = 'R';
        beacon.protocol [2] = 'E';
        beacon.version = BEACON_VERSION;
        beacon.port = htons (self->port);
        zuuid_export (self->uuid, beacon.uuid);
        zsock_send (self->beacon, "sbi", "PUBLISH",
            (byte *) &beacon, sizeof (beacon_t), self->interval);
        zsock_send (self->beacon, "sb", "SUBSCRIBE", (byte *) "ZRE", 3);
        zpoller_add (self->poller, self->beacon);
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
        zstr_sendx (self->gossip, "PUBLISH", zuuid_str (self->uuid), self->endpoint, NULL);
        //  Start polling on zgossip
        zpoller_add (self->poller, self->gossip);
    }
    //  Start polling on inbox
    zpoller_add (self->poller, self->inbox);
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
        beacon.version = BEACON_VERSION;
        beacon.port = 0;            //  Zero means we're stopping
        zuuid_export (self->uuid, beacon.uuid);
        zsock_send (self->beacon, "sbi", "PUBLISH",
            (byte *) &beacon, sizeof (beacon_t), self->interval);
        zclock_sleep (1);           //  Allow 1 msec for beacon to go out
        zpoller_remove (self->poller, self->beacon);
        zactor_destroy (&self->beacon);
    }
    //  Stop polling on inbox
    zpoller_remove (self->poller, self->inbox);
    return 0;
}


//  Send message to one peer; called via zhash_foreach

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
zyre_node_log_item (const char *key, void *item, void *argument)
{
    zsys_info ("   - %s", key);
    return 0;
}

//  Prints node information

static void
zyre_node_dump (zyre_node_t *self)
{
    zsys_info ("zyre_node: dump state");
    zsys_info (" - name=%s uuid=%s", self->name, zuuid_str (self->uuid));
    zsys_info (" - endpoint=%s", self->endpoint);
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
    zhash_foreach (self->headers, (zhash_foreach_fn *) zyre_node_log_item, self);
    
    zsys_info (" - peers=%zu:", zhash_size (self->peers));
    zhash_foreach (self->peers, (zhash_foreach_fn *) zyre_node_log_item, self);

    zsys_info (" - groups=%zu:", zhash_size (self->own_groups));
    zhash_foreach (self->own_groups, (zhash_foreach_fn *) zyre_node_log_item, self);
}


//  Here we handle the different control messages from the front-end

static void
zyre_node_recv_api (zyre_node_t *self)
{
    //  Get the whole message off the pipe in one go
    zmsg_t *request = zmsg_recv (self->pipe);
    if (!request)
        return;                 //  Interrupted
        
    char *command = zmsg_popstr (request);
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
    if (streq (command, "SET INTERVAL")) {
        char *value = zmsg_popstr (request);
        self->interval = atol (value);
        zstr_free (&value);
    }
    else
    if (streq (command, "SET ENDPOINT")) {
        zyre_node_gossip_start (self);
        char *endpoint = zmsg_popstr (request);
        if (zsock_bind (self->inbox, "%s", endpoint) != -1) {
            zstr_free (&self->endpoint);
            self->endpoint = endpoint;
            zsock_signal (self->pipe, 0);
        }
        else {
            zstr_free (&endpoint);
            zsock_signal (self->pipe, 1);
        }
    }
    else
    if (streq (command, "GOSSIP BIND")) {
        zyre_node_gossip_start (self);
        zstr_free (&self->gossip_bind);
        self->gossip_bind = zmsg_popstr (request);
        zstr_sendx (self->gossip, "BIND", self->gossip_bind, NULL);
    }
    else
    if (streq (command, "GOSSIP CONNECT")) {
        zyre_node_gossip_start (self);
        zstr_free (&self->gossip_connect);
        self->gossip_connect = zmsg_popstr (request);
        zstr_sendx (self->gossip, "CONNECT", self->gossip_connect, NULL);
    }
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
            zre_msg_t *msg = zre_msg_new (ZRE_MSG_WHISPER);
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
            zre_msg_t *msg = zre_msg_new (ZRE_MSG_SHOUT);
            zre_msg_set_group (msg, name);
            zre_msg_set_content (msg, &request);
            zyre_group_send (group, &msg);
        }
        zstr_free (&name);
    }
    else
    if (streq (command, "JOIN")) {
        char *name = zmsg_popstr (request);
        zyre_group_t *group = (zyre_group_t *) zhash_lookup (self->own_groups, name);
        if (!group) {
            //  Only send if we're not already in group
            group = zyre_group_new (name, self->own_groups);
            zre_msg_t *msg = zre_msg_new (ZRE_MSG_JOIN);
            zre_msg_set_group (msg, name);
            //  Update status before sending command
            zre_msg_set_status (msg, ++(self->status));
            zhash_foreach (self->peers, (zhash_foreach_fn *) zyre_node_send_peer, msg);
            zre_msg_destroy (&msg);
            if (self->verbose)
                zsys_info ("(%s) JOIN group=%s", self->name, name);
        }
        zstr_free (&name);
    }
    else
    if (streq (command, "LEAVE")) {
        char *name = zmsg_popstr (request);
        zyre_group_t *group = (zyre_group_t *) zhash_lookup (self->own_groups, name);
        if (group) {
            //  Only send if we are actually in group
            zre_msg_t *msg = zre_msg_new (ZRE_MSG_LEAVE);
            zre_msg_set_group (msg, name);
            //  Update status before sending command
            zre_msg_set_status (msg, ++(self->status));
            zhash_foreach (self->peers, (zhash_foreach_fn *) zyre_node_send_peer, msg);
            zre_msg_destroy (&msg);
            zhash_delete (self->own_groups, name);
            if (self->verbose)
                zsys_info ("(%s) LEAVE group=%s", self->name, name);
        }
        zstr_free (&name);
    }
    else
    if (streq (command, "PEERS"))
        zsock_send (self->pipe, "p", zhash_keys (self->peers));
    else
    if (streq (command, "PEER ENDPOINT")) {
        char *uuid = zmsg_popstr (request);
        zyre_peer_t *peer = (zyre_peer_t *) zhash_lookup (self->peers, (void *) uuid);
        assert (peer);
        zsock_send (self->pipe, "s", zyre_peer_endpoint (peer));
        zstr_free (&uuid);
    }
    else
    if (streq (command, "PEER HEADER")) {
        char *uuid = zmsg_popstr (request);
        char *key = zmsg_popstr (request);
        zyre_peer_t *peer = (zyre_peer_t *) zhash_lookup (self->peers, (void *) uuid);
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
        zsock_send (self->pipe, "p", zhash_keys (self->own_groups));
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
zyre_node_require_peer (zyre_node_t *self, zuuid_t *uuid, const char *endpoint)
{
    assert (self);
    assert (endpoint);

    zyre_peer_t *peer = (zyre_peer_t *) zhash_lookup (self->peers, zuuid_str (uuid));
    if (!peer) {
        //  Purge any previous peer on same endpoint
        zhash_foreach (self->peers, (zhash_foreach_fn *) zyre_node_purge_peer, (char *) endpoint);

        peer = zyre_peer_new (self->peers, uuid);
        assert (peer);
        zyre_peer_set_origin (peer, self->name);
        zyre_peer_set_verbose (peer, self->verbose);
        zyre_peer_connect (peer, self->uuid, endpoint);

        //  Handshake discovery by sending HELLO as first message
        zlist_t *groups = zhash_keys (self->own_groups);
        zhash_t *headers = zhash_dup (self->headers);
        zre_msg_t *msg = zre_msg_new (ZRE_MSG_HELLO);
        zre_msg_set_endpoint (msg, self->endpoint);
        zre_msg_set_groups (msg, &groups);
        zre_msg_set_status (msg, self->status);
        zre_msg_set_name (msg, self->name);
        zre_msg_set_headers (msg, &headers);
        zyre_peer_send (peer, &msg);
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
    //  Tell the calling application the peer has gone
    zstr_sendm (self->outbox, "EXIT");
    zstr_sendm (self->outbox, zyre_peer_identity (peer));
    zstr_send (self->outbox, zyre_peer_name (peer));

    if (self->verbose)
        zsys_info ("(%s) EXIT name=%s endpoint=%s",
                self->name, zyre_peer_name (peer), zyre_peer_endpoint (peer));
    
    //  Remove peer from any groups we've got it in
    zhash_foreach (self->peer_groups, (zhash_foreach_fn *) zyre_node_delete_peer, peer);
    //  To destroy peer, we remove from peers hash table
    zhash_delete (self->peers, zyre_peer_identity (peer));
}


//  Find or create group via its name

static zyre_group_t *
zyre_node_require_peer_group (zyre_node_t *self, const char *name)
{
    zyre_group_t *group = (zyre_group_t *) zhash_lookup (self->peer_groups, (void *) name);
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

//  Here we handle messages coming from other peers

static void
zyre_node_recv_peer (zyre_node_t *self)
{
    //  Router socket tells us the identity of this peer
    zre_msg_t *msg = zre_msg_recv (self->inbox);
    if (!msg)
        return;                 //  Interrupted

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
        peer = zyre_node_require_peer (self, uuid, zre_msg_endpoint (msg));
        assert (peer);
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
        zframe_t *headers = zhash_pack (zyre_peer_headers (peer));
        zframe_send (&headers, self->outbox, ZFRAME_MORE);
        zstr_send (self->outbox, zre_msg_endpoint (msg));

        if (self->verbose)
            zsys_info ("(%s) ENTER name=%s endpoint=%s",
                self->name, zyre_peer_name (peer), zyre_peer_endpoint (peer));
        
        //  Join peer to listed groups
        const char *name = zre_msg_groups_first (msg);
        while (name) {
            zyre_node_join_peer_group (self, peer, name);
            name = zre_msg_groups_next (msg);
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
        zre_msg_t *msg = zre_msg_new (ZRE_MSG_PING_OK);
        zyre_peer_send (peer, &msg);
    }
    else
    if (zre_msg_id (msg) == ZRE_MSG_JOIN) {
        zyre_node_join_peer_group (self, peer, zre_msg_group (msg));
        assert (zre_msg_status (msg) == zyre_peer_status (peer));
    }
    else
    if (zre_msg_id (msg) == ZRE_MSG_LEAVE) {
        zyre_node_leave_peer_group (self, peer, zre_msg_group (msg));
        assert (zre_msg_status (msg) == zyre_peer_status (peer));
    }
    zuuid_destroy (&uuid);
    zre_msg_destroy (&msg);
    
    //  Activity from peer resets peer timers
    zyre_peer_refresh (peer);
}

//  Handle beacon data

static void
zyre_node_recv_beacon (zyre_node_t *self)
{
    //  Get IP address and beacon of peer
    char *ipaddress = zstr_recv (self->beacon);
    zframe_t *frame = zframe_recv (self->beacon);
    if (ipaddress == NULL)
        return;                 //  Interrupted

    //  Ignore anything that isn't a valid beacon
    beacon_t beacon = { { 0 } };
    if (zframe_size (frame) == sizeof (beacon_t))
        memcpy (&beacon, zframe_data (frame), zframe_size (frame));
    zframe_destroy (&frame);
    if (beacon.version != BEACON_VERSION)
        return;                 //  Garbage beacon, ignore it

    zuuid_t *uuid = zuuid_new ();
    zuuid_set (uuid, beacon.uuid);
    if (beacon.port) {
        char endpoint [30];
        sprintf (endpoint, "tcp://%s:%d", ipaddress, ntohs (beacon.port));
        zyre_peer_t *peer = zyre_node_require_peer (self, uuid, endpoint);
        zyre_peer_refresh (peer);
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
    //  Require peer, if it's not us
    if (strneq (endpoint, self->endpoint)) {
        zuuid_t *uuid = zuuid_new ();
        zuuid_set_str (uuid, uuidstr);
        zyre_node_require_peer (self, uuid, endpoint);
        zuuid_destroy (&uuid);
    }
    zstr_free (&command);
    zstr_free (&uuidstr);
    zstr_free (&endpoint);
}


//  We do this once a second:
//  - if peer has gone quiet, send TCP ping
//  - if peer has disappeared, expire it

static int
zyre_node_ping_peer (const char *key, void *item, void *argument)
{
    zyre_peer_t *peer = (zyre_peer_t *) item;
    zyre_node_t *self = (zyre_node_t *) argument;
    if (zclock_time () >= zyre_peer_expired_at (peer)) {
        if (self->verbose)
            zsys_info ("(%s) peer expired name=%s endpoint=%s",
                self->name, zyre_peer_name (peer), zyre_peer_endpoint (peer));
        zyre_node_remove_peer (self, peer);
    }
    else
    if (zclock_time () >= zyre_peer_evasive_at (peer)) {
        //  If peer is being evasive, force a TCP ping.
        //  TODO: do this only once for a peer in this state;
        //  it would be nicer to use a proper state machine
        //  for peer management.
        if (self->verbose)
            zsys_info ("(%s) peer seems dead/slow name=%s endpoint=%s",
                self->name, zyre_peer_name (peer), zyre_peer_endpoint (peer));
        zre_msg_t *msg = zre_msg_new (ZRE_MSG_PING);
        zyre_peer_send (peer, &msg);
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
    int64_t reap_at = zclock_time () + REAP_INTERVAL;
    while (!self->terminated) {
        int timeout = (int) (reap_at - zclock_time ());
        assert (timeout <= REAP_INTERVAL);
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
        if (zpoller_expired (self->poller)) {
            if (zclock_time () >= reap_at) {
                reap_at = zclock_time () + REAP_INTERVAL;
                //  Ping all peers and reap any expired ones
                zhash_foreach (self->peers, (zhash_foreach_fn *) zyre_node_ping_peer, self);
            }
        }
        else
        if (zpoller_terminated (self->poller))
            break;          //  Interrupted
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
