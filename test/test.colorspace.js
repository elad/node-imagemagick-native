/* jshint node: true */
var fs          = require('fs');
var test        = require('tap').test;
var imagemagick = require('..');
var debug       = false;

process.chdir(__dirname);

console.log('image magick\'s version is: ' + imagemagick.version());

function saveToFileIfDebug (buffer, file) {
    if (debug) {
        fs.writeFileSync( file, buffer, 'binary' );
        console.log( 'wrote file: '+file );
    }
}

test( 'Get colorspace from identify', function (t) {
    var info = imagemagick.identify({
        srcData: require('fs').readFileSync( 'test.CMYK.jpg' ),
        debug: debug
    });

    t.equal( info.colorspace, 'CMYK', 'colorspace is CMYK' );
    t.end();
});

test( 'convert CMYK JPEG -> sRGB PNG', function (t) {
    t.plan(2);

    var buffer = imagemagick.convert({
        srcData: require('fs').readFileSync( 'test.CMYK.jpg' ), // 58x66
        format: 'PNG',
        colorspace: 'sRGB',
        debug: debug
    });
    t.equal( Buffer.isBuffer(buffer), true, 'buffer is Buffer' );
    saveToFileIfDebug( buffer, 'out.CMYK.to.sRGB.png' );

    var info = imagemagick.identify({ srcData: buffer, debug: debug });
    t.equal( info.colorspace, 'sRGB', 'colorspace is sRGB' );
    t.end();
});
