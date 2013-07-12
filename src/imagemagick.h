#include <Magick++.h>
#include <node.h>
#include <node_buffer.h>

using namespace v8;

#define THROW_ERROR_EXCEPTION(x) ThrowException(v8::Exception::Error(String::New(x)))
