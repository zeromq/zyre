/*  =========================================================================
    zyre - API wrapping one Zyre node

    -------------------------------------------------------------------------
    Copyright (c) 1991-2013 iMatix Corporation <www.imatix.com>
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

        ENTER fromnode headers
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
//  Structure of zyre_msg class

struct _zyre_msg_t {
    char *command;      // command type of the message
    char *peerid;         // uuid from router
    zhash_t *headers;   // headers send by enter 
    char *group;        // group name send by shout
    zmsg_t *data;       // actual message data
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
    if (self->pipe) {
        char *status = zstr_recv (self->pipe);
        if (strneq (status, "OK"))
            zyre_destroy (&self);
        zstr_free (&status);
    }
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
        char *reply = zstr_recv (self->pipe);
        zstr_free (&reply);
        free (self);
        *self_p = NULL;
    }
}

//  ---------------------------------------------------------------------
//  Constructor, creates a new Zyre message.

zyre_msg_t *
zyre_msg_new ()
{
    zyre_msg_t *self = (zyre_msg_t *) zmalloc(sizeof (zyre_t));
    assert (self);

    return self;
}

//  ---------------------------------------------------------------------
//  Destructor, destroys a Zyre message.

void
zyre_msg_destroy (zyre_msg_t **self_p) 
{
    assert (self_p);
    if (*self_p) {
        zyre_msg_t *self = *self_p;
        free (self->command);
        free (self->peerid);
        if (self->headers) 
            zhash_destroy (&self->headers);
        if (self->data)
            zmsg_destroy (&self->data);
        free (self->group);
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
//  Start node, after setting header values. When you start a node it
//  begins discovery and connection. There is no stop method; to stop
//  a node, destroy it.

void
zyre_start (zyre_t *self)
{
    zstr_send (self->pipe, "START");
    char *reply = zstr_recv (self->pipe);
    zstr_free (&reply);
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

zyre_msg_t *
zyre_recv (zyre_t *self)
{
    assert (self);
    zmsg_t *msg = zmsg_recv (self->pipe);
    
    // recveice message, get command and peerid
    zyre_msg_t *zyre_msg = zyre_msg_new (); 
    zyre_msg->command = zmsg_popstr (msg);
    zyre_msg->peerid = zmsg_popstr (msg);
    
    if (streq (zyre_msg->command, "ENTER")) {
        // get and unpack headers
        zframe_t *headers_packed = zmsg_pop (msg);
        zyre_msg->headers = zhash_dup (zhash_unpack (headers_packed));
        // cleanup
        zframe_destroy (&headers_packed);
    } else if (streq (zyre_msg->command, "SHOUT")) {
        zyre_msg->group = zmsg_popstr (msg);        
    }

    
    // rest of the message is data
    zyre_msg->data = msg;

    return zyre_msg;
}


//  ---------------------------------------------------------------------
//  Send message to single peer; peer ID is first frame in message
//  Destroys message after sending

int
zyre_whisper (zyre_t *self, zmsg_t **msg_p, char *peerid)
{
    assert (self);
    zstr_sendm (self->pipe, "WHISPER");
    zmsg_pushstr (*msg_p, peerid);
    zmsg_send (msg_p, self->pipe);
    return 0;
}


//  ---------------------------------------------------------------------
//  Send message to a group of peers

int
zyre_shout (zyre_t *self, zmsg_t **msg_p, char *group)
{
    assert (self);
    zstr_sendm (self->pipe, "SHOUT");
    zmsg_pushstr (*msg_p, group); // push group in fron of message
    zmsg_send (msg_p, self->pipe);
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

//  ---------------------------------------------------------------------
//  Gets the message type

char *
zyre_msg_cmd (zyre_msg_t *self)
{   
    assert (self);
    return self->command;
}

//  ---------------------------------------------------------------------
//  Gets the message peer uuid

char *
zyre_msg_peerid (zyre_msg_t *self) 
{
    assert (self);
    return self->peerid;
}

//  ---------------------------------------------------------------------
//  Gets the message headers

zhash_t *
zyre_msg_headers (zyre_msg_t *self)
{
    assert (self);
    return self->headers;  
}

//  ---------------------------------------------------------------------
//  Gets the message group in case of shout

char *
zyre_msg_group (zyre_msg_t *self) 
{
    assert (self);
    return self->group;
}

//  ---------------------------------------------------------------------
//  Gets the actual message data

zmsg_t *
zyre_msg_data (zyre_msg_t *self) 
{
    assert (self);
    return self->data;
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
    zyre_set_header (node1, "X-FILEMQ", "tcp://128.0.0.1:6777");
    zyre_set_header (node1, "X-HELLO", "World");
    zyre_start (node1);
    zyre_start (node2);
    zyre_join (node1, "GLOBAL");
    zyre_join (node2, "GLOBAL");

    //  Give time for them to interconnect
    zclock_sleep (250);

    //  One node shouts to GLOBAL
    zmsg_t *msg = zmsg_new ();
    zmsg_addstr (msg, "Hello, World");
    zyre_shout (node1, &msg, "GLOBAL");

    //  TODO: should timeout and not hang if there's no networking
    //  ALSO why doesn't this work with localhost? zbeacon?
    //  Second node should receive ENTER, JOIN, and SHOUT
   
    // parse ENTER 
    zyre_msg_t *zyre_msg = zyre_recv (node2);
    msg = zyre_msg_data (zyre_msg);
    assert (streq (zyre_msg_cmd (zyre_msg) , "ENTER"));
    char *peerid = zyre_msg_peerid (zyre_msg);
    zhash_t *headers = zyre_msg_headers (zyre_msg);
    assert (streq (zhash_lookup (headers, "X-HELLO"), "World"));
    zyre_msg_destroy (&zyre_msg);
    
    // parse JOIN
    zyre_msg = zyre_recv (node2);
    assert (streq (zyre_msg_cmd (zyre_msg), "JOIN"));
    zyre_msg_destroy (&zyre_msg);
    
    // parse SHOUT
    zyre_msg = zyre_recv (node2);
    msg = zyre_msg_data (zyre_msg);
    assert (streq (zyre_msg_cmd (zyre_msg), "SHOUT"));
    assert (streq (zyre_msg_group (zyre_msg), "GLOBAL"));
    zyre_msg_destroy (&zyre_msg);
    
    zyre_destroy (&node1);
    zyre_destroy (&node2);
    zctx_destroy (&ctx);
    //  @end
    printf ("OK\n");
}

