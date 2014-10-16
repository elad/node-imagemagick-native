# node-imagemagick-native

[Imagemagick](http://www.imagemagick.org/)'s [Magick++](http://www.imagemagick.org/Magick++/) binding for [Node](http://nodejs.org/).

[![Build Status](https://travis-ci.org/mash/node-imagemagick-native.png)](https://travis-ci.org/mash/node-imagemagick-native)

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
        format: 'JPEG',
        filter: 'Lagrange',
        blur: 0.8,
        strip: true
    });

    require('fs').writeFileSync('./test/out.png', resizedBuffer, 'binary');

## API

### convert( options )

Convert a buffer provided as `options.srcData` and return a Buffer.

The `options` argument can have following values:

    {
        srcData:        required. Buffer with binary image data
        quality:        optional. 0-100 integer, default 75. JPEG/MIFF/PNG compression level.
        width:          optional. px.
        height:         optional. px.
        density         optional. Integer dpi value to convert
        resizeStyle:    optional. default: "aspectfill". can be "aspectfit", "fill"
                        aspectfill: keep aspect ratio, get the exact provided size,
                                    crop top/bottom or left/right if necessary
                        aspectfit:  keep aspect ratio, get maximum image that fits inside provided size
                        fill:       forget aspect ratio, get the exact provided size
        format:         optional. one of http://www.imagemagick.org/script/formats.php ex: "JPEG"
        filter:         optional. ex: "Lagrange", "Lanczos". see ImageMagick's magick/option.c for candidates
        blur:           optional. ex: 0.8
        strip:          optional. default: false. strips comments out from image.
        rotate:         optional. degrees.
        debug:          optional. 1 or 0
        ignoreWarnings: optional. 1 or 0
    }

### identify( options )

Identify a buffer provided as `srcData` and return an object.

The `options` argument can have following values:

    {
        srcData:        required. Buffer with binary image data
        debug:          optional. 1 or 0
        ignoreWarnings: optional. 1 or 0
    }

The method returns an object similar to:

    {
        format: 'JPEG',
        width: 3904,
        height: 2622,
        depth: 8,
        density : {
            width : 300,
            height : 300
        },
        exif: {
            orientation: 0 # 0 if none exists
        }
    }

### quantizeColors( options )

Quantize the image to a specified amount of colors from a buffer provided as `srcData` and return an array.

The `options` argument can have following values:

    {
        srcData:        required. Buffer with binary image data
        colors:         required. number of colors to extract, defaults to 5
        debug:          optional. 1 or 0
        ignoreWarnings: optional. 1 or 0
    }

The method returns an array similar to:

    [
        {
            r: 83,
            g: 56,
            b: 35,
            hex: '533823'
        },
        {
            r: 149,
            g: 110,
            b: 73,
            hex: '956e49'
        },
        {
            r: 165,
            g: 141,
            b: 111,
            hex: 'a58d6f
        }
    ]

### composite( options )

Composite a buffer provided as `options.compositeData` on a buffer provided as `options.srcData` with gravity specified by `options.gravity` and return a Buffer.

The `options` argument can have following values:

    {
        srcData:        required. Buffer with binary image data
        compositeData:  required. Buffer with binary image data
        gravity:        optional. Can be one of "CenterGravity" "EastGravity" "ForgetGravity" "NorthEastGravity" "NorthGravity" "NorthWestGravity" "SouthEastGravity" "SouthGravity" "SouthWestGravity" "WestGravity"
        debug:          optional. 1 or 0
        ignoreWarnings: optional. 1 or 0
    }

This library currently provide only these, please try [node-imagemagick](https://github.com/rsms/node-imagemagick/) if you want more.

### getConstPixels( options )

Get pixels of provided rectangular region.

The `options` argument can have following values:

    {
        srcData:        required. Buffer with binary image data
        x:              required. x,y,columns,rows provide the area of interest.
        y:              required.
        columns:        required.
        rows:           required.
    }

Example usage:

    // retrieve first pixel of image
    var pixels = imagemagick.getConstPixels({
        srcData: imageBuffer, // white image
        x: 0,
        y: 0,
        columns: 1,
        rows: 1
    });

Returns:

    [ { red: 65535, green: 65535, blue: 65535, opacity: 65535 } ]

Where each color value's size is `imagemagick.quantumDepth` bits.

### version

Return ImageMagick's version as string.  
ex: "6.7.7"

### quantumDepth

Return ImageMagick's QuantumDepth, which is defined in compile time.  
ex: 16

## Installation

### Linux / Mac

Install [Imagemagick](http://www.imagemagick.org/) with headers before installing this module.
Tested with ImageMagick 6.7.7 on CentOS6 and MacOS10.7, Ubuntu12.04 .

    brew install imagemagick

      or

    sudo yum install ImageMagick-c++ ImageMagick-c++-devel

      or

    sudo apt-get install libmagick++-dev

Make sure you can find Magick++-config in your PATH.
Then:

    npm install imagemagick-native

### Windows

Tested on Windows 7 x64.

1. Install Python 2.7.3 only 2.7.3 nothing else works quite right!

    If you use Cygwin ensure you don't have Python installed in Cygwin setup as there will be some confusion about what version to use.

2. Install [Visual Studio C++ 2010 Express](http://www.microsoft.com/en-us/download/details.aspx?id=8279)

3. (64-bit only) [Install Windows 7 64-bit SDK](http://www.microsoft.com/en-us/download/details.aspx?id=8279)

4. Install [Imagemagick dll binary x86 Q16](http://www.imagemagick.org/download/binaries/ImageMagick-6.8.5-10-Q16-x86-dll.exe) or [Imagemagick dll binary x64 Q16](http://www.imagemagick.org/download/binaries/ImageMagick-6.8.5-10-Q16-x64-dll.exe), check for libraries and includes during install.

Then:

    npm install imagemagick-native

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
