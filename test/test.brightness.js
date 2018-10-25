var test        = require('tap').test
    ,   imagemagick = require('..')
    ,   debug       = true
;

process.chdir(__dirname);

console.log("image magick's version is: " + imagemagick.version());
var versions = imagemagick.version().split(".");

function saveToFileIfDebug (buffer, file) {
    if (debug) {
        require('fs').writeFileSync( file, buffer, 'binary' );
        console.log( "wrote file: "+file );
    }
}

test( 'convert brightness to 100', function (t) {
    var buffer = imagemagick.convert({
        srcData: require('fs').readFileSync( "test.png" ), // 58x66
        width: 100,
        height: 100,
        quality: 80,
        format: 'PNG',
        brightness: 100,
        debug: debug
    });
    t.equal( Buffer.isBuffer(buffer), true, 'buffer is Buffer' );
    saveToFileIfDebug( buffer, "out.png-brightness-high.png" );
    t.end();
});

test( 'convert brightness to -100', function (t) {
    var buffer = imagemagick.convert({
        srcData: require('fs').readFileSync( "test.png" ), // 58x66
        width: 100,
        height: 100,
        quality: 80,
        format: 'PNG',
        brightness: -100,
        debug: debug
    });
    t.equal( Buffer.isBuffer(buffer), true, 'buffer is Buffer' );
    saveToFileIfDebug( buffer, "out.png-brightness-low.png" );
    t.end();
});