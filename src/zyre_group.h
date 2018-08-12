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

#ifndef __ZYRE_GROUP_H_INCLUDED__
#define __ZYRE_GROUP_H_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif

//  Constructor
ZYRE_PRIVATE zyre_group_t *
    zyre_group_new (const char *name, zhash_t *container);

//  Destructor
ZYRE_PRIVATE void
    zyre_group_destroy (zyre_group_t **self_p);

//  Add peer to group
ZYRE_PRIVATE void
    zyre_group_join (zyre_group_t *self, zyre_peer_t *peer);

//  Remove peer from group
ZYRE_PRIVATE void
    zyre_group_leave (zyre_group_t *self, zyre_peer_t *peer);

//  Send message to all peers in group
ZYRE_PRIVATE void
    zyre_group_send (zyre_group_t *self, zre_msg_t **msg_p);

//  Return zlist of peer ids currently in this group
//  Caller owns return value and must destroy it when done.
ZYRE_PRIVATE zlist_t *
   zyre_group_peers (zyre_group_t *self);

//  Find or create an election for a group
zyre_election_t *
    zyre_group_require_election (zyre_group_t *self);

//  Enables peer to actively contest for leadership in this group.
void
    zyre_group_set_contest (zyre_group_t *self);

bool
    zyre_group_contest (zyre_group_t *self);

//  Return the election handler for this group.
zyre_election_t *
    zyre_group_election (zyre_group_t *self);

//  Sets the election handler for this group.
void
    zyre_group_set_election (zyre_group_t *self, zyre_election_t *election);

//  Return the peer that has been elected leader of this group.
zyre_peer_t *
    zyre_group_leader (zyre_group_t *self);

//  Sets the peer that has been elected leader of this group.
void
    zyre_group_set_leader (zyre_group_t *self, zyre_peer_t *leader);

//  Self test of this class
ZYRE_PRIVATE void
    zyre_group_test (bool verbose);

#ifdef __cplusplus
}
#endif

#endif
