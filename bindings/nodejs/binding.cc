/*  =========================================================================
    zyre Node.js binding implementation

    Copyright (c) the Contributors as noted in the AUTHORS file.           
                                                                           
    This file is part of Zyre, an open-source framework for proximity-based
    peer-to-peer applications -- See http://zyre.org.                      
                                                                           
    This Source Code Form is subject to the terms of the Mozilla Public    
    License, v. 2.0. If a copy of the MPL was not distributed with this    
    file, You can obtain one at http://mozilla.org/MPL/2.0/.               
    =========================================================================
*/

#include "binding.h"

using namespace v8;
using namespace Nan;

NAN_MODULE_INIT (Zyre::Init) {
    Nan::HandleScope scope;

    // Prepare constructor template
    Local <FunctionTemplate> tpl = Nan::New <FunctionTemplate> (New);
    tpl->SetClassName (Nan::New ("Zyre").ToLocalChecked ());
    tpl->InstanceTemplate ()->SetInternalFieldCount (1);

    // Prototypes
    Nan::SetPrototypeMethod (tpl, "destroy", destroy);
    Nan::SetPrototypeMethod (tpl, "defined", defined);
    Nan::SetPrototypeMethod (tpl, "uuid", uuid);
    Nan::SetPrototypeMethod (tpl, "name", name);
    Nan::SetPrototypeMethod (tpl, "setHeader", set_header);
    Nan::SetPrototypeMethod (tpl, "setVerbose", set_verbose);
    Nan::SetPrototypeMethod (tpl, "setPort", set_port);
    Nan::SetPrototypeMethod (tpl, "setInterval", set_interval);
    Nan::SetPrototypeMethod (tpl, "setInterface", set_interface);
    Nan::SetPrototypeMethod (tpl, "setEndpoint", set_endpoint);
    Nan::SetPrototypeMethod (tpl, "gossipBind", gossip_bind);
    Nan::SetPrototypeMethod (tpl, "gossipConnect", gossip_connect);
    Nan::SetPrototypeMethod (tpl, "start", start);
    Nan::SetPrototypeMethod (tpl, "stop", stop);
    Nan::SetPrototypeMethod (tpl, "join", join);
    Nan::SetPrototypeMethod (tpl, "leave", leave);
    Nan::SetPrototypeMethod (tpl, "whispers", whispers);
    Nan::SetPrototypeMethod (tpl, "shouts", shouts);
    Nan::SetPrototypeMethod (tpl, "peerAddress", peer_address);
    Nan::SetPrototypeMethod (tpl, "peerHeaderValue", peer_header_value);
    Nan::SetPrototypeMethod (tpl, "print", print);
    Nan::SetPrototypeMethod (tpl, "version", version);
    Nan::SetPrototypeMethod (tpl, "test", test);

    constructor ().Reset (Nan::GetFunction (tpl).ToLocalChecked ());
    Nan::Set (target, Nan::New ("Zyre").ToLocalChecked (),
    Nan::GetFunction (tpl).ToLocalChecked ());
}

Zyre::Zyre (const char *name) {
    self = zyre_new (name);
}

Zyre::Zyre (zyre_t *self_) {
    self = self_;
}

Zyre::~Zyre () {
}

NAN_METHOD (Zyre::New) {
    assert (info.IsConstructCall ());
        char *name;
        if (info [0]->IsUndefined ())
            name = NULL;
        else
        if (!info [0]->IsString ())
            return Nan::ThrowTypeError ("`name` must be a string");
        else {
            Nan::Utf8String name_utf8 (info [0].As<String>());
            name = *name_utf8;
        }
    Zyre *zyre = new Zyre (name);
    if (zyre) {
        zyre->Wrap (info.This ());
        info.GetReturnValue ().Set (info.This ());
    }
}

NAN_METHOD (Zyre::destroy) {
    Zyre *zyre = Nan::ObjectWrap::Unwrap <Zyre> (info.Holder ());
    zyre_destroy (&zyre->self);
}


NAN_METHOD (Zyre::defined) {
    Zyre *zyre = Nan::ObjectWrap::Unwrap <Zyre> (info.Holder ());
    info.GetReturnValue ().Set (Nan::New (zyre->self != NULL));
}

