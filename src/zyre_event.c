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
    This class provides a higher-level API to the zyre_recv call, by doing
    work that you will want to do in many cases, such as unpacking the peer
    headers for each ENTER event received.
@discuss
    Sending via zyre_event_t might be cumbersome. Alternatively we could use
    zyre_event_whisper (zmsg, recevier_uuid) and 
    zyre_event_shout (zmsg, group_name)

    Also we should deliver all of the payload frames.
@end
*/

#include "zyre_classes.h"

//  ---------------------------------------------------------------------
//  Structure of zyre_event class

struct _zyre_event_t {
    zyre_event_type_t type; //  Event type
    char *sender;           //  Sender UUID as string
    zhash_t *headers;       //  Headers, for a ENTER event
    char *group;            //  Group name for a SHOUT event
    zmsg_t *msg;            //  Message payload for SHOUT or WHISPER
};


//  ---------------------------------------------------------------------
//  Constructor; creates a new event of a specified type

zyre_event_t *
zyre_event_new (zyre_event_type_t type)
{
    zyre_event_t *self = (zyre_event_t *) zmalloc (sizeof (zyre_event_t));
    assert (self);
    self->type = type;
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
        free (self->group);
        zhash_destroy (&self->headers);
        zmsg_destroy (&self->msg);
        free (self);
        *self_p = NULL;
    }
}


//  ---------------------------------------------------------------------
//  Receive an event from the zyre node, wraps zyre_recv.
//  The event may be a control message (ENTER, EXIT, JOIN, LEAVE)
//  or data (WHISPER, SHOUT).

zyre_event_t *
zyre_event_recv (zyre_t *self)
{
    assert (self);
    zmsg_t *msg = zyre_recv (self);
    zyre_event_t *event = zyre_event_new (0);
    char *type = zmsg_popstr (msg);
    event->sender = zmsg_popstr (msg);

    if (streq (type, "ENTER")) {
        event->type = ZYRE_EVENT_ENTER;
        zframe_t *headers = zmsg_pop (msg);
        event->headers = zhash_unpack (headers);
        zframe_destroy (&headers);
    }
    else
    if (streq (type, "EXIT"))
        event->type = ZYRE_EVENT_EXIT;
    else
    if (streq (type, "JOIN")) {
        event->type = ZYRE_EVENT_JOIN;
        event->group = zmsg_popstr (msg);
    }
    else
    if (streq (type, "LEAVE")) {
        event->type = ZYRE_EVENT_LEAVE;
        event->group = zmsg_popstr (msg);
    }
    else
    if (streq (type, "WHISPER")) {
        event->type = ZYRE_EVENT_WHISPER;
        event->msg = msg;
        msg = NULL;
    }
    else
    if (streq (type, "SHOUT")) {
        event->type = ZYRE_EVENT_SHOUT;
        event->group = zmsg_popstr (msg);
        event->msg = msg;
        msg = NULL;
    }
    free (type);
    zmsg_destroy (&msg);
    return event;
}

//  ---------------------------------------------------------------------
//  Sends an zyre whisper. Returns 0 if succesful else 1.
//  Destroys msg after sending

int
zyre_event_send_whisper (zyre_t *zyre, zmsg_t *msg, char *receiver) 
{
    assert (zyre);
    assert (msg);

    zmsg_pushstr (msg, receiver);
    int rc = zyre_whisper (zyre, &msg); 
    zmsg_destroy (&msg);
    return rc;
}

//  ---------------------------------------------------------------------
//  Sends an zyre shout. Returns 0 if succesful else 1.
//  Destroys msg after sending

int
zyre_event_send_shout (zyre_t *zyre, zmsg_t *msg, char *group)
{
    assert (zyre);
    assert (msg);
    
    zmsg_pushstr (msg, group);
    int rc = zyre_shout (zyre, &msg);
    zmsg_destroy (&msg);
    return rc;
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
    zyre_event_send_shout (node1, msg, "GLOBAL");

    //  Parse ENTER
    zyre_event_t *zyre_event = zyre_event_recv (node2);
    assert (zyre_event_type (zyre_event) == ZYRE_EVENT_ENTER);
    char *sender = zyre_event_sender (zyre_event);
    assert (streq (zyre_event_header (zyre_event, "X-HELLO"), "World"));
    msg = zyre_event_msg (zyre_event);
    zyre_event_destroy (&zyre_event);
    
    //  Parse JOIN
    zyre_event = zyre_event_recv (node2);
    assert (zyre_event_type (zyre_event) == ZYRE_EVENT_JOIN);
    zyre_event_destroy (&zyre_event);
    
    //  Parse SHOUT
    zyre_event = zyre_event_recv (node2);
    assert (zyre_event_type (zyre_event) == ZYRE_EVENT_SHOUT);
    assert (streq (zyre_event_group (zyre_event), "GLOBAL"));
    msg = zyre_event_msg (zyre_event);
    assert (streq (zmsg_popstr (msg), "Hello, World"));
    zyre_event_destroy (&zyre_event);
    
    zyre_destroy (&node1);
    zyre_destroy (&node2);
    zctx_destroy (&ctx);
    //  @end
    printf ("OK\n");
}

