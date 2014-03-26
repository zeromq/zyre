/*  =========================================================================
    logger - ZRE/LOG collector

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

#include "../include/zyre.h"
#include "../include/zre_log_msg.h"

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
        case ZRE_LOG_MSG_EVENT_SEND:
            event = "Send";
            break;
        case ZRE_LOG_MSG_EVENT_RECV:
            event = "Recv";
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
    zbeacon_t *beacon = zbeacon_new (ctx, ZRE_DISCOVERY_PORT);
    char *host = zbeacon_hostname (beacon);

    //  Bind to an ephemeral port
    void *collector = zsocket_new (ctx, ZMQ_SUB);
    int port = zsocket_bind (collector, "tcp://%s:*", host);
    zsocket_set_subscribe (collector, "");

    //  Announce this to all peers we connect to
    zyre_t *node = zyre_new (ctx);
    zyre_set_header (node, "X-ZRELOG", "tcp://%s:%d", host, port);
    zyre_start (node);

    zpoller_t *poller = zpoller_new (collector, zyre_socket (node), NULL);
    while (!zctx_interrupted) {
        void *which = zpoller_wait (poller, -1);
        if (which == collector)
            s_print_log_msg (collector);
        else
        if (which == zyre_socket (node)) {
            zmsg_t *msg = zyre_recv (node);
            if (!msg)
                break;              //  Interrupted
            zmsg_destroy (&msg);
        }
        else
            break;                  //  Interrupted
    }
    zyre_destroy (&node);
    zbeacon_destroy (&beacon);
    zctx_destroy (&ctx);
    return 0;
}
