//  Minimal sanity test

var ZyreBinding = require ('bindings')('zyre');
var zyre = new ZyreBinding.Zyre ();

console.log ('Node name is: ' + zyre.name ());
console.log ('Waiting for 2 seconds, then exiting');

var sleep = require ('sleep');
sleep.sleep (2);

zyre.destroy ();
