/*  =========================================================================
    ZreMsg.java
    
    Generated codec class for ZreMsg
    -------------------------------------------------------------------------
    Copyright (c) 1991-2012 iMatix Corporation -- http://www.imatix.com     
    Copyright other contributors as noted in the AUTHORS file.              
                                                                            
    This file is part of Zyre, an open-source framework for proximity-based 
    peer-to-peer applications -- See http://zyre.org.                       
                                                                            
    This is free software; you can redistribute it and/or modify it under   
    the terms of the GNU Lesser General Public License as published by the  
    Free Software Foundation; either version 3 of the License, or (at your  
    option) any later version.                                              
                                                                            
    This software is distributed in the hope that it will be useful, but    
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTA-   
    BILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General  
    Public License for more details.                                        
                                                                            
    You should have received a copy of the GNU Lesser General Public License
    along with this program. If not, see http://www.gnu.org/licenses/.      
    =========================================================================
*/

/*  These are the zre_msg messages
    HELLO - Greet a peer so it can connect back to us
        sequence      number 2
        ipaddress     string
        mailbox       number 2
        groups        strings
        status        number 1
        headers       dictionary
    WHISPER - Send a message to a peer
        sequence      number 2
        content       frame
    SHOUT - Send a message to a group
        sequence      number 2
        group         string
        content       frame
    JOIN - Join a group
        sequence      number 2
        group         string
        status        number 1
    LEAVE - Leave a group
        sequence      number 2
        group         string
        status        number 1
    PING - Ping a peer that has gone silent
        sequence      number 2
    PING_OK - Reply to a peer's ping
        sequence      number 2
*/

package org.zyre;

import java.util.Collection;
import java.util.List;
import java.util.Map;
import java.util.ArrayList;
import java.util.HashMap;
import java.nio.ByteBuffer;

import org.zeromq.ZFrame;
import org.zeromq.ZMQ;
import org.zeromq.ZMQ.Socket;

//  Opaque class structure
public class ZreMsg 
{
    public static final int ZRE_MSG_VERSION                 = 1;

    public static final int HELLO                 = 1;
    public static final int WHISPER               = 2;
    public static final int SHOUT                 = 3;
    public static final int JOIN                  = 4;
    public static final int LEAVE                 = 5;
    public static final int PING                  = 6;
    public static final int PING_OK               = 7;

    //  Structure of our class
    private ZFrame address;             //  Address of peer if any
    private int id;                     //  ZreMsg message ID
    private ByteBuffer needle;          //  Read/write pointer for serialization
    private int sequence;
    private String ipaddress;
    private int mailbox;
    private List <String> groups;
    private int status;
    private Map <String, String> headers;
    private int headersBytes;
    private ZFrame content;
    private String group;


    //  --------------------------------------------------------------------------
    //  Create a new ZreMsg

    public ZreMsg (int id)
    {
        this.id = id;
    }


    //  --------------------------------------------------------------------------
    //  Destroy the zre_msg

    public void destroy ()
    {
        //  Free class properties
        if (address != null)
            address.destroy ();
        address = null;
    }


    //  --------------------------------------------------------------------------
    //  Network data encoding macros


    //  Put a 1-byte number to the frame
    private final void putNumber1 (int value) 
    {
        needle.put ((byte) value);
    }

    //  Get a 1-byte number to the frame
    //  then make it unsigned
    private int getNumber1 () 
    { 
        int value = needle.get (); 
        if (value < 0)
            value = (0xff) & value;
        return value;
    }

    //  Put a 2-byte number to the frame
    private final void putNumber2 (int value) 
    {
        needle.putShort ((short) value);
    }

    //  Get a 2-byte number to the frame
    private int getNumber2 () 
    { 
        int value = needle.getShort (); 
        if (value < 0)
            value = (0xffff) & value;
        return value;
    }

    //  Put a 4-byte number to the frame
    private final void putNumber4 (long value) 
    {
        needle.putInt ((int) value);
    }

    //  Get a 4-byte number to the frame
    //  then make it unsigned
    private long getNumber4 () 
    { 
        long value = needle.getInt (); 
        if (value < 0)
            value = (0xffffffff) & value;
        return value;
    }

