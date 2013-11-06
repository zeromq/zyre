/*  =========================================================================
    zyre_logmsg - work with zre logging messages

    Generated codec implementation for zyre_logmsg
    -------------------------------------------------------------------------
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
    zyre_logmsg - work with zre logging messages
@discuss
@end
*/

#include <czmq.h>
#include "../include/zyre_logmsg.h"

//  Structure of our class

struct _zyre_logmsg_t {
    zframe_t *address;          //  Address of peer if any
    int id;                     //  zyre_logmsg message ID
    byte *needle;               //  Read/write pointer for serialization
    byte *ceiling;              //  Valid upper limit for read pointer
    byte level;
    byte event;
    uint16_t node;
    uint16_t peer;
    uint64_t time;
    char *data;
};

//  --------------------------------------------------------------------------
//  Network data encoding macros

//  Strings are encoded with 1-byte length
#define STRING_MAX  255

//  Put a block to the frame
#define PUT_BLOCK(host,size) { \
    memcpy (self->needle, (host), size); \
    self->needle += size; \
    }

//  Get a block from the frame
#define GET_BLOCK(host,size) { \
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
    string_size = strlen (host); \
    PUT_NUMBER1 (string_size); \
    memcpy (self->needle, (host), string_size); \
    self->needle += string_size; \
    }

//  Get a string from the frame
#define GET_STRING(host) { \
    GET_NUMBER1 (string_size); \
    if (self->needle + string_size > (self->ceiling)) \
        goto malformed; \
    (host) = (char *) malloc (string_size + 1); \
    memcpy ((host), self->needle, string_size); \
    (host) [string_size] = 0; \
    self->needle += string_size; \
    }


//  --------------------------------------------------------------------------
//  Create a new zyre_logmsg

zyre_logmsg_t *
zyre_logmsg_new (int id)
{
    zyre_logmsg_t *self = (zyre_logmsg_t *) zmalloc (sizeof (zyre_logmsg_t));
    self->id = id;
    return self;
}


//  --------------------------------------------------------------------------
//  Destroy the zyre_logmsg

void
zyre_logmsg_destroy (zyre_logmsg_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zyre_logmsg_t *self = *self_p;

        //  Free class properties
        zframe_destroy (&self->address);
        free (self->data);

        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}


//  --------------------------------------------------------------------------
//  Receive and parse a zyre_logmsg from the socket. Returns new object or
//  NULL if error. Will block if there's no message waiting.

zyre_logmsg_t *
zyre_logmsg_recv (void *input)
{
    assert (input);
    zyre_logmsg_t *self = zyre_logmsg_new (0);
    zframe_t *frame = NULL;
    size_t string_size;
    size_t list_size;
    size_t hash_size;

    //  Read valid message frame from socket; we loop over any
    //  garbage data we might receive from badly-connected peers
    while (true) {
        //  If we're reading from a ROUTER socket, get address
        if (zsocket_type (input) == ZMQ_ROUTER) {
            zframe_destroy (&self->address);
            self->address = zframe_recv (input);
            if (!self->address)
                goto empty;         //  Interrupted
            if (!zsocket_rcvmore (input))
                goto malformed;
        }
        //  Read and parse command in frame
        frame = zframe_recv (input);
        if (!frame)
            goto empty;             //  Interrupted

        //  Get and check protocol signature
        self->needle = zframe_data (frame);
        self->ceiling = self->needle + zframe_size (frame);
        uint16_t signature;
        GET_NUMBER2 (signature);
        if (signature == (0xAAA0 | 2))
            break;                  //  Valid signature

        //  Protocol assertion, drop message
        while (zsocket_rcvmore (input)) {
            zframe_destroy (&frame);
            frame = zframe_recv (input);
        }
        zframe_destroy (&frame);
    }
    //  Get message id and parse per message type
    GET_NUMBER1 (self->id);

    switch (self->id) {
        case ZYRE_LOGMSG_LOG:
            GET_NUMBER1 (self->level);
            GET_NUMBER1 (self->event);
            GET_NUMBER2 (self->node);
            GET_NUMBER2 (self->peer);
            GET_NUMBER8 (self->time);
            free (self->data);
            GET_STRING (self->data);
            break;

        default:
            goto malformed;
    }
    //  Successful return
    zframe_destroy (&frame);
    return self;

    //  Error returns
    malformed:
        printf ("E: malformed message '%d'\n", self->id);
    empty:
        zframe_destroy (&frame);
        zyre_logmsg_destroy (&self);
        return (NULL);
}


