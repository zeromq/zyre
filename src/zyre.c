/*  =========================================================================
    zyre - API wrapping one Zyre node

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

/*
@header
    Zyre does local area discovery and clustering. A Zyre node broadcasts
    UDP beacons, and connects to peers that it finds. This class wraps a
    Zyre node with a message-based API.

    All incoming events are zmsg_t messages delivered via the zyre_recv
    call. The first frame defines the type of the message, and following
    frames provide further values:

        ENTER fromnode name headers ipaddress:port
            a new peer has entered the network
        EXIT fromnode name
            a peer has left the network
        JOIN fromnode name groupname
            a peer has joined a specific group
        LEAVE fromnode name groupname
            a peer has joined a specific group
        WHISPER fromnode name message
            a peer has sent this node a message
        SHOUT fromnode name groupname message
            a peer has sent one of our groups a message
            
    In SHOUT and WHISPER the message is zero or more frames, and can hold
    any ZeroMQ message. In ENTER, the headers frame contains a packed
    dictionary, see zhash_pack/unpack.

    To join or leave a group, use the zyre_join and zyre_leave methods.
    To set a header value, use the zyre_set_header method. To send a message
    to a single peer, use zyre_whisper. To send a message to a group, use
    zyre_shout.
@discuss
    Todo: allow multipart contents
@end
*/

#include "../include/zyre.h"
#include "zyre_classes.h"

//  --------------------------------------------------------------------------
//  Structure of our class

struct _zyre_t {
    zactor_t *actor;            //  A Zyre instance wraps the actor instance
    zsock_t *inbox;             //  Receives incoming cluster traffic
    char *uuid;                 //  Copy of node UUID string
    char *name;                 //  Copy of node name
    char *endpoint;             //  Copy of last endpoint bound to
};


//  --------------------------------------------------------------------------
//  Constructor, creates a new Zyre node. Note that until you start the
//  node it is silent and invisible to other nodes on the network.
//  The node name is provided to other nodes during discovery. If you
//  specify NULL, Zyre generates a randomized node name from the UUID.

zyre_t *
zyre_new (const char *name)
{
    zyre_t *self = (zyre_t *) zmalloc (sizeof (zyre_t));
    assert (self);

    //  Create front-to-back pipe pair for data traffic
    zsock_t *outbox;
    self->inbox = zsys_create_pipe (&outbox);
    
    //  Start node engine and wait for it to be ready
    self->actor = zactor_new (zyre_node_actor, outbox);
    assert (self->actor);

    //  Send name, if any, to node ending
    if (name)
        zstr_sendx (self->actor, "SET NAME", name, NULL);
    
    return self;
}


//  --------------------------------------------------------------------------
//  Destructor, destroys a Zyre node. When you destroy a node, any
//  messages it is sending or receiving will be discarded.

void
zyre_destroy (zyre_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zyre_t *self = *self_p;
        zactor_destroy (&self->actor);
        zsock_destroy (&self->inbox);
        zstr_free (&self->uuid);
        zstr_free (&self->name);
        zstr_free (&self->endpoint);
        free (self);
        *self_p = NULL;
    }
}


//  --------------------------------------------------------------------------
//  Return our node UUID string, after successful initialization

const char *
zyre_uuid (zyre_t *self)
{
    assert (self);
    //  Hold uuid string in zyre object so caller gets a safe reference
    zstr_free (&self->uuid);
    zstr_sendx (self->actor, "UUID", NULL);
    self->uuid = zstr_recv (self->actor);
    return self->uuid;
}


//  --------------------------------------------------------------------------
//  Return our node name, after successful initialization. By default
//  is taken from the UUID and shortened.

const char *
zyre_name (zyre_t *self)
{
    assert (self);
    //  Hold name in zyre object so caller gets a safe reference
    zstr_free (&self->name);
    zstr_sendx (self->actor, "NAME", NULL);
    self->name = zstr_recv (self->actor);
    return self->name;
}


