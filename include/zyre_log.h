/*  =========================================================================
    zyre_log - record log data

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

#ifndef __ZYRE_LOG_H_INCLUDED__
#define __ZYRE_LOG_H_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _zyre_log_t zyre_log_t;

// @interface
//  Constructor
CZMQ_EXPORT zyre_log_t *
    zyre_log_new (zctx_t *ctx, const char *sender);

//  Destructor
CZMQ_EXPORT void
    zyre_log_destroy (zyre_log_t **self_p);

//  Connect log to remote subscriber endpoint
CZMQ_EXPORT void
    zyre_log_connect (zyre_log_t *self, const char *format, ...);

//  Broadcast a log information message
CZMQ_EXPORT void
    zyre_log_info (zyre_log_t *self, int event, const char *peer, const char *format, ...);

//  Broadcast a log warning message
CZMQ_EXPORT void
    zyre_log_warning (zyre_log_t *self, const char *peer, const char *format, ...);

//  Broadcast a log error message
CZMQ_EXPORT void
    zyre_log_error (zyre_log_t *self, const char *peer, const char *format, ...);

//  Self test of this class
CZMQ_EXPORT void
    zyre_log_test (bool verbose);
// @end
    
#ifdef __cplusplus
}
#endif

#endif
