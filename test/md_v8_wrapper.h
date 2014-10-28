#ifndef MD_V8_WRAPPER_H_
#define MD_V8_WRAPPER_H_

#include <memory>
#include "v8.h"

#include "md_env.h"
#include "md_env_inl.h"

namespace Mordor
{
namespace Test
{

class MD_Worker;

class MD_V8Wrapper
{
public:
    // Creates a new execution environment containing the built-in
    // functions.
    static v8::Handle<v8::Context> createContext(v8::Isolate* isolate);
    static bool execString(Environment* env, v8::Handle<v8::String> source, bool print_result, bool report_exceptions);
    static bool execString(Environment* env, const char* str, bool print_result, bool report_exceptions);
    static bool execString(Environment* env, const std::string& str, bool print_result, bool report_exceptions);

public:
    template<typename T>
    static v8::Handle<v8::String> toV8String(v8::Isolate* isolate, T str)
    {
        return MD_V8Wrapper::toV8String(isolate, str);
    }

    static v8::Handle<v8::String> toV8String(v8::Isolate* isolate, const char* str)
    {
        return v8::String::NewFromUtf8(isolate, str);
    }

    static v8::Handle<v8::String> toV8String(v8::Isolate* isolate, const std::string& str)
    {
        return MD_V8Wrapper::toV8String(isolate, str.c_str());
    }

protected:

    // The callback that is invoked by v8 whenever the JavaScript 'print'
    // function is called.  Prints its arguments on stdout separated by
    // spaces and ending with a newline.
    static void Print(const v8::FunctionCallbackInfo<v8::Value>& args);
    // The callback that is invoked by v8 whenever the JavaScript 'read'
    // function is called.  This function loads the content of the file named in
    // the argument into a JavaScript string.
    static void Read(const v8::FunctionCallbackInfo<v8::Value>& args);
    // The callback that is invoked by v8 whenever the JavaScript 'load'
    // function is called.  Loads, compiles and executes its argument
    // JavaScript file.
    static void Load(const v8::FunctionCallbackInfo<v8::Value>& args);
    // The callback that is invoked by v8 whenever the JavaScript 'quit'
    // function is called.  Quits.
    static void Quit(const v8::FunctionCallbackInfo<v8::Value>& args);

    static void Version(const v8::FunctionCallbackInfo<v8::Value>& args);

    static void Exception(const v8::FunctionCallbackInfo<v8::Value>& args);
};

} } // namespace Mordor::Test

#endif // MD_V8_WRAPPER_H_
