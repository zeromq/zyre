/*  =========================================================================
    zyre Node.js binding header file

    Copyright (c) the Contributors as noted in the AUTHORS file.           
                                                                           
    This file is part of Zyre, an open-source framework for proximity-based
    peer-to-peer applications -- See http://zyre.org.                      
                                                                           
    This Source Code Form is subject to the terms of the Mozilla Public    
    License, v. 2.0. If a copy of the MPL was not distributed with this    
    file, You can obtain one at http://mozilla.org/MPL/2.0/.               
    =========================================================================
*/

#ifndef ZYRE_BINDING_H_INCLUDED
#define ZYRE_BINDING_H_INCLUDED

#include "zyre.h"
#include "nan.h"

class Zyre: public Nan::ObjectWrap {
    public:
        static NAN_MODULE_INIT (Init);
    private:
        explicit Zyre (const char *name);
        ~Zyre ();

    static NAN_METHOD (New);
    static NAN_METHOD (destroy);
    static NAN_METHOD (defined);
    static NAN_METHOD (uuid);
    static NAN_METHOD (name);
    static NAN_METHOD (set_header);
    static NAN_METHOD (set_verbose);
    static NAN_METHOD (set_port);
    static NAN_METHOD (set_interval);
    static NAN_METHOD (set_interface);
    static NAN_METHOD (set_endpoint);
    static NAN_METHOD (gossip_bind);
    static NAN_METHOD (gossip_connect);
    static NAN_METHOD (start);
    static NAN_METHOD (stop);
    static NAN_METHOD (join);
    static NAN_METHOD (leave);
    static NAN_METHOD (recv);
    static NAN_METHOD (whispers);
    static NAN_METHOD (shouts);
    static NAN_METHOD (peers);
    static NAN_METHOD (own_groups);
    static NAN_METHOD (peer_groups);
    static NAN_METHOD (peer_address);
    static NAN_METHOD (peer_header_value);
    static NAN_METHOD (socket);
    static NAN_METHOD (print);
    static NAN_METHOD (version);
    static NAN_METHOD (test);
    zyre_t *self;
    public:
        zyre_t *get_self ();
};

class ZyreEvent: public Nan::ObjectWrap {
    public:
        static NAN_MODULE_INIT (Init);
    private:
        explicit ZyreEvent (zyre_t *node);
        ~ZyreEvent ();

    static NAN_METHOD (New);
    static NAN_METHOD (destroy);
    static NAN_METHOD (defined);
    static NAN_METHOD (type);
    static NAN_METHOD (peer_uuid);
    static NAN_METHOD (peer_name);
    static NAN_METHOD (peer_addr);
    static NAN_METHOD (headers);
    static NAN_METHOD (header);
    static NAN_METHOD (group);
    static NAN_METHOD (msg);
    static NAN_METHOD (print);
    static NAN_METHOD (test);
    zyre_event_t *self;
    public:
        zyre_event_t *get_self ();
};

#endif
