/*  =========================================================================
    zre_logger - ZRE/LOG collector

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

#include <czmq.h>
#include "../include/zre_internal.h"

static void
s_print_log_msg (void *collector)
{
    zre_log_msg_t *msg = zre_log_msg_recv (collector);
    if (!msg)
        return;                 //  Interrupted

    time_t curtime = zre_log_msg_time (msg);
    char *event = NULL;
    switch (zre_log_msg_event (msg)) {
        case ZRE_LOG_MSG_EVENT_JOIN:
            event = "Join group";
            break;
        case ZRE_LOG_MSG_EVENT_LEAVE:
            event = "Leave group";
            break;
        case ZRE_LOG_MSG_EVENT_ENTER:
            event = "Peer enters";
            break;
        case ZRE_LOG_MSG_EVENT_EXIT:
            event = "Peer exits";
            break;
    }
    struct tm *loctime = localtime (&curtime);
    char timestr [20];
    strftime (timestr, 20, "%y-%m-%d %H:%M:%S ", loctime);

    printf ("%s I: [%04X] [%04X] - %s %s\n",
        timestr,
        zre_log_msg_node (msg),
        zre_log_msg_peer (msg),
        event,
        zre_log_msg_data (msg));

    zre_log_msg_destroy (&msg);
}


int main (int argc, char *argv [])
{
    zctx_t *ctx = zctx_new ();

    //  Use the CZMQ zbeacon class to make sure we listen on the 
    //  same network interface as our peers
    void *collector = zsocket_new (ctx, ZMQ_SUB);
    zbeacon_t *beacon = zbeacon_new (ZRE_DISCOVERY_PORT);
    char *host = zbeacon_hostname (beacon);

    //  Bind to an ephemeral port
    int port = zsocket_bind (collector, "tcp://%s:*", host);

    //  Announce this to all peers we connect to
    zre_node_t *node = zre_node_new ();
    zre_node_set_header (node, "X-ZRELOG", "tcp://%s:%d", host, port);

    //  Get all log messages (don't filter)
    zsocket_set_subscribe (collector, "");

    zmq_pollitem_t pollitems [] = {
        { collector, 0, ZMQ_POLLIN, 0 },
        { zre_node_handle (node), 0, ZMQ_POLLIN, 0 }
    };

    while (!zctx_interrupted) {
        if (zmq_poll (pollitems, 2, 1000 * ZMQ_POLL_MSEC) == -1)
            break;              //  Interrupted

        //  Handle input on collector
        if (pollitems [0].revents & ZMQ_POLLIN)
            s_print_log_msg (collector);

        //  Handle event from node (ignore it)
        if (pollitems [1].revents & ZMQ_POLLIN) {
            zmsg_t *msg = zre_node_recv (node);
            if (!msg)
                break;              //  Interrupted
            zmsg_destroy (&msg);
        }
    }
    zre_node_destroy (&node);
    zbeacon_destroy (&beacon);
    zctx_destroy (&ctx);
    return 0;
}
