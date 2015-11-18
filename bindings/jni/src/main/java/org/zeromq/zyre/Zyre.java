package org.zeromq.zyre;

import org.zeromq.czmq.ZMsg;

public class Zyre implements AutoCloseable {
    static {
        try {
            System.loadLibrary("zyrejni");
            System.loadLibrary("czmqjni");
        } catch (Exception e) {
            System.exit(-1);
        }
    }

    long pointer;

    public Zyre(String name) {
        pointer = Zyre.__init(name);
    }

    static native long __init(String name);

    static native void __destroy(long pointer);

    static native String __uuid(long pointer);

    static native String __name(long pointer);

    static native void __shout(long pointer, String group, long zmsgPtr);

    static native void __whisper(long pointer, String peer, long zmsgPtr);

    static native int __start(long pointer);

    static native void __stop(long pointer);

    static native int __join(long pointer, String group);

    static native int __leave(long pointer, String group);

    static native long __zyre_recv(long pointer);

    public String uuid() {
        return Zyre.__uuid(pointer);
    }

    public String name() {
        return Zyre.__name(pointer);
    }

    public void shout(String group, ZMsg msg) {
        Zyre.__shout(pointer, group, msg.getAddress());
    }

    public void whisper(String peer, ZMsg msg) {
        Zyre.__whisper(pointer, peer, msg.getAddress());
    }

    public boolean start() {
        return 0 == Zyre.__start(pointer);
    }

    public void stop() {
        Zyre.__stop(pointer);
    }

    public boolean join(String group) {
        return 0 == Zyre.__join(pointer, group);
    }

    public boolean leave(String group) {
        return 0 == Zyre.__leave(pointer, group);
    }

    public ZMsg recv() {
        final long zmsgPointer = __zyre_recv(pointer);
        return new ZMsg(zmsgPointer);
    }
    
    @Override
    public void close() {
        Zyre.__destroy(pointer);
    }
}
