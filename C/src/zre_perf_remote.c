#include <czmq.h>
#include "../include/zre.h"

// to test performances, run zre_perf_remote [interface_count] first then zre_perf_local

#define MAX_GROUP 10

static void
interface_task (void *args, zctx_t *ctx, void *pipe)
{
    zre_interface_t *interface = zre_interface_new (false);
    int64_t counter = 0;
    char *to_peer = NULL;        //  Either of these set,
    char *to_group = NULL;       //    and we set a message
    char *cookie = NULL;         //  received message
    char *sending_cookie = NULL; //  sending message
    
    zmq_pollitem_t pollitems [] = {
        { pipe,                             0, ZMQ_POLLIN, 0 },
        { zre_interface_handle (interface), 0, ZMQ_POLLIN, 0 }
    };

    // all interface joins GLOBAL
    zre_interface_join (interface, "GLOBAL");

    while (!zctx_interrupted) {
        if (zmq_poll (pollitems, 2, randof (1000) * ZMQ_POLL_MSEC) == -1)
            break;              //  Interrupted

        if (pollitems [0].revents & ZMQ_POLLIN)
            break;              //  Any command from parent means EXIT

        //  Process an event from interface
        if (pollitems [1].revents & ZMQ_POLLIN) {
            zmsg_t *incoming = zre_interface_recv (interface);
            if (!incoming)
                break;              //  Interrupted

            char *event = zmsg_popstr (incoming);
            if (streq (event, "ENTER")) {
                //  Always say hello to new peer
                to_peer = zmsg_popstr (incoming);
                sending_cookie = "R:HELLO";
            }
            else
            if (streq (event, "EXIT")) {
                //  Do nothing
            }
            else
            if (streq (event, "WHISPER")) {
                to_peer = zmsg_popstr (incoming);
                cookie = zmsg_popstr (incoming);

                // if a message comes from zre_perf_local, send back a special response
                if (streq (cookie, "S:WHISPER")) {
                    sending_cookie = "R:WHISPER";
                }
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

                // if a message comes from zre_perf_local, send back a special response
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
                zmsg_t *outgoing = zmsg_new ();
                zmsg_addstr (outgoing, to_peer);
                zmsg_addstr (outgoing, sending_cookie);
                zre_interface_whisper (interface, &outgoing);
                free (to_peer);
                to_peer = NULL;
            }
            if (to_group) {
                zmsg_t *outgoing = zmsg_new ();
                zmsg_addstr (outgoing, to_group);
                zmsg_addstr (outgoing, sending_cookie);
                zre_interface_shout (interface, &outgoing);
                free (to_group);
                to_group = NULL;
            }
            if (cookie) {
                free (cookie);
                cookie = NULL;
            }
        }
    }
    zre_interface_destroy (&interface);
}


int main (int argc, char *argv [])
{
    //  Initialize context for talking to tasks
    zctx_t *ctx = zctx_new ();
    zctx_set_linger (ctx, 100);
    
    //  Get number of interfaces to simulate, default 100
    int max_interface = 100;
    int nbr_interface = 0;
    if (argc > 1)
        max_interface = atoi (argv [1]);

    //  We address interfaces as an array of pipes
    void **pipes = zmalloc (sizeof (void *) * max_interface);

    for (nbr_interface = 0; nbr_interface < max_interface; nbr_interface++) {
        pipes [nbr_interface] = zthread_fork (ctx, interface_task, NULL);
        zclock_log ("I: Started interface (%d running)", nbr_interface + 1);
    }
    //  We will randomly start and stop interface threads
    while (!zctx_interrupted) {
        zclock_sleep (1000);
    }
    zclock_log ("I: Stopped perl_remote");
    zctx_destroy (&ctx);
    free (pipes);
    return 0;
}