//  --------------------------------------------------------------------------
//  Set node header; these are provided to other nodes during discovery
//  and come in each ENTER message.

void
zyre_set_name (zyre_t *self, const char *name)
{
    assert (self);
    zstr_sendx (self->actor, "SET NAME", name, NULL);
}


//  --------------------------------------------------------------------------
//  Set node header; these are provided to other nodes during discovery
//  and come in each ENTER message.

void
zyre_set_header (zyre_t *self, const char *name, const char *format, ...)
{
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    char *string = zsys_vprintf (format, argptr);
    va_end (argptr);

    zstr_sendx (self->actor, "SET HEADER", name, string, NULL);
    zstr_free (&string);
}


//  --------------------------------------------------------------------------
//  Set verbose mode; this tells the node to log all traffic as well as
//  all major events.

void
zyre_set_verbose (zyre_t *self)
{
    assert (self);
    zstr_sendx (self->actor, "SET VERBOSE", "1", NULL);
}


//  --------------------------------------------------------------------------
//  Set UDP beacon discovery port; defaults to 5670, this call overrides
//  that so you can create independent clusters on the same network, for
//  e.g. development vs. production. Has no effect after zyre_start().

void
zyre_set_port (zyre_t *self, int port_nbr)
{
    assert (self);
    zstr_sendm (self->actor, "SET PORT");
    zstr_sendf (self->actor, "%d", port_nbr);
}


//  --------------------------------------------------------------------------
//  Set UDP beacon discovery interval, in milliseconds. Default is instant
//  beacon exploration followed by pinging every 1,000 msecs.

void
zyre_set_interval (zyre_t *self, size_t interval)
{
    assert (self);
    zstr_sendm (self->actor, "SET INTERVAL");
    zstr_sendf (self->actor, "%zd", interval);
}


//  --------------------------------------------------------------------------
//  Set network interface for UDP beacons. If you do not set this, CZMQ will
//  choose an interface for you. On boxes with several interfaces you should
//  specify which one you want to use, or strange things can happen.

void
zyre_set_interface (zyre_t *self, const char *value)
{
    //  Implemented by zsys global for now
    zsys_set_interface (value);
}


//  --------------------------------------------------------------------------
//  By default, Zyre binds to an ephemeral TCP port and broadcasts the local
//  host name using UDP beaconing. When you call this method, Zyre will use
//  gossip discovery instead of UDP beaconing. You MUST set-up the gossip
//  service separately using zyre_gossip_bind() and _connect(). Note that the
//  endpoint MUST be valid for both bind and connect operations. You can use
//  inproc://, ipc://, or tcp:// transports (for tcp://, use an IP address
//  that is meaningful to remote as well as local nodes). Returns 0 if
//  the bind was successful, else -1.

int
zyre_set_endpoint (zyre_t *self, const char *format, ...)
{
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    char *string = zsys_vprintf (format, argptr);
    va_end (argptr);
    
    zstr_sendx (self->actor, "SET ENDPOINT", string, NULL);
    free (string);
    return zsock_wait (self->actor) == 0? 0: -1;
}


//  --------------------------------------------------------------------------
//  Set-up gossip discovery of other nodes. At least one node in the cluster
//  must bind to a well-known gossip endpoint, so other nodes can connect to
//  it. Note that gossip endpoints are completely distinct from Zyre node
//  endpoints, and should not overlap (they can use the same transport).

void
zyre_gossip_bind (zyre_t *self, const char *format, ...)
{
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    char *string = zsys_vprintf (format, argptr);
    va_end (argptr);

    zstr_sendx (self->actor, "GOSSIP BIND", string, NULL);
    free (string);
}


//  --------------------------------------------------------------------------
//  Set-up gossip discovery of other nodes. A node may connect to multiple
//  other nodes, for redundancy paths. For details of the gossip network
//  design, see the CZMQ zgossip class.

