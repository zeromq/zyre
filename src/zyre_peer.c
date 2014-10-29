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

#include "../include/zyre.h"
#include "zyre_classes.h"

//  --------------------------------------------------------------------------
//  Structure of our class

struct _zyre_peer_t {
    zsock_t *mailbox;           //  Socket through to peer
    zuuid_t *uuid;              //  Identity object
    char *endpoint;             //  Endpoint connected to
    char *name;                 //  Peer's public name
    char *origin;               //  Origin node's public name
    uint64_t evasive_at;        //  Peer is being evasive
    uint64_t expired_at;        //  Peer has expired by now
    bool connected;             //  Peer will send messages
    bool ready;                 //  Peer has said Hello to us
    byte status;                //  Our status counter
    uint16_t sent_sequence;     //  Outgoing message sequence
    uint16_t want_sequence;     //  Incoming message sequence
    zhash_t *headers;           //  Peer headers
    bool verbose;               //  Do we log traffic & failures?
};


//  Callback when we remove peer from container

static void
s_delete_peer (void *argument)
{
    zyre_peer_t *peer = (zyre_peer_t *) argument;
    zyre_peer_destroy (&peer);
}


//  --------------------------------------------------------------------------
//  Construct new peer object

zyre_peer_t *
zyre_peer_new (zhash_t *container, zuuid_t *uuid)
{
    zyre_peer_t *self = (zyre_peer_t *) zmalloc (sizeof (zyre_peer_t));
    self->uuid = zuuid_dup (uuid);
    self->ready = false;
    self->connected = false;
    self->sent_sequence = 0;
    self->want_sequence = 0;

    //  Insert into container if requested
    if (container) {
        int rc = zhash_insert (container, zuuid_str (self->uuid), self);
        assert (rc == 0);
        zhash_freefn (container, zuuid_str (self->uuid), s_delete_peer);
    }
    return self;
}


//  --------------------------------------------------------------------------
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
        free (self->name);
        free (self->origin);
        free (self);
        *self_p = NULL;
    }
}


//  --------------------------------------------------------------------------
//  Connect peer mailbox
//  Configures mailbox and connects to peer's router endpoint

void
zyre_peer_connect (zyre_peer_t *self, zuuid_t *from, const char *endpoint)
{
    assert (self);
    assert (!self->connected);

    //  Create new outgoing socket (drop any messages in transit)
    self->mailbox = zsock_new (ZMQ_DEALER);
    if (!self->mailbox)
        return;             //  Null when we're shutting down
    
    //  Set our own identity on the socket so that receiving node
    //  knows who each message came from. Note that we cannot use
    //  the UUID directly as the identity since it may contain a
    //  zero byte at the start, which libzmq does not like for
    //  historical and arguably bogus reasons that it nonetheless
    //  enforces.
    byte routing_id [ZUUID_LEN + 1] = { 1 };
    memcpy (routing_id + 1, zuuid_data (from), ZUUID_LEN);
    int rc = zmq_setsockopt (zsock_resolve (self->mailbox),
                             ZMQ_IDENTITY, routing_id, ZUUID_LEN + 1);
    assert (rc == 0);

    //  Set a high-water mark that allows for reasonable activity
    zsock_set_sndhwm (self->mailbox, PEER_EXPIRED * 100);

    //  Send messages immediately or return EAGAIN
    zsock_set_sndtimeo (self->mailbox, 0);

    //  Connect through to peer node
    rc = zsock_connect (self->mailbox, "%s", endpoint);
    if (rc != 0) {
        zsys_error ("(%s) cannot connect to endpoint=%s",
                    self->origin, endpoint);
        //  Don't really have any error handling yet; if connect
        //  fails, there's something wrong with connect endpoint?
        assert (false);
    }
    assert (rc == 0);
    if (self->verbose)
        zsys_info ("(%s) connect to peer: endpoint=%s",
                   self->origin, endpoint);

    self->endpoint = strdup (endpoint);
    self->connected = true;
    self->ready = false;
}


//  --------------------------------------------------------------------------
//  Disconnect peer mailbox
//  No more messages will be sent to peer until connected again

void
zyre_peer_disconnect (zyre_peer_t *self)
{
    //  If connected, destroy socket and drop all pending messages
    assert (self);
    if (self->connected) {
        zsock_destroy (&self->mailbox);
        free (self->endpoint);
        self->mailbox = NULL;
        self->endpoint = NULL;
        self->connected = false;
        self->ready = false;
    }
}


//  ---------------------------------------------------------------------
//  Send message to peer

int
zyre_peer_send (zyre_peer_t *self, zre_msg_t **msg_p)
{
    assert (self);
    zre_msg_t *msg = *msg_p;
    assert (msg);
    if (self->connected) {
        self->sent_sequence += 1;
        zre_msg_set_sequence (msg, self->sent_sequence);
        if (self->verbose)
            zsys_info ("(%s) send %s to peer=%s sequence=%d",
                self->origin,
                zre_msg_command (msg),
                self->name? self->name: "-",
                zre_msg_sequence (msg));
            
        if (zre_msg_send (msg_p, self->mailbox)) {
            if (errno == EAGAIN) {
                if (self->verbose)
                    zsys_info ("(%s) disconnect from peer (EAGAIN): name=%s",
                        self->origin, self->name);
                zyre_peer_disconnect (self);
                return -1;
            }
            //  Can't get any other error here
            assert (false);
        }
    }
    else
        zre_msg_destroy (msg_p);
    
    return 0;
}


//  --------------------------------------------------------------------------
//  Return peer connected status

