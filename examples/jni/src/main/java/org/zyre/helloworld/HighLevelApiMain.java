package org.zyre.helloworld;

import org.zeromq.czmq.Zmsg;
import org.zeromq.zyre.Zyre;
import org.zeromq.zyre.ZyreEvent;

public class HighLevelApiMain
{

    public static void main(String[] args) throws InterruptedException
    {
        Zyre peer1 = new Zyre("peer1");
        Zyre peer2 = new Zyre("peer2");

        peer1.start();
        peer2.start();
        Thread.sleep(250);  //  Give them time to connect to each other

        // ENTER message - A new peer discovered
        ZyreEvent peer2EnterMsg = new ZyreEvent(peer1);
        peer2EnterMsg.print();

        ZyreEvent peer1EnterMsg = new ZyreEvent(peer2);
        peer1EnterMsg.print();


        peer1.join("CHAT");
        peer2.join("CHAT");

        // JOIN messages - Peer joined group
        ZyreEvent peer2JoinMsg = new ZyreEvent(peer1);
        peer2JoinMsg.print();

        ZyreEvent peer1JoinMsg = new ZyreEvent(peer2);
        peer1JoinMsg.print();


        // SHOUT message - Message is send to all peers in group 'CHAT'
        Zmsg msgToSend = new Zmsg();
        msgToSend.addstr("Hello Mate!");
        peer1.shout("CHAT", msgToSend);

        ZyreEvent peer1ShoutMsg =  new ZyreEvent(peer2);
        peer1ShoutMsg.print();


        // WHISPER message - Message is only send the one peer
        Zmsg msgToSend2 = new Zmsg();
        msgToSend2.addstr("Hello back at you!");
        peer2.whisper(peer1ShoutMsg.peerUuid(), msgToSend2);

        ZyreEvent peer2WhisperMsg = new ZyreEvent(peer1);
        peer2WhisperMsg.print();

        peer1.stop();
        peer2.stop();

        peer1.close();
        peer2.close();
    }

}
