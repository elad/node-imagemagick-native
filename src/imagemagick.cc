#ifndef BUILDING_NODE_EXTENSION
#define BUILDING_NODE_EXTENSION
#endif  // BUILDING_NODE_EXTENSION

#include "imagemagick.h"
#include <list>
#include <string.h>
#include <exception>

#define THROW_ERROR_EXCEPTION(x) ThrowException(v8::Exception::Error(String::New(x))); \
    scope.Close(Undefined())

// RAII to reset image magick's resource limit
class LocalResourceLimiter
{
public:
    LocalResourceLimiter()
      : originalMemory(0),
        memoryLimited(0),
        originalDisk(0),
        diskLimited(0) {
    }
    ~LocalResourceLimiter() {
        if (memoryLimited) {
            MagickCore::SetMagickResourceLimit(MagickCore::MemoryResource,originalMemory);
        }
        if (diskLimited) {
            MagickCore::SetMagickResourceLimit(MagickCore::DiskResource,originalDisk);
        }
    }
    void LimitMemory(MagickCore::MagickSizeType to) {
        if ( ! memoryLimited ) {
            memoryLimited  = true;
            originalMemory = MagickCore::GetMagickResourceLimit(MagickCore::MemoryResource);
        }
        MagickCore::SetMagickResourceLimit(MagickCore::MemoryResource, to);
    }
    void LimitDisk(MagickCore::MagickSizeType to) {
        if ( ! diskLimited ) {
            diskLimited  = true;
            originalDisk = MagickCore::GetMagickResourceLimit(MagickCore::DiskResource);
        }
        MagickCore::SetMagickResourceLimit(MagickCore::DiskResource,   to);
    }

private:
    MagickCore::MagickSizeType originalMemory;
    bool memoryLimited;
    MagickCore::MagickSizeType originalDisk;
    bool diskLimited;
};

