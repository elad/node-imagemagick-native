var ben       = require('ben')
,   im        = require('imagemagick')
,   im_native = require('..')
,   async     = require('async')
,   assert    = require('assert')
,   debug     = 0
;

var file  = "./test/test.jpg";

var body  = require('fs').readFileSync( file );

function resize (callback) {
    im.resize({
        width: 48,
        height: 48,
        format: 'jpg',
        quality: 0.8,
        srcData: body
    }, function (err,stdout,stderr) {
        assert( stdout.length > 0 );
        // console.log( "im length: "+stdout.length );
        require('fs').writeFileSync( "./test/out.fork.jpg", stdout, 'binary' );
        callback();
    });
}
function resize_native (callback) {
    var stdout = im_native.convert({
        srcData: body,
        width: 48,
        height: 48,
        resizeStyle: 'aspectfit',
        quality: 80,
        format: 'JPEG',
        debug: debug
    });
    assert( stdout.length > 0 );
    // console.log( "im_native length: "+stdout.length );
    require('fs').writeFileSync( "./test/out.native.jpg", stdout, 'binary' );
    callback();
}

async.waterfall([
    function (callback) {
        console.log( "before resize: ", process.memoryUsage() );
        callback();
    },
    function (callback) {
        ben.async( resize, function (ms) {
            console.log( "resize: " + ms + "ms per iteration" );
            callback();
        });
    },
    function (callback) {
        console.log( "after resize: ", process.memoryUsage() );
        callback();
    },
    function (callback) {
        resize_native( callback );
    },
    function (callback) {
        console.log( "after resize_native 1st: ", process.memoryUsage() );
        callback();
    },
    function (callback) {
        ben.async( resize_native, function (ms) {
            console.log( "resize_native: " + ms + "ms per iteration" );
            callback();
        });
    },
    function (callback) {
        console.log( "after resize_native: ", process.memoryUsage() );
        callback();
    },
], function(err) {
    if ( err ) {
        console.log("err: ", err);
    }
});

/*
resize:       34.58ms per iteration
resize_native: 2.49ms per iteration
*/
