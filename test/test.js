var test        = require('tap').test
,   imagemagick = require('..')
,   debug       = 0
;

function saveToFileIfDebug (buffer, file) {
    if (debug) {
        require('fs').writeFileSync( file, buffer, 'binary' );
        console.log( "wrote file: "+file );
    }
}

test( 'convert invalid number of arguments', function (t) {
    var error = 0;
    try {
        imagemagick.convert();
    } catch (e) {
        error = e;
    }

    t.equal( error.name, 'Error' );
    t.equal( error.message, 'convert() requires 1 (option) argument!' );
    t.end();
});

test( 'convert invalid resizeStyle', function (t) {
    var buffer;
    try {
        buffer = imagemagick.convert({
            srcData: require('fs').readFileSync( "./test/test.png" ), // 58x66
            width: 100,
            height: 100,
            resizeStyle: "fit", // not defined
            quality: 80,
            format: 'PNG',
            debug: debug
        });
    } catch (e) {
        t.equal( e.message, 'resizeStyle not supported', 'err message' );
    }
    t.equal( buffer, undefined, 'buffer undefined' );
    t.end();
});

test( 'convert srcData is a Buffer', function (t) {
    var buffer;
    try {
        imagemagick.convert({
            srcData: require('fs').readFileSync( "./test/test.png", 'binary' ),
            width: 100,
            height: 100,
            resizeStyle: "fit", // not defined
            quality: 80,
            format: 'PNG',
            debug: debug
        });
    } catch (e) {
        t.equal( e.message, "convert()'s 1st argument should have \"srcData\" key with a Buffer instance" );
    }
    t.equal( buffer, undefined, 'buffer undefined' );
    t.end();
});

test( 'convert png -> png aspectfill', function (t) {
    var buffer = imagemagick.convert({
        srcData: require('fs').readFileSync( "./test/test.png" ), // 58x66
        width: 100,
        height: 100,
        resizeStyle: 'aspectfill', // default.
        quality: 80,
        format: 'PNG',
        debug: debug
    });
    t.equal( Buffer.isBuffer(buffer), true, 'buffer is Buffer' );
    // t.equal( buffer.length, 9545, 'converted buffer size ok' );
    saveToFileIfDebug( buffer, "./test/out.png-aspectfill.png" );
    t.end();
});

test( 'convert png -> jpg aspectfill', function (t) {
    var buffer = imagemagick.convert({
        srcData: require('fs').readFileSync( "./test/test.png" ),
        width: 100,
        height: 100,
        quality: 80,
        format: 'JPEG',
        debug: debug
    });
    t.equal( Buffer.isBuffer(buffer), true, 'buffer is Buffer' );
    // t.equal( buffer.length, 2295, 'converted buffer size ok' );
    saveToFileIfDebug( buffer, "./test/out.png-aspectfill.jpg" );
    t.end();
});

test( 'convert png.wide -> png.wide aspectfill', function (t) {
    var buffer = imagemagick.convert({
        srcData: require('fs').readFileSync( "./test/test.wide.png" ),
        width: 100,
        height: 100,
        quality: 80,
        format: 'PNG',
        debug: debug
    });
    t.equal( Buffer.isBuffer(buffer), true, 'buffer is Buffer' );
    // t.equal( buffer.length, 9615, 'converted buffer size ok' );
    saveToFileIfDebug( buffer, "./test/out.wide.png-aspectfill.png" );
    t.end();
});

test( 'convert jpg -> jpg fill', function (t) {
    var buffer = imagemagick.convert({
        srcData: require('fs').readFileSync( "./test/test.jpg" ),
        width: 100,
        height: 100,
        resizeStyle: "fill",
        quality: 80,
        format: 'JPEG',
        debug: debug
    });
    t.equal( Buffer.isBuffer(buffer), true, 'buffer is Buffer' );
    // t.equal( buffer.length, 3184, 'converted buffer size ok' );
    saveToFileIfDebug( buffer, "./test/out.jpg-fill.jpg" );
    t.end();
});

test( 'convert jpg -> jpg aspectfit', function (t) {
    var buffer = imagemagick.convert({
        srcData: require('fs').readFileSync( "./test/test.jpg" ),
        width: 100,
        height: 100,
        resizeStyle: "aspectfit",
        quality: 80,
        format: 'JPEG',
        debug: debug
    });
    t.equal( Buffer.isBuffer(buffer), true, 'buffer is Buffer' );
    // t.equal( buffer.length, 3012, 'converted buffer size ok' );
    saveToFileIfDebug( buffer, "./test/out.jpg-aspectfit.jpg" );
    t.end();
});

test( 'convert broken png', function (t) {
    var srcData = require('fs').readFileSync( "./test/broken.png" )
    , buffer;

    try {
        buffer = imagemagick.convert({
            srcData: srcData,
            width: 100,
            height: 100,
            resizeStyle: "aspectfit",
            quality: 80,
            format: 'JPEG',
            // debug: debug
        });
    } catch (e) {
        t.similar( e.message,
                   new RegExp("CRC error|image\\.read failed with error: Magick:") );
    }
    t.end();
});

test( 'identify invalid number of arguments', function (t) {
    var error = 0;
    try {
        imagemagick.identify();
    } catch (e) {
        error = e;
    }
    
    t.equal( error.name, 'Error' );
    t.equal( error.message, 'identify() requires 1 (option) argument!' );
    t.end();
});

test( 'identify srcData is a Buffer', function (t) {
    var buffer;
    try {
        buffer = imagemagick.identify({
            srcData: require('fs').readFileSync( "./test/test.png", 'binary' )
        });
    } catch (e) {
        t.equal( e.message, "identify()'s 1st argument should have \"srcData\" key with a Buffer instance" );
    }
    t.equal( buffer, undefined, 'buffer undefined' );
    t.end();
});

test( 'identify results', function (t) {
    var results = imagemagick.identify({
        srcData: require('fs').readFileSync( "./test/test.png" )
    });
    t.equal( results.width, 58, 'width is 58' );
    t.equal( results.height, 66, 'height is 66' );
    t.equal( results.depth, 8, 'depth is 8' );
    t.equal( results.format, 'PNG', 'format is PNG' );
    t.end();
});