void
zyre_gossip_connect (zyre_t *self, const char *format, ...)
{
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    char *string = zsys_vprintf (format, argptr);
    va_end (argptr);

    zstr_sendx (self->actor, "GOSSIP CONNECT", string, NULL);
    free (string);
}


//  --------------------------------------------------------------------------
//  Start node, after setting header values. When you start a node it
//  begins discovery and connection. Returns 0 if OK, -1 if it wasn't
//  possible to start the node. If you want to use gossip discovery, set
//  the endpoint (optionally), then bind/connect the gossip network, and
//  only then start the node.

int
zyre_start (zyre_t *self)
{
    assert (self);
    zstr_sendx (self->actor, "START", NULL);
    return zsock_wait (self->actor) == 0? 0: -1;
}


//  --------------------------------------------------------------------------
//  Stop node; this signals to other peers that this node will go away.
//  This is polite; however you can also just destroy the node without
//  stopping it.

void
zyre_stop (zyre_t *self)
{
    assert (self);
    zstr_sendx (self->actor, "STOP", NULL);
    zsock_wait (self->actor);
}


//  --------------------------------------------------------------------------
//  Join a named group; after joining a group you can send messages to
//  the group and all Zyre nodes in that group will receive them.

int
zyre_join (zyre_t *self, const char *group)
{
    assert (self);
    zstr_sendx (self->actor, "JOIN", group, NULL);
    return 0;
}


//  --------------------------------------------------------------------------
//  Leave a group

int
zyre_leave (zyre_t *self, const char *group)
{
    assert (self);
    zstr_sendx (self->actor, "LEAVE", group, NULL);
    return 0;
}


//  --------------------------------------------------------------------------
//  Receive next message from network; the message may be a control
//  message (ENTER, EXIT, JOIN, LEAVE) or data (WHISPER, SHOUT).
//  Returns zmsg_t object, or NULL if interrupted

zmsg_t *
zyre_recv (zyre_t *self)
{
    assert (self);
    return zmsg_recv (self->inbox);
}


//  --------------------------------------------------------------------------
//  Send message to single peer, specified as a UUID string
//  Destroys message after sending

int
zyre_whisper (zyre_t *self, const char *peer, zmsg_t **msg_p)
{
    assert (self);
    assert (peer);
    zstr_sendm (self->actor, "WHISPER");
    zstr_sendm (self->actor, peer);
    zmsg_send (msg_p, self->actor);
    return 0;
}


//  --------------------------------------------------------------------------
//  Send message to a named group
//  Destroys message after sending

int
zyre_shout (zyre_t *self, const char *group, zmsg_t **msg_p)
{
    assert (self);
    assert (group);
    zstr_sendm (self->actor, "SHOUT");
    zstr_sendm (self->actor, group);
    zmsg_send (msg_p, self->actor);
    return 0;
}


//  --------------------------------------------------------------------------
//  Send formatted string to a single peer specified as UUID string

int
zyre_whispers (zyre_t *self, const char *peer, const char *format, ...)
{
    assert (self);
    assert (peer);
    assert (format);

    va_list argptr;
    va_start (argptr, format);
    char *string = zsys_vprintf (format, argptr);
    va_end (argptr);

    zstr_sendm (self->actor, "WHISPER");
    zstr_sendm (self->actor, peer);
    zstr_send (self->actor, string);
    free (string);
    return 0;
}


//  --------------------------------------------------------------------------
//  Send formatted string to a named group

int
zyre_shouts (zyre_t *self, const char *group, const char *format, ...)
{
    assert (self);
    assert (group);
    assert (format);

    va_list argptr;
    va_start (argptr, format);
    char *string = zsys_vprintf (format, argptr);
    va_end (argptr);

    zstr_sendm (self->actor, "SHOUT");
    zstr_sendm (self->actor, group);
    zstr_send  (self->actor, string);
    free (string);
    return 0;
}


