/*  =========================================================================
    zre_log - record log data

    -------------------------------------------------------------------------
    Copyright (c) 1991-2012 iMatix Corporation <www.imatix.com>
    Copyright other contributors as noted in the AUTHORS file.

    This file is part of ZyRE, the ZeroMQ Realtime Experience framework:
    http://zyre.org.

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

#ifndef __ZRE_LOG_H_INCLUDED__
#define __ZRE_LOG_H_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _zre_log_t zre_log_t;

//  Constructor
zre_log_t *
    zre_log_new (char *endpoint);

//  Destructor
void
    zre_log_destroy (zre_log_t **self_p);

//  Connect log to remote endpoint
void
    zre_log_connect (zre_log_t *self, char *endpoint);

//  Record one log event
void
    zre_log_info (zre_log_t *self, int event, char *peer, char *format, ...);

#ifdef __cplusplus
}
#endif

#endif
