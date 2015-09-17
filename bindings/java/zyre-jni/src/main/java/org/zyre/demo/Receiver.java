package org.zyre.demo;

import java.util.Arrays;
import java.util.HashMap;
import java.util.List;

import org.zyre.Zyre;

import com.fasterxml.jackson.databind.ObjectMapper;

public class Receiver implements Runnable {
	
	Zyre zyre;
	
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
				//Convert the JSON string into a Map
				//@SuppressWarnings("unchecked")
				//HashMap<String,String> result = new ObjectMapper().readValue(msg, HashMap.class);

				HashMap<String,String> result = new HashMap<String,String>();
				List<String> pairs = Arrays.asList( msg.split(("\\|")) );
				if (pairs.size() != 4) {
					throw new Exception("recv() did not return exactly 4 key/value pairs");
				}

				for (String pair : pairs) {
					List<String> kv = Arrays.asList( pair.split(("::")) );
					if (kv.size() == 0) {
						// key and value are empty - do nothing
					}
					else if (kv.size() == 1) {
						// value is null
						result.put(kv.get(0), null);
					}
					else {
						result.put(kv.get(0), kv.get(1));
					}
				}
				
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
