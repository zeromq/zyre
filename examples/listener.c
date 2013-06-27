/************** listener.c **********************************************/

#include <zre.h>

int main (int argc, char *argv [])
{
    zre_node_t *node = zre_node_new ();
    while (true) {
        zmsg_t *incoming = zre_node_recv (node);
        if (!incoming)
            break;
        zmsg_dump (incoming);
        zmsg_destroy (&incoming);
    }
    zre_node_destroy (&node);
    return 0;
}
