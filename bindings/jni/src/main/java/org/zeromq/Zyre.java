package org.zeromq.zyre;
import org.zeromq.zyre.Zhash;
import org.zeromq.zyre.Zlist;
import org.zeromq.zyre.Zmsg;

public class Zyre {
    public native void create ();
    public native void destroy ();
    public native void print ();
    public native String uuid ();
    public native String name ();
    public native void setVerbose ();
    public native void setPort (int PortNbr);
    public native void setInterval (long Interval);
    public native void setInterface (String Value);
    public native int start ();
    public native void stop ();
    public native int join (String Group);
    public native int leave (String Group);
    public native Zmsg recv ();
    public native int whisper (String Peer, Zmsg MsgP);
    public native int shout (String Group, Zmsg MsgP);
    public native Zlist peers ();
    public native Zlist ownGroups ();
    public native Zlist peerGroups ();
    public native String peerAddress (String Peer);
    public native String peerHeaderValue (String Peer, String Name);
    public native void version (int Major, int Minor, int Patch);
    public native void test (boolean Verbose);
}
