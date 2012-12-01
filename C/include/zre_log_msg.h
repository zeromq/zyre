/*  =========================================================================
    zre_log_msg.h
    
    Generated codec header for zre_log_msg
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

#ifndef __ZRE_LOG_MSG_H_INCLUDED__
#define __ZRE_LOG_MSG_H_INCLUDED__

/*  These are the zre_log_msg messages
    LOG - Log an event
        level         number 1
        event         number 1
        node          number 2
        peer          number 2
        time          number 8
        data          string
*/

#define ZRE_LOG_MSG_VERSION                 1
#define ZRE_LOG_MSG_LEVEL_ERROR             1
#define ZRE_LOG_MSG_LEVEL_WARNING           2
#define ZRE_LOG_MSG_LEVEL_INFO              3
#define ZRE_LOG_MSG_EVENT_JOIN              1
#define ZRE_LOG_MSG_EVENT_LEAVE             2
#define ZRE_LOG_MSG_EVENT_ENTER             3
#define ZRE_LOG_MSG_EVENT_EXIT              4

#define ZRE_LOG_MSG_LOG                     1

#ifdef __cplusplus
extern "C" {
#endif

//  Opaque class structure
typedef struct _zre_log_msg_t zre_log_msg_t;

//  Create a new zre_log_msg
zre_log_msg_t *
    zre_log_msg_new (int id);

//  Destroy the zre_log_msg
void
    zre_log_msg_destroy (zre_log_msg_t **self_p);

//  Receive and parse a zre_log_msg from the input
zre_log_msg_t *
    zre_log_msg_recv (void *input);

//  Send the zre_log_msg to the output, and destroy it
int
    zre_log_msg_send (zre_log_msg_t **self_p, void *output);

//  Send the LOG to the output in one step
int
    zre_log_msg_send_log (void *output,
        byte level,
        byte event,
        uint16_t node,
        uint16_t peer,
        uint64_t time,
        char *data);
    
//  Duplicate the zre_log_msg message
zre_log_msg_t *
    zre_log_msg_dup (zre_log_msg_t *self);

//  Print contents of message to stdout
void
    zre_log_msg_dump (zre_log_msg_t *self);

//  Get/set the message address
zframe_t *
    zre_log_msg_address (zre_log_msg_t *self);
void
    zre_log_msg_address_set (zre_log_msg_t *self, zframe_t *address);

//  Get the zre_log_msg id and printable command
int
    zre_log_msg_id (zre_log_msg_t *self);
void
    zre_log_msg_id_set (zre_log_msg_t *self, int id);
char *
    zre_log_msg_command (zre_log_msg_t *self);

//  Get/set the level field
byte
    zre_log_msg_level (zre_log_msg_t *self);
void
    zre_log_msg_level_set (zre_log_msg_t *self, byte level);

//  Get/set the event field
byte
    zre_log_msg_event (zre_log_msg_t *self);
void
    zre_log_msg_event_set (zre_log_msg_t *self, byte event);

//  Get/set the node field
uint16_t
    zre_log_msg_node (zre_log_msg_t *self);
void
    zre_log_msg_node_set (zre_log_msg_t *self, uint16_t node);

//  Get/set the peer field
uint16_t
    zre_log_msg_peer (zre_log_msg_t *self);
void
    zre_log_msg_peer_set (zre_log_msg_t *self, uint16_t peer);

//  Get/set the time field
uint64_t
    zre_log_msg_time (zre_log_msg_t *self);
void
    zre_log_msg_time_set (zre_log_msg_t *self, uint64_t time);

//  Get/set the data field
char *
    zre_log_msg_data (zre_log_msg_t *self);
void
    zre_log_msg_data_set (zre_log_msg_t *self, char *format, ...);

//  Self test of this class
int
    zre_log_msg_test (bool verbose);
    
#ifdef __cplusplus
}
#endif

#endif
