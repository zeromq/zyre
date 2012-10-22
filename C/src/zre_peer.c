/*  =========================================================================
    zre_peer - one of our peers in a ZyRE network

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

#include <czmq.h>
#include "../include/zre.h"


//  ---------------------------------------------------------------------
//  Structure of our class

struct _zre_peer_t {
    zctx_t *ctx;                //  CZMQ context
    void *mailbox;              //  Socket through to peer
    char *identity;             //  Identity string
    uint64_t evasive_at;        //  Peer is being evasive
    uint64_t expired_at;        //  Peer has expired by now
    bool connected;             //  Peer is ready for work
    byte status;                //  Our status counter
};


//  Callback when we remove peer from container

static void
s_delete_peer (void *argument)
{
    zre_peer_t *peer = (zre_peer_t *) argument;
    zre_peer_destroy (&peer);
}


//  ---------------------------------------------------------------------
//  Construct new peer object

zre_peer_t *
zre_peer_new (char *identity, zhash_t *container, zctx_t *ctx)
{
    zre_peer_t *self = (zre_peer_t *) zmalloc (sizeof (zre_peer_t));
    self->ctx = ctx;
    self->mailbox = zsocket_new (ctx, ZMQ_DEALER);
    self->identity = strdup (identity);
    
    //  Insert into container if requested
    if (container) {
        zhash_insert (container, identity, self);
        zhash_freefn (container, identity, s_delete_peer);
    }
    return self;
}


//  ---------------------------------------------------------------------
//  Destroy peer object

void
zre_peer_destroy (zre_peer_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zre_peer_t *self = *self_p;
        free (self->identity);
        free (self);
        *self_p = NULL;
    }
}


//  ---------------------------------------------------------------------
//  Connect peer mailbox
//  Configures mailbox and connects to peer's router endpoint

void
zre_peer_connect (zre_peer_t *self, char *reply_to, char *address, int port)
{
    //  Not allowed to call if already connected
    assert (!self->connected);

    //  Set our caller 'From' identity so that receiving node
    //  knows who each message came from.
    zsocket_set_identity (self->mailbox, reply_to);

    //  We'll use credit based flow control later, for now set a
    //  low high-water mark to provoke blockage during tests
    zsocket_set_sndhwm (self->mailbox, 10);

    //  Connect through to peer node
    printf ("DEALER connecting to: tcp://%s:%d\n", address, port);
    zsocket_connect (self->mailbox, "tcp://%s:%d", address, port);

    self->connected = true;
}


//  ---------------------------------------------------------------------
//  Return peer connected status

bool
zre_peer_connected (zre_peer_t *self)
{
    assert (self);
    return self->connected;
}


//  ---------------------------------------------------------------------
//  Send message to peer

void
zre_peer_send (zre_peer_t *self, zre_msg_t **msg_p)
{
    assert (self);
    zre_msg_send (msg_p, self->mailbox);
}


//  ---------------------------------------------------------------------
//  Return peer identity string

char *
zre_peer_identity (zre_peer_t *self)
{
    assert (self);
    return self->identity;
}


//  ---------------------------------------------------------------------
//  Register activity at peer

void
zre_peer_refresh (zre_peer_t *self)
{
    assert (self);
    self->evasive_at = zclock_time () + PEER_EVASIVE;
    self->expired_at = zclock_time () + PEER_EXPIRED;
}


//  ---------------------------------------------------------------------
//  Return peer future evasive time

int64_t
zre_peer_evasive_at (zre_peer_t *self)
{
    assert (self);
    return self->evasive_at;
}


//  ---------------------------------------------------------------------
//  Return peer future expired time

int64_t
zre_peer_expired_at (zre_peer_t *self)
{
    assert (self);
    return self->expired_at;
}


//  ---------------------------------------------------------------------
//  Update peer status
//  This gives us a state change count for the peer, which we can
//  check against its claimed status, to detect message loss.

void
zre_peer_status_bump (zre_peer_t *self)
{
    assert (self);
    self->status++;
}


//  ---------------------------------------------------------------------
//  Return peer cycle

byte
zre_peer_status (zre_peer_t *self)
{
    assert (self);
    return self->status;
}