NAN_METHOD (Zyre::uuid) {
    Zyre *zyre = Nan::ObjectWrap::Unwrap <Zyre> (info.Holder ());
    const char * return_value = zyre_uuid (zyre->self);
    info.GetReturnValue ().Set (Nan::New (return_value).ToLocalChecked ());
}

NAN_METHOD (Zyre::name) {
    Zyre *zyre = Nan::ObjectWrap::Unwrap <Zyre> (info.Holder ());
    const char * return_value = zyre_name (zyre->self);
    info.GetReturnValue ().Set (Nan::New (return_value).ToLocalChecked ());
}

NAN_METHOD (Zyre::set_header) {
    Zyre *zyre = Nan::ObjectWrap::Unwrap <Zyre> (info.Holder ());
        char *name;
        if (info [0]->IsUndefined ())
            return Nan::ThrowTypeError ("method requires a `name`");
        else
        if (!info [0]->IsString ())
            return Nan::ThrowTypeError ("`name` must be a string");
        else {
            Nan::Utf8String name_utf8 (info [0].As<String>());
            name = *name_utf8;
        }
        char *format;
        if (info [1]->IsUndefined ())
            return Nan::ThrowTypeError ("method requires a `format`");
        else
        if (!info [1]->IsString ())
            return Nan::ThrowTypeError ("`format` must be a string");
        else {
            Nan::Utf8String format_utf8 (info [1].As<String>());
            format = *format_utf8;
        }
    zyre_set_header (zyre->self, name, "%s", format);
}

NAN_METHOD (Zyre::set_verbose) {
    Zyre *zyre = Nan::ObjectWrap::Unwrap <Zyre> (info.Holder ());
    zyre_set_verbose (zyre->self);
}

NAN_METHOD (Zyre::set_port) {
    Zyre *zyre = Nan::ObjectWrap::Unwrap <Zyre> (info.Holder ());
        if (info [0]->IsUndefined ())
            return Nan::ThrowTypeError ("method requires a `port nbr`");
        else
        if (!info [0]->IsNumber ())
            return Nan::ThrowTypeError ("`port nbr` must be a number");
        int port_nbr = Nan::To<int>(info [0]).FromJust ();

    zyre_set_port (zyre->self, port_nbr);
}

NAN_METHOD (Zyre::set_interval) {
    Zyre *zyre = Nan::ObjectWrap::Unwrap <Zyre> (info.Holder ());
        if (info [0]->IsUndefined ())
            return Nan::ThrowTypeError ("method requires a `interval`");
        else
        if (!info [0]->IsNumber ())
            return Nan::ThrowTypeError ("`interval` must be a number");
        size_t interval = Nan::To<int64_t>(info [0]).FromJust ();

    zyre_set_interval (zyre->self, interval);
}

NAN_METHOD (Zyre::set_interface) {
    Zyre *zyre = Nan::ObjectWrap::Unwrap <Zyre> (info.Holder ());
        char *value;
        if (info [0]->IsUndefined ())
            return Nan::ThrowTypeError ("method requires a `value`");
        else
        if (!info [0]->IsString ())
            return Nan::ThrowTypeError ("`value` must be a string");
        else {
            Nan::Utf8String value_utf8 (info [0].As<String>());
            value = *value_utf8;
        }
    zyre_set_interface (zyre->self, value);
}

NAN_METHOD (Zyre::set_endpoint) {
    Zyre *zyre = Nan::ObjectWrap::Unwrap <Zyre> (info.Holder ());
        char *format;
        if (info [0]->IsUndefined ())
            return Nan::ThrowTypeError ("method requires a `format`");
        else
        if (!info [0]->IsString ())
            return Nan::ThrowTypeError ("`format` must be a string");
        else {
            Nan::Utf8String format_utf8 (info [0].As<String>());
            format = *format_utf8;
        }
    int return_value = zyre_set_endpoint (zyre->self, "%s", format);
    info.GetReturnValue().Set (Nan::New<Number>(return_value));
}

NAN_METHOD (Zyre::gossip_bind) {
    Zyre *zyre = Nan::ObjectWrap::Unwrap <Zyre> (info.Holder ());
        char *format;
        if (info [0]->IsUndefined ())
            return Nan::ThrowTypeError ("method requires a `format`");
        else
        if (!info [0]->IsString ())
            return Nan::ThrowTypeError ("`format` must be a string");
        else {
            Nan::Utf8String format_utf8 (info [0].As<String>());
            format = *format_utf8;
        }
    zyre_gossip_bind (zyre->self, "%s", format);
}

