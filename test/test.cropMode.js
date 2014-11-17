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

test( 'taller default', function (t) {
    var buffer = imagemagick.convert({
        srcData: require('fs').readFileSync( "test.png" ), // 58x66
        width: 100,
        height: 100,
        resizeStyle: 'aspectfill',
        quality: 80,
        format: 'PNG',
        debug: debug
    });
    t.equal( Buffer.isBuffer(buffer), true, 'buffer is Buffer' );
    saveToFileIfDebug( buffer, "out.cropMode-default.png" );
    t.end();
});

test( 'taller none', function (t) {
    var buffer = imagemagick.convert({
        srcData: require('fs').readFileSync( "test.png" ), // 58x66
        width: 100,
        height: 100,
        resizeStyle: 'aspectfill',
        cropMode: 'none',
        quality: 80,
        format: 'PNG',
        debug: debug
    });
    t.equal( Buffer.isBuffer(buffer), true, 'buffer is Buffer' );
    saveToFileIfDebug( buffer, "out.cropMode-none.png" );
    t.end();
});

['top', 'middle', 'bottom'].forEach( function (vertical) {
    ['left', 'center', 'right'].forEach( function( horizontal ) {
        var cropMode = vertical + '-' + horizontal;
        test( 'taller ' + cropMode, function (t) {
            var buffer = imagemagick.convert({
                srcData: require('fs').readFileSync( "test.png" ), // 58x66
                width: 100,
                height: 100,
                resizeStyle: 'aspectfill',
                cropMode: cropMode,
                quality: 80,
                format: 'PNG',
                debug: debug
            });
            t.equal( Buffer.isBuffer(buffer), true, 'buffer is Buffer' );
            saveToFileIfDebug( buffer, "out.cropMode-"+cropMode+".png" );
            t.end();
        });

        test( 'wide ' + cropMode, function (t) {
            var buffer = imagemagick.convert({
                srcData: require('fs').readFileSync( "test.wide.png" ), // 58x66
                width: 100,
                height: 100,
                resizeStyle: 'aspectfill',
                cropMode: cropMode,
                quality: 80,
                format: 'PNG',
                debug: debug
            });
            t.equal( Buffer.isBuffer(buffer), true, 'buffer is Buffer' );
            saveToFileIfDebug( buffer, "out.wide.cropMode-"+cropMode+".png" );
            t.end();
        });
    } );
} );


