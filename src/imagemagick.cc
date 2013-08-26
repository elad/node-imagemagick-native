#ifndef BUILDING_NODE_EXTENSION
#define BUILDING_NODE_EXTENSION
#endif  // BUILDING_NODE_EXTENSION

#include "imagemagick.h"
#include <list>
#include <string.h>
#include <exception>

#define THROW_ERROR_EXCEPTION(x) ThrowException(v8::Exception::Error(String::New(x))); \
    scope.Close(Undefined())

// input
//   args[ 0 ]: options. required, object with following key,values
//              {
//                  srcData:     required. Buffer with binary image data
//                  quality:     optional. 0-100 integer, default 75. JPEG/MIFF/PNG compression level.
//                  width:       optional. px.
//                  height:      optional. px.
//                  resizeStyle: optional. default: "aspectfill". can be "aspectfit", "fill"
//                  format:      optional. one of http://www.imagemagick.org/script/formats.php ex: "JPEG"
//                  debug:       optional. 1 or 0
//              }
Handle<Value> Convert(const Arguments& args) {
    HandleScope scope;
    MagickCore::SetMagickResourceLimit(MagickCore::ThreadResource, 1);

    if ( args.Length() != 1 ) {
        return THROW_ERROR_EXCEPTION("convert() requires 1 (option) argument!");
    }
    if ( ! args[ 0 ]->IsObject() ) {
        return THROW_ERROR_EXCEPTION("convert()'s 1st argument should be an object");
    }
    Local<Object> obj = Local<Object>::Cast( args[ 0 ] );

    Local<Object> srcData = Local<Object>::Cast( obj->Get( String::NewSymbol("srcData") ) );
    if ( srcData->IsUndefined() || ! node::Buffer::HasInstance(srcData) ) {
        return THROW_ERROR_EXCEPTION("convert()'s 1st argument should have \"srcData\" key with a Buffer instance");
    }

    int debug = obj->Get( String::NewSymbol("debug") )->Uint32Value();
    if (debug) printf( "debug: on\n" );

    Magick::Blob srcBlob( node::Buffer::Data(srcData), node::Buffer::Length(srcData) );

    Magick::Image image;
    try {
        image.read( srcBlob );
    }
    catch (std::exception& err) {
        std::string message = "image.read failed with error: ";
        message            += err.what();
        return THROW_ERROR_EXCEPTION(message.c_str());
    }
    catch (...) {
        return THROW_ERROR_EXCEPTION("unhandled error");
    }

    if (debug) printf("original width,height: %d, %d\n", (int) image.columns(), (int) image.rows());

    unsigned int width = obj->Get( String::NewSymbol("width") )->Uint32Value();
    if (debug) printf( "width: %d\n", width );

    unsigned int height = obj->Get( String::NewSymbol("height") )->Uint32Value();
    if (debug) printf( "height: %d\n", height );

    Local<Value> resizeStyleValue = obj->Get( String::NewSymbol("resizeStyle") );
    const char* resizeStyle = "aspectfill";
    String::AsciiValue resizeStyleAsciiValue( resizeStyleValue->ToString() );
    if ( ! resizeStyleValue->IsUndefined() ) {
        resizeStyle = *resizeStyleAsciiValue;
    }
    if (debug) printf( "resizeStyle: %s\n", resizeStyle );

    Local<Value> formatValue = obj->Get( String::NewSymbol("format") );
    String::AsciiValue format( formatValue->ToString() );
    if ( ! formatValue->IsUndefined() ) {
        if (debug) printf( "format: %s\n", *format );
        image.magick( *format );
    }

    if ( width || height ) {
        if ( ! width  ) { width  = image.columns(); }
        if ( ! height ) { height = image.rows();    }

        // do resize
        if ( strcmp( resizeStyle, "aspectfill" ) == 0 ) {
            // ^ : Fill Area Flag ('^' flag)
            // is not implemented in Magick++
            // and gravity: center, extent doesnt look like working as exptected
            // so we do it ourselves

            // keep aspect ratio, get the exact provided size, crop top/bottom or left/right if necessary
            double aspectratioExpected = (double)height / (double)width;
            double aspectratioOriginal = (double)image.rows() / (double)image.columns();
            unsigned int xoffset = 0;
            unsigned int yoffset = 0;
            unsigned int resizewidth;
            unsigned int resizeheight;
            if ( aspectratioExpected > aspectratioOriginal ) {
                // expected is taller
                resizewidth  = (unsigned int)( (double)height / (double)image.rows() * (double)image.columns() + 1. );
                resizeheight = height;
                xoffset      = (unsigned int)( (resizewidth - width) / 2. );
                yoffset      = 0;
            }
            else {
                // expected is wider
                resizewidth  = width;
                resizeheight = (unsigned int)( (double)width / (double)image.columns() * (double)image.rows() + 1. );
                xoffset      = 0;
                yoffset      = (unsigned int)( (resizeheight - height) / 2. );
            }

            if (debug) printf( "resize to: %d, %d\n", resizewidth, resizeheight );
            Magick::Geometry resizeGeometry( resizewidth, resizeheight, 0, 0, 0, 0 );
            image.resize( resizeGeometry );

            // limit canvas size to cropGeometry
            if (debug) printf( "crop to: %d, %d, %d, %d\n", width, height, xoffset, yoffset );
            Magick::Geometry cropGeometry( width, height, xoffset, yoffset, 0, 0 );

            Magick::Color transparent( "white" );
            if ( strcmp( *format, "PNG" ) == 0 ) {
                // make background transparent for PNG
                // JPEG background becomes black if set transparent here
                transparent.alpha( 1. );
            }
            image.extent( cropGeometry, transparent );
        }
        else if ( strcmp ( resizeStyle, "aspectfit" ) == 0 ) {
            // keep aspect ratio, get the maximum image which fits inside specified size
            char geometryString[ 32 ];
            sprintf( geometryString, "%dx%d", width, height );
            if (debug) printf( "resize to: %s\n", geometryString );
            image.resize( geometryString );
        }
        else if ( strcmp ( resizeStyle, "fill" ) == 0 ) {
            // change aspect ratio and fill specified size
            char geometryString[ 32 ];
            sprintf( geometryString, "%dx%d!", width, height );
            if (debug) printf( "resize to: %s\n", geometryString );
            image.resize( geometryString );
        }
        else {
            return THROW_ERROR_EXCEPTION("resizeStyle not supported");
        }
        if (debug) printf( "resized to: %d, %d\n", (int)image.columns(), (int)image.rows() );
    }

    unsigned int quality = obj->Get( String::NewSymbol("quality") )->Uint32Value();
    if ( quality ) {
        if (debug) printf( "quality: %d\n", quality );
        image.quality( quality );
    }

    Magick::Blob dstBlob;
    image.write( &dstBlob );

    node::Buffer* retBuffer = node::Buffer::New( dstBlob.length() );
    memcpy( node::Buffer::Data( retBuffer->handle_ ), dstBlob.data(), dstBlob.length() );
    return scope.Close( retBuffer->handle_ );
}

