#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <functional>

#include "md_runner.h"

#include "mordor/type_name.h"
#include "mordor/thread.h"
#include "mordor/fiber.h"
#include "mordor/assert.h"
#include "mordor/coroutine.h"
#include "mordor/streams/file.h"
#include "mordor/streams/std.h"

#include "v8.h"
#include "v8/include/libplatform/libplatform.h"
#include "md_v8_wrapper.h"

#include "md_env.h"
#include "md_env_inl.h"
#include "md_v8_util_inl.h"

#include "js_objects/process.h"

extern int g_argc;
extern char** g_argv;

#define MD_V8_OPTIONS  "--es_staging --harmony --use_strict"

namespace Mordor
{
namespace Test
{

LineEditor *LineEditor::current_ = NULL;

LineEditor::LineEditor(Type type, const char* name) :
        type_(type), name_(name)
{
    if (current_ == NULL || current_->type_ < type)
        current_ = this;
}

/**
 * ArrayBufferAllocator
 */
class ArrayBufferAllocator: public v8::ArrayBuffer::Allocator
{
public:
    // Impose an upper limit to avoid out of memory errors that bring down
    // the process.
    static const size_t kMaxLength = 0x3fffffff;
    static ArrayBufferAllocator the_singleton;
    virtual ~ArrayBufferAllocator()
    {
    }
    virtual void* Allocate(size_t length);
    virtual void* AllocateUninitialized(size_t length);
    virtual void Free(void* data, size_t length);
private:
    ArrayBufferAllocator()
    {
    }
    ArrayBufferAllocator(const ArrayBufferAllocator&);
    void operator=(const ArrayBufferAllocator&);
};

ArrayBufferAllocator ArrayBufferAllocator::the_singleton;

void* ArrayBufferAllocator::Allocate(size_t length)
{
    if (length > kMaxLength)
        return NULL;
    char* data = new char[length];
    memset(data, 0, length);
    return data;
}

void* ArrayBufferAllocator::AllocateUninitialized(size_t length)
{
    if (length > kMaxLength)
        return NULL;
    return new char[length];
}

void ArrayBufferAllocator::Free(void* data, size_t length)
{
    delete[] static_cast<char*>(data);
}

static void readScript(Coroutine<const char*>& self)
{
    LineEditor* console = LineEditor::Get();
    console->Open();
    std::string line;
    do {
        line = console->Prompt("> ");
        if (line.empty())
            continue;
        self.yield(line.c_str());
    } while (true);
}

static void AppendExceptionLine(Environment* env, v8::Handle<v8::Value> er, v8::Handle<v8::Message> message)
{
    if (message.IsEmpty())
        return;

    v8::HandleScope scope(env->isolate());
    v8::Local<v8::Object> err_obj;
    if (!er.IsEmpty() && er->IsObject()) {
        err_obj = er.As<v8::Object>();

        // Do it only once per message
        if (!err_obj->GetHiddenValue(env->processed_string()).IsEmpty())
            return;
        err_obj->SetHiddenValue(env->processed_string(), True(env->isolate()));
    }

    static char arrow[1024];

    // Print (filename):(line number): (message).
    v8::String::Utf8Value filename(message->GetScriptResourceName());
    const char* filename_string = *filename;
    int linenum = message->GetLineNumber();
    // Print line of source code.
    v8::String::Utf8Value sourceline(message->GetSourceLine());
    const char* sourceline_string = *sourceline;

    // Because of how node modules work, all scripts are wrapped with a
    // "function (module, exports, __filename, ...) {"
    // to provide script local variables.
    //
    // When reporting errors on the first line of a script, this wrapper
    // function is leaked to the user. There used to be a hack here to
    // truncate off the first 62 characters, but it caused numerous other
    // problems when vm.runIn*Context() methods were used for non-module
    // code.
    //
    // If we ever decide to re-instate such a hack, the following steps
    // must be taken:
    //
    // 1. Pass a flag around to say "this code was wrapped"
    // 2. Update the stack frame output so that it is also correct.
    //
    // It would probably be simpler to add a line rather than add some
    // number of characters to the first line, since V8 truncates the
    // sourceline to 78 characters, and we end up not providing very much
    // useful debugging info to the user if we remove 62 characters.

    int start = message->GetStartColumn();
    int end = message->GetEndColumn();

    int off = snprintf(arrow, sizeof(arrow), "%s:%i\n%s\n", filename_string, linenum, sourceline_string);
    assert(off >= 0);

    // Print wavy underline (GetUnderline is deprecated).
    for (int i = 0; i < start; i++) {
        if (sourceline_string[i] == '\0' || static_cast<size_t>(off) >= sizeof(arrow)) {
            break;
        }
        assert(static_cast<size_t>(off) < sizeof(arrow));
        arrow[off++] = (sourceline_string[i] == '\t') ? '\t' : ' ';
    }
    for (int i = start; i < end; i++) {
        if (sourceline_string[i] == '\0' || static_cast<size_t>(off) >= sizeof(arrow)) {
            break;
        }
        assert(static_cast<size_t>(off) < sizeof(arrow));
        arrow[off++] = '^';
    }
    assert(static_cast<size_t>(off - 1) <= sizeof(arrow) - 1);
    arrow[off++] = '\n';
    arrow[off] = '\0';

    v8::Local<v8::String> arrow_str = v8::String::NewFromUtf8(env->isolate(), arrow);
    v8::Local<v8::Value> msg;
    v8::Local<v8::Value> stack;

    // Allocation failed, just print it out
    if (arrow_str.IsEmpty() || err_obj.IsEmpty() || !err_obj->IsNativeError())
        goto print;

    msg = err_obj->Get(env->message_string());
    stack = err_obj->Get(env->stack_string());

    if (msg.IsEmpty() || stack.IsEmpty())
        goto print;

    err_obj->Set(env->message_string(), v8::String::Concat(arrow_str, msg->ToString()));
    err_obj->Set(env->stack_string(), v8::String::Concat(arrow_str, stack->ToString()));
    return;

    print: if (env->printed_error())
        return;
    env->set_printed_error(true);
//  uv_tty_reset_mode();
    fprintf(stderr, "\n%s", arrow);
}

static void ReportException(Environment* env, v8::Handle<v8::Value> er, v8::Handle<v8::Message> message)
{
    v8::HandleScope scope(env->isolate());

    AppendExceptionLine(env, er, message);

    v8::Local<v8::Value> trace_value;

    if (er->IsUndefined() || er->IsNull())
        trace_value = Undefined(env->isolate());
    else
        trace_value = er->ToObject()->Get(env->stack_string());

    v8::String::Utf8Value trace(trace_value);

    // range errors have a trace member set to undefined
    if (trace.length() > 0 && !trace_value->IsUndefined()) {
        fprintf(stderr, "%s\n", *trace);
    } else {
        // this really only happens for RangeErrors, since they're the only
        // kind that won't have all this info in the trace, or when non-Error
        // objects are thrown manually.
        v8::Local<v8::Value> message;
        v8::Local<v8::Value> name;

        if (er->IsObject()) {
            v8::Local<v8::Object> err_obj = er.As<v8::Object>();
            message = err_obj->Get(env->message_string());
            name = err_obj->Get(FIXED_UTF8_STRING(env->isolate(), "name"));
        }

        if (message.IsEmpty() || message->IsUndefined() || name.IsEmpty() || name->IsUndefined()) {
            // Not an error object. Just print as-is.
            v8::String::Utf8Value message(er);
            fprintf(stderr, "%s\n", *message);
        } else {
            v8::String::Utf8Value name_string(name);
            v8::String::Utf8Value message_string(message);
            fprintf(stderr, "%s: %s\n", *name_string, *message_string);
        }
    }

    fflush(stderr);
}

static void ReportException(Environment* env, const v8::TryCatch& try_catch)
{
    ReportException(env, try_catch.Exception(), try_catch.Message());
}

// Executes a str within the current v8 context.
static v8::Local<v8::Value> ExecuteString(Environment* env,
        v8::Handle<v8::String> source,
        v8::Handle<v8::String> filename,
        bool exitOnError = false)
{
    v8::Isolate* isolate = env->isolate();
    v8::EscapableHandleScope scope(isolate);
    v8::TryCatch try_catch;

    // try_catch must be nonverbose to disable FatalException() handler,
    // we will handle exceptions ourself.
    try_catch.SetVerbose(false);

    v8::Local<v8::Script> script = v8::Script::Compile(source, filename);
    if (script.IsEmpty()) {
        ReportException(env, try_catch);
        if(exitOnError)
            exit(3);
        return scope.Escape(v8::Undefined(isolate)->ToObject());
    }

    v8::Local<v8::Value> result = script->Run();
    if (result.IsEmpty()) {
        ReportException(env, try_catch);
        if(exitOnError)
            exit(4);
        return scope.Escape(v8::Undefined(isolate)->ToObject());
    }

    v8::String::Utf8Value str(result);
    const char* cstr = JSObjectUtils::toCString(str);
    std::cout << cstr << std::endl << std::flush;
    return scope.Escape(result);
}

MD_Runner::MD_Runner(Scheduler& sched) :
        sched_(sched)
{
    sched_.schedule(std::bind(&MD_Runner::run, this), gettid());
    this->wait();
}

MD_Runner::~MD_Runner()
{
}

void MD_Runner::run()
{
    fprintf(stderr, "V8 version %s [mordor shell]\n", v8::V8::GetVersion());

    v8::Platform* v8_platform = v8::platform::CreateDefaultPlatform(1);
    v8::V8::InitializeICU();
    v8::V8::InitializePlatform(v8_platform);
    v8::V8::Initialize();
    int argc = g_argc;
    v8::V8::SetFlagsFromCommandLine(&argc, g_argv, true);
    v8::V8::SetFlagsFromString(MD_V8_OPTIONS, sizeof(MD_V8_OPTIONS) - 1);
    v8::V8::SetArrayBufferAllocator(&ArrayBufferAllocator::the_singleton);

    v8::Isolate* isolate = v8::Isolate::New();
    {
        v8::Isolate::Scope isolate_scope(isolate);
        v8::Locker locker(isolate);
        v8::HandleScope handle_scope(isolate);
        v8::Local<v8::Context> context = v8::Context::New(isolate);
        v8::Context::Scope context_scope(context);
        Environment* env = Environment::New(context, Scheduler::getThis());

        ProcessObject po(env);
        po.setup();
        {
            Coroutine<const char*> coReadScript(&readScript);
            const char* script;
            bool running = true;
            do {
                script = coReadScript.call();
                if (coReadScript.state() == Fiber::State::TERM) {
                    break;
                }
                v8::HandleScope handle_scope(context->GetIsolate());
                v8::Local<v8::String> script_str = Utf8String(isolate, script);
                ExecuteString(env, script_str, Utf8String(isolate, "md_shell"));
                running = env->running();
            } while (running);
            std::cout << "bye." << std::endl;
        }
        Environment::environment.reset();
    }
    isolate->Dispose();

    LineEditor* line_editor = LineEditor::Get();
    if (line_editor)
        line_editor->Close();

    v8::V8::Dispose();
    v8::V8::ShutdownPlatform();
    delete v8_platform;

    this->over();
}

}
}  // namespace Mordor::Test
