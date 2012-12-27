/*  =========================================================================
    ZrePeer - one of our peers in a ZyRE network

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

import java.util.HashMap;
import java.util.Map;

import org.zeromq.ZContext;
import org.zeromq.ZMQ;
import org.zeromq.ZMQ.Socket;

public class ZrePeer
{
    private static final int USHORT_MAX = 0xffff;
    private static final int UBYTE_MAX = 0xff;
    
    private ZContext ctx;                //  CZMQ context
    private Socket mailbox;              //  Socket through to peer
    private String identity;             //  Identity string
    private String endpoint;             //  Endpoint connected to
    private long evasive_at;             //  Peer is being evasive
    private long expired_at;             //  Peer has expired by now
    private boolean connected;           //  Peer will send messages
    private boolean ready;               //  Peer has said Hello to us
    private int status;                  //  Our status counter
    private int sent_sequence;           //  Outgoing message sequence
    private int want_sequence;           //  Incoming message sequence
    private Map <String, String> headers;           //  Peer headers
    
    private ZrePeer (ZContext ctx, String identity)
    {
        this.ctx = ctx;
        this.identity = identity;
        
        ready = false;
        connected = false;
        sent_sequence = 0;
        want_sequence = 0;
    }
    
    //  ---------------------------------------------------------------------
    //  Construct new peer object
    public static ZrePeer newPeer (String identity, Map<String, ZrePeer> container, ZContext ctx)
    {
        ZrePeer peer = new ZrePeer (ctx, identity);
        container.put (identity, peer);
        
        return peer;
    }
    
    //  ---------------------------------------------------------------------
    //  Destroy peer object
    public void destory ()
    {
        disconnect ();
    }

    //  ---------------------------------------------------------------------
    //  Connect peer mailbox
    //  Configures mailbox and connects to peer's router endpoint
    public void connect (String replyTo, String endpoint)
    {
        //  Create new outgoing socket (drop any messages in transit)
        mailbox = ctx.createSocket (ZMQ.DEALER);

        //  Null if shutting down
        if (mailbox != null) {
            //  Set our caller 'From' identity so that receiving node knows
            //  who each message came from.
            mailbox.setIdentity (replyTo.getBytes ());
    
            //  Set a high-water mark that allows for reasonable activity
            mailbox.setSndHWM (ZreInterface.PEER_EXPIRED * 100);
           
            //  Send messages immediately or return EAGAIN
            mailbox.setSendTimeOut (0);
    
            //  Connect through to peer node
            mailbox.connect (String.format ("tcp://%s", endpoint));
            this.endpoint = endpoint;
            connected = true;
            ready = false;        
        }
    }

    //  ---------------------------------------------------------------------
    //  Disconnect peer mailbox
    //  No more messages will be sent to peer until connected again
    public void disconnect ()
    {
        ctx.destroySocket (mailbox);
        mailbox = null;
        endpoint = null;
        connected = false;
    }

    public boolean send (ZreMsg msg)
    {
        if (connected) {
            if (++sent_sequence > USHORT_MAX)
                sent_sequence = 0;
            msg.setSequence (sent_sequence);
            if (!msg.send (mailbox)) {
                disconnect ();
                return false;
            }
        }
        else
            msg.destroy ();

        return true;
    }

    //  ---------------------------------------------------------------------
    //  Return peer connection endpoint
    public String endpoint ()
    {
        if (connected)
            return endpoint;
        else
            return "";
                
    }

    //  ---------------------------------------------------------------------
    //  Register activity at peer
    public void refresh ()
    {
        evasive_at = System.currentTimeMillis () + ZreInterface.PEER_EVASIVE;
        expired_at = System.currentTimeMillis () + ZreInterface.PEER_EXPIRED;
    }

    //  ---------------------------------------------------------------------
    //  Return peer future expired time
    public long expiredAt ()
    {
        return expired_at;
    }

    //  ---------------------------------------------------------------------
    //  Return peer future evasive time
    public long evasiveAt ()
    {
        return evasive_at;
    }

    
    public void setReady (boolean ready)
    {
        this.ready = ready;
    }

    public boolean ready ()
    {
        return ready;
    }

    public void setStatus (int status)
    {
        this.status = status;
    }
    
    public int status ()
    {
        return status;
    }

    public void incStatus ()
    {
        if (++status > UBYTE_MAX)
            status = 0;
    }
    
    public String identity ()
    {
        return identity;
    }

    public String header (String key, String defaultValue)
    {
        if (headers.containsKey (key))
            return headers.get (key);
        
        return defaultValue;
    }
    
    public void setHeaders (Map<String, String> headers)
    {
        this.headers = new HashMap <String, String> (headers);
    }

    public boolean checkMessage (ZreMsg msg)
    {
        int recd_sequence = msg.sequence ();
        if (++want_sequence > USHORT_MAX)
            want_sequence = 0;
        
        boolean valid = want_sequence == recd_sequence;
        if (!valid) {
            if (--want_sequence < 0)    //  Rollback
                want_sequence = USHORT_MAX;
        }
        return valid;
    }

}
