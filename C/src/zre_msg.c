/*  =========================================================================
    zre_msg.c

    Generated codec implementation for zre_msg
    -------------------------------------------------------------------------
    Copyright (c) 1991-2012 iMatix Corporation -- http://www.imatix.com     
    Copyright other contributors as noted in the AUTHORS file.              
                                                                            
    This file is part of FILEMQ, see http://filemq.org.                     
                                                                            
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

#include <czmq.h>
#include "../include/zre_msg.h"

//  Structure of our class

struct _zre_msg_t {
    zframe_t *address;          //  Address of peer if any
    int id;                     //  zre_msg message ID
    byte *needle;               //  Read/write pointer for serialization
    byte *ceiling;              //  Valid upper limit for read pointer
    char *from;
    int64_t port;
    zframe_t *cookies;
};

//  --------------------------------------------------------------------------
//  Network data encoding macros

//  Strings are encoded with 1-byte length
#define STRING_MAX  255

//  Put an octet to the frame
#define PUT_OCTET(host) { \
    *(byte *) self->needle = (host); \
    self->needle++; \
    }

//  Get an octet from the frame
#define GET_OCTET(host) { \
    if (self->needle + 1 > self->ceiling) \
        goto malformed; \
    (host) = *(byte *) self->needle; \
    self->needle++; \
    }

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

//  Put a long long integer to the frame
#define PUT_NUMBER(host) { \
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

//  Get a number from the frame
#define GET_NUMBER(host) { \
    if (self->needle + 8 > self->ceiling) \
        goto malformed; \
    (host) = ((int64_t) (self->needle [0]) << 56) \
           + ((int64_t) (self->needle [1]) << 48) \
           + ((int64_t) (self->needle [2]) << 40) \
           + ((int64_t) (self->needle [3]) << 32) \
           + ((int64_t) (self->needle [4]) << 24) \
           + ((int64_t) (self->needle [5]) << 16) \
           + ((int64_t) (self->needle [6]) << 8) \
           +  (int64_t) (self->needle [7]); \
    self->needle += 8; \
    }

//  Put a string to the frame
#define PUT_STRING(host) { \
    string_size = strlen (host); \
    PUT_OCTET (string_size); \
    memcpy (self->needle, (host), string_size); \
    self->needle += string_size; \
    }

//  Get a string from the frame
#define GET_STRING(host) { \
    GET_OCTET (string_size); \
    if (self->needle + string_size > (self->ceiling)) \
        goto malformed; \
    (host) = (char *) malloc (string_size + 1); \
    memcpy ((host), self->needle, string_size); \
    (host) [string_size] = 0; \
    self->needle += string_size; \
    }


//  --------------------------------------------------------------------------
//  Create a new zre_msg

zre_msg_t *
zre_msg_new (int id)
{
    zre_msg_t *self = (zre_msg_t *) zmalloc (sizeof (zre_msg_t));
    self->id = id;
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
        zframe_destroy (&self->address);
        free (self->from);
        zframe_destroy (&self->cookies);

        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}


//  --------------------------------------------------------------------------
//  Receive and parse a zre_msg from the socket. Returns new object or
//  NULL if error. Will block if there's no message waiting.

zre_msg_t *
zre_msg_recv (void *socket)
{
    assert (socket);
    zre_msg_t *self = zre_msg_new (0);
    zframe_t *frame = NULL;

    //  If we're reading from a ROUTER socket, get address
    if (zsockopt_type (socket) == ZMQ_ROUTER) {
        self->address = zframe_recv (socket);
        if (!self->address)
            goto empty;         //  Interrupted
        if (!zsocket_rcvmore (socket))
            goto malformed;
    }
    //  Read and parse command in frame
    frame = zframe_recv (socket);
    if (!frame)
        goto empty;             //  Interrupted
    self->needle = zframe_data (frame);
    self->ceiling = self->needle + zframe_size (frame);
    size_t string_size;
    size_t list_size;
    size_t hash_size;

    //  Get message id, which is first byte in frame
    GET_OCTET (self->id);

    switch (self->id) {
        case ZRE_MSG_OHAI:
            free (self->from);
            GET_STRING (self->from);
            GET_NUMBER (self->port);
            break;

        case ZRE_MSG_NOM:
            //  Get next frame, leave current untouched
            if (!zsocket_rcvmore (socket))
                goto malformed;
            self->cookies = zframe_recv (socket);
            break;

        case ZRE_MSG_HUGZ:
            break;

        case ZRE_MSG_HUGZ_OK:
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
        zre_msg_destroy (&self);
        return (NULL);
}



//  --------------------------------------------------------------------------
//  Send the zre_msg to the socket, and destroy it

void
zre_msg_send (zre_msg_t **self_p, void *socket)
{
    assert (socket);
    assert (self_p);
    assert (*self_p);

    //  Calculate size of serialized data
    zre_msg_t *self = *self_p;
    size_t frame_size = 1;
    switch (self->id) {
        case ZRE_MSG_OHAI:
            //  from is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->from)
                frame_size += strlen (self->from);
            //  port is an 8-byte integer
            frame_size += 8;
            break;
            
        case ZRE_MSG_NOM:
            break;
            
        case ZRE_MSG_HUGZ:
            break;
            
        case ZRE_MSG_HUGZ_OK:
            break;
            
        default:
            printf ("E: bad message type '%d', not sent\n", self->id);
            return;
    }
    //  Now serialize message into the frame
    zframe_t *frame = zframe_new (NULL, frame_size + 1);
    self->needle = zframe_data (frame);
    size_t string_size;
    int frame_flags = 0;
    PUT_OCTET (self->id);

    switch (self->id) {
        case ZRE_MSG_OHAI:
            if (self->from) {
                PUT_STRING (self->from);
            }
            else
                PUT_OCTET (0);      //  Empty string
            PUT_NUMBER (self->port);
            break;
            
        case ZRE_MSG_NOM:
            frame_flags = ZFRAME_MORE;
            break;
            
        case ZRE_MSG_HUGZ:
            break;
            
        case ZRE_MSG_HUGZ_OK:
            break;
            
    }
    //  If we're sending to a ROUTER, we send the address first
    if (zsockopt_type (socket) == ZMQ_ROUTER) {
        assert (self->address);
        zframe_send (&self->address, socket, ZFRAME_MORE);
    }
    //  Now send the data frame
    zframe_send (&frame, socket, frame_flags);
    
    //  Now send any frame fields, in order
    switch (self->id) {
        case ZRE_MSG_NOM:
        //  If cookies isn't set, send an empty frame
        if (!self->cookies)
            self->cookies = zframe_new (NULL, 0);
            zframe_send (&self->cookies, socket, 0);
            break;
    }
    //  Destroy zre_msg object
    zre_msg_destroy (self_p);
}



//  --------------------------------------------------------------------------
//  Print contents of message to stdout

void
zre_msg_dump (zre_msg_t *self)
{
    assert (self);
    switch (self->id) {
        case ZRE_MSG_OHAI:
            puts ("OHAI:");
            if (self->from)
                printf ("    from='%s'\n", self->from);
            else
                printf ("    from=\n");
            printf ("    port=%ld\n", (long) self->port);
            break;
            
        case ZRE_MSG_NOM:
            puts ("NOM:");
            printf ("    cookies={\n");
            if (self->cookies) {
                size_t size = zframe_size (self->cookies);
                byte *data = zframe_data (self->cookies);
                printf ("        size=%td\n", zframe_size (self->cookies));
                if (size > 32)
                    size = 32;
                int cookies_index;
                for (cookies_index = 0; cookies_index < size; cookies_index++) {
                    if (cookies_index && (cookies_index % 4 == 0))
                        printf ("-");
                    printf ("%02X", data [cookies_index]);
                }
            }
            printf ("    }\n");
            break;
            
        case ZRE_MSG_HUGZ:
            puts ("HUGZ:");
            break;
            
        case ZRE_MSG_HUGZ_OK:
            puts ("HUGZ_OK:");
            break;
            
    }
}


//  --------------------------------------------------------------------------
//  Get/set the message address

zframe_t *
zre_msg_address (zre_msg_t *self)
{
    assert (self);
    return self->address;
}

void
zre_msg_address_set (zre_msg_t *self, zframe_t *address)
{
    if (self->address)
        zframe_destroy (&self->address);
    self->address = zframe_dup (address);
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
zre_msg_id_set (zre_msg_t *self, int id)
{
    self->id = id;
}

//  --------------------------------------------------------------------------
//  Get/set the from field

char *
zre_msg_from (zre_msg_t *self)
{
    assert (self);
    return self->from;
}

void
zre_msg_from_set (zre_msg_t *self, char *format, ...)
{
    //  Format into newly allocated string
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    free (self->from);
    self->from = (char *) malloc (STRING_MAX + 1);
    assert (self->from);
    vsnprintf (self->from, STRING_MAX, format, argptr);
    va_end (argptr);
}


//  --------------------------------------------------------------------------
//  Get/set the port field

int64_t
zre_msg_port (zre_msg_t *self)
{
    assert (self);
    return self->port;
}

void
zre_msg_port_set (zre_msg_t *self, int64_t port)
{
    assert (self);
    self->port = port;
}


//  --------------------------------------------------------------------------
//  Get/set the cookies field

zframe_t *
zre_msg_cookies (zre_msg_t *self)
{
    assert (self);
    return self->cookies;
}

//  Takes ownership of supplied frame
void
zre_msg_cookies_set (zre_msg_t *self, zframe_t *frame)
{
    assert (self);
    if (self->cookies)
        zframe_destroy (&self->cookies);
    self->cookies = frame;
}


//  --------------------------------------------------------------------------
//  Selftest

int
zre_msg_test (bool verbose)
{
    printf (" * zre_msg: ");

    //  Simple create/destroy test
    zre_msg_t *self = zre_msg_new (0);
    assert (self);
    zre_msg_destroy (&self);

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

    self = zre_msg_new (ZRE_MSG_OHAI);
    zre_msg_from_set (self, "Life is short but Now lasts for ever");
    zre_msg_port_set (self, 12345678);
    zre_msg_send (&self, output);
    
    self = zre_msg_recv (input);
    assert (self);
    assert (streq (zre_msg_from (self), "Life is short but Now lasts for ever"));
    assert (zre_msg_port (self) == 12345678);
    zre_msg_destroy (&self);

    self = zre_msg_new (ZRE_MSG_NOM);
    zre_msg_cookies_set (self, zframe_new ("Captcha Diem", 12));
    zre_msg_send (&self, output);
    
    self = zre_msg_recv (input);
    assert (self);
    assert (zframe_streq (zre_msg_cookies (self), "Captcha Diem"));
    zre_msg_destroy (&self);

    self = zre_msg_new (ZRE_MSG_HUGZ);
    zre_msg_send (&self, output);
    
    self = zre_msg_recv (input);
    assert (self);
    zre_msg_destroy (&self);

    self = zre_msg_new (ZRE_MSG_HUGZ_OK);
    zre_msg_send (&self, output);
    
    self = zre_msg_recv (input);
    assert (self);
    zre_msg_destroy (&self);

    zctx_destroy (&ctx);
    printf ("OK\n");
    return 0;
}
