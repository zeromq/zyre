# Node.js Binding for zyre

This is a development kit. Note: this README is generated automatically
by zproject from project.xml. Please DO NOT modify by hand except for test
purposes.

## Prerequisites

### Node.js

* You need Python (v2.7 recommended, v3.x not supported)
* You need (I recommend) nvm and Node.js.
* If your Linux has an existing 'node' command, `sudo apt-get remove node`.
* In every terminal, or .bashrc: `nvm use v5.5.0`

To install the necessary Node tools:

```
sudo apt-get update
sudo apt-get install build-essential libssl-dev
curl https://raw.githubusercontent.com/creationix/nvm/v0.11.1/install.sh | bash
# close terminal, re-open
nvm ls-remote
nvm install v5.5.0
npm install -g nan
npm install -g node-ninja
npm install -g prebuild
npm install -g bindings
```

To build:

```
mkdir -p $HOME/temp
cd $HOME/temp
git clone https://github.com/zeromq/zyre
cd zyre/bindings/nodejs
#   Clones dependencies, builds everything
./build.sh
```

## API

This is a wrapping of the native C libzyre library. See binding.cc for the code.

### The Zyre class - An open-source framework for proximity-based P2P apps

Constructor:

```
var zyre = require ('bindings')('zyre')
var my_zyre = new zyre.Zyre (String)
```

You *must* call the destructor on every Zyre instance:

```
my_zyre.destroy ()
```

Methods:

```
string my_zyre.uuid ()
```

Return our node UUID string, after successful initialization

```
string my_zyre.name ()
```

Return our node name, after successful initialization. First 6
characters of UUID by default.

```
nothing my_zyre.setName (String)
```

Set the public name of this node overriding the default. The name is
provide during discovery and come in each ENTER message.

```
nothing my_zyre.setHeader (String, String)
```

Set node header; these are provided to other nodes during discovery
and come in each ENTER message.

```
nothing my_zyre.setVerbose ()
```

Set verbose mode; this tells the node to log all traffic as well as
all major events.

```
nothing my_zyre.setPort (Number)
```

Set UDP beacon discovery port; defaults to 5670, this call overrides
that so you can create independent clusters on the same network, for
e.g. development vs. production. Has no effect after zyre_start().

```
nothing my_zyre.setBeaconPeerPort (Number)
```

Set the TCP port bound by the ROUTER peer-to-peer socket (beacon mode).
Defaults to * (the port is randomly assigned by the system).
This call overrides this, to bypass some firewall issues when ports are
random. Has no effect after zyre_start().

```
nothing my_zyre.setEvasiveTimeout (Number)
```

Set the peer evasiveness timeout, in milliseconds. Default is 5000.
This can be tuned in order to deal with expected network conditions
and the response time expected by the application. This is tied to
the beacon interval and rate of messages received.

```
nothing my_zyre.setExpiredTimeout (Number)
```

Set the peer expiration timeout, in milliseconds. Default is 30000.
This can be tuned in order to deal with expected network conditions
and the response time expected by the application. This is tied to
the beacon interval and rate of messages received.

```
nothing my_zyre.setInterval ()
```

Set UDP beacon discovery interval, in milliseconds. Default is instant
beacon exploration followed by pinging every 1,000 msecs.

```
nothing my_zyre.setInterface (String)
```

Set network interface for UDP beacons. If you do not set this, CZMQ will
choose an interface for you. On boxes with several interfaces you should
specify which one you want to use, or strange things can happen.

```
integer my_zyre.setEndpoint (String)
```

By default, Zyre binds to an ephemeral TCP port and broadcasts the local
host name using UDP beaconing. When you call this method, Zyre will use
gossip discovery instead of UDP beaconing. You MUST set-up the gossip
service separately using zyre_gossip_bind() and _connect(). Note that the
endpoint MUST be valid for both bind and connect operations. You can use
inproc://, ipc://, or tcp:// transports (for tcp://, use an IP address
that is meaningful to remote as well as local nodes). Returns 0 if
the bind was successful, else -1.

```
nothing my_zyre.setContestInGroup (String)
```

This options enables a peer to actively contest for leadership in the
given group. If this option is not set the peer will still participate in
elections but never gets elected. This ensures that a consent for a leader
is reached within a group even though not every peer is contesting for
leadership.

```
nothing my_zyre.setAdvertisedEndpoint (String)
```

Set an alternative endpoint value when using GOSSIP ONLY. This is useful
if you're advertising an endpoint behind a NAT.

```
nothing my_zyre.setZcert (Zcert)
```

Apply a azcert to a Zyre node.

```
nothing my_zyre.setZapDomain (String)
```

Specify the ZAP domain (for use with CURVE).

