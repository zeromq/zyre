/*  =========================================================================
    zyre_msg.h - Parsing Zyre messages

    -------------------------------------------------------------------------
    Copyright (c) 1991-2013 iMatix Corporation <www.imatix.com>
    Copyright other contributors as noted in the AUTHORS file.

    This file is part of Zyre, an open-source framework for proximity-based
    peer-to-peer applications -- See http://zyre.org.

    This is free software; you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 3 of the License, or (at
    your option) any later version.

    This software is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this program. If not, see
    <http://www.gnu.org/licenses/>.
    =========================================================================
*/

#ifndef __ZYRE_MSG_H_INCLUDED__
#define __ZYRE_MSG_H_INCLUDED__

#define ZYRE_MSG_ENTER 0x1
#define ZYRE_MSG_JOIN 0x2
#define ZYRE_MSG_LEAVE 0x3
#define ZYRE_MSG_EXIT 0x4
#define ZYRE_MSG_WHISPER 0x5
#define ZYRE_MSG_SHOUT 0x6

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _zyre_msg_t zyre_msg_t;

//  Destructor, destroys a Zyre message.
CZMQ_EXPORT void
    zyre_msg_destroy (zyre_msg_t **self_p);

// Wrapper for zyre_recv
CZMQ_EXPORT zyre_msg_t *
    zyre_msg_recv (zyre_t *self);

//  Gets the message type 
CZMQ_EXPORT int
    zyre_msg_cmd (zyre_msg_t *self);

//  Gets the peer that did send the message
CZMQ_EXPORT char *
    zyre_msg_peerid (zyre_msg_t *self);

// Returns all headers or NULL
CZMQ_EXPORT zhash_t *
    zyre_msg_headers (zyre_msg_t *self);

//  Returns value of header attribute name from the message headers 
//  obtained by ENTER. Return NULL if no value was found.
CZMQ_EXPORT char *
    zyre_msg_get_header (zyre_msg_t *self, char *name);

//  Gets the group name that shout send
CZMQ_EXPORT char *
    zyre_msg_group (zyre_msg_t *self);

//  Gets the actual message data
CZMQ_EXPORT zmsg_t *
    zyre_msg_data (zyre_msg_t *self);

// Self test of this class
CZMQ_EXPORT void
    zyre_msg_test (bool verbose);

#ifdef __cplusplus
}
#endif

#endif
