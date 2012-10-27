#include <czmq.h>
#include "../include/zre.h"

#define MAX_GROUP 10

//  List of active interfaces
static zhash_t *interfaces;

static void
interface_task (void *args, zctx_t *ctx, void *pipe)
{
    zre_interface_t *interface = zre_interface_new (true);
    int64_t counter = 0;
    char *to_peer = NULL;        //  Either of these set,
    char *to_group = NULL;       //    and we set a message
    
    zmq_pollitem_t pollitems [] = {
        { pipe,                             0, ZMQ_POLLIN, 0 },
        { zre_interface_handle (interface), 0, ZMQ_POLLIN, 0 }
    };
    //  Do something once a second
    int64_t trigger = zclock_time () + 1000;
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
            }
            else
            if (streq (event, "EXIT")) {
                //  Always try talk to departed peer
                to_peer = zmsg_popstr (incoming);
            }
            else
            if (streq (event, "WHISPER")) {
                //  Send back response 1/2 the time
                if (randof (2) == 0)
                    to_peer = zmsg_popstr (incoming);
            }
            else
            if (streq (event, "SHOUT")) {
                to_peer = zmsg_popstr (incoming);
                to_group = zmsg_popstr (incoming);
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
            free (event);
            zmsg_destroy (&incoming);

            //  Send outgoing messages if needed
            if (to_peer) {
                zmsg_t *outgoing = zmsg_new ();
                zmsg_addstr (outgoing, to_peer);
                zmsg_addstr (outgoing, "%lu", counter++);
                zre_interface_whisper (interface, &outgoing);
                free (to_peer);
                to_peer = NULL;
            }
            if (to_group) {
                zmsg_t *outgoing = zmsg_new ();
                zmsg_addstr (outgoing, to_group);
                zmsg_addstr (outgoing, "%lu", counter++);
                zre_interface_shout (interface, &outgoing);
                free (to_group);
                to_group = NULL;
            }
        }
        if (zclock_time () >= trigger) {
            trigger = zclock_time () + 1000;
            char group [10];
            sprintf (group, "GROUP%03d", randof (MAX_GROUP));
            if (randof (4) == 0)
                zre_interface_join (interface, group);
            else
                zre_interface_leave (interface, group);
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
    int nbr_interfaces = 0;
    if (argc > 1)
        max_interface = atoi (argv [1]);

    //  We address interfaces as an array of pipes
    void **pipes = zmalloc (sizeof (void *) * max_interface);

    //  We will randomly start and stop interface threads
    while (!zctx_interrupted) {
        uint index = randof (max_interface);
        //  Toggle interface thread
        if (pipes [index]) {
            zstr_send (pipes [index], "STOP");
            zsocket_destroy (ctx, pipes [index]);
            pipes [index] = NULL;
            zclock_log ("I: Stopped interface (%d running)", --nbr_interfaces);
        }
        else {
            pipes [index] = zthread_fork (ctx, interface_task, NULL);
            zclock_log ("I: Started interface (%d running)", ++nbr_interfaces);
        }
        //  Sleep ~750 msecs randomly so we smooth out activity
        zclock_sleep (randof (500) + 500);
    }
    zctx_destroy (&ctx);
    return 0;
}
