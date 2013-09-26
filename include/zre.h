/*  =========================================================================
    zre.h - Zyre library header

    -------------------------------------------------------------------------
    Copyright (c) 1991-2012 iMatix Corporation <www.imatix.com>
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

#ifndef __ZRE_H_INCLUDED__
#define __ZRE_H_INCLUDED__

#define ZRE_VERSION_MAJOR 1
#define ZRE_VERSION_MINOR 0
#define ZRE_VERSION_PATCH 0

#define ZRE_MAKE_VERSION(major, minor, patch) \
    ((major) * 10000 + (minor) * 100 + (patch))
#define ZRE_VERSION \
    ZRE_MAKE_VERSION(ZRE_VERSION_MAJOR, ZRE_VERSION_MINOR, ZRE_VERSION_PATCH)

#include <czmq.h>
#if CZMQ_VERSION < 2000
#   error "Zyre needs CZMQ/2.0.0 or later"
#endif

#include <fmq.h>
#if FMQ_VERSION < 10300
#   error "Zyre needs FileMQ/1.3.0 or later"
#endif

//  IANA-assigned port for ZRE discovery protocol
#define ZRE_DISCOVERY_PORT  5670

//  This is the only class that applications should use
#include "zre_node.h"

#endif
