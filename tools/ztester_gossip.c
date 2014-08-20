/*  =========================================================================
    ztester_gossip - bulk test tool using gossip discovery

    This tool starts many nodes, each as a thread (zactor), which discover
    each other using gossip discovery (zgossip) and interconnect over inproc:

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
node_actor (zsock_t *pipe, void *args)
{
    zyre_t *node = zyre_new (NULL);
    if (!node)
        return;                 //  Could not create new node
    zyre_set_verbose (node);

    zyre_set_endpoint (node, "inproc://%s", (char *) args);
    free (args);

    //  Connect to test hub
    zyre_gossip_connect (node, "inproc://zyre-hub");
    zyre_start (node);
    zsock_signal (pipe, 0);     //  Signal "ready" to caller

    int counter = 0;
    char *to_peer = NULL;        //  Either of these set,
    char *to_group = NULL;       //    and we set a message
    char *cookie = NULL;

    zpoller_t *poller = zpoller_new (pipe, zyre_socket (node), NULL);
    int64_t trigger = zclock_time () + 1000;
    while (true) {
        void *which = zpoller_wait (poller, randof (1000));
        if (!which)
            break;              //  Interrupted

        //  $TERM from parent means exit; anything else is breach of
        //  contract so we should assert
        if (which == pipe) {
            char *command = zstr_recv (pipe);
            assert (streq (command, "$TERM"));
            zstr_free (&command);
            break;              //  Finished
        }
        //  Process an event from node
        if (which == zyre_socket (node)) {
            zmsg_t *incoming = zyre_recv (node);
            if (!incoming)
                break;          //  Interrupted

            char *event = zmsg_popstr (incoming);
            char *peer = zmsg_popstr (incoming);
            char *name = zmsg_popstr (incoming);
            if (streq (event, "ENTER"))
                //  Always say hello to new peer
                to_peer = strdup (peer);
            else
            if (streq (event, "EXIT"))
                //  Always try talk to departed peer
                to_peer = strdup (peer);
            else
            if (streq (event, "WHISPER")) {
                //  Send back response 1/2 the time
                if (randof (2) == 0) {
                    to_peer = strdup (peer);
                    cookie = zmsg_popstr (incoming);
                }
            }
            else
            if (streq (event, "SHOUT")) {
                to_peer = strdup (peer);
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
                char *group = zmsg_popstr (incoming);
                printf ("I: %s joined %s\n", name, group);
                free (group);
            }
            else
            if (streq (event, "LEAVE")) {
                char *group = zmsg_popstr (incoming);
                printf ("I: %s left %s\n", name, group);
                free (group);
            }
            free (event);
            free (peer);
            free (name);
            zmsg_destroy (&incoming);

            //  Send outgoing messages if needed
            if (to_peer) {
                zyre_whispers (node, to_peer, "%d", counter++);
                free (to_peer);
                to_peer = NULL;
            }
            if (to_group) {
                zyre_shouts (node, to_group, "%d", counter++);
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
    //  Get number of nodes N to simulate
    //  We need 3 x N x N + 3N file handles
    int max_nodes = 10;
    int nbr_nodes = 0;
    if (argc > 1)
        max_nodes = atoi (argv [1]);
    assert (max_nodes);

    int max_iterations = -1;
    int nbr_iterations = 0;
    if (argc > 2)
        max_iterations = atoi (argv [2]);

    //  Our gossip network will use one fixed hub (not a Zyre node),
    //  to which all nodes will connect
    zactor_t *hub = zactor_new (zgossip, "hub");
    zstr_sendx (hub, "BIND", "inproc://zyre-hub", NULL);
        
    //  We address nodes as an array of actors
    zactor_t **actors = zmalloc (sizeof (zactor_t *) * max_nodes);

    //  We will randomly start and stop node threads
    uint index;
    while (!zsys_interrupted) {
        index = randof (max_nodes);
        //  Toggle node thread
        if (actors [index]) {
            zactor_destroy (&actors [index]);
            actors [index] = NULL;
            zsys_info ("stopped node (%d running)", --nbr_nodes);
        }
        else {
            char node_name [10];
            sprintf (node_name, "node-%d", index);
            actors [index] = zactor_new (node_actor, strdup (node_name));
            zsys_info ("started node (%d running)", ++nbr_nodes);
        }
        nbr_iterations++;
        if (max_iterations > 0 && nbr_iterations >= max_iterations)
            break;
        //  Sleep ~300 msecs randomly so we smooth out activity
        zclock_sleep (randof (100) + 100);
    }
    zsys_info ("stopped tester (%d iterations)", nbr_iterations);

    //  Stop all remaining actors
    for (index = 0; index < max_nodes; index++) {
        if (actors [index])
            zactor_destroy (&actors [index]);
    }
    free (actors);
    
    zactor_destroy (&hub);
    return 0;
}
