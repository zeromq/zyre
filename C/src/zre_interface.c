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
//  Send message to single peer
//  Destroys message after sending

int
zre_interface_send (zre_interface_t *self, zframe_t *peer, zmsg_t **msg_p)
{
    assert (self);
    zmsg_destroy (msg_p);
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

#define BEACON_PROTOCOL     "ZRE"
#define BEACON_VERSION      0x01

typedef struct {
    byte protocol [3];
    byte version;
    uuid_t uuid;
    uint16_t port;
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

//  Callback when we remove peer from agent->peers

static void
s_delete_peer (void *argument)
{
    zre_peer_t *peer = (zre_peer_t *) argument;
    zre_peer_destroy (&peer);
}


//  This structure holds the context for our agent, so we can
//  pass that around cleanly to methods which need it

typedef struct {
    zctx_t *ctx;                //  CZMQ context
    void *pipe;                 //  Pipe back to application
    zre_udp_t *udp;             //  UDP object
    uuid_t uuid;                //  Our UUID as binary blob
    void *router;               //  Our router socket
    int port;                   //  Our router port number
    zhash_t *peers;             //  Hash of known peers, fast lookup
} agent_t;

static agent_t *
agent_new (zctx_t *ctx, void *pipe)
{
    agent_t *self = (agent_t *) zmalloc (sizeof (agent_t));
    self->ctx = ctx;
    self->pipe = pipe;
    self->udp = zre_udp_new (PING_PORT_NUMBER);
    self->router = zsocket_new (self->ctx, ZMQ_ROUTER);
    self->port = zsocket_bind (self->router, "tcp://*:*");
    self->peers = zhash_new ();
    uuid_generate (self->uuid);
    return self;
}

static void
agent_destroy (agent_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        agent_t *self = *self_p;
        zhash_destroy (&self->peers);
        zre_udp_destroy (&self->udp);
        free (self);
        *self_p = NULL;
    }
}

//  Here we handle the different control messages from the front-end;

static int
agent_control_message (agent_t *self)
{
    //  Get the whole message off the pipe in one go
    zmsg_t *msg = zmsg_recv (self->pipe);
    char *command = zmsg_popstr (msg);
    if (command == NULL)
        return -1;      //  Interrupted

    //  We don't actually implement any control commands yet
    //  but if we did, this would be where we did it...
    //  if (streq (command, "EXAMPLE")) {
    //  }
    
    free (command);
    zmsg_destroy (&msg);
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

    //  Broadcast the beacon to anyone who is listening
    zre_udp_send (self->udp, (byte *) &beacon, sizeof (beacon_t));
}


//  Handle beacon

static int
agent_beacon_recv (agent_t *self)
{
    beacon_t beacon;
    //  Get beacon frame from network
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
        char *uuid_str = s_uuid_str (beacon.uuid);
        //  Find or create peer via its UUID string
        zre_peer_t *peer = (zre_peer_t *) zhash_lookup (self->peers, uuid_str);
        if (peer == NULL) {
            //  Connect our router socket to peer
            char *address = zre_udp_sender (self->udp);
            int port = ntohs (beacon.port);
            zsocket_connect (self->router, "tcp://%s:%d", address, port);
            printf ("Connecting to: %s:%d\n", address, port);
            free (address);

            //  Create new peer entity in hash table
            peer = zre_peer_new (beacon.uuid, uuid_str);
            zhash_insert (self->peers, uuid_str, peer);
            zhash_freefn (self->peers, uuid_str, s_delete_peer);
            
            //  Report peer joined the network
            zstr_sendm (self->pipe, "JOINED");
            zstr_send (self->pipe, uuid_str);
        }
        //  Any activity from the peer means it's alive
        zre_peer_is_alive (peer);
        free (uuid_str);
    }
    return 0;
}

//  Reap peer if no signs of life by expiry time

static int
agent_reap_peer (const char *key, void *item, void *argument)
{
    agent_t *self = (agent_t *) argument;
    zre_peer_t *peer = (zre_peer_t *) item;
    if (zclock_time () >= zre_peer_expires_at (peer)) {
        //  Report peer left the network
        zstr_sendm (self->pipe, "LEFT");
        zstr_send (self->pipe, zre_peer_uuid_str (peer));
        zhash_delete (self->peers, zre_peer_uuid_str (peer));
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
        { 0, zre_udp_handle (self->udp), ZMQ_POLLIN, 0 }
    };
    
    while (!zctx_interrupted) {
        long timeout = (long) (ping_at - zclock_time ());
        if (timeout < 0)
            timeout = 0;
        if (zmq_poll (pollitems, 2, timeout * ZMQ_POLL_MSEC) == -1)
            break;              //  Interrupted

        if (pollitems [0].revents & ZMQ_POLLIN)
            agent_control_message (self);
        
        if (pollitems [1].revents & ZMQ_POLLIN)
            agent_beacon_recv (self);

        if (zclock_time () >= ping_at) {
            agent_beacon_send (self);
            ping_at = zclock_time () + PING_INTERVAL;
        }
        //  Delete and report any expired peers
        zhash_foreach (self->peers, agent_reap_peer, self);
    }
    agent_destroy (&self);
}
