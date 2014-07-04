/*  =========================================================================
    zyre_group - group known to this node

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

#ifndef __ZYRE_GROUP_H_INCLUDED__
#define __ZYRE_GROUP_H_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _zyre_group_t zyre_group_t;

//  Constructor
zyre_group_t *
    zyre_group_new (const char *name, zhash_t *container);

//  Destructor
void
    zyre_group_destroy (zyre_group_t **self_p);

//  Add peer to group
void
    zyre_group_join (zyre_group_t *self, zyre_peer_t *peer);

//  Remove peer from group
void
    zyre_group_leave (zyre_group_t *self, zyre_peer_t *peer);

//  Send message to all peers in group
void
    zyre_group_send (zyre_group_t *self, zre_msg_t **msg_p);

//  Self test of this class
ZYRE_EXPORT void
    zyre_group_test (bool verbose);
    
#ifdef __cplusplus
}
#endif

#endif
