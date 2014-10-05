
#include <assert.h>
#include <iostream>

#include <memory>
#include <functional>

#include "mordor/exception.h"
#include "mordor/coroutine.h"
#include "mordor/iomanager.h"
#include "mordor/streams/file.h"
#include "mordor/streams/std.h"
#include "mordor/streams/buffer.h"
#include "mordor/sleep.h"

#include "md_v8_wrapper.h"
#include "md_worker.h"
#include "md_task.h"
#include "libplatform/libplatform.h"
#include "libplatform/md_platform.h"

/*
static const char* g_harmony_opts = " --harmony --harmony_scoping --harmony_modules"
                           " --harmony_proxies --harmony_generators"
                           " --harmony_numeric_literals --harmony_strings"
                           " --harmony_arrays --harmony_arrow_functions"
                           " --use_strict";
                           */

// Extracts a C string from a V8 Utf8Value.
static const char* ToCString(const v8::String::Utf8Value& value)
{
    return *value ? *value : "<string conversion failed>";
}

static void ReportException(v8::Isolate* isolate, v8::TryCatch* try_catch)
{
    v8::HandleScope handle_scope(isolate);
    v8::String::Utf8Value exception(try_catch->Exception());
    const char* exception_string = ::ToCString(exception);
    v8::Handle<v8::Message> message = try_catch->Message();
    if (message.IsEmpty()) {
        // V8 didn't provide any extra information about this error; just
        // print the exception.
        fprintf(stderr, "%s\n", exception_string);
    } else {
        // Print (filename):(line number): (message).
        v8::String::Utf8Value filename(message->GetScriptOrigin().ResourceName());
        const char* filename_string = ::ToCString(filename);
        int linenum = message->GetLineNumber();
        fprintf(stderr, "%s:%i: %s\n", filename_string, linenum, exception_string);
        // Print line of source code.
        v8::String::Utf8Value sourceline(message->GetSourceLine());
        const char* sourceline_string = ::ToCString(sourceline);
        fprintf(stderr, "%s\n", sourceline_string);
        // Print wavy underline (GetUnderline is deprecated).
        int start = message->GetStartColumn();
        for (int i = 0; i < start; i++) {
            fprintf(stderr, " ");
        }
        int end = message->GetEndColumn();
        for (int i = start; i < end; i++) {
            fprintf(stderr, "^");
        }
        fprintf(stderr, "\n");
        v8::String::Utf8Value stack_trace(try_catch->StackTrace());
        if (stack_trace.length() > 0) {
            const char* stack_trace_string = ::ToCString(stack_trace);
            fprintf(stderr, "%s\n", stack_trace_string);
        }
    }
}