bool
zyre_peer_connected (zyre_peer_t *self)
{
    assert (self);
    return self->connected;
}


//  --------------------------------------------------------------------------
//  Return peer identity string

char *
zyre_peer_identity (zyre_peer_t *self)
{
    assert (self);
    return zuuid_str (self->uuid);
}


//  --------------------------------------------------------------------------
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


//  --------------------------------------------------------------------------
//  Register activity at peer

void
zyre_peer_refresh (zyre_peer_t *self)
{
    assert (self);
    self->evasive_at = zclock_time () + PEER_EVASIVE;
    self->expired_at = zclock_time () + PEER_EXPIRED;
}


//  --------------------------------------------------------------------------
//  Return peer future evasive time

int64_t
zyre_peer_evasive_at (zyre_peer_t *self)
{
    assert (self);
    return self->evasive_at;
}


//  --------------------------------------------------------------------------
//  Return peer future expired time

int64_t
zyre_peer_expired_at (zyre_peer_t *self)
{
    assert (self);
    return self->expired_at;
}


//  --------------------------------------------------------------------------
//  Return peer name

char *
zyre_peer_name (zyre_peer_t *self)
{
    assert (self);
    return self->name? self->name: "";
}


//  --------------------------------------------------------------------------
//  Set peer name

void
zyre_peer_set_name (zyre_peer_t *self, const char *name)
{
    assert (self);
    free (self->name);
    self->name = strdup (name);
}


//  --------------------------------------------------------------------------
//  Set current node name, for logging

void
zyre_peer_set_origin (zyre_peer_t *self, const char *origin)
{
    assert (self);
    free (self->origin);
    self->origin = strdup (origin);
}


//  --------------------------------------------------------------------------
//  Return peer cycle
//  This gives us a state change count for the peer, which we can
//  check against its claimed status, to detect message loss.

byte
zyre_peer_status (zyre_peer_t *self)
{
    assert (self);
    return self->status;
}


//  --------------------------------------------------------------------------
//  Set peer status

void
zyre_peer_set_status (zyre_peer_t *self, byte status)
{
    assert (self);
    self->status = status;
}


//  --------------------------------------------------------------------------
//  Return peer ready state

byte
zyre_peer_ready (zyre_peer_t *self)
{
    assert (self);
    return self->ready;
}


//  --------------------------------------------------------------------------
//  Set peer ready

void
zyre_peer_set_ready (zyre_peer_t *self, bool ready)
{
    assert (self);
    self->ready = ready;
}


//  --------------------------------------------------------------------------
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


//  --------------------------------------------------------------------------
//  Get peer headers table

zhash_t *
zyre_peer_headers (zyre_peer_t *self)
{
    assert (self);
    return self->headers;
}


//  --------------------------------------------------------------------------
//  Set peer headers from provided dictionary

void
zyre_peer_set_headers (zyre_peer_t *self, zhash_t *headers)
{
    assert (self);
    zhash_destroy (&self->headers);
    self->headers = zhash_dup (headers);
}


//  --------------------------------------------------------------------------
//  Check if messages were lost from peer, returns true if they were

bool
zyre_peer_messages_lost (zyre_peer_t *self, zre_msg_t *msg)
{
    assert (self);
    assert (msg);

    //  The sequence number set by the peer, and our own calculated
    //  sequence number should be the same.
    if (self->verbose)
        zsys_info ("(%s) recv %s from peer=%s sequence=%d",
            self->origin,
            zre_msg_command (msg),
            self->name? self->name: "-",
            zre_msg_sequence (msg));

    //  HELLO always MUST have sequence = 1
    if (zre_msg_id (msg) == ZRE_MSG_HELLO)
        self->want_sequence = 1;
    else
        self->want_sequence += 1;
        
    if (self->want_sequence != zre_msg_sequence (msg)) {
        zsys_info ("(%s) seq error from peer=%s expect=%d, got=%d",
            self->origin,
            self->name? self->name: "-",
            self->want_sequence,
            zre_msg_sequence (msg));
        return true;
    }
    return false;
}


//  --------------------------------------------------------------------------
//  Ask peer to log all traffic via zsys

void
zyre_peer_set_verbose (zyre_peer_t *self, bool verbose)
{
    assert (self);
    self->verbose = verbose;
}


//  --------------------------------------------------------------------------
//  Self test of this class

void
zyre_peer_test (bool verbose)
{
    printf (" * zyre_peer: ");

    zsock_t *mailbox = zsock_new_dealer ("@tcp://127.0.0.1:5551");
    zhash_t *peers = zhash_new ();
    zuuid_t *you = zuuid_new ();
    zuuid_t *me = zuuid_new ();
    zyre_peer_t *peer = zyre_peer_new (peers, you);
    assert (!zyre_peer_connected (peer));
    zyre_peer_connect (peer, me, "tcp://127.0.0.1:5551");
    assert (zyre_peer_connected (peer));
    zyre_peer_set_name (peer, "peer");
    assert (streq (zyre_peer_name (peer), "peer"));

    zre_msg_t *msg = zre_msg_new (ZRE_MSG_HELLO);
    zre_msg_set_endpoint (msg, "tcp://127.0.0.1:5552");
    int rc = zyre_peer_send (peer, &msg);
    assert (rc == 0);

    msg = zre_msg_recv (mailbox);
    assert (msg);
    if (verbose)
        zre_msg_print (msg);
    zre_msg_destroy (&msg);

    //  Destroying container destroys all peers it contains
    zhash_destroy (&peers);
    zuuid_destroy (&me);
    zuuid_destroy (&you);
    zsock_destroy (&mailbox);

    printf ("OK\n");
}
