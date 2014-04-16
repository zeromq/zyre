/*  =========================================================================
    zyre.h - Zyre public API

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
#if CZMQ_VERSION < 20200
#   error "Zyre needs CZMQ/2.2.0 or later"
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

//  Return our own UUID, after successful initialization.
CZMQ_EXPORT const char *
    zyre_uuid (zyre_t *self);

//  Set node header; these are provided to other nodes during discovery
//  and come in each ENTER message.
CZMQ_EXPORT void
    zyre_set_header (zyre_t *self, char *name, char *format, ...);

//  Set verbose mode; this tells the node to log all traffic as well
//  as all major events.
CZMQ_EXPORT void
    zyre_set_verbose (zyre_t *self);

//  Set ZRE discovery port; defaults to 5670, this call overrides that
//  so you can create independent clusters on the same network, for e.g
//  development vs. production. Has no effect after zyre_start().
CZMQ_EXPORT void
    zyre_set_port (zyre_t *self, int port_nbr);

//  Set ZRE discovery interval, in milliseconds. Default is instant
//  beacon exploration followed by pinging every 1,000 msecs.
CZMQ_EXPORT void
    zyre_set_interval (zyre_t *self, size_t interval);

//  Start node, after setting header values. When you start a node it
//  begins discovery and connection.
CZMQ_EXPORT void
    zyre_start (zyre_t *self);

//  Stop node; this signals to other peers that this node will go away.
//  This is polite; however you can also just destroy the node without
//  stopping it.
CZMQ_EXPORT void
    zyre_stop (zyre_t *self);

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

//  Send message to single peer, specified as a UUID string
//  Destroys message after sending
CZMQ_EXPORT int
    zyre_whisper (zyre_t *self, char *peer, zmsg_t **msg_p);

//  Send message to a named group
//  Destroys message after sending
CZMQ_EXPORT int
    zyre_shout (zyre_t *self, char *group, zmsg_t **msg_p);

//  Send formatted string to a single peer specified as UUID string
CZMQ_EXPORT int
    zyre_whispers (zyre_t *self, char *peer, char *format, ...);

//  Send formatted string to a named group
CZMQ_EXPORT int
    zyre_shouts (zyre_t *self, char *group, char *format, ...);

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

//  Additional public API classes

#include "zyre_event.h"

#endif

