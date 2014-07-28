/*  =========================================================================
    perf_remote - remote performance peer

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
#include "../include/zyre.h"

// to test performances, run perf_remote [node_count] first then perf_local

#define MAX_GROUP 10

static void
node_actor (zsock_t *pipe, void *args)
{
    zyre_t *node = zyre_new (NULL);
    zyre_start (node);
    zsock_signal (pipe, 0);
    
    char *to_peer = NULL;        //  Either of these set,
    char *to_group = NULL;       //    and we set a message
    char *cookie = NULL;         //  received message
    char *sending_cookie = NULL; //  sending message

    zpoller_t *poller = zpoller_new (pipe, zyre_socket (node), NULL);
    zyre_join (node, "GLOBAL");
    
    while (!zsys_interrupted) {
        void *which = zpoller_wait (poller, randof (1000));
        if (which == pipe || which == NULL)
            break;              //  Any command from parent means exit

        zmsg_t *incoming = zyre_recv (node);
        if (!incoming)
            break;              //  Interrupted

        char *event = zmsg_popstr (incoming);
        if (streq (event, "ENTER")) {
            //  Always say hello to new peer
            to_peer = zmsg_popstr (incoming);
            sending_cookie = "R:HELLO";
        }
        else
        if (streq (event, "EXIT"))
            ;   //  Do nothing
        else
        if (streq (event, "WHISPER")) {
            to_peer = zmsg_popstr (incoming);
            cookie = zmsg_popstr (incoming);

            //  If a message comes from perf_local,
            //  send back a special response
            if (streq (cookie, "S:WHISPER"))
                sending_cookie = "R:WHISPER";
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

            //  If a message comes from perf_local,
            //  send back a special response
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
            zyre_whispers (node, to_peer, "%s", sending_cookie);
            free (to_peer);
            to_peer = NULL;
        }
        if (to_group) {
            zyre_shouts (node, to_group, "%s", sending_cookie);
            free (to_group);
            to_group = NULL;
        }
        if (cookie) {
            free (cookie);
            cookie = NULL;
        }
    }
    zpoller_destroy (&poller);
    zyre_destroy (&node);
}


int main (int argc, char *argv [])
{
    //  Get number of nodes to simulate, default 100
    int max_node = 100;
    if (argc > 1)
        max_node = atoi (argv [1]);

    //  We address nodes as an array of pipes
    void **nodes = zmalloc (sizeof (zactor_t *) * max_node);

    int node_nbr;
    for (node_nbr = 0; node_nbr < max_node; node_nbr++) {
        nodes [node_nbr] = zactor_new (node_actor, NULL);
        zclock_log ("I: Started node %d", node_nbr + 1);
    }
    //  We will randomly start and stop node threads
    while (!zsys_interrupted)
        zclock_sleep (1000);

    for (node_nbr = 0; node_nbr < max_node; node_nbr++) {
        zclock_log ("I: Stopped node %d", node_nbr + 1);
        zactor_destroy (nodes [node_nbr]);
    }
    zclock_log ("I: Stopped perl_remote");
    free (nodes);
    return 0;
}
