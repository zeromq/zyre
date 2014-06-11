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
    //  We send command replies and signals to the pipe
    zsock_t *pipe;              //  Pipe back to application
    //  We send all Zyre messages to the outbox
    zsock_t *outbox;            //  Outbox back to application
    bool terminated;            //  API shut us down
    bool verbose;               //  Log all traffic
    int beacon_port;            //  Beacon port number
    size_t interval;            //  Beacon interval
    zpoller_t *poller;          //  Socket poller
    zbeacon_t *beacon;          //  Beacon object
    zuuid_t *uuid;              //  Our UUID as object
    zsock_t *inbox;             //  Our inbox socket (ROUTER)
    char *name;                 //  Our public name
    char *endpoint;             //  Our public endpoint
    int port;                   //  Our inbox port, if any
    bool bound;                 //  Did app bind node explicitly?
    byte status;                //  Our own change counter
    zhash_t *peers;             //  Hash of known peers, fast lookup
    zhash_t *peer_groups;       //  Groups that our peers are in
    zhash_t *own_groups;        //  Groups that we are in
    zhash_t *headers;           //  Our header values
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


//  ---------------------------------------------------------------------
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
    self->name = strndup (zuuid_str (self->uuid), 6);

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
        zsock_destroy (&self->inbox);
        zsock_destroy (&self->outbox);
        zbeacon_destroy (&self->beacon);
        zstr_free (&self->endpoint);
        free (self->name);
        free (self);
        *self_p = NULL;
    }
}

//  Bind node to endpoint

static int
zyre_node_bind (zyre_node_t *self, const char *endpoint)
{
    assert (!self->endpoint);
    self->port = zsock_bind (self->inbox, "%s", endpoint);
    if (self->port < 0)
        return 1;               //  Endpoint is occupied, or invalid

    //  Successful bind, save endpoint and disable beaconing
    self->endpoint = strdup (endpoint);
    self->bound = true;
    self->beacon_port = 0;
    return 0;
}

//  Connect node to endpoint explicitly

static int
zyre_node_connect (zyre_node_t *self, const char *endpoint)
{
    return 1;
}


//  Start node, return 0 if OK, 1 if not possible

static int
zyre_node_start (zyre_node_t *self)
{
    //  If application didn't bind explicitly, we grab an ephemeral port
    //  on all available network interfaces. This is orthogonal to
    //  beaconing, since we can connect to other peers and they will
    //  gossip our endpoint to others.
    if (!self->bound) {
        self->port = zsock_bind (self->inbox, "tcp://*:*");
        if (self->port < 0)
            return 1;           //  Could not get new port to bind to?
        self->bound = true;
    }
    //  Start UDP beaconing, if the application didn't disable it
    if (self->beacon_port) {
        assert (!self->beacon);
        self->beacon = zbeacon_new (NULL, self->beacon_port);
        if (!self->beacon)
            return 1;               //  Not possible to start beacon

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
        assert (!self->endpoint);
        self->endpoint = zsys_sprintf ("tcp://%s:%d",
            zbeacon_hostname (self->beacon), self->port);
    }
    else
    if (!self->endpoint) {
        char *hostname = zsys_hostname ();
        self->endpoint = zsys_sprintf ("tcp://%s:%d", hostname, self->port);
        zstr_free (&hostname);
    }
    //  Start polling on inbox
    zpoller_add (self->poller, self->inbox);
    return 0;
}

//  Stop node discovery and interconnection
//  TODO: clear peer tables

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
        zbeacon_publish (self->beacon, (byte *) &beacon, sizeof (beacon_t));
        zclock_sleep (1);           //  Allow 1 msec for beacon to go out
        zbeacon_destroy (&self->beacon);
    }
    //  Stop polling on inbox
    zpoller_destroy (&self->poller);
    self->poller = zpoller_new (self->pipe, NULL);
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

//  Dump hash key to stdout
static int
zyre_node_hash_key_dump (const char *key, void *item, void *argument)
{
    printf ("        %s\n", key);
    return 0;
}

//  Prints node information

