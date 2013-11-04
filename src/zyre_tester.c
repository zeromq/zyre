/*  =========================================================================
    zre_tester - bulk test tool

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

#define MAX_GROUP 10

//  List of active nodes
static zhash_t *nodes;

static void
node_task (void *args, zctx_t *ctx, void *pipe)
{
    zre_node_t *node = zre_node_new ();
    int64_t counter = 0;
    char *to_peer = NULL;        //  Either of these set,
    char *to_group = NULL;       //    and we set a message
    char *cookie = NULL;
    
    zmq_pollitem_t pollitems [] = {
        { pipe,                   0, ZMQ_POLLIN, 0 },
        { zre_node_handle (node), 0, ZMQ_POLLIN, 0 }
    };
    //  Do something once a second
    int64_t trigger = zclock_time () + 1000;
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
            }
            else
            if (streq (event, "EXIT")) {
                //  Always try talk to departed peer
                to_peer = zmsg_popstr (incoming);
            }
            else
            if (streq (event, "WHISPER")) {
                //  Send back response 1/2 the time
                if (randof (2) == 0) {
                    to_peer = zmsg_popstr (incoming);
                    cookie = zmsg_popstr (incoming);
                }
            }
            else
            if (streq (event, "SHOUT")) {
                to_peer = zmsg_popstr (incoming);
                to_group = zmsg_popstr (incoming);
                cookie = zmsg_popstr (incoming);
                //  Send peer response 1/3rd the time
                if (randof (3) > 0) {
                    free (to_peer);
                    to_peer = NULL;
                }
                //  Send group response 1/3rd the time
                if (randof (3) > 0) {
                    free (to_group);
                    to_group = NULL;
                }
            }
            else
            if (streq (event, "JOIN")) {
                char *from_peer = zmsg_popstr (incoming);
                char *group = zmsg_popstr (incoming);
                if (randof (3) > 0) {
                    zre_node_join (node, group);
                }
                free (from_peer);
                free (group);
            }
            else
            if (streq (event, "LEAVE")) {
                char *from_peer = zmsg_popstr (incoming);
                char *group = zmsg_popstr (incoming);
                if (randof (3) > 0) {
                    zre_node_leave (node, group);
                }
                free (from_peer);
                free (group);
            }
            else
            if (streq (event, "DELIVER")) {
                char *filename = zmsg_popstr (incoming);
                char *fullname = zmsg_popstr (incoming);
                printf ("I: received file %s\n", fullname);
                free (fullname);
                free (filename);
            }
            free (event);
            zmsg_destroy (&incoming);

            //  Send outgoing messages if needed
            if (to_peer) {
                zmsg_t *outgoing = zmsg_new ();
                zmsg_addstr (outgoing, to_peer);
                zmsg_addstr (outgoing, "%lu", counter++);
                zre_node_whisper (node, &outgoing);
                free (to_peer);
                to_peer = NULL;
            }
            if (to_group) {
                zmsg_t *outgoing = zmsg_new ();
                zmsg_addstr (outgoing, to_group);
                zmsg_addstr (outgoing, "%lu", counter++);
                zre_node_shout (node, &outgoing);
                free (to_group);
                to_group = NULL;
            }
            if (cookie) {
                free (cookie);
                cookie = NULL;
            }
        }
        if (zclock_time () >= trigger) {
            trigger = zclock_time () + 1000;
            char group [10];
            sprintf (group, "GROUP%03d", randof (MAX_GROUP));
            if (randof (4) == 0)
                zre_node_join (node, group);
            else
            if (randof (3) == 0)
                zre_node_leave (node, group);
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
    int nbr_nodes = 0;
    int max_iterations = -1;
    int nbr_iterations = 0;
    if (argc > 1)
        max_node = atoi (argv [1]);
    if (argc > 2)
        max_iterations = atoi (argv [2]);

    //  We address nodes as an array of pipes
    void **pipes = zmalloc (sizeof (void *) * max_node);

    //  We will randomly start and stop node threads
    while (!zctx_interrupted) {
        uint index = randof (max_node);
        //  Toggle node thread
        if (pipes [index]) {
            zstr_send (pipes [index], "STOP");
            zsocket_destroy (ctx, pipes [index]);
            pipes [index] = NULL;
            zclock_log ("I: Stopped node (%d running)", --nbr_nodes);
        }
        else {
            pipes [index] = zthread_fork (ctx, node_task, NULL);
            zclock_log ("I: Started node (%d running)", ++nbr_nodes);
        }
        nbr_iterations++;
        if (max_iterations > 0 && nbr_iterations >= max_iterations)
            break;
        //  Sleep ~750 msecs randomly so we smooth out activity
        zclock_sleep (randof (500) + 500);
    }
    zclock_log ("I: Stopped tester (%d iterations)", nbr_iterations);
    zctx_destroy (&ctx);
    free (pipes);
    return 0;
}
