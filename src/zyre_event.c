/*  =========================================================================
    zyre_event.h - Parsing Zyre messages

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
    This class provides a higher-level API to the zyre_recv call, by doing
    work that you will want to do in many cases, such as unpacking the peer
    headers for each ENTER event received.
@discuss
@end
*/

#include "zyre_classes.h"

//  ---------------------------------------------------------------------
//  Structure of zyre_event class

struct _zyre_event_t {
    zyre_event_type_t type; //  Event type
    char *sender;           //  Sender UUID as string
    char *address;          //  Sender ipaddress as string, for an ENTER event
    zhash_t *headers;       //  Headers, for an ENTER event
    char *group;            //  Group name for a SHOUT event
    zmsg_t *msg;            //  Message payload for SHOUT or WHISPER
};


//  ---------------------------------------------------------------------
//  Constructor: receive an event from the zyre node, wraps zyre_recv.
//  The event may be a control message (ENTER, EXIT, JOIN, LEAVE) or
//  data (WHISPER, SHOUT).

zyre_event_t *
zyre_event_new (zyre_t *node)
{
    zmsg_t *msg = zyre_recv (node);
    if (!msg)
        return NULL;            //  Interrupted

    zyre_event_t *self = (zyre_event_t *) zmalloc (sizeof (zyre_event_t));
    assert (self);

    char *type = zmsg_popstr (msg);
    self->sender = zmsg_popstr (msg);

    if (streq (type, "ENTER")) {
        self->type = ZYRE_EVENT_ENTER;
        zframe_t *headers = zmsg_pop (msg);
        self->headers = zhash_unpack (headers);
        zframe_destroy (&headers);
        self->address = zmsg_popstr (msg);
    }
    else
    if (streq (type, "EXIT"))
        self->type = ZYRE_EVENT_EXIT;
    else
    if (streq (type, "JOIN")) {
        self->type = ZYRE_EVENT_JOIN;
        self->group = zmsg_popstr (msg);
    }
    else
    if (streq (type, "LEAVE")) {
        self->type = ZYRE_EVENT_LEAVE;
        self->group = zmsg_popstr (msg);
    }
    else
    if (streq (type, "WHISPER")) {
        self->type = ZYRE_EVENT_WHISPER;
        self->msg = msg;
        msg = NULL;
    }
    else
    if (streq (type, "SHOUT")) {
        self->type = ZYRE_EVENT_SHOUT;
        self->group = zmsg_popstr (msg);
        self->msg = msg;
        msg = NULL;
    }
    free (type);
    zmsg_destroy (&msg);
    return self;
}


//  ---------------------------------------------------------------------
//  Destructor; destroys an event instance

void
zyre_event_destroy (zyre_event_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zyre_event_t *self = *self_p;
        free (self->sender);
        free (self->address);
        free (self->group);
        zhash_destroy (&self->headers);
        zmsg_destroy (&self->msg);
        free (self);
        *self_p = NULL;
    }
}


//  ---------------------------------------------------------------------
//  Returns event type, which is a zyre_event_type_t

zyre_event_type_t
zyre_event_type (zyre_event_t *self)
{
    assert (self);
    return self->type;
}


//  ---------------------------------------------------------------------
//  Return the sending peer's id as a string

char *
zyre_event_sender (zyre_event_t *self)
{
    assert (self);
    return self->sender;
}


//  ---------------------------------------------------------------------
//  Return the sending peer's ipaddress as a string

char *
zyre_event_address (zyre_event_t *self)
{
    assert (self);
    return self->address;
}


//  ---------------------------------------------------------------------
//  Returns the event headers, or NULL if there are none

zhash_t *
zyre_event_headers (zyre_event_t *self)
{
    assert (self);
    return self->headers;
}


//  ---------------------------------------------------------------------
//  Returns value of a header from the message headers
//  obtained by ENTER. Return NULL if no value was found.

char *
zyre_event_header (zyre_event_t *self, char *name)
{
    assert (self);
    return zhash_lookup (self->headers, name);
}


//  ---------------------------------------------------------------------
//  Returns the group name that a SHOUT event was sent to

char *
zyre_event_group (zyre_event_t *self)
{
    assert (self);
    return self->group;
}

//  ---------------------------------------------------------------------
//  Returns the incoming message payload (currently one frame)

zmsg_t *
zyre_event_msg (zyre_event_t *self)
{
    assert (self);
    return self->msg;
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
    zyre_shout (node1, "GLOBAL", &msg);

    //  Parse ENTER
    zyre_event_t *event = zyre_event_new (node2);
    assert (zyre_event_type (event) == ZYRE_EVENT_ENTER);
    char *sender = zyre_event_sender (event);
    assert (sender);
    char *address = zyre_event_address (event);
    assert (address);
    assert (streq (zyre_event_header (event, "X-HELLO"), "World"));
    msg = zyre_event_msg (event);
    zyre_event_destroy (&event);
    
    //  Parse JOIN
    event = zyre_event_new (node2);
    assert (zyre_event_type (event) == ZYRE_EVENT_JOIN);
    zyre_event_destroy (&event);
    
    //  Parse SHOUT
    event = zyre_event_new (node2);
    assert (zyre_event_type (event) == ZYRE_EVENT_SHOUT);
    assert (streq (zyre_event_group (event), "GLOBAL"));
    msg = zyre_event_msg (event);
    char *string = zmsg_popstr (msg);
    assert (streq (string, "Hello, World"));
    free (string);
    zyre_event_destroy (&event);
    
    zyre_destroy (&node1);
    zyre_destroy (&node2);
    zctx_destroy (&ctx);
    //  @end
    printf ("OK\n");
}

