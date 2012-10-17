#include <czmq.h>
#include "../include/zre.h"

//  -----------------------------------------------------------------
//  UDP instance

struct _zre_udp_t {
    int handle;                 //  Socket for send/recv
    int port_nbr;               //  UDP port number we work on
    struct sockaddr_in address;     //  Own address
    struct sockaddr_in broadcast;   //  Broadcast address
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
        zclock_log ("E: (udp) error '%s' on %s", strerror (errno), reason);
        assert (false);
    }
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

    if (setsockopt (self->handle, SOL_SOCKET,
                    SO_REUSEPORT, &on, sizeof (on)) == -1)
        s_handle_io_error ("setsockopt (SO_REUSEPORT)");

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
            }
            interface = interface->ifa_next;
        }
    }
    freeifaddrs (interfaces);
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
        free (self);
        *self_p = NULL;
    }
}


//  Returns UDP socket handle

int
zre_udp_handle (zre_udp_t *self)
{
    assert (self);
    return self->handle;
}

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

//  Receive message from UDP broadcast
//  Returns size of received message, or -1

ssize_t
zre_udp_recv (zre_udp_t *self, byte *buffer, size_t length)
{
    assert (self);
    
    struct sockaddr_in sockaddr;
    socklen_t si_len = sizeof (struct sockaddr_in);

    ssize_t size = recvfrom (self->handle, buffer, length, 0, 
                             (struct sockaddr *) &sockaddr, &si_len);
    if (size == -1)
        s_handle_io_error ("recvfrom");

    return size;
}
