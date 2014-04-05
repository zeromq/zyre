/*  =========================================================================
    zre_log_msg - work with ZRE logging messages

    Codec class for zre_log_msg.

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

    * The XML model used for this code generation: zre_log_msg.xml
    * The code generation script that built this file: zproto_codec_c
    ************************************************************************
    
    Copyright (c) 1991-2012 iMatix Corporation -- http://www.imatix.com     
    Copyright other contributors as noted in the AUTHORS file.              
                                                                            
    This file is part of Zyre, an open-source framework for proximity-based 
    peer-to-peer applications -- See http://zyre.org.                       
                                                                            
    This is free software; you can redistribute it and/or modify it under   
    the terms of the GNU Lesser General Public License as published by the  
    Free Software Foundation; either version 3 of the License, or (at your  
    option) any later version.                                              
                                                                            
    This software is distributed in the hope that it will be useful, but    
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTA-   
    BILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General  
    Public License for more details.                                        
                                                                            
    You should have received a copy of the GNU Lesser General Public License
    along with this program. If not, see http://www.gnu.org/licenses/.      
    =========================================================================
*/

/*
@header
    zre_log_msg - work with ZRE logging messages
@discuss
@end
*/

#include <czmq.h>
#include "../include/zre_log_msg.h"

//  Structure of our class

struct _zre_log_msg_t {
    zframe_t *routing_id;               //  Routing_id from ROUTER, if any
    int id;                             //  zre_log_msg message ID
    byte *needle;                       //  Read/write pointer for serialization
    byte *ceiling;                      //  Valid upper limit for read pointer
    byte level;                         //  
    byte event;                         //  
    uint16_t node;                      //  
    uint16_t peer;                      //  
    uint64_t time;                      //  
    char *data;                         //  
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
    if (self->needle + size > self->ceiling) \
        goto malformed; \
    memcpy ((host), self->needle, size); \
    self->needle += size; \
}

//  Put a 1-byte number to the frame
#define PUT_NUMBER1(host) { \
    *(byte *) self->needle = (host); \
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
    if (self->needle + 1 > self->ceiling) \
        goto malformed; \
    (host) = *(byte *) self->needle; \
    self->needle++; \
}

//  Get a 2-byte number from the frame
#define GET_NUMBER2(host) { \
    if (self->needle + 2 > self->ceiling) \
        goto malformed; \
    (host) = ((uint16_t) (self->needle [0]) << 8) \
           +  (uint16_t) (self->needle [1]); \
    self->needle += 2; \
}

//  Get a 4-byte number from the frame
#define GET_NUMBER4(host) { \
    if (self->needle + 4 > self->ceiling) \
        goto malformed; \
    (host) = ((uint32_t) (self->needle [0]) << 24) \
           + ((uint32_t) (self->needle [1]) << 16) \
           + ((uint32_t) (self->needle [2]) << 8) \
           +  (uint32_t) (self->needle [3]); \
    self->needle += 4; \
}

//  Get a 8-byte number from the frame
#define GET_NUMBER8(host) { \
    if (self->needle + 8 > self->ceiling) \
        goto malformed; \
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
    if (self->needle + string_size > (self->ceiling)) \
        goto malformed; \
    (host) = (char *) malloc (string_size + 1); \
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
    if (self->needle + string_size > (self->ceiling)) \
        goto malformed; \
    (host) = (char *) malloc (string_size + 1); \
    memcpy ((host), self->needle, string_size); \
    (host) [string_size] = 0; \
    self->needle += string_size; \
}


//  --------------------------------------------------------------------------
//  Create a new zre_log_msg

zre_log_msg_t *
zre_log_msg_new (int id)
{
    zre_log_msg_t *self = (zre_log_msg_t *) zmalloc (sizeof (zre_log_msg_t));
    self->id = id;
    return self;
}


//  --------------------------------------------------------------------------
//  Destroy the zre_log_msg

void
zre_log_msg_destroy (zre_log_msg_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zre_log_msg_t *self = *self_p;

        //  Free class properties
        zframe_destroy (&self->routing_id);
        free (self->data);

        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}


//  --------------------------------------------------------------------------
//  Parse a zre_log_msg from zmsg_t. Returns a new object, or NULL if
//  the message could not be parsed, or was NULL. If the socket type is
//  ZMQ_ROUTER, then parses the first frame as a routing_id. Destroys msg
//  and nullifies the msg refernce.

