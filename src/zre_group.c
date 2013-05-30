/*  =========================================================================
    zre_group - group known to this node

    -------------------------------------------------------------------------
    Copyright (c) 1991-2012 iMatix Corporation <www.imatix.com>
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

#include <czmq.h>
#include "../include/zre_internal.h"


//  ---------------------------------------------------------------------
//  Structure of our class

struct _zre_group_t {
    char *name;                 //  Group name
    zhash_t *peers;             //  Peers in group
};


//  Callback when we remove group from container

static void
s_delete_group (void *argument)
{
    zre_group_t *group = (zre_group_t *) argument;
    zre_group_destroy (&group);
}


//  ---------------------------------------------------------------------
//  Construct new group object

zre_group_t *
zre_group_new (char *name, zhash_t *container)
{
    zre_group_t *self = (zre_group_t *) zmalloc (sizeof (zre_group_t));
    self->name = strdup (name);
    self->peers = zhash_new ();
    
    //  Insert into container if requested
    if (container) {
        zhash_insert (container, name, self);
        zhash_freefn (container, name, s_delete_group);
    }
    return self;
}


//  ---------------------------------------------------------------------
//  Destroy group object

void
zre_group_destroy (zre_group_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zre_group_t *self = *self_p;
        zhash_destroy (&self->peers);
        free (self->name);
        free (self);
        *self_p = NULL;
    }
}


//  ---------------------------------------------------------------------
//  Add peer to group
//  Ignore duplicate joins

void
zre_group_join (zre_group_t *self, zre_peer_t *peer)
{
    assert (self);
    assert (peer);
    zhash_insert (self->peers, zre_peer_identity (peer), peer);
    zre_peer_set_status (peer, zre_peer_status (peer) + 1);
}


//  ---------------------------------------------------------------------
//  Remove peer from group

void
zre_group_leave (zre_group_t *self, zre_peer_t *peer)
{
    assert (self);
    assert (peer);
    zhash_delete (self->peers, zre_peer_identity (peer));
    zre_peer_set_status (peer, zre_peer_status (peer) + 1);
}


static int
s_peer_send (const char *key, void *item, void *argument)
{
    zre_peer_t *peer = (zre_peer_t *) item;
    zre_msg_t *msg = zre_msg_dup ((zre_msg_t *) argument);
    zre_peer_send (peer, &msg);
    return 0;
}

//  ---------------------------------------------------------------------
//  Send message to all peers in group

void
zre_group_send (zre_group_t *self, zre_msg_t **msg_p)
{
    assert (self);
    zhash_foreach (self->peers, s_peer_send, *msg_p);
    zre_msg_destroy (msg_p);
}
