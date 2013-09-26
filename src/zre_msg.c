/*  =========================================================================
    zre_msg - work with zre messages

    Generated codec implementation for zre_msg
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
    zre_msg - work with zre messages
@discuss
@end
*/

#include <czmq.h>
#include "../include/zre_msg.h"

//  Structure of our class

struct _zre_msg_t {
    zframe_t *address;          //  Address of peer if any
    int id;                     //  zre_msg message ID
    byte *needle;               //  Read/write pointer for serialization
    byte *ceiling;              //  Valid upper limit for read pointer
    uint16_t sequence;
    char *ipaddress;
    uint16_t mailbox;
    zlist_t *groups;
    byte status;
    zhash_t *headers;
    size_t headers_bytes;       //  Size of dictionary content
    zframe_t *content;
    char *group;
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
        free (self->ipaddress);
        if (self->groups)
            zlist_destroy (&self->groups);
        zhash_destroy (&self->headers);
        zframe_destroy (&self->content);
        free (self->group);

        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}


//  --------------------------------------------------------------------------
//  Receive and parse a zre_msg from the socket. Returns new object or
//  NULL if error. Will block if there's no message waiting.

zre_msg_t *
zre_msg_recv (void *input)
{
    assert (input);
    zre_msg_t *self = zre_msg_new (0);
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
        if (signature == (0xAAA0 | 1))
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
        case ZRE_MSG_HELLO:
            GET_NUMBER2 (self->sequence);
            free (self->ipaddress);
            GET_STRING (self->ipaddress);
            GET_NUMBER2 (self->mailbox);
            GET_NUMBER1 (list_size);
            self->groups = zlist_new ();
            zlist_autofree (self->groups);
            while (list_size--) {
                char *string;
                GET_STRING (string);
                zlist_append (self->groups, string);
                free (string);
            }
            GET_NUMBER1 (self->status);
            GET_NUMBER1 (hash_size);
            self->headers = zhash_new ();
            zhash_autofree (self->headers);
            while (hash_size--) {
                char *string;
                GET_STRING (string);
                char *value = strchr (string, '=');
                if (value)
                    *value++ = 0;
                zhash_insert (self->headers, string, value);
                free (string);
            }
            break;

        case ZRE_MSG_WHISPER:
            GET_NUMBER2 (self->sequence);
            //  Get next frame, leave current untouched
            if (!zsocket_rcvmore (input))
                goto malformed;
            self->content = zframe_recv (input);
            break;

        case ZRE_MSG_SHOUT:
            GET_NUMBER2 (self->sequence);
            free (self->group);
            GET_STRING (self->group);
            //  Get next frame, leave current untouched
            if (!zsocket_rcvmore (input))
                goto malformed;
            self->content = zframe_recv (input);
            break;

        case ZRE_MSG_JOIN:
            GET_NUMBER2 (self->sequence);
            free (self->group);
            GET_STRING (self->group);
            GET_NUMBER1 (self->status);
            break;

        case ZRE_MSG_LEAVE:
            GET_NUMBER2 (self->sequence);
            free (self->group);
            GET_STRING (self->group);
            GET_NUMBER1 (self->status);
            break;

        case ZRE_MSG_PING:
            GET_NUMBER2 (self->sequence);
            break;

        case ZRE_MSG_PING_OK:
            GET_NUMBER2 (self->sequence);
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

//  Count size of key=value pair
static int
s_headers_count (const char *key, void *item, void *argument)
{
    zre_msg_t *self = (zre_msg_t *) argument;
    self->headers_bytes += strlen (key) + 1 + strlen ((char *) item) + 1;
    return 0;
}

//  Serialize headers key=value pair
static int
s_headers_write (const char *key, void *item, void *argument)
{
    zre_msg_t *self = (zre_msg_t *) argument;
    char string [STRING_MAX + 1];
    snprintf (string, STRING_MAX, "%s=%s", key, (char *) item);
    size_t string_size;
    PUT_STRING (string);
    return 0;
}


//  --------------------------------------------------------------------------
//  Send the zre_msg to the socket, and destroy it
//  Returns 0 if OK, else -1

int
zre_msg_send (zre_msg_t **self_p, void *output)
{
    assert (output);
    assert (self_p);
    assert (*self_p);

    //  Calculate size of serialized data
    zre_msg_t *self = *self_p;
    size_t frame_size = 2 + 1;          //  Signature and message ID
    switch (self->id) {
        case ZRE_MSG_HELLO:
            //  sequence is a 2-byte integer
            frame_size += 2;
            //  ipaddress is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->ipaddress)
                frame_size += strlen (self->ipaddress);
            //  mailbox is a 2-byte integer
            frame_size += 2;
            //  groups is an array of strings
            frame_size++;       //  Size is one octet
            if (self->groups) {
                //  Add up size of list contents
                char *groups = (char *) zlist_first (self->groups);
                while (groups) {
                    frame_size += 1 + strlen (groups);
                    groups = (char *) zlist_next (self->groups);
                }
            }
            //  status is a 1-byte integer
            frame_size += 1;
            //  headers is an array of key=value strings
            frame_size++;       //  Size is one octet
            if (self->headers) {
                self->headers_bytes = 0;
                //  Add up size of dictionary contents
                zhash_foreach (self->headers, s_headers_count, self);
            }
            frame_size += self->headers_bytes;
            break;
            
        case ZRE_MSG_WHISPER:
            //  sequence is a 2-byte integer
            frame_size += 2;
            break;
            
        case ZRE_MSG_SHOUT:
            //  sequence is a 2-byte integer
            frame_size += 2;
            //  group is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->group)
                frame_size += strlen (self->group);
            break;
            
        case ZRE_MSG_JOIN:
            //  sequence is a 2-byte integer
            frame_size += 2;
            //  group is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->group)
                frame_size += strlen (self->group);
            //  status is a 1-byte integer
            frame_size += 1;
            break;
            
        case ZRE_MSG_LEAVE:
            //  sequence is a 2-byte integer
            frame_size += 2;
            //  group is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->group)
                frame_size += strlen (self->group);
            //  status is a 1-byte integer
            frame_size += 1;
            break;
            
        case ZRE_MSG_PING:
            //  sequence is a 2-byte integer
            frame_size += 2;
            break;
            
        case ZRE_MSG_PING_OK:
            //  sequence is a 2-byte integer
            frame_size += 2;
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
    PUT_NUMBER2 (0xAAA0 | 1);
    PUT_NUMBER1 (self->id);

    switch (self->id) {
        case ZRE_MSG_HELLO:
            PUT_NUMBER2 (self->sequence);
            if (self->ipaddress) {
                PUT_STRING (self->ipaddress);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            PUT_NUMBER2 (self->mailbox);
            if (self->groups != NULL) {
                PUT_NUMBER1 (zlist_size (self->groups));
                char *groups = (char *) zlist_first (self->groups);
                while (groups) {
                    PUT_STRING (groups);
                    groups = (char *) zlist_next (self->groups);
                }
            }
            else
                PUT_NUMBER1 (0);    //  Empty string array
            PUT_NUMBER1 (self->status);
            if (self->headers != NULL) {
                PUT_NUMBER1 (zhash_size (self->headers));
                zhash_foreach (self->headers, s_headers_write, self);
            }
            else
                PUT_NUMBER1 (0);    //  Empty dictionary
            break;
            
        case ZRE_MSG_WHISPER:
            PUT_NUMBER2 (self->sequence);
            frame_flags = ZFRAME_MORE;
            break;
            
        case ZRE_MSG_SHOUT:
            PUT_NUMBER2 (self->sequence);
            if (self->group) {
                PUT_STRING (self->group);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            frame_flags = ZFRAME_MORE;
            break;
            
        case ZRE_MSG_JOIN:
            PUT_NUMBER2 (self->sequence);
            if (self->group) {
                PUT_STRING (self->group);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            PUT_NUMBER1 (self->status);
            break;
            
        case ZRE_MSG_LEAVE:
            PUT_NUMBER2 (self->sequence);
            if (self->group) {
                PUT_STRING (self->group);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            PUT_NUMBER1 (self->status);
            break;
            
        case ZRE_MSG_PING:
            PUT_NUMBER2 (self->sequence);
            break;
            
        case ZRE_MSG_PING_OK:
            PUT_NUMBER2 (self->sequence);
            break;
            
    }
    //  If we're sending to a ROUTER, we send the address first
    if (zsocket_type (output) == ZMQ_ROUTER) {
        assert (self->address);
        if (zframe_send (&self->address, output, ZFRAME_MORE)) {
            zframe_destroy (&frame);
            zre_msg_destroy (self_p);
            return -1;
        }
    }
    //  Now send the data frame
    if (zframe_send (&frame, output, frame_flags)) {
        zframe_destroy (&frame);
        zre_msg_destroy (self_p);
        return -1;
    }
    
    //  Now send any frame fields, in order
    switch (self->id) {
        case ZRE_MSG_WHISPER:
            //  If content isn't set, send an empty frame
            if (!self->content)
                self->content = zframe_new (NULL, 0);
            if (zframe_send (&self->content, output, 0)) {
                zframe_destroy (&frame);
                zre_msg_destroy (self_p);
                return -1;
            }
            break;
        case ZRE_MSG_SHOUT:
            //  If content isn't set, send an empty frame
            if (!self->content)
                self->content = zframe_new (NULL, 0);
            if (zframe_send (&self->content, output, 0)) {
                zframe_destroy (&frame);
                zre_msg_destroy (self_p);
                return -1;
            }
            break;
    }
    //  Destroy zre_msg object
    zre_msg_destroy (self_p);
    return 0;
}


//  --------------------------------------------------------------------------
//  Send the HELLO to the socket in one step

int
zre_msg_send_hello (
    void *output,
    uint16_t sequence,
    char *ipaddress,
    uint16_t mailbox,
    zlist_t *groups,
    byte status,
    zhash_t *headers)
{
    zre_msg_t *self = zre_msg_new (ZRE_MSG_HELLO);
    zre_msg_set_sequence (self, sequence);
    zre_msg_set_ipaddress (self, ipaddress);
    zre_msg_set_mailbox (self, mailbox);
    zre_msg_set_groups (self, zlist_dup (groups));
    zre_msg_set_status (self, status);
    zre_msg_set_headers (self, zhash_dup (headers));
    return zre_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the WHISPER to the socket in one step

int
zre_msg_send_whisper (
    void *output,
    uint16_t sequence,
    zframe_t *content)
{
    zre_msg_t *self = zre_msg_new (ZRE_MSG_WHISPER);
    zre_msg_set_sequence (self, sequence);
    zre_msg_set_content (self, zframe_dup (content));
    return zre_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the SHOUT to the socket in one step

int
zre_msg_send_shout (
    void *output,
    uint16_t sequence,
    char *group,
    zframe_t *content)
{
    zre_msg_t *self = zre_msg_new (ZRE_MSG_SHOUT);
    zre_msg_set_sequence (self, sequence);
    zre_msg_set_group (self, group);
    zre_msg_set_content (self, zframe_dup (content));
    return zre_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the JOIN to the socket in one step

int
zre_msg_send_join (
    void *output,
    uint16_t sequence,
    char *group,
    byte status)
{
    zre_msg_t *self = zre_msg_new (ZRE_MSG_JOIN);
    zre_msg_set_sequence (self, sequence);
    zre_msg_set_group (self, group);
    zre_msg_set_status (self, status);
    return zre_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the LEAVE to the socket in one step

int
zre_msg_send_leave (
    void *output,
    uint16_t sequence,
    char *group,
    byte status)
{
    zre_msg_t *self = zre_msg_new (ZRE_MSG_LEAVE);
    zre_msg_set_sequence (self, sequence);
    zre_msg_set_group (self, group);
    zre_msg_set_status (self, status);
    return zre_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the PING to the socket in one step

int
zre_msg_send_ping (
    void *output,
    uint16_t sequence)
{
    zre_msg_t *self = zre_msg_new (ZRE_MSG_PING);
    zre_msg_set_sequence (self, sequence);
    return zre_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the PING_OK to the socket in one step

int
zre_msg_send_ping_ok (
    void *output,
    uint16_t sequence)
{
    zre_msg_t *self = zre_msg_new (ZRE_MSG_PING_OK);
    zre_msg_set_sequence (self, sequence);
    return zre_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Duplicate the zre_msg message

zre_msg_t *
zre_msg_dup (zre_msg_t *self)
{
    if (!self)
        return NULL;
        
    zre_msg_t *copy = zre_msg_new (self->id);
    if (self->address)
        copy->address = zframe_dup (self->address);
    switch (self->id) {
        case ZRE_MSG_HELLO:
            copy->sequence = self->sequence;
            copy->ipaddress = strdup (self->ipaddress);
            copy->mailbox = self->mailbox;
            copy->groups = zlist_dup (self->groups);
            copy->status = self->status;
            copy->headers = zhash_dup (self->headers);
            break;

        case ZRE_MSG_WHISPER:
            copy->sequence = self->sequence;
            copy->content = zframe_dup (self->content);
            break;

        case ZRE_MSG_SHOUT:
            copy->sequence = self->sequence;
            copy->group = strdup (self->group);
            copy->content = zframe_dup (self->content);
            break;

        case ZRE_MSG_JOIN:
            copy->sequence = self->sequence;
            copy->group = strdup (self->group);
            copy->status = self->status;
            break;

        case ZRE_MSG_LEAVE:
            copy->sequence = self->sequence;
            copy->group = strdup (self->group);
            copy->status = self->status;
            break;

        case ZRE_MSG_PING:
            copy->sequence = self->sequence;
            break;

        case ZRE_MSG_PING_OK:
            copy->sequence = self->sequence;
            break;

    }
    return copy;
}


//  Dump headers key=value pair to stdout
static int
s_headers_dump (const char *key, void *item, void *argument)
{
    zre_msg_t *self = (zre_msg_t *) argument;
    printf ("        %s=%s\n", key, (char *) item);
    return 0;
}


//  --------------------------------------------------------------------------
//  Print contents of message to stdout

void
zre_msg_dump (zre_msg_t *self)
{
    assert (self);
    switch (self->id) {
        case ZRE_MSG_HELLO:
            puts ("HELLO:");
            printf ("    sequence=%ld\n", (long) self->sequence);
            if (self->ipaddress)
                printf ("    ipaddress='%s'\n", self->ipaddress);
            else
                printf ("    ipaddress=\n");
            printf ("    mailbox=%ld\n", (long) self->mailbox);
            printf ("    groups={");
            if (self->groups) {
                char *groups = (char *) zlist_first (self->groups);
                while (groups) {
                    printf (" '%s'", groups);
                    groups = (char *) zlist_next (self->groups);
                }
            }
            printf (" }\n");
            printf ("    status=%ld\n", (long) self->status);
            printf ("    headers={\n");
            if (self->headers)
                zhash_foreach (self->headers, s_headers_dump, self);
            printf ("    }\n");
            break;
            
        case ZRE_MSG_WHISPER:
            puts ("WHISPER:");
            printf ("    sequence=%ld\n", (long) self->sequence);
            printf ("    content={\n");
            if (self->content) {
                size_t size = zframe_size (self->content);
                byte *data = zframe_data (self->content);
                printf ("        size=%td\n", zframe_size (self->content));
                if (size > 32)
                    size = 32;
                int content_index;
                for (content_index = 0; content_index < size; content_index++) {
                    if (content_index && (content_index % 4 == 0))
                        printf ("-");
                    printf ("%02X", data [content_index]);
                }
            }
            printf ("    }\n");
            break;
            
        case ZRE_MSG_SHOUT:
            puts ("SHOUT:");
            printf ("    sequence=%ld\n", (long) self->sequence);
            if (self->group)
                printf ("    group='%s'\n", self->group);
            else
                printf ("    group=\n");
            printf ("    content={\n");
            if (self->content) {
                size_t size = zframe_size (self->content);
                byte *data = zframe_data (self->content);
                printf ("        size=%td\n", zframe_size (self->content));
                if (size > 32)
                    size = 32;
                int content_index;
                for (content_index = 0; content_index < size; content_index++) {
                    if (content_index && (content_index % 4 == 0))
                        printf ("-");
                    printf ("%02X", data [content_index]);
                }
            }
            printf ("    }\n");
            break;
            
        case ZRE_MSG_JOIN:
            puts ("JOIN:");
            printf ("    sequence=%ld\n", (long) self->sequence);
            if (self->group)
                printf ("    group='%s'\n", self->group);
            else
                printf ("    group=\n");
            printf ("    status=%ld\n", (long) self->status);
            break;
            
        case ZRE_MSG_LEAVE:
            puts ("LEAVE:");
            printf ("    sequence=%ld\n", (long) self->sequence);
            if (self->group)
                printf ("    group='%s'\n", self->group);
            else
                printf ("    group=\n");
            printf ("    status=%ld\n", (long) self->status);
            break;
            
        case ZRE_MSG_PING:
            puts ("PING:");
            printf ("    sequence=%ld\n", (long) self->sequence);
            break;
            
        case ZRE_MSG_PING_OK:
            puts ("PING_OK:");
            printf ("    sequence=%ld\n", (long) self->sequence);
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
zre_msg_set_address (zre_msg_t *self, zframe_t *address)
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
zre_msg_set_id (zre_msg_t *self, int id)
{
    self->id = id;
}

//  --------------------------------------------------------------------------
//  Return a printable command string

char *
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
//  Get/set the ipaddress field

char *
zre_msg_ipaddress (zre_msg_t *self)
{
    assert (self);
    return self->ipaddress;
}

void
zre_msg_set_ipaddress (zre_msg_t *self, char *format, ...)
{
    //  Format into newly allocated string
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    free (self->ipaddress);
    self->ipaddress = (char *) malloc (STRING_MAX + 1);
    assert (self->ipaddress);
    vsnprintf (self->ipaddress, STRING_MAX, format, argptr);
    va_end (argptr);
}


//  --------------------------------------------------------------------------
//  Get/set the mailbox field

uint16_t
zre_msg_mailbox (zre_msg_t *self)
{
    assert (self);
    return self->mailbox;
}

void
zre_msg_set_mailbox (zre_msg_t *self, uint16_t mailbox)
{
    assert (self);
    self->mailbox = mailbox;
}


//  --------------------------------------------------------------------------
//  Get/set the groups field

zlist_t *
zre_msg_groups (zre_msg_t *self)
{
    assert (self);
    return self->groups;
}

//  Greedy function, takes ownership of groups; if you don't want that
//  then use zlist_dup() to pass a copy of groups

void
zre_msg_set_groups (zre_msg_t *self, zlist_t *groups)
{
    assert (self);
    zlist_destroy (&self->groups);
    self->groups = groups;
}

//  --------------------------------------------------------------------------
//  Iterate through the groups field, and append a groups value

char *
zre_msg_groups_first (zre_msg_t *self)
{
    assert (self);
    if (self->groups)
        return (char *) (zlist_first (self->groups));
    else
        return NULL;
}

char *
zre_msg_groups_next (zre_msg_t *self)
{
    assert (self);
    if (self->groups)
        return (char *) (zlist_next (self->groups));
    else
        return NULL;
}

void
zre_msg_groups_append (zre_msg_t *self, char *format, ...)
{
    //  Format into newly allocated string
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    char *string = (char *) malloc (STRING_MAX + 1);
    assert (string);
    vsnprintf (string, STRING_MAX, format, argptr);
    va_end (argptr);
    
    //  Attach string to list
    if (!self->groups) {
        self->groups = zlist_new ();
        zlist_autofree (self->groups);
    }
    zlist_append (self->groups, string);
    free (string);
}

size_t
zre_msg_groups_size (zre_msg_t *self)
{
    return zlist_size (self->groups);
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
//  Get/set the headers field

zhash_t *
zre_msg_headers (zre_msg_t *self)
{
    assert (self);
    return self->headers;
}

//  Greedy function, takes ownership of headers; if you don't want that
//  then use zhash_dup() to pass a copy of headers

void
zre_msg_set_headers (zre_msg_t *self, zhash_t *headers)
{
    assert (self);
    zhash_destroy (&self->headers);
    self->headers = headers;
}

//  --------------------------------------------------------------------------
//  Get/set a value in the headers dictionary

char *
zre_msg_headers_string (zre_msg_t *self, char *key, char *default_value)
{
    assert (self);
    char *value = NULL;
    if (self->headers)
        value = (char *) (zhash_lookup (self->headers, key));
    if (!value)
        value = default_value;

    return value;
}

uint64_t
zre_msg_headers_number (zre_msg_t *self, char *key, uint64_t default_value)
{
    assert (self);
    uint64_t value = default_value;
    char *string = NULL;
    if (self->headers)
        string = (char *) (zhash_lookup (self->headers, key));
    if (string)
        value = atol (string);

    return value;
}

void
zre_msg_headers_insert (zre_msg_t *self, char *key, char *format, ...)
{
    //  Format string into buffer
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    char *string = (char *) malloc (STRING_MAX + 1);
    assert (string);
    vsnprintf (string, STRING_MAX, format, argptr);
    va_end (argptr);

    //  Store string in hash table
    if (!self->headers) {
        self->headers = zhash_new ();
        zhash_autofree (self->headers);
    }
    zhash_update (self->headers, key, string);
    free (string);
}

size_t
zre_msg_headers_size (zre_msg_t *self)
{
    return zhash_size (self->headers);
}


//  --------------------------------------------------------------------------
//  Get/set the content field

zframe_t *
zre_msg_content (zre_msg_t *self)
{
    assert (self);
    return self->content;
}

//  Takes ownership of supplied frame
void
zre_msg_set_content (zre_msg_t *self, zframe_t *frame)
{
    assert (self);
    if (self->content)
        zframe_destroy (&self->content);
    self->content = frame;
}

//  --------------------------------------------------------------------------
//  Get/set the group field

char *
zre_msg_group (zre_msg_t *self)
{
    assert (self);
    return self->group;
}

void
zre_msg_set_group (zre_msg_t *self, char *format, ...)
{
    //  Format into newly allocated string
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    free (self->group);
    self->group = (char *) malloc (STRING_MAX + 1);
    assert (self->group);
    vsnprintf (self->group, STRING_MAX, format, argptr);
    va_end (argptr);
}



//  --------------------------------------------------------------------------
//  Selftest

int
zre_msg_test (bool verbose)
{
    printf (" * zre_msg: ");

    //  @selftest
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

    self = zre_msg_new (ZRE_MSG_HELLO);
    zre_msg_set_sequence (self, 123);
    zre_msg_set_ipaddress (self, "Life is short but Now lasts for ever");
    zre_msg_set_mailbox (self, 123);
    zre_msg_groups_append (self, "Name: %s", "Brutus");
    zre_msg_groups_append (self, "Age: %d", 43);
    zre_msg_set_status (self, 123);
    zre_msg_headers_insert (self, "Name", "Brutus");
    zre_msg_headers_insert (self, "Age", "%d", 43);
    zre_msg_send (&self, output);
    
    self = zre_msg_recv (input);
    assert (self);
    assert (zre_msg_sequence (self) == 123);
    assert (streq (zre_msg_ipaddress (self), "Life is short but Now lasts for ever"));
    assert (zre_msg_mailbox (self) == 123);
    assert (zre_msg_groups_size (self) == 2);
    assert (streq (zre_msg_groups_first (self), "Name: Brutus"));
    assert (streq (zre_msg_groups_next (self), "Age: 43"));
    assert (zre_msg_status (self) == 123);
    assert (zre_msg_headers_size (self) == 2);
    assert (streq (zre_msg_headers_string (self, "Name", "?"), "Brutus"));
    assert (zre_msg_headers_number (self, "Age", 0) == 43);
    zre_msg_destroy (&self);

    self = zre_msg_new (ZRE_MSG_WHISPER);
    zre_msg_set_sequence (self, 123);
    zre_msg_set_content (self, zframe_new ("Captcha Diem", 12));
    zre_msg_send (&self, output);
    
    self = zre_msg_recv (input);
    assert (self);
    assert (zre_msg_sequence (self) == 123);
    assert (zframe_streq (zre_msg_content (self), "Captcha Diem"));
    zre_msg_destroy (&self);

    self = zre_msg_new (ZRE_MSG_SHOUT);
    zre_msg_set_sequence (self, 123);
    zre_msg_set_group (self, "Life is short but Now lasts for ever");
    zre_msg_set_content (self, zframe_new ("Captcha Diem", 12));
    zre_msg_send (&self, output);
    
    self = zre_msg_recv (input);
    assert (self);
    assert (zre_msg_sequence (self) == 123);
    assert (streq (zre_msg_group (self), "Life is short but Now lasts for ever"));
    assert (zframe_streq (zre_msg_content (self), "Captcha Diem"));
    zre_msg_destroy (&self);

    self = zre_msg_new (ZRE_MSG_JOIN);
    zre_msg_set_sequence (self, 123);
    zre_msg_set_group (self, "Life is short but Now lasts for ever");
    zre_msg_set_status (self, 123);
    zre_msg_send (&self, output);
    
    self = zre_msg_recv (input);
    assert (self);
    assert (zre_msg_sequence (self) == 123);
    assert (streq (zre_msg_group (self), "Life is short but Now lasts for ever"));
    assert (zre_msg_status (self) == 123);
    zre_msg_destroy (&self);

    self = zre_msg_new (ZRE_MSG_LEAVE);
    zre_msg_set_sequence (self, 123);
    zre_msg_set_group (self, "Life is short but Now lasts for ever");
    zre_msg_set_status (self, 123);
    zre_msg_send (&self, output);
    
    self = zre_msg_recv (input);
    assert (self);
    assert (zre_msg_sequence (self) == 123);
    assert (streq (zre_msg_group (self), "Life is short but Now lasts for ever"));
    assert (zre_msg_status (self) == 123);
    zre_msg_destroy (&self);

    self = zre_msg_new (ZRE_MSG_PING);
    zre_msg_set_sequence (self, 123);
    zre_msg_send (&self, output);
    
    self = zre_msg_recv (input);
    assert (self);
    assert (zre_msg_sequence (self) == 123);
    zre_msg_destroy (&self);

    self = zre_msg_new (ZRE_MSG_PING_OK);
    zre_msg_set_sequence (self, 123);
    zre_msg_send (&self, output);
    
    self = zre_msg_recv (input);
    assert (self);
    assert (zre_msg_sequence (self) == 123);
    zre_msg_destroy (&self);

    zctx_destroy (&ctx);
    //  @end

    printf ("OK\n");
    return 0;
}
