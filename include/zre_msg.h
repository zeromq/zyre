/*  =========================================================================
    zre_msg - work with ZRE messages
    
    Codec header for zre_msg.

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

     * The XML model used for this code generation: zre_msg.xml, or
     * The code generation script that built this file: zproto_codec_c_v1
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

#ifndef __ZRE_MSG_H_INCLUDED__
#define __ZRE_MSG_H_INCLUDED__

/*  These are the zre_msg messages:

    HELLO - Greet a peer so it can connect back to us
        version             number 1    Version number (2)
        sequence            number 2    Cyclic sequence number
        endpoint            string      Sender connect endpoint
        groups              strings     List of groups sender is in
        status              number 1    Sender groups status value
        name                string      Sender public name
        headers             hash        Sender header properties

    WHISPER - Send a multi-part message to a peer
        version             number 1    Version number (2)
        sequence            number 2    Cyclic sequence number
        content             msg         Wrapped message content

    SHOUT - Send a multi-part message to a group
        version             number 1    Version number (2)
        sequence            number 2    Cyclic sequence number
        group               string      Group to send to
        content             msg         Wrapped message content

    JOIN - Join a group
        version             number 1    Version number (2)
        sequence            number 2    Cyclic sequence number
        group               string      Name of group
        status              number 1    Sender groups status value

    LEAVE - Leave a group
        version             number 1    Version number (2)
        sequence            number 2    Cyclic sequence number
        group               string      Name of group
        status              number 1    Sender groups status value

    PING - Ping a peer that has gone silent
        version             number 1    Version number (2)
        sequence            number 2    Cyclic sequence number

    PING_OK - Reply to a peer's ping
        version             number 1    Version number (2)
        sequence            number 2    Cyclic sequence number
*/


#define ZRE_MSG_HELLO                       1
#define ZRE_MSG_WHISPER                     2
#define ZRE_MSG_SHOUT                       3
#define ZRE_MSG_JOIN                        4
#define ZRE_MSG_LEAVE                       5
#define ZRE_MSG_PING                        6
#define ZRE_MSG_PING_OK                     7

#include <czmq.h>

