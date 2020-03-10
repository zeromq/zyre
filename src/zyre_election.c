/*  =========================================================================
    zyre_election - class description

    Copyright (c) the Contributors as noted in the AUTHORS file.

    This file is part of Zyre, an open-source framework for proximity-based
    peer-to-peer applications -- See http://zyre.org.

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
    =========================================================================
*/

/*
@header
    zyre_election - Holds an election with all connected peer. The peer with the
                    lowest ID will win.
@discuss
@end
*/

#include "zyre_classes.h"

//  Structure of our class

struct _zyre_election_t {
    char *caw;              //  Current active wave
    zyre_peer_t *father;    //  Father in the current active wave
    unsigned int erec;      //  Number of received election messages
    unsigned int lrec;      //  Number of received leader messages
    bool state;             //  True if leader else false

    char *leader;           //  Leader identity
};


//  --------------------------------------------------------------------------
//  Create a new zyre_election

zyre_election_t *
zyre_election_new ()
{
    zyre_election_t *self = (zyre_election_t *) zmalloc (sizeof (zyre_election_t));
    assert (self);
    //  Initialize class properties here
    self->caw = NULL;
    self->father = NULL;
    self->erec = 0;
    self->lrec = 0;
    self->state = false;

    self->leader = NULL;
    return self;
}


//  --------------------------------------------------------------------------
//  Destroy the zyre_election

void
zyre_election_destroy (zyre_election_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zyre_election_t *self = *self_p;
        //  Free class properties here
        zstr_free (&self->caw);
        zstr_free (&self->leader);
        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}


bool
zyre_election_challenger_superior (zyre_election_t *self, const char *r) {
    assert (self);
    assert (r);
    return !self->caw || strcmp (r, self->caw) < 0;
}


void
zyre_election_reset (zyre_election_t *self)
{
    assert (self);
    zstr_free (&self->caw);     //  Free caw when re-initiated
    zstr_free (&self->leader);  //  Free leader when re-initiated
    self->father = NULL;        //  Reset father when re-initiated
    self->erec = 0;
    self->lrec = 0;
}


void
zyre_election_set_caw (zyre_election_t *self, char *caw)
{
    assert (self);
    self->caw = caw;
}


void
zyre_election_set_father (zyre_election_t *self, zyre_peer_t *father)
{
    assert (self);
    self->father = father;
}


zyre_peer_t *
zyre_election_father (zyre_election_t *self)
{
    assert (self);
    return self->father;
}


zre_msg_t *
zyre_election_build_elect_msg (zyre_election_t *self)
{
    assert (self);
    zre_msg_t *election_msg = zre_msg_new ();
    zre_msg_set_id (election_msg, ZRE_MSG_ELECT);
    zre_msg_set_challenger_id (election_msg, self->caw);
    return election_msg;
}


zre_msg_t *
zyre_election_build_leader_msg (zyre_election_t *self)
{
    assert (self);
    assert (self->caw);
    zre_msg_t *election_msg = zre_msg_new ();
    zre_msg_set_id (election_msg, ZRE_MSG_LEADER);
    zre_msg_set_leader_id (election_msg, self->caw);
    return election_msg;
}


bool
zyre_election_supporting_challenger (zyre_election_t *self, const char *r)
{
    assert (self);
    assert (self->caw);
    assert (r);
    return strcmp (self->caw, r) == 0;
}


const char *
zyre_election_caw (zyre_election_t *self)
{
    assert (self);
    return self->caw;
}


void
zyre_election_increment_erec (zyre_election_t *self)
{
    assert (self);
    self->erec++;
}


void
zyre_election_increment_lrec (zyre_election_t *self)
{
    assert (self);
    self->lrec++;
}


bool
zyre_election_erec_complete (zyre_election_t *self, zyre_group_t *group)
{
    assert (self);
    zlist_t *neighbors = zyre_group_peers (group);
    bool complete = self->erec == zlist_size (neighbors);
    zlist_destroy (&neighbors);
    return complete;
}


bool
zyre_election_lrec_started (zyre_election_t *self)
{
    assert (self);
    return self->lrec > 0;
}


bool
zyre_election_lrec_complete (zyre_election_t *self, zyre_group_t *group)
{
    assert (self);
    zlist_t *neighbors = zyre_group_peers (group);
    bool complete = self->lrec == zlist_size (neighbors);
    zlist_destroy (&neighbors);
    return complete;
}


//  --------------------------------------------------------------------------
//  Sets the leader if an election is finished, otherwise NULL.

void
zyre_election_set_leader (zyre_election_t *self, char *leader)
{
    assert (self);
    zstr_free (&self->leader);
    self->leader = leader;
}


//  --------------------------------------------------------------------------
//  Returns the leader if an election is finished, otherwise NULL.

const char *
zyre_election_leader (zyre_election_t *self)
{
    assert (self);
    return self->leader;
}


//  --------------------------------------------------------------------------
//  Returns true if an election is won, otherwise false.

bool
zyre_election_won (zyre_election_t *self)
{
    assert (self);
    return self->leader? self->state: false;
}

//  --------------------------------------------------------------------------
//  Returns true if an election is finished, otherwise false.

