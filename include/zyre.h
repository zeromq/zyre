/*  =========================================================================
    zyre.h - Zyre public API

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

#ifndef __ZYRE_H_INCLUDED__
#define __ZYRE_H_INCLUDED__

#define ZYRE_VERSION_MAJOR 1
#define ZYRE_VERSION_MINOR 1
#define ZYRE_VERSION_PATCH 0

#define ZYRE_MAKE_VERSION(major, minor, patch) \
    ((major) * 10000 + (minor) * 100 + (patch))
#define ZYRE_VERSION \
    ZYRE_MAKE_VERSION(ZYRE_VERSION_MAJOR, ZYRE_VERSION_MINOR, ZYRE_VERSION_PATCH)

#include <czmq.h>
#if CZMQ_VERSION < 20100
#   error "Zyre needs CZMQ/2.1.0 or later"
#endif

//  IANA-assigned port for ZYRE discovery protocol
#define ZRE_DISCOVERY_PORT  5670

//  The public API consists of the "zyre_t" class

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _zyre_t zyre_t;

//  @interface
//  Constructor, creates a new Zyre node. Note that until you start the
//  node it is silent and invisible to other nodes on the network.
CZMQ_EXPORT zyre_t *
    zyre_new (zctx_t *ctx);

//  Destructor, destroys a Zyre node. When you destroy a node, any
//  messages it is sending or receiving will be discarded.
CZMQ_EXPORT void
    zyre_destroy (zyre_t **self_p);

//  Set node header; these are provided to other nodes during discovery
//  and come in each ENTER message.
CZMQ_EXPORT void
    zyre_set_header (zyre_t *self, char *name, char *format, ...);

//  Start node, after setting header values. When you start a node it
//  begins discovery and connection. There is no stop method; to stop
//  a node, destroy it.
CZMQ_EXPORT void
    zyre_start (zyre_t *self);

//  Join a named group; after joining a group you can send messages to
//  the group and all Zyre nodes in that group will receive them.
CZMQ_EXPORT int
    zyre_join (zyre_t *self, const char *group);

//  Leave a group
CZMQ_EXPORT int
    zyre_leave (zyre_t *self, const char *group);

//  Receive next message from network; the message may be a control
//  message (ENTER, EXIT, JOIN, LEAVE) or data (WHISPER, SHOUT).
//  Returns zmsg_t object, or NULL if interrupted
CZMQ_EXPORT zmsg_t *
    zyre_recv (zyre_t *self);

//  Send message to single peer; peer ID is first frame in message
//  Destroys message after sending
CZMQ_EXPORT int
    zyre_whisper (zyre_t *self, zmsg_t **msg_p);

//  Send message to a group of peers
CZMQ_EXPORT int
    zyre_shout (zyre_t *self, zmsg_t **msg_p);

//  Return handle to the Zyre node, for polling
CZMQ_EXPORT void *
    zyre_socket (zyre_t *self);

//  Self test of this class
CZMQ_EXPORT void
    zyre_test (bool verbose);
//  @end

#ifdef __cplusplus
}
#endif

#endif

