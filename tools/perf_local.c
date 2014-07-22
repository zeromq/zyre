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

static bool
s_node_recv (zyre_t *node, char* command, char* expected)
{
    bool result = false;
    zmsg_t *incoming = zyre_recv (node);
    assert (incoming);

    char *event = zmsg_popstr (incoming);
    if (streq (event, command)) {
        char *peer = zmsg_popstr (incoming);
        char *group = NULL;
        if (streq (command, "SHOUT"))
            group = zmsg_popstr (incoming);
        
        char *cookie = zmsg_popstr (incoming);
        if (streq (cookie, expected))
            result = true;
        
        free (peer);
        if (group)
            free (group);
        free (cookie);
    }
    free (event);
    zmsg_destroy (&incoming);

    return result;
}


int
main (int argc, char *argv [])
{
    //  Get number of remote nodes to simulate, default 100
    //  If we run multiple perf_remote on multiple machines,
    //  max_node must be sum of all the remote node counts.
    int max_node = 100;
    int max_message = 10000;
    int nbr_node = 0;
    int nbr_hello_response = 0;
    int nbr_message = 0;
    int nbr_message_response = 0;

    if (argc > 1)
        max_node = atoi (argv [1]);
    if (argc > 2)
        max_message = atoi (argv [2]);

    zyre_t *node = zyre_new (NULL);
    zyre_start (node);
    zyre_join (node, "GLOBAL");

    int64_t start = zclock_time ();
    int64_t elapse;

    char **peers = zmalloc (sizeof (char *) * max_node);

    while (true) {
        zmsg_t *incoming = zyre_recv (node);
        if (!incoming)
            break;              //  Interrupted

        //  If new peer, say hello to it and wait for it to answer us
        char *event = zmsg_popstr (incoming);
        if (streq (event, "ENTER")) {
            char *peer = zmsg_popstr (incoming);
            peers [nbr_node++] = peer;

            if (nbr_node == max_node) {
                // got HELLO from the all remote nodes
                elapse = zclock_time () - start;
                printf ("Took %ld ms to coordinate with all remote\n",
                        (long) elapse);
            }
        }
        else
        if (streq (event, "WHISPER")) {
            char *peer = zmsg_popstr (incoming);
            char *cookie = zmsg_popstr (incoming);

            if (streq (cookie, "R:HELLO")) {
                if (++nbr_hello_response == max_node) {
                    // got HELLO from the all remote nodes
                    elapse = zclock_time () - start;
                    printf ("Took %ld ms to get greeting from all remote\n",
                            (long) elapse);
                }
            }
            free (peer);
            free (cookie);
        }
        free (event);
        zmsg_destroy (&incoming);

        if (nbr_node == max_node && nbr_hello_response == max_node)
            break;
    }

    //  Send WHISPER message
    start = zclock_time ();
    zpoller_t *poller = zpoller_new (zyre_socket (node), NULL);
    for (nbr_message = 0; nbr_message < max_message; nbr_message++) {
        zyre_whispers (node, peers [nbr_message % max_node], "S:WHISPER");
        while (zpoller_wait (poller, 1000))
            if (s_node_recv (node, "WHISPER", "R:WHISPER"))
                nbr_message_response++;
    }

    while (nbr_message_response < max_message)
        if (s_node_recv (node, "WHISPER", "R:WHISPER"))
            nbr_message_response++;

    //  Got WHISPER response from the all remote nodes
    elapse = zclock_time () - start;
    printf ("Took %ld ms to send/receive %d message. %.2f msg/s \n", (long)elapse, max_message, (float) max_message * 1000 / elapse);

    //  send SPOUT message
    start = zclock_time ();
    nbr_message = 0;
    nbr_message_response = 0;

    max_message = max_message / max_node;

    for (nbr_message = 0; nbr_message < max_message; nbr_message++) {
        zyre_shouts (node, "GLOBAL", "S:SHOUT");
        while (zpoller_wait (poller, 1000))
            if (s_node_recv (node, "SHOUT", "R:SHOUT"))
                nbr_message_response++;
    }

    while (nbr_message_response < max_message * max_node)
        if (s_node_recv (node, "SHOUT", "R:SHOUT"))
            nbr_message_response++;

    // got SHOUT response from the all remote nodes
    elapse = zclock_time () - start;
    printf ("Took %ld ms to send %d, recv %d GROUP message. %.2f msg/s \n",
            (long) elapse, max_message, max_node * max_message,
            (float) max_node * max_message * 1000 / elapse);

    zyre_destroy (&node);
    for (nbr_node = 0; nbr_node < max_node; nbr_node++) {
        free (peers[nbr_node]);
    }
    zpoller_destroy (&poller);
    free (peers);
    return 0;
}
