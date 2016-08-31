var test        = require('tap').test
,   imagemagick = require('..')
,   debug       = false
;

function saveToFileIfDebug (buffer, file) {
    if (debug) {
        require('fs').writeFileSync( file, buffer, 'binary' );
        console.log( "wrote file: "+ file );
    }
}

var colors = [
    {
        name: 'red',
        value: 2
    },
    {
        name: 'green',
        value: 4
    },
    {
        name: 'blue',
        value: 8,
    },
    {
        name: 'white',
        value: 14
    },
    {
        name: 'black',
        value: 0
    }
];

test('background', function (t) {
    colors.forEach(function (color) {
        ['png', 'gif', 'jpg'].forEach(function (format) {
            testBackground(t, color, format);
        });
    });

    t.end();
});

function testBackground (t, color, format) {
    var srcData = require('fs').readFileSync(__dirname + "/test.background.png");

    var pixels = getPixels(srcData);

    t.deepEqual(pixels, [0, 16]);

    var buffer = imagemagick.convert({
        srcData: srcData,
        format: format,
        background: color.name,
        quality: 100,
        debug: debug
    });

    t.equal( Buffer.isBuffer(buffer), true, 'buffer is Buffer' );

    pixels = getPixels(buffer);

    t.equal(pixels[0], 0);
    t.equal(pixels[1], color.value);

    saveToFileIfDebug(
        buffer,
        __dirname + "/out.background-" + color.name + "." + format);
}

function getPixels(buffer) {
    return imagemagick.getConstPixels({
        srcData: buffer,
        x: 0,
        y: 0,
        columns: 2,
        rows: 1
    }).map(function (pixel) {
        return ['red', 'green', 'blue', 'opacity'].reduce(function (num, key, index) {
            if (pixel[key] > 500) {
                num += Math.pow(2, index + 1);
            }
            return num;
        }, 0);
    });
}
