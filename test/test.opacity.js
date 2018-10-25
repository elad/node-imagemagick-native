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

test( 'convert opacity transparent', function (t) {
    var buffer = imagemagick.convert({
        srcData: require('fs').readFileSync( "test.opacity.png" ), // 58x66
        width: 100,
        height: 100,
        quality: 80,
        format: 'PNG',
        opacity: 40,
        debug: debug
    });
    t.equal( Buffer.isBuffer(buffer), true, 'buffer is Buffer' );
    saveToFileIfDebug( buffer, "out.png-opacity-transparent.png" );
    t.end();
});

test( 'convert opacity color', function (t) {
    var buffer = imagemagick.convert({
        srcData: require('fs').readFileSync( "test.opacity.png" ), // 58x66
        blur: 0,
        rotate: 0,
        brightness: 0,
        contrast: 0,
        opacity: 75,
        opacityColor: '#FF0000',
        debug: true,
        ignoreWarnings: true
    });
    t.equal( Buffer.isBuffer(buffer), true, 'buffer is Buffer' );
    saveToFileIfDebug( buffer, "out.png-opacity-color.png" );
    t.end();
});

