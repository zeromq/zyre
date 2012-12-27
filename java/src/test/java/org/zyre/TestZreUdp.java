/*  =========================================================================
    TestZreUDP - UDP test management class

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

import static org.junit.Assert.*;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.nio.ByteBuffer;
import java.nio.channels.DatagramChannel;

import org.junit.Test;
import org.zeromq.ZContext;
import org.zeromq.ZMQ;

public class TestZreUdp
{
    private static final int MAX_REQUESTS = 5;
    private static final int PORT = 9991;
    
    private static class UdpAgent extends Thread
    {
        private ZContext ctx;

        public 
        UdpAgent (ZContext ctx)
        {
            this.ctx = ctx;
        }

        @Override
        public void 
        run ()
        {
            int request = 0;
            
            ZreUdp udp = new ZreUdp (PORT);
            DatagramChannel handle = udp.handle ();
            
            ZMQ.Poller items = ctx.getContext().poller();
            items.register(handle, ZMQ.Poller.POLLIN);
            
            
            while (!Thread.currentThread ().isInterrupted () && request < MAX_REQUESTS) {
                if (items.poll () == -1)
                    break;   //  Interrupted
                
                if (items.pollin (0)) {
                    ByteBuffer buffer = ByteBuffer.allocate (5);
                    int read = 0;
                    try {
                        read = udp.recv (buffer);
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                    assertEquals (read, buffer.capacity ());
                    assertEquals ("Hello", new String(buffer.array ()));
                    request ++;
                }
            }
            
            udp.destroy ();
        }
    }
    
    @Test
    public void 
    testBroadcast () throws Exception
    {
        ZContext ctx = new ZContext ();
        ctx.setContext (ZMQ.context (1));
        
        assert (ctx != null);

        UdpAgent udp1 = new UdpAgent (ctx);
        udp1.start ();
        Thread.sleep (1000);
        
        DatagramSocket sock = new DatagramSocket(null); 
        sock.setBroadcast (true);
        for (int i = 0; i < MAX_REQUESTS; i++) {
            DatagramPacket packet = 
                    new DatagramPacket ("Hello".getBytes (), 0, 5, InetAddress.getByName ("255.255.255.255"), PORT);
            sock.send (packet);
        }
        Thread.sleep (1000);
        ctx.destroy ();
        udp1.join ();
        sock.close ();

    }
}