    //  Put a 8-byte number to the frame
    public void putNumber8 (long value) 
    {
        needle.putLong (value);
    }

    //  Get a 8-byte number to the frame
    public long getNumber8 () 
    {
        return needle.getLong ();
    }


    //  Put a block to the frame
    private void putBlock (byte [] value, int size) 
    {
        needle.put (value, 0, size);
    }

    private byte [] getBlock (int size) 
    {
        byte [] value = new byte [size]; 
        needle.get (value);

        return value;
    }

    //  Put a string to the frame
    public void putString (String value) 
    {
        needle.put ((byte) value.length ());
        needle.put (value.getBytes());
    }

    //  Get a string from the frame
    public String getString () 
    {
        int size = getNumber1 ();
        byte [] value = new byte [size];
        needle.get (value);

        return new String (value);
    }

    //  --------------------------------------------------------------------------
    //  Receive and parse a ZreMsg from the socket. Returns new object or
    //  null if error. Will block if there's no message waiting.

    public static ZreMsg recv (Socket input)
    {
        assert (input != null);
        ZreMsg self = new ZreMsg (0);
        ZFrame frame = null;

        try {
            //  Read valid message frame from socket; we loop over any
            //  garbage data we might receive from badly-connected peers
            while (true) {
                //  If we're reading from a ROUTER socket, get address
                if (input.getType () == ZMQ.ROUTER) {
                    self.address = ZFrame.recvFrame (input);
                    if (self.address == null)
                        return null;         //  Interrupted
                    if (!input.hasReceiveMore ())
                        throw new IllegalArgumentException ();
                }
                //  Read and parse command in frame
                frame = ZFrame.recvFrame (input);
                if (frame == null)
                    return null;             //  Interrupted

                //  Get and check protocol signature
                self.needle = ByteBuffer.wrap (frame.getData ()); 
                int signature = self.getNumber2 ();
                if (signature == (0xAAA0 | 1))
                    break;                  //  Valid signature

                //  Protocol assertion, drop message
                while (input.hasReceiveMore ()) {
                    frame.destroy ();
                    frame = ZFrame.recvFrame (input);
                }
                frame.destroy ();
            }

            //  Get message id, which is first byte in frame
            self.id = self.getNumber1 ();
            int listSize;
            int hashSize;

            switch (self.id) {
            case HELLO:
                self.sequence = self.getNumber2 ();
                self.ipaddress = self.getString ();
                self.mailbox = self.getNumber2 ();
                listSize = self.getNumber1 ();
                self.groups = new ArrayList<String> ();
                while (listSize-- > 0) {
                    String string = self.getString ();
                    self.groups.add (string);
                }
                self.status = self.getNumber1 ();
                hashSize = self.getNumber1 ();
                self.headers = new HashMap <String, String> ();
                while (hashSize-- > 0) {
                    String string = self.getString ();
                    String [] kv = string.split("=");
                    self.headers.put(kv[0], kv[1]);
                }

                break;

            case WHISPER:
                self.sequence = self.getNumber2 ();
                //  Get next frame, leave current untouched
                if (!input.hasReceiveMore ())
                    throw new IllegalArgumentException ();
                self.content = ZFrame.recvFrame (input);
                break;

            case SHOUT:
                self.sequence = self.getNumber2 ();
                self.group = self.getString ();
                //  Get next frame, leave current untouched
                if (!input.hasReceiveMore ())
                    throw new IllegalArgumentException ();
                self.content = ZFrame.recvFrame (input);
                break;

            case JOIN:
                self.sequence = self.getNumber2 ();
                self.group = self.getString ();
                self.status = self.getNumber1 ();
                break;

            case LEAVE:
                self.sequence = self.getNumber2 ();
                self.group = self.getString ();
                self.status = self.getNumber1 ();
                break;

            case PING:
                self.sequence = self.getNumber2 ();
                break;

            case PING_OK:
                self.sequence = self.getNumber2 ();
                break;

            default:
                throw new IllegalArgumentException ();
            }

            return self;

        } catch (Exception e) {
            //  Error returns
            System.out.printf ("E: malformed message '%d'\n", self.id);
            self.destroy ();
            return null;
        } finally {
            if (frame != null)
                frame.destroy ();
        }
    }


