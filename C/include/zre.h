/*  =========================================================================
    zre.h - ZyRE framework in C

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

#ifndef __ZRE_H_INCLUDED__
#define __ZRE_H_INCLUDED__

#define ZRE_VERSION_MAJOR 0
#define ZRE_VERSION_MINOR 1
#define ZRE_VERSION_PATCH 0

#define ZRE_MAKE_VERSION(major, minor, patch) \
    ((major) * 10000 + (minor) * 100 + (patch))
#define ZRE_VERSION \
    ZRE_MAKE_VERSION(ZRE_VERSION_MAJOR, ZRE_VERSION_MINOR, ZRE_VERSION_PATCH)

#if CZMQ_VERSION < 10301
#   error "ZyRE needs CZMQ/1.3.1 or later, please get from GitHub"
#endif

//  Constants, to be configured/reviewed

#define PING_PORT_NUMBER 9999
#define PING_INTERVAL    1000  //  Once per second
#define PEER_EVASIVE     3000  //  Three seconds' silence is evasive
#define PEER_EXPIRED     5000  //  Five seconds' silence is expired

//  Classes in this stack

#include "zre_udp.h"
#include "zre_msg.h"
#include "zre_peer.h"
#include "zre_group.h"
#include "zre_interface.h"

#endif
