//  --------------------------------------------------------------------------
//  Selftest

package org.zyre;

import static org.junit.Assert.*;
import org.junit.Test;
import org.zeromq.ZMQ;
import org.zeromq.ZMQ.Socket;
import org.zeromq.ZFrame;
import org.zeromq.ZContext;

public class TestZreMsg
{
    @Test
    public void testZreMsg ()
    {
        System.out.printf (" * zre_msg: ");

        //  Simple create/destroy test
        ZreMsg self = new ZreMsg (0);
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

        self = new ZreMsg (ZreMsg.HELLO);
        self.setSequence ((byte) 123);
        self.setIpaddress ("Life is short but Now lasts for ever");
        self.setMailbox ((byte) 123);
        self.appendGroups ("Name: %s", "Brutus");
        self.appendGroups ("Age: %d", 43);
        self.setStatus ((byte) 123);
        self.insertHeaders ("Name", "Brutus");
        self.insertHeaders ("Age", "%d", 43);
        self.send (output);
    
        self = ZreMsg.recv (input);
        assert (self != null);
        assertEquals (self.sequence (), 123);
        assertEquals (self.ipaddress (), "Life is short but Now lasts for ever");
        assertEquals (self.mailbox (), 123);
        assertEquals (self.groups ().size (), 2);
        assertEquals (self.groups ().get (0), "Name: Brutus");
        assertEquals (self.groups ().get (1), "Age: 43");
        assertEquals (self.status (), 123);
        assertEquals (self.headers ().size (), 2);
        assertEquals (self.headersString ("Name", "?"), "Brutus");
        assertEquals (self.headersNumber ("Age", 0), 43);
        self.destroy ();

        self = new ZreMsg (ZreMsg.WHISPER);
        self.setSequence ((byte) 123);
        self.setContent (new ZFrame ("Captcha Diem"));
        self.send (output);
    
        self = ZreMsg.recv (input);
        assert (self != null);
        assertEquals (self.sequence (), 123);
        assertTrue (self.content ().streq ("Captcha Diem"));
        self.destroy ();

        self = new ZreMsg (ZreMsg.SHOUT);
        self.setSequence ((byte) 123);
        self.setGroup ("Life is short but Now lasts for ever");
        self.setContent (new ZFrame ("Captcha Diem"));
        self.send (output);
    
        self = ZreMsg.recv (input);
        assert (self != null);
        assertEquals (self.sequence (), 123);
        assertEquals (self.group (), "Life is short but Now lasts for ever");
        assertTrue (self.content ().streq ("Captcha Diem"));
        self.destroy ();

        self = new ZreMsg (ZreMsg.JOIN);
        self.setSequence ((byte) 123);
        self.setGroup ("Life is short but Now lasts for ever");
        self.setStatus ((byte) 123);
        self.send (output);
    
        self = ZreMsg.recv (input);
        assert (self != null);
        assertEquals (self.sequence (), 123);
        assertEquals (self.group (), "Life is short but Now lasts for ever");
        assertEquals (self.status (), 123);
        self.destroy ();

        self = new ZreMsg (ZreMsg.LEAVE);
        self.setSequence ((byte) 123);
        self.setGroup ("Life is short but Now lasts for ever");
        self.setStatus ((byte) 123);
        self.send (output);
    
        self = ZreMsg.recv (input);
        assert (self != null);
        assertEquals (self.sequence (), 123);
        assertEquals (self.group (), "Life is short but Now lasts for ever");
        assertEquals (self.status (), 123);
        self.destroy ();

        self = new ZreMsg (ZreMsg.PING);
        self.setSequence ((byte) 123);
        self.send (output);
    
        self = ZreMsg.recv (input);
        assert (self != null);
        assertEquals (self.sequence (), 123);
        self.destroy ();

        self = new ZreMsg (ZreMsg.PING_OK);
        self.setSequence ((byte) 123);
        self.send (output);
    
        self = ZreMsg.recv (input);
        assert (self != null);
        assertEquals (self.sequence (), 123);
        self.destroy ();

        ctx.destroy ();
        System.out.printf ("OK\n");
    }
}