NAN_METHOD (Zyre::gossip_connect) {
    Zyre *zyre = Nan::ObjectWrap::Unwrap <Zyre> (info.Holder ());
        char *format;
        if (info [0]->IsUndefined ())
            return Nan::ThrowTypeError ("method requires a `format`");
        else
        if (!info [0]->IsString ())
            return Nan::ThrowTypeError ("`format` must be a string");
        else {
            Nan::Utf8String format_utf8 (info [0].As<String>());
            format = *format_utf8;
        }
    zyre_gossip_connect (zyre->self, "%s", format);
}

NAN_METHOD (Zyre::start) {
    Zyre *zyre = Nan::ObjectWrap::Unwrap <Zyre> (info.Holder ());
    int return_value = zyre_start (zyre->self);
    info.GetReturnValue().Set (Nan::New<Number>(return_value));
}

NAN_METHOD (Zyre::stop) {
    Zyre *zyre = Nan::ObjectWrap::Unwrap <Zyre> (info.Holder ());
    zyre_stop (zyre->self);
}

NAN_METHOD (Zyre::join) {
    Zyre *zyre = Nan::ObjectWrap::Unwrap <Zyre> (info.Holder ());
        char *group;
        if (info [0]->IsUndefined ())
            return Nan::ThrowTypeError ("method requires a `group`");
        else
        if (!info [0]->IsString ())
            return Nan::ThrowTypeError ("`group` must be a string");
        else {
            Nan::Utf8String group_utf8 (info [0].As<String>());
            group = *group_utf8;
        }
    int return_value = zyre_join (zyre->self, group);
    info.GetReturnValue().Set (Nan::New<Number>(return_value));
}

NAN_METHOD (Zyre::leave) {
    Zyre *zyre = Nan::ObjectWrap::Unwrap <Zyre> (info.Holder ());
        char *group;
        if (info [0]->IsUndefined ())
            return Nan::ThrowTypeError ("method requires a `group`");
        else
        if (!info [0]->IsString ())
            return Nan::ThrowTypeError ("`group` must be a string");
        else {
            Nan::Utf8String group_utf8 (info [0].As<String>());
            group = *group_utf8;
        }
    int return_value = zyre_leave (zyre->self, group);
    info.GetReturnValue().Set (Nan::New<Number>(return_value));
}

NAN_METHOD (Zyre::whispers) {
    Zyre *zyre = Nan::ObjectWrap::Unwrap <Zyre> (info.Holder ());
        char *peer;
        if (info [0]->IsUndefined ())
            return Nan::ThrowTypeError ("method requires a `peer`");
        else
        if (!info [0]->IsString ())
            return Nan::ThrowTypeError ("`peer` must be a string");
        else {
            Nan::Utf8String peer_utf8 (info [0].As<String>());
            peer = *peer_utf8;
        }
        char *format;
        if (info [1]->IsUndefined ())
            return Nan::ThrowTypeError ("method requires a `format`");
        else
        if (!info [1]->IsString ())
            return Nan::ThrowTypeError ("`format` must be a string");
        else {
            Nan::Utf8String format_utf8 (info [1].As<String>());
            format = *format_utf8;
        }
    int return_value = zyre_whispers (zyre->self, peer, "%s", format);
    info.GetReturnValue().Set (Nan::New<Number>(return_value));
}

NAN_METHOD (Zyre::shouts) {
    Zyre *zyre = Nan::ObjectWrap::Unwrap <Zyre> (info.Holder ());
        char *group;
        if (info [0]->IsUndefined ())
            return Nan::ThrowTypeError ("method requires a `group`");
        else
        if (!info [0]->IsString ())
            return Nan::ThrowTypeError ("`group` must be a string");
        else {
            Nan::Utf8String group_utf8 (info [0].As<String>());
            group = *group_utf8;
        }
        char *format;
        if (info [1]->IsUndefined ())
            return Nan::ThrowTypeError ("method requires a `format`");
        else
        if (!info [1]->IsString ())
            return Nan::ThrowTypeError ("`format` must be a string");
        else {
            Nan::Utf8String format_utf8 (info [1].As<String>());
            format = *format_utf8;
        }
    int return_value = zyre_shouts (zyre->self, group, "%s", format);
    info.GetReturnValue().Set (Nan::New<Number>(return_value));
}

