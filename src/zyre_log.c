/*  =========================================================================
    zyre_log - record log data remotely to a collector

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

/*
@header
    This class lets you send ZRE log messages to a remote collector.
@discuss
@end
*/

#include "zyre_classes.h"

//  ---------------------------------------------------------------------
//  Structure of our class

struct _zyre_log_t {
    void *publisher;            //  Socket to send to
    zhash_t *subscribers;       //  Known subscribers
    uint16_t nodeid;            //  Own correlation ID
};


//  ---------------------------------------------------------------------
//  Construct new log object

zyre_log_t *
zyre_log_new (zctx_t *ctx, const char *sender)
{
    zyre_log_t *self = (zyre_log_t *) zmalloc (sizeof (zyre_log_t));
    //  If the system can't create new sockets, discard log data silently
    self->publisher = zsocket_new (ctx, ZMQ_PUB);
    self->subscribers = zhash_new ();

    //  We calculate a short node ID by hashing the full endpoint
    //  Use a modified Bernstein hashing function
    while (*sender)
        self->nodeid = 33 * self->nodeid ^ *sender++;
    
    return self;
}


//  ---------------------------------------------------------------------
//  Destroy log object

void
zyre_log_destroy (zyre_log_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zyre_log_t *self = *self_p;
        zlist_t *subscribers = zhash_keys (self->subscribers);
        char *subscriber = (char *) zlist_first (subscribers);
        while (subscriber) {
            zhash_delete (self->subscribers, subscriber);
            zsocket_disconnect (self->publisher, "%s", subscriber);
            subscriber = (char *) zlist_next (subscribers);
        }
        zlist_destroy (&subscribers);
        zhash_destroy (&self->subscribers);
        free (self);
        *self_p = NULL;
    }
}


//  ---------------------------------------------------------------------
//  Connect log to remote subscriber endpoint

void
zyre_log_connect (zyre_log_t *self, const char *format, ...)
{
    assert (self);
    if (self->publisher) {
        va_list argptr;
        va_start (argptr, format);
        char *endpoint = zsys_vprintf (format, argptr);
        va_end (argptr);

        //  Only connect once to any given subscriber
        if (!zhash_lookup (self->subscribers, endpoint)) {
            int rc = zsocket_connect (self->publisher, "%s", endpoint);
            assert (rc == 0);
            zhash_insert (self->subscribers, endpoint, "TRUE");
        }
        free (endpoint);
    }
}


//  ---------------------------------------------------------------------
//  Broadcast a log information message

void
zyre_log_info (zyre_log_t *self, int event, const char *peer, const char *format, ...)
{
    assert (self);

    //  Calculate the nodeid for the peer this message references
    uint16_t peer_nodeid = 0;
    while (peer && *peer)
        peer_nodeid = 33 * peer_nodeid ^ *peer++;

    //  Format body if any
    char body [256];
    if (format) {
        va_list argptr;
        va_start (argptr, format);
        vsnprintf (body, 255, format, argptr);
        va_end (argptr);
    }
    else
        *body = 0;
    
    if (self->publisher)
        zre_log_msg_send_log (self->publisher,
                              ZRE_LOG_MSG_LEVEL_INFO,
                              event, self->nodeid, peer_nodeid,
                              time (NULL), body);
}


//  ---------------------------------------------------------------------
//  Broadcast a log warning message

void
zyre_log_warning (zyre_log_t *self, const char *peer, const char *format, ...)
{
    assert (self);
    assert (format);

    //  Calculate the nodeid for the peer this message references
    uint16_t peer_nodeid = 0;
    while (peer && *peer)
        peer_nodeid = 33 * peer_nodeid ^ *peer++;

    va_list argptr;
    va_start (argptr, format);
    char *message = zsys_vprintf (format, argptr);
    va_end (argptr);

    if (self->publisher)
        zre_log_msg_send_log (self->publisher,
                              ZRE_LOG_MSG_LEVEL_WARNING,
                              0, self->nodeid, peer_nodeid,
                              time (NULL), message);
    free (message);
}


//  ---------------------------------------------------------------------
//  Broadcast a log error message

void
zyre_log_error (zyre_log_t *self, const char *peer, const char *format, ...)
{
    assert (self);

    //  Calculate the nodeid for the peer this message references
    uint16_t peer_nodeid = 0;
    while (peer && *peer)
        peer_nodeid = 33 * peer_nodeid ^ *peer++;

    va_list argptr;
    va_start (argptr, format);
    char *message = zsys_vprintf (format, argptr);
    va_end (argptr);

    if (self->publisher)
        zre_log_msg_send_log (self->publisher,
                              ZRE_LOG_MSG_LEVEL_ERROR,
                              0, self->nodeid, peer_nodeid,
                              time (NULL), message);
    free (message);
}


//  --------------------------------------------------------------------------
//  Self test of this class

void
zyre_log_test (bool verbose)
{
    printf (" * zyre_log: ");

    //  @selftest
    zctx_t *ctx = zctx_new ();
    //  Get all incoming log messages
    void *collector = zsocket_new (ctx, ZMQ_SUB);
    zsocket_bind (collector, "tcp://127.0.0.1:5555");
    zsocket_set_subscribe (collector, "");

    //  Create a log instance to send log messages
    zyre_log_t *log = zyre_log_new (ctx, "this is me");
    zyre_log_connect (log, "tcp://127.0.0.1:5555");

    //  Workaround for issue 270; give time for connect to
    //  happen and subscriptions to go to pub socket; 200
    //  msec should be enough for under valgrind on a slow PC
    zpoller_t *poller = zpoller_new (collector, NULL);
    zpoller_wait (poller, 200);

    //  Send some messages
    zyre_log_info (log, ZRE_LOG_MSG_EVENT_JOIN, NULL, "this is you");
    zyre_log_info (log, ZRE_LOG_MSG_EVENT_EXIT, "Pizza time", "this is you");
    zyre_log_warning (log, "this is you", "Time flies like an %s", "arrow");
    zyre_log_error (log, "this is you", "Fruit flies like a %s", "banana");

    int count = 0;
    while (count < 4) {
        zre_log_msg_t *msg = zre_log_msg_recv (collector);
        assert (msg);
        if (verbose)
            zre_log_msg_dump (msg);
        zre_log_msg_destroy (&msg);
        count++;
    }
    zpoller_destroy (&poller);
    zyre_log_destroy (&log);
    zctx_destroy (&ctx);
    //  @end
    printf ("OK\n");
}
