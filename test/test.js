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

test( 'convert invalid format', function (t) {
    var buffer;
    try {
        buffer = imagemagick.convert({
            srcData: require('fs').readFileSync( "./test/test.png" ), // 58x66
            width: 100,
            height: 100,
            quality: 80,
            format: 'PNGX',
            debug: debug
        });
    } catch (e) {
        t.like( e.message, /no decode delegate for this image format/, 'err message' );
    }
    t.equal( buffer, undefined, 'buffer undefined' );
    t.end();
});

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

test( 'convert filter not supported', function (t) {
    var buffer;
    try {
        imagemagick.convert({
            srcData: require('fs').readFileSync( "./test/test.png" ),
            width: 100,
            height: 100,
            quality: 80,
            format: 'PNG',
            filter: 'Lagrang', // typo
            debug: debug
        });
    } catch (e) {
        t.equal( e.message, "filter not supported" );
    }
    t.equal( buffer, undefined, 'buffer undefined' );
    t.end();
});

test( 'convert filter Lagrange', function (t) {
    var buffer = imagemagick.convert({
        srcData: require('fs').readFileSync( "./test/test.png" ), // 58x66
        width: 100,
        height: 100,
        filter: 'Lagrange',
        quality: 80,
        format: 'PNG',
        debug: debug
    });
    t.equal( Buffer.isBuffer(buffer), true, 'buffer is Buffer' );
    saveToFileIfDebug( buffer, "out.png-lagrange.png" );
    t.end();
});

test( 'convert blur', function (t) {
    var buffer = imagemagick.convert({
        srcData: require('fs').readFileSync( "./test/test.png" ), // 58x66
        width: 100,
        height: 100,
        quality: 80,
        format: 'PNG',
        blur: 0.8,
        debug: debug
    });
    t.equal( Buffer.isBuffer(buffer), true, 'buffer is Buffer' );
    saveToFileIfDebug( buffer, "out.png-blur.png" );
    t.end();
});

test( 'convert strip', function (t) {
    var buffer = imagemagick.convert({
        srcData: require('fs').readFileSync( "./test/test.png" ),
        width: 100,
        height: 100,
        quality: 80,
        format: 'PNG',
        strip: true,
        debug: debug
    });
    t.equal( Buffer.isBuffer(buffer), true, 'buffer is Buffer' );
    saveToFileIfDebug( buffer, "out.png-strip.png" );
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
    saveToFileIfDebug( buffer, "out.png-aspectfill.png" );
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
    saveToFileIfDebug( buffer, "out.png-aspectfill.jpg" );
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
    saveToFileIfDebug( buffer, "out.wide.png-aspectfill.png" );
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
    saveToFileIfDebug( buffer, "out.jpg-fill.jpg" );
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
    saveToFileIfDebug( buffer, "out.jpg-aspectfit.jpg" );
    t.end();
});

test( 'convert png -> png aspectwithbg', function (t) {
    var buffer = imagemagick.convert({
        srcData: require('fs').readFileSync( "./test/test.png" ), // 58x66
        width: 100,
        height: 100,
        resizeStyle: 'aspectwithbg',
        background: '#4d4d4d',
        quality: 80,
        format: 'PNG',
        debug: debug
    });
    t.equal( Buffer.isBuffer(buffer), true, 'buffer is Buffer' );
    var info = imagemagick.identify({srcData: buffer });
    t.equal( info.width, 100 );
    t.equal( info.height, 100 );
    saveToFileIfDebug( buffer, "out.png-aspectwithbg.png" );
    t.end();
});

test( 'convert png.wide -> png.wide aspectwithbg', function (t) {
    var buffer = imagemagick.convert({
        srcData: require('fs').readFileSync( "./test/test.wide.png" ), // 66x58
        width: 100,
        height: 100,
        resizeStyle: 'aspectwithbg',
        background: '#4d4d4d',
        quality: 80,
        format: 'PNG',
        debug: debug
    });
    t.equal( Buffer.isBuffer(buffer), true, 'buffer is Buffer' );
    var info = imagemagick.identify({srcData: buffer });
    t.equal( info.width, 100 );
    t.equal( info.height, 100 );
    saveToFileIfDebug( buffer, "out.wide.png-aspectwithbg.png" );
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
                   new RegExp("CRC error|image\\.read failed with error: Magick:|corrupt image") );
    }
    t.end();
});

