/*  =========================================================================
    zre_logger - ZyRE log collector

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

int main (int argc, char *argv [])
{
    zctx_t *ctx = zctx_new ();

    //  Use the ZyRE UDP class to make sure we listen on the same
    //  network interface as our peers
    zre_udp_t *udp = zre_udp_new (PING_PORT_NUMBER);
    char *host = zre_udp_host (udp);
    void *collector = zsocket_new (ctx, ZMQ_SUB);
    zsocket_bind (collector, "tcp://%s:%d", host, LOG_PORT_NUMBER);
    zsocket_set_subscribe (collector, "");

    zre_interface_t *interface = zre_interface_new ();
    zre_interface_header_set (interface, "LOG_COLLECTOR", "tcp://%s:%d", host, LOG_PORT_NUMBER);
    
    zmq_pollitem_t pollitems [] = {
        { collector, 0, ZMQ_POLLIN, 0 },
        { zre_interface_handle (interface), 0, ZMQ_POLLIN, 0 }
    };
    
    while (!zctx_interrupted) {
        if (zmq_poll (pollitems, 2, 1000 * ZMQ_POLL_MSEC) == -1)
            break;              //  Interrupted

        //  Handle input on collector
        if (pollitems [0].revents & ZMQ_POLLIN) {
            char *data = zstr_recv (collector);
            puts (data);
            free (data);
        }
        //  Handle event from interface
        if (pollitems [1].revents & ZMQ_POLLIN) {
            zmsg_t *msg = zre_interface_recv (interface);
            if (!msg)
                break;              //  Interrupted
            zmsg_destroy (&msg);
        }
    }
    zre_interface_destroy (&interface);
    zre_udp_destroy (&udp);
    zctx_destroy (&ctx);
    return 0;
}
