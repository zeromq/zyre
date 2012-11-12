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

#include <czmq.h>
#include "../include/zre.h"
#include "platform.h"

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

//  -----------------------------------------------------------------
//  UDP instance

struct _zre_udp_t {
    int handle;                 //  Socket for send/recv
    int port_nbr;               //  UDP port number we work on
    struct sockaddr_in address;     //  Own address
    struct sockaddr_in broadcast;   //  Broadcast address
    struct sockaddr_in sender;      //  Where last recv came from
    char *host;                 //  Our own address as string
    char *from;                 //  Sender address of last message
};

//  Handle error from I/O operation

static void
s_handle_io_error (char *reason)
{
#   ifdef __WINDOWS__
    switch (WSAGetLastError ()) {
        case WSAEINTR:        errno = EINTR;      break;
        case WSAEBADF:        errno = EBADF;      break;
        case WSAEWOULDBLOCK:  errno = EAGAIN;     break;
        case WSAEINPROGRESS:  errno = EAGAIN;     break;
        case WSAENETDOWN:     errno = ENETDOWN;   break;
        case WSAECONNRESET:   errno = ECONNRESET; break;
        case WSAECONNABORTED: errno = EPIPE;      break;
        case WSAESHUTDOWN:    errno = ECONNRESET; break;
        case WSAEINVAL:       errno = EPIPE;      break;
        default:              errno = GetLastError ();
    }
#   endif
    if (errno == EAGAIN
    ||  errno == ENETDOWN
    ||  errno == EPROTO
    ||  errno == ENOPROTOOPT
    ||  errno == EHOSTDOWN
    ||  errno == ENONET
    ||  errno == EHOSTUNREACH
    ||  errno == EOPNOTSUPP
    ||  errno == ENETUNREACH
    ||  errno == EWOULDBLOCK
    ||  errno == EINTR)
        return;             //  Ignore error and try again
    else
    if (errno == EPIPE
    ||  errno == ECONNRESET)
        return;             //  Ignore error and try again
    else {
        zclock_log ("E: (UDP) error '%s' on %s", strerror (errno), reason);
        assert (false);
    }
}

//  check if given NIC name is wireless
static bool
s_wireless_nic (const char* name)
{
    int sock = 0;
    bool result = FALSE;

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        return FALSE;
    
#   if defined (SIOCGIFMEDIA)
    struct ifmediareq ifmr;
    memset (&ifmr, 0, sizeof (struct ifmediareq));
    strlcpy(ifmr.ifm_name, name, sizeof(ifmr.ifm_name));
    if (ioctl (sock, SIOCGIFMEDIA, (caddr_t) &ifmr) != -1)
        result = (IFM_TYPE (ifmr.ifm_current) == IFM_IEEE80211);
    
#   elif defined (SIOCGIWNAME)
    struct iwreq wrq;
    strncpy (wrq.ifr_name, name, IFNAMSIZ);
    if (ioctl (sock, SIOCGIWNAME, (caddr_t) &wrq) != -1)
        result = TRUE;
#   endif
    close(sock);
    return result;
}

//  -----------------------------------------------------------------
//  Constructor

zre_udp_t *
zre_udp_new (int port_nbr)
{
    zre_udp_t *self = (zre_udp_t *) zmalloc (sizeof (zre_udp_t));
    self->port_nbr = port_nbr;

    //  Create UDP socket
    self->handle = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (self->handle == -1)
        s_handle_io_error ("socket");

    //  Ask operating system to let us do broadcasts from socket
    int on = 1;
    if (setsockopt (self->handle, SOL_SOCKET,
                    SO_BROADCAST, &on, sizeof (on)) == -1)
        s_handle_io_error ("setsockopt (SO_BROADCAST)");

    //  Allow multiple processes to bind to socket; incoming
    //  messages will come to each process
    if (setsockopt (self->handle, SOL_SOCKET,
                    SO_REUSEADDR, &on, sizeof (on)) == -1)
        s_handle_io_error ("setsockopt (SO_REUSEADDR)");

#if defined (SO_REUSEPORT)
    if (setsockopt (self->handle, SOL_SOCKET,
                    SO_REUSEPORT, &on, sizeof (on)) == -1)
        s_handle_io_error ("setsockopt (SO_REUSEPORT)");
#endif
    //  PROBLEM: this design will not survive the network interface being
    //  killed and restarted while the program is running.
    struct sockaddr_in sockaddr = { 0 };
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons (self->port_nbr);
    sockaddr.sin_addr.s_addr = htonl (INADDR_ANY);
    if (bind (self->handle, (struct sockaddr *) &sockaddr, sizeof (sockaddr)) == -1)
        s_handle_io_error ("bind");

#   if defined (__UNIX__)
    struct ifaddrs *interfaces;
    if (getifaddrs (&interfaces) == 0) {
        struct ifaddrs *interface = interfaces;
        while (interface) {
            //  Hopefully the last interface will be WiFi
            if (interface->ifa_addr->sa_family == AF_INET) {
                self->address = *(struct sockaddr_in *) interface->ifa_addr;
                self->broadcast = *(struct sockaddr_in *) interface->ifa_broadaddr;
                self->broadcast.sin_port = htons (self->port_nbr);

                if (s_wireless_nic (interface->ifa_name))
                    break;
            }
            interface = interface->ifa_next;
        }
    }
    freeifaddrs (interfaces);
    if (self->host)
        free (self->host);
    self->host = zmalloc (INET_ADDRSTRLEN);
    inet_ntop (AF_INET, &self->address.sin_addr, self->host, sizeof (sockaddr));

#   else
#       error "Interface detection TBD on this operating system"
#   endif
    return self;
}


//  -----------------------------------------------------------------
//  Destructor

void
zre_udp_destroy (zre_udp_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zre_udp_t *self = *self_p;
        close (self->handle);
        free (self->host);
        free (self->from);
        free (self);
        *self_p = NULL;
    }
}


//  -----------------------------------------------------------------
//  Returns UDP socket handle

int
zre_udp_handle (zre_udp_t *self)
{
    assert (self);
    return self->handle;
}


//  -----------------------------------------------------------------
//  Send message using UDP broadcast

void
zre_udp_send (zre_udp_t *self, byte *buffer, size_t length)
{
    assert (self);
    inet_aton ("255.255.255.255", &self->broadcast.sin_addr);
    if (sendto (self->handle, buffer, length, 0,
                (struct sockaddr *) &self->broadcast, 
                sizeof (struct sockaddr_in)) == -1)
        s_handle_io_error ("sendto");
}


//  -----------------------------------------------------------------
//  Receive message from UDP broadcast
//  Returns size of received message, or -1

ssize_t
zre_udp_recv (zre_udp_t *self, byte *buffer, size_t length)
{
    assert (self);
    
    socklen_t si_len = sizeof (struct sockaddr_in);
    ssize_t size = recvfrom (self->handle,
        buffer, length, 0, (struct sockaddr *) &self->sender, &si_len);
    if (size == -1)
        s_handle_io_error ("recvfrom");

    //  Store sender address as printable string
    if (self->from)
        free (self->from);
    self->from = zmalloc (INET_ADDRSTRLEN);
    inet_ntop (AF_INET, &self->sender.sin_addr, self->from, si_len);
    
    return size;
}


//  -----------------------------------------------------------------
//  Return our own IP address as printable string

char *
zre_udp_host (zre_udp_t *self)
{
    assert (self);
    return self->host;
}


//  -----------------------------------------------------------------
//  Return IP address of peer that sent last message

char *
zre_udp_from (zre_udp_t *self)
{
    assert (self);
    return self->from;
}