// segmentation faults on Mac...
// if (versions[1] > 6) {
//     test( 'convert too wide jpg', function (t) {
//         var srcData = require('fs').readFileSync( "test.maxmemory.jpg" )
//         , buffer
//         , seenError = 0;

//         try {
//             buffer = imagemagick.convert({
//                 srcData: srcData,
//                 width: 640,
//                 height: 960,
//                 resizeStyle: "aspectfill",
//                 quality: 80,
//                 format: 'JPEG',
//                 maxMemory: 100 * 1000, // 100kB
//                 debug: debug
//             });
//         } catch (e) {
//             seenError = 1;
//             t.similar( e.message,
//                        new RegExp("cache resources exhausted") );
//         }
//         saveToFileIfDebug( buffer, "out.jpg-maxmemory.jpg" );
//         t.equal( seenError, 1 );
//         t.end();
//     });
// }

test( 'convert to rotate 90 degrees', function (t) {
  var buffer = imagemagick.convert({
    srcData: require('fs').readFileSync( "./test/test.jpg" ),
    rotate: 90,
    debug: debug
  });
  var info = imagemagick.identify({srcData: buffer });
  t.equal( info.width, 66 );
  t.equal( info.height, 58 );
  saveToFileIfDebug( buffer, "out.jpg-rotate90.jpg" );
  t.end();
});

test( 'convert to rotate -90 degrees', function (t) {
  var buffer = imagemagick.convert({
    srcData: require('fs').readFileSync( "./test/test.jpg" ),
    rotate: -90,
    debug: debug
  });
  var info = imagemagick.identify({srcData: buffer });
  t.equal( info.width, 66 );
  t.equal( info.height, 58 );
  saveToFileIfDebug( buffer, "out.jpg-rotate-90.jpg" );
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
    var info = imagemagick.identify({
        srcData: require('fs').readFileSync( "./test/test.png" )
    });
    t.equal( info.width, 58, 'width is 58' );
    t.equal( info.height, 66, 'height is 66' );
    t.equal( info.depth, 8, 'depth is 8' );
    t.equal( info.format, 'PNG', 'format is PNG' );
    t.equal( info.exif.orientation, 0, 'orientation doesnt exist' );
    t.end();
});

test( 'quantizeColors invalid number of arguments', function (t) {
    var error = 0;
    try {
        imagemagick.quantizeColors();
    } catch (e) {
        error = e;
    }

    t.equal( error.name, 'Error' );
    t.equal( error.message, 'quantizeColors() requires 1 (option) argument!' );
    t.end();
});

test( 'quantizeColors srcData is a Buffer', function (t) {
    var buffer;
    try {
        buffer = imagemagick.quantizeColors({
            srcData: require('fs').readFileSync( "./test/test.png", 'binary' )
        });
    } catch (e) {
        t.equal( e.message, "quantizeColors()'s 1st argument should have \"srcData\" key with a Buffer instance" );
    }
    t.equal( buffer, undefined, 'buffer undefined' );
    t.end();
});

test( 'quantizeColors results, 1 color', function (t) {
    var results = imagemagick.quantizeColors({
        srcData: require('fs').readFileSync( "./test/test.quantizeColors.png" ),
        colors: 1
    });

    t.equal( results[0].r, 161, 'results[0] red is 161' );
    t.equal( results[0].g, 93, 'results[0] green is 93' );
    t.equal( results[0].b, 85, 'results[0] blue is 85' );
    t.equal( results[0].hex, 'a15d55', 'results[0] hex is a15d55' );

    t.end();
});