//  --------------------------------------------------------------------------
//  Send the zyre_logmsg to the socket, and destroy it
//  Returns 0 if OK, else -1

int
zyre_logmsg_send (zyre_logmsg_t **self_p, void *output)
{
    assert (output);
    assert (self_p);
    assert (*self_p);

    //  Calculate size of serialized data
    zyre_logmsg_t *self = *self_p;
    size_t frame_size = 2 + 1;          //  Signature and message ID
    switch (self->id) {
        case ZYRE_LOGMSG_LOG:
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
    size_t string_size;
    int frame_flags = 0;
    PUT_NUMBER2 (0xAAA0 | 2);
    PUT_NUMBER1 (self->id);

    switch (self->id) {
        case ZYRE_LOGMSG_LOG:
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
    //  If we're sending to a ROUTER, we send the address first
    if (zsocket_type (output) == ZMQ_ROUTER) {
        assert (self->address);
        if (zframe_send (&self->address, output, ZFRAME_MORE)) {
            zframe_destroy (&frame);
            zyre_logmsg_destroy (self_p);
            return -1;
        }
    }
    //  Now send the data frame
    if (zframe_send (&frame, output, frame_flags)) {
        zframe_destroy (&frame);
        zyre_logmsg_destroy (self_p);
        return -1;
    }
    
    //  Now send any frame fields, in order
    switch (self->id) {
    }
    //  Destroy zyre_logmsg object
    zyre_logmsg_destroy (self_p);
    return 0;
}


//  --------------------------------------------------------------------------
//  Send the LOG to the socket in one step

int
zyre_logmsg_send_log (
    void *output,
    byte level,
    byte event,
    uint16_t node,
    uint16_t peer,
    uint64_t time,
    char *data)
{
    zyre_logmsg_t *self = zyre_logmsg_new (ZYRE_LOGMSG_LOG);
    zyre_logmsg_set_level (self, level);
    zyre_logmsg_set_event (self, event);
    zyre_logmsg_set_node (self, node);
    zyre_logmsg_set_peer (self, peer);
    zyre_logmsg_set_time (self, time);
    zyre_logmsg_set_data (self, data);
    return zyre_logmsg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Duplicate the zyre_logmsg message

zyre_logmsg_t *
zyre_logmsg_dup (zyre_logmsg_t *self)
{
    if (!self)
        return NULL;
        
    zyre_logmsg_t *copy = zyre_logmsg_new (self->id);
    if (self->address)
        copy->address = zframe_dup (self->address);
    switch (self->id) {
        case ZYRE_LOGMSG_LOG:
            copy->level = self->level;
            copy->event = self->event;
            copy->node = self->node;
            copy->peer = self->peer;
            copy->time = self->time;
            copy->data = strdup (self->data);
            break;

    }
    return copy;
}



//  --------------------------------------------------------------------------
//  Print contents of message to stdout

void
zyre_logmsg_dump (zyre_logmsg_t *self)
{
    assert (self);
    switch (self->id) {
        case ZYRE_LOGMSG_LOG:
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
//  Get/set the message address

zframe_t *
zyre_logmsg_address (zyre_logmsg_t *self)
{
    assert (self);
    return self->address;
}

void
zyre_logmsg_set_address (zyre_logmsg_t *self, zframe_t *address)
{
    if (self->address)
        zframe_destroy (&self->address);
    self->address = zframe_dup (address);
}


//  --------------------------------------------------------------------------
//  Get/set the zyre_logmsg id

int
zyre_logmsg_id (zyre_logmsg_t *self)
{
    assert (self);
    return self->id;
}

void
zyre_logmsg_set_id (zyre_logmsg_t *self, int id)
{
    self->id = id;
}

//  --------------------------------------------------------------------------
//  Return a printable command string

char *
zyre_logmsg_command (zyre_logmsg_t *self)
{
    assert (self);
    switch (self->id) {
        case ZYRE_LOGMSG_LOG:
            return ("LOG");
            break;
    }
    return "?";
}

//  --------------------------------------------------------------------------
//  Get/set the level field

byte
zyre_logmsg_level (zyre_logmsg_t *self)
{
    assert (self);
    return self->level;
}

void
zyre_logmsg_set_level (zyre_logmsg_t *self, byte level)
{
    assert (self);
    self->level = level;
}


//  --------------------------------------------------------------------------
//  Get/set the event field

byte
zyre_logmsg_event (zyre_logmsg_t *self)
{
    assert (self);
    return self->event;
}

void
zyre_logmsg_set_event (zyre_logmsg_t *self, byte event)
{
    assert (self);
    self->event = event;
}


//  --------------------------------------------------------------------------
//  Get/set the node field

uint16_t
zyre_logmsg_node (zyre_logmsg_t *self)
{
    assert (self);
    return self->node;
}

void
zyre_logmsg_set_node (zyre_logmsg_t *self, uint16_t node)
{
    assert (self);
    self->node = node;
}


//  --------------------------------------------------------------------------
//  Get/set the peer field

uint16_t
zyre_logmsg_peer (zyre_logmsg_t *self)
{
    assert (self);
    return self->peer;
}

void
zyre_logmsg_set_peer (zyre_logmsg_t *self, uint16_t peer)
{
    assert (self);
    self->peer = peer;
}


//  --------------------------------------------------------------------------
//  Get/set the time field

uint64_t
zyre_logmsg_time (zyre_logmsg_t *self)
{
    assert (self);
    return self->time;
}

void
zyre_logmsg_set_time (zyre_logmsg_t *self, uint64_t time)
{
    assert (self);
    self->time = time;
}


//  --------------------------------------------------------------------------
//  Get/set the data field

char *
zyre_logmsg_data (zyre_logmsg_t *self)
{
    assert (self);
    return self->data;
}

void
zyre_logmsg_set_data (zyre_logmsg_t *self, char *format, ...)
{
    //  Format into newly allocated string
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    free (self->data);
    self->data = (char *) malloc (STRING_MAX + 1);
    assert (self->data);
    vsnprintf (self->data, STRING_MAX, format, argptr);
    va_end (argptr);
}



//  --------------------------------------------------------------------------
//  Selftest

int
zyre_logmsg_test (bool verbose)
{
    printf (" * zyre_logmsg: ");

    //  @selftest
    //  Simple create/destroy test
    zyre_logmsg_t *self = zyre_logmsg_new (0);
    assert (self);
    zyre_logmsg_destroy (&self);

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

    self = zyre_logmsg_new (ZYRE_LOGMSG_LOG);
    zyre_logmsg_set_level (self, 123);
    zyre_logmsg_set_event (self, 123);
    zyre_logmsg_set_node (self, 123);
    zyre_logmsg_set_peer (self, 123);
    zyre_logmsg_set_time (self, 123);
    zyre_logmsg_set_data (self, "Life is short but Now lasts for ever");
    zyre_logmsg_send (&self, output);
    
    self = zyre_logmsg_recv (input);
    assert (self);
    assert (zyre_logmsg_level (self) == 123);
    assert (zyre_logmsg_event (self) == 123);
    assert (zyre_logmsg_node (self) == 123);
    assert (zyre_logmsg_peer (self) == 123);
    assert (zyre_logmsg_time (self) == 123);
    assert (streq (zyre_logmsg_data (self), "Life is short but Now lasts for ever"));
    zyre_logmsg_destroy (&self);

    zctx_destroy (&ctx);
    //  @end

    printf ("OK\n");
    return 0;
}
