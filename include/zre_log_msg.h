/*  =========================================================================
    zre_log_msg - work with ZRE logging messages
    
    Codec header for zre_log_msg.

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

#ifndef __ZRE_LOG_MSG_H_INCLUDED__
#define __ZRE_LOG_MSG_H_INCLUDED__

/*  These are the zre_log_msg messages:

    LOG - Log an event
        level               number 1    
        event               number 1    
        node                number 2    
        peer                number 2    
        time                number 8    
        data                string      
*/

#define ZRE_LOG_MSG_VERSION                 1
#define ZRE_LOG_MSG_LEVEL_ERROR             1
#define ZRE_LOG_MSG_LEVEL_WARNING           2
#define ZRE_LOG_MSG_LEVEL_INFO              3
#define ZRE_LOG_MSG_EVENT_JOIN              1
#define ZRE_LOG_MSG_EVENT_LEAVE             2
#define ZRE_LOG_MSG_EVENT_ENTER             3
#define ZRE_LOG_MSG_EVENT_EXIT              4
#define ZRE_LOG_MSG_EVENT_SEND              5
#define ZRE_LOG_MSG_EVENT_RECV              6

#define ZRE_LOG_MSG_LOG                     1

#ifdef __cplusplus
extern "C" {
#endif

//  Opaque class structure
typedef struct _zre_log_msg_t zre_log_msg_t;

//  @interface
//  Create a new zre_log_msg
CZMQ_EXPORT zre_log_msg_t *
    zre_log_msg_new (int id);

//  Destroy the zre_log_msg
CZMQ_EXPORT void
    zre_log_msg_destroy (zre_log_msg_t **self_p);

//  Parse a zre_log_msg from zmsg_t. Returns a new object, or NULL if
//  the message could not be parsed, or was NULL. If the socket type is
//  ZMQ_ROUTER, then parses the first frame as a routing_id. Destroys msg
//  and nullifies the msg refernce.
CZMQ_EXPORT zre_log_msg_t *
    zre_log_msg_decode (zmsg_t **msg_p, int socket_type);

//  Encode zre_log_msg into zmsg and destroy it. Returns a newly created
//  object or NULL if error. Use when not in control of sending the message.
//  If the socket_type is ZMQ_ROUTER, then stores the routing_id as the
//  first frame of the resulting message.
CZMQ_EXPORT zmsg_t *
    zre_log_msg_encode (zre_log_msg_t *self, int socket_type);

//  Receive and parse a zre_log_msg from the socket. Returns new object, 
//  or NULL if error. Will block if there's no message waiting.
CZMQ_EXPORT zre_log_msg_t *
    zre_log_msg_recv (void *input);

//  Receive and parse a zre_log_msg from the socket. Returns new object, 
//  or NULL either if there was no input waiting, or the recv was interrupted.
CZMQ_EXPORT zre_log_msg_t *
    zre_log_msg_recv_nowait (void *input);

//  Send the zre_log_msg to the output, and destroy it
CZMQ_EXPORT int
    zre_log_msg_send (zre_log_msg_t **self_p, void *output);

//  Send the zre_log_msg to the output, and do not destroy it
CZMQ_EXPORT int
    zre_log_msg_send_again (zre_log_msg_t *self, void *output);

//  Send the LOG to the output in one step
CZMQ_EXPORT int
    zre_log_msg_send_log (void *output,
        byte level,
        byte event,
        uint16_t node,
        uint16_t peer,
        uint64_t time,
        const char *data);
    
//  Duplicate the zre_log_msg message
CZMQ_EXPORT zre_log_msg_t *
    zre_log_msg_dup (zre_log_msg_t *self);

//  Print contents of message to stdout
CZMQ_EXPORT void
    zre_log_msg_dump (zre_log_msg_t *self);

//  Get/set the message routing id
CZMQ_EXPORT zframe_t *
    zre_log_msg_routing_id (zre_log_msg_t *self);
CZMQ_EXPORT void
    zre_log_msg_set_routing_id (zre_log_msg_t *self, zframe_t *routing_id);

//  Get the zre_log_msg id and printable command
CZMQ_EXPORT int
    zre_log_msg_id (zre_log_msg_t *self);
CZMQ_EXPORT void
    zre_log_msg_set_id (zre_log_msg_t *self, int id);
CZMQ_EXPORT const char *
    zre_log_msg_command (zre_log_msg_t *self);

//  Get/set the level field
CZMQ_EXPORT byte
    zre_log_msg_level (zre_log_msg_t *self);
CZMQ_EXPORT void
    zre_log_msg_set_level (zre_log_msg_t *self, byte level);

//  Get/set the event field
CZMQ_EXPORT byte
    zre_log_msg_event (zre_log_msg_t *self);
CZMQ_EXPORT void
    zre_log_msg_set_event (zre_log_msg_t *self, byte event);

//  Get/set the node field
CZMQ_EXPORT uint16_t
    zre_log_msg_node (zre_log_msg_t *self);
CZMQ_EXPORT void
    zre_log_msg_set_node (zre_log_msg_t *self, uint16_t node);

//  Get/set the peer field
CZMQ_EXPORT uint16_t
    zre_log_msg_peer (zre_log_msg_t *self);
CZMQ_EXPORT void
    zre_log_msg_set_peer (zre_log_msg_t *self, uint16_t peer);

//  Get/set the time field
CZMQ_EXPORT uint64_t
    zre_log_msg_time (zre_log_msg_t *self);
CZMQ_EXPORT void
    zre_log_msg_set_time (zre_log_msg_t *self, uint64_t time);

//  Get/set the data field
CZMQ_EXPORT const char *
    zre_log_msg_data (zre_log_msg_t *self);
CZMQ_EXPORT void
    zre_log_msg_set_data (zre_log_msg_t *self, const char *format, ...);

//  Self test of this class
CZMQ_EXPORT int
    zre_log_msg_test (bool verbose);
//  @end

#ifdef __cplusplus
}
#endif

#endif