    //  Count size of key=value pair
    private static void 
    headersCount (final Map.Entry <String, String> entry, ZreMsg self)
    {
        self.headersBytes += entry.getKey ().length () + 1 + entry.getValue ().length () + 1;
    }

    //  Serialize headers key=value pair
    private static void
    headersWrite (final Map.Entry <String, String> entry, ZreMsg self)
    {
        String string = entry.getKey () + "=" + entry.getValue ();
        self.putString (string);
    }


    //  --------------------------------------------------------------------------
    //  Send the ZreMsg to the socket, and destroy it

    public boolean send (Socket socket)
    {
        assert (socket != null);

        //  Calculate size of serialized data
        int frameSize = 2 + 1;          //  Signature and message ID
        switch (id) {
        case HELLO:
            //  sequence is a 2-byte integer
            frameSize += 2;
            //  ipaddress is a string with 1-byte length
            frameSize++;       //  Size is one octet
            if (ipaddress != null)
                frameSize += ipaddress.length ();
            //  mailbox is a 2-byte integer
            frameSize += 2;
            //  groups is an array of strings
            frameSize++;       //  Size is one octet
            if (groups != null) {
                for (String value : groups) 
                    frameSize += 1 + value.length ();
            }
            //  status is a 1-byte integer
            frameSize += 1;
            //  headers is an array of key=value strings
            frameSize++;       //  Size is one octet
            if (headers != null) {
                headersBytes = 0;
                for (Map.Entry <String, String> entry: headers.entrySet ()) {
                    headersCount (entry, this);
                }
                frameSize += headersBytes;
            }
            break;
            
        case WHISPER:
            //  sequence is a 2-byte integer
            frameSize += 2;
            break;
            
        case SHOUT:
            //  sequence is a 2-byte integer
            frameSize += 2;
            //  group is a string with 1-byte length
            frameSize++;       //  Size is one octet
            if (group != null)
                frameSize += group.length ();
            break;
            
        case JOIN:
            //  sequence is a 2-byte integer
            frameSize += 2;
            //  group is a string with 1-byte length
            frameSize++;       //  Size is one octet
            if (group != null)
                frameSize += group.length ();
            //  status is a 1-byte integer
            frameSize += 1;
            break;
            
        case LEAVE:
            //  sequence is a 2-byte integer
            frameSize += 2;
            //  group is a string with 1-byte length
            frameSize++;       //  Size is one octet
            if (group != null)
                frameSize += group.length ();
            //  status is a 1-byte integer
            frameSize += 1;
            break;
            
        case PING:
            //  sequence is a 2-byte integer
            frameSize += 2;
            break;
            
        case PING_OK:
            //  sequence is a 2-byte integer
            frameSize += 2;
            break;
            
        default:
            System.out.printf ("E: bad message type '%d', not sent\n", id);
            assert (false);
        }
        //  Now serialize message into the frame
        ZFrame frame = new ZFrame (new byte [frameSize]);
        needle = ByteBuffer.wrap (frame.getData ()); 
        int frameFlags = 0;
        putNumber2 (0xAAA0 | 1);
        putNumber1 ((byte) id);

        switch (id) {
        case HELLO:
            putNumber2 (sequence);
            if (ipaddress != null)
                putString (ipaddress);
            else
                putNumber1 ((byte) 0);      //  Empty string
            putNumber2 (mailbox);
            if (groups != null) {
                putNumber1 ((byte) groups.size ());
                for (String value : groups) {
                    putString (value);
                }
            }
            else
                putNumber1 ((byte) 0);      //  Empty string array
            putNumber1 (status);
            if (headers != null) {
                putNumber1 ((byte) headers.size ());
                for (Map.Entry <String, String> entry: headers.entrySet ()) {
                    headersWrite (entry, this);
                }
            }
            else
                putNumber1 ((byte) 0);      //  Empty dictionary
            break;
            
        case WHISPER:
            putNumber2 (sequence);
            frameFlags = ZMQ.SNDMORE;
            break;
            
        case SHOUT:
            putNumber2 (sequence);
            if (group != null)
                putString (group);
            else
                putNumber1 ((byte) 0);      //  Empty string
            frameFlags = ZMQ.SNDMORE;
            break;
            
        case JOIN:
            putNumber2 (sequence);
            if (group != null)
                putString (group);
            else
                putNumber1 ((byte) 0);      //  Empty string
            putNumber1 (status);
            break;
            
        case LEAVE:
            putNumber2 (sequence);
            if (group != null)
                putString (group);
            else
                putNumber1 ((byte) 0);      //  Empty string
            putNumber1 (status);
            break;
            
        case PING:
            putNumber2 (sequence);
            break;
            
        case PING_OK:
            putNumber2 (sequence);
            break;
            
        }
        //  If we're sending to a ROUTER, we send the address first
        if (socket.getType () == ZMQ.ROUTER) {
            assert (address != null);
            if (!address.sendAndDestroy (socket, ZMQ.SNDMORE)) {
                destroy ();
                return false;
            }
        }
        //  Now send the data frame
        if (!frame.sendAndDestroy (socket, frameFlags)) {
            frame.destroy ();
            destroy ();
            return false;
        }
        
        //  Now send any frame fields, in order
        switch (id) {
        case WHISPER:
            //  If content isn't set, send an empty frame
            if (content == null)
                content = new ZFrame ("".getBytes ());
            if (!content.sendAndDestroy (socket, 0)) {
                frame.destroy ();
                destroy ();
                return false;
            }
            break;
        case SHOUT:
            //  If content isn't set, send an empty frame
            if (content == null)
                content = new ZFrame ("".getBytes ());
            if (!content.sendAndDestroy (socket, 0)) {
                frame.destroy ();
                destroy ();
                return false;
            }
            break;
        }
        //  Destroy ZreMsg object
        destroy ();
        return true;
    }


//  --------------------------------------------------------------------------
//  Send the HELLO to the socket in one step

