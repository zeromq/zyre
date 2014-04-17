/*  =========================================================================
    tester - bulk test tool

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

#include "../include/zyre.h"

#define MAX_GROUP 10

static void
node_task (void *args, zctx_t *ctx, void *pipe)
{
    zyre_t *node = zyre_new (ctx);
    if (!node)
        return;                 //  Could not create new node
    zyre_set_verbose (node);
    zyre_start (node);

    int64_t counter = 0;
    char *to_peer = NULL;        //  Either of these set,
    char *to_group = NULL;       //    and we set a message
    char *cookie = NULL;

    zpoller_t *poller = zpoller_new (pipe, zyre_socket (node), NULL);
    int64_t trigger = zclock_time () + 1000;
    while (!zctx_interrupted) {
        void *which = zpoller_wait (poller, randof (1000));

        //  Any command from parent means EXIT
        if (which == pipe)
            break;

        //  Process an event from node
        if (which == zyre_socket (node)) {
            zmsg_t *incoming = zyre_recv (node);
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
                printf ("I: %s joined %s\n", from_peer, group);
                free (from_peer);
                free (group);
            }
            else
            if (streq (event, "LEAVE")) {
                char *from_peer = zmsg_popstr (incoming);
                char *group = zmsg_popstr (incoming);
                printf ("I: %s left %s\n", from_peer, group);
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
                zyre_whispers (node, to_peer, "%lu", counter++);
                free (to_peer);
                to_peer = NULL;
            }
            if (to_group) {
                zyre_shouts (node, to_group, "%lu", counter++);
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
                zyre_join (node, group);
            else
            if (randof (3) == 0)
                zyre_leave (node, group);
        }
    }
    zpoller_destroy (&poller);
    zyre_destroy (&node);
}


int main (int argc, char *argv [])
{
    //  Initialize context for talking to tasks
    zctx_t *ctx = zctx_new ();
    zctx_set_linger (ctx, 100);
    
    //  Get number of nodes N to simulate
    //  We need 3 x N x N + 3N file handles
    int max_nodes = 10;
    int nbr_nodes = 0;
    if (argc > 1)
        max_nodes = atoi (argv [1]);

    int max_iterations = -1;
    int nbr_iterations = 0;
    if (argc > 2)
        max_iterations = atoi (argv [2]);

    //  We address nodes as an array of pipes
    void **pipes = zmalloc (sizeof (void *) * max_nodes);

    //  We will randomly start and stop node threads
    while (!zctx_interrupted) {
        uint index = randof (max_nodes);
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
