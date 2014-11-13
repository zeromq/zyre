# Zyre - an open-source framework for proximity-based peer-to-peer applications

[![Build Status](https://travis-ci.org/zeromq/zyre.svg?branch=master)](https://travis-ci.org/zeromq/zyre)

## Description

Zyre does local area discovery and clustering. A Zyre node broadcasts
UDP beacons, and connects to peers that it finds. This class wraps a
Zyre node with a message-based API.

All incoming events are zmsg_t messages delivered via the zyre_recv
call. The first frame defines the type of the message, and following
frames provide further values:

    ENTER fromnode headers ipaddress
        a new peer has entered the network
    EXIT fromnode
        a peer has left the network
    JOIN fromnode groupname
        a peer has joined a specific group
    LEAVE fromnode groupname
        a peer has left a specific group
    WHISPER fromnode message
        a peer has sent this node a message
    SHOUT fromnode groupname message
        a peer has sent one of our groups a message

In SHOUT and WHISPER the message is a single frame in this version
of Zyre. In ENTER, the headers frame contains a packed dictionary,
see zhash_pack/unpack.

To join or leave a group, use the zyre_join and zyre_leave methods.
To set a header value, use the zyre_set_header method. To send a message
to a single peer, use zyre_whisper. To send a message to a group, use
zyre_shout.

## API

    //  Constructor, creates a new Zyre node. Note that until you start the
    //  node it is silent and invisible to other nodes on the network.
    zyre_t *
        zyre_new (zctx_t *ctx);

    //  Destructor, destroys a Zyre node. When you destroy a node, any
    //  messages it is sending or receiving will be discarded.
    void
        zyre_destroy (zyre_t **self_p);

    //  Set node header; these are provided to other nodes during discovery
    //  and come in each ENTER message.
    void
        zyre_set_header (zyre_t *self, char *name, char *format, ...);

    //  Start node, after setting header values. When you start a node it
    //  begins discovery and connection. There is no stop method; to stop
    //  a node, destroy it.
    void
        zyre_start (zyre_t *self);

    // Stop node, this signals to other peers that this node will go away.
    // This is polite; however you can also just destroy the node without
    // stopping it.
    void
        zyre_stop (zyre_t *self);

    //  Join a named group; after joining a group you can send messages to
    //  the group and all Zyre nodes in that group will receive them.
    int
        zyre_join (zyre_t *self, const char *group);

    //  Leave a group
    int
        zyre_leave (zyre_t *self, const char *group);

    //  Receive next message from network; the message may be a control
    //  message (ENTER, EXIT, JOIN, LEAVE) or data (WHISPER, SHOUT).
    //  Returns zmsg_t object, or NULL if interrupted
    zmsg_t *
        zyre_recv (zyre_t *self);

    // Send message to single peer, specified as a UUID string
    // Destroys message after sending
    int
        zyre_whisper (zyre_t *self, char *peer, zmsg_t **msg_p);


    // Send message to a named group
    // Destroys message after sending
    int
        zyre_shout (zyre_t *self, char *group, zmsg_t **msg_p);

    // Send string to single peer specified as a UUID string.
    // String is formatted using printf specifiers.
    int
        zyre_whispers (zyre_t *self, char *peer, char *format, ...);

    // Send message to a named group
    // Destroys message after sending
    int
        zyre_shouts (zyre_t *self, char *group, char *format, ...);
        
    //  Return handle to the Zyre node, for polling
    void *
        zyre_socket (zyre_t *self);

    //  Self test of this class
    void
        zyre_test (bool verbose);

## Example

    zctx_t *ctx = zctx_new ();
    // Create two nodes
    zyre_t *node1 = zyre_new (ctx);
    zyre_t *node2 = zyre_new (ctx);
    zyre_set_header (node1, "X-FILEMQ", "tcp://128.0.0.1:6777");
    zyre_set_header (node1, "X-HELLO", "World");
    zyre_start (node1);
    zyre_start (node2);
    zyre_join (node1, "GLOBAL");
    zyre_join (node2, "GLOBAL");

    // Give time for them to interconnect
    zclock_sleep (250);

    // One node shouts to GLOBAL
    zyre_shouts (node1,"GLOBAL", "Hello, World");

    // TODO: should timeout and not hang if there's no networking
    // ALSO why doesn't this work with localhost? zbeacon?
    // Second node should receive ENTER, JOIN, and SHOUT
    zmsg_t *msg = zyre_recv (node2);
    assert (msg);
    char *command = zmsg_popstr (msg);
    assert (streq (command, "ENTER"));
    free (command);
    char *peerid = zmsg_popstr (msg);
    free (peerid);
    zframe_t *headers_packed = zmsg_pop (msg);

    assert (headers_packed);
    zhash_t *headers = zhash_unpack (headers_packed);
    assert (headers);
    zframe_destroy (&headers_packed);
    assert (streq (zhash_lookup (headers, "X-HELLO"), "World"));
    zhash_destroy (&headers);
    zmsg_destroy (&msg);
    
    msg = zyre_recv (node2);
    assert (msg);
    command = zmsg_popstr (msg);
    assert (streq (command, "JOIN"));
    free (command);
    zmsg_destroy (&msg);
    
    msg = zyre_recv (node2);
    assert (msg);
    command = zmsg_popstr (msg);
    assert (streq (command, "SHOUT"));
    free (command);
    zmsg_destroy (&msg);
    
    zyre_stop (node1);
    zyre_stop (node2);
    
    zyre_destroy (&node1);
    zyre_destroy (&node2);
    zctx_destroy (&ctx);


## Extended API

Besides the plain receive method Zyre provides an API to receive an wrapped event. The obove explained message types are hereby replaced with the following enumerations:

    ENTER   = ZYRE_EVENT_ENTER

    EXIT    = ZYRE_EVENT_EXIT

    JOIN    = ZYRE_EVENT_JOIN

    LEAVE   = ZYRE_EVENT_LEAVE

    WHISPER = ZYRE_EVENT_WHISPER

    SHOUT   = ZYRE_EVENT_SHOUT

Events must be destroyed once you no longer need them.
    
    // Constructor: receive an event from the zyre node, wraps zyre_recv.
    // The event may be a control message (ENTER, EXIT, JOIN, LEAVE) or
    // data (WHISPER, SHOUT).
    CZMQ_EXPORT zyre_event_t *
    zyre_event_new (zyre_t *self);
    
    // Destructor; destroys an event instance
    void
        zyre_event_destroy (zyre_event_t **self_p);

    // Receive an event from the zyre node, wraps zyre_recv.
    // The event may be a control message (ENTER, EXIT, JOIN, LEAVE)
    // or data (WHISPER, SHOUT).
    zyre_event_t *
        zyre_event_recv (zyre_t *self);

    // Returns event type, which is a zyre_event_type_t
    zyre_event_type_t
        zyre_event_type (zyre_event_t *self);

    // Return the sending peer's id as a string
    char *
        zyre_event_sender (zyre_event_t *self);

    // Return the sending peer's ipaddress as a string
    char *
        zyre_event_address (zyre_event_t *self);

    // Returns the event headers, or NULL if there are none
    zhash_t *
        zyre_event_headers (zyre_event_t *self);

    // Returns value of a header from the message headers
    // obtained by ENTER. Return NULL if no value was found.
    char *
        zyre_event_header (zyre_event_t *self, char *name);

    // Returns the group name that a SHOUT event was sent to
    char *
        zyre_event_group (zyre_event_t *self);

    // Returns the incoming message payload (currently one frame)
    zmsg_t *
        zyre_event_msg (zyre_event_t *self);

## Example 

    zctx_t *ctx = zctx_new ();
    // Create two nodes
    zyre_t *node1 = zyre_new (ctx);
    zyre_t *node2 = zyre_new (ctx);
    zyre_set_header (node1, "X-FILEMQ", "tcp://128.0.0.1:6777");
    zyre_set_header (node1, "X-HELLO", "World");
    zyre_start (node1);
    zyre_start (node2);
    zyre_join (node1, "GLOBAL");
    zyre_join (node2, "GLOBAL");

    // Give time for them to interconnect
    zclock_sleep (250);

    // One node shouts to GLOBAL
    zmsg_t *msg = zmsg_new ();
    zmsg_addstr (msg, "Hello, World");
    zyre_shout (node1, "GLOBAL", &msg);

    // Parse ENTER
    zyre_event_t *zyre_event = zyre_event_recv (node2);
    assert (zyre_event_type (zyre_event) == ZYRE_EVENT_ENTER);
    char *sender = zyre_event_sender (zyre_event);
    assert (streq (zyre_event_header (zyre_event, "X-HELLO"), "World"));
    msg = zyre_event_msg (zyre_event);
    zyre_event_destroy (&zyre_event);
    
    // Parse JOIN
    zyre_event = zyre_event_recv (node2);
    assert (zyre_event_type (zyre_event) == ZYRE_EVENT_JOIN);
    zyre_event_destroy (&zyre_event);
    
    // Parse SHOUT
    zyre_event = zyre_event_recv (node2);
    assert (zyre_event_type (zyre_event) == ZYRE_EVENT_SHOUT);
    assert (streq (zyre_event_group (zyre_event), "GLOBAL"));
    msg = zyre_event_msg (zyre_event);
    char *string = zmsg_popstr (msg);
    assert (streq (string, "Hello, World"));
    free (string);
    zyre_event_destroy (&zyre_event);
    
    zyre_destroy (&node1);
    zyre_destroy (&node2);
    zctx_destroy (&ctx);

## Project Organization

Zyre is owned by all its authors and contributors. This is an open source
project licensed under the LGPLv3. To contribute to Zyre please read the
[C4.1 process](http://rfc.zeromq.org/spec:22) that we use.
