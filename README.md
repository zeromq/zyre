
<A name="toc1-3" title="Zyre - Local Area Clustering for Peer-to-Peer Applications" />
# Zyre - Local Area Clustering for Peer-to-Peer Applications

[![Build Status](https://travis-ci.org/zeromq/zyre.svg?branch=master)](https://travis-ci.org/zeromq/zyre)

<A name="toc2-8" title="Contents" />
## Contents

&emsp;<a href="#toc2-13">Overview</a>

**<a href="#toc3-16">Scope and Goals</a>**

**<a href="#toc3-45">Ownership and License</a>**
&emsp;<a href="#toc2-56">Using Zyre</a>

**<a href="#toc3-59">Building on Linux</a>**
&emsp;<a href="#toc1-87">only execute this next line if interested in updating the man pages as well (adds to build time):</a>

**<a href="#toc3-112">Building on Windows</a>**

**<a href="#toc3-135">Building on Windows</a>**

**<a href="#toc3-140">Linking with an Application</a>**

**<a href="#toc3-182">Linking with an Application</a>**

**<a href="#toc3-189">API Summary</a>**
*  <a href="#toc4-194">zyre - API wrapping one Zyre node</a>
*  <a href="#toc4-574">zyre_event - no title found</a>

**<a href="#toc3-719">Hints to Contributors</a>**

**<a href="#toc3-730">This Document</a>**

<A name="toc2-13" title="Overview" />
## Overview

<A name="toc3-16" title="Scope and Goals" />
### Scope and Goals

Zyre provides reliable group messaging over local area networks. It has these key characteristics:

* Zyre needs no administration or configuration.
* Peers may join and leave the network at any time.
* Peers talk to each other without any central brokers or servers.
* Peers can talk directly to each other.
* Peers can join groups, and then talk to groups.
* Zyre is reliable, and loses no messages even when the network is heavily loaded.
* Zyre is fast and has low latency, requiring no consensus protocols.
* Zyre is designed for WiFi networks, yet also works well on Ethernet networks.
* Time for a new peer to join a network is about one second.

Typical use cases for Zyre are:

* Local service discovery.
* Clustering of a set of services on the same Ethernet network.
* Controlling a network of smart devices (Internet of Things).
* Multi-user mobile applications (like smart classrooms).

Technical details:

* Uses RFC 36 (http://rfc.zeromq.org/spec:36/ZRE) protocol for discovery and heartbeating.
* Uses reliable Dealer-Router pattern for interconnection, assuring that messages are not lost unless a peer application terminates.
* Optimized for WiFi, using UDP broadcasts for discovery and heartbeating…
* Offers alternative discovery mechanism (gossip) for Ethernet networks.

<A name="toc3-45" title="Ownership and License" />
### Ownership and License

The contributors are listed in AUTHORS. This project uses the MPL v2 license, see LICENSE.

Zyre uses the [C4.1 (Collective Code Construction Contract)](http://rfc.zeromq.org/spec:22) process for contributions.

Zyre uses the [CLASS (C Language Style for Scalabilty)](http://rfc.zeromq.org/spec:21) guide for code style.

To report an issue, use the [Zyre issue tracker](https://github.com/zeromq/zyre/issues) at github.com.

<A name="toc2-56" title="Using Zyre" />
## Using Zyre

<A name="toc3-59" title="Building on Linux" />
### Building on Linux

To start with, you need at least these packages:

* {{git-all}} -- git is how we share code with other people.

* {{build-essential}}, {{libtool}}, {{pkg-config}} - the C compiler and related tools.

* {{autotools-dev}}, {{autoconf}}, {{automake}} - the GNU autoconf makefile generators.

* {{cmake}} - the CMake makefile generators (an alternative to autoconf).

Plus some others:

* {{uuid-dev}}, {{libpcre3-dev}} - utility libraries.

* {{valgrind}} - a useful tool for checking your code.

Which we install like this (using the Debian-style apt-get package manager):

```
sudo apt-get update
sudo apt-get install -y \
    git-all build-essential libtool \
    pkg-config autotools-dev autoconf automake cmake \
    uuid-dev libpcre3-dev valgrind

<A name="toc1-87" title="only execute this next line if interested in updating the man pages as well (adds to build time):" />
# only execute this next line if interested in updating the man pages as well (adds to build time):
sudo apt-get install -y asciidoc
```
Here's how to build Zyre from GitHub (building from packages is very similar, you don't clone a repo but unpack a tarball), including the libsodium (for security) and libzmq (ZeroMQ core) libraries:

```
    git clone --depth 1 -b stable https://github.com/jedisct1/libsodium.git
    cd libsodium
    ./autogen.sh && ./configure && make check
    sudo make install
    cd ..

    git clone git://github.com/zeromq/libzmq.git
    cd libzmq
    ./autogen.sh
    # do not specify "--with-libsodium" if you prefer to use internal tweetnacl security implementation (recommended for development)
    ./configure --with-libsodium
    make check
    sudo make install
    sudo ldconfig
    cd ..

    git clone git://github.com/zeromq/czmq.git
    cd czmq
    ./autogen.sh && ./configure && make check
    sudo make install
    sudo ldconfig
    cd ..

    git clone git://github.com/zeromq/zyre.git
    cd zyre
    ./autogen.sh && ./configure && make check
    sudo make install
    sudo ldconfig
    cd ..
```


Test by running the `zyre_selftest` command:

    zyre\src\.libs\zyre_selftest

Test by running the `zpinger` command, from two or more PCs.

    zyre\src\.libs\zpinger

<A name="toc3-112" title="Building on Windows" />
<A name="toc3-135" title="Building on Windows" />
### Building on Windows

To start with, you need MS Visual Studio (C/C++). The free community edition works well.

Then, install git, and make sure it works from a DevStudio command prompt:

```
git
```

Now let's build Zyre from GitHub:

```
    git clone --depth 1 -b stable https://github.com/jedisct1/libsodium.git
    git clone git://github.com/zeromq/libzmq.git
    git clone git://github.com/zeromq/czmq.git
    git clone git://github.com/zeromq/zyre.git
    cd zyre\builds\msvc
    configure.bat
    cd build
    buildall.bat
    cd ..\..\..\..
```

Test by running the `zyre_selftest` command:
```
    dir/s/b zyre_selftest.exe
    zyre\builds\msvc\vs2013\DebugDEXE\zyre_selftest.exe
    zyre\builds\msvc\vs2013\ReleaseDEXE\zyre_selftest.exe

    :: select your choice and run it
    zyre\builds\msvc\vs2013\DebugDEXE\zyre_selftest.exe
```
Test by running `zpinger` from two or more PCs:

```
    dir/s/b zpinger.exe
    zyre\builds\msvc\vs2013\DebugDEXE\zpinger.exe
    zyre\builds\msvc\vs2013\ReleaseDEXE\zpinger.exe
    zyre\builds\msvc\vs2013\x64\DebugDEXE\zpinger.exe

    :: select your choice and run it
    zyre\builds\msvc\vs2013\ReleaseDEXE\zpinger.exe
```

<A name="toc3-140" title="Linking with an Application" />
<A name="toc3-182" title="Linking with an Application" />
### Linking with an Application

Include `zyre.h` in your application and link with libzyre. Here is a typical gcc link command:

    gcc -lzyre -lczmq -lzmq myapp.c -o myapp

<A name="toc3-189" title="API Summary" />
### API Summary

This is the API provided by Zyre 2.x, in alphabetical order.

<A name="toc4-194" title="zyre - API wrapping one Zyre node" />
#### zyre - API wrapping one Zyre node

Zyre does local area discovery and clustering. A Zyre node broadcasts
UDP beacons, and connects to peers that it finds. This class wraps a
Zyre node with a message-based API.

All incoming events are zmsg_t messages delivered via the zyre_recv
call. The first frame defines the type of the message, and following
frames provide further values:

    ENTER fromnode name headers ipaddress:port
        a new peer has entered the network
    EVASIVE fromnode name
	    a peer is being evasive (quiet for too long)
	EXIT fromnode name
        a peer has left the network
    JOIN fromnode name groupname
        a peer has joined a specific group
    LEAVE fromnode name groupname
        a peer has joined a specific group
    WHISPER fromnode name message
        a peer has sent this node a message
    SHOUT fromnode name groupname message
        a peer has sent one of our groups a message

In SHOUT and WHISPER the message is zero or more frames, and can hold
any ZeroMQ message. In ENTER, the headers frame contains a packed
dictionary, see zhash_pack/unpack.

To join or leave a group, use the zyre_join and zyre_leave methods.
To set a header value, use the zyre_set_header method. To send a message
to a single peer, use zyre_whisper. To send a message to a group, use
zyre_shout.

Todo: allow multipart contents

This is the class interface:

    //  This is a draft class, and may change without notice. It is disabled in
    //  stable builds by default. If you use this in applications, please ask
    //  for it to be pushed to stable state. Use --enable-drafts to enable.
    #ifdef ZYRE_BUILD_DRAFT_API
    //  *** Draft method, for development use, may change without warning ***
    //  Constructor, creates a new Zyre node. Note that until you start the
    //  node it is silent and invisible to other nodes on the network.     
    //  The node name is provided to other nodes during discovery. If you  
    //  specify NULL, Zyre generates a randomized node name from the UUID. 
    ZYRE_EXPORT zyre_t *
        zyre_new (const char *name);
    
    //  *** Draft method, for development use, may change without warning ***
    //  Destructor, destroys a Zyre node. When you destroy a node, any
    //  messages it is sending or receiving will be discarded.        
    ZYRE_EXPORT void
        zyre_destroy (zyre_t **self_p);
    
    //  *** Draft method, for development use, may change without warning ***
    //  Return our node UUID string, after successful initialization
    ZYRE_EXPORT const char *
        zyre_uuid (zyre_t *self);
    
    //  *** Draft method, for development use, may change without warning ***
    //  Return our node name, after successful initialization
    ZYRE_EXPORT const char *
        zyre_name (zyre_t *self);
    
    //  *** Draft method, for development use, may change without warning ***
    //  Set node header; these are provided to other nodes during discovery
    //  and come in each ENTER message.                                    
    ZYRE_EXPORT void
        zyre_set_header (zyre_t *self, const char *name, const char *format, ...);
    
    //  *** Draft method, for development use, may change without warning ***
    //  Set verbose mode; this tells the node to log all traffic as well as 
    //  all major events.                                                   
    ZYRE_EXPORT void
        zyre_set_verbose (zyre_t *self);
    
    //  *** Draft method, for development use, may change without warning ***
    //  Set UDP beacon discovery port; defaults to 5670, this call overrides 
    //  that so you can create independent clusters on the same network, for 
    //  e.g. development vs. production. Has no effect after zyre_start().   
    ZYRE_EXPORT void
        zyre_set_port (zyre_t *self, int port_nbr);
    
    //  *** Draft method, for development use, may change without warning ***
    //  Set UDP beacon discovery interval, in milliseconds. Default is instant
    //  beacon exploration followed by pinging every 1,000 msecs.             
    ZYRE_EXPORT void
        zyre_set_interval (zyre_t *self, size_t interval);
    
    //  *** Draft method, for development use, may change without warning ***
    //  Set network interface for UDP beacons. If you do not set this, CZMQ will
    //  choose an interface for you. On boxes with several interfaces you should
    //  specify which one you want to use, or strange things can happen.        
    ZYRE_EXPORT void
        zyre_set_interface (zyre_t *self, const char *value);
    
    //  *** Draft method, for development use, may change without warning ***
    //  By default, Zyre binds to an ephemeral TCP port and broadcasts the local 
    //  host name using UDP beaconing. When you call this method, Zyre will use  
    //  gossip discovery instead of UDP beaconing. You MUST set-up the gossip    
    //  service separately using zyre_gossip_bind() and _connect(). Note that the
    //  endpoint MUST be valid for both bind and connect operations. You can use 
    //  inproc://, ipc://, or tcp:// transports (for tcp://, use an IP address   
    //  that is meaningful to remote as well as local nodes). Returns 0 if       
    //  the bind was successful, else -1.                                        
    ZYRE_EXPORT int
        zyre_set_endpoint (zyre_t *self, const char *format, ...);
    
    //  *** Draft method, for development use, may change without warning ***
    //  Set-up gossip discovery of other nodes. At least one node in the cluster
    //  must bind to a well-known gossip endpoint, so other nodes can connect to
    //  it. Note that gossip endpoints are completely distinct from Zyre node   
    //  endpoints, and should not overlap (they can use the same transport).    
    ZYRE_EXPORT void
        zyre_gossip_bind (zyre_t *self, const char *format, ...);
    
    //  *** Draft method, for development use, may change without warning ***
    //  Set-up gossip discovery of other nodes. A node may connect to multiple
    //  other nodes, for redundancy paths. For details of the gossip network  
    //  design, see the CZMQ zgossip class.                                   
    ZYRE_EXPORT void
        zyre_gossip_connect (zyre_t *self, const char *format, ...);
    
    //  *** Draft method, for development use, may change without warning ***
    //  Start node, after setting header values. When you start a node it
    //  begins discovery and connection. Returns 0 if OK, -1 if it wasn't
    //  possible to start the node.                                      
    ZYRE_EXPORT int
        zyre_start (zyre_t *self);
    
    //  *** Draft method, for development use, may change without warning ***
    //  Stop node; this signals to other peers that this node will go away.
    //  This is polite; however you can also just destroy the node without 
    //  stopping it.                                                       
    ZYRE_EXPORT void
        zyre_stop (zyre_t *self);
    
    //  *** Draft method, for development use, may change without warning ***
    //  Join a named group; after joining a group you can send messages to
    //  the group and all Zyre nodes in that group will receive them.     
    ZYRE_EXPORT int
        zyre_join (zyre_t *self, const char *group);
    
    //  *** Draft method, for development use, may change without warning ***
    //  Leave a group
    ZYRE_EXPORT int
        zyre_leave (zyre_t *self, const char *group);
    
    //  *** Draft method, for development use, may change without warning ***
    //  Receive next message from network; the message may be a control
    //  message (ENTER, EXIT, JOIN, LEAVE) or data (WHISPER, SHOUT).   
    //  Returns zmsg_t object, or NULL if interrupted                  
    ZYRE_EXPORT zmsg_t *
        zyre_recv (zyre_t *self);
    
    //  *** Draft method, for development use, may change without warning ***
    //  Send message to single peer, specified as a UUID string
    //  Destroys message after sending                         
    ZYRE_EXPORT int
        zyre_whisper (zyre_t *self, const char *peer, zmsg_t **msg_p);
    
    //  *** Draft method, for development use, may change without warning ***
    //  Send message to a named group 
    //  Destroys message after sending
    ZYRE_EXPORT int
        zyre_shout (zyre_t *self, const char *group, zmsg_t **msg_p);
    
    //  *** Draft method, for development use, may change without warning ***
    //  Send formatted string to a single peer specified as UUID string
    ZYRE_EXPORT int
        zyre_whispers (zyre_t *self, const char *peer, const char *format, ...);
    
    //  *** Draft method, for development use, may change without warning ***
    //  Send formatted string to a named group
    ZYRE_EXPORT int
        zyre_shouts (zyre_t *self, const char *group, const char *format, ...);
    
    //  *** Draft method, for development use, may change without warning ***
    //  Return zlist of current peer ids.
    //  Caller owns return value and must destroy it when done.
    ZYRE_EXPORT zlist_t *
        zyre_peers (zyre_t *self);
    
    //  *** Draft method, for development use, may change without warning ***
    //  Return zlist of currently joined groups.
    //  Caller owns return value and must destroy it when done.
    ZYRE_EXPORT zlist_t *
        zyre_own_groups (zyre_t *self);
    
    //  *** Draft method, for development use, may change without warning ***
    //  Return zlist of groups known through connected peers.
    //  Caller owns return value and must destroy it when done.
    ZYRE_EXPORT zlist_t *
        zyre_peer_groups (zyre_t *self);
    
    //  *** Draft method, for development use, may change without warning ***
    //  Return the endpoint of a connected peer.
    //  Caller owns return value and must destroy it when done.
    ZYRE_EXPORT char *
        zyre_peer_address (zyre_t *self, const char *peer);
    
    //  *** Draft method, for development use, may change without warning ***
    //  Return the value of a header of a conected peer. 
    //  Returns null if peer or key doesn't exits.       
    //  Caller owns return value and must destroy it when done.
    ZYRE_EXPORT char *
        zyre_peer_header_value (zyre_t *self, const char *peer, const char *name);
    
    //  *** Draft method, for development use, may change without warning ***
    //  Return socket for talking to the Zyre node, for polling
    ZYRE_EXPORT zsock_t *
        zyre_socket (zyre_t *self);
    
    //  *** Draft method, for development use, may change without warning ***
    //  Print zyre node information to stdout
    ZYRE_EXPORT void
        zyre_print (zyre_t *self);
    
    //  *** Draft method, for development use, may change without warning ***
    //  Return the Zyre version for run-time API detection
    ZYRE_EXPORT void
        zyre_version (int *major, int *minor, int *patch);
    
    //  *** Draft method, for development use, may change without warning ***
    //  Self test of this class.
    ZYRE_EXPORT void
        zyre_test (bool verbose);
    
    #endif // ZYRE_BUILD_DRAFT_API

This is the class self test code:

    //  We'll use inproc gossip discovery so that this works without networking
    
    int major, minor, patch;
    zyre_version (&major, &minor, &patch);
    assert (major == ZYRE_VERSION_MAJOR);
    assert (minor == ZYRE_VERSION_MINOR);
    assert (patch == ZYRE_VERSION_PATCH);
    
    //  Create two nodes
    zyre_t *node1 = zyre_new ("node1");
    assert (node1);
    assert (streq (zyre_name (node1), "node1"));
    zyre_set_header (node1, "X-HELLO", "World");
    if (verbose)
        zyre_set_verbose (node1);
    
    //  Set inproc endpoint for this node
    int rc = zyre_set_endpoint (node1, "inproc://zyre-node1");
    assert (rc == 0);
    //  Set up gossip network for this node
    zyre_gossip_bind (node1, "inproc://gossip-hub");
    rc = zyre_start (node1);
    assert (rc == 0);
    
    zyre_t *node2 = zyre_new ("node2");
    assert (node2);
    assert (streq (zyre_name (node2), "node2"));
    if (verbose)
        zyre_set_verbose (node2);
    
    //  Set inproc endpoint for this node
    //  First, try to use existing name, it'll fail
    rc = zyre_set_endpoint (node2, "inproc://zyre-node1");
    assert (rc == -1);
    //  Now use available name and confirm that it succeeds
    rc = zyre_set_endpoint (node2, "inproc://zyre-node2");
    assert (rc == 0);
    
    //  Set up gossip network for this node
    zyre_gossip_connect (node2, "inproc://gossip-hub");
    rc = zyre_start (node2);
    assert (rc == 0);
    assert (strneq (zyre_uuid (node1), zyre_uuid (node2)));
    
    zyre_join (node1, "GLOBAL");
    zyre_join (node2, "GLOBAL");
    
    //  Give time for them to interconnect
    zclock_sleep (250);
    if (verbose)
        zyre_dump (node1);
    
    zlist_t *peers = zyre_peers (node1);
    assert (peers);
    assert (zlist_size (peers) == 1);
    zlist_destroy (&peers);
    
    zyre_join (node1, "node1 group of one");
    zyre_join (node2, "node2 group of one");
    
    // Give them time to join their groups
    zclock_sleep (250);
    
    zlist_t *own_groups = zyre_own_groups (node1);
    assert (own_groups);
    assert (zlist_size (own_groups) == 2);
    zlist_destroy (&own_groups);
    
    zlist_t *peer_groups = zyre_peer_groups (node1);
    assert (peer_groups);
    assert (zlist_size (peer_groups) == 2);
    zlist_destroy (&peer_groups);
    
    char *value = zyre_peer_header_value (node2, zyre_uuid (node1), "X-HELLO");
    assert (streq (value, "World"));
    zstr_free (&value);
    
    //  One node shouts to GLOBAL
    zyre_shouts (node1, "GLOBAL", "Hello, World");
    
    //  Second node should receive ENTER, JOIN, and SHOUT
    zmsg_t *msg = zyre_recv (node2);
    assert (msg);
    char *command = zmsg_popstr (msg);
    assert (streq (command, "ENTER"));
    zstr_free (&command);
    assert (zmsg_size (msg) == 4);
    char *peerid = zmsg_popstr (msg);
    char *name = zmsg_popstr (msg);
    assert (streq (name, "node1"));
    zstr_free (&name);
    zframe_t *headers_packed = zmsg_pop (msg);
    
    char *address = zmsg_popstr (msg);
    char *endpoint = zyre_peer_address (node2, peerid);
    assert (streq (address, endpoint));
    zstr_free (&peerid);
    zstr_free (&endpoint);
    zstr_free (&address);
    
    assert (headers_packed);
    zhash_t *headers = zhash_unpack (headers_packed);
    assert (headers);
    zframe_destroy (&headers_packed);
    assert (streq ((char *) zhash_lookup (headers, "X-HELLO"), "World"));
    zhash_destroy (&headers);
    zmsg_destroy (&msg);
    
    msg = zyre_recv (node2);
    assert (msg);
    command = zmsg_popstr (msg);
    assert (streq (command, "JOIN"));
    zstr_free (&command);
    assert (zmsg_size (msg) == 3);
    zmsg_destroy (&msg);
    
    msg = zyre_recv (node2);
    assert (msg);
    command = zmsg_popstr (msg);
    assert (streq (command, "JOIN"));
    zstr_free (&command);
    assert (zmsg_size (msg) == 3);
    zmsg_destroy (&msg);
    
    msg = zyre_recv (node2);
    assert (msg);
    command = zmsg_popstr (msg);
    assert (streq (command, "SHOUT"));
    zstr_free (&command);
    zmsg_destroy (&msg);
    
    zyre_stop (node2);
    
    msg = zyre_recv (node2);
    assert (msg);
    command = zmsg_popstr (msg);
    assert (streq (command, "STOP"));
    zstr_free (&command);
    zmsg_destroy (&msg);
    
    zyre_stop (node1);
    
    zyre_destroy (&node1);
    zyre_destroy (&node2);

<A name="toc4-574" title="zyre_event - no title found" />
#### zyre_event - no title found

This class provides a higher-level API to the zyre_recv call, by doing
work that you will want to do in many cases, such as unpacking the peer
headers for each ENTER event received.

Please add @discuss section in ../src/zyre_event.c.

This is the class interface:

    //  This API is a draft, and may change without notice.
    #ifdef ZYRE_BUILD_DRAFT_API
    //  *** Draft method, for development use, may change without warning ***
    //  Constructor: receive an event from the zyre node, wraps zyre_recv.
    //  The event may be a control message (ENTER, EXIT, JOIN, LEAVE) or  
    //  data (WHISPER, SHOUT).                                            
    ZYRE_EXPORT zyre_event_t *
        zyre_event_new (zyre_t *node);
    
    //  *** Draft method, for development use, may change without warning ***
    //  Destructor; destroys an event instance
    ZYRE_EXPORT void
        zyre_event_destroy (zyre_event_t **self_p);
    
    //  *** Draft method, for development use, may change without warning ***
    //  Returns event type, as printable uppercase string
    ZYRE_EXPORT const char *
        zyre_event_type (zyre_event_t *self);
    
    //  *** Draft method, for development use, may change without warning ***
    //  Return the sending peer's uuid as a string
    ZYRE_EXPORT const char *
        zyre_event_peer_uuid (zyre_event_t *self);
    
    //  *** Draft method, for development use, may change without warning ***
    //  Return the sending peer's public name as a string
    ZYRE_EXPORT const char *
        zyre_event_peer_name (zyre_event_t *self);
    
    //  *** Draft method, for development use, may change without warning ***
    //  Return the sending peer's ipaddress as a string
    ZYRE_EXPORT const char *
        zyre_event_peer_addr (zyre_event_t *self);
    
    //  *** Draft method, for development use, may change without warning ***
    //  Returns the event headers, or NULL if there are none
    ZYRE_EXPORT zhash_t *
        zyre_event_headers (zyre_event_t *self);
    
    //  *** Draft method, for development use, may change without warning ***
    //  Returns value of a header from the message headers   
    //  obtained by ENTER. Return NULL if no value was found.
    ZYRE_EXPORT const char *
        zyre_event_header (zyre_event_t *self, const char *name);
    
    //  *** Draft method, for development use, may change without warning ***
    //  Returns the group name that a SHOUT event was sent to
    ZYRE_EXPORT const char *
        zyre_event_group (zyre_event_t *self);
    
    //  *** Draft method, for development use, may change without warning ***
    //  Returns the incoming message payload
    ZYRE_EXPORT zmsg_t *
        zyre_event_msg (zyre_event_t *self);
    
    //  *** Draft method, for development use, may change without warning ***
    //  Print event to zsys log
    ZYRE_EXPORT void
        zyre_event_print (zyre_event_t *self);
    
    //  *** Draft method, for development use, may change without warning ***
    //  Self test of this class.
    ZYRE_EXPORT void
        zyre_event_test (bool verbose);
    
    #endif // ZYRE_BUILD_DRAFT_API

This is the class self test code:

    //  Create two nodes
    zyre_t *node1 = zyre_new ("node1");
    assert (node1);
    zyre_set_header (node1, "X-HELLO", "World");
    if (verbose)
        zyre_set_verbose (node1);
    if (zyre_start (node1)) {
        zyre_destroy (&node1);
        printf ("OK (skipping test, no UDP discovery)\n");
        return;
    }
    zyre_join (node1, "GLOBAL");
    
    zyre_t *node2 = zyre_new ("node2");
    assert (node2);
    if (verbose)
        zyre_set_verbose (node2);
    int rc = zyre_start (node2);
    assert (rc == 0);
    zyre_join (node2, "GLOBAL");
    
    //  Give time for them to interconnect
    zclock_sleep (250);
    
    //  One node shouts to GLOBAL
    zmsg_t *msg = zmsg_new ();
    zmsg_addstr (msg, "Hello, World");
    zyre_shout (node1, "GLOBAL", &msg);
    zclock_sleep (100);
    
    //  Parse ENTER
    zyre_event_t *event = zyre_event_new (node2);
    assert (streq (zyre_event_type (event), "ENTER"));
    const char *sender = zyre_event_peer_uuid (event);
    assert (sender);
    const char *name = zyre_event_peer_name (event);
    assert (name);
    assert (streq (name, "node1"));
    const char *address = zyre_event_peer_addr (event);
    assert (address);
    const char *header = zyre_event_header (event, "X-HELLO");
    assert (header);
    zyre_event_destroy (&event);
    
    //  Parse JOIN
    //  We tolerate other events, which we can get if there are instances
    //  of Zyre running somewhere on the network.
    event = zyre_event_new (node2);
    if (streq (zyre_event_type (event), "JOIN")) {
        //  Parse SHOUT
        zyre_event_destroy (&event);
        event = zyre_event_new (node2);
        if (streq (zyre_event_type (event), "SHOUT")) {
            assert (streq (zyre_event_group (event), "GLOBAL"));
            msg = zyre_event_msg (event);
            char *string = zmsg_popstr (msg);
            assert (streq (string, "Hello, World"));
            free (string);
        }
        zyre_event_destroy (&event);
    }
    zyre_destroy (&node1);
    zyre_destroy (&node2);


<A name="toc3-719" title="Hints to Contributors" />
### Hints to Contributors

Zyre is a nice, neat library, and you may not immediately appreciate why. Read the CLASS style guide please, and write your code to make it indistinguishable from the rest of the code in the library. That is the only real criteria for good style: it's invisible.

Don't include system headers in source files. The right place for these is CZMQ.

Do read your code after you write it and ask, "Can I make this simpler?" We do use a nice minimalist and yet readable style. Learn it, adopt it, use it.

Before opening a pull request read our [contribution guidelines](https://github.com/zeromq/zyre/blob/master/CONTRIBUTING.md). Thanks!

<A name="toc3-730" title="This Document" />
### This Document

This document is originally at README.txt and is built using [gitdown](http://github.com/imatix/gitdown).

_This documentation was generated using [Gitdown](https://github.com/zeromq/gitdown)_
