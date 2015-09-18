package org.zyre.demo;

import org.zyre.Zyre;

public class Sender implements Runnable {
	
	@Override
	public void run() {
		System.out.println("SENDER starting");
		Zyre zyre = new Zyre();
		zyre.create();
		zyre.join("GLOBAL");
		
		try { Thread.sleep(500); } 
		catch (InterruptedException e) { e.printStackTrace(); }
			
		for (int i=0; i < 1000; i++) {
			zyre.shout("GLOBAL", "hello-" + i);
		}

		try { Thread.sleep(2000); } 
		catch (InterruptedException e) { e.printStackTrace(); }

		zyre.destroy();
	}
}
