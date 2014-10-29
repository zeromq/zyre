/*  =========================================================================
    zyre_group - group known to this node

    -------------------------------------------------------------------------
    Copyright (c) 1991-2014 iMatix Corporation <www.imatix.com>
    Copyright other contributors as noted in the AUTHORS file.

    This file is part of Zyre, an open-source framework for proximity-based
    peer-to-peer applications -- See http://zyre.org.

    This is free software; you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 3 of the License, or (at
    your option) any later version.

    This software is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this program. If not, see
    <http://www.gnu.org/licenses/>.
    =========================================================================
*/

#include "../include/zyre.h"
#include "zyre_classes.h"

//  --------------------------------------------------------------------------
//  Structure of our class

struct _zyre_group_t {
    char *name;                 //  Group name
    zhash_t *peers;             //  Peers in group
};


//  Callback when we remove group from container

static void
s_delete_group (void *argument)
{
    zyre_group_t *group = (zyre_group_t *) argument;
    zyre_group_destroy (&group);
}


//  --------------------------------------------------------------------------
//  Construct new group object

zyre_group_t *
zyre_group_new (const char *name, zhash_t *container)
{
    zyre_group_t *self = (zyre_group_t *) zmalloc (sizeof (zyre_group_t));
    self->name = strdup (name);
    self->peers = zhash_new ();
    
    //  Insert into container if requested
    if (container) {
        zhash_insert (container, (void *) name, self);
        zhash_freefn (container, (void *) name, s_delete_group);
    }
    return self;
}


//  --------------------------------------------------------------------------
//  Destroy group object

void
zyre_group_destroy (zyre_group_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zyre_group_t *self = *self_p;
        zhash_destroy (&self->peers);
        free (self->name);
        free (self);
        *self_p = NULL;
    }
}


//  --------------------------------------------------------------------------
//  Add peer to group
//  Ignore duplicate joins

void
zyre_group_join (zyre_group_t *self, zyre_peer_t *peer)
{
    assert (self);
    assert (peer);
    zhash_insert (self->peers, zyre_peer_identity (peer), peer);
    zyre_peer_set_status (peer, zyre_peer_status (peer) + 1);
}


//  --------------------------------------------------------------------------
//  Remove peer from group

void
zyre_group_leave (zyre_group_t *self, zyre_peer_t *peer)
{
    assert (self);
    assert (peer);
    zhash_delete (self->peers, zyre_peer_identity (peer));
    zyre_peer_set_status (peer, zyre_peer_status (peer) + 1);
}


static int
s_peer_send (const char *key, void *item, void *argument)
{
    zyre_peer_t *peer = (zyre_peer_t *) item;
    zre_msg_t *msg = zre_msg_dup ((zre_msg_t *) argument);
    zyre_peer_send (peer, &msg);
    return 0;
}

//  --------------------------------------------------------------------------
//  Send message to all peers in group

void
zyre_group_send (zyre_group_t *self, zre_msg_t **msg_p)
{
    assert (self);
    zhash_foreach (self->peers, (zhash_foreach_fn *) s_peer_send, *msg_p);
    zre_msg_destroy (msg_p);
}


//  --------------------------------------------------------------------------
//  Self test of this class

void
zyre_group_test (bool verbose)
{
    printf (" * zyre_group: ");
    zsock_t *mailbox = zsock_new (ZMQ_DEALER);
    zsock_bind (mailbox, "tcp://127.0.0.1:5552");

    zhash_t *groups = zhash_new ();
    zyre_group_t *group = zyre_group_new ("tests", groups);

    zhash_t *peers = zhash_new ();
    zuuid_t *you = zuuid_new ();
    zuuid_t *me = zuuid_new ();
    zyre_peer_t *peer = zyre_peer_new (peers, you);
    assert (!zyre_peer_connected (peer));
    zyre_peer_connect (peer, me, "tcp://127.0.0.1:5552");
    assert (zyre_peer_connected (peer));

    zyre_group_join (group, peer);
    
    zre_msg_t *msg = zre_msg_new (ZRE_MSG_HELLO);
    zre_msg_set_endpoint (msg, "tcp://127.0.0.1:5552");
    zyre_group_send (group, &msg);

    msg = zre_msg_recv (mailbox);
    assert (msg);
    if (verbose)
        zre_msg_print (msg);
    zre_msg_destroy (&msg);

    zuuid_destroy (&me);
    zuuid_destroy (&you);
    zhash_destroy (&peers);
    zhash_destroy (&groups);
    zsock_destroy (&mailbox);
    printf ("OK\n");
}

