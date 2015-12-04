package org.zyre.demo;

import java.util.HashMap;

import org.zyre.Zyre;

import com.fasterxml.jackson.databind.ObjectMapper;

public class Receiver implements Runnable {

	@SuppressWarnings("unchecked")
	@Override
	public void run() {
		System.out.println("RECEIVER starting");
		Zyre zyre = new Zyre();
		zyre.create();
		zyre.join("GLOBAL");

		try { Thread.sleep(500); } 
		catch (InterruptedException e) { e.printStackTrace(); }
		
		while(true) {
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
	}

}
