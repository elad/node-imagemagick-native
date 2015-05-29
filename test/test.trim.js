var test        = require('tap').test
,   imagemagick = require('..')
,   debug       = 0
;

console.log("image magick's version is: " + imagemagick.version());
var versions = imagemagick.version().split(".");

function saveToFileIfDebug (buffer, file) {
    if (debug) {
        require('fs').writeFileSync( file, buffer, 'binary' );
        console.log( "wrote file: "+file );
    }
}

test( 'trim default', function (t) {
    var buffer = imagemagick.convert({
        srcData: require('fs').readFileSync( "./test/test.trim.jpg" ), // 87x106
        format: 'PNG',
        trim: true,
        debug: debug
    });
    t.equal( Buffer.isBuffer(buffer), true, 'buffer is Buffer' );
    var info = imagemagick.identify({srcData: buffer });
    t.equal( info.width, 61 );
    t.equal( info.height, 72 );
    saveToFileIfDebug( buffer, "out.trim-default.png" );
    t.end();
});

test( 'trim exact color fuzz', function (t) {
    var buffer = imagemagick.convert({
        srcData: require('fs').readFileSync( "./test/test.trim.jpg" ), // 87x106
        format: 'PNG',
        trim: true,
        trimFuzz: 0,
        debug: debug
    });
    t.equal( Buffer.isBuffer(buffer), true, 'buffer is Buffer' );
    var info = imagemagick.identify({srcData: buffer });
    t.equal( info.width, 61 );
    t.equal( info.height, 72 );
    saveToFileIfDebug( buffer, "out.trim-exact.png" );
    t.end();
});

test( 'trim half color fuzz', function (t) {
    var buffer = imagemagick.convert({
        srcData: require('fs').readFileSync( "./test/test.trim.jpg" ), // 87x106
        format: 'PNG',
        trim: true,
        trimFuzz: 0.5,
        debug: debug
    });
    t.equal( Buffer.isBuffer(buffer), true, 'buffer is Buffer' );
    var info = imagemagick.identify({srcData: buffer });
    t.equal( info.width, 57 );
    t.equal( info.height, 65 );
    saveToFileIfDebug( buffer, "out.trim-half.png" );
    t.end();
});

test( 'trim 92% color fuzz', function (t) {
    var buffer = imagemagick.convert({
        srcData: require('fs').readFileSync( "./test/test.trim.jpg" ), // 87x106
        format: 'PNG',
        trim: true,
        trimFuzz: 0.92,
        debug: debug
    });
    t.equal( Buffer.isBuffer(buffer), true, 'buffer is Buffer' );
    var info = imagemagick.identify({srcData: buffer });
    t.equal( info.width, 1 );
    t.equal( info.height, 1 );
    saveToFileIfDebug( buffer, "out.trim-92.png" );
    t.end();
});

test( 'trim half color fuzz resize', function (t) {
    var buffer = imagemagick.convert({
        srcData: require('fs').readFileSync( "./test/test.trim.jpg" ), // 87x106
        format: 'PNG',
        trim: true,
        trimFuzz: 0.5,
        width: 50,
        height: 50,
        resizeStyle: 'aspectfill',
        debug: debug
    });
    t.equal( Buffer.isBuffer(buffer), true, 'buffer is Buffer' );
    var info = imagemagick.identify({srcData: buffer });
    t.equal( info.width, 50 );
    t.equal( info.height, 50 );
    saveToFileIfDebug( buffer, "out.trim-half-resize.png" );
    t.end();
});

