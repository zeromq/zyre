/*  =========================================================================
    zyre_msg - work with zre messages
    
    Generated codec header for zyre_msg
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

#ifndef __ZYRE_MSG_H_INCLUDED__
#define __ZYRE_MSG_H_INCLUDED__

/*  These are the zyre_msg messages
    HELLO - Greet a peer so it can connect back to us
        sequence      number 2
        ipaddress     string
        mailbox       number 2
        groups        strings
        status        number 1
        headers       dictionary
    WHISPER - Send a message to a peer
        sequence      number 2
        content       frame
    SHOUT - Send a message to a group
        sequence      number 2
        group         string
        content       frame
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

#define ZYRE_MSG_VERSION                    1

#define ZYRE_MSG_HELLO                      1
#define ZYRE_MSG_WHISPER                    2
#define ZYRE_MSG_SHOUT                      3
#define ZYRE_MSG_JOIN                       4
#define ZYRE_MSG_LEAVE                      5
#define ZYRE_MSG_PING                       6
#define ZYRE_MSG_PING_OK                    7

#ifdef __cplusplus
extern "C" {
#endif

//  Opaque class structure
typedef struct _zyre_msg_t zyre_msg_t;

//  @interface
//  Create a new zyre_msg
zyre_msg_t *
    zyre_msg_new (int id);

//  Destroy the zyre_msg
void
    zyre_msg_destroy (zyre_msg_t **self_p);

//  Receive and parse a zyre_msg from the input
zyre_msg_t *
    zyre_msg_recv (void *input);

//  Send the zyre_msg to the output, and destroy it
int
    zyre_msg_send (zyre_msg_t **self_p, void *output);

//  Send the HELLO to the output in one step
int
    zyre_msg_send_hello (void *output,
        uint16_t sequence,
        char *ipaddress,
        uint16_t mailbox,
        zlist_t *groups,
        byte status,
        zhash_t *headers);
    
//  Send the WHISPER to the output in one step
int
    zyre_msg_send_whisper (void *output,
        uint16_t sequence,
        zframe_t *content);
    
//  Send the SHOUT to the output in one step
int
    zyre_msg_send_shout (void *output,
        uint16_t sequence,
        char *group,
        zframe_t *content);
    
//  Send the JOIN to the output in one step
int
    zyre_msg_send_join (void *output,
        uint16_t sequence,
        char *group,
        byte status);
    
//  Send the LEAVE to the output in one step
int
    zyre_msg_send_leave (void *output,
        uint16_t sequence,
        char *group,
        byte status);
    
//  Send the PING to the output in one step
int
    zyre_msg_send_ping (void *output,
        uint16_t sequence);
    
//  Send the PING_OK to the output in one step
int
    zyre_msg_send_ping_ok (void *output,
        uint16_t sequence);
    
//  Duplicate the zyre_msg message
zyre_msg_t *
    zyre_msg_dup (zyre_msg_t *self);

//  Print contents of message to stdout
void
    zyre_msg_dump (zyre_msg_t *self);

//  Get/set the message address
zframe_t *
    zyre_msg_address (zyre_msg_t *self);
void
    zyre_msg_set_address (zyre_msg_t *self, zframe_t *address);

//  Get the zyre_msg id and printable command
int
    zyre_msg_id (zyre_msg_t *self);
void
    zyre_msg_set_id (zyre_msg_t *self, int id);
char *
    zyre_msg_command (zyre_msg_t *self);

//  Get/set the sequence field
uint16_t
    zyre_msg_sequence (zyre_msg_t *self);
void
    zyre_msg_set_sequence (zyre_msg_t *self, uint16_t sequence);

//  Get/set the ipaddress field
char *
    zyre_msg_ipaddress (zyre_msg_t *self);
void
    zyre_msg_set_ipaddress (zyre_msg_t *self, char *format, ...);

//  Get/set the mailbox field
uint16_t
    zyre_msg_mailbox (zyre_msg_t *self);
void
    zyre_msg_set_mailbox (zyre_msg_t *self, uint16_t mailbox);

//  Get/set the groups field
zlist_t *
    zyre_msg_groups (zyre_msg_t *self);
void
    zyre_msg_set_groups (zyre_msg_t *self, zlist_t *groups);

//  Iterate through the groups field, and append a groups value
char *
    zyre_msg_groups_first (zyre_msg_t *self);
char *
    zyre_msg_groups_next (zyre_msg_t *self);
void
    zyre_msg_groups_append (zyre_msg_t *self, char *format, ...);
size_t
    zyre_msg_groups_size (zyre_msg_t *self);

//  Get/set the status field
byte
    zyre_msg_status (zyre_msg_t *self);
void
    zyre_msg_set_status (zyre_msg_t *self, byte status);

//  Get/set the headers field
zhash_t *
    zyre_msg_headers (zyre_msg_t *self);
void
    zyre_msg_set_headers (zyre_msg_t *self, zhash_t *headers);
    
//  Get/set a value in the headers dictionary
char *
    zyre_msg_headers_string (zyre_msg_t *self, char *key, char *default_value);
uint64_t
    zyre_msg_headers_number (zyre_msg_t *self, char *key, uint64_t default_value);
void
    zyre_msg_headers_insert (zyre_msg_t *self, char *key, char *format, ...);
size_t
    zyre_msg_headers_size (zyre_msg_t *self);

//  Get/set the content field
zframe_t *
    zyre_msg_content (zyre_msg_t *self);
void
    zyre_msg_set_content (zyre_msg_t *self, zframe_t *frame);

//  Get/set the group field
char *
    zyre_msg_group (zyre_msg_t *self);
void
    zyre_msg_set_group (zyre_msg_t *self, char *format, ...);

//  Self test of this class
int
    zyre_msg_test (bool verbose);
//  @end

#ifdef __cplusplus
}
#endif

#endif
