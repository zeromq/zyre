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

typedef struct _zyre_peer_t zyre_peer_t;

//  Constructor
zyre_peer_t *
    zyre_peer_new (zhash_t *container, zuuid_t *uuid);

//  Destructor
void
    zyre_peer_destroy (zyre_peer_t **self_p);

//  Connect peer mailbox
void
    zyre_peer_connect (zyre_peer_t *self, zuuid_t *from, const char *endpoint, uint64_t expired_timeout);

//  Connect peer mailbox
void
    zyre_peer_disconnect (zyre_peer_t *self);

//  Return peer connected status
bool
    zyre_peer_connected (zyre_peer_t *self);

//  Return peer connection endpoint
const char *
    zyre_peer_endpoint (zyre_peer_t *self);

//  Send message to peer
int
    zyre_peer_send (zyre_peer_t *self, zre_msg_t **msg_p);

//  Return peer identity string
const char *
    zyre_peer_identity (zyre_peer_t *self);

//  Register activity at peer
void
    zyre_peer_refresh (zyre_peer_t *self, uint64_t evasive_timeout, uint64_t expired_timeout);

//  Return peer future evasive time
int64_t
    zyre_peer_evasive_at (zyre_peer_t *self);

//  Return peer future expired time
int64_t
    zyre_peer_expired_at (zyre_peer_t *self);

//  Return peer name
const char *
    zyre_peer_name (zyre_peer_t *self);

//  Set peer name
void
    zyre_peer_set_name (zyre_peer_t *self, const char *name);

//  Set current node name, for logging
void
    zyre_peer_set_origin (zyre_peer_t *self, const char *origin);

//  Return peer status
byte
    zyre_peer_status (zyre_peer_t *self);

//  Set peer status
void
    zyre_peer_set_status (zyre_peer_t *self, byte status);

//  Return peer ready state
byte
    zyre_peer_ready (zyre_peer_t *self);

//  Set peer ready
void
    zyre_peer_set_ready (zyre_peer_t *self, bool ready);

//  Get peer header value
const char *
    zyre_peer_header (zyre_peer_t *self, char *key, char *default_value);

//  Get peer headers table
zhash_t *
    zyre_peer_headers (zyre_peer_t *self);

//  Set peer headers from provided dictionary
void
    zyre_peer_set_headers (zyre_peer_t *self, zhash_t *headers);

//  Check if messages were lost from peer, returns true if they were.
bool
    zyre_peer_messages_lost (zyre_peer_t *self, zre_msg_t *msg);

//  Ask peer to log all traffic via zsys
void
    zyre_peer_set_verbose (zyre_peer_t *self, bool verbose);

//  Self test of this class
ZYRE_EXPORT void
    zyre_peer_test (bool verbose);

#ifdef __cplusplus
}
#endif

#endif
