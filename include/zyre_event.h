/*  =========================================================================
    zyre_event.h - Parsing Zyre messages

    -------------------------------------------------------------------------
    Copyright (c) 1991-2014 iMatix Corporation <www.imatix.com>
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

#ifndef __ZYRE_EVENT_H_INCLUDED__
#define __ZYRE_EVENT_H_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif

// @interface
typedef enum {
    ZYRE_EVENT_ENTER = 1,
    ZYRE_EVENT_JOIN = 2,
    ZYRE_EVENT_LEAVE = 3,
    ZYRE_EVENT_EXIT = 4,
    ZYRE_EVENT_WHISPER = 5,
    ZYRE_EVENT_SHOUT = 6
} zyre_event_type_t;


//  Constructor: receive an event from the zyre node, wraps zyre_recv.
//  The event may be a control message (ENTER, EXIT, JOIN, LEAVE) or
//  data (WHISPER, SHOUT).
ZYRE_EXPORT zyre_event_t *
    zyre_event_new (zyre_t *self);

//  Destructor; destroys an event instance
ZYRE_EXPORT void
    zyre_event_destroy (zyre_event_t **self_p);

//  Returns event type, which is a zyre_event_type_t
ZYRE_EXPORT zyre_event_type_t
    zyre_event_type (zyre_event_t *self);

//  Return the sending peer's id as a string
ZYRE_EXPORT char *
    zyre_event_sender (zyre_event_t *self);

//  Return the sending peer's public name as a string
ZYRE_EXPORT char *
    zyre_event_name (zyre_event_t *self);

//  Return the sending peer's ipaddress as a string
ZYRE_EXPORT char *
    zyre_event_address (zyre_event_t *self);

//  Returns the event headers, or NULL if there are none
ZYRE_EXPORT zhash_t *
    zyre_event_headers (zyre_event_t *self);

//  Returns value of a header from the message headers
//  obtained by ENTER. Return NULL if no value was found.
ZYRE_EXPORT char *
    zyre_event_header (zyre_event_t *self, char *name);

//  Returns the group name that a SHOUT event was sent to
ZYRE_EXPORT char *
    zyre_event_group (zyre_event_t *self);

//  Returns the incoming message payload (currently one frame)
ZYRE_EXPORT zmsg_t *
    zyre_event_msg (zyre_event_t *self);

// Self test of this class
ZYRE_EXPORT void
    zyre_event_test (bool verbose);
// @end

#ifdef __cplusplus
}
#endif

#endif
