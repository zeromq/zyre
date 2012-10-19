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
        if (streq (event, "READY")) {
            char *peer = zmsg_popstr (incoming);
            printf ("I: [%s] new peer\n", peer);
            zmsg_t *outgoing = zmsg_new ();
            zmsg_addstr (outgoing, peer);
            zmsg_addstr (outgoing, "Hello");
            zre_interface_sendto (interface, &outgoing);
            free (peer);
        }
        else
        if (streq (event, "FROM")) {
            char *peer = zmsg_popstr (incoming);
            printf ("I: [%s] received OHAI\n", peer);
            free (peer);
        }
        else
        if (streq (event, "EXPIRED")) {
            char *peer = zmsg_popstr (incoming);
            printf ("I: [%s] peer expired\n", peer);
            free (peer);
        }
        free (event);
        zmsg_destroy (&incoming);
    }
    zre_interface_destroy (&interface);
    return 0;
}
