/*  =========================================================================
    zre_internal.h - Zyre internal library header

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

#ifndef __ZRE_INTERNAL_H_INCLUDED__
#define __ZRE_INTERNAL_H_INCLUDED__

#include "zre.h"
#include "platform.h"

#if (!defined (__WINDOWS__))
#include <uuid/uuid.h>
#endif
#ifdef HAVE_LINUX_WIRELESS_H
# include <linux/wireless.h>
#else
#  ifdef HAVE_NET_IF_H
#   include <net/if.h>
#  endif
#  ifdef HAVE_NET_IF_MEDIA_H
#   include <net/if_media.h>
#  endif
#endif

#if defined (__WINDOWS__)
#   if (_WIN32_WINNT >= 0x0501)
#   else
#       undef _WIN32_WINNT
#       define _WIN32_WINNT 0x0501
#   endif
#include <ws2tcpip.h>           // getnameinfo()
#include <iphlpapi.h>           // GetAdaptersAddresses()
#endif


//  Constants, to be configured/reviewed

#define PING_INTERVAL    1000   //  Once per second
#define PEER_EVASIVE     5000   //  Five seconds' silence is evasive
#define PEER_EXPIRED    10000   //  Ten seconds' silence is expired

//  Classes in this stack

#include "zre_udp.h"
#include "zre_msg.h"
#include "zre_peer.h"
#include "zre_group.h"
#include "zre_log.h"
#include "zre_log_msg.h"

#endif
