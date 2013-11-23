/*  =========================================================================
    zyre_msg.h - Parsing Zyre messages

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
@discuss
@end
*/

#include "zyre_classes.h"

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
//  Constructor, creates a new Zyre message.

zyre_msg_t *
zyre_msg_new ()
{
    zyre_msg_t *self = (zyre_msg_t *) zmalloc (sizeof (zyre_msg_t));
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
//  Receive next message from network; the message may be a control
//  message (ENTER, EXIT, JOIN, LEAVE) or data (WHISPER, SHOUT).
//  Returns zmsg_t object, or NULL if interrupted

zyre_msg_t *
zyre_msg_recv (zyre_t *self)
{
    assert (self);
    zmsg_t *msg = zyre_recv (self);
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
zyre_msg_test (bool verbose)
{
    printf (" * zyre_msg: ");

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
    zmsg_addstr (msg, "GLOBAL");
    zmsg_addstr (msg, "Hello, World");
    zyre_shout (node1, &msg);

    //  TODO: should timeout and not hang if there's no networking
    //  ALSO why doesn't this work with localhost? zbeacon?
    //  Second node should receive ENTER, JOIN, and SHOUT
   
    // parse ENTER 
    zyre_msg_t *zyre_msg = zyre_msg_recv (node2);
    msg = zyre_msg_data (zyre_msg);
    assert (streq (zyre_msg_cmd (zyre_msg) , "ENTER"));
    char *peerid = zyre_msg_peerid (zyre_msg);
    zhash_t *headers = zyre_msg_headers (zyre_msg);
    assert (streq (zhash_lookup (headers, "X-HELLO"), "World"));
    zyre_msg_destroy (&zyre_msg);
    
    // parse JOIN
    zyre_msg = zyre_msg_recv (node2);
    assert (streq (zyre_msg_cmd (zyre_msg), "JOIN"));
    zyre_msg_destroy (&zyre_msg);
    
    // parse SHOUT
    zyre_msg = zyre_msg_recv (node2);
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

