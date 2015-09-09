package org.zyre;

public class HelloZyre {
	// Load native libraries at runtime
	static {
		String path = System.getProperty("java.library.path");
		System.out.println("java.library.path: " + path);
		System.loadLibrary("zyre"); 
		System.loadLibrary("zrejni"); 
	}


	public static void main(String[] args) {
		
		Zyre zyre = new Zyre();
		zyre.create();
		zyre.join("GLOBAL");
		zyre.shout("GLOBAL", "hello");
		zyre.whisper("id1", "hey hey");
//		zyre.recv();
		zyre.destroy();
	}

}
