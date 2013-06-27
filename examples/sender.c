/************** sender.c ************************************************/

#include <zre.h>

int main (int argc, char *argv [])
{
    if (argc < 2) {
        puts ("Syntax: sender filename virtualname");
        return 0;
    }
    printf ("Publishing %s as %s\n", argv [1], argv [2]);
    zre_node_t *node = zre_node_new ();
    zre_node_publish (node, argv [1], argv [2]);
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
