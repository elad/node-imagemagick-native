#include <node.h>
#include <node_buffer.h>
#include <Magick++.h>

using namespace v8;

#define THROW_ERROR_EXCEPTION(x) ThrowException(v8::Exception::Error(String::New(x)))