zre_log_msg_t *
zre_log_msg_decode (zmsg_t **msg_p, int socket_type)
{
    assert (msg_p);
    zmsg_t *msg = *msg_p;
    if (msg == NULL)
        return NULL;
        
    zre_log_msg_t *self = zre_log_msg_new (0);
    //  If message came from a router socket, first frame is routing_id
    if (socket_type == ZMQ_ROUTER) {
        self->routing_id = zmsg_pop (msg);
        //  If message was not valid, forget about it
        if (!self->routing_id || !zmsg_next (msg)) {
            zre_log_msg_destroy (&self);
            return (NULL);      //  Malformed or empty
        }
    }
    //  Read and parse command in frame
    zframe_t *frame = zmsg_pop (msg);
    if (!frame) 
        goto empty;             //  Malformed or empty

    //  Get and check protocol signature
    self->needle = zframe_data (frame);
    self->ceiling = self->needle + zframe_size (frame);
    uint16_t signature;
    GET_NUMBER2 (signature);
    if (signature != (0xAAA0 | 2))
        goto empty;             //  Invalid signature

    //  Get message id and parse per message type
    GET_NUMBER1 (self->id);

    switch (self->id) {
        case ZRE_LOG_MSG_LOG:
            GET_NUMBER1 (self->level);
            GET_NUMBER1 (self->event);
            GET_NUMBER2 (self->node);
            GET_NUMBER2 (self->peer);
            GET_NUMBER8 (self->time);
            GET_STRING (self->data);
            break;

        default:
            goto malformed;
    }
    //  Successful return
    zframe_destroy (&frame);
    zmsg_destroy (msg_p);
    return self;

    //  Error returns
    malformed:
        printf ("E: malformed message '%d'\n", self->id);
    empty:
        zframe_destroy (&frame);
        zmsg_destroy (msg_p);
        zre_log_msg_destroy (&self);
        return (NULL);
}


//  --------------------------------------------------------------------------
//  Receive and parse a zre_log_msg from the socket. Returns new object or
//  NULL if error. Will block if there's no message waiting.

zre_log_msg_t *
zre_log_msg_recv (void *input)
{
    assert (input);
    zmsg_t *msg = zmsg_recv (input);
    return zre_log_msg_decode (&msg, zsocket_type (input));
}


//  --------------------------------------------------------------------------
//  Receive and parse a zre_log_msg from the socket. Returns new object, 
//  or NULL either if there was no input waiting, or the recv was interrupted.

zre_log_msg_t *
zre_log_msg_recv_nowait (void *input)
{
    assert (input);
    zmsg_t *msg = zmsg_recv_nowait (input);
    return zre_log_msg_decode (&msg, zsocket_type (input));
}


//  Encode zre_log_msg into zmsg and destroy it. Returns a newly created
//  object or NULL if error. Use when not in control of sending the message.
//  If the socket_type is ZMQ_ROUTER, then stores the routing_id as the
//  first frame of the resulting message.

