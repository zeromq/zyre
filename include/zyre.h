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
#if CZMQ_VERSION < 2003
#   error "Zyre needs CZMQ/2.0.3 or later"
#endif

//  IANA-assigned port for ZYRE discovery protocol
#define ZRE_DISCOVERY_PORT  5670

//  The public API consists of the "zyre_t" class

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _zyre_t zyre_t;

//  @interface
//  Constructor
CZMQ_EXPORT zyre_t *
    zyre_new (zctx_t *ctx);

//  Destructor
CZMQ_EXPORT void
    zyre_destroy (zyre_t **self_p);

//  Set node tracing on or off
CZMQ_EXPORT void
    zyre_set_verbose (zyre_t *self, bool verbose);

//  Join a group
CZMQ_EXPORT int
    zyre_join (zyre_t *self, const char *group);

//  Leave a group
CZMQ_EXPORT int
    zyre_leave (zyre_t *self, const char *group);

//  Receive next message from node
CZMQ_EXPORT zmsg_t *
    zyre_recv (zyre_t *self);

//  Send message to single peer; peer ID is first frame in message
CZMQ_EXPORT int
    zyre_whisper (zyre_t *self, zmsg_t **msg_p);

//  Send message to a group of peers
CZMQ_EXPORT int
    zyre_shout (zyre_t *self, zmsg_t **msg_p);

//  Return node handle, for polling
CZMQ_EXPORT void *
    zyre_socket (zyre_t *self);

//  Set node property value
CZMQ_EXPORT void
    zyre_set (zyre_t *self, char *name, char *format, ...);

//  Self test of this class
CZMQ_EXPORT void
    zyre_test (bool verbose);
//  @end

#ifdef __cplusplus
}
#endif

#endif
