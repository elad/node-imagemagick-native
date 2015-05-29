var test        = require('tap').test
,   imagemagick = require('..')
,   debug       = 1
;

console.log("image magick's version is: " + imagemagick.version());
var versions = imagemagick.version().split(".");

function saveToFileIfDebug (buffer, file) {
    if (debug) {
        require('fs').writeFileSync( file, buffer, 'binary' );
        console.log( "wrote file: "+file );
    }
}

test( 'taller default', function (t) {
    var buffer = imagemagick.convert({
        srcData: require('fs').readFileSync( "./test/test.png" ), // 58x66
        width: 100,
        height: 100,
        resizeStyle: 'aspectfill',
        quality: 80,
        format: 'PNG',
        debug: debug
    });
    t.equal( Buffer.isBuffer(buffer), true, 'buffer is Buffer' );
    saveToFileIfDebug( buffer, "out.gravity-default.png" );
    t.end();
});

[   "Center",
    "East",
    "West",
    "North",
    "South",
    "NorthEast",
    "NorthWest",
    "SouthEast",
    "SouthWest",
    "None",
].forEach( function (gravity) {
    test( 'taller ' + gravity, function (t) {
        var buffer = imagemagick.convert({
            srcData: require('fs').readFileSync( "./test/test.png" ), // 58x66
            width: 100,
            height: 100,
            resizeStyle: 'aspectfill',
            gravity: gravity,
            quality: 80,
            format: 'PNG',
            debug: debug
        });
        t.equal( Buffer.isBuffer(buffer), true, 'buffer is Buffer' );
        saveToFileIfDebug( buffer, "out.gravity-"+gravity+".png" );
        t.end();
    });
    test( 'wide ' + gravity, function (t) {
        var buffer = imagemagick.convert({
            srcData: require('fs').readFileSync( "./test/test.wide.png" ), // 58x66
            width: 100,
            height: 100,
            resizeStyle: 'aspectfill',
            gravity: gravity,
            quality: 80,
            format: 'PNG',
            debug: debug
        });
        t.equal( Buffer.isBuffer(buffer), true, 'buffer is Buffer' );
        saveToFileIfDebug( buffer, "out.wide.gravity-"+gravity+".png" );
        t.end();
    });
} );
