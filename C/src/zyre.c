#include <czmq.h>
#include "../include/zre.h"

int main (int argc, char *argv [])
{
    zre_interface_t *interface = zre_interface_new ();
    while (true) {
        zmsg_t *msg = zre_interface_recv (interface);
        if (!msg)
            break;              //  Interrupted
        zmsg_dump (msg);
    }
    zre_interface_destroy (&interface);
    return 0;
}
