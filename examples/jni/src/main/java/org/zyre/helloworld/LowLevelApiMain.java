package org.zyre.helloworld;

import org.zeromq.czmq.Zmsg;
import org.zeromq.zyre.Zyre;

public class LowLevelApiMain
{

    public static void main(String[] args) throws InterruptedException
    {
        Zyre peer1 = new Zyre("peer1");
        Zyre peer2 = new Zyre("peer2");

        peer1.start();
        peer2.start();
        Thread.sleep(250);  //  Give them time to connect to each other

        // ENTER message - A new peer discovered
        Zmsg peer2EnterMsg = peer1.recv();
        printZyreMessage(peer2EnterMsg);

        Zmsg peer1EnterMsg = peer2.recv();
        printZyreMessage(peer1EnterMsg);

        peer1.join("CHAT");
        peer2.join("CHAT");

        // JOIN messages - Peer joined group
        Zmsg peer2JoinMsg = peer1.recv();
        printZyreMessage(peer2JoinMsg);

        Zmsg peer1JoinMsg = peer2.recv();
        printZyreMessage(peer1JoinMsg);

        // SHOUT message - Message is send to all peers in group 'CHAT'
        Zmsg msgToSend = new Zmsg();
        msgToSend.addstr("Hello Mate!");
        peer1.shout("CHAT", msgToSend);

        Zmsg peer1ShoutMsg = peer2.recv();
        String peerUuid = printZyreMessage(peer1ShoutMsg);

        // WHISPER message - Message is only send the one peer
        Zmsg msgToSend2 = new Zmsg();
        msgToSend2.addstr("Hello back at you!");
        peer2.whisper(peerUuid, msgToSend2);

        Zmsg peer2WhisperMsg = peer1.recv();
        printZyreMessage(peer2WhisperMsg);

        peer1.stop();
        peer2.stop();

        peer1.close();
        peer2.close();
    }

    private static String printZyreMessage(Zmsg zyreMessage)
    {
        System.out.println("{");
        String cmd = zyreMessage.popstr();
        System.out.println("  Command: " + cmd);

        String peerUuid = zyreMessage.popstr();
        System.out.println("  From peer uuid: " + peerUuid);

        String peerName = zyreMessage.popstr();
        System.out.println("  From peer name: " + peerName);

        if (!("ENTER".equals(cmd) || "WHISPER".equals(cmd))) {
            String group = zyreMessage.popstr();
            System.out.println("  Group: " + group);
        }
        if ("SHOUT".equals(cmd) || "WHISPER".equals(cmd)) {
            String content = zyreMessage.popstr();
            System.out.println("  Content: " + content);
        }
        System.out.println("}");
        return peerUuid;
    }

}