    public static void sendHello (
        Socket output,
        int sequence,
        String ipaddress,
        int mailbox,
        Collection <String> groups,
        int status,
        Map <String, String> headers) 
    {
        ZreMsg self = new ZreMsg (ZreMsg.HELLO);
        self.setSequence (sequence);
        self.setIpaddress (ipaddress);
        self.setMailbox (mailbox);
        self.setGroups (new ArrayList <String> (groups));
        self.setStatus (status);
        self.setHeaders (new HashMap <String, String> (headers));
        self.send (output); 
    }

//  --------------------------------------------------------------------------
//  Send the WHISPER to the socket in one step

    public static void sendWhisper (
        Socket output,
        int sequence,
        ZFrame content) 
    {
        ZreMsg self = new ZreMsg (ZreMsg.WHISPER);
        self.setSequence (sequence);
        self.setContent (content.duplicate ());
        self.send (output); 
    }

//  --------------------------------------------------------------------------
//  Send the SHOUT to the socket in one step

    public static void sendShout (
        Socket output,
        int sequence,
        String group,
        ZFrame content) 
    {
        ZreMsg self = new ZreMsg (ZreMsg.SHOUT);
        self.setSequence (sequence);
        self.setGroup (group);
        self.setContent (content.duplicate ());
        self.send (output); 
    }

//  --------------------------------------------------------------------------
//  Send the JOIN to the socket in one step

    public static void sendJoin (
        Socket output,
        int sequence,
        String group,
        int status) 
    {
        ZreMsg self = new ZreMsg (ZreMsg.JOIN);
        self.setSequence (sequence);
        self.setGroup (group);
        self.setStatus (status);
        self.send (output); 
    }

//  --------------------------------------------------------------------------
//  Send the LEAVE to the socket in one step

    public static void sendLeave (
        Socket output,
        int sequence,
        String group,
        int status) 
    {
        ZreMsg self = new ZreMsg (ZreMsg.LEAVE);
        self.setSequence (sequence);
        self.setGroup (group);
        self.setStatus (status);
        self.send (output); 
    }

//  --------------------------------------------------------------------------
//  Send the PING to the socket in one step

    public static void sendPing (
        Socket output,
        int sequence) 
    {
        ZreMsg self = new ZreMsg (ZreMsg.PING);
        self.setSequence (sequence);
        self.send (output); 
    }

//  --------------------------------------------------------------------------
//  Send the PING_OK to the socket in one step

