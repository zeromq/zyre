/*  =========================================================================
    zyre_peer - ZRE network peer

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
    zyre_peer_connect (zyre_peer_t *self, zuuid_t *from, const char *endpoint);

//  Connect peer mailbox
void
    zyre_peer_disconnect (zyre_peer_t *self);

//  Return peer connected status
bool
    zyre_peer_connected (zyre_peer_t *self);

//  Return peer connection endpoint
char *
    zyre_peer_endpoint (zyre_peer_t *self);

//  Send message to peer
int
    zyre_peer_send (zyre_peer_t *self, zre_msg_t **msg_p);

//  Return peer identity string
char *
    zyre_peer_identity (zyre_peer_t *self);
    
//  Register activity at peer
void
    zyre_peer_refresh (zyre_peer_t *self);
    
//  Return peer future evasive time
int64_t
    zyre_peer_evasive_at (zyre_peer_t *self);

//  Return peer future expired time
int64_t
    zyre_peer_expired_at (zyre_peer_t *self);

//  Return peer name
char *
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
char *
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