bool
zyre_election_finished (zyre_election_t *self)
{
    assert (self);
    return !self->caw && self->leader;
}


//  --------------------------------------------------------------------------
//  Print election status to command line

void
zyre_election_print (zyre_election_t *self) {
    printf ("zyre_election : {\n");
    printf ("    father: %s\n", zyre_peer_name (self->father));
    printf ("    CAW: %s\n", self->caw);
    printf ("    election count: %d\n", self->erec);
    printf ("    leader count: %d\n", self->lrec);
    printf ("    state: %s\n", !self->leader? "undecided": self->state? "leader": "looser");
    printf ("    leader: %s\n", self->leader);
    printf ("}\n");
}


//  --------------------------------------------------------------------------
//  Self test of this class

// If your selftest reads SCMed fixture data, please keep it in
// src/selftest-ro; if your test creates filesystem objects, please
// do so under src/selftest-rw.
// The following pattern is suggested for C selftest code:
//    char *filename = NULL;
//    filename = zsys_sprintf ("%s/%s", SELFTEST_DIR_RO, "mytemplate.file");
//    assert (filename);
//    ... use the "filename" for I/O ...
//    zstr_free (&filename);
// This way the same "filename" variable can be reused for many subtests.
#define SELFTEST_DIR_RO "src/selftest-ro"
#define SELFTEST_DIR_RW "src/selftest-rw"

void
zyre_election_test (bool verbose)
{
    printf (" * zyre_election: ");

// Test runs unreliable on windows CI so skip it for now
#if !defined (__WINDOWS__)
    //  @selftest
    int rc;
    //  Init zyre nodes
    zyre_t *node1 = zyre_new ("node1");
    assert (node1);
    if (verbose)
        zyre_set_verbose (node1);

    //  Set inproc endpoint for this node
    rc = zyre_set_endpoint (node1, "inproc://zyre-node1");
    assert (rc == 0);
    //  Set up gossip network for this node
    zyre_gossip_bind (node1, "inproc://gossip-hub");
    rc = zyre_start (node1);
    assert (rc == 0);

    zyre_t *node2 = zyre_new ("node2");
    assert (node2);
    if (verbose)
        zyre_set_verbose (node2);

    //  Set inproc endpoint for this node
    rc = zyre_set_endpoint (node2, "inproc://zyre-node2");
    assert (rc == 0);
    //  Set up gossip network for this node
    zyre_gossip_connect (node2, "inproc://gossip-hub");
    rc = zyre_start (node2);
    assert (rc == 0);

    //  Give time for them to interconnect
    zclock_sleep (250);

    //  Join topology
    zyre_set_contest_in_group (node1, "GROUP_1");
    zyre_set_contest_in_group (node2, "GROUP_1");
    zyre_join (node1, "GROUP_1");
    zyre_join (node2, "GROUP_1");

    zyre_set_contest_in_group (node2, "GROUP_2");
    zyre_join (node1, "GROUP_2");
    zyre_join (node2, "GROUP_2");

    zyre_join (node1, "GROUP_3");
    zyre_join (node2, "GROUP_3");

    //  Check election results
    int num_of_global_leaders = 0;
    int num_of_global1_leaders = 0;
    int num_of_leader_messages = 0;

    zyre_event_t *event;
    do {
        //  Recv from node1
        event = zyre_event_new (node1);
        if (streq (zyre_event_type (event), "LEADER")) {
            if (streq (zyre_event_group (event), "GROUP_1")) {
                num_of_leader_messages++;
                if (streq (zyre_uuid (node1), zyre_event_peer_uuid (event)))
                    num_of_global_leaders++;
            }
            else
            if (streq (zyre_event_group (event), "GROUP_2")) {
                num_of_leader_messages++;
                if (streq (zyre_uuid (node1), zyre_event_peer_uuid (event)))
                    num_of_global1_leaders++;
            }
            else
            if (streq (zyre_event_group (event), "GROUP_3"))
                assert (false);
        }
        zyre_event_destroy (&event);

        //  Recv from node2
        event = zyre_event_new (node2);
        if (streq (zyre_event_type (event), "LEADER")) {
            if (streq (zyre_event_group (event), "GROUP_1")) {
                num_of_leader_messages++;
                if (streq (zyre_uuid (node2), zyre_event_peer_uuid (event)))
                    num_of_global_leaders++;
            }
            else
            if (streq (zyre_event_group (event), "GROUP_2")) {
                num_of_leader_messages++;
                if (streq (zyre_uuid (node2), zyre_event_peer_uuid (event)))
                    num_of_global1_leaders++;
            }
            else
            if (streq (zyre_event_group (event), "GROUP_3"))
                assert (false);
        }
        zyre_event_destroy (&event);
    } while (num_of_leader_messages < 4);

    assert (num_of_global_leaders == 1);
    assert (num_of_global1_leaders == 1);

    //  @TODO: Test leaving leader

    zyre_stop (node1);
    zyre_stop (node2);

    zyre_destroy (&node1);
    zyre_destroy (&node2);
#endif
#if defined (__WINDOWS__)
    zsys_shutdown();
#endif
    //  @end
    printf ("OK\n");
}
