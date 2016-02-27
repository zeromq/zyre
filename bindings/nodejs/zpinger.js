/*  =========================================================================
    zpinger.js - ping other peers in a ZRE network

    -------------------------------------------------------------------------
    Copyright (c) the Contributors as noted in the AUTHORS file.

    This file is part of Zyre, an open-source framework for proximity-based
    peer-to-peer applications -- See http://zyre.org.

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
    =========================================================================
*/

/*
@header
zpinger tells you what other Zyre nodes are running. Use this to debug
network issues. Run with -v option to get more detail on Zyre's internal
flow for each event.
@discuss
Note that this will detect and speak to Zyre nodes on the entire local
network.
@end
*/

//  Implementation of zpinger in Node.js
//  When you run this, it shows activity on the Zyre network

Date.prototype.DateTime = function () {
    return (
        +  (this.getYear () - 100) + "-"
        + ((this.getMonth () + 1 < 10)? "0" :"") + (this.getMonth () + 1) + "-"
        + ((this.getDate () < 10)? "0" :"") + this.getDate () + " "
        + ((this.getHours () < 10)? "0": "") + this.getHours () + ":"
        + ((this.getMinutes () < 10)? "0": "") + this.getMinutes () + ":"
        + ((this.getSeconds () < 10)? "0": "") + this.getSeconds ()
    );
}

//  Print informational message prefixed by date/time
function info (string)
{
    var date = new Date ();
    console.log ("I: " + date.DateTime () + " " + string);
}

var verbose = false;

process.argv.forEach (function (value, index, array) {
    if (value == "-h" || value == "--help") {
        console.log ("zpinger.js [options] ...");
        console.log ("  --verbose / -v         verbose test output");
        console.log ("  --help / -h            this help");
        process.exit ();
    }
    else
    if (value == "-v" || value == "--verbose") {
        verbose = true;
        console.log (process.versions);
    }
});

var ZyreBinding = require ('bindings')('zyre');
var zyre = new ZyreBinding.Zyre ();
info ("Create Zyre node, uuid=" + zyre.uuid () + ", name=" + zyre.name ());

if (verbose)
    zyre.setVerbose ();

zyre.start ();
zyre.join ("GLOBAL");
if (verbose)
    zyre.print ();

while (true) {
    var event = new ZyreBinding.ZyreEvent (zyre);
    if (!event.defined ())
        break;              //  Interrupted
    if (verbose)
        event.print ();

    if (event.type () == "ENTER") {
        //  If new peer, say hello to it and wait for it to answer us
        info ("[" + event.peerName () + "] peer entered");
        zyre.whispers (event.peerUuid (), "Hello");
    }
    else
    if (event.type () == "EXIT")
        info ("[" + event.peerName () + "] peer exited");
    else
    if (event.type () == "WHISPER") {
        info ("[" + event.peerName () + "] received ping (WHISPER)");
        zyre.shouts ("GLOBAL", "Hello");
    }
    else
    if (event.type () == "SHOUT")
        info ("[" + event.peerName () + "]("
                  + event.group () + ") received ping (SHOUT)");

    event.destroy ();
}
zyre.stop ();
zyre.destroy ();
