/*  =========================================================================
    zpinger - ping other peers in a ZRE network

    -------------------------------------------------------------------------
    Copyright (c) the Contributors as noted in the AUTHORS file.

    This file is part of Zyre, an open-source framework for proximity-based
    peer-to-peer applications -- See http://zyre.org.

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
    =========================================================================
*/

/*
@header
    zpinger tells you what other nodes are running. Use this to debug network
    issues.
@discuss
@end
*/

#include <czmq.h>
#include "../include/zyre.h"

int main (int argc, char *argv [])
{
    zyre_t *node = zyre_new (NULL);
    zyre_start (node);
    zyre_join (node, "GLOBAL");

    while (true) {
        zmsg_t *incoming = zyre_recv (node);
        if (!incoming)
            break;              //  Interrupted

        //  If new peer, say hello to it and wait for it to answer us
        char *event = zmsg_popstr (incoming);
        char *peer = zmsg_popstr (incoming);
        char *name = zmsg_popstr (incoming);
        if (streq (event, "ENTER")) {
            printf ("I: [%s] peer entered\n", name);
            zyre_whispers (node, peer, "Hello");
        }
        else
        if (streq (event, "EXIT")) {
            printf ("I: [%s] peer exited\n", name);
        }
        else
        if (streq (event, "WHISPER")) {
            printf ("I: [%s] received ping (WHISPER)\n", name);
            zyre_shouts (node, "GLOBAL", "Hello");
        }
        else
        if (streq (event, "SHOUT")) {
            char *group = zmsg_popstr (incoming);
            printf ("I: [%s](%s) received ping (SHOUT)\n", name, group);
            free (group);
        }
        free (event);
        free (peer);
        free (name);
        zmsg_destroy (&incoming);
    }
    zyre_destroy (&node);
    return 0;
}
