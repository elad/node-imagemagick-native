#ifndef BUILDING_NODE_EXTENSION
#define BUILDING_NODE_EXTENSION
#endif  // BUILDING_NODE_EXTENSION

#if _MSC_VER
#define snprintf _snprintf
#endif

#include "imagemagick.h"
#include <list>
#include <string.h>
#include <exception>

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
//                  srcData:     required. Buffer with binary image data
//                  quality:     optional. 0-100 integer, default 75. JPEG/MIFF/PNG compression level.
//                  width:       optional. px.
//                  height:      optional. px.
//                  resizeStyle: optional. default: "aspectfill". can be "aspectfit", "fill"
//                  format:      optional. one of http://www.imagemagick.org/script/formats.php ex: "JPEG"
//                  maxMemory:   optional. set the maximum width * height of an image that can reside in the pixel cache memory.
//                  debug:       optional. 1 or 0
//              }
NAN_METHOD(Convert) {
    NanScope();
    MagickCore::SetMagickResourceLimit(MagickCore::ThreadResource, 1);
    LocalResourceLimiter limiter;

    if ( args.Length() != 1 ) {
        return NanThrowError("convert() requires 1 (option) argument!");
    }
    if ( ! args[ 0 ]->IsObject() ) {
        return NanThrowError("convert()'s 1st argument should be an object");
    }
    Local<Object> obj = Local<Object>::Cast( args[ 0 ] );

    Local<Object> srcData = Local<Object>::Cast( obj->Get( NanNew<String>("srcData") ) );
    if ( srcData->IsUndefined() || ! Buffer::HasInstance(srcData) ) {
        return NanThrowError("convert()'s 1st argument should have \"srcData\" key with a Buffer instance");
    }

    int debug          = obj->Get( NanNew<String>("debug") )->Uint32Value();
    int ignoreWarnings = obj->Get( NanNew<String>("ignoreWarnings") )->Uint32Value();
    if (debug) printf( "debug: on\n" );
    if (debug) printf( "ignoreWarnings: %d\n", ignoreWarnings );

    unsigned int maxMemory = obj->Get( NanNew<String>("maxMemory") )->Uint32Value();
    if (maxMemory > 0) {
        limiter.LimitMemory(maxMemory);
        limiter.LimitDisk(maxMemory); // avoid using unlimited disk as cache
        if (debug) printf( "maxMemory set to: %d\n", maxMemory );
    }

    Magick::Blob srcBlob( Buffer::Data(srcData), Buffer::Length(srcData) );

    Magick::Image image;
    try {
        image.read( srcBlob );
    }
    catch (std::exception& err) {
        std::string what (err.what());
        std::string message = std::string("image.read failed with error: ") + what;
        std::size_t found   = what.find( "warn" );
        if (ignoreWarnings && (found != std::string::npos)) {
            if (debug) printf("warning: %s\n", message.c_str());
        }
        else {
            return NanThrowError(message.c_str());
        }
    }
    catch (...) {
        return NanThrowError("unhandled error");
    }

    if (debug) printf("original width,height: %d, %d\n", (int) image.columns(), (int) image.rows());

    unsigned int width = obj->Get( NanNew<String>("width") )->Uint32Value();
    if (debug) printf( "width: %d\n", width );

    unsigned int height = obj->Get( NanNew<String>("height") )->Uint32Value();
    if (debug) printf( "height: %d\n", height );

    Local<Value> resizeStyleValue = obj->Get( NanNew<String>("resizeStyle") );
    const char* resizeStyle = "aspectfill";
    if ( ! resizeStyleValue->IsUndefined() ) {
        size_t count;
        resizeStyle = NanCString(resizeStyleValue, &count);
    }
    if (debug) printf( "resizeStyle: %s\n", resizeStyle );

    Local<Value> formatValue = obj->Get( NanNew<String>("format") );
    const char* format;
    if ( ! formatValue->IsUndefined() ) {
        size_t count;
        format = NanCString(formatValue, &count);
        if (debug) printf( "format: %s\n", format );
        image.magick( format );
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
                return NanThrowError(message.c_str());
            }
            catch (...) {
                return NanThrowError("unhandled error");
            }

            // limit canvas size to cropGeometry
            if (debug) printf( "crop to: %d, %d, %d, %d\n", width, height, xoffset, yoffset );
            Magick::Geometry cropGeometry( width, height, xoffset, yoffset, 0, 0 );

            Magick::Color transparent( "white" );
            if ( strcmp( format, "PNG" ) == 0 ) {
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
                return NanThrowError(message.c_str());
            }
            catch (...) {
                return NanThrowError("unhandled error");
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
                return NanThrowError(message.c_str());
            }
            catch (...) {
                return NanThrowError("unhandled error");
            }
        }
        else {
            return NanThrowError("resizeStyle not supported");
        }
        if (debug) printf( "resized to: %d, %d\n", (int)image.columns(), (int)image.rows() );
    }

    unsigned int quality = obj->Get( NanNew<String>("quality") )->Uint32Value();
    if ( quality ) {
        if (debug) printf( "quality: %d\n", quality );
        image.quality( quality );
    }

    int rotate = obj->Get( NanNew<String>("rotate") )->Int32Value();
    if ( rotate ) {
        if (debug) printf( "rotate: %d\n", rotate );
        image.rotate(rotate);
    }

    Magick::Blob dstBlob;
    image.write( &dstBlob );

    const Handle<Object> retBuffer = NanNewBufferHandle(dstBlob.length());
    memcpy( Buffer::Data(retBuffer), dstBlob.data(), dstBlob.length() );
    NanReturnValue(retBuffer);
}

