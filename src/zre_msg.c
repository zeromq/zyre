/*  =========================================================================
    zre_msg - work with ZRE messages

    Codec class for zre_msg.

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

     * The XML model used for this code generation: zre_msg.xml, or
     * The code generation script that built this file: zproto_codec_c
    ************************************************************************
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
    zre_msg - work with ZRE messages
@discuss
@end
*/

#ifdef NDEBUG
#undef NDEBUG
#endif

#include "../include/zyre.h"
#include "./zre_msg.h"

//  Structure of our class

struct _zre_msg_t {
    zframe_t *routing_id;               //  Routing_id from ROUTER, if any
    int id;                             //  zre_msg message ID
    byte *needle;                       //  Read/write pointer for serialization
    byte *ceiling;                      //  Valid upper limit for read pointer
    uint16_t sequence;                  //  Cyclic sequence number
    char endpoint [256];                //  Sender connect endpoint
    zlist_t *groups;                    //  List of groups sender is in
    byte status;                        //  Sender groups status value
    char name [256];                    //  Sender public name
    zhash_t *headers;                   //  Sender header properties
    size_t headers_bytes;               //  Size of hash content
    zmsg_t *content;                    //  Wrapped message content
    char group [256];                   //  Group to send to
};

//  --------------------------------------------------------------------------
//  Network data encoding macros

//  Put a block of octets to the frame
#define PUT_OCTETS(host,size) { \
    memcpy (self->needle, (host), size); \
    self->needle += size; \
}

//  Get a block of octets from the frame
#define GET_OCTETS(host,size) { \
    if (self->needle + size > self->ceiling) { \
        zsys_warning ("zre_msg: GET_OCTETS failed"); \
        goto malformed; \
    } \
    memcpy ((host), self->needle, size); \
    self->needle += size; \
}

//  Put a 1-byte number to the frame
#define PUT_NUMBER1(host) { \
    *(byte *) self->needle = (byte) (host); \
    self->needle++; \
}

//  Put a 2-byte number to the frame
#define PUT_NUMBER2(host) { \
    self->needle [0] = (byte) (((host) >> 8)  & 255); \
    self->needle [1] = (byte) (((host))       & 255); \
    self->needle += 2; \
}

//  Put a 4-byte number to the frame
#define PUT_NUMBER4(host) { \
    self->needle [0] = (byte) (((host) >> 24) & 255); \
    self->needle [1] = (byte) (((host) >> 16) & 255); \
    self->needle [2] = (byte) (((host) >> 8)  & 255); \
    self->needle [3] = (byte) (((host))       & 255); \
    self->needle += 4; \
}

//  Put a 8-byte number to the frame
#define PUT_NUMBER8(host) { \
    self->needle [0] = (byte) (((host) >> 56) & 255); \
    self->needle [1] = (byte) (((host) >> 48) & 255); \
    self->needle [2] = (byte) (((host) >> 40) & 255); \
    self->needle [3] = (byte) (((host) >> 32) & 255); \
    self->needle [4] = (byte) (((host) >> 24) & 255); \
    self->needle [5] = (byte) (((host) >> 16) & 255); \
    self->needle [6] = (byte) (((host) >> 8)  & 255); \
    self->needle [7] = (byte) (((host))       & 255); \
    self->needle += 8; \
}

//  Get a 1-byte number from the frame
#define GET_NUMBER1(host) { \
    if (self->needle + 1 > self->ceiling) { \
        zsys_warning ("zre_msg: GET_NUMBER1 failed"); \
        goto malformed; \
    } \
    (host) = *(byte *) self->needle; \
    self->needle++; \
}

//  Get a 2-byte number from the frame
#define GET_NUMBER2(host) { \
    if (self->needle + 2 > self->ceiling) { \
        zsys_warning ("zre_msg: GET_NUMBER2 failed"); \
        goto malformed; \
    } \
    (host) = ((uint16_t) (self->needle [0]) << 8) \
           +  (uint16_t) (self->needle [1]); \
    self->needle += 2; \
}

//  Get a 4-byte number from the frame
#define GET_NUMBER4(host) { \
    if (self->needle + 4 > self->ceiling) { \
        zsys_warning ("zre_msg: GET_NUMBER4 failed"); \
        goto malformed; \
    } \
    (host) = ((uint32_t) (self->needle [0]) << 24) \
           + ((uint32_t) (self->needle [1]) << 16) \
           + ((uint32_t) (self->needle [2]) << 8) \
           +  (uint32_t) (self->needle [3]); \
    self->needle += 4; \
}

