#include "zyre.h"
#include "nan.h"

using namespace v8;
using namespace Nan;

class Zyre: public Nan::ObjectWrap {
    public:
        static NAN_MODULE_INIT (Init) {
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
            Nan::SetPrototypeMethod (tpl, "start", start);
            Nan::SetPrototypeMethod (tpl, "stop", stop);
            Nan::SetPrototypeMethod (tpl, "setHeader", set_header);
            Nan::SetPrototypeMethod (tpl, "setVerbose", set_verbose);
            Nan::SetPrototypeMethod (tpl, "join", join);
            Nan::SetPrototypeMethod (tpl, "leave", leave);
            Nan::SetPrototypeMethod (tpl, "print", print);
            Nan::SetPrototypeMethod (tpl, "whisper", whisper);
            Nan::SetPrototypeMethod (tpl, "shout", shout);
            Nan::SetPrototypeMethod (tpl, "recv", recv);

            constructor ().Reset (Nan::GetFunction (tpl).ToLocalChecked ());
            Nan::Set (target, Nan::New ("Zyre").ToLocalChecked (),
            Nan::GetFunction (tpl).ToLocalChecked ());
        }
    private:
        explicit Zyre (char *name = NULL) {
            self = zyre_new (name);
        }
        ~Zyre () {
        }

    static NAN_METHOD (New) {
        assert (info.IsConstructCall ());
        Zyre *zyre;
        if (info [0]->IsString ()) {
            Nan::Utf8String name (info [0].As<String>());
            zyre = new Zyre (*name);
        }
        else
            zyre = new Zyre ();

        if (zyre) {
            zyre->Wrap (info.This ());
            info.GetReturnValue ().Set (info.This ());
        }
    }

    static NAN_METHOD (destroy) {
        Zyre *zyre = Nan::ObjectWrap::Unwrap <Zyre> (info.Holder ());
        zyre_destroy (&zyre->self);
    }

    static NAN_METHOD (defined) {
        Zyre *zyre = Nan::ObjectWrap::Unwrap <Zyre> (info.Holder ());
        info.GetReturnValue ().Set (Nan::New (zyre->self != NULL));
    }

    static NAN_METHOD (uuid) {
        Zyre *zyre = Nan::ObjectWrap::Unwrap <Zyre> (info.Holder ());
        info.GetReturnValue ().Set (Nan::New (zyre_uuid (zyre->self)).ToLocalChecked ());
    }

    static NAN_METHOD (name) {
        Zyre *zyre = Nan::ObjectWrap::Unwrap <Zyre> (info.Holder ());
        info.GetReturnValue ().Set (Nan::New (zyre_name (zyre->self)).ToLocalChecked ());
    }

    static NAN_METHOD (start) {
        Zyre *zyre = Nan::ObjectWrap::Unwrap <Zyre> (info.Holder ());
        zyre_start (zyre->self);
    }

    static NAN_METHOD (stop) {
        Zyre *zyre = Nan::ObjectWrap::Unwrap <Zyre> (info.Holder ());
        zyre_stop (zyre->self);
    }

    static NAN_METHOD (set_verbose) {
        Zyre *zyre = Nan::ObjectWrap::Unwrap <Zyre> (info.Holder ());
        zyre_set_verbose (zyre->self);
    }

    static NAN_METHOD (set_header) {
        if (info [0]->IsString ()
        &&  info [1]->IsString ()) {
            Zyre *zyre = Nan::ObjectWrap::Unwrap <Zyre> (info.Holder ());
            Nan::Utf8String name (info [0].As<String>());
            Nan::Utf8String value (info [0].As<String>());
            zyre_set_header (zyre->self, *name, "%s", *value);
        }
        else
            return Nan::ThrowTypeError (".set_header() expects name and value as strings");
    }

    static NAN_METHOD (join) {
        if (info [0]->IsString ()) {
            Zyre *zyre = Nan::ObjectWrap::Unwrap <Zyre> (info.Holder ());
            Nan::Utf8String group (info [0].As<String>());
            zyre_join (zyre->self, *group);
        }
        else
            return Nan::ThrowTypeError (".join() expects group as string");
    }

    static NAN_METHOD (leave) {
        if (info [0]->IsString ()) {
            Zyre *zyre = Nan::ObjectWrap::Unwrap <Zyre> (info.Holder ());
            Nan::Utf8String group (info [0].As<String>());
            zyre_leave (zyre->self, *group);
        }
        else
            return Nan::ThrowTypeError (".leave() expects group as string");
    }

    static NAN_METHOD (whisper) {
        if (info [0]->IsString ()
        &&  info [1]->IsString ()) {
            Zyre *zyre = Nan::ObjectWrap::Unwrap <Zyre> (info.Holder ());
            Nan::Utf8String peer (info [0].As<String>());
            Nan::Utf8String string (info [0].As<String>());
            zmsg_t *msg = zmsg_new ();
            zmsg_pushstr (msg, *string);
            zyre_whisper (zyre->self, *peer, &msg);
        }
        else
            return Nan::ThrowTypeError (".whisper() expects peer and message as strings");
    }

