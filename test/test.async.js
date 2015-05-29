
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

test( 'convert filter Lagrange', function (t) {
    t.plan(1);
    imagemagick.convert({
        srcData: require('fs').readFileSync( "./test/test.png" ), // 58x66
        width: 100,
        height: 100,
        filter: 'Lagrange',
        quality: 80,
        format: 'PNG',
        debug: debug
    },function(err,buffer){
        t.equal( Buffer.isBuffer(buffer), true, 'buffer is Buffer' );
        saveToFileIfDebug( buffer, "out.png-lagrange.png" );
        t.end();
    });
});

test( 'identify results async', function (t) {
    t.plan(5);
    imagemagick.identify({
        srcData: require('fs').readFileSync( "./test/test.png" )
    },function(err,info){
        t.equal( info.width, 58, 'width is 58' );
        t.equal( info.height, 66, 'height is 66' );
        t.equal( info.depth, 8, 'depth is 8' );
        t.equal( info.format, 'PNG', 'format is PNG' );
        t.equal( info.exif.orientation, 0, 'orientation doesnt exist' );
        t.end();
    });
});

test( 'composite async', function (t) {
    t.plan(1);
    imagemagick.composite({
        srcData: require('fs').readFileSync( "./test/test.quantizeColors.png" ),
        compositeData: require('fs').readFileSync("./test/test.png"),
        debug: debug
    },function(err,buffer){
        t.equal( Buffer.isBuffer(buffer), true, 'buffer is Buffer' );
        saveToFileIfDebug( buffer, "out.composite-async.png" );
        t.end();
    });
});
