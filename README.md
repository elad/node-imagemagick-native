# node-imagemagick-native

[ImageMagick](http://www.imagemagick.org/)'s [Magick++](http://www.imagemagick.org/Magick++/) binding for [Node](http://nodejs.org/).

Features

  * Native bindings to the C/C++ Magick++ library
  * Async, sync, and stream API
  * Support for `convert`, `identify`, `composite`, and other utility functions

[![Build Status](https://travis-ci.org/elad/node-imagemagick-native.png)](https://travis-ci.org/elad/node-imagemagick-native)

Table of contents

  * [Examples](#examples)
    * [Convert formats](#example-convert) (PNG to JPEG)
    * [Blur](#example-blur)
    * [Resize](#example-resize)
    * [Rotate, flip, and mirror](#example-rotate-flip-mirror)
  * [API Reference](#api)
    * [`convert`](#convert)
    * [`identify`](#identify)
    * [`quantizeColors`](#quantizeColors)
    * [`composite`](#composite)
    * [`getConstPixels`](#getConstPixels)
    * [`quantumDepth`](#quantumDepth)
    * [`version`](#version)
  * [Installation](#installation)
    * [Linux / Mac OS X](#installation-unix)
    * [Windows](#installation-windows)
  * [Performance](#performance)
  * [Contributing](#contributing)
  * [License](#license)

<a name='examples'></a>

## Examples

<a name='example-convert'></a>

### Convert formats

Convert from one format to another with quality control:

```js
fs.writeFileSync('after.png', imagemagick.convert({
	srcData: fs.readFileSync('before.jpg'),
	format: 'PNG',
	quality: 100 // (best) to 1 (worst)
}));
```

Original JPEG:

![alt text](http://elad.github.io/node-imagemagick-native/examples/quality.jpg 'Original')

Converted to PNG:

quality 100 | quality 50 | quality 1
:---: | :---: | :---:
![alt text](http://elad.github.io/node-imagemagick-native/examples/quality_100.png 'quality 100') | ![alt text](http://elad.github.io/node-imagemagick-native/examples/quality_50.png 'quality 50') | ![alt text](http://elad.github.io/node-imagemagick-native/examples/quality_1.png 'quality 1')

*Image courtesy of [David Yu](https://www.flickr.com/photos/davidyuweb/14175248591).*

<a name='example-blur'></a>

### Blur

Blur image:

```js
fs.writeFileSync('after.jpg', imagemagick.convert({
	srcData: fs.readFileSync('before.jpg'),
	blur: 5
}));
```

![alt text](http://elad.github.io/node-imagemagick-native/examples/blur_before.jpg 'Before blur') becomes ![alt text](http://elad.github.io/node-imagemagick-native/examples/blur_after.jpg 'After blur')

*Image courtesy of [Tambako The Jaguar](https://www.flickr.com/photos/tambako/3574360498).*

<a name='example-resize'></a>

### Resize

Resized images by specifying `width` and `height`. There are three resizing styles:

  * `aspectfill`: Default. The resulting image will be exactly the specified size, and may be cropped.
  * `aspectfit`: Scales the image so that it will not have to be cropped.
  * `fill`: Squishes or stretches the image so that it fills exactly the specified size.

```js
fs.writeFileSync('after_resize.jpg', imagemagick.convert({
	srcData: fs.readFileSync('before_resize.jpg'),
	width: 100,
	height: 100,
	resizeStyle: 'aspectfill', // is the default, or 'aspectfit' or 'fill'
	gravity: 'Center' // optional: position crop area when using 'aspectfill'
}));
```

Original:
  
![alt text](http://elad.github.io/node-imagemagick-native/examples/resize.jpg 'Original')

Resized:

aspectfill | aspectfit | fill
:---: | :---: | :---:
![alt text](http://elad.github.io/node-imagemagick-native/examples/resize_aspectfill.jpg 'aspectfill') | ![alt text](http://elad.github.io/node-imagemagick-native/examples/resize_aspectfit.jpg 'aspectfit') | ![alt text](http://elad.github.io/node-imagemagick-native/examples/resize_fill.jpg 'fill')

*Image courtesy of [Christoph](https://www.flickr.com/photos/scheinwelten/381994831).*

<a name='example-rotate-flip-mirror'></a>

### Rotate, flip, and mirror

Rotate and flip images, and combine the two to mirror:

```js
fs.writeFileSync('after_rotateflip.jpg', imagemagick.convert({
	srcData: fs.readFileSync('before_rotateflip.jpg'),
	rotate: 180,
	flip: true
}));
```

Original:

![alt text](http://elad.github.io/node-imagemagick-native/examples/rotateflip.jpg 'Original')

Modified:

rotate 90 degrees | rotate 180 degrees | flip | flip + rotate 180 degrees = mirror
:---: | :---: | :---: | :---:
![alt text](http://elad.github.io/node-imagemagick-native/examples/rotateflip_rotate_90.jpg 'rotate 90') | ![alt text](http://elad.github.io/node-imagemagick-native/examples/rotateflip_rotate_180.jpg 'rotate 180') | ![alt text](http://elad.github.io/node-imagemagick-native/examples/rotateflip_flip.jpg 'flip') | ![alt text](http://elad.github.io/node-imagemagick-native/examples/rotateflip_mirror.jpg 'flip + rotate 180 = mirror')

*Image courtesy of [Bill Gracey](https://www.flickr.com/photos/9422878@N08/6482704235).*

<a name='api'></a>

## API Reference

<a name='convert'></a>

### convert(options, [callback])

Convert a buffer provided as `options.srcData` and return a Buffer.

The `options` argument can have following values:

    {
        srcData:        required. Buffer with binary image data
        srcFormat:      optional. force source format if not detected (e.g. 'ICO'), one of http://www.imagemagick.org/script/formats.php
        quality:        optional. 1-100 integer, default 75. JPEG/MIFF/PNG compression level.
        trim:           optional. default: false. trims edges that are the background color.
        trimFuzz:       optional. [0-1) float, default 0. trimmed color distance to edge color, 0 is exact.
        width:          optional. px.
        height:         optional. px.
        density         optional. Integer dpi value to convert
        resizeStyle:    optional. default: 'aspectfill'. can be 'aspectfit', 'fill'
                        aspectfill: keep aspect ratio, get the exact provided size.
                        aspectfit:  keep aspect ratio, get maximum image that fits inside provided size
                        fill:       forget aspect ratio, get the exact provided size
        gravity:        optional. default: 'Center'. used to position the crop area when resizeStyle is 'aspectfill'
                                  can be 'NorthWest', 'North', 'NorthEast', 'West',
                                  'Center', 'East', 'SouthWest', 'South', 'SouthEast', 'None'
        format:         optional. output format, ex: 'JPEG'. see below for candidates
        filter:         optional. resize filter. ex: 'Lagrange', 'Lanczos'.  see below for candidates
        blur:           optional. ex: 0.8
        strip:          optional. default: false. strips comments out from image.
        rotate:         optional. degrees.
        flip:           optional. vertical flip, true or false.
        debug:          optional. true or false
        ignoreWarnings: optional. true or false
    }

Notes

  * `format` values can be found [here](http://www.imagemagick.org/script/formats.php)
  * `filter` values can be found [here](http://www.imagemagick.org/script/command-line-options.php?ImageMagick=9qgp8o06f469m3cna9lfigirc5#filter)

An optional `callback` argument can be provided, in which case `convert` will run asynchronously. When it is done, `callback` will be called with the error and the result buffer:

```js
imagemagick.convert({
    // options
}, function (err, buffer) {
    // check err, use buffer
});
```

There is also a stream version:

```js
fs.createReadStream('input.png').pipe(imagemagick.streams.convert({
    // options
})).pipe(fs.createWriteStream('output.png'));
```

<a name='identify'></a>

### identify(options, [callback])

Identify a buffer provided as `srcData` and return an object.

The `options` argument can have following values:

    {
        srcData:        required. Buffer with binary image data
        debug:          optional. true or false
        ignoreWarnings: optional. true or false
    }

An optional `callback` argument can be provided, in which case `identify` will run asynchronously. When it is done, `callback` will be called with the error and the result object:

```js
imagemagick.identify({
    // options
}, function (err, result) {
    // check err, use result
});
```

The method returns an object similar to:

```js
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
        orientation: 0 // if none exists or e.g. 3 (portrait iPad pictures)
    }
}
```

<a name='quantizeColors'></a>

### quantizeColors(options)

Quantize the image to a specified amount of colors from a buffer provided as `srcData` and return an array.

The `options` argument can have following values:

    {
        srcData:        required. Buffer with binary image data
        colors:         required. number of colors to extract, defaults to 5
        debug:          optional. true or false
        ignoreWarnings: optional. true or false
    }

The method returns an array similar to:

```js
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
        hex: 'a58d6f'
    }
]
```

<a name='composite'></a>

### composite(options, [callback])

Composite a buffer provided as `options.compositeData` on a buffer provided as `options.srcData` with gravity specified by `options.gravity` and return a Buffer.

The `options` argument can have following values:

    {
        srcData:        required. Buffer with binary image data
        compositeData:  required. Buffer with binary image data
        gravity:        optional. Can be one of 'CenterGravity' 'EastGravity' 'ForgetGravity' 'NorthEastGravity' 'NorthGravity' 'NorthWestGravity' 'SouthEastGravity' 'SouthGravity' 'SouthWestGravity' 'WestGravity'
        debug:          optional. true or false
        ignoreWarnings: optional. true or false
    }

An optional `callback` argument can be provided, in which case `composite` will run asynchronously. When it is done, `callback` will be called with the error and the result buffer:

```js
imagemagick.composite(options, function (err, buffer) {
    // check err, use buffer
});
```

This library currently provide only these, please try [node-imagemagick](https://github.com/rsms/node-imagemagick/) if you want more.

<a name='getConstPixels'></a>

### getConstPixels(options)

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

```js
// retrieve first pixel of image
var pixels = imagemagick.getConstPixels({
    srcData: imageBuffer, // white image
    x: 0,
    y: 0,
    columns: 1,
    rows: 1
});
```

Returns:

```js
[ { red: 65535, green: 65535, blue: 65535, opacity: 65535 } ]
```

Where each color value's size is `imagemagick.quantumDepth` bits.

<a name='quantumDepth'></a>

### quantumDepth

Return ImageMagick's QuantumDepth, which is defined in compile time.  
ex: 16

<a name='version'></a>

### version

Return ImageMagick's version as string.  
ex: '6.7.7'

<a name='installation'></a>

## Installation

<a name='installation-unix'></a>

### Linux / Mac OS X

Install [ImageMagick](http://www.imagemagick.org/) with headers before installing this module.
Tested with ImageMagick 6.7.7 on CentOS 6 and Mac OS X Lion, Ubuntu 12.04 .

    brew install imagemagick

      or

    sudo yum install ImageMagick-c++ ImageMagick-c++-devel

      or

    sudo apt-get install libmagick++-dev

Make sure you can find Magick++-config in your PATH. Packages on some newer distributions, such as Ubuntu 16.04, might be missing a link into `/usr/bin`. If that is the case, do this.

    sudo ln -s `ls /usr/lib/\`uname -p\`-linux-gnu/ImageMagick-*/bin-Q16/Magick++-config | head -n 1` /usr/local/bin/

Then:

    npm install imagemagick-native

**Installation notes**

  * RHEL/CentOS: If the version of ImageMagick required by `node-imagemagick-native` is not available in an official RPM repository, please try the `-last` version offered by Les RPM de Remi, for example:

  ```
  sudo yum remove -y ImageMagick
  sudo yum install -y http://rpms.famillecollet.com/enterprise/remi-release-6.rpm
  sudo yum install -y --enablerepo=remi ImageMagick-last-c++-devel
  ```

  * Mac OS X: You might need to install `pkgconfig` first:

  ```
  brew install pkgconfig
  ```
  
  * Docker: to run `node-gyp` properly is required `build-essential` in Devian distros or `Development Tools` in CentOS


      sudo apt-get install build-essential

       	or

      sudo yum groupinstall "Development Tools"

  

<a name='installation-windows'></a>

### Windows

Tested on Windows 7 x64.

1. Install Python 2.7.3 only 2.7.3 nothing else works quite right!

    If you use Cygwin ensure you don't have Python installed in Cygwin setup as there will be some confusion about what version to use.

2. Install [Visual Studio C++ 2010 Express](http://www.microsoft.com/en-us/download/details.aspx?id=8279)

3. (64-bit only) [Install Windows 7 64-bit SDK](http://www.microsoft.com/en-us/download/details.aspx?id=8279)

4. Install [Imagemagick dll binary x86 Q16](http://www.imagemagick.org/download/binaries/ImageMagick-6.8.5-10-Q16-x86-dll.exe) or [Imagemagick dll binary x64 Q16](http://www.imagemagick.org/download/binaries/ImageMagick-6.8.5-10-Q16-x64-dll.exe), check for libraries and includes during install.

Then:

    npm install imagemagick-native

<a name='performance'></a>

## Performance - simple thumbnail creation

    imagemagick:       16.09ms per iteration
    imagemagick-native: 0.89ms per iteration

See `node test/benchmark.js` for details.

**Note:** `node-imagemagick-native`'s primary advantage is that it uses ImageMagick's API directly rather than by executing one of its command line tools. This means that it will be much faster when the amount of time spent inside the library is small and less so otherwise. See [issue #46](https://github.com/mash/node-imagemagick-native/issues/46) for discussion.

<a name='contributing'></a>

## Contributing

This project follows the ["OPEN Open Source"](https://gist.github.com/substack/e205f5389890a1425233) philosophy. If you submit a pull request and it gets merged you will most likely be given commit access to this repository.

<a name='license'></a>

## License (MIT)

Copyright (c) Masakazu Ohtsuka <http://maaash.jp/>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the 'Software'), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