//  --------------------------------------------------------------------------
//  Return zlist of current peers. The caller owns this list and should
//  destroy it when finished with it.

zlist_t *
zyre_peers (zyre_t *self)
{
    zlist_t *peers;
    zstr_send (self->actor, "PEERS");
    zsock_recv (self->actor, "p", &peers);
    return peers;
}

//  --------------------------------------------------------------------------
//  Return the endpoint of a connected peer. Caller owns the
//  string.

char *
zyre_peer_address(zyre_t *self, const char *peer)
{
    assert (self);
    char *address;
    zstr_sendm (self->actor, "PEER ENDPOINT");
    zstr_send (self->actor, peer);
    zsock_recv (self->actor, "s", &address);
    return address;
}

//  --------------------------------------------------------------------------
//  Return the value of a header of a conected peer. 
//  Returns null if peer or key doesn't exits. Caller
//  owns the string
char *
zyre_peer_header_value (zyre_t *self, const char *peer, const char *name)
{
    assert (self);
    assert (peer);
    assert (name);
    char *value;
    zstr_sendm (self->actor, "PEER HEADER");
    zstr_sendm (self->actor, peer);
    zstr_send (self->actor, name);
    value = zstr_recv (self->actor);
    return value;
}

//  --------------------------------------------------------------------------
//  Return zlist of currently joined groups. The caller owns this list and
//  should destroy it when finished with it.

zlist_t *
zyre_own_groups (zyre_t *self)
{
    zlist_t *groups;
    zstr_send (self->actor, "OWN GROUPS");
    zsock_recv (self->actor, "p", &groups);
    return groups;
}


//  --------------------------------------------------------------------------
//  Return zlist of groups known through connected peers. The caller owns this
//  list and should destroy it when finished with it.

zlist_t *
zyre_peer_groups (zyre_t *self)
{
    zlist_t *groups;
    zstr_send (self->actor, "PEER GROUPS");
    zsock_recv (self->actor, "p", &groups);
    return groups;
}


//  --------------------------------------------------------------------------
//  Return node zsock_t socket, for direct polling of socket

zsock_t *
zyre_socket (zyre_t *self)
{
    assert (self);
    return self->inbox;
}


//  --------------------------------------------------------------------------
//  Prints zyre node information

void
zyre_dump (zyre_t *self)
{
    zstr_send (self->actor, "DUMP");
}


//  --------------------------------------------------------------------------
//  Return the Zyre version for run-time API detection

void zyre_version (int *major, int *minor, int *patch)
{
    *major = ZYRE_VERSION_MAJOR;
    *minor = ZYRE_VERSION_MINOR;
    *patch = ZYRE_VERSION_PATCH;
}


//  --------------------------------------------------------------------------
//  Self test of this class

