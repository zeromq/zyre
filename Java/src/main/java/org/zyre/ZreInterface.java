/*  =========================================================================
    ZreInterface - interface to a ZyRE network

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

import java.io.File;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.UUID;

import org.filemq.FmqClient;
import org.filemq.FmqDir;
import org.filemq.FmqFile;
import org.filemq.FmqServer;
import org.zeromq.ZContext;
import org.zeromq.ZFrame;
import org.zeromq.ZMQ;
import org.zeromq.ZMQ.Poller;
import org.zeromq.ZMQ.Socket;
import org.zeromq.ZMsg;
import org.zeromq.ZThread;

public class ZreInterface
{
    public static final int UBYTE_MAX = 0xff;
    //  Defined port numbers, pending IANA submission
    public static final int PING_PORT_NUMBER = 9991;
    public static final int LOG_PORT_NUMBER = 9992;

    //  Constants, to be configured/reviewed
    public static final int PING_INTERVAL   = 1000;   //  Once per second
    public static final int PEER_EVASIVE    = 5000;   //  Five seconds' silence is evasive
    public static final int PEER_EXPIRED   = 10000;   //  Ten seconds' silence is expired
    
    private ZContext ctx;       //  Our context wrapper
    private Socket pipe;        //  Pipe through to agent

    //  ---------------------------------------------------------------------
    //  Constructor
    
    public ZreInterface () 
    {
        ctx = new ZContext ();
        pipe = ZThread.fork (ctx, new ZreInterfaceAgent ());
    }
    
    //  ---------------------------------------------------------------------
    //  Destructor
    public void destroy ()
    {
        ctx.destroy ();
    }
    
    //  ---------------------------------------------------------------------
    //  Receive next message from interface
    //  Returns ZMsg object, or NULL if interrupted
    public ZMsg recv ()
    {
        return ZMsg.recvMsg (pipe);
    }
    
    //  ---------------------------------------------------------------------
    //  Join a group
    public void join (String group) 
    {
        pipe.sendMore ("JOIN");
        pipe.send (group);
    }
    
    //  ---------------------------------------------------------------------
    //  Leave a group
    public void leave (String group) 
    {
        pipe.sendMore ("LEAVE");
        pipe.send (group);
    }
    
    //  ---------------------------------------------------------------------
    //  Send message to single peer; peer ID is first frame in message
    //  Destroys message after sending
    public void whisper (ZMsg msg) 
    {
        pipe.sendMore ("WHISPER");
        msg.send (pipe);
    }

    //  ---------------------------------------------------------------------
    //  Send message to a group of peers
    public void shout (ZMsg msg) 
    {
        pipe.sendMore ("SHOUT");
        msg.send (pipe);
    }
    
    //  ---------------------------------------------------------------------
    //  Return interface handle, for polling
    public Socket handle ()
    {
        return pipe;
    }
    
    //  ---------------------------------------------------------------------
    //  Set node header value
    public void setHeader (String name, String format, Object ... args)
    {
        pipe.sendMore ("SET");
        pipe.sendMore (name);
        pipe.send (String.format (format, args));
    }
    
    //  ---------------------------------------------------------------------
    //  Publish file into virtual space
    public void publish (String pathname, String virtual)
    {
        pipe.sendMore ("PUBLISH");
        pipe.sendMore (pathname);
        pipe.send (virtual);
    }
    
    //  =====================================================================
    //  Asynchronous part, works in the background
    
    //  Beacon frame has this format:
    //
    //  Z R E       3 bytes
    //  version     1 byte, %x01
    //  UUID        16 bytes
    //  port        2 bytes in network order

    protected static class Beacon 
    {
        public static final int BEACON_SIZE = 22;

        public static final String BEACON_PROTOCOL = "ZRE";
        public static final byte BEACON_VERSION = 0x01;
        
        private final byte [] protocol = BEACON_PROTOCOL.getBytes ();
        private final byte version = BEACON_VERSION;
        private UUID uuid;
        private int port;
        
        public Beacon (ByteBuffer buffer)
        {
            long msb = buffer.getLong ();
            long lsb = buffer.getLong ();
            uuid = new UUID (msb, lsb);
            port = buffer.getShort ();
            if (port < 0)
                port = (0xffff) & port;
        }
        
        public Beacon (UUID uuid, int port)
        {
            this.uuid = uuid;
            this.port = port;
        }
        
        public ByteBuffer getBuffer ()
        {
            ByteBuffer buffer = ByteBuffer.allocate (BEACON_SIZE);
            buffer.put (protocol);
            buffer.put (version);
            buffer.putLong (uuid.getMostSignificantBits ());
            buffer.putLong (uuid.getLeastSignificantBits ());
            buffer.putShort ((short) port);
            buffer.flip ();
            return buffer;
        }

    }
    
    private static String uuidStr (UUID uuid)
    {
        return uuid.toString ().replace ("-","").toUpperCase ();
    }
    
    private static final String OUTBOX = ".outbox";
    private static final String INBOX = ".inbox";
    
    protected static class Agent 
    {
        private final ZContext ctx;             //  CZMQ context
        private final Socket pipe;              //  Pipe back to application
        private final ZreUdp udp;               //  UDP object
        private final ZreLog log;               //  Log object
        private final UUID uuid;                //  Our UUID as binary blob
        private final String identity;          //  Our UUID as hex string
        private final Socket inbox;             //  Our inbox socket (ROUTER)
        private final String host;              //  Our host IP address
        private final int port;                 //  Our inbox port number
        private final String endpoint;          //  ipaddress:port endpoint
        private int status;                     //  Our own change counter
        private final Map <String, ZrePeer> peers;            //  Hash of known peers, fast lookup
        private final Map <String, ZreGroup> peer_groups;     //  Groups that our peers are in
        private final Map <String, ZreGroup> own_groups;      //  Groups that we are in
        private final Map <String, String> headers;           //  Our header values
        
        private final FmqServer fmq_server;           //  FileMQ server object
        private final int fmq_service;                //  FileMQ server port
        private final String fmq_outbox;              //  FileMQ server outbox

        private final FmqClient fmq_client;           //  FileMQ client object
        private final String fmq_inbox;               //  FileMQ client inbox
        
        private Agent (ZContext ctx, Socket pipe, Socket inbox, 
                                     ZreUdp udp, int port)
        {
            this.ctx = ctx;
            this.pipe = pipe;
            this.inbox = inbox;
            this.udp = udp;
            this.port = port;
            
            host = udp.host ();
            uuid = UUID.randomUUID ();
            identity = uuidStr (uuid);
            endpoint = String.format ("%s:%d", host, port);
            peers = new HashMap <String, ZrePeer> ();
            peer_groups = new HashMap <String, ZreGroup> ();
            own_groups = new HashMap <String, ZreGroup> ();
            headers = new HashMap <String, String> ();
            
            log = new ZreLog (endpoint);
            
            //  Set up content distribution network: Each server binds to an
            //  ephemeral port and publishes a temporary directory that acts
            //  as the outbox for this node.
            //
            fmq_outbox = String.format ("%s/%s", OUTBOX, identity);
            new File (fmq_outbox).mkdir ();
            
            fmq_inbox = String.format ("%s/%s", INBOX, identity);
            new File (fmq_inbox).mkdir ();
            
            fmq_server = new FmqServer ();
            fmq_service = fmq_server.bind ("tcp://*:*");
            fmq_server.publish (fmq_outbox, "/");
            fmq_server.setAnonymous (1);
            String publisher = String.format ("tcp://%s:%d", host, fmq_service);
            headers.put ("X-FILEMQ", publisher);
            
            //  Client will connect as it discovers new nodes
            fmq_client = new FmqClient ();
            fmq_client.setInbox (fmq_inbox);
            fmq_client.setResync (1);
            fmq_client.subscribe ("/");
        }
        
        protected static Agent newAgent (ZContext ctx, Socket pipe) 
        {
            Socket inbox = ctx.createSocket (ZMQ.ROUTER);
            if (inbox == null)      //  Interrupted
                return null;

            ZreUdp udp = new ZreUdp (PING_PORT_NUMBER);
            int port = inbox.bindToRandomPort ("tcp://*", 0xc000, 0xffff);
            if (port < 0) {          //  Interrupted
                System.err.println ("Failed to bind a random port");
                udp.destroy ();
                return null;
            }
            
            return new Agent (ctx, pipe, inbox, udp, port);
        }
        
        protected void destory () 
        {
            FmqDir inbox = FmqDir.newFmqDir (fmq_inbox, null);
            if (inbox != null) {
                inbox.remove (true);
                inbox.destroy ();
            }
            
            FmqDir outbox = FmqDir.newFmqDir (fmq_outbox, null);
            if (outbox != null) {
                outbox.remove (true);
                outbox.destroy ();
            }
            
            for (ZrePeer peer : peers.values ())
                peer.destory ();
            for (ZreGroup group : peer_groups.values ())
                group.destory ();
            for (ZreGroup group : own_groups.values ())
                group.destory ();
            
            fmq_server.destroy ();
            fmq_client.destroy ();
            udp.destroy ();
            log.destory ();
            
        }
        
        private int incStatus ()
        {
            if (++status > UBYTE_MAX)
                status = 0;
            return status;
        }
        
        //  Delete peer for a given endpoint
        private void purgePeer ()
        {
            for (Map.Entry <String, ZrePeer> entry : peers.entrySet ()) {
                ZrePeer peer = entry.getValue ();
                if (peer.endpoint ().equals (endpoint))
                    peer.disconnect ();
            }
        }
        
        //  Find or create peer via its UUID string
        private ZrePeer requirePeer (String identity, String address, int port)
        {
            ZrePeer peer = peers.get (identity);
            if (peer == null) {
                //  Purge any previous peer on same endpoint
                String endpoint = String.format ("%s:%d", address, port);
                
                purgePeer ();

                peer = ZrePeer.newPeer (identity, peers, ctx);
                peer.connect (this.identity, endpoint);

                //  Handshake discovery by sending HELLO as first message
                ZreMsg msg = new ZreMsg (ZreMsg.HELLO);
                msg.setIpaddress (this.udp.host ()); 
                msg.setMailbox (this.port);
                msg.setGroups (own_groups.keySet ());
                msg.setStatus (status);
                msg.setHeaders (new HashMap <String, String> (headers));
                peer.send (msg);

                log.info (ZreLogMsg.ZRE_LOG_MSG_EVENT_ENTER,
                              peer.endpoint (), endpoint);

                //  Now tell the caller about the peer
                pipe.sendMore ("ENTER");
                pipe.send (identity);
            }
            return peer;
        }
        
        //  Find or create group via its name
        private ZreGroup requirePeerGroup (String name)
        {
            ZreGroup group = peer_groups.get (name);
            if (group == null)
                group = ZreGroup.newGroup (name, peer_groups);
            return group;

        }
        
        private ZreGroup joinPeerGroup (ZrePeer peer, String name)
        {
            ZreGroup group = requirePeerGroup (name);
            group.join (peer);
            
            //  Now tell the caller about the peer joined a group
            pipe.sendMore ("JOIN");
            pipe.sendMore (identity);
            pipe.send (name);
            
            return group;
        }
        
        private ZreGroup leavePeerGroup (ZrePeer peer, String name)
        {
            ZreGroup group = requirePeerGroup (name);
            group.leave (peer);
            
            //  Now tell the caller about the peer joined a group
            pipe.sendMore ("LEAVE");
            pipe.sendMore (identity);
            pipe.send (name);
            
            return group;
        }

        //  Here we handle the different control messages from the front-end
        protected boolean recvFromApi ()
        {
            //  Get the whole message off the pipe in one go
            ZMsg request = ZMsg.recvMsg (pipe);
            String command = request.popString ();
            if (command == null)
                return false;                  //  Interrupted

            if (command.equals ("WHISPER")) {
                //  Get peer to send message to
                String identity = request.popString ();
                ZrePeer peer = peers.get (identity);

                //  Send frame on out to peer's mailbox, drop message
                //  if peer doesn't exist (may have been destroyed)
                if (peer != null) {
                    ZreMsg msg = new ZreMsg (ZreMsg.WHISPER);
                    msg.setContent (request.pop ());
                    peer.send (msg);
                }
            } else if (command.equals ("SHOUT")) {
                //  Get group to send message to
                String name = request.popString ();
                ZreGroup group = peer_groups.get (name);
                if (group != null) {
                    ZreMsg msg = new ZreMsg (ZreMsg.SHOUT);
                    msg.setGroup (name);
                    msg.setContent (request.pop ());
                    group.send (msg);
                }
            } else if (command.equals ("JOIN")) {
                String name = request.popString ();
                ZreGroup group = own_groups.get (name);
                if (group == null) {
                    //  Only send if we're not already in group
                    group = ZreGroup.newGroup (name, own_groups);
                    ZreMsg msg = new ZreMsg (ZreMsg.JOIN);
                    msg.setGroup (name);
                    //  Update status before sending command
                    msg.setStatus (incStatus ());
                    sendPeers (peers, msg);
                    msg.destroy ();
                    log.info (ZreLogMsg.ZRE_LOG_MSG_EVENT_JOIN, null, name);
                }
            } else if (command.equals ("LEAVE")) {
                String name = request.popString ();
                ZreGroup group = own_groups.get (name);
                if (group != null) {
                    //  Only send if we are actually in group
                    ZreMsg msg = new ZreMsg (ZreMsg.LEAVE);
                    msg.setGroup (name);
                    //  Update status before sending command
                    msg.setStatus (incStatus ());
                    sendPeers (peers, msg);
                    own_groups.remove (name);
                    log.info (ZreLogMsg.ZRE_LOG_MSG_EVENT_LEAVE, null, name);
                }
            } else if (command.equals ("SET")) {
                String name = request.popString ();
                String value = request.popString ();
                headers.put (name, value);
            } else if (command.equals ("PUBLISH")) {
                String filename = request.popString ();
                String virtual = request.popString ();
                //  Virtual filename must start with slash
                assert (virtual.startsWith ("/"));
                //  We create symbolic link pointing to real file
                String symlink = String.format ("%s.ln", virtual.substring (1));
                FmqFile file = new FmqFile (fmq_outbox, symlink);
                boolean rc = file.output ();
                assert (rc);
                file.write (filename, 0);
                file.destroy ();
            }
            
            request.destroy ();
            return true;
        }
        
        //  Here we handle messages coming from other peers
        protected boolean recvFromPeer ()
        {
            //  Router socket tells us the identity of this peer
            ZreMsg msg = ZreMsg.recv (inbox);
            if (msg == null)
                return false;               //  Interrupted

            String identity = new String (msg.address ().getData ());
            
            //  On HELLO we may create the peer if it's unknown
            //  On other commands the peer must already exist
            ZrePeer peer = peers.get (identity);
            if (msg.id () == ZreMsg.HELLO) {
                peer = requirePeer (
                    identity, msg.ipaddress (), msg.mailbox ());
                assert (peer != null);
                peer.setReady (true);
            }
            //  Ignore command if peer isn't ready
            if (peer == null || !peer.ready ()) {
                msg.destroy ();
                return true;
            }

            if (!peer.checkMessage (msg)) {
                System.err.printf ("W: [%s] lost messages from %s\n", this.identity, identity);
                assert (false);
            }

            //  Now process each command
            if (msg.id () == ZreMsg.HELLO) {
                //  Join peer to listed groups
                for (String name : msg.groups ()) {
                    joinPeerGroup (peer, name);
                }
                //  Hello command holds latest status of peer
                peer.setStatus (msg.status ());

                //  Store peer headers for future reference
                peer.setHeaders (msg.headers ());

                //  If peer is a log collector, connect to it
                String collector = msg.headersString ("X-ZRELOG", null);
                if (collector != null)
                    log.connect (collector);
                
                //  If peer is a log collector, connect to it
                String publisher = msg.headersString ("X-FILEMQ", null);
                if (publisher != null)
                    fmq_client.connect (publisher);
            }
            else
            if (msg.id () == ZreMsg.WHISPER) {
                //  Pass up to caller API as WHISPER event
                ZFrame cookie = msg.content ();
                pipe.sendMore ("WHISPER");
                pipe.sendMore (identity);
                cookie.sendAndKeep (pipe); // let msg free the frame
            }
            else
            if (msg.id () == ZreMsg.SHOUT) {
                //  Pass up to caller as SHOUT event
                ZFrame cookie = msg.content ();
                pipe.sendMore ("SHOUT");
                pipe.sendMore (identity);
                pipe.sendMore (msg.group ());
                cookie.sendAndKeep (pipe); // let msg free the frame
            }
            else
            if (msg.id () == ZreMsg.PING) {
                ZreMsg pingOK = new ZreMsg (ZreMsg.PING_OK);
                peer.send (pingOK);
            }
            else
            if (msg.id () == ZreMsg.JOIN) {
                joinPeerGroup (peer, msg.group ());
                assert (msg.status () == peer.status ());
            }
            else
            if (msg.id () == ZreMsg.LEAVE) {
                leavePeerGroup (peer, msg.group ());
                assert (msg.status () == peer.status ());
            }
            msg.destroy ();

            //  Activity from peer resets peer timers
            peer.refresh ();
            return true;
        }

        //  Handle beacon
        protected boolean recvUdpBeacon ()
        {
            ByteBuffer buffer = ByteBuffer.allocate (Beacon.BEACON_SIZE);
            
            //  Get beacon frame from network
            int size = 0;
            try {
                size = udp.recv (buffer);
            } catch (IOException e) {
                e.printStackTrace();
            }
            buffer.rewind ();
            
            //  Basic validation on the frame
            if (size != Beacon.BEACON_SIZE
                    || buffer.get () != 'Z'
                    || buffer.get () != 'R'
                    || buffer.get () != 'E'
                    || buffer.get () != Beacon.BEACON_VERSION)
                return true;       //  Ignore invalid beacons
            
            //  If we got a UUID and it's not our own beacon, we have a peer
            Beacon beacon = new Beacon (buffer);
            if (!beacon.uuid.equals (uuid)) {
                String identity = uuidStr (beacon.uuid);
                ZrePeer peer = requirePeer (identity, udp.from (), beacon.port);
                peer.refresh ();
            }
            
            return true;
        }

        //  Send moar beacon
        public void sendBeacon ()
        {
            Beacon beacon = new Beacon (uuid, port);
            try {
                udp.send (beacon.getBuffer ());
            } catch (IOException e) {
                e.printStackTrace ();
            }
        }
        
        //  We do this once a second:
        //  - if peer has gone quiet, send TCP ping
        //  - if peer has disappeared, expire it
        public void pingAllPeers ()
        {
            Iterator <Map.Entry <String, ZrePeer>> it = peers.entrySet ().iterator ();
            while (it.hasNext ()) {
                Map.Entry<String, ZrePeer> entry = it.next ();
                String identity = entry.getKey ();
                ZrePeer peer = entry.getValue ();
                if (System.currentTimeMillis () >= peer.expiredAt ()) {
                    log.info (ZreLogMsg.ZRE_LOG_MSG_EVENT_EXIT,
                            peer.endpoint (),
                            peer.endpoint ());
                    //  If peer has really vanished, expire it
                    pipe.sendMore ("EXIT");
                    pipe.send (identity);
                    deletePeerFromGroups (peer_groups, peer);
                    it.remove ();
                    peer.destory ();
                } 
                else 
                if (System.currentTimeMillis () >= peer.evasiveAt ()){
                    //  If peer is being evasive, force a TCP ping.
                    //  TODO: do this only once for a peer in this state;
                    //  it would be nicer to use a proper state machine
                    //  for peer management.
                    ZreMsg msg = new ZreMsg (ZreMsg.PING);
                    peer.send (msg);
                }
            }
        }
        
        public void recvFmqEvent ()
        {
            ZMsg msg = fmq_client.recv ();
            if (msg == null)
                return;
            msg.send (pipe);
        }
    }
    
    //  Send message to all peers
    private static void sendPeers (Map <String, ZrePeer> peers, ZreMsg msg)
    {
        for (ZrePeer peer : peers.values ())
            peer.send (msg);
    }
    
    //  Remove peer from group, if it's a member
    private static void deletePeerFromGroups (Map <String, ZreGroup> groups, ZrePeer peer)
    {
        for (ZreGroup group : groups.values ())
            group.leave (peer);
    }
    
    private static class ZreInterfaceAgent 
                            implements ZThread.IAttachedRunnable 
    {

        @Override
        public void run (Object[] args, ZContext ctx, Socket pipe)
        {
            Agent agent = Agent.newAgent (ctx, pipe);
            if (agent == null)   //  Interrupted
                return;
            
            long pingAt = System.currentTimeMillis ();
            Poller items = ctx.getContext ().poller ();
            
            items.register (agent.pipe, Poller.POLLIN);
            items.register (agent.inbox, Poller.POLLIN);
            items.register (agent.udp.handle (), Poller.POLLIN);
            items.register (agent.fmq_client.handle (), Poller.POLLIN);
            
            while (!Thread.currentThread ().isInterrupted ()) {
                long timeout = pingAt - System.currentTimeMillis ();
                assert (timeout <= PING_INTERVAL);
                
                if (timeout < 0)
                    timeout = 0;
                
                if (items.poll (timeout) < 0)
                    break;      // Interrupted
                
                if (items.pollin (0))
                    agent.recvFromApi ();
                
                if (items.pollin (1))
                    agent.recvFromPeer ();
                
                if (items.pollin (2))
                    agent.recvUdpBeacon ();
                
                if (items.pollin (3))
                    agent.recvFmqEvent ();
                
                if (System.currentTimeMillis () >= pingAt) {
                    agent.sendBeacon ();
                    pingAt = System.currentTimeMillis () + PING_INTERVAL;
                    //  Ping all peers and reap any expired ones
                    agent.pingAllPeers ();
                }
            }
            agent.destory ();
        }
    }
}
