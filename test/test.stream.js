
var test        = require('tap').test
,   imagemagick = require('..')
,   fs          = require('fs')
,   debug       = 0
;

console.log("image magick's version is: " + imagemagick.version());
var versions = imagemagick.version().split(".");

test( 'stream.convert returns a stream like object', function (t) {
    t.plan(2);
    var stream = imagemagick.streams.convert({
        width: 100,
        height: 100,
        filter: 'Lagrange',
        quality: 80,
        format: 'PNG',
        debug: debug
    });

    t.equal( typeof(stream.on), 'function' );
    t.equal( typeof(stream.pipe), 'function' );
    t.end()
});

test( 'stream.convert can pipes streams', function (t) {
    t.plan(2);
    var stream = imagemagick.streams.convert({
        width: 100,
        height: 100,
        filter: 'Lagrange',
        quality: 80,
        format: 'PNG',
        debug: debug
    });

    var input = fs.createReadStream('test.png');
    var output = fs.createWriteStream('test-async-temp.png');

    output.on('close',function(){
        var ret = imagemagick.identify({
            srcData: fs.readFileSync( './test/test-async-temp.png' )
        });
        t.equal( ret.width, 100 );
        t.equal( ret.height, 100 );
        fs.unlinkSync( 'test-async-temp.png' );
        t.end();
    });

    input.pipe(stream).pipe(output);
});
