#include "imagemagick.h"
#include <list>
#include <string.h>

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
    Magick::Image image( srcBlob );
    if (debug) printf("original width,height: %d, %d\n", (int) image.columns(), (int) image.rows());

    unsigned int width = obj->Get( String::NewSymbol("width") )->Uint32Value();
    if (debug) printf( "width: %d\n", width );

    unsigned int height = obj->Get( String::NewSymbol("height") )->Uint32Value();
    if (debug) printf( "height: %d\n", height );

    Local<Value> resizeStyleValue = obj->Get( String::NewSymbol("resizeStyle") );
    const char* resizeStyle = "aspectfill";
    if ( ! resizeStyleValue->IsUndefined() ) {
        String::AsciiValue resizeStyleAsciiValue( resizeStyleValue->ToString() );
        resizeStyle = *resizeStyleAsciiValue;
    }
    if (debug) printf( "resizeStyle: %s\n", resizeStyle );

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

            if (debug) printf( "crop to: %d, %d, %d, %d\n", width, height, xoffset, yoffset );
            Magick::Geometry cropGeometry( width, height, xoffset, yoffset, 0, 0 );
            image.extent( cropGeometry );
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

    Local<Value> formatValue = obj->Get( String::NewSymbol("format") );
    if ( ! formatValue->IsUndefined() ) {
        String::AsciiValue format( formatValue->ToString() );
        if (debug) printf( "format: %s\n", *format );
        image.magick( *format );
    }

    // make background white when converting transparent png to jpg
    std::list<Magick::Image> imageList( 1, image );
    Magick::flattenImages( &image, imageList.begin(), imageList.end() );
    imageList.clear();

    Magick::Blob dstBlob;
    image.write( &dstBlob );

    node::Buffer* retBuffer = node::Buffer::New( dstBlob.length() );
    memcpy( node::Buffer::Data( retBuffer->handle_ ), dstBlob.data(), dstBlob.length() );
    return scope.Close( retBuffer->handle_ );
}

void init(Handle<Object> target) {
    NODE_SET_METHOD(target, "convert", Convert);
}

NODE_MODULE(imagemagick, init);
