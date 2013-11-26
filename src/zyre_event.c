/*  =========================================================================
    zyre_event.h - Parsing Zyre messages

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
//  Structure of zyre_event class

struct _zyre_event_t {
    int command;      // command type of the message
    char *peerid;         // uuid from router
    zhash_t *headers;   // headers send by enter 
    char *group;        // group name send by shout
    zmsg_t *data;       // actual message data
};

//  ---------------------------------------------------------------------
//  Constructor, creates a new Zyre message.

zyre_event_t *
zyre_event_new ()
{
    zyre_event_t *self = (zyre_event_t *) zmalloc (sizeof (zyre_event_t));
    assert (self);

    return self;
}

//  ---------------------------------------------------------------------
//  Destructor, destroys a Zyre message.

void
zyre_event_destroy (zyre_event_t **self_p) 
{
    assert (self_p);
    if (*self_p) {
        zyre_event_t *self = *self_p;
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

zyre_event_t *
zyre_event_recv (zyre_t *self)
{
    assert (self);
    zmsg_t *msg = zyre_recv (self);
    zyre_event_t *zyre_event = zyre_event_new (); 
    char *command = zmsg_popstr (msg);
    zyre_event->peerid = zmsg_popstr (msg);
    
    if (streq (command, "ENTER")) {
        zyre_event->command = ZYRE_EVENT_ENTER;
        // get and unpack headers
        zframe_t *headers_packed = zmsg_pop (msg);
        zyre_event->headers = zhash_dup (zhash_unpack (headers_packed));
        // cleanup
        zframe_destroy (&headers_packed);
    }
    else
    if (streq (command, "JOIN")) {
       zyre_event->command = ZYRE_EVENT_JOIN;
    }
    else
    if (streq (command, "LEAVE")) {
        zyre_event->command = ZYRE_EVENT_LEAVE;
    }
    else
    if (streq (command, "EXIT")) {
        zyre_event->command = ZYRE_EVENT_EXIT;
    }
    else
    if (streq (command, "WHISPER")) {
        zyre_event->command = ZYRE_EVENT_WHISPER;
    }
    else 
    if (streq (command, "SHOUT")) {
        zyre_event->command = ZYRE_EVENT_SHOUT;
        zyre_event->group = zmsg_popstr (msg);        
    }

    
    // rest of the message is data
    zyre_event->data = msg;

    return zyre_event;
}

//  ---------------------------------------------------------------------
//  Gets the message type

int
zyre_event_cmd (zyre_event_t *self)
{   
    assert (self);
    return self->command;
}

//  ---------------------------------------------------------------------
//  Gets the message peer uuid

char *
zyre_event_peerid (zyre_event_t *self) 
{
    assert (self);
    return self->peerid;
}

//  ---------------------------------------------------------------------
//  Gets the message headers

zhash_t *
zyre_event_headers (zyre_event_t *self)
{
    assert (self);
    return self->headers;  
}

//  ---------------------------------------------------------------------
//  Gets message a header value from the headers 
//  obtained by ENTER message.

char *
zyre_event_get_header (zyre_event_t *self, char *name) 
{
    assert (self);
    zhash_t *headers = zyre_event_headers (self);
    return zhash_lookup (headers, name);
}

//  ---------------------------------------------------------------------
//  Gets the message group in case of shout

char *
zyre_event_group (zyre_event_t *self) 
{
    assert (self);
    return self->group;
}

//  ---------------------------------------------------------------------
//  Gets the actual message data

zmsg_t *
zyre_event_data (zyre_event_t *self) 
{
    assert (self);
    return self->data;
}

//  --------------------------------------------------------------------------
//  Self test of this class

void
zyre_event_test (bool verbose)
{
    printf (" * zyre_event: ");

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
    zyre_event_t *zyre_event = zyre_event_recv (node2);
    msg = zyre_event_data (zyre_event);
    assert (zyre_event_cmd (zyre_event) == ZYRE_EVENT_ENTER);
    char *peerid = zyre_event_peerid (zyre_event);
    assert (streq (zyre_event_get_header (zyre_event, "X-HELLO"), "World"));
    zyre_event_destroy (&zyre_event);
    
    // parse JOIN
    zyre_event = zyre_event_recv (node2);
    assert (zyre_event_cmd (zyre_event) == ZYRE_EVENT_JOIN);
    zyre_event_destroy (&zyre_event);
    
    // parse SHOUT
    zyre_event = zyre_event_recv (node2);
    msg = zyre_event_data (zyre_event);
    assert (zyre_event_cmd (zyre_event) == ZYRE_EVENT_SHOUT);
    assert (streq (zyre_event_group (zyre_event), "GLOBAL"));
    zyre_event_destroy (&zyre_event);
    
    zyre_destroy (&node1);
    zyre_destroy (&node2);
    zctx_destroy (&ctx);
    //  @end
    printf ("OK\n");
}