```
nothing my_zyre.gossipBind (String)
```

Set-up gossip discovery of other nodes. At least one node in the cluster
must bind to a well-known gossip endpoint, so other nodes can connect to
it. Note that gossip endpoints are completely distinct from Zyre node
endpoints, and should not overlap (they can use the same transport).

```
nothing my_zyre.gossipConnect (String)
```

Set-up gossip discovery of other nodes. A node may connect to multiple
other nodes, for redundancy paths. For details of the gossip network
design, see the CZMQ zgossip class.

```
nothing my_zyre.gossipConnectCurve (String, String)
```

Set-up gossip discovery with CURVE enabled.

```
nothing my_zyre.gossipUnpublish (String)
```

Unpublish a GOSSIP node from local list, useful in removing nodes from list when they EXIT

```
integer my_zyre.start ()
```

Start node, after setting header values. When you start a node it
begins discovery and connection. Returns 0 if OK, -1 if it wasn't
possible to start the node.

```
nothing my_zyre.stop ()
```

Stop node; this signals to other peers that this node will go away.
This is polite; however you can also just destroy the node without
stopping it.

```
integer my_zyre.join (String)
```

Join a named group; after joining a group you can send messages to
the group and all Zyre nodes in that group will receive them.

```
integer my_zyre.leave (String)
```

Leave a group

```
zmsg my_zyre.recv ()
```

Receive next message from network; the message may be a control
message (ENTER, EXIT, JOIN, LEAVE) or data (WHISPER, SHOUT).
Returns zmsg_t object, or NULL if interrupted

```
integer my_zyre.whisper (String, Zmsg)
```

Send message to single peer, specified as a UUID string
Destroys message after sending

```
integer my_zyre.shout (String, Zmsg)
```

Send message to a named group
Destroys message after sending

```
integer my_zyre.whispers (String, String)
```

Send formatted string to a single peer specified as UUID string

```
integer my_zyre.shouts (String, String)
```

Send formatted string to a named group

```
zlist my_zyre.peers ()
```

Return zlist of current peer ids.

```
zlist my_zyre.peersByGroup (String)
```

Return zlist of current peers of this group.

```
zlist my_zyre.ownGroups ()
```

Return zlist of currently joined groups.

```
zlist my_zyre.peerGroups ()
```

Return zlist of groups known through connected peers.

```
string my_zyre.peerAddress (String)
```

Return the endpoint of a connected peer.
Returns empty string if peer does not exist.

```
string my_zyre.peerHeaderValue (String, String)
```

Return the value of a header of a conected peer.
Returns null if peer or key doesn't exits.

```
integer my_zyre.requirePeer (String, String, String)
```

Explicitly connect to a peer

```
zsock my_zyre.socket ()
```

Return socket for talking to the Zyre node, for polling

```
nothing my_zyre.print ()
```

Print zyre node information to stdout

```
number my_zyre.version ()
```

Return the Zyre version for run-time API detection; returns
major * 10000 + minor * 100 + patch, as a single integer.

```
nothing my_zyre.test (Boolean)
```

Self test of this class.

### The ZyreEvent class - Parsing Zyre messages

Constructor:

```
var zyre = require ('bindings')('zyre')
var my_zyre_event = new zyre.ZyreEvent (Zyre)
```

You *must* call the destructor on every ZyreEvent instance:

```
my_zyre_event.destroy ()
```

Methods:

```
string my_zyre_event.type ()
```

Returns event type, as printable uppercase string. Choices are:
"ENTER", "EXIT", "JOIN", "LEAVE", "EVASIVE", "WHISPER" and "SHOUT"
and for the local node: "STOP"

```
string my_zyre_event.peerUuid ()
```

Return the sending peer's uuid as a string

```
string my_zyre_event.peerName ()
```

Return the sending peer's public name as a string

```
string my_zyre_event.peerAddr ()
```

Return the sending peer's ipaddress as a string

```
zhash my_zyre_event.headers ()
```

Returns the event headers, or NULL if there are none

```
string my_zyre_event.header (String)
```

Returns value of a header from the message headers
obtained by ENTER. Return NULL if no value was found.

```
string my_zyre_event.group ()
```

Returns the group name that a SHOUT event was sent to

```
zmsg my_zyre_event.msg ()
```

Returns the incoming message payload; the caller can modify the
message but does not own it and should not destroy it.

```
zmsg my_zyre_event.getMsg ()
```

Returns the incoming message payload, and pass ownership to the
caller. The caller must destroy the message when finished with it.
After called on the given event, further calls will return NULL.

```
nothing my_zyre_event.print ()
```

Print event to zsys log

```
nothing my_zyre_event.test (Boolean)
```

Self test of this class.