// input
//   args[ 0 ]: options. required, object with following key,values
//              {
//                  srcData:        required. Buffer with binary image data
//                  quality:        optional. 0-100 integer, default 75. JPEG/MIFF/PNG compression level.
//                  width:          optional. px.
//                  height:         optional. px.
//                  resizeStyle:    optional. default: "aspectfill". can be "aspectfit", "fill"
//                  format:         optional. one of http://www.imagemagick.org/script/formats.php ex: "JPEG"
//                  maxMemory:      optional. set the maximum width * height of an image that can reside in the pixel cache memory.
//                  debug:          optional. 1 or 0
//                  ignoreWarnings: optional. 1 or 0
//              }
Handle<Value> Convert(const Arguments& args) {
    HandleScope scope;
    MagickCore::SetMagickResourceLimit(MagickCore::ThreadResource, 1);
    LocalResourceLimiter limiter;

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
    int ignoreWarnings = obj->Get( String::NewSymbol("ignoreWarnings") )->Uint32Value();
    if (debug) printf( "debug: on\n" );

    unsigned int maxMemory = obj->Get( String::NewSymbol("maxMemory") )->Uint32Value();
    if (maxMemory > 0) {
        limiter.LimitMemory(maxMemory);
        limiter.LimitDisk(maxMemory); // avoid using unlimited disk as cache
        if (debug) printf( "maxMemory set to: %d\n", maxMemory );
    }

    Magick::Blob srcBlob( node::Buffer::Data(srcData), node::Buffer::Length(srcData) );

    Magick::Image image;
    try {
        image.read( srcBlob );
    }
    catch (std::exception& err) {
        std::string message = "image.read failed with error: ";
        message            += err.what();
        
        std::string warn ("warn");
        std::size_t found = err.what().find(warn);
        if (ignoreWarnings && found != std::string::npos) {
            if (debug) printf("warning: %s\n", message.c_str());
        } else {
            return THROW_ERROR_EXCEPTION(message.c_str());
        }
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
            try {
                image.resize( resizeGeometry );
            }
            catch (std::exception& err) {
                std::string message = "image.resize failed with error: ";
                message            += err.what();
                return THROW_ERROR_EXCEPTION(message.c_str());
            }
            catch (...) {
                return THROW_ERROR_EXCEPTION("unhandled error");
            }

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

            try {
                image.resize( geometryString );
            }
            catch (std::exception& err) {
                std::string message = "image.resize failed with error: ";
                message            += err.what();
                return THROW_ERROR_EXCEPTION(message.c_str());
            }
            catch (...) {
                return THROW_ERROR_EXCEPTION("unhandled error");
            }
        }
        else if ( strcmp ( resizeStyle, "fill" ) == 0 ) {
            // change aspect ratio and fill specified size
            char geometryString[ 32 ];
            sprintf( geometryString, "%dx%d!", width, height );
            if (debug) printf( "resize to: %s\n", geometryString );

            try {
                image.resize( geometryString );
            }
            catch (std::exception& err) {
                std::string message = "image.resize failed with error: ";
                message            += err.what();
                return THROW_ERROR_EXCEPTION(message.c_str());
            }
            catch (...) {
                return THROW_ERROR_EXCEPTION("unhandled error");
            }
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

    int rotate = obj->Get( String::NewSymbol("rotate") )->Int32Value();
    if ( rotate ) {
        if (debug) printf( "rotate: %d\n", rotate );
        image.rotate(rotate);
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

// input
//   args[ 0 ]: options. required, object with following key,values
//              {
//                  srcData:        required. Buffer with binary image data
//                  compositeData:  required. Buffer with image to composite
//                  gravity:        optional. One of CenterGravity EastGravity
//                                  ForgetGravity NorthEastGravity NorthGravity
//                                  NorthWestGravity SouthEastGravity SouthGravity
//                                  SouthWestGravity WestGravity
//                  debug:          optional. 1 or 0
//              }
Handle<Value> Composite(const Arguments& args) {
    HandleScope scope;
    MagickCore::SetMagickResourceLimit(MagickCore::ThreadResource, 1);

    if ( args.Length() != 1 ) {
        return THROW_ERROR_EXCEPTION("composite() requires 1 (option) argument!");
    }
    Local<Object> obj = Local<Object>::Cast( args[ 0 ] );

    Local<Object> srcData = Local<Object>::Cast( obj->Get( String::NewSymbol("srcData") ) );
    if ( srcData->IsUndefined() || ! node::Buffer::HasInstance(srcData) ) {
        return THROW_ERROR_EXCEPTION("composite()'s 1st argument should have \"srcData\" key with a Buffer instance");
    }

    Local<Object> compositeData = Local<Object>::Cast( obj->Get( String::NewSymbol("compositeData") ) );
    if ( compositeData->IsUndefined() || ! node::Buffer::HasInstance(compositeData) ) {
        return THROW_ERROR_EXCEPTION("composite()'s 1st argument should have \"compositeData\" key with a Buffer instance");
    }


    int debug = obj->Get( String::NewSymbol("debug") )->Uint32Value();
    if (debug) printf( "debug: on\n" );

    Magick::Blob srcBlob( node::Buffer::Data(srcData), node::Buffer::Length(srcData) );
    Magick::Blob compositeBlob( node::Buffer::Data(compositeData), node::Buffer::Length(compositeData) );

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

    Magick::GravityType gravityType;

    Local<Value> gravityValue = obj->Get( String::NewSymbol("gravity") );
    String::AsciiValue gravity( gravityValue->ToString() );

    if(strcmp("CenterGravity",*gravity)==0)         gravityType=Magick::CenterGravity;
    else if(strcmp("EastGravity",*gravity)==0)      gravityType=Magick::EastGravity;
    else if(strcmp("ForgetGravity",*gravity)==0)    gravityType=Magick::ForgetGravity;
    else if(strcmp("NorthEastGravity",*gravity)==0) gravityType=Magick::NorthEastGravity;
    else if(strcmp("NorthGravity",*gravity)==0)     gravityType=Magick::NorthGravity;
    else if(strcmp("NorthWestGravity",*gravity)==0) gravityType=Magick::NorthWestGravity;
    else if(strcmp("SouthEastGravity",*gravity)==0) gravityType=Magick::SouthEastGravity;
    else if(strcmp("SouthGravity",*gravity)==0)     gravityType=Magick::SouthGravity;
    else if(strcmp("SouthWestGravity",*gravity)==0) gravityType=Magick::SouthWestGravity;
    else if(strcmp("WestGravity",*gravity)==0)      gravityType=Magick::WestGravity;
    else {
        gravityType = Magick::ForgetGravity;
        if (debug) printf( "invalid gravity: '%s' fell through to ForgetGravity\n",*gravity);
    }

    if (debug) printf( "gravity: %s (%d)\n",*gravity,(int) gravityType);

    Magick::Image compositeImage;
        try {
            compositeImage.read( compositeBlob );
        }
        catch (std::exception& err) {
            std::string message = "compositeImage.read failed with error: ";
            message            += err.what();
            return THROW_ERROR_EXCEPTION(message.c_str());
        }
        catch (...) {
            return THROW_ERROR_EXCEPTION("unhandled error");
        }

    image.composite(compositeImage,gravityType,Magick::OverCompositeOp);

    Magick::Blob dstBlob;
    image.write( &dstBlob );

    node::Buffer* retBuffer = node::Buffer::New( dstBlob.length() );
    memcpy( node::Buffer::Data( retBuffer->handle_ ), dstBlob.data(), dstBlob.length() );
    return scope.Close( retBuffer->handle_ );
}

Handle<Value> Version(const Arguments& args) {
    HandleScope scope;

    Handle<String> out = String::New( MagickLibVersionText );

    return scope.Close( out );
}

void init(Handle<Object> target) {
    target->Set(String::NewSymbol("convert"), FunctionTemplate::New(Convert)->GetFunction());
    target->Set(String::NewSymbol("identify"), FunctionTemplate::New(Identify)->GetFunction());
    target->Set(String::NewSymbol("quantizeColors"), FunctionTemplate::New(QuantizeColors)->GetFunction());
    target->Set(String::NewSymbol("composite"), FunctionTemplate::New(Composite)->GetFunction());
    target->Set(String::NewSymbol("version"), FunctionTemplate::New(Version)->GetFunction());
}

// There is no semi-colon after NODE_MODULE as it's not a function (see node.h).
// see http://nodejs.org/api/addons.html
NODE_MODULE(imagemagick, init)
