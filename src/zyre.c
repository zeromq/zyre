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

        ENTER fromnode headers ipaddress
            a new peer has entered the network
        EXIT fromnode
            a peer has left the network
        JOIN fromnode groupname
            a peer has joined a specific group
        LEAVE fromnode groupname
            a peer has joined a specific group
        WHISPER fromnode message
            a peer has sent this node a message
        SHOUT fromnode groupname message
            a peer has sent one of our groups a message
            
    In SHOUT and WHISPER the message is a single frame in this version
    of Zyre. In ENTER, the headers frame contains a packed dictionary,
    see zhash_pack/unpack.

    To join or leave a group, use the zyre_join and zyre_leave methods.
    To set a header value, use the zyre_set_header method. To send a message
    to a single peer, use zyre_whisper. To send a message to a group, use
    zyre_shout.
@discuss
    Todo: allow multipart contents
@end
*/

#include "zyre_classes.h"

//  ---------------------------------------------------------------------
//  Structure of our class

struct _zyre_t {
    void *pipe;                 //  Pipe through to node
};


//  ---------------------------------------------------------------------
//  Constructor, creates a new Zyre node. Note that until you start the
//  node it is silent and invisible to other nodes on the network.

zyre_t *
zyre_new (zctx_t *ctx)
{
    zyre_t *self = (zyre_t *) zmalloc (sizeof (zyre_t));
    assert (self);

    //  Start node engine and wait for it to be ready
    assert (ctx);
    self->pipe = zthread_fork (ctx, zyre_node_engine, NULL);
    if (self->pipe)
        zsocket_wait (self->pipe);
    else {
        free (self);
        self = NULL;
    }
    return self;
}


//  ---------------------------------------------------------------------
//  Destructor, destroys a Zyre node. When you destroy a node, any
//  messages it is sending or receiving will be discarded.

void
zyre_destroy (zyre_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zyre_t *self = *self_p;
        zstr_send (self->pipe, "TERMINATE");
        zsocket_wait (self->pipe);
        free (self);
        *self_p = NULL;
    }
}


//  ---------------------------------------------------------------------
//  Set node header; these are provided to other nodes during discovery
//  and come in each ENTER message.

void
zyre_set_header (zyre_t *self, char *name, char *format, ...)
{
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    char *string = zsys_vprintf (format, argptr);
    va_end (argptr);

    zstr_sendm (self->pipe, "SET");
    zstr_sendm (self->pipe, name);
    zstr_send  (self->pipe, string);
    free (string);
}


//  ---------------------------------------------------------------------
//  Set verbose mode; this tells the node to log all traffic as well
//  as all major events.

void
zyre_set_verbose (zyre_t *self)
{
    zstr_send (self->pipe, "VERBOSE");
    zsocket_wait (self->pipe);
}


//  ---------------------------------------------------------------------
//  Start node, after setting header values. When you start a node it
//  begins discovery and connection.

void
zyre_start (zyre_t *self)
{
    zstr_send (self->pipe, "START");
    zsocket_wait (self->pipe);
}


//  ---------------------------------------------------------------------
//  Stop node; this signals to other peers that this node will go away.
//  This is polite; however you can also just destroy the node without
//  stopping it.

void
zyre_stop (zyre_t *self)
{
    zstr_send (self->pipe, "STOP");
    zsocket_wait (self->pipe);
}


//  ---------------------------------------------------------------------
//  Join a named group; after joining a group you can send messages to
//  the group and all Zyre nodes in that group will receive them.

int
zyre_join (zyre_t *self, const char *group)
{
    assert (self);
    zstr_sendm (self->pipe, "JOIN");
    zstr_send  (self->pipe, group);
    return 0;
}


//  ---------------------------------------------------------------------
//  Leave a group

int
zyre_leave (zyre_t *self, const char *group)
{
    assert (self);
    zstr_sendm (self->pipe, "LEAVE");
    zstr_send  (self->pipe, group);
    return 0;
}


//  ---------------------------------------------------------------------
//  Receive next message from network; the message may be a control
//  message (ENTER, EXIT, JOIN, LEAVE) or data (WHISPER, SHOUT).
//  Returns zmsg_t object, or NULL if interrupted

