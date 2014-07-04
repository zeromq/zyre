/*  =========================================================================
    zyre_node - ZRE node on the network

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

#ifndef __ZYRE_NODE_H_INCLUDED__
#define __ZYRE_NODE_H_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _zyre_node_t zyre_node_t;

//  This is the actor that runs a single node; it uses one thread, creates
//  a zyre_node object at start and destroys that when finishing.
void
    zyre_node_actor (zsock_t *pipe, void *args);

//  Self test of this class
ZYRE_EXPORT void
    zyre_node_test (bool verbose);

#ifdef __cplusplus
}
#endif

#endif
