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

static bool
s_interface_recv (zre_interface_t *interface, char* command, char* expected)
{
    bool result = false;
    zmsg_t *incoming = zre_interface_recv (interface);

    assert (incoming);

    char *event = zmsg_popstr (incoming);

    if (streq (event, command)) {
        char *peer = zmsg_popstr (incoming);
        char *group = NULL;
        if (streq (command, "SHOUT"))
            group = zmsg_popstr (incoming);
        char *cookie = zmsg_popstr (incoming);

        if (streq (cookie, expected)) {
            result = true;
        }

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
    //  Get number of remote interfaces to simulate, default 100
    //  If we run multiple zre_perf_remote on multiple machines,
    //  max_interface must be sum of all the remote interface counts.
    int max_interface = 100;
    int max_message = 10000;
    int nbr_interface = 0;
    int nbr_hello_response = 0;
    int nbr_message = 0;
    int nbr_message_response = 0;

    if (argc > 1)
        max_interface = atoi (argv [1]);
    if (argc > 2)
        max_message = atoi (argv [2]);

    zre_interface_t *interface = zre_interface_new ();
    zre_interface_join (interface, "GLOBAL");

    int64_t start = zclock_time ();
    int64_t elapse;

    char **peers = zmalloc (sizeof (char *) * max_interface);

    while (true) {
        zmsg_t *incoming = zre_interface_recv (interface);
        if (!incoming)
            break;              //  Interrupted

        //  If new peer, say hello to it and wait for it to answer us
        char *event = zmsg_popstr (incoming);
        if (streq (event, "ENTER")) {
            char *peer = zmsg_popstr (incoming);
            peers[nbr_interface++] = peer;

            if (nbr_interface == max_interface) {
                // got HELLO from the all remote interfaces
                elapse = zclock_time () - start;
                printf ("Took %ld ms to coordinate with all remote\n", (long)elapse);
            }
        }
        else
        if (streq (event, "WHISPER")) {
            char *peer = zmsg_popstr (incoming);
            char *cookie = zmsg_popstr (incoming);

            if (streq (cookie, "R:HELLO")) {
                if (++nbr_hello_response == max_interface) {
                    // got HELLO from the all remote interfaces
                    elapse = zclock_time () - start;
                    printf ("Took %ld ms to get greeting from all remote\n", (long)elapse);
                }
            }
            free (peer);
            free (cookie);
        }
        free (event);
        zmsg_destroy (&incoming);

        if (nbr_interface == max_interface && nbr_hello_response == max_interface)
            break;
    }

    zmq_pollitem_t pollitems [] = {
        { zre_interface_handle (interface), 0, ZMQ_POLLIN, 0 }
    };

    //  send WHISPER message
    start = zclock_time ();
    for (nbr_message = 0; nbr_message < max_message; nbr_message++) {
        zmsg_t *outgoing = zmsg_new ();
        zmsg_addstr (outgoing, peers [nbr_message % max_interface]);
        zmsg_addstr (outgoing, "S:WHISPER");
        zre_interface_whisper (interface, &outgoing);

        while (zmq_poll (pollitems, 1, 0) > 0) {
            if (s_interface_recv (interface, "WHISPER", "R:WHISPER"))
                nbr_message_response++;
        }
    }

    while (nbr_message_response < max_message)
        if (s_interface_recv (interface, "WHISPER", "R:WHISPER"))
            nbr_message_response++;

    // got WHISPER response from the all remote interfaces
    elapse = zclock_time () - start;
    printf ("Took %ld ms to send/receive %d message. %.2f msg/s \n", (long)elapse, max_message, (float) max_message * 1000 / elapse);

    //  send SPOUT message
    start = zclock_time ();
    nbr_message = 0;
    nbr_message_response = 0;

    max_message = max_message / max_interface;

    for (nbr_message = 0; nbr_message < max_message; nbr_message++) {
        zmsg_t *outgoing = zmsg_new ();
        zmsg_addstr (outgoing, "GLOBAL");
        zmsg_addstr (outgoing, "S:SHOUT");
        zre_interface_shout (interface, &outgoing);

        while (zmq_poll (pollitems, 1, 0) > 0) {
            if (s_interface_recv (interface, "SHOUT", "R:SHOUT"))
                nbr_message_response++;
        }
    }

    while (nbr_message_response < max_message * max_interface)
        if (s_interface_recv (interface, "SHOUT", "R:SHOUT"))
            nbr_message_response++;

    // got SHOUT response from the all remote interfaces
    elapse = zclock_time () - start;
    printf ("Took %ld ms to send %d, recv %d GROUP message. %.2f msg/s \n",
            (long) elapse, max_message, max_interface * max_message,
            (float) max_interface * max_message * 1000 / elapse);


    zre_interface_destroy (&interface);
    for (nbr_interface = 0; nbr_interface < max_interface; nbr_interface++) {
        free (peers[nbr_interface]);
    }
    free (peers);
    return 0;
}
