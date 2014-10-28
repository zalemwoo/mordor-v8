
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
#include "md_env.h"

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
extern bool g_running;

// Reads a file into a v8 string.
v8::Local<v8::String> ReadFile(v8::Isolate* isolate, const char* name)
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
        v8::Local<v8::String> result = v8::String::NewFromUtf8(isolate, buf.toString().c_str(), v8::String::kNormalString, size);
        return result;
    } catch (...) {
        return v8::String::Empty(isolate);
    }
}

v8::Handle<v8::Context> MD_V8Wrapper::createContext(v8::Isolate* isolate)
{
    v8::Handle<v8::Context> context;
    // Create a template for the global object.
    v8::Handle<v8::ObjectTemplate> global = v8::ObjectTemplate::New(isolate);
    // Bind the global 'print' function to the C++ Print callback.
    global->Set(toV8String(isolate, "p"), v8::FunctionTemplate::New(isolate, MD_V8Wrapper::Print));
    // Bind the global 'read' function to the C++ Read callback.
    global->Set(toV8String(isolate, "read"), v8::FunctionTemplate::New(isolate, MD_V8Wrapper::Read));
    // Bind the global 'load' function to the C++ Load callback.
    global->Set(toV8String(isolate, "load"), v8::FunctionTemplate::New(isolate, MD_V8Wrapper::Load));
    // Bind the 'version' function
    global->Set(toV8String(isolate, "v"), v8::FunctionTemplate::New(isolate, MD_V8Wrapper::Version));

    global->Set(toV8String(isolate, "e"), v8::FunctionTemplate::New(isolate, MD_V8Wrapper::Exception));

    context = v8::Context::New(isolate, NULL, global);

    if (context.IsEmpty()) {
        MORDOR_THROW_EXCEPTION(std::runtime_error("Error creating context"));
    }

    return context;
}

void co_execString(MD_Task<bool(TASK_V8)> &self, v8::Handle<v8::String> source)
{
    v8::Isolate* isolate = self.isolate();
    v8::TryCatch try_catch;
    v8::ScriptOrigin origin(MD_V8Wrapper::toV8String(isolate, "md_shell"));
    v8::Handle<v8::Script> script = v8::Script::Compile(source, &origin);
    if (script.IsEmpty()) {
        ::ReportException(isolate, &try_catch);
        self.setResult(false);
        return;
    } else {
        v8::Handle<v8::Value> result = script->Run();
        if (result.IsEmpty()) {
            assert(try_catch.HasCaught());
            // Print errors that happened during executio
            ::ReportException(isolate, &try_catch);
            self.setResult(false);
            return;
        } else {
            assert(!try_catch.HasCaught());
            if (!result->IsUndefined()) {
                // If all went well and the result wasn't undefined then print
                // the returned value.
                v8::String::Utf8Value str(result);
                const char* cstr = ::ToCString(str);
                printf("%s\n", cstr);
            }
            self.setResult(true);
            return;
        }
    }
}

bool MD_V8Wrapper::execString(
        Environment* env,
        const std::string& str,
        bool print_result,
        bool report_exceptions)
{
    return execString(env, str.c_str(), print_result, report_exceptions);
}

bool MD_V8Wrapper::execString(
        Environment* env,
        v8::Handle<v8::String> source,
        bool print_result,
        bool report_exceptions)
{
    bool result;
    env->worker()->doTask<bool,TASK_V8>(env->context(), std::bind(&co_execString, std::placeholders::_1, source), result);
    return result;
}

// Executes a string within the current v8 context.
bool MD_V8Wrapper::execString(
        Environment* env,
        const char* source,
        bool print_result,
        bool report_exceptions)
{
    v8::Handle<v8::String> src = toV8String(env->isolate(), source);
    return execString(env, src, print_result, report_exceptions);
}

static void co_print(MD_Task<void(TASK)> &self, const std::vector<std::string>& args)
{
    bool first = true;

    std::vector<std::string>::const_iterator it = args.begin();
    for (;it != args.end(); it++) {
        if (first) {
            first = false;
        } else {
            std::cout << " ";
        }
        std::cout << *it;
    }
    std::cout << std::endl << std::flush;;
}

