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
    char *endpoint;             //  Endpoint connected to
    uint64_t evasive_at;        //  Peer is being evasive
    uint64_t expired_at;        //  Peer has expired by now
    bool connected;             //  Peer will send messages
    bool ready;                 //  Peer has said Hello to us
    byte status;                //  Our status counter
    uint32_t sent_sequence;     //  Outgoing message sequence
    uint32_t received_sequence; //  Incoming message sequence
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
    self->identity = strdup (identity);
    self->ready = false;
    self->sent_sequence = 0;
    self->received_sequence = 0;
    
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
        free (self->endpoint);
        free (self);
        *self_p = NULL;
    }
}


//  ---------------------------------------------------------------------
//  Connect peer mailbox
//  Configures mailbox and connects to peer's router endpoint

void
zre_peer_connect (zre_peer_t *self, char *reply_to, char *endpoint)
{
    //  If already connected, destroy old socket and start again
    if (self->connected)
        zsocket_destroy (self->ctx, self->mailbox);

    //  Create new outgoing socket (drop any messages in transit)
    self->mailbox = zsocket_new (self->ctx, ZMQ_DEALER);
    
    //  Set our caller 'From' identity so that receiving node knows
    //  who each message came from.
    zsocket_set_identity (self->mailbox, reply_to);

    //  Set a high-water mark that allows for reasonable activity
    zsocket_set_sndhwm (self->mailbox, PEER_EXPIRED * 100);
    
    //  Send messages immediately or return EAGAIN
    zsocket_set_sndtimeo (self->mailbox, 0);
    
    //  Connect through to peer node
    zsocket_connect (self->mailbox, "tcp://%s", endpoint);
    self->endpoint = strdup (endpoint);
    self->connected = true;
    self->ready = false;
}


//  ---------------------------------------------------------------------
//  Disconnect peer mailbox
//  No more messages will be sent to peer until connected again

void
zre_peer_disconnect (zre_peer_t *self)
{
    //  If connected, destroy socket and drop all pending messages
    assert (self);
    if (self->connected) {
        zsocket_destroy (self->ctx, self->mailbox);
        free (self->endpoint);
        self->endpoint = NULL;
        self->connected = false;
    }
}


//  ---------------------------------------------------------------------
//  Send message to peer

int
zre_peer_send (zre_peer_t *self, zre_msg_t **msg_p)
{
    assert (self);
    if (self->connected) {
        zre_msg_sequence_set (*msg_p, ++(self->sent_sequence));
        if (zre_msg_send (msg_p, self->mailbox) && errno == EAGAIN) {
            zre_peer_disconnect (self);
            return -1;
        }
    }
    return 0;
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
//  Return peer identity string

char *
zre_peer_identity (zre_peer_t *self)
{
    assert (self);
    return self->identity;
}


//  ---------------------------------------------------------------------
//  Return peer connection endpoint

char *
zre_peer_endpoint (zre_peer_t *self)
{
    assert (self);
    if (self->connected)
        return self->endpoint;
    else
        return "";
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
//  Return peer cycle
//  This gives us a state change count for the peer, which we can
//  check against its claimed status, to detect message loss.

byte
zre_peer_status (zre_peer_t *self)
{
    assert (self);
    return self->status;
}


//  ---------------------------------------------------------------------
//  Set peer status

void
zre_peer_status_set (zre_peer_t *self, byte status)
{
    assert (self);
    self->status = status;
}


//  ---------------------------------------------------------------------
//  Return peer ready state

byte
zre_peer_ready (zre_peer_t *self)
{
    assert (self);
    return self->ready;
}


//  ---------------------------------------------------------------------
//  Set peer ready

void
zre_peer_ready_set (zre_peer_t *self, bool ready)
{
    assert (self);
    self->ready = ready;
}

//  ---------------------------------------------------------------------
//  Check peer message sequence

bool
zre_peer_check_message (zre_peer_t *self, zre_msg_t *msg)
{
    assert (self);
    assert (msg);
    uint32_t received = (uint32_t) zre_msg_sequence (msg);

    bool valid = ++(self->received_sequence) == received;
    if (!valid)
        --(self->received_sequence); // rollback

    return valid;
}
