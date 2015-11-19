package org.zeromq.zyre;

import org.zeromq.czmq.ZList;
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

    static native void __set_header(long pointer, String key, String value);

    static native void __set_verbose(long pointer);

    static native int __set_endpoint(long pointer, String endpoint);

    static native void __gossip_bind(long pointer, String endpoint);

    static native void __dump(long pointer);

    static native long __own_groups(long pointer);

    static native long __peers(long pointer);

    static native void __gossip_connect(long pointer, String endpoint);

    static native long __peer_groups(long pointer);

    static native String __peer_header_value(long pointer, String peer, String name);

    static native int __shouts(long pointer, String group, String value);

    static native int __whispers(long pointer, String peer, String value);

    static native String __peerAddress(long pointer, String peerId);

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

    public void setHeader(String key, String format, Object... args) {
        final String str = String.format(format, args);
        Zyre.__set_header(pointer, key, str);
    }

    public void setVerbose() {
        Zyre.__set_verbose(pointer);
    }

    public boolean setEndpoint(String format, Object... args) {
        final String str = String.format(format, args);
        return 0 == Zyre.__set_endpoint(pointer, str);
    }

    public void gossipBind(String format, Object... args) {
        final String str = String.format(format, args);
        Zyre.__gossip_bind(pointer, str);
    }

    public void dump() {
        Zyre.__dump(pointer);
    }

    public ZList ownGroups() {
        final long zlistPtr = Zyre.__own_groups(pointer);
        return new ZList(zlistPtr);
    }

    public ZList peers() {
        final long zlistPtr = Zyre.__peers(pointer);
        return new ZList(zlistPtr);
    }

    public void gossipConnect(String format, Object... args) {
        final String str = String.format(format, args);
        Zyre.__gossip_connect(pointer, str);
    }

    public ZList peerGroups() {
        final long zlistPtr = Zyre.__peer_groups(pointer);
        return new ZList(zlistPtr);
    }

    public String peerHeaderValue(String peer, String name) {
        String value = Zyre.__peer_header_value(pointer, peer, name);
        return value;
    }

    public boolean shouts(String group, String format, Object... args) {
        final String str = String.format(format, args);
        return 0 == Zyre.__shouts(pointer, group, str);
    }

    public boolean whispers(String peer, String format, Object... args) {
        final String str = String.format(format, args);
        return 0 == Zyre.__whispers(pointer, peer, str);
    }

    public String peerAddress(String peerId) {
        return Zyre.__peerAddress(pointer, peerId);
    }

    @Override
    public void close() {
        Zyre.__destroy(pointer);
    }
}
