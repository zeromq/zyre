/*  =========================================================================
    zre_group - group known to this node

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

#ifndef __ZRE_GROUP_H_INCLUDED__
#define __ZRE_GROUP_H_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _zre_group_t zre_group_t;

//  Constructor
zre_group_t *
    zre_group_new (char *name, zhash_t *container);

//  Destructor
void
    zre_group_destroy (zre_group_t **self_p);

//  Add peer to group
void
    zre_group_join (zre_group_t *self, zre_peer_t *peer);

//  Remove peer from group
void
    zre_group_leave (zre_group_t *self, zre_peer_t *peer);

//  Send message to all peers in group
void
    zre_group_send (zre_group_t *self, zre_msg_t **msg_p);

#ifdef __cplusplus
}
#endif

#endif
