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
#include <uuid/uuid.h>
#include "../include/zre.h"


//  ---------------------------------------------------------------------
//  Structure of our class

struct _zre_peer_t {
    uuid_t uuid;                //  Peer's UUID as binary blob
    char *uuid_str;             //  UUID as printable string
    uint64_t expires_at;
};


//  ---------------------------------------------------------------------
//  Construct new peer object

zre_peer_t *
zre_peer_new (uuid_t uuid, char *uuid_str)
{
    zre_peer_t *self = (zre_peer_t *) zmalloc (sizeof (zre_peer_t));
    memcpy (self->uuid, uuid, sizeof (uuid_t));
    self->uuid_str = strdup (uuid_str);
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
        free (self->uuid_str);
        free (self);
        *self_p = NULL;
    }
}


//  ---------------------------------------------------------------------
//  Return peer UUID blob

byte *
zre_peer_uuid (zre_peer_t *self)
{
    assert (self);
    return self->uuid;
}


//  ---------------------------------------------------------------------
//  Return peer UUID string

char *
zre_peer_uuid_str (zre_peer_t *self)
{
    assert (self);
    return self->uuid_str;
}


//  ---------------------------------------------------------------------
//  Register activity at peer

void
zre_peer_is_alive (zre_peer_t *self)
{
    assert (self);
    self->expires_at = zclock_time () + PEER_EXPIRY;
}

//  ---------------------------------------------------------------------
//  Return peer expiry time

int64_t
zre_peer_expires_at (zre_peer_t *self)
{
    assert (self);
    return self->expires_at;
}
