/*  =========================================================================
    zyre_group - group known to this node

    -------------------------------------------------------------------------
    Copyright (c) the Contributors as noted in the AUTHORS file.

    This file is part of Zyre, an open-source framework for proximity-based
    peer-to-peer applications -- See http://zyre.org.

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
    =========================================================================
*/

#include "zyre_classes.h"

//  --------------------------------------------------------------------------
//  Structure of our class

struct _zyre_group_t {
    char *name;                 //  Group name
    zhash_t *peers;             //  Peers in group
//  DRAFT-API: Election
    bool contest;               //  Wheather the peer actively contest for leadership of this group
    zyre_peer_t *leader;        //  Peer that has been elected as leader for this group
    zyre_election_t *election;  //  Election handler, is NULL if there's no active election
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
    self->contest = false;

    //  Insert into container if requested
    if (container) {
        zhash_insert (container, name, self);
        zhash_freefn (container, name, s_delete_group);
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
        zyre_election_destroy (&self->election);
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
    void *item;
    assert (self);
    for (item = zhash_first (self->peers); item != NULL;
            item = zhash_next (self->peers))
        s_peer_send (zhash_cursor (self->peers), item, *msg_p);
    zre_msg_destroy (msg_p);
}


//  --------------------------------------------------------------------------
//  Return zlist of peer ids currently in this group
//  Caller owns return value and must destroy it when done.

zlist_t *
zyre_group_peers (zyre_group_t *self)
{
    return zhash_keys (self->peers);
}

//  --------------------------------------------------------------------------
//  Find or create an election for a group

zyre_election_t *
zyre_group_require_election (zyre_group_t *self)
{
    assert (self);
    if (!self->election)
        self->election = zyre_election_new ();

    return self->election;
}

//  --------------------------------------------------------------------------
//  Enables peer to actively contest for leadership in this group.

void
zyre_group_set_contest (zyre_group_t *self) {
    assert (self);
    self->contest = true;
}

//  --------------------------------------------------------------------------
//  Returns true if this peer actively contests for leadership, otherwise
//  false.

bool
zyre_group_contest (zyre_group_t *self) {
    assert (self);
    return self->contest;
}

//  --------------------------------------------------------------------------
//  Return the election handler for this group.

zyre_election_t *
zyre_group_election (zyre_group_t *self) {
    assert (self);
    return self->election;
}


//  --------------------------------------------------------------------------
//  Sets the election handler for this group.

void
zyre_group_set_election (zyre_group_t *self, zyre_election_t *election) {
    assert (self);
    self->election = election;
}


//  --------------------------------------------------------------------------
//  Return the peer that has been elected leader of this group.

zyre_peer_t *
zyre_group_leader (zyre_group_t *self) {
    assert (self);
    return self->leader;
}


//  --------------------------------------------------------------------------
//  Sets the peer that has been elected leader of this group.

void
zyre_group_set_leader (zyre_group_t *self, zyre_peer_t *leader) {
    assert (self);
    self->leader = leader;
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
    assert (!zyre_peer_connect (peer, me, "tcp://127.0.0.1:5552", 30000));
    assert (zyre_peer_connected (peer));

    zyre_group_join (group, peer);

    zre_msg_t *msg = zre_msg_new ();
    zre_msg_set_id (msg, ZRE_MSG_HELLO);
    zre_msg_set_endpoint (msg, "tcp://127.0.0.1:5552");
    zyre_group_send (group, &msg);

    msg = zre_msg_new ();
    int rc = zre_msg_recv (msg, mailbox);
    assert (rc == 0);
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

