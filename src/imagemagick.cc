#ifndef BUILDING_NODE_EXTENSION
#define BUILDING_NODE_EXTENSION
#endif  // BUILDING_NODE_EXTENSION

#if _MSC_VER
#define snprintf _snprintf
#endif

#include "imagemagick.h"
#include <list>
#include <sstream>
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

// Base context for calls shared on sync and async code paths
struct im_ctx_base {
    NanCallback * callback;
    std::string error;

    char* srcData;
    size_t length;
    int debug;
    int ignoreWarnings;

    // generated blob by convert or composite
    Magick::Blob dstBlob;

    virtual ~im_ctx_base() {}
};
// Extra context for identify
struct identify_im_ctx : im_ctx_base {
    Magick::Image image;

    identify_im_ctx() {}
};
// Extra context for convert
struct convert_im_ctx : im_ctx_base {
    unsigned int maxMemory;

    unsigned int width;
    unsigned int height;
    bool strip;
    std::string resizeStyle;
    std::string format;
    std::string filter;
    std::string blur;
    unsigned int quality;
    int rotate;
    int density;
    int flip;

    std::string srcFormat;

    convert_im_ctx() {}
};
// Extra context for composite
struct composite_im_ctx : im_ctx_base {
    char* compositeData;
    size_t compositeLength;

    std::string gravity;

    composite_im_ctx() {}
};

void DoConvert(uv_work_t* req) {

    convert_im_ctx* context = static_cast<convert_im_ctx*>(req->data);

    MagickCore::SetMagickResourceLimit(MagickCore::ThreadResource, 1);
    LocalResourceLimiter limiter;

    int debug = context->debug;

    if (debug) printf( "debug: on\n" );
    if (debug) printf( "ignoreWarnings: %d\n", context->ignoreWarnings );

    if (context->maxMemory > 0) {
        limiter.LimitMemory(context->maxMemory);
        limiter.LimitDisk(context->maxMemory); // avoid using unlimited disk as cache
        if (debug) printf( "maxMemory set to: %d\n", context->maxMemory );
    }

    Magick::Blob srcBlob( context->srcData, context->length );

    Magick::Image image;

    if( ! context->srcFormat.empty() ){
        if (debug) printf( "srcFormat: %s\n", context->srcFormat.c_str() );
        image.magick( context->srcFormat.c_str() );
    }

    try {
        image.read( srcBlob );
    }
    catch (Magick::Warning& warning) {
        if (!context->ignoreWarnings) {
            context->error = warning.what();
            return;
        } else if (debug) {
            printf("warning: %s\n", warning.what());
        }
    }
    catch (std::exception& err) {
        context->error = err.what();
        return;
    }
    catch (...) {
        context->error = std::string("unhandled error");
        return;
    }

    if (debug) printf("original width,height: %d, %d\n", (int) image.columns(), (int) image.rows());

    unsigned int width = context->width;
    if (debug) printf( "width: %d\n", width );

    unsigned int height = context->height;
    if (debug) printf( "height: %d\n", height );

    if ( context->strip ) {
        if (debug) printf( "strip: true\n" );
        image.strip();
    }

    const char* resizeStyle = context->resizeStyle.c_str();
    if (debug) printf( "resizeStyle: %s\n", resizeStyle );

    if( ! context->format.empty() ){
        if (debug) printf( "format: %s\n", context->format.c_str() );
        image.magick( context->format.c_str() );
    }

    if( ! context->filter.empty() ){
        const char *filter = context->filter.c_str();

        ssize_t option_info = MagickCore::ParseCommandOption(MagickCore::MagickFilterOptions, Magick::MagickFalse, filter);
        if (option_info != -1) {
            if (debug) printf( "filter: %s\n", filter );
            image.filterType( (Magick::FilterTypes)option_info );
        }
        else {
            context->error = std::string("filter not supported");
            return;
        }
    }

    if( ! context->blur.empty() ) {
        double blur = atof (context->blur.c_str());
        if (debug) printf( "blur: %.1f\n", blur );
        image.blur(0, blur);
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
                image.zoom( resizeGeometry );
            }
            catch (std::exception& err) {
                std::string message = "image.resize failed with error: ";
                message            += err.what();
                context->error = message;
                return;
            }
            catch (...) {
                context->error = std::string("unhandled error");
                return;
            }

            // limit canvas size to cropGeometry
            if (debug) printf( "crop to: %d, %d, %d, %d\n", width, height, xoffset, yoffset );
            Magick::Geometry cropGeometry( width, height, xoffset, yoffset, 0, 0 );

            Magick::Color transparent( "transparent" );
            if ( strcmp( context->format.c_str(), "PNG" ) == 0 ) {
                // make background transparent for PNG
                // JPEG background becomes black if set transparent here
                transparent.alpha( 1. );
            }

            #if MagickLibVersion > 0x654
                image.extent( cropGeometry, transparent );
            #else
                image.extent( cropGeometry );
            #endif
        }
        else if ( strcmp ( resizeStyle, "aspectfit" ) == 0 ) {
            // keep aspect ratio, get the maximum image which fits inside specified size
            char geometryString[ 32 ];
            sprintf( geometryString, "%dx%d", width, height );
            if (debug) printf( "resize to: %s\n", geometryString );

            try {
                image.zoom( geometryString );
            }
            catch (std::exception& err) {
                std::string message = "image.resize failed with error: ";
                message            += err.what();
                context->error = message;
                return;
            }
            catch (...) {
                context->error = std::string("unhandled error");
                return;
            }
        }
        else if ( strcmp ( resizeStyle, "fill" ) == 0 ) {
            // change aspect ratio and fill specified size
            char geometryString[ 32 ];
            sprintf( geometryString, "%dx%d!", width, height );
            if (debug) printf( "resize to: %s\n", geometryString );

            try {
                image.zoom( geometryString );
            }
            catch (std::exception& err) {
                std::string message = "image.resize failed with error: ";
                message            += err.what();
                context->error = message;
                return;
            }
            catch (...) {
                context->error = std::string("unhandled error");
                return;
            }
        }
        else {
            context->error = std::string("resizeStyle not supported");
            return;
        }
        if (debug) printf( "resized to: %d, %d\n", (int)image.columns(), (int)image.rows() );
    }

    if ( context->quality ) {
        if (debug) printf( "quality: %d\n", context->quality );
        image.quality( context->quality );
    }

    if ( context->rotate ) {
        if (debug) printf( "rotate: %d\n", context->rotate );
        image.rotate( context->rotate );
    }

    if ( context->flip ) {
        if ( debug ) printf( "flip\n" );
        image.flip();
    }

    if (context->density) {
        image.density(Magick::Geometry(context->density, context->density));
    }

    Magick::Blob dstBlob;
    try {
        image.write( &dstBlob );
    }
    catch (std::exception& err) {
        std::string message = "image.write failed with error: ";
        message            += err.what();
        context->error = message;
        return;
    }
    catch (...) {
        context->error = std::string("unhandled error");
        return;
    }
    context->dstBlob = dstBlob;
}

