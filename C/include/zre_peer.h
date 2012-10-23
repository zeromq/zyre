/*  =========================================================================
    zre_peer - peer to a ZyRE network

    -------------------------------------------------------------------------
    Copyright (c) 1991-2012 iMatix Corporation <www.imatix.com>
    Copyright other contributors as noted in the AUTHORS file.

    This file is part of ZyRE, the ZeroMQ Realtime Experience framework:
    http://zyre.org.

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

#ifndef __ZRE_PEER_H_INCLUDED__
#define __ZRE_PEER_H_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _zre_peer_t zre_peer_t;

//  Constructor
zre_peer_t *
    zre_peer_new (char *identity, zhash_t *container, zctx_t *ctx);

//  Destructor
void
    zre_peer_destroy (zre_peer_t **self_p);

//  Connect peer mailbox
void
    zre_peer_connect (zre_peer_t *self, char *reply_to, char *address, int port);

//  Return peer connected status
bool
    zre_peer_connected (zre_peer_t *self);

//  Send message to peer
void
    zre_peer_send (zre_peer_t *self, zre_msg_t **msg_p);

//  Return peer identity string
char *
    zre_peer_identity (zre_peer_t *self);
    
//  Register activity at peer
void
    zre_peer_refresh (zre_peer_t *self);
    
//  Return peer future evasive time
int64_t
    zre_peer_evasive_at (zre_peer_t *self);

//  Return peer future expired time
int64_t
    zre_peer_expired_at (zre_peer_t *self);

//  Return peer ready flag
bool
    zre_peer_ready (zre_peer_t *self);
    
//  Set peer ready flag
bool
    zre_peer_ready_set (zre_peer_t *self, bool ready);

//  Return peer status
byte
    zre_peer_status (zre_peer_t *self);

//  Set peer status
void
    zre_peer_status_set (zre_peer_t *self, byte status);

#ifdef __cplusplus
}
#endif

#endif
