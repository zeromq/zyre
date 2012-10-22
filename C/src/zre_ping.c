#include <czmq.h>
#include "../include/zre.h"

int main (int argc, char *argv [])
{
    zre_interface_t *interface = zre_interface_new ();
    while (true) {
        zmsg_t *incoming = zre_interface_recv (interface);
        if (!incoming)
            break;              //  Interrupted

        //  If new peer, say hello to it and wait for it to answer us
        char *event = zmsg_popstr (incoming);
        if (streq (event, "ENTER")) {
            char *peer = zmsg_popstr (incoming);
            printf ("I: [%s] peer entered\n", peer);
            zmsg_t *outgoing = zmsg_new ();
            zmsg_addstr (outgoing, peer);
            zmsg_addstr (outgoing, "Hello");
            zre_interface_whisper (interface, &outgoing);
            free (peer);
        }
        else
        if (streq (event, "EXIT")) {
            char *peer = zmsg_popstr (incoming);
            printf ("I: [%s] peer exited\n", peer);
            free (peer);
        }
        else
        if (streq (event, "FROM")) {
            char *peer = zmsg_popstr (incoming);
            printf ("I: [%s] received cookies\n", peer);
            free (peer);
            zre_interface_join (interface, "GLOBAL");
            zre_interface_join (interface, "MASHUP");
            zre_interface_join (interface, "CANVAS");
            zre_interface_join (interface, "ZEROMQ");
            zre_interface_leave (interface, "GLOBAL");
            zre_interface_leave (interface, "MASHUP");
            zre_interface_leave (interface, "CANVAS");
            zre_interface_leave (interface, "ZEROMQ");
        }
        free (event);
        zmsg_destroy (&incoming);
    }
    zre_interface_destroy (&interface);
    return 0;
}
