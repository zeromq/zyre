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

#include "zyre_classes.h"

//  ---------------------------------------------------------------------
//  Structure of our class

struct _zyre_node_t {
    zctx_t *ctx;                //  CZMQ context
    void *pipe;                 //  Pipe back to application
    bool terminated;            //  API shut us down
    bool verbose;               //  Log all traffic
    int beacon_port;            //  Beacon port number
    size_t interval;            //  Beacon interval
    zpoller_t *poller;          //  Socket poller
    zbeacon_t *beacon;          //  Beacon object
    zyre_log_t *log;            //  Log object
    zuuid_t *uuid;              //  Our UUID as object
    void *inbox;                //  Our inbox socket (ROUTER)
    char *host;                 //  Our host IP address
    int port;                   //  Our inbox port number
    byte status;                //  Our own change counter
    zhash_t *peers;             //  Hash of known peers, fast lookup
    zhash_t *peer_groups;       //  Groups that our peers are in
    zhash_t *own_groups;        //  Groups that we are in
    zhash_t *headers;           //  Our header values
};


//  ---------------------------------------------------------------------
//  Constructor

static zyre_node_t *
zyre_node_new (zctx_t *ctx, void *pipe)
{
    zyre_node_t *self = (zyre_node_t *) zmalloc (sizeof (zyre_node_t));
    self->ctx = ctx;
    self->pipe = pipe;
    self->inbox = zsocket_new (ctx, ZMQ_ROUTER);
    if (self->inbox == NULL) {
        free (self);
        return NULL;            //  Interrupted 0MQ call
    }
    self->port = zsocket_bind (self->inbox, "tcp://*:*");
    if (self->port < 0) {
        zsocket_destroy (self->ctx, self->inbox);
        free (self);
        return NULL;            //  Interrupted 0MQ call
    }
    self->poller = zpoller_new (self->pipe, self->inbox, NULL);
    self->beacon_port = ZRE_DISCOVERY_PORT;
    self->interval = 0;         //  Use default
    self->uuid = zuuid_new ();
    self->peers = zhash_new ();
    self->peer_groups = zhash_new ();
    self->own_groups = zhash_new ();
    self->headers = zhash_new ();
    zhash_autofree (self->headers);

    return self;
}


//  ---------------------------------------------------------------------
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
        zsocket_destroy (self->ctx, self->inbox);
        zbeacon_destroy (&self->beacon);
        zyre_log_destroy (&self->log);
        free (self);
        *self_p = NULL;
    }
}

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


static void
zyre_node_start (zyre_node_t *self)
{
    assert (!self->beacon);
    self->beacon = zbeacon_new (self->ctx, self->beacon_port);
    if (self->interval)
        zbeacon_set_interval (self->beacon, self->interval);
    zpoller_add (self->poller, zbeacon_socket (self->beacon));
    
    //  Set broadcast/listen beacon
    beacon_t beacon;
    beacon.protocol [0] = 'Z';
    beacon.protocol [1] = 'R';
    beacon.protocol [2] = 'E';
    beacon.version = BEACON_VERSION;
    beacon.port = htons (self->port);
    zuuid_export (self->uuid, beacon.uuid);
    zbeacon_noecho (self->beacon);
    zbeacon_publish (self->beacon, (byte *) &beacon, sizeof (beacon_t));
    zbeacon_subscribe (self->beacon, (byte *) "ZRE", 3);

    //  Our own host endpoint is provided by the beacon
    self->host = zbeacon_hostname (self->beacon);

    //  Set up log instance
    char sender [30];         //  ipaddress:port endpoint
    sprintf (sender, "%s:%d", self->host, self->port);
    self->log = zyre_log_new (self->ctx, sender);
}