// The callback that is invoked by v8 whenever the JavaScript 'print'
// function is called.  Prints its arguments on stdout separated by
// spaces and ending with a newline.
void MD_V8Wrapper::Print(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    v8::Isolate* isolate = args.GetIsolate();
    std::vector<std::string> strs;
    for (int i = 0; i < args.Length(); i++) {
           v8::HandleScope handle_scope(isolate);
           v8::String::Utf8Value str(args[i]);
           const char* cstr = ::ToCString(str);
           strs.push_back(std::string(cstr));
       }
    Environment::GetCurrentWorker(isolate)->doTask<void, TASK>(std::bind(&co_print, std::placeholders::_1, std::cref(strs)));
}

static void co_read(MD_Task<v8::Local<v8::String>(TASK_V8)> &self, const v8::FunctionCallbackInfo<v8::Value>& args)
{
    v8::Isolate* isolate = self.isolate();
    Environment* env = Environment::GetCurrent(isolate);
    if (args.Length() != 1) {
        env->ThrowError("Bad parameters");
        self.setResult(v8::String::Empty(isolate));
        return;
    }
    v8::String::Utf8Value file(args[0]);
    if (*file == NULL) {
        env->ThrowError("Error loading file");
        self.setResult(v8::String::Empty(isolate));
        return;
    }
    v8::Local<v8::String> source = ReadFile(isolate, *file);
    if (source.IsEmpty()) {
        env->ThrowError("Error loading file");
        self.setResult(v8::String::Empty(isolate));
        return;
    }

    self.setResult(source);
}

void MD_V8Wrapper::Read(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::String> source;
    Environment* env = Environment::GetCurrent(isolate);
    env->worker()->doTask<v8::Local<v8::String>, TASK_V8>(env->context(), std::bind(&co_read, std::placeholders::_1, args), source);
    args.GetReturnValue().Set(source);
}

void MD_V8Wrapper::Load(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    v8::Isolate* isolate = args.GetIsolate();
    Environment* env = Environment::GetCurrent(isolate);
    for (int i = 0; i < args.Length(); i++) {
        v8::HandleScope handle_scope(isolate);
        v8::String::Utf8Value file(args[i]);
        if (*file == NULL) {
            env->ThrowError("Error loading file");
            return;
        }
        v8::Local<v8::String> source;
        env->worker()->doTask<v8::Local<v8::String>, TASK_V8>(env->context(), std::bind(&co_read, std::placeholders::_1, args), source);

        if (source.IsEmpty()) {
            env->ThrowError("Error loading file");
            return;
        }
        if (!MD_V8Wrapper::execString(Environment::GetCurrent(isolate), source, false, false)) {
            env->ThrowError("Error executing file");
            return;
        }
    }
}

static void co_version(MD_Task<const char*(TASK)> &self)
{
    const char* ret = v8::V8::GetVersion();
    self.setResult(ret);
}

void MD_V8Wrapper::Version(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    v8::Isolate* isolate = args.GetIsolate();
    const char* result;
    Environment::GetCurrentWorker(isolate)->doTask<const char*,TASK>(&co_version, result);
    v8::Handle<v8::Value> ret = MD_V8Wrapper::toV8String(isolate, result);
    args.GetReturnValue().Set(ret);
}

static void co_exception(MD_Task<void(TASK_V8)> &self, int err)
{
    v8::TryCatch try_catch;
    v8::Isolate* isolate = self.isolate();
    Environment* env = Environment::GetCurrent(isolate);
    env->ThrowErrnoException(err);
    ::ReportException(isolate, &try_catch);
}

void MD_V8Wrapper::Exception(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    v8::Isolate* isolate = args.GetIsolate();
    Environment* env = Environment::GetCurrent(isolate);
    if(args.Length() != 1){
        env->ThrowRangeError("arguments length must be 1.");
        args.GetReturnValue().Set(v8::Undefined(isolate));
    }

    v8::Integer* arg = v8::Integer::Cast(*args[0]);
    int err = arg->Int32Value();
    env->worker()->doTask<void,TASK_V8>(env->context(), std::bind(&co_exception, std::placeholders::_1, err));
    args.GetReturnValue().Set(v8::Undefined(isolate));
}

} } // namespace Mordor::Test
