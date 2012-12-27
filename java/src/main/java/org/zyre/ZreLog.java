/*  =========================================================================
    ZreLog - record log data
        
    -------------------------------------------------------------------------
    Copyright (c) 1991-2012 iMatix Corporation <www.imatix.com>
    Copyright other contributors as noted in the AUTHORS file.
    
    This file is part of ZyRE, the ZeroMQ Realtime Experience framework:
    http://zyre.org.
        
    This is free software; you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 3 of the License, or (at
    your option) any later version.
        
    This software is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this program. If not, see
    <http://www.gnu.org/licenses/>.
    =========================================================================
*/ 
package org.zyre;

import org.zeromq.ZContext;
import org.zeromq.ZMQ;
import org.zeromq.ZMQ.Socket;

public class ZreLog
{

    private final ZContext ctx;         //  CZMQ context
    private final Socket publisher;     //  Socket to send to
    private int nodeid;                //  Own correlation ID
    
    //  ---------------------------------------------------------------------
    //  Construct new log object
    public ZreLog (String endpoint)
    {
        ctx = new ZContext ();
        publisher = ctx.createSocket (ZMQ.PUB);
        //  Modified Bernstein hashing function
        nodeid = endpoint.hashCode ();
    }

    //  ---------------------------------------------------------------------
    //  Destroy log object
    public void destory ()
    {
        ctx.destroy ();
    }


    //  ---------------------------------------------------------------------
    //  Connect log to remote endpoint
    public void connect (String endpoint)
    {
        publisher.connect (endpoint);
    }
    
    //  ---------------------------------------------------------------------
    //  Record one log event
    public void info (int event, String peer, String format, Object ... args)
    {
        int peerid = peer != null ? peer.hashCode () : 0;
        String body = format != null ? String.format (format, args) : "";
        
        sendLog (publisher, ZreLogMsg.ZRE_LOG_MSG_LEVEL_INFO,
                event, nodeid, peerid, System.currentTimeMillis (), body);

    }

    //  --------------------------------------------------------------------------
    //  Send the LOG to the socket in one step
    public static boolean sendLog (Socket output, 
                                    int level, 
                                    int event, 
                                    int node, 
                                    int peer, 
                                    long time, 
                                    String data)
    {
        ZreLogMsg msg = new ZreLogMsg (ZreLogMsg.LOG);
        msg.setLevel (level);
        msg.setEvent (event);
        msg.setNode (node);
        msg.setPeer (peer);
        msg.setTime (time);
        msg.setData (data);
        
        return msg.send (output);
    }
    
}
