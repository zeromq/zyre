/*  =========================================================================
    zyre_classes - all classes in proper order for building

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

#ifndef __ZYRE_CLASSES_H_INCLUDED__
#define __ZYRE_CLASSES_H_INCLUDED__

#include "../include/zyre.h"
#include "../include/zre_msg.h"
#include "../include/zre_log_msg.h"
#include "../include/zyre.h"
#include "../include/zyre_event.h"
#include "../include/zyre_log.h"
#include "zyre_peer.h"
#include "zyre_group.h"
#include "zyre_node.h"

//  Constants, to be configured/reviewed
#define PEER_EVASIVE     5000   //  Five seconds' silence is evasive
#define PEER_EXPIRED    10000   //  Ten seconds' silence is expired
#define REAP_INTERVAL    1000   //  Once per second

#endif
