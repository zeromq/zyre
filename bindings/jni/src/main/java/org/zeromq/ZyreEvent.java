package org.zeromq.zyre;
import org.zeromq.zyre.Zhash;
import org.zeromq.zyre.Zlist;
import org.zeromq.zyre.Zmsg;

public class ZyreEvent {
    public native void create ();
    public native void destroy ();
    public native void print ();
    public native String sender ();
    public native String name ();
    public native String address ();
    public native Zhash headers ();
    public native String header (String Name);
    public native String group ();
    public native Zmsg msg ();
    public native void test (boolean Verbose);
}