class IdentifyWorker : public NanAsyncWorker
{
public:
    IdentifyWorker(NanCallback *callback, char* srcData, size_t length, int debug, int ignoreWarnings)
      : NanAsyncWorker(callback),
        srcData(srcData),
        length(length),
        debug(debug),
        ignoreWarnings(ignoreWarnings) {
    }
    ~IdentifyWorker() {
    }

    void Execute () {
        Magick::Blob srcBlob( srcData, length );

        try {
            image.read( srcBlob );
        }
        catch (std::exception& err) {
            std::string what (err.what());
            std::string message = std::string("image.read failed with error: ") + what;
            std::size_t found   = what.find( "warn" );
            if (ignoreWarnings && (found != std::string::npos)) {
                if (debug) printf("warning: %s\n", message.c_str());
            }
            else {
                SetErrorMessage(message.c_str());
                return;
            }
        }
        catch (...) {
            SetErrorMessage("unhandled error");
            return;
        }

        if (debug) printf("original width,height: %d, %d\n", (int) image.columns(), (int) image.rows());
    }

    void HandleOKCallback () {
        NanScope();

        Local<Object> out = NanNew<Object>();

        out->Set(NanNew<String>("width"), NanNew<Integer>(image.columns()));
        out->Set(NanNew<String>("height"), NanNew<Integer>(image.rows()));
        out->Set(NanNew<String>("depth"), NanNew<Integer>(image.depth()));
        out->Set(NanNew<String>("format"), NanNew<String>(image.magick().c_str()));

        Local<Object> out_exif = NanNew<Object>();
        out_exif->Set(NanNew<String>("orientation"), NanNew<Integer>(atoi(image.attribute("EXIF:Orientation").c_str())));
        out->Set(NanNew<String>("exif"), out_exif);

        Local<Value> argv[] = {
              NanNull()
            , out
        };

        callback->Call(2, argv);
    };

    private:
        char* srcData;
        size_t length;
        int debug;
        int ignoreWarnings;
        Magick::Image image;
};

// input
//   args[ 0 ]: options. required, object with following key,values
//              {
//                  srcData:        required. Buffer with binary image data
//                  debug:          optional. 1 or 0
//              }
NAN_METHOD(Identify) {
    NanScope();
    MagickCore::SetMagickResourceLimit(MagickCore::ThreadResource, 1);

    if ( args.Length() != 2 ) {
        return NanThrowError("identify() requires 2 arguments!");
    }
    Local<Object> obj = Local<Object>::Cast( args[ 0 ] );
    Local<Object> srcData = Local<Object>::Cast( obj->Get( NanNew<String>("srcData") ) );
    if ( srcData->IsUndefined() || ! Buffer::HasInstance(srcData) ) {
        NanThrowError("identify()'s 1st argument should have \"srcData\" key with a Buffer instance");
        NanReturnUndefined();
    }

    int debug          = obj->Get( NanNew<String>("debug") )->Uint32Value();
    int ignoreWarnings = obj->Get( NanNew<String>("ignoreWarnings") )->Uint32Value();
    if (debug) printf( "debug: on\n" );
    if (debug) printf( "ignoreWarnings: %d\n", ignoreWarnings );

    NanCallback *callback = new NanCallback(args[1].As<Function>());
    NanAsyncQueueWorker(new IdentifyWorker(callback, Buffer::Data(srcData), Buffer::Length(srcData), debug, ignoreWarnings));
    NanReturnUndefined();
}

