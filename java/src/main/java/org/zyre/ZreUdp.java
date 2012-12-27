/*  =========================================================================
    ZreUdp - UDP management class

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
    public void destroy () 
    {
        try {
            handle.close ();
        } catch (IOException e) {
        }
    }
    
    //  -----------------------------------------------------------------
    //  Returns UDP socket handle
    public DatagramChannel handle ()
    {
        return handle;
    }
    
    //  -----------------------------------------------------------------
    //  Return our own IP address as printable string
    public String host ()
    {
        return host;
    }
    
    //  -----------------------------------------------------------------
    //  Return IP address of peer that sent last message
    public String from ()
    {
        return from;
    }
    
    //  -----------------------------------------------------------------
    //  Send message using UDP broadcast
    public void send (ByteBuffer buffer) throws IOException
    {
        broadcast = InetAddress.getByName ("255.255.255.255");
        handle.send (
                buffer, new InetSocketAddress (broadcast, port_nbr));
    }
    
    //  -----------------------------------------------------------------
    //  Receive message from UDP broadcast
    //  Returns size of received message, or -1
    public int recv (ByteBuffer buffer) throws IOException
    {
        int read = buffer.remaining ();
        sender = handle.receive (buffer);
        if (sender == null)
            return -1;
        from = ((InetSocketAddress)sender).getAddress ().getHostAddress ();
        return read - buffer.remaining ();
    }

}