static void
zyre_node_stop (zyre_node_t *self)
{
    //  Set broadcast/listen beacon
    beacon_t beacon;
    beacon.protocol [0] = 'Z';
    beacon.protocol [1] = 'R';
    beacon.protocol [2] = 'E';
    beacon.version = BEACON_VERSION;
    beacon.port = 0;            //  Zero means we're stopping
    zuuid_export (self->uuid, beacon.uuid);
    zbeacon_publish (self->beacon, (byte *) &beacon, sizeof (beacon_t));
    zclock_sleep (1);           //  Allow 1 msec for beacon to go out
    
    zbeacon_destroy (&self->beacon);
    zpoller_destroy (&self->poller);
    self->poller = zpoller_new (self->pipe, self->inbox, NULL);
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


//  Here we handle the different control messages from the front-end

static int
zyre_node_recv_api (zyre_node_t *self)
{
    //  Get the whole message off the pipe in one go
    zmsg_t *request = zmsg_recv (self->pipe);
    char *command = zmsg_popstr (request);
    if (!command)
        return -1;                  //  Interrupted

    if (streq (command, "SET")) {
        char *name = zmsg_popstr (request);
        char *value = zmsg_popstr (request);
        zhash_update (self->headers, name, value);
        zstr_free (&name);
        zstr_free (&value);
    }
    else
    if (streq (command, "VERBOSE")) {
        self->verbose = true;
        zsocket_signal (self->pipe);
    }
    else
    if (streq (command, "PORT")) {
        char *value = zmsg_popstr (request);
        self->beacon_port = atoi (value);
        zstr_free (&value);
    }
    else
    if (streq (command, "INTERVAL")) {
        char *value = zmsg_popstr (request);
        self->interval = atol (value);
        zstr_free (&value);
    }
    else
    if (streq (command, "START")) {
        zyre_node_start (self);
        zsocket_signal (self->pipe);
    }
    else
    if (streq (command, "STOP")) {
        zyre_node_stop (self);
        zsocket_signal (self->pipe);
    }
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
            zhash_foreach (self->peers, zyre_node_send_peer, msg);
            zre_msg_destroy (&msg);
            zyre_log_info (self->log, ZRE_LOG_MSG_EVENT_JOIN, NULL, name);
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
            zhash_foreach (self->peers, zyre_node_send_peer, msg);
            zre_msg_destroy (&msg);
            zhash_delete (self->own_groups, name);
            zyre_log_info (self->log, ZRE_LOG_MSG_EVENT_LEAVE, NULL, name);
        }
        zstr_free (&name);
    }
    else
    if (streq (command, "TERMINATE")) {
        self->terminated = true;
        zsocket_signal (self->pipe);
    }
    else {
        printf ("E: invalid command '%s'\n", command);
        assert (false);
    }
    zstr_free (&command);
    zmsg_destroy (&request);
    return 0;
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
zyre_node_require_peer (zyre_node_t *self, zuuid_t *uuid, const char *address, uint16_t port)
{
    zyre_peer_t *peer = (zyre_peer_t *) zhash_lookup (self->peers, zuuid_str (uuid));
    if (!peer) {
        //  Purge any previous peer on same endpoint
        char endpoint [100];
        snprintf (endpoint, 100, "tcp://%s:%hu", address, port);
        zhash_foreach (self->peers, zyre_node_purge_peer, endpoint);

        peer = zyre_peer_new (self->ctx, self->peers, uuid);
        zyre_peer_connect (peer, self->uuid, endpoint);
        if (self->verbose)
            zyre_peer_set_log (peer, self->log);

        //  Handshake discovery by sending HELLO as first message
        zlist_t *groups = zhash_keys (self->own_groups);
        zhash_t *headers = zhash_dup (self->headers);
        
        zre_msg_t *msg = zre_msg_new (ZRE_MSG_HELLO);
        zre_msg_set_ipaddress (msg, self->host);
        zre_msg_set_mailbox (msg, self->port);
        zre_msg_set_groups (msg, &groups);
        zre_msg_set_status (msg, self->status);
        zre_msg_set_headers (msg, &headers);
        zyre_peer_send (peer, &msg);

        //  Send new peer event to logger, if any
        zyre_log_info (self->log, ZRE_LOG_MSG_EVENT_ENTER,
                      zyre_peer_endpoint (peer), endpoint);
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
    zstr_sendm (self->pipe, "EXIT");
    zstr_send (self->pipe, zyre_peer_identity (peer));
    //  Send a log event
    zyre_log_info (self->log, ZRE_LOG_MSG_EVENT_EXIT,
                   zyre_peer_endpoint (peer),
                   zyre_peer_endpoint (peer));
    //  Remove peer from any groups we've got it in
    zhash_foreach (self->peer_groups, zyre_node_delete_peer, peer);
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
    zstr_sendm (self->pipe, "JOIN");
    zstr_sendm (self->pipe, zyre_peer_identity (peer));
    zstr_send (self->pipe, name);

    return group;
}

static zyre_group_t *
zyre_node_leave_peer_group (zyre_node_t *self, zyre_peer_t *peer, const char *name)
{
    zyre_group_t *group = zyre_node_require_peer_group (self, name);
    zyre_group_leave (group, peer);

    //  Now tell the caller about the peer left group
    zstr_sendm (self->pipe, "LEAVE");
    zstr_sendm (self->pipe, zyre_peer_identity (peer));
    zstr_send (self->pipe, name);

    return group;
}

//  Here we handle messages coming from other peers

static int
zyre_node_recv_peer (zyre_node_t *self)
{
    //  Router socket tells us the identity of this peer
    zre_msg_t *msg = zre_msg_recv (self->inbox);
    if (msg == NULL)
        return 0;               //  Interrupted

    //  First frame is sender identity, holding binary UUID
    zuuid_t *uuid = zuuid_new ();
    zuuid_set (uuid, zframe_data (zre_msg_routing_id (msg)));
    if (self->verbose)
        zyre_log_info (self->log, ZRE_LOG_MSG_EVENT_RECV,
                    zuuid_str (self->uuid), zuuid_str (uuid));

    //  On HELLO we may create the peer if it's unknown
    //  On other commands the peer must already exist
    zyre_peer_t *peer = (zyre_peer_t *) zhash_lookup (self->peers, zuuid_str (uuid));
    if (zre_msg_id (msg) == ZRE_MSG_HELLO) {
        if (peer) {
            // Remove fake peers
            if (zyre_peer_ready (peer)) {
                zyre_node_remove_peer (self, peer);
                assert (!(zyre_peer_t *) zhash_lookup (self->peers, zuuid_str (uuid)));
            }
            else {
                char endpoint_node [30];
                sprintf (endpoint_node, "tcp://%s:%d", self->host, self->port);
                // We ignore HELLO, because peer has same host:port as current node
                if (streq (endpoint_node, zyre_peer_endpoint (peer))) {
                    zre_msg_destroy (&msg);
                    zuuid_destroy (&uuid);
                    return 0;
                }
            }
        }
        peer = zyre_node_require_peer (
            self, uuid, zre_msg_ipaddress (msg), zre_msg_mailbox (msg));
        assert (peer);
        zyre_peer_set_ready (peer, true);
    }
    //  Ignore command if peer isn't ready
    if (peer == NULL || !zyre_peer_ready (peer)) {
        zre_msg_destroy (&msg);
        zuuid_destroy (&uuid);
        return 0;
    }
    if (!zyre_peer_check_message (peer, msg)) {
        zclock_log ("W: [%s] lost messages from %s", zuuid_str (self->uuid), zuuid_str (uuid));
        assert (false);
    }

    //  Now process each command
    if (zre_msg_id (msg) == ZRE_MSG_HELLO) {
        //  Tell the caller about the peer
        zstr_sendm (self->pipe, "ENTER");
        zstr_sendm (self->pipe, zuuid_str (uuid));
        zframe_t *headers = zhash_pack (zre_msg_headers (msg));
        zframe_send (&headers, self->pipe, ZFRAME_MORE);
        zstr_sendf (self->pipe, "%s:%d",
                    zre_msg_ipaddress (msg), zre_msg_mailbox (msg));
        
        //  Join peer to listed groups
        const char *name = zre_msg_groups_first (msg);
        while (name) {
            zyre_node_join_peer_group (self, peer, name);
            name = zre_msg_groups_next (msg);
        }
        //  Hello command holds latest status of peer
        zyre_peer_set_status (peer, zre_msg_status (msg));
        
        //  Store peer headers for future reference
        zyre_peer_set_headers (peer, zre_msg_headers (msg));

        //  If peer is a ZRE/LOG collector, connect to it
        const char *collector = zre_msg_headers_string (msg, "X-ZRELOG", NULL);
        if (collector)
            zyre_log_connect (self->log, collector);
    }
    else
    if (zre_msg_id (msg) == ZRE_MSG_WHISPER) {
        //  Pass up to caller API as WHISPER event
        zstr_sendm (self->pipe, "WHISPER");
        zstr_sendm (self->pipe, zuuid_str (uuid));
        zmsg_t *content = zmsg_dup (zre_msg_content (msg));
        zmsg_send (&content, self->pipe);
    }
    else
    if (zre_msg_id (msg) == ZRE_MSG_SHOUT) {
        //  Pass up to caller as SHOUT event
        zstr_sendm (self->pipe, "SHOUT");
        zstr_sendm (self->pipe, zuuid_str (uuid));
        zstr_sendm (self->pipe, zre_msg_group (msg));
        zmsg_t *content = zmsg_dup (zre_msg_content (msg));
        zmsg_send (&content, self->pipe);
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
    return 0;
}

//  Handle beacon

static int
zyre_node_recv_beacon (zyre_node_t *self)
{
    //  Get IP address and beacon of peer
    char *ipaddress = zstr_recv (zbeacon_socket (self->beacon));
    zframe_t *frame = zframe_recv (zbeacon_socket (self->beacon));

    //  Ignore anything that isn't a valid beacon
    bool is_valid = true;
    beacon_t beacon;
    if (zframe_size (frame) == sizeof (beacon_t)) {
        memcpy (&beacon, zframe_data (frame), zframe_size (frame));
        if (beacon.version != BEACON_VERSION)
            is_valid = false;
    }
    else
        is_valid = false;

    //  Check that the peer, identified by its UUID, exists
    if (is_valid) {
        zuuid_t *uuid = zuuid_new ();
        zuuid_set (uuid, beacon.uuid);
        if (beacon.port) {
            zyre_peer_t *peer = zyre_node_require_peer (
                self, uuid, ipaddress, ntohs (beacon.port));
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
    }
    zstr_free (&ipaddress);
    zframe_destroy (&frame);
    return 0;
}


//  We do this once a second:
//  - if peer has gone quiet, send TCP ping
//  - if peer has disappeared, expire it

static int
zyre_node_ping_peer (const char *key, void *item, void *argument)
{
    zyre_peer_t *peer = (zyre_peer_t *) item;
    zyre_node_t *self = (zyre_node_t *) argument;
    if (zclock_time () >= zyre_peer_expired_at (peer))
        zyre_node_remove_peer (self, peer);
    else
    if (zclock_time () >= zyre_peer_evasive_at (peer)) {
        //  If peer is being evasive, force a TCP ping.
        //  TODO: do this only once for a peer in this state;
        //  it would be nicer to use a proper state machine
        //  for peer management.
        zre_msg_t *msg = zre_msg_new (ZRE_MSG_PING);
        zyre_peer_send (peer, &msg);
    }
    return 0;
}


//  --------------------------------------------------------------------------
//  This is the engine that runs a single node; it uses one thread, creates
//  a zyre_node object at start and destroys that when finishing.

void
zyre_node_engine (void *args, zctx_t *ctx, void *pipe)
{
    //  Create node instance to pass around
    zyre_node_t *self = zyre_node_new (ctx, pipe);
    if (!self)                  //  Interrupted
        return;
    
    //  Signal success by returning our node ID to API
    zstr_send (self->pipe, zuuid_str (self->uuid));

    uint64_t reap_at = zclock_time () + REAP_INTERVAL;

    //  Loop until the agent is terminated one way or another
    while (!self->terminated) {
        int timeout = (int) (reap_at - zclock_time ());
        assert (timeout <= REAP_INTERVAL);
        if (timeout < 0)
            timeout = 0;
        
        void *which = zpoller_wait (self->poller, timeout);
        if (which == self->pipe)
            zyre_node_recv_api (self);
        else
        if (which == self->inbox)
            zyre_node_recv_peer (self);
        else
        if (self->beacon
        &&  which == zbeacon_socket (self->beacon))
            zyre_node_recv_beacon (self);
        else
        if (zpoller_expired (self->poller)) {
            if (zclock_time () >= reap_at) {
                reap_at = zclock_time () + REAP_INTERVAL;
                //  Ping all peers and reap any expired ones
                zhash_foreach (self->peers, zyre_node_ping_peer, self);
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
    zctx_t *ctx = zctx_new ();
    void *pipe = zsocket_new (ctx, ZMQ_PAIR);
    zyre_node_t *node = zyre_node_new (ctx, pipe);
    zyre_node_destroy (&node);
    zctx_destroy (&ctx);
    printf ("OK\n");
}