namespace Mordor {
namespace Test {

// Reads a file into a v8 string.
v8::Handle<v8::String> ReadFile(v8::Isolate* isolate, const char* name)
{
    try {
        Stream::ptr inStream(new FileStream(name, FileStream::READ));
        size_t size = inStream->size();

        Buffer buf;
        size_t readed = 0;
        size_t tmpsize = size;

        do{
            readed = inStream->read(buf, tmpsize);
            tmpsize -= readed;
        }while(readed);

        inStream->close();
        v8::Handle<v8::String> result = v8::String::NewFromUtf8(isolate, buf.toString().c_str(), v8::String::kNormalString, size);
        return result;
    } catch (...) {
        std::cerr << boost::current_exception_diagnostic_information() << std::endl;
        return v8::String::Empty(isolate);
    }
}

/**
 * ArrayBufferAllocator
 */
class ArrayBufferAllocator : public v8::ArrayBuffer::Allocator {
 public:
  // Impose an upper limit to avoid out of memory errors that bring down
  // the process.
  static const size_t kMaxLength = 0x3fffffff;
  static ArrayBufferAllocator the_singleton;
  virtual ~ArrayBufferAllocator() {}
  virtual void* Allocate(size_t length);
  virtual void* AllocateUninitialized(size_t length);
  virtual void Free(void* data, size_t length);
 private:
  ArrayBufferAllocator() {}
  ArrayBufferAllocator(const ArrayBufferAllocator&);
  void operator=(const ArrayBufferAllocator&);
};

ArrayBufferAllocator ArrayBufferAllocator::the_singleton;

void* ArrayBufferAllocator::Allocate(size_t length) {
  if (length > kMaxLength)
    return NULL;
  char* data = new char[length];
  memset(data, 0, length);
  return data;
}

void* ArrayBufferAllocator::AllocateUninitialized(size_t length) {
  if (length > kMaxLength)
    return NULL;
  return new char[length];
}

void ArrayBufferAllocator::Free(void* data, size_t length) {
  delete[] static_cast<char*>(data);
}

/**
 * MD_V8Wrapper
 */
bool MD_V8Wrapper::s_inited_ = false;
int MD_V8Wrapper::s_argc_ = 0;
char** MD_V8Wrapper::s_argv_ = NULL;
std::shared_ptr<MD_V8Wrapper> MD_V8Wrapper::s_curr_;
std::unique_ptr<v8::Platform> MD_V8Wrapper::s_platform_;
std::unique_ptr<MD_Worker> MD_V8Wrapper::s_worker_;

void MD_V8Wrapper::init(int argc, char** argv)
{
    if(MD_V8Wrapper::s_inited_) return;

    MD_V8Wrapper::s_inited_ = true;

    MD_V8Wrapper::s_argc_ = argc;
    MD_V8Wrapper::s_argv_ = argv;

    v8::V8::InitializeICU();
    MD_V8Wrapper::s_platform_.reset(Mordor::Platform::CreatePlatform(1));
    MD_V8Wrapper::s_worker_.reset(CreateWorker(4));

    v8::V8::InitializePlatform(MD_V8Wrapper::s_platform_.get());
    v8::V8::Initialize();
    v8::V8::SetFlagsFromCommandLine(&MD_V8Wrapper::s_argc_, MD_V8Wrapper::s_argv_, true);
//    v8::V8::SetFlagsFromString(g_harmony_opts, sizeof(g_harmony_opts) - 1);
    v8::V8::SetArrayBufferAllocator(&ArrayBufferAllocator::the_singleton);
}

void MD_V8Wrapper::shutdown()
{
    MD_V8Wrapper::s_curr_.reset();
    v8::V8::Dispose();
    v8::V8::ShutdownPlatform();
    MD_V8Wrapper::s_platform_.reset();
    MD_V8Wrapper::s_worker_.reset();
}

v8::Handle<v8::Context> MD_V8Wrapper::createContext()
{
    // Create a template for the global object.
    v8::Handle<v8::ObjectTemplate> global = v8::ObjectTemplate::New(isolate_);
    // Bind the global 'print' function to the C++ Print callback.
    global->Set(toV8String("print"), v8::FunctionTemplate::New(isolate_, MD_V8Wrapper::Print));
    // Bind the global 'read' function to the C++ Read callback.
    global->Set(toV8String("read"), v8::FunctionTemplate::New(isolate_, MD_V8Wrapper::Read));
    // Bind the global 'load' function to the C++ Load callback.
    global->Set(toV8String("load"), v8::FunctionTemplate::New(isolate_, MD_V8Wrapper::Load));
    // Bind the 'quit' function
    global->Set(toV8String("q"), v8::FunctionTemplate::New(isolate_, MD_V8Wrapper::Quit));
    // Bind the 'version' function
    global->Set(toV8String("v"), v8::FunctionTemplate::New(isolate_, MD_V8Wrapper::Version));

    context_ = v8::Context::New(isolate_, NULL, global);

    if (context_.IsEmpty()) {
        MORDOR_THROW_EXCEPTION(std::runtime_error("Error creating context"));
    }

    return context_;
}


bool MD_V8Wrapper::execString(const std::string& str, bool print_result, bool report_exceptions)
{
    return execString(str.c_str(), print_result, report_exceptions);
}

bool MD_V8Wrapper::execString(
        v8::Handle<v8::String> source,
        bool print_result,
        bool report_exceptions)
{
    v8::HandleScope handle_scope(isolate_);
    v8::TryCatch try_catch;
    v8::ScriptOrigin origin(toV8String(name_));
    v8::Handle<v8::Script> script = v8::Script::Compile(source, &origin);
    if (script.IsEmpty()) {
        // Print errors that happened during compilation.
        if (report_exceptions)
            ::ReportException(isolate_, &try_catch);
        return false;
    } else {
        v8::Handle<v8::Value> result = script->Run();
        if (result.IsEmpty()) {
            assert(try_catch.HasCaught());
            // Print errors that happened during execution.
            if (report_exceptions)
                ::ReportException(isolate_, &try_catch);
            return false;
        } else {
            assert(!try_catch.HasCaught());
            if (print_result && !result->IsUndefined()) {
                // If all went well and the result wasn't undefined then print
                // the returned value.
                v8::String::Utf8Value str(result);
                const char* cstr = ::ToCString(str);
                printf("%s\n", cstr);
            }
            return true;
        }
    }
}

// Executes a string within the current v8 context.
bool MD_V8Wrapper::execString(
        const char* source,
        bool print_result,
        bool report_exceptions)
{
    v8::Handle<v8::String> src = toV8String(source);
    return execString(src, print_result, report_exceptions);
}

// The callback that is invoked by v8 whenever the JavaScript 'print'
// function is called.  Prints its arguments on stdout separated by
// spaces and ending with a newline.
void MD_V8Wrapper::Print(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    bool first = true;
    for (int i = 0; i < args.Length(); i++) {
        v8::HandleScope handle_scope(args.GetIsolate());
        if (first) {
            first = false;
        } else {
            printf(" ");
        }
        v8::String::Utf8Value str(args[i]);
        const char* cstr = ::ToCString(str);
        printf("%s", cstr);
    }
    printf("\n");
    fflush(stdout);
}

// The callback that is invoked by v8 whenever the JavaScript 'read'
// function is called.  This function loads the content of the file named in
// the argument into a JavaScript string.
void MD_V8Wrapper::Read(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    v8::Isolate* isolate = args.GetIsolate();
    if (args.Length() != 1) {
        isolate->ThrowException(MD_V8Wrapper::toV8String(isolate, "Bad parameters"));
        return;
    }
    v8::String::Utf8Value file(args[0]);
    if (*file == NULL) {
        isolate->ThrowException(MD_V8Wrapper::toV8String(isolate, "Error loading file"));
        return;
    }
    v8::Handle<v8::String> source = ReadFile(isolate, *file);
    if (source.IsEmpty()) {
        isolate->ThrowException(MD_V8Wrapper::toV8String(isolate, "Error loading file"));
        return;
    }
    args.GetReturnValue().Set(source);
}

// The callback that is invoked by v8 whenever the JavaScript 'load'
// function is called.  Loads, compiles and executes its argument
// JavaScript file.
void MD_V8Wrapper::Load(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    v8::Isolate* isolate = args.GetIsolate();
    for (int i = 0; i < args.Length(); i++) {
        v8::HandleScope handle_scope(isolate);
        v8::String::Utf8Value file(args[i]);
        if (*file == NULL) {
            isolate->ThrowException(MD_V8Wrapper::toV8String(isolate, "Error loading file"));
            return;
        }
        v8::Handle<v8::String> source = ReadFile(isolate, *file);
        if (source.IsEmpty()) {
            isolate->ThrowException(MD_V8Wrapper::toV8String(isolate, "Error loading file"));
            return;
        }
        if (!MD_V8Wrapper::s_curr_->execString(source, false, false)) {
            isolate->ThrowException(MD_V8Wrapper::toV8String(isolate, "Error executing file"));
            return;
        }
    }
}

// The callback that is invoked by v8 whenever the JavaScript 'quit'
// function is called.  Quits.
void MD_V8Wrapper::Quit(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    // If not arguments are given args[0] will yield undefined which
    // converts to the integer value 0.
    // int exit_code = args[0]->Int32Value();
    fflush(stdout);
    fflush(stderr);
    MD_V8Wrapper::s_curr_->running_ = false;
}

static void co_version(MD_Task<const char*> &self)
{
    const char* ret = v8::V8::GetVersion();
    v8::Isolate* isolate = self.getIsolate();
    v8::Isolate::Scope isolate_scope(isolate);
    v8::HandleScope handle_scope(isolate);
    v8::Handle<v8::Context> context = MD_V8Wrapper::s_curr_->getContext();
//    v8::Context::Scope context_scope(context);
    v8::Handle<v8::String> s = MD_V8Wrapper::toV8String(isolate, "co_version");
    v8::String::Utf8Value str(s);
    const char* cstr = ::ToCString(str);
    printf("%s\n", cstr);
    self.setResult(ret);
}

void MD_V8Wrapper::Version(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    v8::Isolate* isolate = args.GetIsolate();
    MD_Task<const char*> task(isolate, &co_version);
    MD_V8Wrapper::s_worker_->doTask(&task);
    const char* result = task.getResult();
    v8::Handle<v8::Value> ret = MD_V8Wrapper::toV8String(isolate, result);
    args.GetReturnValue().Set(ret);
}

} } // namespace Mordor::Test
