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
zpinger tells you what other Zyre nodes are running. Use this to debug
network issues. Run with -v option to get more detail on Zyre's internal
flow for each event.
@discuss
Note that this will detect and speak to Zyre nodes on the entire local
network.
@end
*/

#include <czmq.h>
#include "zyre_classes.h"

int main (int argc, char *argv [])
{
    bool verbose = false;
    int argn;
    for (argn = 1; argn < argc; argn++) {
        if (streq (argv [argn], "--help")
        ||  streq (argv [argn], "-h")) {
            puts ("zpinger [options] ...");
            puts ("  --verbose / -v         verbose test output");
            puts ("  --help / -h            this help");
            return 0;
        }
        if (streq (argv [argn], "--verbose")
        ||  streq (argv [argn], "-v"))
            verbose = true;
        else {
            printf ("Unknown option: %s\n", argv [argn]);
            return 1;
        }
    }
    zyre_t *zyre = zyre_new (NULL);
    zsys_info ("Create Zyre node, uuid=%s, name=%s", zyre_uuid (zyre), zyre_name (zyre));
    if (verbose)
        zyre_set_verbose (zyre);
    zyre_start (zyre);
    zyre_join (zyre, "GLOBAL");
    if (verbose)
        zyre_print (zyre);

    while (true) {
        zyre_event_t *event = zyre_event_new (zyre);
        if (!event)
            break;              //  Interrupted
        if (verbose)
            zyre_event_print (event);

        if (zyre_event_type (event) == ZYRE_EVENT_ENTER) {
            //  If new peer, say hello to it and wait for it to answer us
            zsys_info ("[%s] peer entered", zyre_event_peer_name (event));
            zyre_whispers (zyre, zyre_event_peer_uuid (event), "Hello");
        }
        else
        if (zyre_event_type (event) == ZYRE_EVENT_EXIT) {
            zsys_info ("[%s] peer exited", zyre_event_peer_name (event));
        }
        else
        if (zyre_event_type (event) == ZYRE_EVENT_WHISPER) {
            zsys_info ("[%s] received ping (WHISPER)", zyre_event_peer_name (event));
            zyre_shouts (zyre, "GLOBAL", "Hello");
        }
        else
        if (zyre_event_type (event) == ZYRE_EVENT_SHOUT) {
            zsys_info ("[%s](%s) received ping (SHOUT)",
                       zyre_event_peer_name (event), zyre_event_group (event));
        }
        zyre_event_destroy (&event);
    }
    zyre_destroy (&zyre);
    return 0;
}
