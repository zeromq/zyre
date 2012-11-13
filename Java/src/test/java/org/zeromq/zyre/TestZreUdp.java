package org.zeromq.zyre;

import static org.junit.Assert.*;

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

        public UdpAgent (ZContext ctx)
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
                    int read = udp.recv (buffer);
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
