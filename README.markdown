# node-imagemagick-native

[Imagemagick](http://www.imagemagick.org/)'s [Magick++](http://www.imagemagick.org/Magick++/) binding for [Node](http://nodejs.org/).

## Example

    var imagemagick = require('imagemagick-native')
    ,   srcData     = require('fs').readFileSync('./test/test.png');

    // returns a Buffer instance
    var resizedBuffer = imagemagick.convert({
        srcData: srcData, // provide a Buffer instance
        width: 100,
        height: 100,
        resizeStyle: "aspectfill",
        quality: 80,
        format: 'JPEG'
    });

    require('fs').writeFileSync('./test/out.png', resizedBuffer, 'binary');

## API

### convert( options )

Convert a buffer provided as `options.srcData` and return a Buffer.

The `options` argument can have following values:

    {
        srcData:     required. Buffer with binary image data
        quality:     optional. 0-100 integer, default 75. JPEG/MIFF/PNG compression level.
        width:       optional. px.
        height:      optional. px.
        resizeStyle: optional. default: "aspectfill". can be "aspectfit", "fill"
                     aspectfill: keep aspect ratio, get the exact provided size,
                                 crop top/bottom or left/right if necessary
                     aspectfit:  keep aspect ratio, get maximum image that fits inside provided size
                     fill:       forget aspect ratio, get the exact provided size
        format:      optional. one of http://www.imagemagick.org/script/formats.php ex: "JPEG"
        debug:       optional. 1 or 0
    }

This library currently provide only this, please try [node-imagemagick](https://github.com/rsms/node-imagemagick/) if you want more.

## Installation

Install [Imagemagick](http://www.imagemagick.org/) with headers before installing this module.
Tested with ImageMagick 6.7.7 on CentOS6 and MacOS10.7 .

    brew install imagemagick
    sudo yum install ImageMagick-c++ ImageMagick-c++-devel

## Performance - simple thumbnail creation

    imagemagick:       16.09ms per iteration
    imagemagick-native: 0.89ms per iteration

See `node test/benchmark.js` for details.


## License (MIT)

Copyright (c) Masakazu Ohtsuka <http://maaash.jp/>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