NAN_METHOD (Zyre::peer_address) {
    Zyre *zyre = Nan::ObjectWrap::Unwrap <Zyre> (info.Holder ());
        char *peer;
        if (info [0]->IsUndefined ())
            return Nan::ThrowTypeError ("method requires a `peer`");
        else
        if (!info [0]->IsString ())
            return Nan::ThrowTypeError ("`peer` must be a string");
        else {
            Nan::Utf8String peer_utf8 (info [0].As<String>());
            peer = *peer_utf8;
        }
    char * return_value = zyre_peer_address (zyre->self, peer);
    info.GetReturnValue ().Set (Nan::New (return_value).ToLocalChecked ());
}

NAN_METHOD (Zyre::peer_header_value) {
    Zyre *zyre = Nan::ObjectWrap::Unwrap <Zyre> (info.Holder ());
        char *peer;
        if (info [0]->IsUndefined ())
            return Nan::ThrowTypeError ("method requires a `peer`");
        else
        if (!info [0]->IsString ())
            return Nan::ThrowTypeError ("`peer` must be a string");
        else {
            Nan::Utf8String peer_utf8 (info [0].As<String>());
            peer = *peer_utf8;
        }
        char *name;
        if (info [1]->IsUndefined ())
            return Nan::ThrowTypeError ("method requires a `name`");
        else
        if (!info [1]->IsString ())
            return Nan::ThrowTypeError ("`name` must be a string");
        else {
            Nan::Utf8String name_utf8 (info [1].As<String>());
            name = *name_utf8;
        }
    char * return_value = zyre_peer_header_value (zyre->self, peer, name);
    info.GetReturnValue ().Set (Nan::New (return_value).ToLocalChecked ());
}

NAN_METHOD (Zyre::print) {
    Zyre *zyre = Nan::ObjectWrap::Unwrap <Zyre> (info.Holder ());
    zyre_print (zyre->self);
}

NAN_METHOD (Zyre::version) {
    uint64_t return_value = zyre_version ();
    info.GetReturnValue().Set (Nan::New<Number>(return_value));
}

NAN_METHOD (Zyre::test) {
        if (info [0]->IsUndefined ())
            return Nan::ThrowTypeError ("method requires a `verbose`");
        else
        if (!info [0]->IsBoolean ())
            return Nan::ThrowTypeError ("`verbose` must be a number");
        bool verbose = Nan::To<bool>(info [0]).FromJust ();

    zyre_test (verbose);
}

Nan::Persistent <Function> &Zyre::constructor () {
    static Nan::Persistent <Function> my_constructor;
    return my_constructor;
}


zyre_t *Zyre::get_self () {
    return self;
}

NAN_MODULE_INIT (ZyreEvent::Init) {
    Nan::HandleScope scope;

    // Prepare constructor template
    Local <FunctionTemplate> tpl = Nan::New <FunctionTemplate> (New);
    tpl->SetClassName (Nan::New ("ZyreEvent").ToLocalChecked ());
    tpl->InstanceTemplate ()->SetInternalFieldCount (1);

    // Prototypes
    Nan::SetPrototypeMethod (tpl, "destroy", destroy);
    Nan::SetPrototypeMethod (tpl, "defined", defined);
    Nan::SetPrototypeMethod (tpl, "type", type);
    Nan::SetPrototypeMethod (tpl, "peerUuid", peer_uuid);
    Nan::SetPrototypeMethod (tpl, "peerName", peer_name);
    Nan::SetPrototypeMethod (tpl, "peerAddr", peer_addr);
    Nan::SetPrototypeMethod (tpl, "header", header);
    Nan::SetPrototypeMethod (tpl, "group", group);
    Nan::SetPrototypeMethod (tpl, "print", print);
    Nan::SetPrototypeMethod (tpl, "test", test);

    constructor ().Reset (Nan::GetFunction (tpl).ToLocalChecked ());
    Nan::Set (target, Nan::New ("ZyreEvent").ToLocalChecked (),
    Nan::GetFunction (tpl).ToLocalChecked ());
}

ZyreEvent::ZyreEvent (zyre_t *node) {
    self = zyre_event_new (node);
}

ZyreEvent::ZyreEvent (zyre_event_t *self_) {
    self = self_;
}

ZyreEvent::~ZyreEvent () {
}