    public static void sendPing_Ok (
        Socket output,
        int sequence) 
    {
        ZreMsg self = new ZreMsg (ZreMsg.PING_OK);
        self.setSequence (sequence);
        self.send (output); 
    }


    //  --------------------------------------------------------------------------
    //  Duplicate the ZreMsg message

    public ZreMsg dup (ZreMsg self)
    {
        if (self == null)
            return null;

        ZreMsg copy = new ZreMsg (self.id);
        if (self.address != null)
            copy.address = self.address.duplicate ();
        switch (self.id) {
        case HELLO:
            copy.sequence = self.sequence;
            copy.ipaddress = self.ipaddress;
            copy.mailbox = self.mailbox;
            copy.groups = new ArrayList <String> (self.groups);
            copy.status = self.status;
            copy.headers = new HashMap <String, String> (self.headers);
        break;
        case WHISPER:
            copy.sequence = self.sequence;
            copy.content = self.content.duplicate ();
        break;
        case SHOUT:
            copy.sequence = self.sequence;
            copy.group = self.group;
            copy.content = self.content.duplicate ();
        break;
        case JOIN:
            copy.sequence = self.sequence;
            copy.group = self.group;
            copy.status = self.status;
        break;
        case LEAVE:
            copy.sequence = self.sequence;
            copy.group = self.group;
            copy.status = self.status;
        break;
        case PING:
            copy.sequence = self.sequence;
        break;
        case PING_OK:
            copy.sequence = self.sequence;
        break;
        }
        return copy;
    }

    //  Dump headers key=value pair to stdout
    public static void headersDump (Map.Entry <String, String> entry, ZreMsg self)
    {
        System.out.printf ("        %s=%s\n", entry.getKey (), entry.getValue ());
    }


    //  --------------------------------------------------------------------------
    //  Print contents of message to stdout

    public void dump ()
    {
        switch (id) {
        case HELLO:
            System.out.println ("HELLO:");
            System.out.printf ("    sequence=%d\n", (long)sequence);
            if (ipaddress != null)
                System.out.printf ("    ipaddress='%s'\n", ipaddress);
            else
                System.out.printf ("    ipaddress=\n");
            System.out.printf ("    mailbox=%d\n", (long)mailbox);
            System.out.printf ("    groups={");
            if (groups != null) {
                for (String value : groups) {
                    System.out.printf (" '%s'", value);
                }
            }
            System.out.printf (" }\n");
            System.out.printf ("    status=%d\n", (long)status);
            System.out.printf ("    headers={\n");
            if (headers != null) {
                for (Map.Entry <String, String> entry : headers.entrySet ())
                    headersDump (entry, this);
            }
            System.out.printf ("    }\n");
            break;
            
        case WHISPER:
            System.out.println ("WHISPER:");
            System.out.printf ("    sequence=%d\n", (long)sequence);
            System.out.printf ("    content={\n");
            if (content != null) {
                int size = content.size ();
                byte [] data = content.getData ();
                System.out.printf ("        size=%d\n", content.size ());
                if (size > 32)
                    size = 32;
                int contentIndex;
                for (contentIndex = 0; contentIndex < size; contentIndex++) {
                    if (contentIndex != 0 && (contentIndex % 4 == 0))
                        System.out.printf ("-");
                    System.out.printf ("%02X", data [contentIndex]);
                }
            }
            System.out.printf ("    }\n");
            break;
            
        case SHOUT:
            System.out.println ("SHOUT:");
            System.out.printf ("    sequence=%d\n", (long)sequence);
            if (group != null)
                System.out.printf ("    group='%s'\n", group);
            else
                System.out.printf ("    group=\n");
            System.out.printf ("    content={\n");
            if (content != null) {
                int size = content.size ();
                byte [] data = content.getData ();
                System.out.printf ("        size=%d\n", content.size ());
                if (size > 32)
                    size = 32;
                int contentIndex;
                for (contentIndex = 0; contentIndex < size; contentIndex++) {
                    if (contentIndex != 0 && (contentIndex % 4 == 0))
                        System.out.printf ("-");
                    System.out.printf ("%02X", data [contentIndex]);
                }
            }
            System.out.printf ("    }\n");
            break;
            
        case JOIN:
            System.out.println ("JOIN:");
            System.out.printf ("    sequence=%d\n", (long)sequence);
            if (group != null)
                System.out.printf ("    group='%s'\n", group);
            else
                System.out.printf ("    group=\n");
            System.out.printf ("    status=%d\n", (long)status);
            break;
            
        case LEAVE:
            System.out.println ("LEAVE:");
            System.out.printf ("    sequence=%d\n", (long)sequence);
            if (group != null)
                System.out.printf ("    group='%s'\n", group);
            else
                System.out.printf ("    group=\n");
            System.out.printf ("    status=%d\n", (long)status);
            break;
            
        case PING:
            System.out.println ("PING:");
            System.out.printf ("    sequence=%d\n", (long)sequence);
            break;
            
        case PING_OK:
            System.out.println ("PING_OK:");
            System.out.printf ("    sequence=%d\n", (long)sequence);
            break;
            
        }
    }


