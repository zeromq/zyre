package org.zyre;

import org.zeromq.ZMsg;

public class Zyre {
	
	public native void create();
	public native void destroy();

	public native void join(String group);
	
	public native void shout(String group, String msg);
	public native void whisper(String nodeId, String msg);
	
	public native ZMsg recv();
	
	/*
	public native void shout(ZMsg zmsg);
	public native void whisper(ZMsg zmsg);
	*/

}
