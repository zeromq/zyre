/*  =========================================================================
    pinger - ping other peers in a ZRE network

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
