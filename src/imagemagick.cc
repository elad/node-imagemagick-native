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
	Nan::Callback * callback;
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
    bool trim;
    double trimFuzz;
    std::string resizeStyle;
    std::string gravity;
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


inline Local<Value> WrapPointer(char *ptr, size_t length) {
    Nan::EscapableHandleScope scope;
    return scope.Escape(Nan::CopyBuffer(ptr, length).ToLocalChecked());
}
inline Local<Value> WrapPointer(char *ptr) {
    return WrapPointer(ptr, 0);
}


#define RETURN_BLOB_OR_ERROR(req) \
    do { \
        im_ctx_base* _context = static_cast<im_ctx_base*>(req->data); \
        if (!_context->error.empty()) { \
            Nan::ThrowError(_context->error.c_str()); \
        } else { \
            const Local<Value> _retBuffer = WrapPointer((char *)_context->dstBlob.data(), _context->dstBlob.length()); \
            info.GetReturnValue().Set(_retBuffer); \
        } \
        delete req; \
    } while(0);


bool ReadImageMagick(Magick::Image *image, Magick::Blob srcBlob, std::string srcFormat, im_ctx_base *context) {
    if( ! srcFormat.empty() ){
        if (context->debug) printf( "reading with format: %s\n", srcFormat.c_str() );
        image->magick( srcFormat.c_str() );
    }

    try {
        image->read( srcBlob );
    }
    catch (Magick::Warning& warning) {
        if (!context->ignoreWarnings) {
            context->error = warning.what();
            return false;
        } else if (context->debug) {
            printf("warning: %s\n", warning.what());
        }
    }
    catch (std::exception& err) {
        context->error = err.what();
        return false;
    }
    catch (...) {
        context->error = std::string("unhandled error");
        return false;
    }
    return true;
}

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

    if ( !ReadImageMagick(&image, srcBlob, context->srcFormat, context) )
        return;

    if (debug) printf("original width,height: %d, %d\n", (int) image.columns(), (int) image.rows());

    unsigned int width = context->width;
    if (debug) printf( "width: %d\n", width );

    unsigned int height = context->height;
    if (debug) printf( "height: %d\n", height );

    if ( context->strip ) {
        if (debug) printf( "strip: true\n" );
        image.strip();
    }

    if ( context->trim ) {
        if (debug) printf( "trim: true\n" );
        double trimFuzz = context->trimFuzz;
        if ( trimFuzz != trimFuzz ) {
            image.trim();
        } else {
            if (debug) printf( "fuzz: %lf\n", trimFuzz );
            double fuzz = image.colorFuzz();
            image.colorFuzz(trimFuzz);
            image.trim();
            image.colorFuzz(fuzz);
            if (debug) printf( "restored fuzz: %lf\n", fuzz );
        }
        if (debug) printf( "trimmed width,height: %d, %d\n", (int) image.columns(), (int) image.rows() );
    }

    const char* resizeStyle = context->resizeStyle.c_str();
    if (debug) printf( "resizeStyle: %s\n", resizeStyle );

    const char* gravity = context->gravity.c_str();
    if ( strcmp("Center", gravity)!=0
      && strcmp("East", gravity)!=0
      && strcmp("West", gravity)!=0
      && strcmp("North", gravity)!=0
      && strcmp("South", gravity)!=0
      && strcmp("NorthEast", gravity)!=0
      && strcmp("NorthWest", gravity)!=0
      && strcmp("SouthEast", gravity)!=0
      && strcmp("SouthWest", gravity)!=0
      && strcmp("None", gravity)!=0
    ) {
        context->error = std::string("gravity not supported");
        return;
    }
    if (debug) printf( "gravity: %s\n", gravity );

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
                if ( strstr(gravity, "West") != NULL ) {
                    xoffset = 0;
                }
                else if ( strstr(gravity, "East") != NULL ) {
                    xoffset = (unsigned int)( resizewidth - width );
                }
                else {
                    xoffset = (unsigned int)( (resizewidth - width) / 2. );
                }
                yoffset = 0;
            }
            else {
                // expected is wider
                resizewidth  = width;
                resizeheight = (unsigned int)( (double)width / (double)image.columns() * (double)image.rows() + 1. );
                xoffset = 0;
                if ( strstr(gravity, "North") != NULL ) {
                    yoffset = 0;
                }
                else if ( strstr(gravity, "South") != NULL ) {
                    yoffset = (unsigned int)( resizeheight - height );
                }
                else {
                    yoffset = (unsigned int)( (resizeheight - height) / 2. );
                }
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

            if ( strcmp ( gravity, "None" ) != 0 ) {
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
    Nan::HandleScope scope;

    im_ctx_base* context = static_cast<im_ctx_base*>(req->data);
    delete req;

    Local<Value> argv[2];

    if (!context->error.empty()) {
        argv[0] = Exception::Error(Nan::New<String>(context->error.c_str()).ToLocalChecked());
        argv[1] = Nan::Undefined();
    }
    else {
        argv[0] = Nan::Undefined();
        argv[1] = WrapPointer((char *)context->dstBlob.data(), context->dstBlob.length());
    }

    Nan::TryCatch try_catch; // don't quite see the necessity of this

    context->callback->Call(2, argv);

    delete context->callback;

    delete context;

    if (try_catch.HasCaught()) {
#if NODE_VERSION_AT_LEAST(0, 12, 0)
        Nan::FatalException(try_catch);
#else
        FatalException(try_catch);
#endif
    }
}

// input
//   info[ 0 ]: options. required, object with following key,values
//              {
//                  srcData:     required. Buffer with binary image data
//                  quality:     optional. 0-100 integer, default 75. JPEG/MIFF/PNG compression level.
//                  trim:        optional. default: false. trims edges that are the background color.
//                  trimFuzz:    optional. [0-1) float, default 0. trimmed color distance to edge color, 0 is exact.
//                  width:       optional. px.
//                  height:      optional. px.
//                  resizeStyle: optional. default: "aspectfill". can be "aspectfit", "fill"
//                  gravity:     optional. default: "Center". used when resizeStyle is "aspectfill"
//                                         can be "NorthWest", "North", "NorthEast", "West",
//                                         "Center", "East", "SouthWest", "South", "SouthEast", "None"
//                  format:      optional. one of http://www.imagemagick.org/script/formats.php ex: "JPEG"
//                  filter:      optional. ex: "Lagrange", "Lanczos". see ImageMagick's magick/option.c for candidates
//                  blur:        optional. ex: 0.8
//                  strip:       optional. default: false. strips comments out from image.
//                  maxMemory:   optional. set the maximum width * height of an image that can reside in the pixel cache memory.
//                  debug:       optional. 1 or 0
//              }
//   info[ 1 ]: callback. optional, if present runs async and returns result with callback(error, buffer)
NAN_METHOD(Convert) {
    Nan::HandleScope();

    bool isSync = (info.Length() == 1);

    if ( info.Length() < 1 ) {
        return Nan::ThrowError("convert() requires 1 (option) argument!");
    }

    if ( ! info[ 0 ]->IsObject() ) {
        return Nan::ThrowError("convert()'s 1st argument should be an object");
    }

    if( ! isSync && ! info[ 1 ]->IsFunction() ) {
        return Nan::ThrowError("convert()'s 2nd argument should be a function");
    }

    Local<Object> obj = Local<Object>::Cast( info[ 0 ] );

    Local<Object> srcData = Local<Object>::Cast( obj->Get( Nan::New<String>("srcData").ToLocalChecked() ) );
    if ( srcData->IsUndefined() || ! Buffer::HasInstance(srcData) ) {
        return Nan::ThrowError("convert()'s 1st argument should have \"srcData\" key with a Buffer instance");
    }

    convert_im_ctx* context = new convert_im_ctx();
    context->srcData = Buffer::Data(srcData);
    context->length = Buffer::Length(srcData);
    context->debug = obj->Get( Nan::New<String>("debug").ToLocalChecked() )->Uint32Value();
    context->ignoreWarnings = obj->Get( Nan::New<String>("ignoreWarnings").ToLocalChecked() )->Uint32Value();
    context->maxMemory = obj->Get( Nan::New<String>("maxMemory").ToLocalChecked() )->Uint32Value();
    context->width = obj->Get( Nan::New<String>("width").ToLocalChecked() )->Uint32Value();
    context->height = obj->Get( Nan::New<String>("height").ToLocalChecked() )->Uint32Value();
    context->quality = obj->Get( Nan::New<String>("quality").ToLocalChecked() )->Uint32Value();
    context->rotate = obj->Get( Nan::New<String>("rotate").ToLocalChecked() )->Int32Value();
    context->flip = obj->Get( Nan::New<String>("flip").ToLocalChecked() )->Uint32Value();
    context->density = obj->Get( Nan::New<String>("density").ToLocalChecked() )->Int32Value();

    Local<Value> trimValue = obj->Get( Nan::New<String>("trim").ToLocalChecked() );
    if ( (context->trim = ! trimValue->IsUndefined() && trimValue->BooleanValue()) ) {
        context->trimFuzz = obj->Get( Nan::New<String>("trimFuzz").ToLocalChecked() )->NumberValue() * (double) (1L << MAGICKCORE_QUANTUM_DEPTH);
    }

    Local<Value> stripValue = obj->Get( Nan::New<String>("strip").ToLocalChecked() );
    context->strip = ! stripValue->IsUndefined() && stripValue->BooleanValue();

    // manage blur as string for detect is empty
    Local<Value> blurValue = obj->Get( Nan::New<String>("blur").ToLocalChecked() );
    context->blur = "";
    if ( ! blurValue->IsUndefined() ) {
        double blurD = blurValue->NumberValue();
        std::ostringstream strs;
        strs << blurD;
        context->blur = strs.str();
    }

    Local<Value> resizeStyleValue = obj->Get( Nan::New<String>("resizeStyle").ToLocalChecked() );
    context->resizeStyle = !resizeStyleValue->IsUndefined() ?
        *String::Utf8Value(resizeStyleValue) : "aspectfill";

    Local<Value> gravityValue = obj->Get( Nan::New<String>("gravity").ToLocalChecked() );
    context->gravity = !gravityValue->IsUndefined() ?
        *String::Utf8Value(gravityValue) : "Center";

    Local<Value> formatValue = obj->Get( Nan::New<String>("format").ToLocalChecked() );
    context->format = !formatValue->IsUndefined() ?
		*String::Utf8Value(formatValue) : "";

    Local<Value> srcFormatValue = obj->Get( Nan::New<String>("srcFormat").ToLocalChecked() );
    context->srcFormat = !srcFormatValue->IsUndefined() ?
        *String::Utf8Value(srcFormatValue) : "";

    Local<Value> filterValue = obj->Get( Nan::New<String>("filter").ToLocalChecked() );
    context->filter = !filterValue->IsUndefined() ?
        *String::Utf8Value(filterValue) : "";

    uv_work_t* req = new uv_work_t();
    req->data = context;
    if(!isSync) {
        context->callback = new Nan::Callback(Local<Function>::Cast(info[1]));

        uv_queue_work(uv_default_loop(), req, DoConvert, (uv_after_work_cb)GeneratedBlobAfter);

		return;
    } else {
        DoConvert(req);
        RETURN_BLOB_OR_ERROR(req)
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

void BuildIdentifyResult(uv_work_t *req, Local<Value> *argv) {
    identify_im_ctx* context = static_cast<identify_im_ctx*>(req->data);

    if (!context->error.empty()) {
        argv[0] = Exception::Error(Nan::New<String>(context->error.c_str()).ToLocalChecked());
        argv[1] = Nan::Undefined();
    }
    else {
        argv[0] = Nan::Undefined();
        Local<Object> out = Nan::New<Object>();

        out->Set(Nan::New<String>("width").ToLocalChecked(), Nan::New<Integer>(static_cast<int>(context->image.columns())));
        out->Set(Nan::New<String>("height").ToLocalChecked(), Nan::New<Integer>(static_cast<int>(context->image.rows())));
        out->Set(Nan::New<String>("depth").ToLocalChecked(), Nan::New<Integer>(static_cast<int>(context->image.depth())));
        out->Set(Nan::New<String>("format").ToLocalChecked(), Nan::New<String>(context->image.magick().c_str()).ToLocalChecked());

        Local<Object> out_density = Nan::New<Object>();
        Magick::Geometry density = context->image.density();
        out_density->Set(Nan::New<String>("width").ToLocalChecked(), Nan::New<Integer>(static_cast<int>(density.width())));
        out_density->Set(Nan::New<String>("height").ToLocalChecked(), Nan::New<Integer>(static_cast<int>(density.height())));
        out->Set(Nan::New<String>("density").ToLocalChecked(), out_density);

        Local<Object> out_exif = Nan::New<Object>();
        out_exif->Set(Nan::New<String>("orientation").ToLocalChecked(), Nan::New<Integer>(atoi(context->image.attribute("EXIF:Orientation").c_str())));
        out->Set(Nan::New<String>("exif").ToLocalChecked(), out_exif);

        argv[1] = out;
    }
}

void IdentifyAfter(uv_work_t* req) {
    Nan::HandleScope scope;

    Local<Value> argv[2];
    BuildIdentifyResult(req,argv);

    identify_im_ctx* context = static_cast<identify_im_ctx*>(req->data);

    Nan::TryCatch try_catch; // don't quite see the necessity of this

    context->callback->Call(2, argv);

    delete context->callback;
    delete context;
    delete req;

    if (try_catch.HasCaught()) {
#if NODE_VERSION_AT_LEAST(0, 12, 0)
        Nan::FatalException(try_catch);
#else
        FatalException(try_catch);
#endif
    }
}

// input
//   info[ 0 ]: options. required, object with following key,values
//              {
//                  srcData:        required. Buffer with binary image data
//                  debug:          optional. 1 or 0
//              }
//   info[ 1 ]: callback. optional, if present runs async and returns result with callback(error, info)
NAN_METHOD(Identify) {
    Nan::HandleScope scope;

    bool isSync = info.Length() == 1;

    if ( info.Length() < 1 ) {
        return Nan::ThrowError("identify() requires 1 (option) argument!");
    }
    Local<Object> obj = Local<Object>::Cast( info[ 0 ] );

    Local<Object> srcData = Local<Object>::Cast( obj->Get( Nan::New<String>("srcData").ToLocalChecked() ) );
    if ( srcData->IsUndefined() || ! Buffer::HasInstance(srcData) ) {
        return Nan::ThrowError("identify()'s 1st argument should have \"srcData\" key with a Buffer instance");
    }

    if( ! isSync && ! info[ 1 ]->IsFunction() ) {
        return Nan::ThrowError("identify()'s 2nd argument should be a function");
    }

    identify_im_ctx* context = new identify_im_ctx();
    context->srcData = Buffer::Data(srcData);
    context->length = Buffer::Length(srcData);

    context->debug          = obj->Get( Nan::New<String>("debug").ToLocalChecked() )->Uint32Value();
    context->ignoreWarnings = obj->Get( Nan::New<String>("ignoreWarnings").ToLocalChecked() )->Uint32Value();

    if (context->debug) printf( "debug: on\n" );
    if (context->debug) printf( "ignoreWarnings: %d\n", context->ignoreWarnings );

    uv_work_t* req = new uv_work_t();
    req->data = context;
    if(!isSync) {
        context->callback = new Nan::Callback(Local<Function>::Cast(info[1]));

        uv_queue_work(uv_default_loop(), req, DoIdentify, (uv_after_work_cb)IdentifyAfter);

        return;
    } else {
        DoIdentify(req);
        Local<Value> argv[2];
        BuildIdentifyResult(req, argv);
        delete static_cast<identify_im_ctx*>(req->data);
        delete req;
        if(argv[0]->IsUndefined()){
            info.GetReturnValue().Set(argv[1]);
        } else {
            return Nan::ThrowError(argv[0]);
        }
    }
}

// input
//   info[ 0 ]: options. required, object with following key,values
//              {
//                  srcData:        required. Buffer with binary image data
//                  x:              required. x,y,columns,rows provide the area of interest.
//                  y:              required.
//                  columns:        required.
//                  rows:           required.
//              }
NAN_METHOD(GetConstPixels) {
    Nan::HandleScope();
    MagickCore::SetMagickResourceLimit(MagickCore::ThreadResource, 1);

    if ( info.Length() != 1 ) {
        return Nan::ThrowError("getConstPixels() requires 1 (option) argument!");
    }
    Local<Object> obj = Local<Object>::Cast( info[ 0 ] );

    Local<Object> srcData = Local<Object>::Cast( obj->Get( Nan::New<String>("srcData").ToLocalChecked() ) );
    if ( srcData->IsUndefined() || ! Buffer::HasInstance(srcData) ) {
        return Nan::ThrowError("getConstPixels()'s 1st argument should have \"srcData\" key with a Buffer instance");
    }

    unsigned int xValue       = obj->Get( Nan::New<String>("x").ToLocalChecked() )->Uint32Value();
    unsigned int yValue       = obj->Get( Nan::New<String>("y").ToLocalChecked() )->Uint32Value();
    unsigned int columnsValue = obj->Get( Nan::New<String>("columns").ToLocalChecked() )->Uint32Value();
    unsigned int rowsValue    = obj->Get( Nan::New<String>("rows").ToLocalChecked() )->Uint32Value();

    int debug          = obj->Get( Nan::New<String>("debug").ToLocalChecked() )->Uint32Value();
    int ignoreWarnings = obj->Get( Nan::New<String>("ignoreWarnings").ToLocalChecked() )->Uint32Value();
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
            return Nan::ThrowError(message.c_str());
        }
    }
    catch (...) {
        return Nan::ThrowError("unhandled error");
    }

    size_t w = image.columns();
    size_t h = image.rows();

    if (xValue+columnsValue > w || yValue+rowsValue > h) {
        return Nan::ThrowError("x/y/columns/rows values are beyond the image\'s dimensions");
    }

    const Magick::PixelPacket *pixels = image.getConstPixels(xValue, yValue, columnsValue, rowsValue);

    Local<Object> out = Nan::New<Array>();
    for (unsigned int i=0; i<columnsValue * rowsValue; i++) {
        Magick::PixelPacket pixel = pixels[ i ];
        Local<Object> color = Nan::New<Object>();

        color->Set(Nan::New<String>("red").ToLocalChecked(),     Nan::New<Integer>(pixel.red));
        color->Set(Nan::New<String>("green").ToLocalChecked(),   Nan::New<Integer>(pixel.green));
        color->Set(Nan::New<String>("blue").ToLocalChecked(),    Nan::New<Integer>(pixel.blue));
        color->Set(Nan::New<String>("opacity").ToLocalChecked(), Nan::New<Integer>(pixel.opacity));

        out->Set(i, color);
    }

    info.GetReturnValue().Set(out);
}

// input
//   info[ 0 ]: options. required, object with following key,values
//              {
//                  srcData:        required. Buffer with binary image data
//                  colors:         optional. 5 by default
//                  debug:          optional. 1 or 0
//              }
NAN_METHOD(QuantizeColors) {
    Nan::HandleScope();
    MagickCore::SetMagickResourceLimit(MagickCore::ThreadResource, 1);

    if ( info.Length() != 1 ) {
        return Nan::ThrowError("quantizeColors() requires 1 (option) argument!");
    }
    Local<Object> obj = Local<Object>::Cast( info[ 0 ] );

    Local<Object> srcData = Local<Object>::Cast( obj->Get( Nan::New<String>("srcData").ToLocalChecked() ) );
    if ( srcData->IsUndefined() || ! Buffer::HasInstance(srcData) ) {
        return Nan::ThrowError("quantizeColors()'s 1st argument should have \"srcData\" key with a Buffer instance");
    }

    int colorsCount = obj->Get( Nan::New<String>("colors").ToLocalChecked() )->Uint32Value();
    if (!colorsCount) colorsCount = 5;

    int debug          = obj->Get( Nan::New<String>("debug").ToLocalChecked() )->Uint32Value();
    int ignoreWarnings = obj->Get( Nan::New<String>("ignoreWarnings").ToLocalChecked() )->Uint32Value();
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
            return Nan::ThrowError(message.c_str());
        }
    }
    catch (...) {
        return Nan::ThrowError("unhandled error");
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

    Local<Object> out = Nan::New<Array>();

    for(int x = 0; x < colorsCount; x++)
        if (debug) printf("found rgb : %d %d %d\n", ((int) colors[x].red) / 255, ((int) colors[x].green) / 255, ((int) colors[x].blue) / 255);

    for(int x = 0; x < colorsCount; x++) {
        Local<Object> color = Nan::New<Object>();

        int r = ((int) colors[x].red) / 255;
        if (r > 255) r = 255;

        int g = ((int) colors[x].green) / 255;
        if (g > 255) g = 255;

        int b = ((int) colors[x].blue) / 255;
        if (b > 255) b = 255;

        color->Set(Nan::New<String>("r").ToLocalChecked(), Nan::New<Integer>(r));
        color->Set(Nan::New<String>("g").ToLocalChecked(), Nan::New<Integer>(g));
        color->Set(Nan::New<String>("b").ToLocalChecked(), Nan::New<Integer>(b));

        char hexcol[16];
        snprintf(hexcol, sizeof hexcol, "%02x%02x%02x", r, g, b);
        color->Set(Nan::New<String>("hex").ToLocalChecked(), Nan::New<String>(hexcol).ToLocalChecked());

        out->Set(x, color);
    }

    delete[] colors;

    info.GetReturnValue().Set(out);
}

