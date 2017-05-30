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

test( 'Allow crop resizeStyle without [x|y]offset', function (t) {
    var buffer = imagemagick.convert({
        srcData: require('fs').readFileSync( 'test.crop.png' ), // 30x30
        width: 3,
        format: 'PNG',
        resizeStyle: 'crop',
        debug: debug
    });
    t.equal( Buffer.isBuffer(buffer), true, 'buffer is Buffer' );
    saveToFileIfDebug( buffer, 'out.crop1.png' );

    var info = imagemagick.identify({ srcData: buffer, debug: debug });
    t.equal( info.width, 3);
    t.equal( info.height, 30);
    var quantize = imagemagick.quantizeColors({ srcData: buffer, colors: 1, debug: debug });
    t.equal( quantize[0].hex, 'ff0000');
    t.end();
});

test( 'Allow crop resizeStyle with [x|y]offset', function (t) {
    var buffer = imagemagick.convert({
        srcData: require('fs').readFileSync( 'test.crop.png' ), // 30x30
        width: 5,
        height: 5,
        xoffset: 4,
        yoffset: 2,
        format: 'PNG',
        resizeStyle: 'crop',
        debug: debug
    });
    t.equal( Buffer.isBuffer(buffer), true, 'buffer is Buffer' );
    saveToFileIfDebug( buffer, 'out.crop2.png' );

    var info = imagemagick.identify({ srcData: buffer, debug: debug });
    t.equal( info.width, 5);
    t.equal( info.height, 5);
    var quantize = imagemagick.quantizeColors({ srcData: buffer, colors: 1, debug: debug });
    t.equal( quantize[0].hex, '0000ff');
    t.end();
});
