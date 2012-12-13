package org.zyre;

import java.util.Random;

import org.zeromq.ZContext;
import org.zeromq.ZMQ.Poller;
import org.zeromq.ZMQ.Socket;
import org.zeromq.ZMsg;
import org.zeromq.ZThread;

public class ZreTester
{

    private static class InterfaceTask implements ZThread.IAttachedRunnable {

        private static final int MAX_GROUP = 10;

        @Override
        public void run (Object[] args, ZContext ctx, Socket pipe)
        {
            Random random = new Random ();

            ZreInterface inf = new ZreInterface ();
            long counter = 0;
            String to_peer = null;        //  Either of these set,
            String to_group = null;       //    and we set a message
            String cookie = null;

            Poller poller = ctx.getContext ().poller ();
            poller.register (pipe, Poller.POLLIN);
            poller.register (inf.handle (), Poller.POLLIN);
            
            //  Do something once a second
            long trigger = System.currentTimeMillis () + 1000;
            while (!Thread.currentThread ().isInterrupted ()) {
                long rc = poller.poll (random.nextInt (1000));
                if (rc == -1)
                    break;              //  Interrupted

                if (poller.pollin (0))
                    break;              //  Any command from parent means EXIT

                //  Process an event from interface
                if (poller.pollin (1)) {
                    ZMsg incoming = inf.recv ();
                    if (incoming == null)
                        break;              //  Interrupted

                    String event = incoming.popString ();
                    if (event.equals ("ENTER")) {
                        //  Always say hello to new peer
                        to_peer = incoming.popString ();
                    }
                    else
                    if (event.equals ("EXIT")) {
                        //  Always try talk to departed peer
                        to_peer = incoming.popString ();
                    }
                    else
                    if (event.equals ("WHISPER")) {
                        //  Send back response 1/2 the time
                        if (random.nextInt (2) == 0) {
                            to_peer = incoming.popString ();
                            cookie = incoming.popString ();
                        }
                    }
                    else
                    if (event.equals ("SHOUT")) {
                        to_peer = incoming.popString ();
                        to_group = incoming.popString ();
                        cookie = incoming.popString ();
                        //  Send peer response 1/3rd the time
                        if (random.nextInt (3) == 0) {
                            to_peer = null;
                        }
                        //  Send group response 1/3rd the time
                        if (random.nextInt (3) == 0) {
                            to_group = null;
                        }
                    }
                    else
                    if (event.equals ("JOIN")) {
                        String from_peer = incoming.popString ();
                        String group = incoming.popString ();
                        if (random.nextInt (3) == 0) {
                            inf.join (group);
                        }
                    }
                    else
                    if (event.equals ("LEAVE")) {
                        String from_peer = incoming.popString ();
                        String group = incoming.popString ();
                        if (random.nextInt (3) == 0) {
                            inf.leave (group);
                        }
                    }
                    else
                    if (event.equals ("DELIVER")) {
                        String filename = incoming.popString ();
                        String fullname = incoming.popString ();
                        System.out.printf ("I: received file %s\n", fullname);
                    }
                    incoming.destroy ();

                    //  Send outgoing messages if needed
                    if (to_peer != null) {
                        ZMsg outgoing = new ZMsg ();
                        outgoing.add (to_peer);
                        outgoing.add (String.format("%d", counter++));
                        inf.whisper (outgoing);
                        to_peer = null;
                    }
                    if (to_group != null) {
                        ZMsg outgoing = new ZMsg ();
                        outgoing.add (to_group);
                        outgoing.add (String.format("%d", counter++));
                        inf.shout (outgoing);
                        to_group = null;
                    }
                    if (System.currentTimeMillis () >= trigger) {
                        trigger = System.currentTimeMillis () + 1000;
                        String group = String.format ("GROUP%03d", random.nextInt (MAX_GROUP));
                        if (random.nextInt (4) == 0)
                            inf.join (group);
                        else
                        if (random.nextInt (3) == 0)
                            inf.leave (group);
                    }
                }
            }
            inf.destroy ();
        }
        
    }
    public static void 
    main (String[] args) throws Exception
    {
        Random random = new Random ();
        //  Initialize context for talking to tasks
        ZContext ctx = new ZContext ();
        ctx.setLinger (100);

        //  Get number of interfaces to simulate, default 100
        int max_interface = 100;
        int nbr_interfaces = 0;
        int max_tries = -1;
        int nbr_tries = 0;
        if (args.length > 0)
            max_interface = Integer.parseInt (args [0]);
        if (args.length > 1)
            max_tries = Integer.parseInt (args [1]);

        //  We address interfaces as an array of pipes
        Socket [] pipes = new Socket [max_interface];

        //  We will randomly start and stop interface threads
        while (!Thread.currentThread ().isInterrupted ()) {
            int index = random.nextInt (max_interface);
            //  Toggle interface thread
            if (pipes [index] != null) {
                pipes [index].send ("STOP");
                ctx.destroySocket (pipes [index]);
                pipes [index] = null;
                System.out.printf ("I: Stopped interface (%d running)\n", --nbr_interfaces);
            }
            else {
                pipes [index] = ZThread.fork (ctx, new InterfaceTask ());
                System.out.printf ("I: Started interface (%d running)\n", ++nbr_interfaces);
            }
            if (max_tries > 0 && ++nbr_tries >= max_tries)
                break;
            //  Sleep ~750 msecs randomly so we smooth out activity
            Thread.sleep (random.nextInt (500) + 500);
        }
        System.out.printf ("I: Stopped tester (%d tries)\n", nbr_tries);
        ctx.destroy ();
    }

}
