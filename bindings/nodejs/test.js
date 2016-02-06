#   Minimal sanity test

var ZyreBinding = require ('bindings')('zyre');
var zyre = new ZyreBinding.Zyre ();
console.log ('Node name is: ' + zyre.name ());
zyre.destroy ();
