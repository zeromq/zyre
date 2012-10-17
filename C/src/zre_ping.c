#include <czmq.h>
#include "../include/zre.h"

int main (int argc, char *argv [])
{
    zre_interface_t *interface = zre_interface_new ();
    while (true) {
        zmsg_t *incoming = zre_interface_recv (interface);
        if (!incoming)
            break;              //  Interrupted
        zmsg_dump (incoming);

        //  If new peer, say hello to it and wait for it to answer us
        char *event = zmsg_popstr (incoming);
        if (streq (event, "JOINED")) {
            //  Peer address is next frame in message
            zframe_t *peer = zmsg_pop (incoming);
            //  Send message to peer
            zmsg_t *outgoing = zmsg_new ();
            zmsg_addstr (outgoing, "Hello");
            zre_interface_send (interface, peer, &outgoing);
        }
        zmsg_destroy (&incoming);
    }
    zre_interface_destroy (&interface);
    return 0;
}
