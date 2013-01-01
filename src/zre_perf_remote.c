/*  =========================================================================
    zre_perf_remote - remote performance peer

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
#include "../include/zre.h"

// to test performances, run zre_perf_remote [node_count] first then zre_perf_local

#define MAX_GROUP 10

static void
node_task (void *args, zctx_t *ctx, void *pipe)
{
    zre_node_t *node = zre_node_new ();
    int64_t counter = 0;
    char *to_peer = NULL;        //  Either of these set,
    char *to_group = NULL;       //    and we set a message
    char *cookie = NULL;         //  received message
    char *sending_cookie = NULL; //  sending message
    
    zmq_pollitem_t pollitems [] = {
        { pipe,                             0, ZMQ_POLLIN, 0 },
        { zre_node_handle (node), 0, ZMQ_POLLIN, 0 }
    };

    // all node joins GLOBAL
    zre_node_join (node, "GLOBAL");

    while (!zctx_interrupted) {
        if (zmq_poll (pollitems, 2, randof (1000) * ZMQ_POLL_MSEC) == -1)
            break;              //  Interrupted

        if (pollitems [0].revents & ZMQ_POLLIN)
            break;              //  Any command from parent means EXIT

        //  Process an event from node
        if (pollitems [1].revents & ZMQ_POLLIN) {
            zmsg_t *incoming = zre_node_recv (node);
            if (!incoming)
                break;              //  Interrupted

            char *event = zmsg_popstr (incoming);
            if (streq (event, "ENTER")) {
                //  Always say hello to new peer
                to_peer = zmsg_popstr (incoming);
                sending_cookie = "R:HELLO";
            }
            else
            if (streq (event, "EXIT")) {
                //  Do nothing
            }
            else
            if (streq (event, "WHISPER")) {
                to_peer = zmsg_popstr (incoming);
                cookie = zmsg_popstr (incoming);

                // if a message comes from zre_perf_local, send back a special response
                if (streq (cookie, "S:WHISPER")) {
                    sending_cookie = "R:WHISPER";
                }
                else {
                    free (to_peer);
                    free (cookie);
                    to_peer = NULL;
                    cookie = NULL;
                }
            }
            else
            if (streq (event, "SHOUT")) {
                to_peer = zmsg_popstr (incoming);
                to_group = zmsg_popstr (incoming);
                cookie = zmsg_popstr (incoming);

                // if a message comes from zre_perf_local, send back a special response
                if (streq (cookie, "S:SHOUT")) {
                    free (to_peer);
                    to_peer = NULL;
                    sending_cookie = "R:SHOUT";
                }
                else {
                    free (to_peer);
                    free (to_group);
                    to_peer = NULL;
                    to_group = NULL;
                }
            }
            free (event);
            zmsg_destroy (&incoming);

            //  Send outgoing messages if needed
            if (to_peer) {
                zmsg_t *outgoing = zmsg_new ();
                zmsg_addstr (outgoing, to_peer);
                zmsg_addstr (outgoing, sending_cookie);
                zre_node_whisper (node, &outgoing);
                free (to_peer);
                to_peer = NULL;
            }
            if (to_group) {
                zmsg_t *outgoing = zmsg_new ();
                zmsg_addstr (outgoing, to_group);
                zmsg_addstr (outgoing, sending_cookie);
                zre_node_shout (node, &outgoing);
                free (to_group);
                to_group = NULL;
            }
            if (cookie) {
                free (cookie);
                cookie = NULL;
            }
        }
    }
    zre_node_destroy (&node);
}


int main (int argc, char *argv [])
{
    //  Initialize context for talking to tasks
    zctx_t *ctx = zctx_new ();
    zctx_set_linger (ctx, 100);
    
    //  Get number of nodes to simulate, default 100
    int max_node = 100;
    int nbr_node = 0;
    if (argc > 1)
        max_node = atoi (argv [1]);

    //  We address nodes as an array of pipes
    void **pipes = zmalloc (sizeof (void *) * max_node);

    for (nbr_node = 0; nbr_node < max_node; nbr_node++) {
        pipes [nbr_node] = zthread_fork (ctx, node_task, NULL);
        zclock_log ("I: Started node (%d running)", nbr_node + 1);
    }
    //  We will randomly start and stop node threads
    while (!zctx_interrupted) {
        zclock_sleep (1000);
    }
    zclock_log ("I: Stopped perl_remote");
    zctx_destroy (&ctx);
    free (pipes);
    return 0;
}