// input
//   args[ 0 ]: options. required, object with following key,values
//              {
//                  srcData:        required. Buffer with binary image data
//                  x:              required. x,y,columns,rows provide the area of interest.
//                  y:              required.
//                  columns:        required.
//                  rows:           required.
//              }
NAN_METHOD(GetConstPixels) {
    NanScope();
    MagickCore::SetMagickResourceLimit(MagickCore::ThreadResource, 1);

    if ( args.Length() != 1 ) {
        return NanThrowError("getConstPixels() requires 1 (option) argument!");
    }
    Local<Object> obj = Local<Object>::Cast( args[ 0 ] );

    Local<Object> srcData = Local<Object>::Cast( obj->Get( NanNew<String>("srcData") ) );
    if ( srcData->IsUndefined() || ! Buffer::HasInstance(srcData) ) {
        return NanThrowError("getConstPixels()'s 1st argument should have \"srcData\" key with a Buffer instance");
    }

    unsigned int xValue       = obj->Get( NanNew<String>("x") )->Uint32Value();
    unsigned int yValue       = obj->Get( NanNew<String>("y") )->Uint32Value();  
    unsigned int columnsValue = obj->Get( NanNew<String>("columns") )->Uint32Value();  
    unsigned int rowsValue    = obj->Get( NanNew<String>("rows") )->Uint32Value();  

    int debug          = obj->Get( NanNew<String>("debug") )->Uint32Value();
    int ignoreWarnings = obj->Get( NanNew<String>("ignoreWarnings") )->Uint32Value();
    if (debug) printf( "debug: on\n" );
    if (debug) printf( "ignoreWarnings: %d\n", ignoreWarnings );

    Magick::Blob srcBlob( Buffer::Data(srcData), Buffer::Length(srcData));

    Magick::Image image;    
    try {
        image.read( srcBlob );        
    }
    catch (std::exception& err) {
        std::string what (err.what());
        std::string message = std::string("image.read failed with error: ") + what;
        std::size_t found   = what.find( "warn" );
        if (ignoreWarnings && (found != std::string::npos)) {
            if (debug) printf("warning: %s\n", message.c_str());
        }
        else {
            return NanThrowError(message.c_str());
        }
    }
    catch (...) {
        return NanThrowError("unhandled error");
    }

    size_t w = image.columns();
    size_t h = image.rows();    

    if (xValue+columnsValue > w || yValue+rowsValue > h) {
        return NanThrowError("x/y/columns/rows values are beyond the image\'s dimensions");
    }

    const Magick::PixelPacket *pixels = image.getConstPixels(xValue, yValue, columnsValue, rowsValue);

    Handle<Object> out = NanNew<Array>();
    for (unsigned int i=0; i<columnsValue * rowsValue; i++) {
        Magick::PixelPacket pixel = pixels[ i ];
        Local<Object> color = NanNew<Object>();

        color->Set(NanNew<String>("red"),     NanNew<Integer>(pixel.red));
        color->Set(NanNew<String>("green"),   NanNew<Integer>(pixel.green));
        color->Set(NanNew<String>("blue"),    NanNew<Integer>(pixel.blue));
        color->Set(NanNew<String>("opacity"), NanNew<Integer>(pixel.opacity));
        
        out->Set(i, color);
    }

    NanReturnValue(out);
}

