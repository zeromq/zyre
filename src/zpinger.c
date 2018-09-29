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

#include "platform.h"
#include "zyre.h"

int main (int argc, char *argv [])
{
    bool verbose = false;
    char *iface = NULL;
    int ipv6 = 0;
    int curve = 0;
    int argn;
    for (argn = 1; argn < argc; argn++) {
        if (streq (argv [argn], "--help")
        ||  streq (argv [argn], "-h")) {
            puts ("zpinger [options] ...");
            puts ("  --help / -h            this help");
            puts ("  --verbose / -v         verbose test output");
            puts ("  --interface / -i       use this interface");
            puts ("  --ipv6 / -6            use IPv6");
            puts ("  --curve / -c           use CURVE encryption");
            return 0;
        }
        if (streq (argv [argn], "--verbose")
        ||  streq (argv [argn], "-v"))
            verbose = true;
        else
        if (streq (argv [argn], "--interface")
        ||  streq (argv [argn], "-i"))
            iface = argv [++argn];
        else
        if (streq (argv [argn], "--ipv6")
        ||  streq (argv [argn], "-6"))
            ipv6 = 1;
#ifdef ZYRE_BUILD_DRAFT_API
        //  DRAFT-API: Security
        else
        if (streq (argv [argn], "--curve")
        ||  streq (argv [argn], "-c")) {
            if (!zsys_has_curve ()) {
                printf ("CURVE requested but CZMQ/libzmq do not support it");
                return 1;
            }
            curve = 1;
        }
#endif
        else {
            printf ("Unknown option: %s\n", argv [argn]);
            return 1;
        }
    }
    zsys_set_ipv6(ipv6);
    zyre_t *zyre = zyre_new (NULL);
    zsys_info ("Create Zyre node, uuid=%s, name=%s", zyre_uuid (zyre), zyre_name (zyre));
    if (verbose)
        zyre_set_verbose (zyre);
    if (iface)
        zyre_set_interface (zyre, iface);
    zactor_t *auth = NULL;
#ifdef ZYRE_BUILD_DRAFT_API
    //  DRAFT-API: Security
    if (curve && zsys_has_curve()) {
        // zap setup
        auth = zactor_new(zauth, NULL);
        assert (auth);
        if (verbose) {
            zstr_sendx(auth, "VERBOSE", NULL);
            zsock_wait(auth);
        }
        zstr_sendx (auth, "CURVE", CURVE_ALLOW_ANY, NULL);
        zsock_wait (auth);

        zyre_set_zap_domain(zyre, "ZPINGER");
        zcert_t *cert = zcert_new ();
        assert (cert);
        zyre_set_zcert (zyre, cert);
        zyre_set_header (zyre, "X-PUBLICKEY", "%s", zcert_public_txt (cert));
    }
#else
    (void)curve; //  Avoid unused variable error
#endif
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

        if (streq (zyre_event_type (event), "ENTER")) {
            //  If new peer, say hello to it and wait for it to answer us
            zsys_info ("[%s] peer entered", zyre_event_peer_name (event));
            zyre_whispers (zyre, zyre_event_peer_uuid (event), "Hello");
        }
        else
        if (streq (zyre_event_type (event), "EXIT")) {
            zsys_info ("[%s] peer exited", zyre_event_peer_name (event));
        }
        else
        if (streq (zyre_event_type (event), "WHISPER")) {
            zsys_info ("[%s] received ping (WHISPER)", zyre_event_peer_name (event));
            zyre_shouts (zyre, "GLOBAL", "Hello");
        }
        else
        if (streq (zyre_event_type (event), "SHOUT")) {
            zsys_info ("[%s](%s) received ping (SHOUT)",
                       zyre_event_peer_name (event), zyre_event_group (event));
        }
        zyre_event_destroy (&event);
    }
    zyre_destroy (&zyre);
    zactor_destroy (&auth);
    return 0;
}
