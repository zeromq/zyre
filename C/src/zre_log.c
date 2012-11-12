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
};


//  ---------------------------------------------------------------------
//  Construct new log object

zre_log_t *
zre_log_new (void)
{
    zre_log_t *self = (zre_log_t *) zmalloc (sizeof (zre_log_t));
    self->ctx = zctx_new ();
    self->publisher = zsocket_new (self->ctx, ZMQ_PUB);
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
zre_log_write (zre_log_t *self, int event, char *node, char *peer, char *data, ...)
{
    assert (self);
    va_list argptr;
    va_start (argptr, data);
    char *body = (char *) malloc (LOGDATA_MAX + 1);
    vsnprintf (body, LOGDATA_MAX, data, argptr);
    
    time_t curtime = time (NULL);
    struct tm *loctime = localtime (&curtime);
    char timenow [10];
    strftime (timenow, 10, "%H:%M:%S", loctime);

    //  send to publisher
    zstr_sendf (self->publisher, "%s %d %s %s %s", timenow, event, node, peer, body);
    
    free (body);
    va_end (argptr);
}