// Make callback from convert or composite
void GeneratedBlobAfter(uv_work_t* req) {
    NanScope();

    im_ctx_base* context = static_cast<im_ctx_base*>(req->data);
    delete req;

    Handle<Value> argv[2];

    if (!context->error.empty()) {
        argv[0] = Exception::Error(NanNew<String>(context->error.c_str()));
        argv[1] = NanUndefined();
    }
    else {
        argv[0] = NanUndefined();
        const Handle<Object> retBuffer = NanNewBufferHandle(context->dstBlob.length());
        memcpy( Buffer::Data(retBuffer), context->dstBlob.data(), context->dstBlob.length() );
        argv[1] = retBuffer;
    }

    TryCatch try_catch; // don't quite see the necessity of this

    context->callback->Call(2, argv);

    delete context->callback;

    delete context;

    if (try_catch.HasCaught())
        FatalException(try_catch);
}

// input
//   args[ 0 ]: options. required, object with following key,values
//              {
//                  srcData:     required. Buffer with binary image data
//                  quality:     optional. 0-100 integer, default 75. JPEG/MIFF/PNG compression level.
//                  width:       optional. px.
//                  height:      optional. px.
//                  resizeStyle: optional. default: "aspectfill". can be "aspectfit", "fill"
//                  format:      optional. one of http://www.imagemagick.org/script/formats.php ex: "JPEG"
//                  filter:      optional. ex: "Lagrange", "Lanczos". see ImageMagick's magick/option.c for candidates
//                  blur:        optional. ex: 0.8
//                  strip:       optional. default: false. strips comments out from image.
//                  maxMemory:   optional. set the maximum width * height of an image that can reside in the pixel cache memory.
//                  debug:       optional. 1 or 0
//              }
//   args[ 1 ]: callback. optional, if present runs async and returns result with callback(error, buffer)
NAN_METHOD(Convert) {
    NanScope();

    bool isSync = (args.Length() == 1);

    if ( args.Length() < 1 ) {
        return NanThrowError("convert() requires 1 (option) argument!");
    }

    if ( ! args[ 0 ]->IsObject() ) {
        return NanThrowError("convert()'s 1st argument should be an object");
    }

    if( ! isSync && ! args[ 1 ]->IsFunction() ) {
        return NanThrowError("convert()'s 2nd argument should be a function");
    }

    Local<Object> obj = Local<Object>::Cast( args[ 0 ] );

    Local<Object> srcData = Local<Object>::Cast( obj->Get( NanNew<String>("srcData") ) );
    if ( srcData->IsUndefined() || ! Buffer::HasInstance(srcData) ) {
        return NanThrowError("convert()'s 1st argument should have \"srcData\" key with a Buffer instance");
    }

    convert_im_ctx* context = new convert_im_ctx();
    context->srcData = Buffer::Data(srcData);
    context->length = Buffer::Length(srcData);
    context->debug = obj->Get( NanNew<String>("debug") )->Uint32Value();
    context->ignoreWarnings = obj->Get( NanNew<String>("ignoreWarnings") )->Uint32Value();
    context->maxMemory = obj->Get( NanNew<String>("maxMemory") )->Uint32Value();
    context->width = obj->Get( NanNew<String>("width") )->Uint32Value();
    context->height = obj->Get( NanNew<String>("height") )->Uint32Value();
    context->quality = obj->Get( NanNew<String>("quality") )->Uint32Value();
    context->rotate = obj->Get( NanNew<String>("rotate") )->Int32Value();
    context->flip = obj->Get( NanNew<String>("flip") )->Uint32Value();
    context->density = obj->Get( NanNew<String>("density") )->Int32Value();

    Local<Value> stripValue = obj->Get( NanNew<String>("strip") );
    context->strip = ! stripValue->IsUndefined() && stripValue->BooleanValue();

    // manage blur as string for detect is empty
    Local<Value> blurValue = obj->Get( NanNew<String>("blur") );
    context->blur = "";
    if ( ! blurValue->IsUndefined() ) {
        double blurD = blurValue->NumberValue();
        std::ostringstream strs;
        strs << blurD;
        context->blur = strs.str();
    }

    // shared unused counter for NanCString
    size_t count;

    Local<Value> resizeStyleValue = obj->Get( NanNew<String>("resizeStyle") );
    context->resizeStyle = !resizeStyleValue->IsUndefined() ?
        NanCString(resizeStyleValue, &count) : "aspectfill";

    Local<Value> formatValue = obj->Get( NanNew<String>("format") );
    context->format = !formatValue->IsUndefined() ?
         NanCString(formatValue, &count) : "";

    Local<Value> srcFormatValue = obj->Get( NanNew<String>("srcFormat") );
    context->srcFormat = !srcFormatValue->IsUndefined() ?
        NanCString(srcFormatValue, &count) : "";

    Local<Value> filterValue = obj->Get( NanNew<String>("filter") );
    context->filter = !filterValue->IsUndefined() ?
        NanCString(filterValue, &count) : "";

    uv_work_t* req = new uv_work_t();
    req->data = context;
    if(!isSync) {
        context->callback = new NanCallback(Local<Function>::Cast(args[1]));

        uv_queue_work(uv_default_loop(), req, DoConvert, (uv_after_work_cb)GeneratedBlobAfter);

        NanReturnUndefined();
    } else {
        DoConvert(req);
        convert_im_ctx* context = static_cast<convert_im_ctx*>(req->data);
        delete req;
        if (!context->error.empty()) {
            const char *err_str = context->error.c_str();
            delete context;
            return NanThrowError(err_str);
        }
        else {
            const Handle<Object> retBuffer = NanNewBufferHandle(context->dstBlob.length());
            memcpy( Buffer::Data(retBuffer), context->dstBlob.data(), context->dstBlob.length() );
            delete context;
            NanReturnValue(retBuffer);
        }
    }
}