//  Get a 8-byte number from the frame
#define GET_NUMBER8(host) { \
    if (self->needle + 8 > self->ceiling) { \
        zsys_warning ("zre_msg: GET_NUMBER8 failed"); \
        goto malformed; \
    } \
    (host) = ((uint64_t) (self->needle [0]) << 56) \
           + ((uint64_t) (self->needle [1]) << 48) \
           + ((uint64_t) (self->needle [2]) << 40) \
           + ((uint64_t) (self->needle [3]) << 32) \
           + ((uint64_t) (self->needle [4]) << 24) \
           + ((uint64_t) (self->needle [5]) << 16) \
           + ((uint64_t) (self->needle [6]) << 8) \
           +  (uint64_t) (self->needle [7]); \
    self->needle += 8; \
}

//  Put a string to the frame
#define PUT_STRING(host) { \
    size_t string_size = strlen (host); \
    PUT_NUMBER1 (string_size); \
    memcpy (self->needle, (host), string_size); \
    self->needle += string_size; \
}

//  Get a string from the frame
#define GET_STRING(host) { \
    size_t string_size; \
    GET_NUMBER1 (string_size); \
    if (self->needle + string_size > (self->ceiling)) { \
        zsys_warning ("zre_msg: GET_STRING failed"); \
        goto malformed; \
    } \
    memcpy ((host), self->needle, string_size); \
    (host) [string_size] = 0; \
    self->needle += string_size; \
}

//  Put a long string to the frame
#define PUT_LONGSTR(host) { \
    size_t string_size = strlen (host); \
    PUT_NUMBER4 (string_size); \
    memcpy (self->needle, (host), string_size); \
    self->needle += string_size; \
}

//  Get a long string from the frame
#define GET_LONGSTR(host) { \
    size_t string_size; \
    GET_NUMBER4 (string_size); \
    if (self->needle + string_size > (self->ceiling)) { \
        zsys_warning ("zre_msg: GET_LONGSTR failed"); \
        goto malformed; \
    } \
    free ((host)); \
    (host) = (char *) malloc (string_size + 1); \
    memcpy ((host), self->needle, string_size); \
    (host) [string_size] = 0; \
    self->needle += string_size; \
}


//  --------------------------------------------------------------------------
//  Create a new zre_msg

zre_msg_t *
zre_msg_new (void)
{
    zre_msg_t *self = (zre_msg_t *) zmalloc (sizeof (zre_msg_t));
    return self;
}


//  --------------------------------------------------------------------------
//  Destroy the zre_msg

void
zre_msg_destroy (zre_msg_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zre_msg_t *self = *self_p;

        //  Free class properties
        zframe_destroy (&self->routing_id);
        if (self->groups)
            zlist_destroy (&self->groups);
        zhash_destroy (&self->headers);
        zmsg_destroy (&self->content);

        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}


//  --------------------------------------------------------------------------
//  Create a deep copy of a zre_msg instance

zre_msg_t *
zre_msg_dup (zre_msg_t *other)
{
    assert (other);
    zre_msg_t *copy = zre_msg_new ();

    // Copy the routing and message id
    zre_msg_set_routing_id (copy, zframe_dup (zre_msg_routing_id (other)));
    zre_msg_set_id (copy, zre_msg_id (other));

    // Copy the rest of the fields
    zre_msg_set_sequence (copy, zre_msg_sequence (other));
    zre_msg_set_endpoint (copy, zre_msg_endpoint (other));
    {
        zlist_t *lcopy = zlist_dup (zre_msg_groups (other));
        zre_msg_set_groups (copy, &lcopy);
    }
    zre_msg_set_status (copy, zre_msg_status (other));
    zre_msg_set_name (copy, zre_msg_name (other));
    zhash_t *dup_hash = zhash_dup (zre_msg_headers (other));
    zre_msg_set_headers (copy, &dup_hash);
    zmsg_t *dup_msg = zmsg_dup (zre_msg_content (other));
    zre_msg_set_content (copy, &dup_msg);
    zre_msg_set_group (copy, zre_msg_group (other));

    return copy;
}

