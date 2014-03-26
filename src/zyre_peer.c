/*  =========================================================================
    zyre_peer - one of our peers in a ZRE network

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

#include "zyre_classes.h"

//  ---------------------------------------------------------------------
//  Structure of our class

struct _zyre_peer_t {
    zctx_t *ctx;                //  CZMQ context
    void *mailbox;              //  Socket through to peer
    zuuid_t *uuid;              //  Identity object
    char *endpoint;             //  Endpoint connected to
    uint64_t evasive_at;        //  Peer is being evasive
    uint64_t expired_at;        //  Peer has expired by now
    bool connected;             //  Peer will send messages
    bool ready;                 //  Peer has said Hello to us
    byte status;                //  Our status counter
    uint16_t sent_sequence;     //  Outgoing message sequence
    uint16_t want_sequence;     //  Incoming message sequence
    zhash_t *headers;           //  Peer headers
    zyre_log_t *log;            //  Log publisher, if any
};


//  Callback when we remove peer from container

static void
s_delete_peer (void *argument)
{
    zyre_peer_t *peer = (zyre_peer_t *) argument;
    zyre_peer_destroy (&peer);
}


//  ---------------------------------------------------------------------
//  Construct new peer object

zyre_peer_t *
zyre_peer_new (zctx_t *ctx, zhash_t *container, zuuid_t *uuid)
{
    zyre_peer_t *self = (zyre_peer_t *) zmalloc (sizeof (zyre_peer_t));
    self->ctx = ctx;
    self->uuid = zuuid_dup (uuid);
    self->ready = false;
    self->connected = false;
    self->sent_sequence = 0;
    self->want_sequence = 0;

    //  Insert into container if requested
    if (container) {
        zhash_insert (container, zuuid_str (self->uuid), self);
        zhash_freefn (container, zuuid_str (self->uuid), s_delete_peer);
    }
    return self;
}


//  ---------------------------------------------------------------------
//  Destroy peer object

void
zyre_peer_destroy (zyre_peer_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zyre_peer_t *self = *self_p;
        zyre_peer_disconnect (self);
        zhash_destroy (&self->headers);
        zuuid_destroy (&self->uuid);
        free (self);
        *self_p = NULL;
    }
}


//  ---------------------------------------------------------------------
//  Connect peer mailbox
//  Configures mailbox and connects to peer's router endpoint

void
zyre_peer_connect (zyre_peer_t *self, zuuid_t *from, char *endpoint)
{
    assert (self);
    assert (!self->connected);

    //  Create new outgoing socket (drop any messages in transit)
    self->mailbox = zsocket_new (self->ctx, ZMQ_DEALER);
    //  Null if shutting down
    if (self->mailbox) {
        //  Set our own identity on the socket so that receiving node
        //  knows who each message came from.
        zmq_setsockopt (self->mailbox, ZMQ_IDENTITY,
                        zuuid_data (from), zuuid_size (from));

        //  Set a high-water mark that allows for reasonable activity
        zsocket_set_sndhwm (self->mailbox, PEER_EXPIRED * 100);

        //  Send messages immediately or return EAGAIN
        zsocket_set_sndtimeo (self->mailbox, 0);

        //  Connect through to peer node
        int rc = zsocket_connect (self->mailbox, "%s", endpoint);
        assert (rc == 0);
        self->endpoint = strdup (endpoint);
        self->connected = true;
        self->ready = false;
    }
}


//  ---------------------------------------------------------------------
//  Disconnect peer mailbox
//  No more messages will be sent to peer until connected again

void
zyre_peer_disconnect (zyre_peer_t *self)
{
    //  If connected, destroy socket and drop all pending messages
    assert (self);
    if (self->connected) {
        zsocket_destroy (self->ctx, self->mailbox);
        free (self->endpoint);
        self->mailbox = NULL;
        self->endpoint = NULL;
        self->connected = false;
    }
}


//  ---------------------------------------------------------------------
//  Send message to peer

int
zyre_peer_send (zyre_peer_t *self, zre_msg_t **msg_p)
{
    assert (self);
    if (self->connected) {
        zre_msg_set_sequence (*msg_p, ++(self->sent_sequence));
        if (self->log)
            zyre_log_info (self->log, ZRE_LOG_MSG_EVENT_SEND,
                zuuid_str (self->uuid), "seq=%d command=%s",
                zre_msg_sequence (*msg_p), zre_msg_command (*msg_p));
        if (zre_msg_send (msg_p, self->mailbox) && errno == EAGAIN) {
            zyre_peer_disconnect (self);
            return -1;
        }
    }
    else
        zre_msg_destroy (msg_p);
    
    return 0;
}


//  ---------------------------------------------------------------------
//  Return peer connected status

bool
zyre_peer_connected (zyre_peer_t *self)
{
    assert (self);
    return self->connected;
}


//  ---------------------------------------------------------------------
//  Return peer identity string

char *
zyre_peer_identity (zyre_peer_t *self)
{
    assert (self);
    return zuuid_str (self->uuid);
}


//  ---------------------------------------------------------------------
//  Return peer connection endpoint

char *
zyre_peer_endpoint (zyre_peer_t *self)
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
zyre_peer_refresh (zyre_peer_t *self)
{
    assert (self);
    self->evasive_at = zclock_time () + PEER_EVASIVE;
    self->expired_at = zclock_time () + PEER_EXPIRED;
}


//  ---------------------------------------------------------------------
//  Return peer future evasive time

int64_t
zyre_peer_evasive_at (zyre_peer_t *self)
{
    assert (self);
    return self->evasive_at;
}


//  ---------------------------------------------------------------------
//  Return peer future expired time

int64_t
zyre_peer_expired_at (zyre_peer_t *self)
{
    assert (self);
    return self->expired_at;
}


//  ---------------------------------------------------------------------
//  Return peer cycle
//  This gives us a state change count for the peer, which we can
//  check against its claimed status, to detect message loss.

byte
zyre_peer_status (zyre_peer_t *self)
{
    assert (self);
    return self->status;
}


//  ---------------------------------------------------------------------
//  Set peer status

void
zyre_peer_set_status (zyre_peer_t *self, byte status)
{
    assert (self);
    self->status = status;
}


//  ---------------------------------------------------------------------
//  Return peer ready state

byte
zyre_peer_ready (zyre_peer_t *self)
{
    assert (self);
    return self->ready;
}


//  ---------------------------------------------------------------------
//  Set peer ready

void
zyre_peer_set_ready (zyre_peer_t *self, bool ready)
{
    assert (self);
    self->ready = ready;
}


//  ---------------------------------------------------------------------
//  Get peer header value

char *
zyre_peer_header (zyre_peer_t *self, char *key, char *default_value)
{
    assert (self);
    char *value = NULL;
    if (self->headers)
        value = (char *) (zhash_lookup (self->headers, key));
    if (!value)
        value = default_value;

    return value;
}


//  ---------------------------------------------------------------------
//  Set peer headers from provided dictionary

void
zyre_peer_set_headers (zyre_peer_t *self, zhash_t *headers)
{
    assert (self);
    zhash_destroy (&self->headers);
    self->headers = zhash_dup (headers);
}


//  ---------------------------------------------------------------------
//  Check peer message sequence

bool
zyre_peer_check_message (zyre_peer_t *self, zre_msg_t *msg)
{
    assert (self);
    assert (msg);
    uint16_t recd_sequence = zre_msg_sequence (msg);

    bool valid = (++(self->want_sequence) == recd_sequence);
    if (!valid)
        --(self->want_sequence);    //  Rollback

    return valid;
}


//  ---------------------------------------------------------------------
//  Ask peer to log all traffic via ZRE_LOG

void
zyre_peer_set_log (zyre_peer_t *self, zyre_log_t *log)
{
    assert (self);
    self->log = log;
}


//  --------------------------------------------------------------------------
//  Self test of this class

void
zyre_peer_test (bool verbose)
{
    printf (" * zyre_peer: ");

    zctx_t *ctx = zctx_new ();
    void *mailbox = zsocket_new (ctx, ZMQ_DEALER);
    zsocket_bind (mailbox, "tcp://127.0.0.1:5555");

    zhash_t *peers = zhash_new ();
    zuuid_t *you = zuuid_new ();
    zuuid_t *me = zuuid_new ();
    zyre_peer_t *peer = zyre_peer_new (ctx, peers, you);
    assert (!zyre_peer_connected (peer));
    zyre_peer_connect (peer, me, "tcp://127.0.0.1:5555");
    assert (zyre_peer_connected (peer));

    zre_msg_t *msg = zre_msg_new (ZRE_MSG_HELLO);
    zre_msg_set_ipaddress (msg, "127.0.0.1");
    int rc = zyre_peer_send (peer, &msg);
    assert (rc == 0);

    msg = zre_msg_recv (mailbox);
    assert (msg);
    if (verbose)
        zre_msg_dump (msg);
    zre_msg_destroy (&msg);

    //  Destroying container destroys all peers it contains
    zuuid_destroy (&me);
    zuuid_destroy (&you);
    zhash_destroy (&peers);
    zctx_destroy (&ctx);

    printf ("OK\n");
}
