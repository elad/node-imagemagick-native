var imagemagick = require('..')
,   debug       = 1
,   memwatch    = require('memwatch')
;

memwatch.on( 'leak', function( info ) {
    console.log( "leak: ", info );
});
memwatch.on('stats', function(stats) {
    console.log( "stats: ", stats );
});
var srcData = require('fs').readFileSync( "test.jpg" );

var hd = new memwatch.HeapDiff();

var ret = imagemagick.convert({
    srcData: srcData,
    width: 100,
    height: 100,
    resizeStyle: "fill",
    format: 'JPEG',
    debug: debug
});
require('assert').ok( ret );

var diff = hd.end();
console.log("diff: ", diff );
