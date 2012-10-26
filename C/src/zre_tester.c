#include <czmq.h>
#include "../include/zre.h"

#define INTEFACE_MODULO 10
#define GROUP_MODULO 5
#define MAX_GROUP 10
#define SLEEP 1
#define DUMMY "value"

//  List of active interfaces
zhash_t *interfaces;

static void
random_group_name (char *buffer, int size, int seed)
{
    assert (size >= 10);
    sprintf (buffer, "GROUP%04X", randof (seed));
}

static void *
zhash_key_at (zhash_t *table, int idx)
{
    assert (zhash_size (table) > idx);
    zlist_t *list = zhash_keys (table);
    void *value = zlist_first (list);
    while (idx-- > 0) {
        value = zlist_next (list);
    }
    return value;

}

static zmsg_t *
gen_msg (char *sender, char *target, char *prefix, uint64_t msg_id)
{
    zmsg_t *outgoing = zmsg_new ();
    zmsg_addstr (outgoing, target);
    zmsg_addstr (outgoing, "%s-%s-%lu", sender, prefix, msg_id);

    return outgoing;
}

static void *
interface_task (void *args)
{
    char *interface_id = (char *) args;
    uint64_t msg_id = 0;
    zre_interface_t *interface = zre_interface_new ();
    printf ("%s: started total %d\n", interface_id, (int) zhash_size (interfaces));

    while (zhash_lookup (interfaces, interface_id)) { // need a nicer way to stop/wake up interface

        if (randof (GROUP_MODULO) == 0) {
            // join a group
            char name [20];
            random_group_name (name, sizeof(name), MAX_GROUP);
            printf ("%s: JOIN group %s\n", interface_id, name);
            zre_interface_join (interface, name);

            zmsg_t *outgoing = gen_msg (interface_id, name, "S", msg_id++);
            zre_interface_shout (interface, &outgoing);
        }

        if (randof (GROUP_MODULO) == 1) {
            // leave a group
            char name [20];
            random_group_name (name, sizeof(name), MAX_GROUP);
            printf ("%s: LEAVE group %s\n", interface_id, name);
            zre_interface_leave (interface, name);
        }

        zmsg_t *incoming = zre_interface_recv (interface);
        if (!incoming) {
            printf ("%s: Interrupted\n", interface_id);
            break;              //  Interrupted
        }

        //  If new peer, say hello to it and wait for it to answer us
        char *event = zmsg_popstr (incoming);
        if (streq (event, "ENTER")) {
            char *peer = zmsg_popstr (incoming);
            printf ("%s: [%s] peer entered\n", interface_id, peer);

            zmsg_t *outgoing = gen_msg (interface_id, peer, "H", msg_id++);
            zre_interface_whisper (interface, &outgoing);
            free (peer);
        }
        else
        if (streq (event, "EXIT")) {
            char *peer = zmsg_popstr (incoming);
            printf ("%s: [%s] peer exited\n", interface_id, peer);
            free (peer);
        }
        else
        if (streq (event, "WHISPER")) {
            char *peer = zmsg_popstr (incoming);
            char name [10];
            random_group_name (name, sizeof(name), MAX_GROUP);
            printf ("%s: [%s] received cookies\n", interface_id, peer);

            if (randof(2) == 0) { // sometimes response back
                zmsg_t *outgoing = gen_msg (interface_id, peer, "W", msg_id++);
                zre_interface_whisper (interface, &outgoing);
            }
            free (peer);
        }
        else
        if (streq (event, "SHOUT")) {
            char *peer = zmsg_popstr (incoming);
            char *group = zmsg_popstr (incoming);
            printf ("%s: [%s](%s) received COOKIES\n", interface_id, peer, group);
            if (randof(3) == 0) { // sometimes response back
                zmsg_t *outgoing = gen_msg (interface_id, group, "S", msg_id++);
                zre_interface_shout (interface, &outgoing);
            }
            if (randof(3) == 1) { // sometimes send a whisper
                zmsg_t *outgoing = gen_msg (interface_id, peer, "W", msg_id++);
                zre_interface_whisper (interface, &outgoing);
            }

            free (peer);
            free (group);
        }
        free (event);
        zmsg_destroy (&incoming);

    }

    printf ("%s: stop interface total:%d\n", interface_id, (int) zhash_size (interfaces));
    zre_interface_destroy (&interface);
    free (interface_id);

    return NULL;
}

int main (int argc, char *argv [])
{
    interfaces =  zhash_new ();
    int max_interface = 100;
    if (argc > 1)
        max_interface = atoi (argv[1]);
    int iid = 0;
    int quotient = 0;

    while (iid < max_interface && !zctx_interrupted) {

        if (quotient == 0) { // new interface at startup
            // create new interface
            char interface_id [10];
            sprintf (interface_id, "IF-%04d", ++iid);
            zhash_insert (interfaces, interface_id, DUMMY);   // faster lookup
            zthread_new (interface_task, strdup (interface_id));
        }
        if (quotient == 1 && zhash_size (interfaces) > 0) {
            // destroy a random interface
            char *interface_id = zhash_key_at (interfaces, randof (zhash_size (interfaces)));
            // might need a nicer way to stop/wake up a interface
            zhash_delete (interfaces, interface_id);
        }
        sleep (1);
        if (iid > 3) // start 3 interface at least
            quotient = randof (INTEFACE_MODULO);
    }

    zhash_destroy (&interfaces);

    return 0;
}
