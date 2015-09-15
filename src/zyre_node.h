/*  =========================================================================
    zyre_node - ZRE node on the network

    -------------------------------------------------------------------------
    Copyright (c) the Contributors as noted in the AUTHORS file.

    This file is part of Zyre, an open-source framework for proximity-based
    peer-to-peer applications -- See http://zyre.org.

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
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
