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


//  Local helper functions

/*
static unsigned long
s_neighbors_count (zyre_election_t *self)
{
    assert (self);
    unsigned long neighbors_count = 0;
    zlist_t *peer_ids = zyre_group_peers (self->group);
    neighbors_count = zlist_size (peer_ids);
    zlist_destroy (&peer_ids);
    return neighbors_count;
}


static zlist_t *
s_neighbors (zyre_election_t *self, bool with_father)
{
    assert (self);
    zlist_t *neighbors_ids = zyre_group_peers (self->group);
    zlist_comparefn (neighbors_ids, (zlist_compare_fn *) strcmp);

    if (with_father) {
        while (neighbors_ids && zlist_size (neighbors_ids) > 0) {
            char *neighbor = (char *) zlist_pop (neighbors_ids);
            if (streq (neighbor, zyre_peer_identity (self->father)))
                zlist_remove (neighbors_ids, neighbor);
        }
    }
    return neighbors_ids;
}

static void
s_send_to (zyre_election_t *self, zre_msg_t *msg, zlist_t *peers)
{
    assert (self);
    assert (msg);
    assert (peers);

    if (zlist_size (peers) == 0)
        goto cleanup;     //  No peers found yet!

    const char *peer = (const char *) zlist_first (peers);
    while (peer) {
        //  Send message to peer
        zmsg_t *copy = zmsg_dup (msg);

        zyre_whisper (self->node, peer, &copy);

        //  Get next peer in list
        peer = (char *) zlist_next (peers);
    }

cleanup:
    zlist_destroy (&peers);
    zmsg_destroy (&msg);
}

//  --------------------------------------------------------------------------
//  Initiate election

void
zyre_election_start (zyre_election_t *self)
{
    assert (self);
    self->caw = strdup (zyre_uuid (self->node));

    zmsg_t *election_msg = zmsg_new ();
    zmsg_addstr (election_msg, "ZLE");
    zmsg_addstr (election_msg, "ELECTION");
    zmsg_addstr (election_msg, zyre_uuid (self->node));

    //  Send election message to all neighbors
    s_send_to (self, election_msg, s_neighbors (self, true));
    if (self->verbose)
        zsys_info ("ELECTION started by %s\n", zyre_uuid (self->node));
}
*/


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
//  Handle received election and leader messages. Return 1 if election is
//  still in progress, 0 if election is concluded and -1 is an error occurred.

    /*
int
zyre_election_recv (zyre_election_t *self, zre_msg_t *msg, zyre_peer_t *sender)
{
    assert (self);
    assert (msg);

    const char *r = zre_msg_challenger_id (msg);

    if (zre_msg_id (msg) == ZRE_MSG_ELECT) {
        //  Initiate or re-initiate leader election
        if (!self->caw || strcmp (r, self->caw) < 0) {
            zstr_free (&self->caw);     //  Free caw when re-initiated
            zstr_free (&self->leader);  //  Free leader when re-initiated
            self->father = NULL;        //  Reset father when re-initiated
            self->caw = strdup (r);
            self->erec = 0;
            self->lrec = 0;
            self->father = sender;

            zre_msg_t *election_msg = zre_msg_new ();
            zre_msg_set_id (election_msg, ZRE_MSG_ELECT);
            zre_msg_set_challenger_id (election_msg, r);

            //  Send election message to all neighbors but father but father
            s_send_to (self, election_msg, s_neighbors (self, false));
            if (self->verbose)
                zsys_info ("Initialise election %s\n", zyre_uuid (self->node));
        }

        //  Participate in current active wave
        if (strcmp (r, self->caw) == 0) {
            self->erec++;
            if (self->erec == s_neighbors_count (self)) {
                if (streq (self->caw, zyre_uuid (self->node))) {
                    zmsg_t *leader_msg = zmsg_new ();
                    zmsg_addstr (leader_msg, "ZLE");
                    zmsg_addstr (leader_msg, "LEADER");
                    zmsg_addstr (leader_msg, r);

                    //  Send leader message to all neighbors
                    s_send_to (self, leader_msg, s_neighbors (self, true));
                    if (self->verbose)
                        zsys_info ("LEADER decision by %s\n", zyre_uuid (self->node));
                }
                else {
                    zmsg_t *election_msg = zmsg_new ();
                    zmsg_addstr (election_msg, "ZLE");
                    zmsg_addstr (election_msg, "ELECTION");
                    zmsg_addstr (election_msg, self->caw);

                    //  Send election message to father
                    zyre_whisper (self->node, self->father, &election_msg);
                    if (self->verbose)
                        zsys_info ("Echo wave to father %s\n", zyre_uuid (self->node));
                }
            }
        }
        //  If r > caw, the message is ignored!
    }
    else
    if (streq (type, "LEADER")) {
        if (self->lrec == 0) {
            zmsg_t *leader_msg = zmsg_new ();
            zmsg_addstr (leader_msg, "ZLE");
            zmsg_addstr (leader_msg, "LEADER");
            zmsg_addstr (leader_msg, r);

            //  Send leader message to all neighbors
            s_send_to (self, leader_msg, s_neighbors (self, true));
            if (self->verbose)
                zsys_info ("Propagate LEADER by %s\n", zyre_uuid (self->node));
        }
        self->lrec++;
        zstr_free (&self->leader);
        self->leader = strdup (r);
        if (self->verbose)
            zsys_info ("Received LEADER by %s\n", zyre_uuid (self->node));
    }

    zstr_free (&type);
    zstr_free (&r);
    zyre_event_destroy (&event);

    if (self->lrec == s_neighbors_count (self)) {
        self->state = streq (self->leader, zyre_uuid (self->node));
        zstr_free (&self->caw);     //  Free caw as election is finished
        if (self->verbose)
            zsys_info ("Election finished %s, %s!\n", zyre_uuid (self->node), self->state? "true": "false");

        return 0;
    }
    else
    if (self->lrec > s_neighbors_count (self)) {
        if (self->verbose)
            zsys_info ("Too much %s, %s!\n", zyre_uuid (self->node), self->state? "true": "false");

        return 1;
    }
    else
        return 1;
}
    */


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
    zclock_sleep (500);

    //  Join topology
    zyre_join (node1, "GLOBAL");
    zyre_join (node2, "GLOBAL");

    zyre_join (node1, "GLOBAL1");
    zyre_join (node2, "GLOBAL1");

    zclock_sleep (1500);

    zyre_stop (node1);
    zyre_stop (node2);

    zyre_destroy (&node1);
    zyre_destroy (&node2);
    //  @end
    printf ("OK\n");
}
