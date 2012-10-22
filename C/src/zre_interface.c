/*  =========================================================================
    zre_interface - interface to a ZyRE network

    -------------------------------------------------------------------------
    Copyright (c) 1991-2012 iMatix Corporation <www.imatix.com>
    Copyright other contributors as noted in the AUTHORS file.

    This file is part of ZyRE, the ZeroMQ Realtime Experience framework:
    http://zyre.org.

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

#include <czmq.h>
#include <uuid/uuid.h>
#include "../include/zre.h"


//  ---------------------------------------------------------------------
//  Structure of our class

struct _zre_interface_t {
    zctx_t *ctx;                //  Our context wrapper
    void *pipe;                 //  Pipe through to agent
};

//  =====================================================================
//  Synchronous part, works in our application thread

//  This is the thread that handles our real interface class
static void
    zre_interface_agent (void *args, zctx_t *ctx, void *pipe);

    
//  ---------------------------------------------------------------------
//  Constructor

zre_interface_t *
zre_interface_new (void)
{
    zre_interface_t
        *self;

    self = (zre_interface_t *) zmalloc (sizeof (zre_interface_t));
    self->ctx = zctx_new ();
    self->pipe = zthread_fork (self->ctx, zre_interface_agent, NULL);
    return self;
}


//  ---------------------------------------------------------------------
//  Destructor

void
zre_interface_destroy (zre_interface_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zre_interface_t *self = *self_p;
        zctx_destroy (&self->ctx);
        free (self);
        *self_p = NULL;
    }
}


//  ---------------------------------------------------------------------
//  Receive next message from interface
//  Returns zmsg_t object, or NULL if interrupted

zmsg_t *
zre_interface_recv (zre_interface_t *self)
{
    assert (self);
    zmsg_t *msg = zmsg_recv (self->pipe);
    return msg;
}


//  ---------------------------------------------------------------------
//  Join a group

int
zre_interface_join (zre_interface_t *self, const char *group)
{
    assert (self);
    zstr_sendm (self->pipe, "JOIN");
    zstr_send  (self->pipe, group);
    return 0;
}


//  ---------------------------------------------------------------------
//  Leave a group

int
zre_interface_leave (zre_interface_t *self, const char *group)
{
    assert (self);
    zstr_sendm (self->pipe, "LEAVE");
    zstr_send  (self->pipe, group);
    return 0;
}


//  ---------------------------------------------------------------------
//  Send message to single peer; peer ID is first frame in message
//  Destroys message after sending

int
zre_interface_whisper (zre_interface_t *self, zmsg_t **msg_p)
{
    assert (self);
    zstr_sendm (self->pipe, "WHISPER");
    zmsg_send (msg_p, self->pipe);
    return 0;
}


//  ---------------------------------------------------------------------
//  Send message to a group of peers

int
zre_interface_shout (zre_interface_t *self, zmsg_t **msg_p)
{
    assert (self);
    zstr_sendm (self->pipe, "SHOUT");
    zmsg_send (msg_p, self->pipe);
    return 0;
}



//  =====================================================================
//  Asynchronous part, works in the background

//  Beacon frame has this format:
//
//  Z R E       3 bytes
//  version     1 byte, %x01
//  UUID        16 bytes
//  port        2 bytes in network order
//  status      peer status as single byte

#define BEACON_PROTOCOL     "ZRE"
#define BEACON_VERSION      0x01

typedef struct {
    byte protocol [3];
    byte version;
    uuid_t uuid;
    uint16_t port;
    byte status;
} beacon_t;

//  Convert binary UUID to freshly allocated string

static char *
s_uuid_str (uuid_t uuid)
{
    char hex_char [] = "0123456789ABCDEF";
    char *string = zmalloc (sizeof (uuid_t) * 2 + 1);
    int byte_nbr;
    for (byte_nbr = 0; byte_nbr < sizeof (uuid_t); byte_nbr++) {
        string [byte_nbr * 2 + 0] = hex_char [uuid [byte_nbr] >> 4];
        string [byte_nbr * 2 + 1] = hex_char [uuid [byte_nbr] & 15];
    }
    return string;
}


//  This structure holds the context for our agent, so we can
//  pass that around cleanly to methods which need it

typedef struct {
    zctx_t *ctx;                //  CZMQ context
    void *pipe;                 //  Pipe back to application
    zre_udp_t *udp;             //  UDP object
    uuid_t uuid;                //  Our UUID as binary blob
    char *identity;             //  Our UUID as hex string
    void *inbox;                //  Our inbox socket (ROUTER)
    char *host;                 //  Our host IP address
    int port;                   //  Our inbox port number
    byte status;                //  Our own change counter
    zhash_t *peers;             //  Hash of known peers, fast lookup
    zhash_t *groups;            //  All known groups, by name
} agent_t;

static agent_t *
agent_new (zctx_t *ctx, void *pipe)
{
    agent_t *self = (agent_t *) zmalloc (sizeof (agent_t));
    self->ctx = ctx;
    self->pipe = pipe;
    self->udp = zre_udp_new (PING_PORT_NUMBER);
    self->inbox = zsocket_new (self->ctx, ZMQ_ROUTER);
    self->host = zre_udp_host (self->udp);
    self->port = zsocket_bind (self->inbox, "tcp://%s:*", self->host);
    uuid_generate (self->uuid);
    self->identity = s_uuid_str (self->uuid);
    self->peers = zhash_new ();
    self->groups = zhash_new ();
    return self;
}

static void
agent_destroy (agent_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        agent_t *self = *self_p;
        zhash_destroy (&self->peers);
        zhash_destroy (&self->groups);
        zre_udp_destroy (&self->udp);
        free (self->identity);
        free (self);
        *self_p = NULL;
    }
}


//  Send message to all peers

static int
s_peer_send (const char *key, void *item, void *argument)
{
    zre_peer_t *peer = (zre_peer_t *) item;
    zre_msg_t *msg = zre_msg_dup ((zre_msg_t *) argument);
    zre_peer_send (peer, &msg);
    return 0;
}


//  Here we handle the different control messages from the front-end

static int
agent_recv_from_api (agent_t *self)
{
    //  Get the whole message off the pipe in one go
    zmsg_t *request = zmsg_recv (self->pipe);
    char *command = zmsg_popstr (request);
    if (command == NULL)
        return -1;      //  Interrupted

    if (streq (command, "WHISPER")) {
        //  Get peer to send message to
        char *identity = zmsg_popstr (request);
        zre_peer_t *peer = (zre_peer_t *) zhash_lookup (self->peers, identity);
        assert (zre_peer_connected (peer));
        
        //  Send frame on out to peer's mailbox, drop message
        //  if peer doesn't exist (may have been destroyed)
        if (peer) {
            zre_msg_t *msg = zre_msg_new (ZRE_MSG_WHISPER);
            zre_msg_cookies_set (msg, zmsg_pop (request));
            zre_peer_send (peer, &msg);
        }
        else
            printf ("W: trying to send to %s, no longer exists\n", identity);
        
        free (identity);
    }
    else
    if (streq (command, "SHOUT")) {
        //  Get group to send message to
        char *name = zmsg_popstr (request);
        zre_group_t *group = (zre_group_t *) zhash_lookup (self->groups, name);
        if (group) {
            zre_msg_t *msg = zre_msg_new (ZRE_MSG_SHOUT);
            zre_msg_cookies_set (msg, zmsg_pop (request));
            zre_group_send (group, &msg);
        }
        free (name);
    }
    else
    if (streq (command, "JOIN")
    ||  streq (command, "LEAVE")) {
        char *name = zmsg_popstr (request);
        zre_msg_t *msg = zre_msg_new (
            streq (command, "JOIN")? ZRE_MSG_JOIN: ZRE_MSG_LEAVE);
        zre_msg_group_set (msg, name);
        zre_msg_status_set (msg, self->status++);
        zhash_foreach (self->peers, s_peer_send, msg);
        zre_msg_destroy (&msg);
        free (name);
    }
    free (command);
    zmsg_destroy (&request);
    return 0;
}


//  Find or create peer via its UUID string

static zre_peer_t *
s_require_peer (agent_t *self, char *identity, char *address, int port)
{
    zre_peer_t *peer = (zre_peer_t *) zhash_lookup (self->peers, identity);
    if (peer == NULL) {
        peer = zre_peer_new (identity, self->peers, self->ctx);
        zre_peer_connect (peer, self->identity, address, port);

        //  Handshake discovery by sending HELLO as first message
        zre_msg_t *msg = zre_msg_new (ZRE_MSG_HELLO);
        zre_msg_from_set (msg, zre_udp_host (self->udp));
        zre_msg_port_set (msg, self->port);
        zre_peer_send (peer, &msg);

        //  Now tell the caller about the peer
        zstr_sendm (self->pipe, "ENTER");
        zstr_send (self->pipe, identity);
    }
    return peer;
}


//  Find or create group via its name

static zre_group_t *
s_require_group (agent_t *self, char *name)
{
    zre_group_t *group = (zre_group_t *) zhash_lookup (self->groups, name);
    if (group == NULL)
        group = zre_group_new (name, self->groups);
    return group;
}


//  Here we handle messages coming from other peers

static int
agent_recv_from_peer (agent_t *self)
{
    //  Router socket tells us the identity of this peer
    zre_msg_t *msg = zre_msg_recv (self->inbox);
    char *identity = zframe_strdup (zre_msg_address (msg));
    
    //  On HELLO we can connect back to peer if needed
    if (zre_msg_id (msg) == ZRE_MSG_HELLO)
        s_require_peer (self, identity,
            zre_msg_from (msg), zre_msg_port (msg));

    //  Peer must exist by now or our code is wrong
    zre_peer_t *peer = (zre_peer_t *) zhash_lookup (self->peers, identity);
    assert (peer);
    zre_peer_refresh (peer);
        
    if (zre_msg_id (msg) == ZRE_MSG_WHISPER) {
        //  Pass up to caller API as RECVFROM event
        zstr_sendm (self->pipe, "FROM");
        zstr_sendm (self->pipe, identity);
        zstr_send (self->pipe, "Cookies");
    }
    else
    if (zre_msg_id (msg) == ZRE_MSG_SHOUT) {
        // todo
    }
    else
    if (zre_msg_id (msg) == ZRE_MSG_PING) {
        zre_msg_t *msg = zre_msg_new (ZRE_MSG_PING_OK);
        zre_peer_send (peer, &msg);
    }
    else
    if (zre_msg_id (msg) == ZRE_MSG_JOIN) {
        zre_group_t *group = s_require_group (self, zre_msg_group (msg));
        zre_group_join (group, peer);
    }
    else
    if (zre_msg_id (msg) == ZRE_MSG_LEAVE) {
        zre_group_t *group = s_require_group (self, zre_msg_group (msg));
        zre_group_leave (group, peer);
    }
    free (identity);
    zre_msg_destroy (&msg);
    return 0;
}

//  Send moar beacon

static void
agent_beacon_send (agent_t *self)
{
    //  Beacon object
    beacon_t beacon;

    //  Format beacon fields
    beacon.protocol [0] = 'Z';
    beacon.protocol [1] = 'R';
    beacon.protocol [2] = 'E';
    beacon.version = BEACON_VERSION;
    memcpy (beacon.uuid, self->uuid, sizeof (uuid_t));
    beacon.port = htons (self->port);
    beacon.status = self->status;

    //  Broadcast the beacon to anyone who is listening
    zre_udp_send (self->udp, (byte *) &beacon, sizeof (beacon_t));
}


//  Handle beacon

static int
agent_recv_udp_beacon (agent_t *self)
{
    //  Get beacon frame from network
    beacon_t beacon;
    ssize_t size = zre_udp_recv (self->udp, (byte *) &beacon, sizeof (beacon_t));

    //  Basic validation on the frame
    if (size != sizeof (beacon_t)
    ||  beacon.protocol [0] != 'Z'
    ||  beacon.protocol [1] != 'R'
    ||  beacon.protocol [2] != 'E'
    ||  beacon.version != BEACON_VERSION)
        return 0;               //  Ignore invalid beacons

    //  If we got a UUID and it's not our own beacon, we have a peer
    if (memcmp (beacon.uuid, self->uuid, sizeof (uuid_t))) {
        char *identity = s_uuid_str (beacon.uuid);
        zre_peer_t *peer = s_require_peer (self, identity,
            zre_udp_from (self->udp), ntohs (beacon.port));
        if (beacon.status != zre_peer_status (peer))
            printf ("W: inconsistent status detected (have %d, claimed %d)\n",
                zre_peer_status (peer), beacon.status);
        free (identity);
    }
    return 0;
}

//  We do this once a second:
//  - if peer has gone quiet, send TCP ping
//  - if peer has disappeared, expire it

static int
agent_ping_peer (const char *key, void *item, void *argument)
{
    agent_t *self = (agent_t *) argument;
    zre_peer_t *peer = (zre_peer_t *) item;
    if (zclock_time () >= zre_peer_expired_at (peer)) {
        //  If peer has really vanished, expire it
        zstr_sendm (self->pipe, "EXIT");
        zstr_send (self->pipe, zre_peer_identity (peer));
        zhash_delete (self->peers, zre_peer_identity (peer));
    }
    else
    if (zclock_time () >= zre_peer_evasive_at (peer)) {
        //  If peer is being evasive, force a TCP ping.
        //  TODO: do this only once for a peer in this state;
        //  it would be nicer to use a proper state machine
        //  for peer management.
        zre_msg_t *msg = zre_msg_new (ZRE_MSG_PING);
        zre_peer_send (peer, &msg);
    }
    return 0;
}

//  The agent handles API commands

static void
zre_interface_agent (void *args, zctx_t *ctx, void *pipe)
{
    //  Create agent instance to pass around
    agent_t *self = agent_new (ctx, pipe);
    
    //  Send first beacon immediately
    uint64_t ping_at = zclock_time ();
    zmq_pollitem_t pollitems [] = {
        { self->pipe, 0, ZMQ_POLLIN, 0 },
        { self->inbox, 0, ZMQ_POLLIN, 0 },
        { 0, zre_udp_handle (self->udp), ZMQ_POLLIN, 0 }
    };
    while (!zctx_interrupted) {
        long timeout = (long) (ping_at - zclock_time ());
        if (timeout < 0)
            timeout = 0;
        if (zmq_poll (pollitems, 3, timeout * ZMQ_POLL_MSEC) == -1)
            break;              //  Interrupted

        if (pollitems [0].revents & ZMQ_POLLIN)
            agent_recv_from_api (self);
        
        if (pollitems [1].revents & ZMQ_POLLIN)
            agent_recv_from_peer (self);

        if (pollitems [2].revents & ZMQ_POLLIN)
            agent_recv_udp_beacon (self);

        if (zclock_time () >= ping_at) {
            agent_beacon_send (self);
            ping_at = zclock_time () + PING_INTERVAL;
            //  Ping all peers and reap any expired ones
            zhash_foreach (self->peers, agent_ping_peer, self);
        }
    }
    agent_destroy (&self);
}