    static NAN_METHOD (shout) {
        if (info [0]->IsString ()
        &&  info [1]->IsString ()) {
            Zyre *zyre = Nan::ObjectWrap::Unwrap <Zyre> (info.Holder ());
            Nan::Utf8String group (info [0].As<String>());
            Nan::Utf8String string (info [0].As<String>());
            zmsg_t *msg = zmsg_new ();
            zmsg_pushstr (msg, *string);
            zyre_shout (zyre->self, *group, &msg);
        }
        else
            return Nan::ThrowTypeError (".shout() expects group and message as strings");
    }

    static NAN_METHOD (recv) {
        Zyre *zyre = Nan::ObjectWrap::Unwrap <Zyre> (info.Holder ());
        zmsg_t *msg = zyre_recv (zyre->self);
        if (msg) {
            char *string = zmsg_popstr (msg);
            info.GetReturnValue ().Set (Nan::New (string).ToLocalChecked ());
            free (string);
        }
    }

    static NAN_METHOD (print) {
        Zyre *zyre = Nan::ObjectWrap::Unwrap <Zyre> (info.Holder ());
        zyre_print (zyre->self);
    }

    static Nan::Persistent <Function> & constructor () {
        static Nan::Persistent <Function> my_constructor;
        return my_constructor;
    }

    zyre_t *self;
    public:
        zyre_t *get_self () {
            return self;
        }
};

class ZyreEvent: public Nan::ObjectWrap {
    public:
        static NAN_MODULE_INIT (Init) {
            Nan::HandleScope scope;

            // Prepare constructor template
            Local <FunctionTemplate> tpl = Nan::New <FunctionTemplate> (New);
            tpl->SetClassName (Nan::New ("ZyreEvent").ToLocalChecked ());
            tpl->InstanceTemplate ()->SetInternalFieldCount (1);

            // Prototypes
            Nan::SetPrototypeMethod (tpl, "destroy", destroy);
            Nan::SetPrototypeMethod (tpl, "defined", defined);
            Nan::SetPrototypeMethod (tpl, "type", type);
            Nan::SetPrototypeMethod (tpl, "type_name", type_name);
            Nan::SetPrototypeMethod (tpl, "peer_uuid", peer_uuid);
            Nan::SetPrototypeMethod (tpl, "peer_name", peer_name);
            Nan::SetPrototypeMethod (tpl, "peer_addr", peer_addr);
            Nan::SetPrototypeMethod (tpl, "header", header);
            Nan::SetPrototypeMethod (tpl, "group", group);
            Nan::SetPrototypeMethod (tpl, "msg", msg);
            Nan::SetPrototypeMethod (tpl, "print", print);

            constructor ().Reset (Nan::GetFunction (tpl).ToLocalChecked ());
            Nan::Set (target, Nan::New ("ZyreEvent").ToLocalChecked (),
            Nan::GetFunction (tpl).ToLocalChecked ());
        }
    private:
        explicit ZyreEvent (Zyre *zyre) {
            self = zyre_event_new (zyre->get_self ());
        }
        ~ZyreEvent () {
        }

    static NAN_METHOD (New) {
        assert (info.IsConstructCall ());
        if (info [0]->IsObject ()) {
            Zyre *zyre = Nan::ObjectWrap::Unwrap<Zyre> (info [0].As<Object>());
            ZyreEvent *zyre_event = new ZyreEvent (zyre);
            zyre_event->Wrap (info.This ());
            info.GetReturnValue ().Set (info.This ());
        }
        else
            return Nan::ThrowTypeError ("New ZyreEvent expects Zyre argument");
    }

    static NAN_METHOD (destroy) {
        ZyreEvent *zyre_event = Nan::ObjectWrap::Unwrap <ZyreEvent> (info.Holder ());
        zyre_event_destroy (&zyre_event->self);
    }

    static NAN_METHOD (defined) {
        ZyreEvent *zyre_event = Nan::ObjectWrap::Unwrap <ZyreEvent> (info.Holder ());
        info.GetReturnValue ().Set (Nan::New (zyre_event->self != NULL));
    }

    static NAN_METHOD (type) {
        ZyreEvent *zyre_event = Nan::ObjectWrap::Unwrap <ZyreEvent> (info.Holder ());
        info.GetReturnValue ().Set (Nan::New (zyre_event_type (zyre_event->self)));
    }