    //  --------------------------------------------------------------------------
    //  Get/set the message address

    public ZFrame address ()
    {
        return address;
    }

    public void setAddress (ZFrame address)
    {
        if (this.address != null)
            this.address.destroy ();
        this.address = address.duplicate ();
    }


    //  --------------------------------------------------------------------------
    //  Get/set the zre_msg id

    public int id ()
    {
        return id;
    }

    public void setId (int id)
    {
        this.id = id;
    }

    //  --------------------------------------------------------------------------
    //  Get/set the sequence field

    public int sequence ()
    {
        return sequence;
    }

    public void setSequence (int sequence)
    {
        this.sequence = sequence;
    }


    //  --------------------------------------------------------------------------
    //  Get/set the ipaddress field

    public String ipaddress ()
    {
        return ipaddress;
    }

    public void setIpaddress (String format, Object ... args)
    {
        //  Format into newly allocated string
        ipaddress = String.format (format, args);
    }


    //  --------------------------------------------------------------------------
    //  Get/set the mailbox field

    public int mailbox ()
    {
        return mailbox;
    }

    public void setMailbox (int mailbox)
    {
        this.mailbox = mailbox;
    }


    //  --------------------------------------------------------------------------
    //  Iterate through the groups field, and append a groups value

    public List <String> groups ()
    {
        return groups;
    }

    public void appendGroups (String format, Object ... args)
    {
        //  Format into newly allocated string
        
        String string = String.format (format, args);
        //  Attach string to list
        if (groups == null)
            groups = new ArrayList <String> ();
        groups.add (string);
    }

    public void setGroups (Collection <String> value)
    {
        groups = new ArrayList (value); 
    }


    //  --------------------------------------------------------------------------
    //  Get/set the status field

    public int status ()
    {
        return status;
    }

    public void setStatus (int status)
    {
        this.status = status;
    }


    //  --------------------------------------------------------------------------
    //  Get/set a value in the headers dictionary

    public Map <String, String> headers ()
    {
        return headers;
    }

    public String headersString (String key, String defaultValue)
    {
        String value = null;
        if (headers != null)
            value = headers.get (key);
        if (value == null)
            value = defaultValue;

        return value;
    }

    public long headersNumber (String key, long defaultValue)
    {
        long value = defaultValue;
        String string = null;
        if (headers != null)
            string = headers.get (key);
        if (string != null)
            value = Long.valueOf (string);

        return value;
    }

    public void insertHeaders (String key, String format, Object ... args)
    {
        //  Format string into buffer
        String string = String.format (format, args);

        //  Store string in hash table
        if (headers == null)
            headers = new HashMap <String, String> ();
        headers.put (key, string);
        headersBytes += key.length () + 1 + string.length ();
    }

    public void setHeaders (Map <String, String> value)
    {
        if (value != null)
            headers = new HashMap <String, String> (value); 
        else
            headers = value;
    }


    //  --------------------------------------------------------------------------
    //  Get/set the content field

    public ZFrame content ()
    {
        return content;
    }

    //  Takes ownership of supplied frame
    public void setContent (ZFrame frame)
    {
        if (content != null)
            content.destroy ();
        content = frame;
    }

    //  --------------------------------------------------------------------------
    //  Get/set the group field

    public String group ()
    {
        return group;
    }

    public void setGroup (String format, Object ... args)
    {
        //  Format into newly allocated string
        group = String.format (format, args);
    }


}

