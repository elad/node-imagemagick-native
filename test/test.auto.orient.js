/* jshint node: true */
var fs          = require('fs');
var path        = require('path');
var test        = require('tap').test;
var imagemagick = require('..');
var debug       = false;

console.log('image magick\'s version is: ' + imagemagick.version());

function saveToFileIfDebug (buffer, file) {
    if (debug) {
        fs.writeFileSync( file, buffer, 'binary' );
        console.log( 'wrote file: '+file );
    }
}

// Test suite from https://github.com/recurser/exif-orientation-examples
var testFiles = [
    'Landscape_1.jpg',
    'Landscape_2.jpg',
    'Landscape_3.jpg',
    'Landscape_4.jpg',
    'Landscape_5.jpg',
    'Landscape_6.jpg',
    'Landscape_7.jpg',
    'Landscape_8.jpg',
];

test( 'convert autoOrient option', function (t) {
    t.plan(8);

    testFiles.forEach( function (f) {

        var buffer = imagemagick.convert({
            srcData: fs.readFileSync( path.join(__dirname, 'orientation-suite', f) ),
            format: 'JPEG',
            autoOrient: true,
            debug: debug
        });
        t.equal( Buffer.isBuffer(buffer), true, 'buffer is Buffer' );
        saveToFileIfDebug( buffer, path.join(__dirname, 'out.' + f) );
    });

    t.end();

});
