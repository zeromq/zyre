/*  =========================================================================
    zre_msg - work with zre messages
    
    Generated codec header for zre_msg
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

#ifndef __ZRE_MSG_H_INCLUDED__
#define __ZRE_MSG_H_INCLUDED__

/*  These are the zre_msg messages
    HELLO - Greet a peer so it can connect back to us
        sequence      number 2
        ipaddress     string
        mailbox       number 2
        groups        strings
        status        number 1
        headers       dictionary
    WHISPER - Send a multi-part message to a peer
        sequence      number 2
        content       msg
    SHOUT - Send a multi-part message to a group
        sequence      number 2
        group         string
        content       msg
    JOIN - Join a group
        sequence      number 2
        group         string
        status        number 1
    LEAVE - Leave a group
        sequence      number 2
        group         string
        status        number 1
    PING - Ping a peer that has gone silent
        sequence      number 2
    PING_OK - Reply to a peer's ping
        sequence      number 2
*/

#define ZRE_MSG_VERSION                     1

#define ZRE_MSG_HELLO                       1
#define ZRE_MSG_WHISPER                     2
#define ZRE_MSG_SHOUT                       3
#define ZRE_MSG_JOIN                        4
#define ZRE_MSG_LEAVE                       5
#define ZRE_MSG_PING                        6
#define ZRE_MSG_PING_OK                     7

#ifdef __cplusplus
extern "C" {
#endif

//  Opaque class structure
typedef struct _zre_msg_t zre_msg_t;

//  @interface
//  Create a new zre_msg
zre_msg_t *
    zre_msg_new (int id);

//  Destroy the zre_msg
void
    zre_msg_destroy (zre_msg_t **self_p);

//  Receive and parse a zre_msg from the input
zre_msg_t *
    zre_msg_recv (void *input);

//  Send the zre_msg to the output, and destroy it
int
    zre_msg_send (zre_msg_t **self_p, void *output);

//  Send the HELLO to the output in one step
int
    zre_msg_send_hello (void *output,
        uint16_t sequence,
        char *ipaddress,
        uint16_t mailbox,
        zlist_t *groups,
        byte status,
        zhash_t *headers);
    
//  Send the WHISPER to the output in one step
int
    zre_msg_send_whisper (void *output,
        uint16_t sequence,
        zmsg_t *content);
    
//  Send the SHOUT to the output in one step
int
    zre_msg_send_shout (void *output,
        uint16_t sequence,
        char *group,
        zmsg_t *content);
    
//  Send the JOIN to the output in one step
int
    zre_msg_send_join (void *output,
        uint16_t sequence,
        char *group,
        byte status);
    
//  Send the LEAVE to the output in one step
int
    zre_msg_send_leave (void *output,
        uint16_t sequence,
        char *group,
        byte status);
    
//  Send the PING to the output in one step
int
    zre_msg_send_ping (void *output,
        uint16_t sequence);
    
//  Send the PING_OK to the output in one step
int
    zre_msg_send_ping_ok (void *output,
        uint16_t sequence);
    
//  Duplicate the zre_msg message
zre_msg_t *
    zre_msg_dup (zre_msg_t *self);

//  Print contents of message to stdout
void
    zre_msg_dump (zre_msg_t *self);

//  Get/set the message address
zframe_t *
    zre_msg_address (zre_msg_t *self);
void
    zre_msg_set_address (zre_msg_t *self, zframe_t *address);

//  Get the zre_msg id and printable command
int
    zre_msg_id (zre_msg_t *self);
void
    zre_msg_set_id (zre_msg_t *self, int id);
char *
    zre_msg_command (zre_msg_t *self);

//  Get/set the sequence field
uint16_t
    zre_msg_sequence (zre_msg_t *self);
void
    zre_msg_set_sequence (zre_msg_t *self, uint16_t sequence);

//  Get/set the ipaddress field
char *
    zre_msg_ipaddress (zre_msg_t *self);
void
    zre_msg_set_ipaddress (zre_msg_t *self, char *format, ...);

//  Get/set the mailbox field
uint16_t
    zre_msg_mailbox (zre_msg_t *self);
void
    zre_msg_set_mailbox (zre_msg_t *self, uint16_t mailbox);

//  Get/set the groups field
zlist_t *
    zre_msg_groups (zre_msg_t *self);
void
    zre_msg_set_groups (zre_msg_t *self, zlist_t *groups);

//  Iterate through the groups field, and append a groups value
char *
    zre_msg_groups_first (zre_msg_t *self);
char *
    zre_msg_groups_next (zre_msg_t *self);
void
    zre_msg_groups_append (zre_msg_t *self, char *format, ...);
size_t
    zre_msg_groups_size (zre_msg_t *self);

//  Get/set the status field
byte
    zre_msg_status (zre_msg_t *self);
void
    zre_msg_set_status (zre_msg_t *self, byte status);

//  Get/set the headers field
zhash_t *
    zre_msg_headers (zre_msg_t *self);
void
    zre_msg_set_headers (zre_msg_t *self, zhash_t *headers);
    
//  Get/set a value in the headers dictionary
char *
    zre_msg_headers_string (zre_msg_t *self, char *key, char *default_value);
uint64_t
    zre_msg_headers_number (zre_msg_t *self, char *key, uint64_t default_value);
void
    zre_msg_headers_insert (zre_msg_t *self, char *key, char *format, ...);
size_t
    zre_msg_headers_size (zre_msg_t *self);

//  Get/set the content field
zmsg_t *
    zre_msg_content (zre_msg_t *self);
void
    zre_msg_set_content (zre_msg_t *self, zmsg_t *msg);

//  Get/set the group field
char *
    zre_msg_group (zre_msg_t *self);
void
    zre_msg_set_group (zre_msg_t *self, char *format, ...);

//  Self test of this class
int
    zre_msg_test (bool verbose);
//  @end

#ifdef __cplusplus
}
#endif

#endif
