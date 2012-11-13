package org.zeromq.zyre;

import java.io.IOException;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.NetworkInterface;
import java.net.SocketAddress;
import java.nio.ByteBuffer;
import java.nio.channels.DatagramChannel;
import java.util.Collections;
import java.util.Enumeration;

public class ZreUdp
{
    private DatagramChannel handle;      //  Socket for send/recv
    private int port_nbr;               //  UDP port number we work on
    private InetAddress address;        //  Own address
    private InetAddress broadcast;      //  Broadcast address
    private SocketAddress sender;       //  Where last recv came from
    private String host;                //  Our own address as string
    private String from;                //  Sender address of last message

    //  -----------------------------------------------------------------
    //  Constructor
    public ZreUdp (int port_nbr) 
    {
        this.port_nbr = port_nbr;
        
        try {
            //  Create UDP socket
            handle = DatagramChannel.open ();
            handle.configureBlocking (false);
            DatagramSocket sock = handle.socket ();
            
            //  Ask operating system to let us do broadcasts from socket
            sock.setBroadcast (true);
            //  Allow multiple processes to bind to socket; incoming
            //  messages will come to each process
            sock.setReuseAddress (true);
            
            Enumeration <NetworkInterface> interfaces = NetworkInterface.getNetworkInterfaces ();
            for (NetworkInterface netint : Collections.list (interfaces)) {

                if (netint.isLoopback ())
                    continue;
                
                Enumeration <InetAddress> inetAddresses = netint.getInetAddresses ();
                for (InetAddress addr : Collections.list (inetAddresses)) {
                    address = addr;
                }
            }
            host = address.getHostAddress ();
            sock.bind (new InetSocketAddress (InetAddress.getByAddress (new byte[]{0,0,0,0}), port_nbr));
        } catch (IOException e) {
            throw new RuntimeException (e);
        }
    }
    
    //  -----------------------------------------------------------------
    //  Destructor
    public void 
    destroy () 
    {
        try {
            handle.close ();
        } catch (IOException e) {
        }
    }
    
    //  -----------------------------------------------------------------
    //  Returns UDP socket handle
    public DatagramChannel
    handle ()
    {
        return handle;
    }
    
    //  -----------------------------------------------------------------
    //  Return our own IP address as printable string
    public String 
    host ()
    {
        return host;
    }
    
    //  -----------------------------------------------------------------
    //  Return IP address of peer that sent last message
    public String
    from ()
    {
        return from;
    }
    
    //  -----------------------------------------------------------------
    //  Send message using UDP broadcast
    public void
    send (ByteBuffer buffer) 
    {
        try {
            broadcast = InetAddress.getByName ("255.255.255.255");
            handle.send (
                    buffer, new InetSocketAddress (broadcast, port_nbr));
        } catch (IOException e) {
            throw new RuntimeException (e);
        }
    }
    
    //  -----------------------------------------------------------------
    //  Receive message from UDP broadcast
    //  Returns size of received message, or -1
    public int
    recv (ByteBuffer buffer)
    {
        int read = buffer.remaining ();
        try {
            sender = handle.receive (buffer);
            if (sender == null)
                return -1;
            return read - buffer.remaining ();
        } catch (IOException e) {
            throw new RuntimeException (e);
        }
    }

}
