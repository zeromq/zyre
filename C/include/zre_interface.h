/*  =========================================================================
    zre_interface - interface to a ZRE network

    -------------------------------------------------------------------------
    Copyright (c) 1991-2012 iMatix Corporation <www.imatix.com>
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

#ifndef __ZRE_INTERFACE_H_INCLUDED__
#define __ZRE_INTERFACE_H_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _zre_interface_t zre_interface_t;

//  Optional global context for zre_interface instances
//  Used for large-scale testing simulation only
CZMQ_EXPORT extern zctx_t *zre_global_ctx;

//  Optional temp directory; set by caller if needed
CZMQ_EXPORT extern char *zre_global_tmpdir;

//  Constructor
CZMQ_EXPORT zre_interface_t *
    zre_interface_new (void);

//  Destructor
CZMQ_EXPORT void
    zre_interface_destroy (zre_interface_t **self_p);

//  Set interface tracing on or off
CZMQ_EXPORT void
    zre_interface_verbose_set (zre_interface_t *self, bool verbose);

//  Join a group
CZMQ_EXPORT int
    zre_interface_join (zre_interface_t *self, const char *group);
    
//  Leave a group
CZMQ_EXPORT int
    zre_interface_leave (zre_interface_t *self, const char *group);

//  Receive next message from interface
CZMQ_EXPORT zmsg_t *
    zre_interface_recv (zre_interface_t *self);

//  Send message to single peer; peer ID is first frame in message
CZMQ_EXPORT int
    zre_interface_whisper (zre_interface_t *self, zmsg_t **msg_p);
    
//  Send message to a group of peers
CZMQ_EXPORT int
    zre_interface_shout (zre_interface_t *self, zmsg_t **msg_p);
    
//  Return interface handle, for polling
CZMQ_EXPORT void *
    zre_interface_handle (zre_interface_t *self);

//  Set node header value
CZMQ_EXPORT void
    zre_interface_header_set (zre_interface_t *self, char *name, char *format, ...);

//  Publish file into virtual space
CZMQ_EXPORT void
    zre_interface_publish (zre_interface_t *self, char *pathname, char *virtual);

//  Retract file publish
CZMQ_EXPORT void
    zre_interface_retract (zre_interface_t *self, char *virtual);

#ifdef __cplusplus
}
#endif

#endif
