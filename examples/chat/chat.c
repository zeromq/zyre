//  --------------------------------------------------------------------------
//  Example Zyre distributed chat application
//
//  --------------------------------------------------------------------------
//  Copyright (c) 2010-2014 iMatix Corporation and Contributors
//
//  Permission is hereby granted, free of charge, to any person obtaining a
//  copy of this software and associated documentation files (the "Software"),
//  to deal in the Software without restriction, including without limitation
//  the rights to use, copy, modify, merge, publish, distribute, sublicense,
//  and/or sell copies of the Software, and to permit persons to whom the
//  Software is furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
//  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//  DEALINGS IN THE SOFTWARE.
//  --------------------------------------------------------------------------


#include "zyre.h"

//  This actor will listen and publish anything received
//  on the CHAT group

static void 
chat_actor (zsock_t *pipe, void *args)
{
    //  Do some initialization
    char*   name = (char*) args;
    zyre_t *node = zyre_new (name);
    if (!node)
        return;                 //  Could not create new node
    //zyre_set_verbose (node);  // uncomment to watch the events
    zyre_start (node);
    zyre_join (node, "CHAT");
    zsock_signal (pipe, 0);     //  Signal "ready" to caller

    bool terminated = false;
    zpoller_t *poller = zpoller_new (pipe, zyre_socket (node), NULL);
    while (!terminated) {
        void *which = zpoller_wait (poller, -1); // no timeout
        if (which == pipe){
            zmsg_t *msg = zmsg_recv (which);
            if (!msg)
                break;              //  Interrupted
            char *command = zmsg_popstr (msg);
            if (streq (command, "$TERM")) {
                terminated = true;
            }
            else
            if (streq (command, "SHOUT")) { 
                char *string = zmsg_popstr (msg);
                zyre_shouts (node, "CHAT", "%s", string);
            }
            else {
                puts ("E: invalid message to actor");
                assert (false);
            }
            free (command);
            zmsg_destroy (&msg);
        }
        else if (which == zyre_socket (node)) {
            zmsg_t *msg = zmsg_recv (which);
            char *event = zmsg_popstr (msg);
            char *peer = zmsg_popstr (msg);
            char *name = zmsg_popstr (msg);
            char *group = zmsg_popstr (msg);
            char *message = zmsg_popstr (msg);

            if (streq (event, "ENTER")) { 
                printf ("%s has joined the chat\n", name);
            }
            else if (streq (event, "EXIT")) { 
                printf ("%s has left the chat\n", name);
            }
            if (streq (event, "SHOUT")) { 
                printf ("%s: %s\n", name, message);
            }
            //printf ("Message from node\n");
            //printf ("event: %s peer: %s  name: %s\n  group: %s message: %s\n", event, peer, name, group, message);

            free (event);
            free (peer);
            free (name);
            free (group);
            free (message);
            zmsg_destroy (&msg);
        }
    }
    zpoller_destroy (&poller);

    // Notify peers that this peer is shutting down. Provide
    // a brief interval to ensure message is emitted.
    zyre_stop(node);
    zclock_sleep(100);

    zyre_destroy (&node);
}



int
main (int argc, char *argv[])
{
    if (argc < 2) {
        puts ("syntax: ./chat myname");
        exit (0);
    }
    zactor_t *actor = zactor_new (chat_actor, argv[1]);
    assert (actor);
    
    while (!zsys_interrupted) {
        char message [1024];
        if (!fgets( message, 1024, stdin))
	        break;
	    message[strlen(message)-1] = 0; // drop the trailing linefeed
	    zstr_sendx (actor, "SHOUT", message, NULL);
    }

    zactor_destroy (&actor);

    return 0;
}


