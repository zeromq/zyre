Problem: can't use Zyre from node.js

Solution: start on binding for Zyre

Note: this an experiment. The real binding will be generated via
zproject. However we will first make this binding work, and then
use it as a template.

Disclaimer: I am not an expert in Node, NAN, or C++, and this is
probably horribly done. The work is not ready for use.

## How to build

You need these repositories checked-out under a common root:

* https://github.com/zeromq/zyre
* https://github.com/zeromq/czmq
* https://github.com/zeromq/libzmq
* https://github.com/jedisct1/libsodium

In zyre/bindings/nodejs,

```
./build.sh
node test.js        #   It crashes nicely
```

# Areas you can help in

* Making this work on Windows or OS/X
* Adding more Zyre methods
* Passing arguments to and fro correctly
* Proper destruction of Zyre object at exit
* Better test case (zyre pinger?)
* Etc.

## Hints for contributors

* You need Python (v2.7 recommended, v3.x not supported)

* You need (ideally) nvm and Node.js:

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

* In every terminal, or .bashrc: `nvm use v5.5.0`

* If your Linux has an existing 'node' command, remove it. You need
  matching versions of node and node-gyp.
