
var test        = require('tap').test;
var imagemagick = require('..');
var debug       = 0;

process.chdir(__dirname);

function saveToFileIfDebug (buffer, file) {
  if (debug) {
    require('fs').writeFileSync( file, buffer, 'binary' );
    console.log( 'wrote file: '+file );
  }
}

test( 'promise convert', function (t) {
  t.plan(1);
  imagemagick.promises
    .convert({
      srcData: require('fs').readFileSync( 'test.png' ), // 58x66
      width: 100,
      height: 100,
      filter: 'Lagrange',
      quality: 80,
      format: 'PNG',
      debug: debug
    })
    .then(function(buffer) {
      t.equal( Buffer.isBuffer(buffer), true, 'buffer is Buffer' );
      saveToFileIfDebug( buffer, 'out.png-lagrange.png' );
      t.end();
    });
});

test( 'promise identify', function (t) {
  t.plan(5);
  imagemagick.promises
    .identify({
      srcData: require('fs').readFileSync( 'test.png' )
    })
    .then(function(info){
      t.equal( info.width, 58, 'width is 58' );
      t.equal( info.height, 66, 'height is 66' );
      t.equal( info.depth, 8, 'depth is 8' );
      t.equal( info.format, 'PNG', 'format is PNG' );
      t.equal( info.exif.orientation, 0, 'orientation doesnt exist' );
      t.end();
    });
});

test( 'promise composite', function (t) {
  t.plan(1);
  imagemagick.promises
    .composite({
      srcData: require('fs').readFileSync( 'test.quantizeColors.png' ),
      compositeData: require('fs').readFileSync('test.png'),
      debug: debug
    })
    .then(function(buffer){
      t.equal( Buffer.isBuffer(buffer), true, 'buffer is Buffer' );
      saveToFileIfDebug( buffer, 'out.composite-async.png' );
      t.end();
    });
});
