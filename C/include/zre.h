#ifndef __ZRE_H_INCLUDED__
#define __ZRE_H_INCLUDED__

#define ZRE_VERSION_MAJOR 1
#define ZRE_VERSION_MINOR 0
#define ZRE_VERSION_PATCH 0

#define ZRE_MAKE_VERSION(major, minor, patch) \
    ((major) * 10000 + (minor) * 100 + (patch))
#define ZRE_VERSION \
    ZRE_MAKE_VERSION(ZRE_VERSION_MAJOR, ZRE_VERSION_MINOR, ZRE_VERSION_PATCH)

#include "zre_udp.h"
#include "zre_interface.h"


#endif
