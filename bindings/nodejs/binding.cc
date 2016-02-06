#include <nan.h>
#include "zyre.h"

class Zyre: public Nan::ObjectWrap {
    public:
        static NAN_MODULE_INIT (Init) {
            Nan::HandleScope scope;

            // Prepare constructor template
            v8::Local <v8::FunctionTemplate> tpl = Nan::New <v8::FunctionTemplate> (New);
            tpl->SetClassName (Nan::New ("Zyre").ToLocalChecked ());
            tpl->InstanceTemplate ()->SetInternalFieldCount (1);

            // Prototypes
            Nan::SetPrototypeMethod (tpl, "uuid", uuid);
            Nan::SetPrototypeMethod (tpl, "name", name);
            Nan::SetPrototypeMethod (tpl, "destroy", destroy);

            constructor ().Reset (Nan::GetFunction (tpl).ToLocalChecked ());
            Nan::Set (target, Nan::New ("Zyre").ToLocalChecked (),
            Nan::GetFunction (tpl).ToLocalChecked ());
        }
    private:
        explicit Zyre (char *name = NULL) {
            self = zyre_new (name);
            printf ("CONSTRUCTOR self=%p\n", self);
        }
        ~Zyre () {
        }

    static NAN_METHOD (New) {
        if (info.IsConstructCall ()) {
            // Invoked as constructor: `new Zyre (...)`
            Zyre *obj = new Zyre ();
            obj->Wrap (info.This ());
            info.GetReturnValue ().Set (info.This ());
        }
        else {
            // Invoked as plain function `Zyre (...)`, turn into construct call.
            Nan::HandleScope scope;
            const int argc = 1;
            v8::Local <v8::Value> argv [argc] = { info [0] };
            v8::Local <v8::Function> cons = Nan::New (constructor ());
            info.GetReturnValue ().Set (cons->NewInstance (argc, argv));
        }
    }

    static NAN_METHOD (uuid) {
        Zyre *obj = Nan::ObjectWrap::Unwrap <Zyre> (info.Holder ());
        info.GetReturnValue ().Set (Nan::New (zyre_uuid (obj->self)).ToLocalChecked ());
    }

    static NAN_METHOD (name) {
        Zyre *obj = Nan::ObjectWrap::Unwrap <Zyre> (info.Holder ());
        info.GetReturnValue ().Set (Nan::New (zyre_name (obj->self)).ToLocalChecked ());
    }

    static NAN_METHOD (destroy) {
        Zyre *obj = Nan::ObjectWrap::Unwrap <Zyre> (info.Holder ());
        printf ("DESTRUCTOR self=%p\n", obj->self);
        zyre_destroy (&obj->self);
    }

    static Nan::Persistent <v8::Function> & constructor () {
        static Nan::Persistent <v8::Function> my_constructor;
        return my_constructor;
    }

    zyre_t *self;
};

NODE_MODULE (zyre, Zyre::Init)
