var test        = require('tap').test;
var imagemagick = require('..');

process.chdir(__dirname);

test( 'identify results', function (t) {
  t.plan(6);
  var info;

  try {
    info = imagemagick.identify({
      srcData: require('fs').readFileSync('test.ext.tga'),
    });
  } catch (ex) {
    t.like(ex.message, /no decode delegate for this image format/, 'err message');
  }

  info = imagemagick.identify({
    srcData: require('fs').readFileSync( 'test.ext.tga' ),
    srcFormat: 'tga',
  });

  t.equal( info.width, 31, 'width is 31' );
  t.equal( info.height, 16, 'height is 16' );
  t.equal( info.depth, 8, 'depth is 8' );
  t.equal( info.format, 'TGA', 'format is TGA' );
  t.equal( info.exif.orientation, 0, 'orientation doesnt exist' );
  t.end();
});
