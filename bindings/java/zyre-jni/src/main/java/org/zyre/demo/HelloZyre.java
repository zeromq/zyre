package org.zyre.demo;

import org.zyre.Zyre;

public class HelloZyre {

	public static void main(String[] args) throws Exception {
		
		Thread senderThread = new Thread(new Sender());
		
		Receiver receiver = new Receiver();
		Thread recvThread = new Thread(receiver);
		
		recvThread.start();

		Thread.sleep(1000);

		senderThread.start();
		senderThread.join();
		System.out.println("done sending, wait for receivers");
		Thread.sleep(5000);
		recvThread.interrupt();
		receiver.destroy();
	}

}
