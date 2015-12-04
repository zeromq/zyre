package org.zyre;

/**
 * Demo class showing how zyre could be wrapped using JNI
 */
public class Zyre {
	
	// Load native libraries and initialize JNI stuff 
	static {
		String path = System.getProperty("java.library.path");
		System.out.println("java.library.path: " + path);
		System.loadLibrary("zyre"); 
		System.loadLibrary("zrejni"); 
		nativeInit();
	}
	
	/*
	 * Native methods, converted to C by javah command
	 */

	private static native void nativeInit();
	
	public native void create();
	public native void destroy();

	public native void join(String group);
	
	public native void shout(String group, String msg);
	public native void whisper(String nodeId, String msg);
	
	public native String recv();
	
	/*
	 * Used by JNI to store C pointers on heap
	 */
	
    private long nodeHandle;
	
    private long getNodeHandle() {
        return this.nodeHandle;
    }
}
