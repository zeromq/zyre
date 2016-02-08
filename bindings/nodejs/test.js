//  Minimal sanity test

var ZyreBinding = require ('bindings')('zyre');
var zyre = new ZyreBinding.Zyre ();

console.log ('Node name is: ' + zyre.name ());

console.log ('Press any key to exit');
process.stdin.setRawMode (true);
process.stdin.resume ();
process.stdin.on ('data', process.exit.bind (process, 0));

zyre.destroy ();