static void
zyre_node_dump (zyre_node_t *self)
{
    fflush (stdout);
    printf ("************** zyre_node_dump *************************\n");
    printf ("node id : %s\n", zuuid_str (self->uuid));
    printf ("    endpoint = %s\n", self->endpoint);
    printf ("    headers [%zu] { \n", zhash_size (self->headers));
    zhash_foreach (self->headers, zyre_node_hash_key_dump, self);
    printf ("    }\n");
    printf ("    peers [%zu] {\n", zhash_size (self->peers));
    zhash_foreach (self->peers, zyre_node_hash_key_dump, self);
    printf ("    }\n");
    printf ("    own groups [%zu] { \n", zhash_size (self->own_groups));
    zhash_foreach (self->own_groups, zyre_node_hash_key_dump, self);
    printf ("    }\n");
    printf ("    peer groups [%zu] {\n", zhash_size (self->peer_groups));
    zhash_foreach (self->peer_groups, zyre_node_hash_key_dump, self);
    printf ("    }\n");
    printf ("*******************************************************\n");
    fflush (stdout);
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
    if (streq (command, "UUID"))
        zstr_send (self->pipe, zuuid_str (self->uuid));
    else
    if (streq (command, "NAME"))
        zstr_send (self->pipe, self->name);
    else
    if (streq (command, "BIND")) {
        char *endpoint = zmsg_popstr (request);
        zsock_signal (self->pipe, zyre_node_bind (self, endpoint));
        zstr_free (&endpoint);
    }
    else
    if (streq (command, "CONNECT")) {
        char *endpoint = zmsg_popstr (request);
        zsock_signal (self->pipe, zyre_node_connect (self, endpoint));
        zstr_free (&endpoint);
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
            zhash_foreach (self->peers, zyre_node_send_peer, msg);
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
            zhash_foreach (self->peers, zyre_node_send_peer, msg);
            zre_msg_destroy (&msg);
            zhash_delete (self->own_groups, name);
            if (self->verbose)
                zsys_info ("(%s) LEAVE group=%s", self->name, name);
        }
        zstr_free (&name);
    }
    else
    if (streq (command, "DUMP"))
        zyre_node_dump (self);
    else
    if (streq (command, "$TERM"))
        self->terminated = true;
    else {
        zsys_error ("invalid command '%s'\n", command);
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
zyre_node_require_peer (zyre_node_t *self, zuuid_t *uuid, const char *endpoint)
{
    assert (self);
    assert (endpoint);

    zyre_peer_t *peer = (zyre_peer_t *) zhash_lookup (self->peers, zuuid_str (uuid));
    if (!peer) {
        //  Purge any previous peer on same endpoint
        zhash_foreach (self->peers, zyre_node_purge_peer, (char *) endpoint);

        peer = zyre_peer_new (self->peers, uuid);
        zyre_peer_set_origin (peer, self->name);
        zyre_peer_set_verbose (peer, self->verbose);
        zyre_peer_connect (peer, self->uuid, (char *) endpoint);

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

static int
zyre_node_recv_peer (zyre_node_t *self)
{
    //  Router socket tells us the identity of this peer
    zre_msg_t *msg = zre_msg_recv (self->inbox);
    if (msg == NULL)
        return 0;               //  Interrupted

    if (self->verbose)
        zre_msg_print (msg);
    
    //  First frame is sender identity
    byte *peerid_data = zframe_data (zre_msg_routing_id (msg));
    size_t peerid_size = zframe_size (zre_msg_routing_id (msg));

    //  Identity must be [1] followed by 16-byte UUID
    if (peerid_size != ZUUID_LEN + 1) {
        zre_msg_destroy (&msg);
        return -1;
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
                return 0;
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
        return 0;
    }
    if (!zyre_peer_check_message (peer, msg)) {
        zsys_warning ("(%s) lost messages from %s", self->name, zyre_peer_name (peer));
        assert (false);
    }

    //  Now process each command
    if (zre_msg_id (msg) == ZRE_MSG_HELLO) {
        //  Store properties from HELLO command into peer
        zyre_peer_set_name (peer, zre_msg_name (msg));
        zyre_peer_set_status (peer, zre_msg_status (msg));
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
    uint64_t reap_at = zclock_time () + REAP_INTERVAL;
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
    zsock_t *pipe = zsock_new (ZMQ_PAIR);
    zsock_t *outbox = zsock_new (ZMQ_PAIR);
    zyre_node_t *node = zyre_node_new (pipe, outbox);
    zyre_node_destroy (&node);
    zsock_destroy (&pipe);
    //  Node takes ownership of outbox and destroys it
    printf ("OK\n");
}