//  --------------------------------------------------------------------------
//  Receive a zre_msg from the socket. Returns 0 if OK, -1 if
//  the recv was interrupted, or -2 if the message is malformed.
//  Blocks if there is no message waiting.

int
zre_msg_recv (zre_msg_t *self, zsock_t *input)
{
    assert (input);
    int rc = 0;
    zmq_msg_t frame;
    zmq_msg_init (&frame);

    if (zsock_type (input) == ZMQ_ROUTER) {
        zframe_destroy (&self->routing_id);
        self->routing_id = zframe_recv (input);
        if (!self->routing_id || !zsock_rcvmore (input)) {
            zsys_warning ("zre_msg: no routing ID");
            rc = -1;            //  Interrupted
            goto malformed;
        }
    }
    int size;
    size = zmq_msg_recv (&frame, zsock_resolve (input), 0);
    if (size == -1) {
        zsys_warning ("zre_msg: interrupted");
        rc = -1;                //  Interrupted
        goto malformed;
    }
    //  Get and check protocol signature
    self->needle = (byte *) zmq_msg_data (&frame);
    self->ceiling = self->needle + zmq_msg_size (&frame);

    uint16_t signature;
    GET_NUMBER2 (signature);
    if (signature != (0xAAA0 | 1)) {
        zsys_warning ("zre_msg: invalid signature");
        rc = -2;                //  Malformed
        goto malformed;
    }
    //  Get message id and parse per message type
    GET_NUMBER1 (self->id);

    switch (self->id) {
        case ZRE_MSG_HELLO:
            {
                byte version;
                GET_NUMBER1 (version);
                if (version != 2) {
                    zsys_warning ("zre_msg: version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            GET_NUMBER2 (self->sequence);
            GET_STRING (self->endpoint);
            {
                size_t list_size;
                GET_NUMBER4 (list_size);
                zlist_destroy (&self->groups);
                self->groups = zlist_new ();
                zlist_autofree (self->groups);
                while (list_size--) {
                    char *string = NULL;
                    GET_LONGSTR (string);
                    zlist_append (self->groups, string);
                    free (string);
                }
            }
            GET_NUMBER1 (self->status);
            GET_STRING (self->name);
            {
                size_t hash_size;
                GET_NUMBER4 (hash_size);
                zhash_destroy (&self->headers);
                self->headers = zhash_new ();
                zhash_autofree (self->headers);
                while (hash_size--) {
                    char key [256];
                    char *value = NULL;
                    GET_STRING (key);
                    GET_LONGSTR (value);
                    zhash_insert (self->headers, key, value);
                    free (value);
                }
            }
            break;

        case ZRE_MSG_WHISPER:
            {
                byte version;
                GET_NUMBER1 (version);
                if (version != 2) {
                    zsys_warning ("zre_msg: version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            GET_NUMBER2 (self->sequence);
            //  Get zero or more remaining frames
            zmsg_destroy (&self->content);
            if (zsock_rcvmore (input))
                self->content = zmsg_recv (input);
            else
                self->content = zmsg_new ();
            break;

        case ZRE_MSG_SHOUT:
            {
                byte version;
                GET_NUMBER1 (version);
                if (version != 2) {
                    zsys_warning ("zre_msg: version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            GET_NUMBER2 (self->sequence);
            GET_STRING (self->group);
            //  Get zero or more remaining frames
            zmsg_destroy (&self->content);
            if (zsock_rcvmore (input))
                self->content = zmsg_recv (input);
            else
                self->content = zmsg_new ();
            break;

        case ZRE_MSG_JOIN:
            {
                byte version;
                GET_NUMBER1 (version);
                if (version != 2) {
                    zsys_warning ("zre_msg: version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            GET_NUMBER2 (self->sequence);
            GET_STRING (self->group);
            GET_NUMBER1 (self->status);
            break;

        case ZRE_MSG_LEAVE:
            {
                byte version;
                GET_NUMBER1 (version);
                if (version != 2) {
                    zsys_warning ("zre_msg: version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            GET_NUMBER2 (self->sequence);
            GET_STRING (self->group);
            GET_NUMBER1 (self->status);
            break;

        case ZRE_MSG_PING:
            {
                byte version;
                GET_NUMBER1 (version);
                if (version != 2) {
                    zsys_warning ("zre_msg: version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            GET_NUMBER2 (self->sequence);
            break;

        case ZRE_MSG_PING_OK:
            {
                byte version;
                GET_NUMBER1 (version);
                if (version != 2) {
                    zsys_warning ("zre_msg: version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            GET_NUMBER2 (self->sequence);
            break;

        default:
            zsys_warning ("zre_msg: bad message ID");
            rc = -2;            //  Malformed
            goto malformed;
    }
    //  Successful return
    zmq_msg_close (&frame);
    return rc;

    //  Error returns
    malformed:
        zmq_msg_close (&frame);
        return rc;              //  Invalid message
}


//  --------------------------------------------------------------------------
//  Send the zre_msg to the socket. Does not destroy it. Returns 0 if
//  OK, else -1.

int
zre_msg_send (zre_msg_t *self, zsock_t *output)
{
    assert (self);
    assert (output);

    if (zsock_type (output) == ZMQ_ROUTER)
        zframe_send (&self->routing_id, output, ZFRAME_MORE + ZFRAME_REUSE);

    size_t frame_size = 2 + 1;          //  Signature and message ID
    switch (self->id) {
        case ZRE_MSG_HELLO:
            frame_size += 1;            //  version
            frame_size += 2;            //  sequence
            frame_size += 1 + strlen (self->endpoint);
            frame_size += 4;            //  Size is 4 octets
            if (self->groups) {
                char *groups = (char *) zlist_first (self->groups);
                while (groups) {
                    frame_size += 4 + strlen (groups);
                    groups = (char *) zlist_next (self->groups);
                }
            }
            frame_size += 1;            //  status
            frame_size += 1 + strlen (self->name);
            frame_size += 4;            //  Size is 4 octets
            if (self->headers) {
                self->headers_bytes = 0;
                char *item = (char *) zhash_first (self->headers);
                while (item) {
                    self->headers_bytes += 1 + strlen (zhash_cursor (self->headers));
                    self->headers_bytes += 4 + strlen (item);
                    item = (char *) zhash_next (self->headers);
                }
            }
            frame_size += self->headers_bytes;
            break;
        case ZRE_MSG_WHISPER:
            frame_size += 1;            //  version
            frame_size += 2;            //  sequence
            break;
        case ZRE_MSG_SHOUT:
            frame_size += 1;            //  version
            frame_size += 2;            //  sequence
            frame_size += 1 + strlen (self->group);
            break;
        case ZRE_MSG_JOIN:
            frame_size += 1;            //  version
            frame_size += 2;            //  sequence
            frame_size += 1 + strlen (self->group);
            frame_size += 1;            //  status
            break;
        case ZRE_MSG_LEAVE:
            frame_size += 1;            //  version
            frame_size += 2;            //  sequence
            frame_size += 1 + strlen (self->group);
            frame_size += 1;            //  status
            break;
        case ZRE_MSG_PING:
            frame_size += 1;            //  version
            frame_size += 2;            //  sequence
            break;
        case ZRE_MSG_PING_OK:
            frame_size += 1;            //  version
            frame_size += 2;            //  sequence
            break;
    }
    //  Now serialize message into the frame
    zmq_msg_t frame;
    zmq_msg_init_size (&frame, frame_size);
    self->needle = (byte *) zmq_msg_data (&frame);
    PUT_NUMBER2 (0xAAA0 | 1);
    PUT_NUMBER1 (self->id);
    bool have_content = false;
    size_t nbr_frames = 1;              //  Total number of frames to send

    switch (self->id) {
        case ZRE_MSG_HELLO:
            PUT_NUMBER1 (2);
            PUT_NUMBER2 (self->sequence);
            PUT_STRING (self->endpoint);
            if (self->groups) {
                PUT_NUMBER4 (zlist_size (self->groups));
                char *groups = (char *) zlist_first (self->groups);
                while (groups) {
                    PUT_LONGSTR (groups);
                    groups = (char *) zlist_next (self->groups);
                }
            }
            else
                PUT_NUMBER4 (0);    //  Empty string array
            PUT_NUMBER1 (self->status);
            PUT_STRING (self->name);
            if (self->headers) {
                PUT_NUMBER4 (zhash_size (self->headers));
                char *item = (char *) zhash_first (self->headers);
                while (item) {
                    PUT_STRING (zhash_cursor (self->headers));
                    PUT_LONGSTR (item);
                    item = (char *) zhash_next (self->headers);
                }
            }
            else
                PUT_NUMBER4 (0);    //  Empty hash
            break;

        case ZRE_MSG_WHISPER:
            PUT_NUMBER1 (2);
            PUT_NUMBER2 (self->sequence);
            nbr_frames += self->content? zmsg_size (self->content): 1;
            have_content = true;
            break;

        case ZRE_MSG_SHOUT:
            PUT_NUMBER1 (2);
            PUT_NUMBER2 (self->sequence);
            PUT_STRING (self->group);
            nbr_frames += self->content? zmsg_size (self->content): 1;
            have_content = true;
            break;

        case ZRE_MSG_JOIN:
            PUT_NUMBER1 (2);
            PUT_NUMBER2 (self->sequence);
            PUT_STRING (self->group);
            PUT_NUMBER1 (self->status);
            break;

        case ZRE_MSG_LEAVE:
            PUT_NUMBER1 (2);
            PUT_NUMBER2 (self->sequence);
            PUT_STRING (self->group);
            PUT_NUMBER1 (self->status);
            break;

        case ZRE_MSG_PING:
            PUT_NUMBER1 (2);
            PUT_NUMBER2 (self->sequence);
            break;

        case ZRE_MSG_PING_OK:
            PUT_NUMBER1 (2);
            PUT_NUMBER2 (self->sequence);
            break;

    }
    //  Now send the data frame
    zmq_msg_send (&frame, zsock_resolve (output), --nbr_frames? ZMQ_SNDMORE: 0);

    //  Now send the content if necessary
    if (have_content) {
        if (self->content) {
            zframe_t *frame = zmsg_first (self->content);
            while (frame) {
                zframe_send (&frame, output, ZFRAME_REUSE + (--nbr_frames? ZFRAME_MORE: 0));
                frame = zmsg_next (self->content);
            }
        }
        else
            zmq_send (zsock_resolve (output), NULL, 0, 0);
    }
    return 0;
}


//  --------------------------------------------------------------------------
//  Print contents of message to stdout

void
zre_msg_print (zre_msg_t *self)
{
    assert (self);
    switch (self->id) {
        case ZRE_MSG_HELLO:
            zsys_debug ("ZRE_MSG_HELLO:");
            zsys_debug ("    version=2");
            zsys_debug ("    sequence=%ld", (long) self->sequence);
            zsys_debug ("    endpoint='%s'", self->endpoint);
            zsys_debug ("    groups=");
            if (self->groups) {
                char *groups = (char *) zlist_first (self->groups);
                while (groups) {
                    zsys_debug ("        '%s'", groups);
                    groups = (char *) zlist_next (self->groups);
                }
            }
            zsys_debug ("    status=%ld", (long) self->status);
            zsys_debug ("    name='%s'", self->name);
            zsys_debug ("    headers=");
            if (self->headers) {
                char *item = (char *) zhash_first (self->headers);
                while (item) {
                    zsys_debug ("        %s=%s", zhash_cursor (self->headers), item);
                    item = (char *) zhash_next (self->headers);
                }
            }
            else
                zsys_debug ("(NULL)");
            break;

        case ZRE_MSG_WHISPER:
            zsys_debug ("ZRE_MSG_WHISPER:");
            zsys_debug ("    version=2");
            zsys_debug ("    sequence=%ld", (long) self->sequence);
            zsys_debug ("    content=");
            if (self->content)
                zmsg_print (self->content);
            else
                zsys_debug ("(NULL)");
            break;

        case ZRE_MSG_SHOUT:
            zsys_debug ("ZRE_MSG_SHOUT:");
            zsys_debug ("    version=2");
            zsys_debug ("    sequence=%ld", (long) self->sequence);
            zsys_debug ("    group='%s'", self->group);
            zsys_debug ("    content=");
            if (self->content)
                zmsg_print (self->content);
            else
                zsys_debug ("(NULL)");
            break;

        case ZRE_MSG_JOIN:
            zsys_debug ("ZRE_MSG_JOIN:");
            zsys_debug ("    version=2");
            zsys_debug ("    sequence=%ld", (long) self->sequence);
            zsys_debug ("    group='%s'", self->group);
            zsys_debug ("    status=%ld", (long) self->status);
            break;

        case ZRE_MSG_LEAVE:
            zsys_debug ("ZRE_MSG_LEAVE:");
            zsys_debug ("    version=2");
            zsys_debug ("    sequence=%ld", (long) self->sequence);
            zsys_debug ("    group='%s'", self->group);
            zsys_debug ("    status=%ld", (long) self->status);
            break;

        case ZRE_MSG_PING:
            zsys_debug ("ZRE_MSG_PING:");
            zsys_debug ("    version=2");
            zsys_debug ("    sequence=%ld", (long) self->sequence);
            break;

        case ZRE_MSG_PING_OK:
            zsys_debug ("ZRE_MSG_PING_OK:");
            zsys_debug ("    version=2");
            zsys_debug ("    sequence=%ld", (long) self->sequence);
            break;

    }
}


//  --------------------------------------------------------------------------
//  Get/set the message routing_id

zframe_t *
zre_msg_routing_id (zre_msg_t *self)
{
    assert (self);
    return self->routing_id;
}

void
zre_msg_set_routing_id (zre_msg_t *self, zframe_t *routing_id)
{
    if (self->routing_id)
        zframe_destroy (&self->routing_id);
    self->routing_id = zframe_dup (routing_id);
}


//  --------------------------------------------------------------------------
//  Get/set the zre_msg id

int
zre_msg_id (zre_msg_t *self)
{
    assert (self);
    return self->id;
}

void
zre_msg_set_id (zre_msg_t *self, int id)
{
    self->id = id;
}

//  --------------------------------------------------------------------------
//  Return a printable command string

const char *
zre_msg_command (zre_msg_t *self)
{
    assert (self);
    switch (self->id) {
        case ZRE_MSG_HELLO:
            return ("HELLO");
            break;
        case ZRE_MSG_WHISPER:
            return ("WHISPER");
            break;
        case ZRE_MSG_SHOUT:
            return ("SHOUT");
            break;
        case ZRE_MSG_JOIN:
            return ("JOIN");
            break;
        case ZRE_MSG_LEAVE:
            return ("LEAVE");
            break;
        case ZRE_MSG_PING:
            return ("PING");
            break;
        case ZRE_MSG_PING_OK:
            return ("PING_OK");
            break;
    }
    return "?";
}

//  --------------------------------------------------------------------------
//  Get/set the sequence field

uint16_t
zre_msg_sequence (zre_msg_t *self)
{
    assert (self);
    return self->sequence;
}

void
zre_msg_set_sequence (zre_msg_t *self, uint16_t sequence)
{
    assert (self);
    self->sequence = sequence;
}


//  --------------------------------------------------------------------------
//  Get/set the endpoint field

const char *
zre_msg_endpoint (zre_msg_t *self)
{
    assert (self);
    return self->endpoint;
}

void
zre_msg_set_endpoint (zre_msg_t *self, const char *value)
{
    assert (self);
    assert (value);
    if (value == self->endpoint)
        return;
    strncpy (self->endpoint, value, 255);
    self->endpoint [255] = 0;
}


//  --------------------------------------------------------------------------
//  Get the groups field, without transferring ownership

zlist_t *
zre_msg_groups (zre_msg_t *self)
{
    assert (self);
    return self->groups;
}

//  Get the groups field and transfer ownership to caller

zlist_t *
zre_msg_get_groups (zre_msg_t *self)
{
    assert (self);
    zlist_t *groups = self->groups;
    self->groups = NULL;
    return groups;
}

//  Set the groups field, transferring ownership from caller

void
zre_msg_set_groups (zre_msg_t *self, zlist_t **groups_p)
{
    assert (self);
    assert (groups_p);
    zlist_destroy (&self->groups);
    self->groups = *groups_p;
    *groups_p = NULL;
}



//  --------------------------------------------------------------------------
//  Get/set the status field

byte
zre_msg_status (zre_msg_t *self)
{
    assert (self);
    return self->status;
}

void
zre_msg_set_status (zre_msg_t *self, byte status)
{
    assert (self);
    self->status = status;
}


//  --------------------------------------------------------------------------
//  Get/set the name field

const char *
zre_msg_name (zre_msg_t *self)
{
    assert (self);
    return self->name;
}

void
zre_msg_set_name (zre_msg_t *self, const char *value)
{
    assert (self);
    assert (value);
    if (value == self->name)
        return;
    strncpy (self->name, value, 255);
    self->name [255] = 0;
}


//  --------------------------------------------------------------------------
//  Get the headers field without transferring ownership

zhash_t *
zre_msg_headers (zre_msg_t *self)
{
    assert (self);
    return self->headers;
}

//  Get the headers field and transfer ownership to caller

zhash_t *
zre_msg_get_headers (zre_msg_t *self)
{
    zhash_t *headers = self->headers;
    self->headers = NULL;
    return headers;
}

//  Set the headers field, transferring ownership from caller

void
zre_msg_set_headers (zre_msg_t *self, zhash_t **headers_p)
{
    assert (self);
    assert (headers_p);
    zhash_destroy (&self->headers);
    self->headers = *headers_p;
    *headers_p = NULL;
}


//  --------------------------------------------------------------------------
//  Get the content field without transferring ownership

zmsg_t *
zre_msg_content (zre_msg_t *self)
{
    assert (self);
    return self->content;
}

//  Get the content field and transfer ownership to caller

zmsg_t *
zre_msg_get_content (zre_msg_t *self)
{
    zmsg_t *content = self->content;
    self->content = NULL;
    return content;
}

//  Set the content field, transferring ownership from caller

void
zre_msg_set_content (zre_msg_t *self, zmsg_t **msg_p)
{
    assert (self);
    assert (msg_p);
    zmsg_destroy (&self->content);
    self->content = *msg_p;
    *msg_p = NULL;
}


//  --------------------------------------------------------------------------
//  Get/set the group field

const char *
zre_msg_group (zre_msg_t *self)
{
    assert (self);
    return self->group;
}

void
zre_msg_set_group (zre_msg_t *self, const char *value)
{
    assert (self);
    assert (value);
    if (value == self->group)
        return;
    strncpy (self->group, value, 255);
    self->group [255] = 0;
}



//  --------------------------------------------------------------------------
//  Selftest

void
zre_msg_test (bool verbose)
{
    printf (" * zre_msg: ");

    if (verbose)
        printf ("\n");

    //  @selftest
    //  Simple create/destroy test
    zre_msg_t *self = zre_msg_new ();
    assert (self);
    zre_msg_destroy (&self);
    //  Create pair of sockets we can send through
    //  We must bind before connect if we wish to remain compatible with ZeroMQ < v4
    zsock_t *output = zsock_new (ZMQ_DEALER);
    assert (output);
    int rc = zsock_bind (output, "inproc://selftest-zre_msg");
    assert (rc == 0);

    zsock_t *input = zsock_new (ZMQ_ROUTER);
    assert (input);
    rc = zsock_connect (input, "inproc://selftest-zre_msg");
    assert (rc == 0);


    //  Encode/send/decode and verify each message type
    int instance;
    self = zre_msg_new ();
    zre_msg_set_id (self, ZRE_MSG_HELLO);

    zre_msg_set_sequence (self, 123);
    zre_msg_set_endpoint (self, "Life is short but Now lasts for ever");
    zlist_t *hello_groups = zlist_new ();
    zlist_append (hello_groups, "Name: Brutus");
    zlist_append (hello_groups, "Age: 43");
    zre_msg_set_groups (self, &hello_groups);
    zre_msg_set_status (self, 123);
    zre_msg_set_name (self, "Life is short but Now lasts for ever");
    zhash_t *hello_headers = zhash_new ();
    zhash_insert (hello_headers, "Name", "Brutus");
    zre_msg_set_headers (self, &hello_headers);
    //  Send twice
    zre_msg_send (self, output);
    zre_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        zre_msg_recv (self, input);
        assert (zre_msg_routing_id (self));
        assert (zre_msg_sequence (self) == 123);
        assert (streq (zre_msg_endpoint (self), "Life is short but Now lasts for ever"));
        zlist_t *groups = zre_msg_get_groups (self);
        assert (groups);
        assert (zlist_size (groups) == 2);
        assert (streq ((char *) zlist_first (groups), "Name: Brutus"));
        assert (streq ((char *) zlist_next (groups), "Age: 43"));
        zlist_destroy (&groups);
        zlist_destroy (&hello_groups);
        assert (zre_msg_status (self) == 123);
        assert (streq (zre_msg_name (self), "Life is short but Now lasts for ever"));
        zhash_t *headers = zre_msg_get_headers (self);
        assert (zhash_size (headers) == 1);
        assert (streq ((char *) zhash_first (headers), "Brutus"));
        assert (streq ((char *) zhash_cursor (headers), "Name"));
        zhash_destroy (&headers);
        if (instance == 1)
            zhash_destroy (&hello_headers);
    }
    zre_msg_set_id (self, ZRE_MSG_WHISPER);

    zre_msg_set_sequence (self, 123);
    zmsg_t *whisper_content = zmsg_new ();
    zre_msg_set_content (self, &whisper_content);
    zmsg_addstr (zre_msg_content (self), "Captcha Diem");
    //  Send twice
    zre_msg_send (self, output);
    zre_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        zre_msg_recv (self, input);
        assert (zre_msg_routing_id (self));
        assert (zre_msg_sequence (self) == 123);
        assert (zmsg_size (zre_msg_content (self)) == 1);
        char *content = zmsg_popstr (zre_msg_content (self));
        assert (streq (content, "Captcha Diem"));
        zstr_free (&content);
        if (instance == 1)
            zmsg_destroy (&whisper_content);
    }
    zre_msg_set_id (self, ZRE_MSG_SHOUT);

    zre_msg_set_sequence (self, 123);
    zre_msg_set_group (self, "Life is short but Now lasts for ever");
    zmsg_t *shout_content = zmsg_new ();
    zre_msg_set_content (self, &shout_content);
    zmsg_addstr (zre_msg_content (self), "Captcha Diem");
    //  Send twice
    zre_msg_send (self, output);
    zre_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        zre_msg_recv (self, input);
        assert (zre_msg_routing_id (self));
        assert (zre_msg_sequence (self) == 123);
        assert (streq (zre_msg_group (self), "Life is short but Now lasts for ever"));
        assert (zmsg_size (zre_msg_content (self)) == 1);
        char *content = zmsg_popstr (zre_msg_content (self));
        assert (streq (content, "Captcha Diem"));
        zstr_free (&content);
        if (instance == 1)
            zmsg_destroy (&shout_content);
    }
    zre_msg_set_id (self, ZRE_MSG_JOIN);

    zre_msg_set_sequence (self, 123);
    zre_msg_set_group (self, "Life is short but Now lasts for ever");
    zre_msg_set_status (self, 123);
    //  Send twice
    zre_msg_send (self, output);
    zre_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        zre_msg_recv (self, input);
        assert (zre_msg_routing_id (self));
        assert (zre_msg_sequence (self) == 123);
        assert (streq (zre_msg_group (self), "Life is short but Now lasts for ever"));
        assert (zre_msg_status (self) == 123);
    }
    zre_msg_set_id (self, ZRE_MSG_LEAVE);

    zre_msg_set_sequence (self, 123);
    zre_msg_set_group (self, "Life is short but Now lasts for ever");
    zre_msg_set_status (self, 123);
    //  Send twice
    zre_msg_send (self, output);
    zre_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        zre_msg_recv (self, input);
        assert (zre_msg_routing_id (self));
        assert (zre_msg_sequence (self) == 123);
        assert (streq (zre_msg_group (self), "Life is short but Now lasts for ever"));
        assert (zre_msg_status (self) == 123);
    }
    zre_msg_set_id (self, ZRE_MSG_PING);

    zre_msg_set_sequence (self, 123);
    //  Send twice
    zre_msg_send (self, output);
    zre_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        zre_msg_recv (self, input);
        assert (zre_msg_routing_id (self));
        assert (zre_msg_sequence (self) == 123);
    }
    zre_msg_set_id (self, ZRE_MSG_PING_OK);

    zre_msg_set_sequence (self, 123);
    //  Send twice
    zre_msg_send (self, output);
    zre_msg_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        zre_msg_recv (self, input);
        assert (zre_msg_routing_id (self));
        assert (zre_msg_sequence (self) == 123);
    }

    zre_msg_destroy (&self);
    zsock_destroy (&input);
    zsock_destroy (&output);
    //  @end

    printf ("OK\n");
}