zmsg_t *
zyre_recv (zyre_t *self)
{
    assert (self);
    zmsg_t *msg = zmsg_recv (self->pipe);
    return msg;
}


//  ---------------------------------------------------------------------
//  Send message to single peer, specified as a UUID string
//  Destroys message after sending

int
zyre_whisper (zyre_t *self, char *peer, zmsg_t **msg_p)
{
    assert (self);
    assert (peer);
    zstr_sendm (self->pipe, "WHISPER");
    zstr_sendm (self->pipe, peer);
    zmsg_send (msg_p, self->pipe);
    return 0;
}


//  ---------------------------------------------------------------------
//  Send message to a named group
//  Destroys message after sending

int
zyre_shout (zyre_t *self, char *group, zmsg_t **msg_p)
{
    assert (self);
    assert (group);
    zstr_sendm (self->pipe, "SHOUT");
    zstr_sendm (self->pipe, group);
    zmsg_send (msg_p, self->pipe);
    return 0;
}


//  ---------------------------------------------------------------------
//  Send string to single peer specified as a UUID string.
//  String is formatted using printf specifiers.

int
zyre_whispers (zyre_t *self, char *peer, char *format, ...)
{
    assert (self);
    assert (peer);
    assert (format);

    va_list argptr;
    va_start (argptr, format);
    char *string = zsys_vprintf (format, argptr);
    va_end (argptr);

    zstr_sendm (self->pipe, "WHISPER");
    zstr_sendm (self->pipe, peer);
    zstr_send  (self->pipe, string);
    free (string);
    return 0;
}


//  ---------------------------------------------------------------------
//  Send message to a named group
//  Destroys message after sending

int
zyre_shouts (zyre_t *self, char *group, char *format, ...)
{
    assert (self);
    assert (group);
    assert (format);

    va_list argptr;
    va_start (argptr, format);
    char *string = zsys_vprintf (format, argptr);
    va_end (argptr);

    zstr_sendm (self->pipe, "SHOUT");
    zstr_sendm (self->pipe, group);
    zstr_send  (self->pipe, string);
    free (string);
    return 0;
}


//  ---------------------------------------------------------------------
//  Return node handle, for polling

void *
zyre_socket (zyre_t *self)
{
    assert (self);    
    return self->pipe;
}


//  --------------------------------------------------------------------------
//  Self test of this class

void
zyre_test (bool verbose)
{
    printf (" * zyre: ");

    //  @selftest
    zctx_t *ctx = zctx_new ();
    //  Create two nodes
    zyre_t *node1 = zyre_new (ctx);
    zyre_t *node2 = zyre_new (ctx);
    zyre_set_header (node1, "X-HELLO", "World");
//     zyre_set_verbose (node1);
//     zyre_set_verbose (node2);
    zyre_start (node1);
    zyre_start (node2);
    zyre_join (node1, "GLOBAL");
    zyre_join (node2, "GLOBAL");

    //  Give time for them to interconnect
    zclock_sleep (250);

    //  One node shouts to GLOBAL
    zyre_shouts (node1,"GLOBAL", "Hello, World");

    //  TODO: should timeout and not hang if there's no networking
    //  ALSO why doesn't this work with localhost? zbeacon?
    //  Second node should receive ENTER, JOIN, and SHOUT
    zmsg_t *msg = zyre_recv (node2);
    assert (msg);
    char *command = zmsg_popstr (msg);
    assert (streq (command, "ENTER"));
    zstr_free (&command);
    char *peerid = zmsg_popstr (msg);
    zstr_free (&peerid);
    zframe_t *headers_packed = zmsg_pop (msg);
    char *peeraddress = zmsg_popstr (msg);
    zstr_free (&peeraddress);

    assert (headers_packed);
    zhash_t *headers = zhash_unpack (headers_packed);
    assert (headers);
    zframe_destroy (&headers_packed);
    assert (streq (zhash_lookup (headers, "X-HELLO"), "World"));
    zhash_destroy (&headers);
    zmsg_destroy (&msg);
    
    msg = zyre_recv (node2);
    assert (msg);
    command = zmsg_popstr (msg);
    assert (streq (command, "JOIN"));
    zstr_free (&command);
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
    zctx_destroy (&ctx);
    //  @end
    printf ("OK\n");
}

