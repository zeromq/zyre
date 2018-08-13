/*  =========================================================================
    zyre_peer - ZRE network peer

    -------------------------------------------------------------------------
    Copyright (c) the Contributors as noted in the AUTHORS file.

    This file is part of Zyre, an open-source framework for proximity-based
    peer-to-peer applications -- See http://zyre.org.

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
    =========================================================================
*/

#ifndef __ZYRE_PEER_H_INCLUDED__
#define __ZYRE_PEER_H_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif

//  Constructor
ZYRE_PRIVATE zyre_peer_t *
    zyre_peer_new (zhash_t *container, zuuid_t *uuid);

//  Destructor
ZYRE_PRIVATE void
    zyre_peer_destroy (zyre_peer_t **self_p);

//  Connect peer mailbox
ZYRE_PRIVATE int
    zyre_peer_connect (zyre_peer_t *self, zuuid_t *from, const char *endpoint, uint64_t expired_timeout);

//  Connect peer mailbox
ZYRE_PRIVATE void
    zyre_peer_disconnect (zyre_peer_t *self);

//  Return peer connected status
ZYRE_PRIVATE bool
    zyre_peer_connected (zyre_peer_t *self);

//  Return peer connection endpoint
ZYRE_PRIVATE const char *
    zyre_peer_endpoint (zyre_peer_t *self);

//  Send message to peer
ZYRE_PRIVATE int
    zyre_peer_send (zyre_peer_t *self, zre_msg_t **msg_p);

//  Return peer identity string
ZYRE_PRIVATE const char *
    zyre_peer_identity (zyre_peer_t *self);

//  Register activity at peer
ZYRE_PRIVATE void
    zyre_peer_refresh (zyre_peer_t *self, uint64_t evasive_timeout, uint64_t expired_timeout);

//  Return peer future evasive time
ZYRE_PRIVATE int64_t
    zyre_peer_evasive_at (zyre_peer_t *self);

//  Return peer future expired time
ZYRE_PRIVATE int64_t
    zyre_peer_expired_at (zyre_peer_t *self);

//  Return peer name
ZYRE_PRIVATE const char *
    zyre_peer_name (zyre_peer_t *self);

//  Set peer name
ZYRE_PRIVATE void
    zyre_peer_set_name (zyre_peer_t *self, const char *name);

//  Set current node name, for logging
ZYRE_PRIVATE void
    zyre_peer_set_origin (zyre_peer_t *self, const char *origin);

//  Return peer status
ZYRE_PRIVATE byte
    zyre_peer_status (zyre_peer_t *self);

//  Set peer status
ZYRE_PRIVATE void
    zyre_peer_set_status (zyre_peer_t *self, byte status);

//  Return peer ready state
ZYRE_PRIVATE byte
    zyre_peer_ready (zyre_peer_t *self);

//  Set peer ready
ZYRE_PRIVATE void
    zyre_peer_set_ready (zyre_peer_t *self, bool ready);

//  Get peer header value
ZYRE_PRIVATE const char *
    zyre_peer_header (zyre_peer_t *self, char *key, char *default_value);

//  Get peer headers table
ZYRE_PRIVATE zhash_t *
    zyre_peer_headers (zyre_peer_t *self);

//  Set peer headers from provided dictionary
ZYRE_PRIVATE void
    zyre_peer_set_headers (zyre_peer_t *self, zhash_t *headers);

//  Check if messages were lost from peer, returns true if they were.
ZYRE_PRIVATE bool
    zyre_peer_messages_lost (zyre_peer_t *self, zre_msg_t *msg);

//  Ask peer to log all traffic via zsys
ZYRE_PRIVATE void
    zyre_peer_set_verbose (zyre_peer_t *self, bool verbose);

//  Return want_sequence
ZYRE_PRIVATE uint16_t
    zyre_peer_want_sequence (zyre_peer_t *self);

//  Return sent_sequence
ZYRE_PRIVATE uint16_t
    zyre_peer_sent_sequence (zyre_peer_t *self);

// curve support
ZYRE_PRIVATE void
    zyre_peer_set_server_key (zyre_peer_t *self, const char *key);

ZYRE_PRIVATE void
    zyre_peer_set_public_key (zyre_peer_t *self, const char *key);

ZYRE_PRIVATE void
    zyre_peer_set_secret_key (zyre_peer_t *self, const char *key);

//  Self test of this class
ZYRE_PRIVATE void
    zyre_peer_test (bool verbose);

#ifdef __cplusplus
}
#endif

#endif
