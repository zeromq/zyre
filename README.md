# Zyre - an open-source framework for proximity-based peer-to-peer applications

Zyre does local area discovery and clustering. A Zyre node broadcasts UDP beacons, and connects to peers that it finds. This class wraps a Zyre node with a message-based API.

All incoming events are zmsg_t messages delivered via the zyre_recv call. The first frame defines the type of the message:

    ENTER       a new peer has entered the network
    EXIT        a peer has left the network
    WHISPER     a peer has sent this node a message
    SHOUT       a peer has sent one of our groups a message

In all these cases the next frame after the type is the sending peer ID. In a SHOUT, the next frame is the group name. After that, in all cases, the following frame is the message content, limited to one frame in this version of Zyre.

To join or leave a group, use the zyre_join and zyre_leave methods. To set a header value, use the zyre_set method. To send a message to
a single peer, use zyre_whisper. To send a message to a group, use zyre_shout.

## Project Organization

Zyre is owned by all its authors and contributors. This is an open source project licensed under the LGPLv3. To contribute to Zyre please read the [C4.1 process](http://rfc.zeromq.org/spec:22) that we use.