    static NAN_METHOD (type_name) {
        ZyreEvent *zyre_event = Nan::ObjectWrap::Unwrap <ZyreEvent> (info.Holder ());
        int type = zyre_event_type (zyre_event->self);
        const char *type_name = "";
        switch (type) {
            case 1: type_name = "ENTER"; break;
            case 2: type_name = "JOIN"; break;
            case 3: type_name = "LEAVE"; break;
            case 4: type_name = "EXIT"; break;
            case 5: type_name = "WHISPER"; break;
            case 6: type_name = "SHOUT"; break;
            case 7: type_name = "STOP"; break;
            case 8: type_name = "EVASIVE"; break;
        }
        info.GetReturnValue ().Set (Nan::New (type_name).ToLocalChecked ());
    }

    static NAN_METHOD (peer_uuid) {
        ZyreEvent *zyre_event = Nan::ObjectWrap::Unwrap <ZyreEvent> (info.Holder ());
        info.GetReturnValue ().Set (Nan::New (zyre_event_peer_uuid (zyre_event->self)).ToLocalChecked ());
    }

    static NAN_METHOD (peer_name) {
        ZyreEvent *zyre_event = Nan::ObjectWrap::Unwrap <ZyreEvent> (info.Holder ());
        info.GetReturnValue ().Set (Nan::New (zyre_event_peer_name (zyre_event->self)).ToLocalChecked ());
    }

    static NAN_METHOD (peer_addr) {
        ZyreEvent *zyre_event = Nan::ObjectWrap::Unwrap <ZyreEvent> (info.Holder ());
        info.GetReturnValue ().Set (Nan::New (zyre_event_peer_addr (zyre_event->self)).ToLocalChecked ());
    }

    static NAN_METHOD (header) {
        if (info [0]->IsString ()) {
            ZyreEvent *zyre_event = Nan::ObjectWrap::Unwrap <ZyreEvent> (info.Holder ());
            Nan::Utf8String name (info [0].As<String>());
            info.GetReturnValue ().Set (Nan::New (zyre_event_header (zyre_event->self, *name)).ToLocalChecked ());
        }
        else
            return Nan::ThrowTypeError (".header() expects name as string");
    }

    static NAN_METHOD (group) {
        ZyreEvent *zyre_event = Nan::ObjectWrap::Unwrap <ZyreEvent> (info.Holder ());
        info.GetReturnValue ().Set (Nan::New (zyre_event_group (zyre_event->self)).ToLocalChecked ());
    }

    static NAN_METHOD (msg) {
        ZyreEvent *zyre_event = Nan::ObjectWrap::Unwrap <ZyreEvent> (info.Holder ());
        info.GetReturnValue ().Set (Nan::New (zyre_event_group (zyre_event->self)).ToLocalChecked ());
        zmsg_t *msg = zmsg_dup (zyre_event_msg (zyre_event->self));
        if (msg) {
            char *string = zmsg_popstr (msg);
            info.GetReturnValue ().Set (Nan::New (string).ToLocalChecked ());
            free (string);
            zmsg_destroy (&msg);
        }
    }

    static NAN_METHOD (print) {
        ZyreEvent *zyre_event = Nan::ObjectWrap::Unwrap <ZyreEvent> (info.Holder ());
        zyre_event_print (zyre_event->self);
    }

    static Nan::Persistent <Function> & constructor () {
        static Nan::Persistent <Function> my_constructor;
        return my_constructor;
    }

    zyre_event_t *self;
    public:
        zyre_event_t *get_self () {
            return self;
        }
};


// <class name = "zyre event">
//     <!--
//     Copyright (c) the Contributors as noted in the AUTHORS file.
//
//     This file is part of Zyre, an open-source framework for proximity-based
//     peer-to-peer applications -- See http://zyre.org.
//
//     This Source Code Form is subject to the terms of the Mozilla Public
//     License, v. 2.0. If a copy of the MPL was not distributed with this
//     file, You can obtain one at http://mozilla.org/MPL/2.0/.
//     -->
//     Parsing Zyre messages
//
//     <enum name = "type">
//         <constant name = "enter" value = "1" />
//         <constant name = "join" value = "2" />
//         <constant name = "leave" value = "3" />
//         <constant name = "exit" value = "4" />
//         <constant name = "whisper" value = "5" />
//         <constant name = "shout" value = "6" />
//         <constant name = "stop" value = "7" />
//         <constant name = "evasive" value = "8" />
//     </enum>
//
//     <method name = "header">
//         Returns value of a header from the message headers
//         obtained by ENTER. Return NULL if no value was found.
//         <argument name = "name" type = "string" />
//         <return type = "string" />
//     </method>
//
//     <method name = "group">
//         Returns the group name that a SHOUT event was sent to
//         <return type = "string" />
//     </method>
//
//     <method name = "msg">
//         Returns the incoming message payload (currently one frame)
//         <return type = "zmsg" />
//     </method>
//
//     <method name = "print">
//         Print event to zsys log
//     </method>
// </class>


extern "C" NAN_MODULE_INIT (zyre_initialize)
{
    Zyre::Init (target);
    ZyreEvent::Init (target);
}


NODE_MODULE (zyre, zyre_initialize)
