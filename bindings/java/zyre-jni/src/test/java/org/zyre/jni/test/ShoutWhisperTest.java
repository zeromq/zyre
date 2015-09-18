package org.zyre.jni.test;

import static org.junit.Assert.*;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.zyre.Utils;
import org.zyre.Zyre;

public class ShoutWhisperTest {
	
	private static final Logger log = LoggerFactory.getLogger(ShoutWhisperTest.class);

	public static final String PING = "ping";
	public static final String PONG = "pong";
	public static final String GROUP = "global";
	
	public static final int NUM_RESP = 3;
	public static final int NUM_MSGS = 2;
	
	private Requester reqThread;
	private List<Responder> respThreads = new ArrayList<Responder>();
	
	private boolean passed = true;

	@Test
	public void test() throws Exception {
		
		reqThread = new Requester();
		
		// start requester, which waits for the responder to JOIN
		reqThread.init();
		reqThread.start(); 

		// start responders
		for (int i=0; i < NUM_RESP; i++) {
			Responder resp = new Responder();
			resp.init();
			resp.start();
			respThreads.add(resp);
		}

		// wait for responders to finish
		for (Responder resp : respThreads) {
			resp.join();
		}

		// wait for requester to finish
		reqThread.join();
		reqThread.destroy();

		// destroy all responders once requester is done
		for (Responder resp : respThreads) {
			resp.destroy();
		}

		// leave some time for resources to be freed
		Thread.sleep(100);
		
		assertTrue(passed);
	}
	
	/**
	 *
	 */
	private class Requester extends ZyreThread {
		
		public void run() {
			String peer;
			int joinCt = 0;

			// wait for responders to join
			while (true) {
				String msg = zyre.recv();
				HashMap<String,String> map = Utils.parseMsg(msg);
				String event = map.get("event");
				peer = map.get("peer");

				if (event.equals("JOIN")) {
					if (peer == null) {
						log.error("Peer is null");
						passed = false;
						return;
					}
					joinCt++;
					log.info("responder joined: " + peer);

					if (joinCt == NUM_RESP) {
						log.info("All responders joined: " + NUM_RESP);
						break;
					}
				}
			}

			try { Thread.sleep(200); } 
			catch (InterruptedException e) { e.printStackTrace(); }

			log.info("sending ping(s) via SHOUT: " + NUM_MSGS);
			for (int i=0; i < NUM_MSGS; i++) {
				zyre.shout(GROUP, PING);
			}
			
			int expected = NUM_RESP * NUM_MSGS;
			int recvCt = 0;

			while(true) {
				String msg = zyre.recv();
				HashMap<String,String> map = Utils.parseMsg(msg);
				String event = map.get("event");				
				
				if (event.equals("WHISPER")) {
					log.info("requester received response: " + msg);

					String text = map.get("message");
					if (!text.equals(PONG)) {
						log.error("Did not receive PONG.  Message was: " + text);
						passed = false;
					}
					recvCt++;
					if (recvCt == expected) {
						log.info("received all messages: " + expected);
						break;
					}
					else {
						log.info("received: " + recvCt + " expected: " + expected);
					}
				}
				else {
					log.info("ignoring event: " + msg);
				}
			}
		}
	}
	
	private class Responder extends ZyreThread {
		
		public void run() {
			int sentCt = 0;

			while(!Thread.currentThread().isInterrupted()) {
				String msg = zyre.recv();
				HashMap<String,String> map = Utils.parseMsg(msg);
				
				String event = map.get("event");
				
				if (event.equals("SHOUT")){
					log.info("responder received shout: " + msg);
					String text = map.get("message");
					String peer = map.get("peer");
					
					if (!text.equals(PING)) {
						log.error("Did not receive PING. Message was: " + text);
						passed = false;
					}
				
					log.info("sending pong");
					zyre.whisper(peer, PONG);
					sentCt++;
					
					if (sentCt == NUM_MSGS) {
						break;
					}
				}
			}
		}
	}
	
	private class ZyreThread extends Thread {
		
		protected Zyre zyre;
		
		public void init() {
			zyre = new Zyre();
			zyre.create();
			zyre.join(GROUP);
		}
		
		public void destroy() {
			zyre.destroy();
		}
	}

}
