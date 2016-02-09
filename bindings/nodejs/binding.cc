#include "zyre.h"
#include "nan.h"

using namespace v8;

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
            Nan::SetPrototypeMethod (tpl, "uuid", uuid);
            Nan::SetPrototypeMethod (tpl, "name", name);
            Nan::SetPrototypeMethod (tpl, "start", start);
            Nan::SetPrototypeMethod (tpl, "stop", stop);
            Nan::SetPrototypeMethod (tpl, "setHeader", set_header);
            Nan::SetPrototypeMethod (tpl, "setVerbose", set_verbose);
            Nan::SetPrototypeMethod (tpl, "join", join);
            Nan::SetPrototypeMethod (tpl, "leave", leave);
            Nan::SetPrototypeMethod (tpl, "print", print);
            Nan::SetPrototypeMethod (tpl, "whispers", whispers);
            Nan::SetPrototypeMethod (tpl, "shouts", shouts);
            Nan::SetPrototypeMethod (tpl, "recv", recv);

            constructor ().Reset (Nan::GetFunction (tpl).ToLocalChecked ());
            Nan::Set (target, Nan::New ("Zyre").ToLocalChecked (),
            Nan::GetFunction (tpl).ToLocalChecked ());
        }
    private:
        explicit Zyre (char *name = NULL) {
            self = zyre_new (name);
            assert (self);
        }
        ~Zyre () {
        }

    static NAN_METHOD (New) {
        assert (info.IsConstructCall ());
        Zyre *obj;
        if (info [0]->IsString ()) {
            Nan::Utf8String name (info [0].As<String>());
            obj = new Zyre (*name);
        }
        else
            obj = new Zyre ();
        obj->Wrap (info.This ());
        info.GetReturnValue ().Set (info.This ());
    }

    static NAN_METHOD (destroy) {
        Zyre *obj = Nan::ObjectWrap::Unwrap <Zyre> (info.Holder ());
        zyre_destroy (&obj->self);
    }

    static NAN_METHOD (uuid) {
        Zyre *obj = Nan::ObjectWrap::Unwrap <Zyre> (info.Holder ());
        info.GetReturnValue ().Set (Nan::New (zyre_uuid (obj->self)).ToLocalChecked ());
    }

    static NAN_METHOD (name) {
        Zyre *obj = Nan::ObjectWrap::Unwrap <Zyre> (info.Holder ());
        info.GetReturnValue ().Set (Nan::New (zyre_name (obj->self)).ToLocalChecked ());
    }

    static NAN_METHOD (start) {
        Zyre *obj = Nan::ObjectWrap::Unwrap <Zyre> (info.Holder ());
        zyre_start (obj->self);
    }

    static NAN_METHOD (stop) {
        Zyre *obj = Nan::ObjectWrap::Unwrap <Zyre> (info.Holder ());
        zyre_stop (obj->self);
    }

    static NAN_METHOD (set_verbose) {
        Zyre *obj = Nan::ObjectWrap::Unwrap <Zyre> (info.Holder ());
        zyre_set_verbose (obj->self);
    }

    static NAN_METHOD (set_header) {
        if (info [0]->IsString ()
        &&  info [1]->IsString ()) {
            Zyre *obj = Nan::ObjectWrap::Unwrap <Zyre> (info.Holder ());
            Nan::Utf8String name (info [0].As<String>());
            Nan::Utf8String value (info [0].As<String>());
            zyre_set_header (obj->self, *name, "%s", *value);
        }
        else
            return Nan::ThrowTypeError (".set_header() expects name and value as strings");
    }

    static NAN_METHOD (join) {
        if (info [0]->IsString ()) {
            Zyre *obj = Nan::ObjectWrap::Unwrap <Zyre> (info.Holder ());
            Nan::Utf8String group (info [0].As<String>());
            zyre_join (obj->self, *group);
        }
        else
            return Nan::ThrowTypeError (".join() expects group as string");
    }

    static NAN_METHOD (leave) {
        if (info [0]->IsString ()) {
            Zyre *obj = Nan::ObjectWrap::Unwrap <Zyre> (info.Holder ());
            Nan::Utf8String group (info [0].As<String>());
            zyre_leave (obj->self, *group);
        }
        else
            return Nan::ThrowTypeError (".leave() expects group as string");
    }

    static NAN_METHOD (whispers) {
        if (info [0]->IsString ()
        &&  info [1]->IsString ()) {
            Zyre *obj = Nan::ObjectWrap::Unwrap <Zyre> (info.Holder ());
            Nan::Utf8String peer (info [0].As<String>());
            Nan::Utf8String message (info [0].As<String>());
            zyre_shouts (obj->self, *peer, "%s", *message);
        }
        else
            return Nan::ThrowTypeError (".set_header() expects peer and message as strings");
    }

    static NAN_METHOD (shouts) {
        if (info [0]->IsString ()
        &&  info [1]->IsString ()) {
            Zyre *obj = Nan::ObjectWrap::Unwrap <Zyre> (info.Holder ());
            Nan::Utf8String group (info [0].As<String>());
            Nan::Utf8String message (info [0].As<String>());
            zyre_shouts (obj->self, *group, "%s", *message);
        }
        else
            return Nan::ThrowTypeError (".set_header() expects group and message as strings");
    }

    static NAN_METHOD (recv) {
        Zyre *obj = Nan::ObjectWrap::Unwrap <Zyre> (info.Holder ());
        zmsg_t *msg = zyre_recv (obj->self);
        if (msg) {
            char *string = zmsg_popstr (msg);
            info.GetReturnValue ().Set (Nan::New (string).ToLocalChecked ());
            free (string);
        }
    }

    static NAN_METHOD (print) {
        Zyre *obj = Nan::ObjectWrap::Unwrap <Zyre> (info.Holder ());
        zyre_print (obj->self);
    }

    static Nan::Persistent <Function> & constructor () {
        static Nan::Persistent <Function> my_constructor;
        return my_constructor;
    }

    zyre_t *self;
};

NODE_MODULE (zyre, Zyre::Init)
