# Zyre binding for Node.js

## Prerequisites

This is not a pre-built module. Sorry. Before running "npm install" you must build/install libsodium, libzmq, libczmq, and libzyre. Frankly it's simpler to build/install without using npm:

```
mkdir -p $HOME/temp
cd $HOME/temp
git clone https://github.com/zeromq/zyre
cd zyre/bindings/nodejs
./build.sh
```

And then copy build/Release/zyre.node to the node_modules/ directory of whatever application you want to tey it in.

## Quick Start

This package wraps the Zyre library for Node.js. Here is an example of use:

```
var ZyreBinding = require ('bindings')('zyre');
var zyre = new ZyreBinding.Zyre ();
console.log ("Create Zyre node, uuid=" + zyre.uuid () + " name=" + zyre.name ());

zyre.start ();
zyre.join ("GLOBAL");
zyre.print ();

while (true) {
    var event = new ZyreBinding.ZyreEvent (zyre);
    if (!event.defined ())
        break;              //  Interrupted
    event.print ();

    if (event.type_name () == "ENTER") {
        //  If new peer, say hello to it and wait for it to answer us
        console.log ("[" + event.peer_name () + "] peer entered");
        zyre.whisper (event.peer_uuid (), "Hello");
    }
    else
    if (event.type_name () == "EXIT") {
        console.log ("[" + event.peer_name () + "] peer exited");
    }
    else
    if (event.type_name () == "WHISPER") {
        console.log ("[" + event.peer_name () + "] received ping (WHISPER)");
        zyre.shout ("GLOBAL", "Hello");
    }
    else
    if (event.type_name () == "SHOUT") {
        console.log ("[" + event.peer_name () + "]("
                  + event.group () + ") received ping (SHOUT)");
    }
    event.destroy ();
}
zyre.stop ();
zyre.destroy ();
```

## Implementation

This is a wrapping of the native C libzyre library. See binding.cc for the code.

We get two classes:

    Zyre        - a Zyre node instance
    ZyreEvent   - an event from the Zyre network

The Zyre class has these methods:

    destroy
    defined
    uuid
    name
    start
    stop
    setHeader (name, value)
    setVerbose
    join (group)
    leave (group)
    print
    whisper (peer_uuid, message)
    shout (group, message)
    recv

The ZyreEvent class has these methods:

    destroy
    defined
    type
    type_name
    peer_uuid
    peer_name
    peer_addr
    header (name)
    group
    msg
    print

## Building from Source

* You need Python (v2.7 recommended, v3.x not supported)
* You need (ideally) nvm and Node.js.
* If your Linux has an existing 'node' command, remove it. You need matching versions of node and node-gyp.
* In every terminal, or .bashrc: `nvm use v5.5.0`

To install the necessary Node tools:

```
sudo apt-get update
sudo apt-get install build-essential libssl-dev
curl https://raw.githubusercontent.com/creationix/nvm/v0.11.1/install.sh | bash
# close terminal, re-open
nvm ls-remote
nvm install v5.5.0
npm install -g node-gyp
npm install -g node-pre-gyp
```

To build (in theory):

```
mkdir -p $HOME/temp
cd $HOME/temp
git clone https://github.com/zeromq/zyre
cd zyre/bindings/nodejs
./build.sh
```

## Making Changes

Note that this version of the binding is destined for destruction; we will replace it with one generated via zproject, with essentially the same API.

Having said that, fixes and improvements are welcome. Please send your pull requests to `https://github.com/zeromq/zyre`.