// input
//   args[ 0 ]: options. required, object with following key,values
//              {
//                  srcData:        required. Buffer with binary image data
//                  debug:          optional. 1 or 0
//              }
Handle<Value> Identify(const Arguments& args) {
    HandleScope scope;
    MagickCore::SetMagickResourceLimit(MagickCore::ThreadResource, 1);

    if ( args.Length() != 1 ) {
        return THROW_ERROR_EXCEPTION("identify() requires 1 (option) argument!");
    }
    Local<Object> obj = Local<Object>::Cast( args[ 0 ] );

    Local<Object> srcData = Local<Object>::Cast( obj->Get( String::NewSymbol("srcData") ) );
    if ( srcData->IsUndefined() || ! node::Buffer::HasInstance(srcData) ) {
        return THROW_ERROR_EXCEPTION("identify()'s 1st argument should have \"srcData\" key with a Buffer instance");
    }

    int debug = obj->Get( String::NewSymbol("debug") )->Uint32Value();
    if (debug) printf( "debug: on\n" );

    Magick::Blob srcBlob( node::Buffer::Data(srcData), node::Buffer::Length(srcData) );

    Magick::Image image;
    try {
        image.read( srcBlob );
    }
    catch (std::exception& err) {
        std::string message = "image.read failed with error: ";
        message            += err.what();
        return THROW_ERROR_EXCEPTION(message.c_str());
    }
    catch (...) {
        return THROW_ERROR_EXCEPTION("unhandled error");
    }

    if (debug) printf("original width,height: %d, %d\n", (int) image.columns(), (int) image.rows());

    Handle<Object> out = Object::New();

    out->Set(String::NewSymbol("width"), Integer::New(image.columns()));
    out->Set(String::NewSymbol("height"), Integer::New(image.rows()));
    out->Set(String::NewSymbol("depth"), Integer::New(image.depth()));
    out->Set(String::NewSymbol("format"), String::New(image.magick().c_str()));

    return scope.Close( out );
}

