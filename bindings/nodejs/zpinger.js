//  Minimal sanity test

var ZyreBinding = require ('bindings')('zyre');
var zyre = new ZyreBinding.Zyre ("World");
zyre.setVerbose ();
zyre.start ();
zyre.join ("GLOBAL");

console.log ('Hello: ' + zyre.name ());
console.log ('Waiting for 2 seconds, then exiting');

var sleep = require ('sleep');
sleep.sleep (2);

zyre.print ();
zyre.stop ();
zyre.destroy ();