void DoIdentify(uv_work_t* req) {

    MagickCore::SetMagickResourceLimit(MagickCore::ThreadResource, 1);

    identify_im_ctx* context = static_cast<identify_im_ctx*>(req->data);

    Magick::Blob srcBlob( context->srcData, context->length );

    Magick::Image image;
    try {
        image.read( srcBlob );
    }
    catch (std::exception& err) {
        std::string what (err.what());
        std::string message = std::string("image.read failed with error: ") + what;
        std::size_t found   = what.find( "warn" );
        if (context->ignoreWarnings && (found != std::string::npos)) {
            if (context->debug) printf("warning: %s\n", message.c_str());
        }
        else {
            context->error = message;
        }
    }
    catch (...) {
        context->error = std::string("unhandled error");
    }
    if(!context->error.empty()) {
        return;
    }

    if (context->debug) printf("original width,height: %d, %d\n", (int) image.columns(), (int) image.rows());

    context->image = image;
}

void BuildIdentifyResult(uv_work_t *req, Handle<Value> *argv) {
    identify_im_ctx* context = static_cast<identify_im_ctx*>(req->data);

    if (!context->error.empty()) {
        argv[0] = Exception::Error(NanNew<String>(context->error.c_str()));
        argv[1] = NanUndefined();
    }
    else {
        argv[0] = NanUndefined();
        Handle<Object> out = NanNew<Object>();

        out->Set(NanNew<String>("width"), NanNew<Integer>(context->image.columns()));
        out->Set(NanNew<String>("height"), NanNew<Integer>(context->image.rows()));
        out->Set(NanNew<String>("depth"), NanNew<Integer>(context->image.depth()));
        out->Set(NanNew<String>("format"), NanNew<String>(context->image.magick().c_str()));

        Handle<Object> out_density = NanNew<Object>();
        Magick::Geometry density = context->image.density();
        out_density->Set(NanNew<String>("width"), NanNew<Integer>(density.width()));
        out_density->Set(NanNew<String>("height"), NanNew<Integer>(density.height()));
        out->Set(NanNew<String>("density"), out_density);

        Handle<Object> out_exif = NanNew<Object>();
        out_exif->Set(NanNew<String>("orientation"), NanNew<Integer>(atoi(context->image.attribute("EXIF:Orientation").c_str())));
        out->Set(NanNew<String>("exif"), out_exif);

        argv[1] = out;
    }
}

