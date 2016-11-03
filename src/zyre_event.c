/*  =========================================================================
    zyre_event.h - Parsing Zyre messages

    -------------------------------------------------------------------------
    Copyright (c) the Contributors as noted in the AUTHORS file.

    This file is part of Zyre, an open-source framework for proximity-based
    peer-to-peer applications -- See http://zyre.org.

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
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

//  --------------------------------------------------------------------------
//  Structure of zyre_event class

struct _zyre_event_t {
    char *type;             //  Event type as string
    char *peer_uuid;        //  Sender UUID as string
    char *peer_name;        //  Sender public name as string
    char *peer_addr;        //  Sender ipaddress as string, for an ENTER event
    zhash_t *headers;       //  Headers, for an ENTER event
    char *group;            //  Group name for a SHOUT event
    zmsg_t *msg;            //  Message payload for SHOUT or WHISPER
};


//  --------------------------------------------------------------------------
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

    self->type = zmsg_popstr (msg);
    self->peer_uuid = zmsg_popstr (msg);
    self->peer_name = zmsg_popstr (msg);

    if (streq (self->type, "ENTER")) {
        zframe_t *headers = zmsg_pop (msg);
        if (headers) {
            self->headers = zhash_unpack (headers);
            zframe_destroy (&headers);
        }
        self->peer_addr = zmsg_popstr (msg);
    }
    else
    if (streq (self->type, "JOIN"))
        self->group = zmsg_popstr (msg);
    else
    if (streq (self->type, "LEAVE"))
        self->group = zmsg_popstr (msg);
    else
    if (streq (self->type, "WHISPER")) {
        self->msg = msg;
        msg = NULL;
    }
    else
    if (streq (self->type, "SHOUT")) {
        self->group = zmsg_popstr (msg);
        self->msg = msg;
        msg = NULL;
    }
    zmsg_destroy (&msg);
    return self;
}


//  --------------------------------------------------------------------------
//  Destructor; destroys an event instance

void
zyre_event_destroy (zyre_event_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zyre_event_t *self = *self_p;
        zhash_destroy (&self->headers);
        zmsg_destroy (&self->msg);
        free (self->peer_uuid);
        free (self->peer_name);
        free (self->peer_addr);
        free (self->group);
        free (self->type);
        free (self);
        *self_p = NULL;
    }
}

static int
zyre_event_log_pair (const char *key, void *item, void *argument)
{
    zsys_info ("   - %s: %s", key, (char *) item);
    return 0;
}


//  --------------------------------------------------------------------------
//  Print event to zsys log

void
zyre_event_print (zyre_event_t *self)
{
    zsys_info ("zyre_event:");
    zsys_info (" - from name=%s uuid=%s",
        zyre_event_peer_name (self),
        zyre_event_peer_uuid (self));
    zsys_info (" - type=%s", self->type);

    if (streq (self->type, "ENTER")) {
        void *item;
        zsys_info (" - headers=%zu:", zhash_size (self->headers));
        for (item = zhash_first (self->headers); item != NULL;
                item = zhash_next (self->headers))
            zyre_event_log_pair (zhash_cursor (self->headers), item, self);
        zsys_info (" - address=%s", zyre_event_peer_addr (self));
    }
    else
    if (streq (self->type, "JOIN")) {
        zsys_info (" - group=%s", zyre_event_group(self));
    }
    else
    if (streq (self->type, "LEAVE")) {
        zsys_info (" - group=%s", zyre_event_group(self));
    }
    else
    if (streq (self->type, "SHOUT")) {
        zsys_info (" - message:");
        zmsg_print (self->msg);
    }
    else
    if (streq (self->type, "WHISPER")) {
        zsys_info (" - message:");
        zmsg_print (self->msg);
    }
}


//  --------------------------------------------------------------------------
//  Returns event type, as printable uppercase string

const char *
zyre_event_type (zyre_event_t *self)
{
    assert (self);
    return self->type;
}


//  --------------------------------------------------------------------------
//  Return the sending peer's UUID as a string

const char *
zyre_event_peer_uuid (zyre_event_t *self)
{
    assert (self);
    return self->peer_uuid;
}


//  --------------------------------------------------------------------------
//  Return the sending peer's public name as a string

const char *
zyre_event_peer_name (zyre_event_t *self)
{
    assert (self);
    return self->peer_name;
}


//  --------------------------------------------------------------------------
//  Return the sending peer's ipaddress as a string

const char *
zyre_event_peer_addr (zyre_event_t *self)
{
    assert (self);
    return self->peer_addr;
}


//  --------------------------------------------------------------------------
//  Returns the event headers, or NULL if there are none

zhash_t *
zyre_event_headers (zyre_event_t *self)
{
    assert (self);
    return self->headers;
}


//  --------------------------------------------------------------------------
//  Returns value of a header from the message headers
//  obtained by ENTER. Return NULL if no value was found.

const char *
zyre_event_header (zyre_event_t *self, const char *name)
{
    assert (self);
    if (!self->headers)
        return NULL;
    return (const char *) zhash_lookup (self->headers, name);
}


//  --------------------------------------------------------------------------
//  Returns the group name that a SHOUT event was sent to

const char *
zyre_event_group (zyre_event_t *self)
{
    assert (self);
    return self->group;
}


//  --------------------------------------------------------------------------
//  Returns the incoming message payload; the caller can modify the message
//  but does not own it and should not destroy it.

zmsg_t *
zyre_event_msg (zyre_event_t *self)
{
    assert (self);
    return self->msg;
}


//  --------------------------------------------------------------------------
//  Returns the incoming message payload, and pass ownership to the caller.
//  The caller must destroy the message when finished with it. After called
//  on the given event, further calls will return NULL.

zmsg_t *
zyre_event_get_msg (zyre_event_t *self)
{
    assert (self);
    zmsg_t *msg = self->msg;
    self->msg = NULL;
    return msg;
}


//  --------------------------------------------------------------------------
//  Self test of this class

void
zyre_event_test (bool verbose)
{
    printf (" * zyre_event: ");

    //  @selftest
    //  Create two nodes
    zyre_t *node1 = zyre_new ("node1");
    assert (node1);
    zyre_set_header (node1, "X-HELLO", "World");
    // use gossiping instead of beaconing, suits Travis better
    zyre_gossip_bind (node1, "inproc://gossip-hub");
    if (verbose)
        zyre_set_verbose (node1);
    if (zyre_start (node1)) {
        zyre_destroy (&node1);
        printf ("OK (skipping test, no UDP discovery)\n");
        return;
    }
    zyre_join (node1, "GLOBAL");

    zyre_t *node2 = zyre_new ("node2");
    assert (node2);
    if (verbose)
        zyre_set_verbose (node2);
    // use gossiping instead of beaconing, suits Travis better
    zyre_gossip_connect (node2, "inproc://gossip-hub");
    int rc = zyre_start (node2);
    assert (rc == 0);
    zyre_join (node2, "GLOBAL");

    //  Give time for them to interconnect
    zclock_sleep (250);

    //  One node shouts to GLOBAL
    zmsg_t *msg = zmsg_new ();
    zmsg_addstr (msg, "Hello, World");
    zyre_shout (node1, "GLOBAL", &msg);
    zclock_sleep (100);

    //  Parse ENTER
    zyre_event_t *event = zyre_event_new (node2);
    assert (streq (zyre_event_type (event), "ENTER"));
    const char *sender = zyre_event_peer_uuid (event);
    assert (sender);
    const char *name = zyre_event_peer_name (event);
    assert (name);
    assert (streq (name, "node1"));
    const char *address = zyre_event_peer_addr (event);
    assert (address);
    const char *header = zyre_event_header (event, "X-HELLO");
    assert (header);
    zyre_event_destroy (&event);

    //  Parse JOIN
    //  We tolerate other events, which we can get if there are instances
    //  of Zyre running somewhere on the network.
    event = zyre_event_new (node2);
    if (streq (zyre_event_type (event), "JOIN")) {
        //  Parse SHOUT
        zyre_event_destroy (&event);
        event = zyre_event_new (node2);
        if (streq (zyre_event_type (event), "SHOUT")) {
            assert (streq (zyre_event_group (event), "GLOBAL"));
            zmsg_t *msg = zyre_event_get_msg (event);
            char *string = zmsg_popstr (msg);
            zmsg_destroy (&msg);
            assert (streq (string, "Hello, World"));
            free (string);
        }
        zyre_event_destroy (&event);
    }
    zyre_destroy (&node1);
    zyre_destroy (&node2);
    //  @end
    printf ("OK\n");
}

