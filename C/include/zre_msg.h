/*  =========================================================================
    zre_msg.h
    
    Generated codec header for zre_msg
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

#ifndef __ZRE_MSG_H_INCLUDED__
#define __ZRE_MSG_H_INCLUDED__

/*  These are the zre_msg messages

    HELLO - Greet a peer so it connect back to us.
        from          string 
        port          number 
        groups        strings 
        status        octet 

    WHISPER - Send a message to a peer.
        cookies       frame 

    SHOUT - Send a message to a group.
        group         string 
        cookies       frame 

    PING - Ping a peer that has gone silent.

    PING_OK - Reply to a peer's ping.

    JOIN - Join a group.
        group         string 
        status        octet 

    LEAVE - Leave a group.
        group         string 
        status        octet 
*/

#define ZRE_MSG_VERSION                     1

#define ZRE_MSG_HELLO                       1
#define ZRE_MSG_WHISPER                     2
#define ZRE_MSG_SHOUT                       3
#define ZRE_MSG_PING                        4
#define ZRE_MSG_PING_OK                     5
#define ZRE_MSG_JOIN                        6
#define ZRE_MSG_LEAVE                       7

#ifdef __cplusplus
extern "C" {
#endif

//  Opaque class structure
typedef struct _zre_msg_t zre_msg_t;

//  Create a new zre_msg
zre_msg_t *
    zre_msg_new (int id);

//  Destroy the zre_msg
void
    zre_msg_destroy (zre_msg_t **self_p);

//  Receive and parse a zre_msg from the socket
zre_msg_t *
    zre_msg_recv (void *socket);

//  Send the zre_msg to the socket, and destroy it
void
    zre_msg_send (zre_msg_t **self_p, void *socket);

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
    zre_msg_address_set (zre_msg_t *self, zframe_t *address);

//  Get the zre_msg id and printable command
int
    zre_msg_id (zre_msg_t *self);
void
    zre_msg_id_set (zre_msg_t *self, int id);
char *
    zre_msg_command (zre_msg_t *self);

//  Get/set the from field
char *
    zre_msg_from (zre_msg_t *self);
void
    zre_msg_from_set (zre_msg_t *self, char *format, ...);

//  Get/set the port field
int64_t
    zre_msg_port (zre_msg_t *self);
void
    zre_msg_port_set (zre_msg_t *self, int64_t port);

//  Get/set the groups field
zlist_t *
    zre_msg_groups (zre_msg_t *self);
void
    zre_msg_groups_set (zre_msg_t *self, zlist_t *groups);

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
    zre_msg_status_set (zre_msg_t *self, byte status);

//  Get/set the cookies field
zframe_t *
    zre_msg_cookies (zre_msg_t *self);
void
    zre_msg_cookies_set (zre_msg_t *self, zframe_t *frame);

//  Get/set the group field
char *
    zre_msg_group (zre_msg_t *self);
void
    zre_msg_group_set (zre_msg_t *self, char *format, ...);

//  Self test of this class
int
    zre_msg_test (bool verbose);
    
#ifdef __cplusplus
}
#endif

#endif