void IdentifyAfter(uv_work_t* req) {
    NanScope();

    Handle<Value> argv[2];
    BuildIdentifyResult(req,argv);

    identify_im_ctx* context = static_cast<identify_im_ctx*>(req->data);

    TryCatch try_catch; // don't quite see the necessity of this

    context->callback->Call(2, argv);

    delete context->callback;
    delete context;
    delete req;

    if (try_catch.HasCaught())
        FatalException(try_catch);
}

// input
//   args[ 0 ]: options. required, object with following key,values
//              {
//                  srcData:        required. Buffer with binary image data
//                  debug:          optional. 1 or 0
//              }
//   args[ 1 ]: callback. optional, if present runs async and returns result with callback(error, info)
NAN_METHOD(Identify) {
    NanScope();

    bool isSync = args.Length() == 1;

    if ( args.Length() < 1 ) {
        return NanThrowError("identify() requires 1 (option) argument!");
    }
    Local<Object> obj = Local<Object>::Cast( args[ 0 ] );

    Local<Object> srcData = Local<Object>::Cast( obj->Get( NanNew<String>("srcData") ) );
    if ( srcData->IsUndefined() || ! Buffer::HasInstance(srcData) ) {
        return NanThrowError("identify()'s 1st argument should have \"srcData\" key with a Buffer instance");
    }

    if( ! isSync && ! args[ 1 ]->IsFunction() ) {
        return NanThrowError("identify()'s 2nd argument should be a function");
    }

    identify_im_ctx* context = new identify_im_ctx();
    context->srcData = Buffer::Data(srcData);
    context->length = Buffer::Length(srcData);

    context->debug          = obj->Get( NanNew<String>("debug") )->Uint32Value();
    context->ignoreWarnings = obj->Get( NanNew<String>("ignoreWarnings") )->Uint32Value();

    if (context->debug) printf( "debug: on\n" );
    if (context->debug) printf( "ignoreWarnings: %d\n", context->ignoreWarnings );

    uv_work_t* req = new uv_work_t();
    req->data = context;
    if(!isSync) {
        context->callback = new NanCallback(Local<Function>::Cast(args[1]));

        uv_queue_work(uv_default_loop(), req, DoIdentify, (uv_after_work_cb)IdentifyAfter);

        NanReturnUndefined();
    } else {
        DoIdentify(req);
        Handle<Value> argv[2];
        BuildIdentifyResult(req,argv);
        delete static_cast<identify_im_ctx*>(req->data);
        delete req;
        if(argv[0]->IsUndefined()){
            NanReturnValue(argv[1]);
        } else {
            return NanThrowError(argv[0]);
        }
    }
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
    image.zoom( resizeGeometry );

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

void DoComposite(uv_work_t* req) {

    composite_im_ctx* context = static_cast<composite_im_ctx*>(req->data);

    MagickCore::SetMagickResourceLimit(MagickCore::ThreadResource, 1);

    if (context->debug) printf( "debug: on\n" );
    if (context->debug) printf( "ignoreWarnings: %d\n", context->ignoreWarnings );

    Magick::Blob srcBlob( context->srcData, context->length );
    Magick::Blob compositeBlob( context->compositeData, context->compositeLength );

    Magick::Image image;
    try {
        image.read( srcBlob );
    }
    catch (std::exception& err) {
        std::string what (err.what());
        std::string message = std::string("image.read failed with error: ") + what;
        std::size_t found   = what.find( "warn" );
        if (context->ignoreWarnings && (found != std::string::npos)) {
            if (context->debug) printf("warning: %s\n", message.c_str());
        }
        else {
            context->error = message;
            return;
        }
    }
    catch (...) {
        context->error = std::string("unhandled error");
        return;
    }

    Magick::GravityType gravityType;

    const char* gravity = context->gravity.c_str();

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
        if (context->debug) printf( "invalid gravity: '%s' fell through to ForgetGravity\n", gravity);
    }

    if (context->debug) printf( "gravity: %s (%d)\n", gravity,(int) gravityType);

    Magick::Image compositeImage;
    try {
        compositeImage.read( compositeBlob );
    }
    catch (std::exception& err) {
        std::string what (err.what());
        std::string message = std::string("compositeImage.read failed with error: ") + what;
        std::size_t found   = what.find( "warn" );
        if (context->ignoreWarnings && (found != std::string::npos)) {
            if (context->debug) printf("warning: %s\n", message.c_str());
        }
        else {
            context->error = message;
            return;
        }
    }
    catch (...) {
        context->error = std::string("unhandled error");
        return;
    }

    image.composite(compositeImage,gravityType,Magick::OverCompositeOp);

    Magick::Blob dstBlob;
    image.write( &dstBlob );

    context->dstBlob = dstBlob;
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
//   args[ 1 ]: callback. optional, if present runs async and returns result with callback(error, buffer)
NAN_METHOD(Composite) {
    NanScope();

    bool isSync = (args.Length() == 1);

    if ( args.Length() < 1 ) {
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

    if( ! isSync && ! args[ 1 ]->IsFunction() ) {
        return NanThrowError("composite()'s 2nd argument should be a function");
    }

    composite_im_ctx* context = new composite_im_ctx();
    context->debug = obj->Get( NanNew<String>("debug") )->Uint32Value();
    context->ignoreWarnings = obj->Get( NanNew<String>("ignoreWarnings") )->Uint32Value();

    context->srcData = Buffer::Data(srcData);
    context->length = Buffer::Length(srcData);

    context->compositeData = Buffer::Data(compositeData);
    context->compositeLength = Buffer::Length(compositeData);

    Local<Value> gravityValue = obj->Get( NanNew<String>("gravity") );
    size_t count;
    context->gravity = !gravityValue->IsUndefined() ?
         NanCString(gravityValue, &count) : "";

    uv_work_t* req = new uv_work_t();
    req->data = context;
    if(!isSync) {
        context->callback = new NanCallback(Local<Function>::Cast(args[1]));

        uv_queue_work(uv_default_loop(), req, DoComposite, (uv_after_work_cb)GeneratedBlobAfter);

        NanReturnUndefined();
    } else {
        DoComposite(req);
        composite_im_ctx* context = static_cast<composite_im_ctx*>(req->data);
        delete req;
        if (!context->error.empty()) {
            const char *err_str = context->error.c_str();
            delete context;
            return NanThrowError(err_str);
        }
        else {
            const Handle<Object> retBuffer = NanNewBufferHandle(context->dstBlob.length());
            memcpy( Buffer::Data(retBuffer), context->dstBlob.data(), context->dstBlob.length() );
            delete context;
            NanReturnValue(retBuffer);
        }
    }
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