// input
//   args[ 0 ]: options. required, object with following key,values
//              {
//                  srcData:        required. Buffer with binary image data
//                  colors:         optional. 5 by default
//                  debug:          optional. 1 or 0
//              }
NAN_METHOD(QuantizeColors) {
    NanScope();
    MagickCore::SetMagickResourceLimit(MagickCore::ThreadResource, 1);

    if ( args.Length() != 1 ) {
        return NanThrowError("quantizeColors() requires 1 (option) argument!");
    }
    Local<Object> obj = Local<Object>::Cast( args[ 0 ] );

    Local<Object> srcData = Local<Object>::Cast( obj->Get( NanNew<String>("srcData") ) );
    if ( srcData->IsUndefined() || ! Buffer::HasInstance(srcData) ) {
        return NanThrowError("quantizeColors()'s 1st argument should have \"srcData\" key with a Buffer instance");
    }

    int colorsCount = obj->Get( NanNew<String>("colors") )->Uint32Value();
    if (!colorsCount) colorsCount = 5;

    int debug          = obj->Get( NanNew<String>("debug") )->Uint32Value();
    int ignoreWarnings = obj->Get( NanNew<String>("ignoreWarnings") )->Uint32Value();
    if (debug) printf( "debug: on\n" );
    if (debug) printf( "ignoreWarnings: %d\n", ignoreWarnings );

    Magick::Blob srcBlob( Buffer::Data(srcData), Buffer::Length(srcData) );

    Magick::Image image;
    try {
        image.read( srcBlob );
    }
    catch (std::exception& err) {
        std::string what (err.what());
        std::string message = std::string("image.read failed with error: ") + what;
        std::size_t found   = what.find( "warn" );
        if (ignoreWarnings && (found != std::string::npos)) {
            if (debug) printf("warning: %s\n", message.c_str());
        }
        else {
            return NanThrowError(message.c_str());
        }
    }
    catch (...) {
        return NanThrowError("unhandled error");
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

    Handle<Object> out = NanNew<Array>();

    for(int x = 0; x < colorsCount; x++)
        if (debug) printf("found rgb : %d %d %d\n", ((int) colors[x].red) / 255, ((int) colors[x].green) / 255, ((int) colors[x].blue) / 255);

    for(int x = 0; x < colorsCount; x++) {
        Local<Object> color = NanNew<Object>();

        int r = ((int) colors[x].red) / 255;
        if (r > 255) r = 255;

        int g = ((int) colors[x].green) / 255;
        if (g > 255) g = 255;

        int b = ((int) colors[x].blue) / 255;
        if (b > 255) b = 255;

        color->Set(NanNew<String>("r"), NanNew<Integer>(r));
        color->Set(NanNew<String>("g"), NanNew<Integer>(g));
        color->Set(NanNew<String>("b"), NanNew<Integer>(b));

        char hexcol[16];
        snprintf(hexcol, sizeof hexcol, "%02x%02x%02x", r, g, b);
        color->Set(NanNew<String>("hex"), NanNew<String>(hexcol));

        out->Set(x, color);
    }

    delete[] colors;

    NanReturnValue(out);
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
NAN_METHOD(Composite) {
    NanScope();
    MagickCore::SetMagickResourceLimit(MagickCore::ThreadResource, 1);

    if ( args.Length() != 1 ) {
        return NanThrowError("composite() requires 1 (option) argument!");
    }
    Local<Object> obj = Local<Object>::Cast( args[ 0 ] );

    Local<Object> srcData = Local<Object>::Cast( obj->Get( NanNew<String>("srcData") ) );
    if ( srcData->IsUndefined() || ! Buffer::HasInstance(srcData) ) {
        return NanThrowError("composite()'s 1st argument should have \"srcData\" key with a Buffer instance");
    }

    Local<Object> compositeData = Local<Object>::Cast( obj->Get( NanNew<String>("compositeData") ) );
    if ( compositeData->IsUndefined() || ! Buffer::HasInstance(compositeData) ) {
        return NanThrowError("composite()'s 1st argument should have \"compositeData\" key with a Buffer instance");
    }

    int debug          = obj->Get( NanNew<String>("debug") )->Uint32Value();
    int ignoreWarnings = obj->Get( NanNew<String>("ignoreWarnings") )->Uint32Value();
    if (debug) printf( "debug: on\n" );
    if (debug) printf( "ignoreWarnings: %d\n", ignoreWarnings );

    Magick::Blob srcBlob( Buffer::Data(srcData), Buffer::Length(srcData) );
    Magick::Blob compositeBlob( Buffer::Data(compositeData), Buffer::Length(compositeData) );

    Magick::Image image;
    try {
        image.read( srcBlob );
    }
    catch (std::exception& err) {
        std::string what (err.what());
        std::string message = std::string("image.read failed with error: ") + what;
        std::size_t found   = what.find( "warn" );
        if (ignoreWarnings && (found != std::string::npos)) {
            if (debug) printf("warning: %s\n", message.c_str());
        }
        else {
            return NanThrowError(message.c_str());
        }
    }
    catch (...) {
        return NanThrowError("unhandled error");
    }

    Magick::GravityType gravityType;

    Local<Value> gravityValue = obj->Get( NanNew<String>("gravity") );
    size_t count;
    const char* gravity = NanCString(gravityValue, &count);

    if(strcmp("CenterGravity", gravity)==0)         gravityType=Magick::CenterGravity;
    else if(strcmp("EastGravity", gravity)==0)      gravityType=Magick::EastGravity;
    else if(strcmp("ForgetGravity", gravity)==0)    gravityType=Magick::ForgetGravity;
    else if(strcmp("NorthEastGravity", gravity)==0) gravityType=Magick::NorthEastGravity;
    else if(strcmp("NorthGravity", gravity)==0)     gravityType=Magick::NorthGravity;
    else if(strcmp("NorthWestGravity", gravity)==0) gravityType=Magick::NorthWestGravity;
    else if(strcmp("SouthEastGravity", gravity)==0) gravityType=Magick::SouthEastGravity;
    else if(strcmp("SouthGravity", gravity)==0)     gravityType=Magick::SouthGravity;
    else if(strcmp("SouthWestGravity", gravity)==0) gravityType=Magick::SouthWestGravity;
    else if(strcmp("WestGravity", gravity)==0)      gravityType=Magick::WestGravity;
    else {
        gravityType = Magick::ForgetGravity;
        if (debug) printf( "invalid gravity: '%s' fell through to ForgetGravity\n", gravity);
    }

    if (debug) printf( "gravity: %s (%d)\n", gravity,(int) gravityType);

    Magick::Image compositeImage;
    try {
        compositeImage.read( compositeBlob );
    }
    catch (std::exception& err) {
        std::string what (err.what());
        std::string message = std::string("compositeImage.read failed with error: ") + what;
        std::size_t found   = what.find( "warn" );
        if (ignoreWarnings && (found != std::string::npos)) {
            if (debug) printf("warning: %s\n", message.c_str());
        }
        else {
            return NanThrowError(message.c_str());
        }
    }
    catch (...) {
        return NanThrowError("unhandled error");
    }

    image.composite(compositeImage,gravityType,Magick::OverCompositeOp);

    Magick::Blob dstBlob;
    image.write( &dstBlob );

    const Handle<Object> retBuffer = NanNewBufferHandle(dstBlob.length());
    memcpy( Buffer::Data(retBuffer), dstBlob.data(), dstBlob.length() );
    NanReturnValue(retBuffer);
}

NAN_METHOD(Version) {
    NanScope();

    NanReturnValue(NanNew<String>(MagickLibVersionText));
}

NAN_METHOD(GetQuantumDepth) {
    NanScope();

    NanReturnValue(NanNew<Integer>(MAGICKCORE_QUANTUM_DEPTH));
}

void init(Handle<Object> exports) {
#if NODE_MODULE_VERSION >= 14
    NODE_SET_METHOD(exports, "convert", Convert);
    NODE_SET_METHOD(exports, "identify", Identify);
    NODE_SET_METHOD(exports, "quantizeColors", QuantizeColors);
    NODE_SET_METHOD(exports, "composite", Composite);
    NODE_SET_METHOD(exports, "version", Version);
    NODE_SET_METHOD(exports, "getConstPixels", GetConstPixels);
    NODE_SET_METHOD(exports, "quantumDepth", GetQuantumDepth); // QuantumDepth is already defined
#else
    exports->Set(NanNew<String>("convert"), FunctionTemplate::New(Convert)->GetFunction());
    exports->Set(NanNew<String>("identify"), FunctionTemplate::New(Identify)->GetFunction());
    exports->Set(NanNew<String>("quantizeColors"), FunctionTemplate::New(QuantizeColors)->GetFunction());
    exports->Set(NanNew<String>("composite"), FunctionTemplate::New(Composite)->GetFunction());
    exports->Set(NanNew<String>("version"), FunctionTemplate::New(Version)->GetFunction());
    exports->Set(NanNew<String>("getConstPixels"), FunctionTemplate::New(GetConstPixels)->GetFunction());
    exports->Set(NanNew<String>("quantumDepth"), FunctionTemplate::New(GetQuantumDepth)->GetFunction());
#endif
}

// There is no semi-colon after NODE_MODULE as it's not a function (see node.h).
// see http://nodejs.org/api/addons.html
NODE_MODULE(imagemagick, init)