#ifdef __cplusplus
extern "C" {
#endif

//  Opaque class structure
#ifndef ZRE_MSG_T_DEFINED
typedef struct _zre_msg_t zre_msg_t;
#define ZRE_MSG_T_DEFINED
#endif

//  @interface
//  Create a new zre_msg
ZYRE_EXPORT zre_msg_t *
    zre_msg_new (int id);

//  Destroy the zre_msg
ZYRE_EXPORT void
    zre_msg_destroy (zre_msg_t **self_p);

//  Parse a zre_msg from zmsg_t. Returns a new object, or NULL if
//  the message could not be parsed, or was NULL. Destroys msg and 
//  nullifies the msg reference.
ZYRE_EXPORT zre_msg_t *
    zre_msg_decode (zmsg_t **msg_p);

//  Encode zre_msg into zmsg and destroy it. Returns a newly created
//  object or NULL if error. Use when not in control of sending the message.
ZYRE_EXPORT zmsg_t *
    zre_msg_encode (zre_msg_t **self_p);

//  Receive and parse a zre_msg from the socket. Returns new object, 
//  or NULL if error. Will block if there's no message waiting.
ZYRE_EXPORT zre_msg_t *
    zre_msg_recv (void *input);

//  Receive and parse a zre_msg from the socket. Returns new object, 
//  or NULL either if there was no input waiting, or the recv was interrupted.
ZYRE_EXPORT zre_msg_t *
    zre_msg_recv_nowait (void *input);

//  Send the zre_msg to the output, and destroy it
ZYRE_EXPORT int
    zre_msg_send (zre_msg_t **self_p, void *output);

//  Send the zre_msg to the output, and do not destroy it
ZYRE_EXPORT int
    zre_msg_send_again (zre_msg_t *self, void *output);

//  Encode the HELLO 
ZYRE_EXPORT zmsg_t *
    zre_msg_encode_hello (
        uint16_t sequence,
        const char *endpoint,
        zlist_t *groups,
        byte status,
        const char *name,
        zhash_t *headers);

//  Encode the WHISPER 
ZYRE_EXPORT zmsg_t *
    zre_msg_encode_whisper (
        uint16_t sequence,
        zmsg_t *content);

//  Encode the SHOUT 
ZYRE_EXPORT zmsg_t *
    zre_msg_encode_shout (
        uint16_t sequence,
        const char *group,
        zmsg_t *content);

//  Encode the JOIN 
ZYRE_EXPORT zmsg_t *
    zre_msg_encode_join (
        uint16_t sequence,
        const char *group,
        byte status);

//  Encode the LEAVE 
ZYRE_EXPORT zmsg_t *
    zre_msg_encode_leave (
        uint16_t sequence,
        const char *group,
        byte status);

//  Encode the PING 
ZYRE_EXPORT zmsg_t *
    zre_msg_encode_ping (
        uint16_t sequence);

//  Encode the PING_OK 
ZYRE_EXPORT zmsg_t *
    zre_msg_encode_ping_ok (
        uint16_t sequence);


//  Send the HELLO to the output in one step
//  WARNING, this call will fail if output is of type ZMQ_ROUTER.
ZYRE_EXPORT int
    zre_msg_send_hello (void *output,
        uint16_t sequence,
        const char *endpoint,
        zlist_t *groups,
        byte status,
        const char *name,
        zhash_t *headers);
    
//  Send the WHISPER to the output in one step
//  WARNING, this call will fail if output is of type ZMQ_ROUTER.
ZYRE_EXPORT int
    zre_msg_send_whisper (void *output,
        uint16_t sequence,
        zmsg_t *content);
    
//  Send the SHOUT to the output in one step
//  WARNING, this call will fail if output is of type ZMQ_ROUTER.
ZYRE_EXPORT int
    zre_msg_send_shout (void *output,
        uint16_t sequence,
        const char *group,
        zmsg_t *content);
    
//  Send the JOIN to the output in one step
//  WARNING, this call will fail if output is of type ZMQ_ROUTER.
ZYRE_EXPORT int
    zre_msg_send_join (void *output,
        uint16_t sequence,
        const char *group,
        byte status);
    
//  Send the LEAVE to the output in one step
//  WARNING, this call will fail if output is of type ZMQ_ROUTER.
ZYRE_EXPORT int
    zre_msg_send_leave (void *output,
        uint16_t sequence,
        const char *group,
        byte status);
    
//  Send the PING to the output in one step
//  WARNING, this call will fail if output is of type ZMQ_ROUTER.
ZYRE_EXPORT int
    zre_msg_send_ping (void *output,
        uint16_t sequence);
    
//  Send the PING_OK to the output in one step
//  WARNING, this call will fail if output is of type ZMQ_ROUTER.
ZYRE_EXPORT int
    zre_msg_send_ping_ok (void *output,
        uint16_t sequence);
    
//  Duplicate the zre_msg message
ZYRE_EXPORT zre_msg_t *
    zre_msg_dup (zre_msg_t *self);

//  Print contents of message to stdout
ZYRE_EXPORT void
    zre_msg_print (zre_msg_t *self);

//  Get/set the message routing id
ZYRE_EXPORT zframe_t *
    zre_msg_routing_id (zre_msg_t *self);
ZYRE_EXPORT void
    zre_msg_set_routing_id (zre_msg_t *self, zframe_t *routing_id);

//  Get the zre_msg id and printable command
ZYRE_EXPORT int
    zre_msg_id (zre_msg_t *self);
ZYRE_EXPORT void
    zre_msg_set_id (zre_msg_t *self, int id);
ZYRE_EXPORT const char *
    zre_msg_command (zre_msg_t *self);

//  Get/set the sequence field
ZYRE_EXPORT uint16_t
    zre_msg_sequence (zre_msg_t *self);
ZYRE_EXPORT void
    zre_msg_set_sequence (zre_msg_t *self, uint16_t sequence);

//  Get/set the endpoint field
ZYRE_EXPORT const char *
    zre_msg_endpoint (zre_msg_t *self);
ZYRE_EXPORT void
    zre_msg_set_endpoint (zre_msg_t *self, const char *format, ...);

//  Get/set the groups field
ZYRE_EXPORT zlist_t *
    zre_msg_groups (zre_msg_t *self);
//  Get the groups field and transfer ownership to caller
ZYRE_EXPORT zlist_t *
    zre_msg_get_groups (zre_msg_t *self);
//  Set the groups field, transferring ownership from caller
ZYRE_EXPORT void
    zre_msg_set_groups (zre_msg_t *self, zlist_t **groups_p);

//  Iterate through the groups field, and append a groups value
ZYRE_EXPORT const char *
    zre_msg_groups_first (zre_msg_t *self);
ZYRE_EXPORT const char *
    zre_msg_groups_next (zre_msg_t *self);
ZYRE_EXPORT void
    zre_msg_groups_append (zre_msg_t *self, const char *format, ...);
ZYRE_EXPORT size_t
    zre_msg_groups_size (zre_msg_t *self);

//  Get/set the status field
ZYRE_EXPORT byte
    zre_msg_status (zre_msg_t *self);
ZYRE_EXPORT void
    zre_msg_set_status (zre_msg_t *self, byte status);

//  Get/set the name field
ZYRE_EXPORT const char *
    zre_msg_name (zre_msg_t *self);
ZYRE_EXPORT void
    zre_msg_set_name (zre_msg_t *self, const char *format, ...);

//  Get/set the headers field
ZYRE_EXPORT zhash_t *
    zre_msg_headers (zre_msg_t *self);
//  Get the headers field and transfer ownership to caller
ZYRE_EXPORT zhash_t *
    zre_msg_get_headers (zre_msg_t *self);
//  Set the headers field, transferring ownership from caller
ZYRE_EXPORT void
    zre_msg_set_headers (zre_msg_t *self, zhash_t **headers_p);
    
//  Get/set a value in the headers dictionary
ZYRE_EXPORT const char *
    zre_msg_headers_string (zre_msg_t *self,
        const char *key, const char *default_value);
ZYRE_EXPORT uint64_t
    zre_msg_headers_number (zre_msg_t *self,
        const char *key, uint64_t default_value);
ZYRE_EXPORT void
    zre_msg_headers_insert (zre_msg_t *self,
        const char *key, const char *format, ...);
ZYRE_EXPORT size_t
    zre_msg_headers_size (zre_msg_t *self);

//  Get a copy of the content field
ZYRE_EXPORT zmsg_t *
    zre_msg_content (zre_msg_t *self);
//  Get the content field and transfer ownership to caller
ZYRE_EXPORT zmsg_t *
    zre_msg_get_content (zre_msg_t *self);
//  Set the content field, transferring ownership from caller
ZYRE_EXPORT void
    zre_msg_set_content (zre_msg_t *self, zmsg_t **msg_p);

//  Get/set the group field
ZYRE_EXPORT const char *
    zre_msg_group (zre_msg_t *self);
ZYRE_EXPORT void
    zre_msg_set_group (zre_msg_t *self, const char *format, ...);

//  Self test of this class
ZYRE_EXPORT int
    zre_msg_test (bool verbose);
//  @end

//  For backwards compatibility with old codecs
#define zre_msg_dump        zre_msg_print

#ifdef __cplusplus
}
#endif

#endif
