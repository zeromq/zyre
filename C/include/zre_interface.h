#ifndef __ZRE_INTERFACE_H_INCLUDED__
#define __ZRE_INTERFACE_H_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _zre_interface_t zre_interface_t;

//  Constructor
zre_interface_t *
    zre_interface_new (void);

//  Destructor
void
    zre_interface_destroy (zre_interface_t **self_p);

zmsg_t *
    zre_interface_recv (zre_interface_t *self);

#ifdef __cplusplus
}
#endif

#endif