zmsg_t *
zre_log_msg_encode (zre_log_msg_t *self, int socket_type)
{
    assert (self);
    zmsg_t *msg = zmsg_new ();

    //  If we're sending to a ROUTER, send the routing_id first
    if (socket_type == ZMQ_ROUTER)
        zmsg_prepend (msg, &self->routing_id);
        
    size_t frame_size = 2 + 1;          //  Signature and message ID
    switch (self->id) {
        case ZRE_LOG_MSG_LOG:
            //  level is a 1-byte integer
            frame_size += 1;
            //  event is a 1-byte integer
            frame_size += 1;
            //  node is a 2-byte integer
            frame_size += 2;
            //  peer is a 2-byte integer
            frame_size += 2;
            //  time is a 8-byte integer
            frame_size += 8;
            //  data is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->data)
                frame_size += strlen (self->data);
            break;
            
        default:
            printf ("E: bad message type '%d', not sent\n", self->id);
            //  No recovery, this is a fatal application error
            assert (false);
    }
    //  Now serialize message into the frame
    zframe_t *frame = zframe_new (NULL, frame_size);
    self->needle = zframe_data (frame);
    PUT_NUMBER2 (0xAAA0 | 2);
    PUT_NUMBER1 (self->id);

    switch (self->id) {
        case ZRE_LOG_MSG_LOG:
            PUT_NUMBER1 (self->level);
            PUT_NUMBER1 (self->event);
            PUT_NUMBER2 (self->node);
            PUT_NUMBER2 (self->peer);
            PUT_NUMBER8 (self->time);
            if (self->data) {
                PUT_STRING (self->data);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            break;

    }
    //  Now send the data frame
    if (zmsg_append (msg, &frame)) {
        zmsg_destroy (&msg);
        zre_log_msg_destroy (&self);
        return NULL;
    }
    //  Destroy zre_log_msg object
    zre_log_msg_destroy (&self);
    return msg;

}

//  --------------------------------------------------------------------------
//  Send the zre_log_msg to the socket, and destroy it
//  Returns 0 if OK, else -1

int
zre_log_msg_send (zre_log_msg_t **self_p, void *output)
{
    assert (self_p);
    assert (*self_p);
    assert (output);

    zre_log_msg_t *self = *self_p;
    zmsg_t *msg = zre_log_msg_encode (self, zsocket_type (output));
    if (msg && zmsg_send (&msg, output) == 0)
        return 0;
    else
        return -1;              //  Failed to encode, or send
}


//  --------------------------------------------------------------------------
//  Send the zre_log_msg to the output, and do not destroy it

int
zre_log_msg_send_again (zre_log_msg_t *self, void *output)
{
    assert (self);
    assert (output);
    self = zre_log_msg_dup (self);
    return zre_log_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the LOG to the socket in one step

int
zre_log_msg_send_log (
    void *output,
    byte level,
    byte event,
    uint16_t node,
    uint16_t peer,
    uint64_t time,
    const char *data)
{
    zre_log_msg_t *self = zre_log_msg_new (ZRE_LOG_MSG_LOG);
    zre_log_msg_set_level (self, level);
    zre_log_msg_set_event (self, event);
    zre_log_msg_set_node (self, node);
    zre_log_msg_set_peer (self, peer);
    zre_log_msg_set_time (self, time);
    zre_log_msg_set_data (self, data);
    return zre_log_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Duplicate the zre_log_msg message

zre_log_msg_t *
zre_log_msg_dup (zre_log_msg_t *self)
{
    if (!self)
        return NULL;
        
    zre_log_msg_t *copy = zre_log_msg_new (self->id);
    if (self->routing_id)
        copy->routing_id = zframe_dup (self->routing_id);
    switch (self->id) {
        case ZRE_LOG_MSG_LOG:
            copy->level = self->level;
            copy->event = self->event;
            copy->node = self->node;
            copy->peer = self->peer;
            copy->time = self->time;
            copy->data = self->data? strdup (self->data): NULL;
            break;

    }
    return copy;
}



//  --------------------------------------------------------------------------
//  Print contents of message to stdout

void
zre_log_msg_dump (zre_log_msg_t *self)
{
    assert (self);
    switch (self->id) {
        case ZRE_LOG_MSG_LOG:
            puts ("LOG:");
            printf ("    level=%ld\n", (long) self->level);
            printf ("    event=%ld\n", (long) self->event);
            printf ("    node=%ld\n", (long) self->node);
            printf ("    peer=%ld\n", (long) self->peer);
            printf ("    time=%ld\n", (long) self->time);
            if (self->data)
                printf ("    data='%s'\n", self->data);
            else
                printf ("    data=\n");
            break;
            
    }
}


//  --------------------------------------------------------------------------
//  Get/set the message routing_id

zframe_t *
zre_log_msg_routing_id (zre_log_msg_t *self)
{
    assert (self);
    return self->routing_id;
}

void
zre_log_msg_set_routing_id (zre_log_msg_t *self, zframe_t *routing_id)
{
    if (self->routing_id)
        zframe_destroy (&self->routing_id);
    self->routing_id = zframe_dup (routing_id);
}


//  --------------------------------------------------------------------------
//  Get/set the zre_log_msg id

int
zre_log_msg_id (zre_log_msg_t *self)
{
    assert (self);
    return self->id;
}

void
zre_log_msg_set_id (zre_log_msg_t *self, int id)
{
    self->id = id;
}

//  --------------------------------------------------------------------------
//  Return a printable command string

const char *
zre_log_msg_command (zre_log_msg_t *self)
{
    assert (self);
    switch (self->id) {
        case ZRE_LOG_MSG_LOG:
            return ("LOG");
            break;
    }
    return "?";
}

//  --------------------------------------------------------------------------
//  Get/set the level field

byte
zre_log_msg_level (zre_log_msg_t *self)
{
    assert (self);
    return self->level;
}

void
zre_log_msg_set_level (zre_log_msg_t *self, byte level)
{
    assert (self);
    self->level = level;
}


//  --------------------------------------------------------------------------
//  Get/set the event field

byte
zre_log_msg_event (zre_log_msg_t *self)
{
    assert (self);
    return self->event;
}

void
zre_log_msg_set_event (zre_log_msg_t *self, byte event)
{
    assert (self);
    self->event = event;
}


//  --------------------------------------------------------------------------
//  Get/set the node field

uint16_t
zre_log_msg_node (zre_log_msg_t *self)
{
    assert (self);
    return self->node;
}

void
zre_log_msg_set_node (zre_log_msg_t *self, uint16_t node)
{
    assert (self);
    self->node = node;
}


//  --------------------------------------------------------------------------
//  Get/set the peer field

uint16_t
zre_log_msg_peer (zre_log_msg_t *self)
{
    assert (self);
    return self->peer;
}

void
zre_log_msg_set_peer (zre_log_msg_t *self, uint16_t peer)
{
    assert (self);
    self->peer = peer;
}


//  --------------------------------------------------------------------------
//  Get/set the time field

uint64_t
zre_log_msg_time (zre_log_msg_t *self)
{
    assert (self);
    return self->time;
}

void
zre_log_msg_set_time (zre_log_msg_t *self, uint64_t time)
{
    assert (self);
    self->time = time;
}


//  --------------------------------------------------------------------------
//  Get/set the data field

const char *
zre_log_msg_data (zre_log_msg_t *self)
{
    assert (self);
    return self->data;
}

void
zre_log_msg_set_data (zre_log_msg_t *self, const char *format, ...)
{
    //  Format data from provided arguments
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    free (self->data);
    self->data = zsys_vprintf (format, argptr);
    va_end (argptr);
}



//  --------------------------------------------------------------------------
//  Selftest

int
zre_log_msg_test (bool verbose)
{
    printf (" * zre_log_msg: ");

    //  @selftest
    //  Simple create/destroy test
    zre_log_msg_t *self = zre_log_msg_new (0);
    assert (self);
    zre_log_msg_destroy (&self);

    //  Create pair of sockets we can send through
    zctx_t *ctx = zctx_new ();
    assert (ctx);

    void *output = zsocket_new (ctx, ZMQ_DEALER);
    assert (output);
    zsocket_bind (output, "inproc://selftest");

    void *input = zsocket_new (ctx, ZMQ_ROUTER);
    assert (input);
    zsocket_connect (input, "inproc://selftest");
    
    //  Encode/send/decode and verify each message type
    int instance;
    zre_log_msg_t *copy;
    self = zre_log_msg_new (ZRE_LOG_MSG_LOG);
    
    //  Check that _dup works on empty message
    copy = zre_log_msg_dup (self);
    assert (copy);
    zre_log_msg_destroy (&copy);

    zre_log_msg_set_level (self, 123);
    zre_log_msg_set_event (self, 123);
    zre_log_msg_set_node (self, 123);
    zre_log_msg_set_peer (self, 123);
    zre_log_msg_set_time (self, 123);
    zre_log_msg_set_data (self, "Life is short but Now lasts for ever");
    //  Send twice from same object
    zre_log_msg_send_again (self, output);
    zre_log_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = zre_log_msg_recv (input);
        assert (self);
        assert (zre_log_msg_routing_id (self));
        
        assert (zre_log_msg_level (self) == 123);
        assert (zre_log_msg_event (self) == 123);
        assert (zre_log_msg_node (self) == 123);
        assert (zre_log_msg_peer (self) == 123);
        assert (zre_log_msg_time (self) == 123);
        assert (streq (zre_log_msg_data (self), "Life is short but Now lasts for ever"));
        zre_log_msg_destroy (&self);
    }

    zctx_destroy (&ctx);
    //  @end

    printf ("OK\n");
    return 0;
}
