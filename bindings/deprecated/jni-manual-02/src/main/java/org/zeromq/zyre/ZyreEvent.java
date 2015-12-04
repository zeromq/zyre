package org.zeromq.zyre;

public class ZyreEvent implements AutoCloseable {
    static {
        try {
            System.loadLibrary("zyrejni");
        } catch (Exception e) {
            System.exit(-1);
        }
    }
    
    long pointer;
    
    public ZyreEvent() {
        pointer = ZyreEvent.__init();
    }

    native static long __init();
    native static void __destroy(long pointer);

    @Override
    public void close() {
        ZyreEvent.__destroy(pointer);
    }
}
