package org.zyre;

import java.text.SimpleDateFormat;
import java.util.Calendar;

import org.zeromq.ZContext;
import org.zeromq.ZMQ;
import org.zeromq.ZMQ.Poller;
import org.zeromq.ZMQ.Socket;
import org.zeromq.ZMsg;

public class ZreLogger
{

    public static void main (String[] args)
    {
        ZContext ctx = new ZContext ();

        //  Use the Zyre UDP class to make sure we listen on the same
        //  network interface as our peers
        ZreUdp udp = new ZreUdp (ZreInterface.PING_PORT_NUMBER);
        String host = udp.host ();
        Socket collector = ctx.createSocket (ZMQ.SUB);

        //  Bind to an ephemeral port
        int port = collector.bindToRandomPort (String.format ("tcp://%s", host));

        //  Announce this to all peers we connect to
        ZreInterface inf = new ZreInterface ();
        inf.setHeader ("X-ZRELOG", "tcp://%s:%d", host, port);

        //  Get all log messages (don't filter)
        collector.subscribe ("".getBytes ());

        Poller items = ctx.getContext ().poller ();
        
        items.register (collector, Poller.POLLIN);
        items.register (inf.handle (), Poller.POLLIN);
        
        while (!Thread.currentThread ().isInterrupted ()) {
            if (items.poll (1000) == -1)
                break;              //  Interrupted
            //  Handle input on collector
            if (items.pollin (0))
                printLogMsg (collector);

            //  Handle event from interface (ignore it)
            if (items.pollin (1)) {
                ZMsg msg = inf.recv ();
                if (msg == null)
                    break;              //  Interrupted
                msg.destroy ();
            }
        }
        inf.destroy ();
        udp.destroy ();
        ctx.destroy ();

    }

    private static void printLogMsg (Socket collector)
    {
        ZreLogMsg msg = ZreLogMsg.recv (collector);
        if (msg == null)
            return;                 //  Interrupted

        long curtime = msg.time ();
        String event = null;
        switch (msg.event ()) {
            case ZreLogMsg.ZRE_LOG_MSG_EVENT_JOIN:
                event = "Join group";
                break;
            case ZreLogMsg.ZRE_LOG_MSG_EVENT_LEAVE:
                event = "Leave group";
                break;
            case ZreLogMsg.ZRE_LOG_MSG_EVENT_ENTER:
                event = "Peer enters";
                break;
            case ZreLogMsg.ZRE_LOG_MSG_EVENT_EXIT:
                event = "Peer exits";
                break;
        }
        
        Calendar now = Calendar.getInstance();
        now.setTimeInMillis (curtime);
        SimpleDateFormat df = new SimpleDateFormat("yyyy-MM-DD HH:mm:SS");
        String timestr = df.format (now.getTime ());

        System.out.printf ("%s I: [%04X] [%04X] - %s %s\n",
            timestr,
            msg.node (),
            msg.peer (),
            event,
            msg.data ());

        msg.destroy ();
        
    }

}