NAN_METHOD (ZyreEvent::New) {
    assert (info.IsConstructCall ());
        Zyre *node = Nan::ObjectWrap::Unwrap<Zyre>(info [0].As<Object>());

    ZyreEvent *zyre_event = new ZyreEvent (node->get_self ());
    if (zyre_event) {
        zyre_event->Wrap (info.This ());
        info.GetReturnValue ().Set (info.This ());
    }
}

NAN_METHOD (ZyreEvent::destroy) {
    ZyreEvent *zyre_event = Nan::ObjectWrap::Unwrap <ZyreEvent> (info.Holder ());
    zyre_event_destroy (&zyre_event->self);
}


NAN_METHOD (ZyreEvent::defined) {
    ZyreEvent *zyre_event = Nan::ObjectWrap::Unwrap <ZyreEvent> (info.Holder ());
    info.GetReturnValue ().Set (Nan::New (zyre_event->self != NULL));
}

NAN_METHOD (ZyreEvent::type) {
    ZyreEvent *zyre_event = Nan::ObjectWrap::Unwrap <ZyreEvent> (info.Holder ());
    const char * return_value = zyre_event_type (zyre_event->self);
    info.GetReturnValue ().Set (Nan::New (return_value).ToLocalChecked ());
}

NAN_METHOD (ZyreEvent::peer_uuid) {
    ZyreEvent *zyre_event = Nan::ObjectWrap::Unwrap <ZyreEvent> (info.Holder ());
    const char * return_value = zyre_event_peer_uuid (zyre_event->self);
    info.GetReturnValue ().Set (Nan::New (return_value).ToLocalChecked ());
}

NAN_METHOD (ZyreEvent::peer_name) {
    ZyreEvent *zyre_event = Nan::ObjectWrap::Unwrap <ZyreEvent> (info.Holder ());
    const char * return_value = zyre_event_peer_name (zyre_event->self);
    info.GetReturnValue ().Set (Nan::New (return_value).ToLocalChecked ());
}

NAN_METHOD (ZyreEvent::peer_addr) {
    ZyreEvent *zyre_event = Nan::ObjectWrap::Unwrap <ZyreEvent> (info.Holder ());
    const char * return_value = zyre_event_peer_addr (zyre_event->self);
    info.GetReturnValue ().Set (Nan::New (return_value).ToLocalChecked ());
}

NAN_METHOD (ZyreEvent::header) {
    ZyreEvent *zyre_event = Nan::ObjectWrap::Unwrap <ZyreEvent> (info.Holder ());
        char *name;
        if (info [0]->IsUndefined ())
            return Nan::ThrowTypeError ("method requires a `name`");
        else
        if (!info [0]->IsString ())
            return Nan::ThrowTypeError ("`name` must be a string");
        else {
            Nan::Utf8String name_utf8 (info [0].As<String>());
            name = *name_utf8;
        }
    const char * return_value = zyre_event_header (zyre_event->self, name);
    info.GetReturnValue ().Set (Nan::New (return_value).ToLocalChecked ());
}

NAN_METHOD (ZyreEvent::group) {
    ZyreEvent *zyre_event = Nan::ObjectWrap::Unwrap <ZyreEvent> (info.Holder ());
    const char * return_value = zyre_event_group (zyre_event->self);
    info.GetReturnValue ().Set (Nan::New (return_value).ToLocalChecked ());
}

NAN_METHOD (ZyreEvent::print) {
    ZyreEvent *zyre_event = Nan::ObjectWrap::Unwrap <ZyreEvent> (info.Holder ());
    zyre_event_print (zyre_event->self);
}

NAN_METHOD (ZyreEvent::test) {
        if (info [0]->IsUndefined ())
            return Nan::ThrowTypeError ("method requires a `verbose`");
        else
        if (!info [0]->IsBoolean ())
            return Nan::ThrowTypeError ("`verbose` must be a number");
        bool verbose = Nan::To<bool>(info [0]).FromJust ();

    zyre_event_test (verbose);
}

Nan::Persistent <Function> &ZyreEvent::constructor () {
    static Nan::Persistent <Function> my_constructor;
    return my_constructor;
}


zyre_event_t *ZyreEvent::get_self () {
    return self;
}

extern "C" NAN_MODULE_INIT (zyre_initialize)
{
    Zyre::Init (target);
    ZyreEvent::Init (target);
}

NODE_MODULE (zyre, zyre_initialize)