void
zyre_test (bool verbose)
{
    printf (" * zyre: ");
    if (verbose)
        printf ("\n");
    
    //  @selftest
    //  We'll use inproc gossip discovery so that this works without networking
    
    int major, minor, patch;
    zyre_version (&major, &minor, &patch);
    assert (major == ZYRE_VERSION_MAJOR);
    assert (minor == ZYRE_VERSION_MINOR);
    assert (patch == ZYRE_VERSION_PATCH);
    
    //  Create two nodes
    zyre_t *node1 = zyre_new ("node1");
    assert (node1);
    assert (streq (zyre_name (node1), "node1"));
    zyre_set_header (node1, "X-HELLO", "World");
    if (verbose)
        zyre_set_verbose (node1);
    
    //  Set inproc endpoint for this node
    int rc = zyre_set_endpoint (node1, "inproc://zyre-node1");
    assert (rc == 0);
    //  Set up gossip network for this node
    zyre_gossip_bind (node1, "inproc://gossip-hub");
    rc = zyre_start (node1);
    assert (rc == 0);

    zyre_t *node2 = zyre_new ("node2");
    assert (node2);
    assert (streq (zyre_name (node2), "node2"));
    if (verbose)
        zyre_set_verbose (node2);
    
    //  Set inproc endpoint for this node
    //  First, try to use existing name, it'll fail
    rc = zyre_set_endpoint (node2, "inproc://zyre-node1");
    assert (rc == -1);
    //  Now use available name and confirm that it succeeds
    rc = zyre_set_endpoint (node2, "inproc://zyre-node2");
    assert (rc == 0);
   
    //  Set up gossip network for this node
    zyre_gossip_connect (node2, "inproc://gossip-hub");
    rc = zyre_start (node2);
    assert (rc == 0);
    assert (strneq (zyre_uuid (node1), zyre_uuid (node2)));
    
    zyre_join (node1, "GLOBAL");
    zyre_join (node2, "GLOBAL");

    //  Give time for them to interconnect
    zclock_sleep (100);
    if (verbose)
        zyre_dump (node1);

    zlist_t *peers = zyre_peers (node1);
    assert (peers);
    assert (zlist_size (peers) == 1);
    zlist_destroy (&peers);

    zyre_join (node1, "node1 group of one");
    zyre_join (node2, "node2 group of one");

    // Give them time to join their groups
    zclock_sleep (100);

    zlist_t *own_groups = zyre_own_groups (node1);
    assert (own_groups);
    assert (zlist_size (own_groups) == 2);
    zlist_destroy (&own_groups);

    zlist_t *peer_groups = zyre_peer_groups (node1);
    assert (peer_groups);
    assert (zlist_size (peer_groups) == 2);
    zlist_destroy (&peer_groups);

    char *value = zyre_peer_header_value (node2, zyre_uuid (node1), "X-HELLO");
    assert (streq (value, "World"));
    zstr_free (&value);

    //  One node shouts to GLOBAL
    zyre_shouts (node1, "GLOBAL", "Hello, World");

    //  Second node should receive ENTER, JOIN, and SHOUT
    zmsg_t *msg = zyre_recv (node2);
    assert (msg);
    char *command = zmsg_popstr (msg);
    assert (streq (command, "ENTER"));
    zstr_free (&command);
    assert (zmsg_size (msg) == 4);
    char *peerid = zmsg_popstr (msg);
    char *name = zmsg_popstr (msg);
    assert (streq (name, "node1"));
    zstr_free (&name);
    zframe_t *headers_packed = zmsg_pop (msg);
    char *peeraddress = zmsg_popstr (msg);
    char *peerendpoint = zyre_peer_address (node2, peerid);
    assert (streq (peeraddress, peerendpoint));

    zstr_free (&peerid);
    zstr_free (&peerendpoint);
    zstr_free (&peeraddress);

    assert (headers_packed);
    zhash_t *headers = zhash_unpack (headers_packed);
    assert (headers);
    zframe_destroy (&headers_packed);
    assert (streq ((char*) zhash_lookup (headers, "X-HELLO"), "World"));
    zhash_destroy (&headers);
    zmsg_destroy (&msg);

    msg = zyre_recv (node2);
    assert (msg);
    command = zmsg_popstr (msg);
    assert (streq (command, "JOIN"));
    zstr_free (&command);
    assert (zmsg_size (msg) == 3);
    zmsg_destroy (&msg);

    msg = zyre_recv (node2);
    assert (msg);
    command = zmsg_popstr (msg);
    assert (streq (command, "JOIN"));
    zstr_free (&command);
    assert (zmsg_size (msg) == 3);
    zmsg_destroy (&msg);

    msg = zyre_recv (node2);
    assert (msg);
    command = zmsg_popstr (msg);
    assert (streq (command, "SHOUT"));
    zstr_free (&command);
    zmsg_destroy (&msg);
    
    zyre_stop (node1);
    zyre_stop (node2);
    
    zyre_destroy (&node1);
    zyre_destroy (&node2);
    //  @end
    printf ("OK\n");
}