void DoComposite(uv_work_t* req) {

    composite_im_ctx* context = static_cast<composite_im_ctx*>(req->data);

    MagickCore::SetMagickResourceLimit(MagickCore::ThreadResource, 1);

    if (context->debug) printf( "debug: on\n" );
    if (context->debug) printf( "ignoreWarnings: %d\n", context->ignoreWarnings );

    Magick::Blob srcBlob( context->srcData, context->length );
    Magick::Blob compositeBlob( context->compositeData, context->compositeLength );

    Magick::Image image;

    if ( !ReadImageMagick(&image, srcBlob, "", context) )
        return;

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

    if ( !ReadImageMagick(&compositeImage, compositeBlob, "", context) )
        return;

    image.composite(compositeImage,gravityType,Magick::OverCompositeOp);

    Magick::Blob dstBlob;
    image.write( &dstBlob );

    context->dstBlob = dstBlob;
}

// input
//   info[ 0 ]: options. required, object with following key,values
//              {
//                  srcData:        required. Buffer with binary image data
//                  compositeData:  required. Buffer with image to composite
//                  gravity:        optional. One of CenterGravity EastGravity
//                                  ForgetGravity NorthEastGravity NorthGravity
//                                  NorthWestGravity SouthEastGravity SouthGravity
//                                  SouthWestGravity WestGravity
//                  debug:          optional. 1 or 0
//              }
//   info[ 1 ]: callback. optional, if present runs async and returns result with callback(error, buffer)
NAN_METHOD(Composite) {
    Nan::HandleScope();

    bool isSync = (info.Length() == 1);

    if ( info.Length() < 1 ) {
        return Nan::ThrowError("composite() requires 1 (option) argument!");
    }
    Local<Object> obj = Local<Object>::Cast( info[ 0 ] );

    Local<Object> srcData = Local<Object>::Cast( obj->Get( Nan::New<String>("srcData").ToLocalChecked() ) );
    if ( srcData->IsUndefined() || ! Buffer::HasInstance(srcData) ) {
        return Nan::ThrowError("composite()'s 1st argument should have \"srcData\" key with a Buffer instance");
    }

    Local<Object> compositeData = Local<Object>::Cast( obj->Get( Nan::New<String>("compositeData").ToLocalChecked() ) );
    if ( compositeData->IsUndefined() || ! Buffer::HasInstance(compositeData) ) {
        return Nan::ThrowError("composite()'s 1st argument should have \"compositeData\" key with a Buffer instance");
    }

    if( ! isSync && ! info[ 1 ]->IsFunction() ) {
        return Nan::ThrowError("composite()'s 2nd argument should be a function");
    }

    composite_im_ctx* context = new composite_im_ctx();
    context->debug = obj->Get( Nan::New<String>("debug").ToLocalChecked() )->Uint32Value();
    context->ignoreWarnings = obj->Get( Nan::New<String>("ignoreWarnings").ToLocalChecked() )->Uint32Value();

    context->srcData = Buffer::Data(srcData);
    context->length = Buffer::Length(srcData);

    context->compositeData = Buffer::Data(compositeData);
    context->compositeLength = Buffer::Length(compositeData);

    Local<Value> gravityValue = obj->Get( Nan::New<String>("gravity").ToLocalChecked() );
    context->gravity = !gravityValue->IsUndefined() ?
         *String::Utf8Value(gravityValue) : "";

    uv_work_t* req = new uv_work_t();
    req->data = context;
    if(!isSync) {
        context->callback = new Nan::Callback(Local<Function>::Cast(info[1]));

        uv_queue_work(uv_default_loop(), req, DoComposite, (uv_after_work_cb)GeneratedBlobAfter);

        return;
    } else {
        DoComposite(req);
        RETURN_BLOB_OR_ERROR(req)
    }
}

NAN_METHOD(Version) {
    Nan::HandleScope();

    info.GetReturnValue().Set(Nan::New<String>(MagickLibVersionText).ToLocalChecked());
}

NAN_METHOD(GetQuantumDepth) {
    Nan::HandleScope();

    info.GetReturnValue().Set(Nan::New<Integer>(MAGICKCORE_QUANTUM_DEPTH));
}

void init(Handle<Object> exports) {
    Nan::SetMethod(exports, "convert", Convert);
    Nan::SetMethod(exports, "identify", Identify);
    Nan::SetMethod(exports, "quantizeColors", QuantizeColors);
    Nan::SetMethod(exports, "composite", Composite);
    Nan::SetMethod(exports, "version", Version);
    Nan::SetMethod(exports, "getConstPixels", GetConstPixels);
    Nan::SetMethod(exports, "quantumDepth", GetQuantumDepth); // QuantumDepth is already defined
}

// There is no semi-colon after NODE_MODULE as it's not a function (see node.h).
// see http://nodejs.org/api/addons.html
NODE_MODULE(imagemagick, init)
