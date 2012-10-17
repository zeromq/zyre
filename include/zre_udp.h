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

#ifdef __cplusplus
}
#endif

#endif
