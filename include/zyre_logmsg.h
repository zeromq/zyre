/*  =========================================================================
    zyre_logmsg - work with zre logging messages
    
    Generated codec header for zyre_logmsg
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

#ifndef __ZYRE_LOGMSG_H_INCLUDED__
#define __ZYRE_LOGMSG_H_INCLUDED__

/*  These are the zyre_logmsg messages
    LOG - Log an event
        level         number 1
        event         number 1
        node          number 2
        peer          number 2
        time          number 8
        data          string
*/

#define ZYRE_LOGMSG_VERSION                 1
#define ZYRE_LOGMSG_LEVEL_ERROR             1
#define ZYRE_LOGMSG_LEVEL_WARNING           2
#define ZYRE_LOGMSG_LEVEL_INFO              3
#define ZYRE_LOGMSG_EVENT_JOIN              1
#define ZYRE_LOGMSG_EVENT_LEAVE             2
#define ZYRE_LOGMSG_EVENT_ENTER             3
#define ZYRE_LOGMSG_EVENT_EXIT              4

#define ZYRE_LOGMSG_LOG                     1

#ifdef __cplusplus
extern "C" {
#endif

//  Opaque class structure
typedef struct _zyre_logmsg_t zyre_logmsg_t;

//  @interface
//  Create a new zyre_logmsg
zyre_logmsg_t *
    zyre_logmsg_new (int id);

//  Destroy the zyre_logmsg
void
    zyre_logmsg_destroy (zyre_logmsg_t **self_p);

//  Receive and parse a zyre_logmsg from the input
zyre_logmsg_t *
    zyre_logmsg_recv (void *input);

//  Send the zyre_logmsg to the output, and destroy it
int
    zyre_logmsg_send (zyre_logmsg_t **self_p, void *output);

//  Send the LOG to the output in one step
int
    zyre_logmsg_send_log (void *output,
        byte level,
        byte event,
        uint16_t node,
        uint16_t peer,
        uint64_t time,
        char *data);
    
//  Duplicate the zyre_logmsg message
zyre_logmsg_t *
    zyre_logmsg_dup (zyre_logmsg_t *self);

//  Print contents of message to stdout
void
    zyre_logmsg_dump (zyre_logmsg_t *self);

//  Get/set the message address
zframe_t *
    zyre_logmsg_address (zyre_logmsg_t *self);
void
    zyre_logmsg_set_address (zyre_logmsg_t *self, zframe_t *address);

//  Get the zyre_logmsg id and printable command
int
    zyre_logmsg_id (zyre_logmsg_t *self);
void
    zyre_logmsg_set_id (zyre_logmsg_t *self, int id);
char *
    zyre_logmsg_command (zyre_logmsg_t *self);

//  Get/set the level field
byte
    zyre_logmsg_level (zyre_logmsg_t *self);
void
    zyre_logmsg_set_level (zyre_logmsg_t *self, byte level);

//  Get/set the event field
byte
    zyre_logmsg_event (zyre_logmsg_t *self);
void
    zyre_logmsg_set_event (zyre_logmsg_t *self, byte event);

//  Get/set the node field
uint16_t
    zyre_logmsg_node (zyre_logmsg_t *self);
void
    zyre_logmsg_set_node (zyre_logmsg_t *self, uint16_t node);

//  Get/set the peer field
uint16_t
    zyre_logmsg_peer (zyre_logmsg_t *self);
void
    zyre_logmsg_set_peer (zyre_logmsg_t *self, uint16_t peer);

//  Get/set the time field
uint64_t
    zyre_logmsg_time (zyre_logmsg_t *self);
void
    zyre_logmsg_set_time (zyre_logmsg_t *self, uint64_t time);

//  Get/set the data field
char *
    zyre_logmsg_data (zyre_logmsg_t *self);
void
    zyre_logmsg_set_data (zyre_logmsg_t *self, char *format, ...);

//  Self test of this class
int
    zyre_logmsg_test (bool verbose);
//  @end

#ifdef __cplusplus
}
#endif

#endif
