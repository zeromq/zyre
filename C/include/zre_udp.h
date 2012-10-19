/*  =========================================================================
    zre_udp - UDP management class

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

#ifndef __ZRE_UDP_H_INCLUDED__
#define __ZRE_UDP_H_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _zre_udp_t zre_udp_t;

//  Constructor
zre_udp_t *
    zre_udp_new (int port_nbr);

//  Destructor
void
    zre_udp_destroy (zre_udp_t **self_p);

//  Returns UDP socket handle
int
    zre_udp_handle (zre_udp_t *self);

//  Send message using UDP broadcast
void
    zre_udp_send (zre_udp_t *self, byte *buffer, size_t length);

//  Receive message from UDP broadcast
ssize_t
    zre_udp_recv (zre_udp_t *self, byte *buffer, size_t length);

//  Return our own IP address as printable string
char *
    zre_udp_host (zre_udp_t *self);

//  Return IP address of peer that sent last message
char *
    zre_udp_from (zre_udp_t *self);
    
#ifdef __cplusplus
}
#endif

#endif
