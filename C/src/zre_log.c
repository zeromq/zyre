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

#include <czmq.h>
#include "../include/zre.h"


//  ---------------------------------------------------------------------
//  Structure of our class

struct _zre_log_t {
    zctx_t *ctx;                //  CZMQ context
    void *publisher;            //  Socket to send to
    uint16_t nodeid;            //  Own correlation ID
};


//  ---------------------------------------------------------------------
//  Construct new log object

zre_log_t *
zre_log_new (char *endpoint)
{
    zre_log_t *self = (zre_log_t *) zmalloc (sizeof (zre_log_t));
    self->ctx = zctx_new ();
    self->publisher = zsocket_new (self->ctx, ZMQ_PUB);
    //  Modified Bernstein hashing function
    while (*endpoint)
        self->nodeid = 33 * self->nodeid ^ *endpoint++;
    
    return self;
}


//  ---------------------------------------------------------------------
//  Destroy log object

void
zre_log_destroy (zre_log_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zre_log_t *self = *self_p;
        zctx_destroy (&self->ctx);
        free (self);
        *self_p = NULL;
    }
}


//  ---------------------------------------------------------------------
//  Connect log to remote endpoint

void
zre_log_connect (zre_log_t *self, char *endpoint)
{
    assert (self);
    zmq_connect (self->publisher, endpoint);
}


//  ---------------------------------------------------------------------
//  Record one log event

void
zre_log_info (zre_log_t *self, int event, char *peer, char *format, ...)
{
    uint16_t peerid = 0;
    while (peer && *peer)
        peerid = 33 * peerid ^ *peer++;

    //  Format body if any
    char body [256];
    if (format) {
        assert (self);
        va_list argptr;
        va_start (argptr, format);
        vsnprintf (body, 255, format, argptr);
        va_end (argptr);
    }
    else
        *body = 0;
    
    zre_log_msg_send_log (self->publisher, ZRE_LOG_MSG_LEVEL_INFO, 
        event, self->nodeid, peerid, time (NULL), body);
}
