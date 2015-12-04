package org.zyre.demo;

import org.zyre.Zyre;

public class HelloZyre {

	public static void main(String[] args) throws Exception {
		
		Thread senderThread = new Thread(new Sender());
		Thread recvThread = new Thread(new Receiver());
		
		recvThread.start();

		Thread.sleep(1000);

		senderThread.start();
	}
	
	public static void hello(String[] args) {
		
		Zyre zyre = new Zyre();
		zyre.create();
		zyre.join("GLOBAL");
		zyre.shout("GLOBAL", "hello");
		zyre.whisper("id1", "hey hey");
		//zyre.recv();
		zyre.destroy();
	}

}
