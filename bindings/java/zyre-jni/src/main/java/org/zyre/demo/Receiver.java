package org.zyre.demo;

import java.util.HashMap;

import org.zyre.Zyre;

import com.fasterxml.jackson.databind.ObjectMapper;

public class Receiver implements Runnable {
	
	Zyre zyre;
	
	@SuppressWarnings("unchecked")
	@Override
	public void run() {
		System.out.println("RECEIVER starting");
		zyre = new Zyre();
		zyre.create();
		zyre.join("GLOBAL");

		try { Thread.sleep(500); } 
		catch (InterruptedException e) { e.printStackTrace(); }
		
		while(!Thread.currentThread().isInterrupted()) {
			String msg = zyre.recv();
			try {
				// Convert the JSON string into a Map
				HashMap<String,String> result = new ObjectMapper().readValue(msg, HashMap.class);
				
				// print out shout messages received
				if (result.get("event").equals("SHOUT")) {
					System.out.println("RECEIVER received message: " + result.get("message"));
				}
			} 
			catch (Exception e) {
				System.err.println("could not parse: " + msg);
				e.printStackTrace();
			} 
		}
		System.out.println("RECEIVER done");
		try { Thread.sleep(10); } catch (InterruptedException e) {/*don't handle*/ }
		zyre.destroy();
	}
	
	public void destroy() {
		zyre.destroy();
	}

}