// input
//   args[ 0 ]: options. required, object with following key,values
//              {
//                  srcData:        required. Buffer with binary image data
//                  colors:         optional. 5 by default
//                  debug:          optional. 1 or 0
//              }
Handle<Value> QuantizeColors(const Arguments& args) {
    HandleScope scope;
    MagickCore::SetMagickResourceLimit(MagickCore::ThreadResource, 1);

    if ( args.Length() != 1 ) {
        return THROW_ERROR_EXCEPTION("quantizeColors() requires 1 (option) argument!");
    }
    Local<Object> obj = Local<Object>::Cast( args[ 0 ] );

    Local<Object> srcData = Local<Object>::Cast( obj->Get( String::NewSymbol("srcData") ) );
    if ( srcData->IsUndefined() || ! node::Buffer::HasInstance(srcData) ) {
        return THROW_ERROR_EXCEPTION("quantizeColors()'s 1st argument should have \"srcData\" key with a Buffer instance");
    }

    int colorsCount = obj->Get( String::NewSymbol("colors") )->Uint32Value();
    if (!colorsCount) colorsCount = 5;

    int debug = obj->Get( String::NewSymbol("debug") )->Uint32Value();
    if (debug) printf( "debug: on\n" );

    Magick::Blob srcBlob( node::Buffer::Data(srcData), node::Buffer::Length(srcData) );

    Magick::Image image;
    try {
        image.read( srcBlob );
    }
    catch (std::exception& err) {
        std::string message = "image.read failed with error: ";
        message            += err.what();
        return THROW_ERROR_EXCEPTION(message.c_str());
    }
    catch (...) {
        return THROW_ERROR_EXCEPTION("unhandled error");
    }

    ssize_t rows = 196; ssize_t columns = 196;

    if (debug) printf( "resize to: %d, %d\n", (int) rows, (int) columns );
    Magick::Geometry resizeGeometry( rows, columns, 0, 0, 0, 0 );
    image.resize( resizeGeometry );

    if (debug) printf("totalColors before: %d\n", (int) image.totalColors());

    image.quantizeColors(colorsCount + 1);
    image.quantize();

    if (debug) printf("totalColors after: %d\n", (int) image.totalColors());

    Magick::PixelPacket* pixels = image.getPixels(0, 0, image.columns(), image.rows());

    Magick::PixelPacket* colors = new Magick::PixelPacket[colorsCount]();
    int index = 0;

    for ( ssize_t x = 0; x < rows ; x++ ) {
        for ( ssize_t y = 0; y < columns ; y++ ) {
            Magick::PixelPacket pixel = pixels[rows * x + y];

            bool found = false;
            for(int x = 0; x < colorsCount; x++)
                if (pixel.red == colors[x].red && pixel.green == colors[x].green && pixel.blue == colors[x].blue) found = true;

            if (!found) colors[index++] = pixel;
            if (index >= colorsCount) break;
        }
        if (index >= colorsCount) break;
    }

    Handle<Object> out = Array::New();

    for(int x = 0; x < colorsCount; x++)
        if (debug) printf("found rgb : %d %d %d\n", ((int) colors[x].red) / 255, ((int) colors[x].green) / 255, ((int) colors[x].blue) / 255);

    for(int x = 0; x < colorsCount; x++) {
        Local<Object> color = Object::New();

        int r = ((int) colors[x].red) / 255;
        if (r > 255) r = 255;

        int g = ((int) colors[x].green) / 255;
        if (g > 255) g = 255;

        int b = ((int) colors[x].blue) / 255;
        if (b > 255) b = 255;

        color->Set(String::NewSymbol("r"), Integer::New(r));
        color->Set(String::NewSymbol("g"), Integer::New(g));
        color->Set(String::NewSymbol("b"), Integer::New(b));

        char hexcol[16];
        snprintf(hexcol, sizeof hexcol, "%02x%02x%02x", r, g, b);
        color->Set(String::NewSymbol("hex"), String::New(hexcol));

        out->Set(x, color);
    }

    delete[] colors;

    return scope.Close( out );
}

void init(Handle<Object> target) {
    target->Set(String::NewSymbol("convert"), FunctionTemplate::New(Convert)->GetFunction());
    target->Set(String::NewSymbol("identify"), FunctionTemplate::New(Identify)->GetFunction());
    target->Set(String::NewSymbol("quantizeColors"), FunctionTemplate::New(QuantizeColors)->GetFunction());
}

NODE_MODULE(imagemagick, init);