test( 'quantizeColors results, 5 colors', function (t) {
    var results = imagemagick.quantizeColors({
        srcData: require('fs').readFileSync( "./test/test.quantizeColors.png" )
    });

    t.equal( results[0].r, 255, 'results[0] red is 255' );
    t.equal( results[0].g, 8, 'results[0] green is 8' );
    t.equal( results[0].b, 8, 'results[0] blue is 8' );
    t.equal( results[0].hex, 'ff0808', 'results[0] hex is ff0808' );

    t.equal( results[1].r, 223, 'results[1] red is 223' );
    t.equal( results[1].g, 0, 'results[1] green is 0' );
    t.equal( results[1].b, 234, 'results[1] blue is 234' );
    t.equal( results[1].hex, 'df00ea', 'results[1] hex is df00ea' );

    t.equal( results[2].r, 28, 'results[2] red is 28' );
    t.equal( results[2].g, 27, 'results[2] green is 27' );
    t.equal( results[2].b, 255, 'results[2] blue is 255' );
    t.equal( results[2].hex, '1c1bff', 'results[2] hex is 1c1bff' );

    t.equal( results[3].r, 0, 'results[3] red is 0' );
    t.equal( results[3].g, 231, 'results[3] green is 231' );
    t.equal( results[3].b, 226, 'results[3] blue is 226' );
    t.equal( results[3].hex, '00e7e2', 'results[3] hex is 00e7e2' );

    t.equal( results[4].r, 25, 'results[4] red is 25' );
    t.equal( results[4].g, 255, 'results[4] green is 255' );
    t.equal( results[4].b, 30, 'results[4] blue is 30' );
    t.equal( results[4].hex, '19ff1e', 'results[4] hex is 19ff1e' );

    t.end();
});

test( 'composite invalid number of arguments', function (t) {
    var error = 0;
    try {
        imagemagick.composite();
    } catch (e) {
        error = e;
    }

    t.equal( error.name, 'Error' );
    t.equal( error.message, 'composite() requires 1 (option) argument!' );
    t.end();
});

test( 'composite srcData is a Buffer', function (t) {
    var buffer;
    try {
        buffer = imagemagick.composite({
            srcData: require('fs').readFileSync( "./test/test.png", 'binary' )
        });
    } catch (e) {
        t.equal( e.message, "composite()'s 1st argument should have \"srcData\" key with a Buffer instance" );
    }
    t.equal( buffer, undefined, 'buffer undefined' );
    t.end();
});

test( 'composite compositeData is a Buffer', function (t) {
    var buffer;
    try {
        buffer = imagemagick.composite({
            srcData: require('fs').readFileSync( "./test/test.quantizeColors.png" ),
            compositeData: require('fs').readFileSync("./test/test.png","binary")
        });
    } catch (e) {
        t.equal( e.message, "composite()'s 1st argument should have \"compositeData\" key with a Buffer instance" );
    }
    t.equal( buffer, undefined, 'buffer undefined' );
    t.end();
});

test( 'composite image not source image',function(t) {

	var srcData = require('fs').readFileSync( "./test/test.quantizeColors.png" );
	var compositeData = require('fs').readFileSync( "./test/test.png" );

    var buffer = imagemagick.composite({
            srcData: srcData,
            compositeData: compositeData,
            gravity: "SouthEastGravity"
        });

    t.notSame( buffer,srcData );
    t.end();
});

test( 'get pixel colors: pixel colors from each 6x6 square', function(t) {
    var srcData = require('fs').readFileSync("./test/test.getPixelColor.png");

    var targetInfo = {
        srcData : srcData,
        x       : 0, // (6 * i),
        y       : 0,
        columns : 1,
        rows    : 1
    };
    var pixelInfo = imagemagick.getConstPixels(targetInfo);
    t.equal(pixelInfo.length, 1);
    // don't test exact pixel value, it may differ according to quantum depth
    t.equal(typeof(pixelInfo[0].red), "number");
    t.equal(typeof(pixelInfo[0].green), "number");
    t.equal(typeof(pixelInfo[0].blue), "number");
    t.equal(typeof(pixelInfo[0].opacity), "number");
    if (debug) {
        console.log(pixelInfo);
    }
    t.end();
});

test( 'quantumDepth', function(t) {
    var q = imagemagick.quantumDepth();
    t.equal(typeof(q), "number");
    t.equal(q >= 8, true);
    t.end();
});
