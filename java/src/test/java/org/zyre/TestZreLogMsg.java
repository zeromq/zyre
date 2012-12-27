//  --------------------------------------------------------------------------
//  Selftest

package org.zyre;

import static org.junit.Assert.*;
import org.junit.Test;
import org.zeromq.ZMQ;
import org.zeromq.ZMQ.Socket;
import org.zeromq.ZFrame;
import org.zeromq.ZContext;

public class TestZreLogMsg
{
    @Test
    public void testZreLogMsg ()
    {
        System.out.printf (" * zre_log_msg: ");

        //  Simple create/destroy test
        ZreLogMsg self = new ZreLogMsg (0);
        assert (self != null);
        self.destroy ();

        //  Create pair of sockets we can send through
        ZContext ctx = new ZContext ();
        assert (ctx != null);

        Socket output = ctx.createSocket (ZMQ.DEALER);
        assert (output != null);
        output.bind ("inproc://selftest");
        Socket input = ctx.createSocket (ZMQ.ROUTER);
        assert (input != null);
        input.connect ("inproc://selftest");
        
        //  Encode/send/decode and verify each message type

        self = new ZreLogMsg (ZreLogMsg.LOG);
        self.setLevel ((byte) 123);
        self.setEvent ((byte) 123);
        self.setNode ((byte) 123);
        self.setPeer ((byte) 123);
        self.setTime ((byte) 123);
        self.setData ("Life is short but Now lasts for ever");
        self.send (output);
    
        self = ZreLogMsg.recv (input);
        assert (self != null);
        assertEquals (self.level (), 123);
        assertEquals (self.event (), 123);
        assertEquals (self.node (), 123);
        assertEquals (self.peer (), 123);
        assertEquals (self.time (), 123);
        assertEquals (self.data (), "Life is short but Now lasts for ever");
        self.destroy ();

        ctx.destroy ();
        System.out.printf ("OK\n");
    }
}
