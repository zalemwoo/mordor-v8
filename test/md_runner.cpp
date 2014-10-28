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

extern int g_argc;
extern char** g_argv;

#define MD_V8_OPTIONS  "--es_staging --harmony --use_strict"

namespace Mordor
{
namespace Test
{
bool g_running = true;

LineEditor *LineEditor::current_ = NULL;

LineEditor::LineEditor(Type type, const char* name)
    : type_(type), name_(name) {
  if (current_ == NULL || current_->type_ < type) current_ = this;
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

static void readScript(Coroutine<const char*>& self)
{
    LineEditor* console = LineEditor::Get();
    console->Open();
    std:: string line;
    do {
        line = console->Prompt("> ");
        if (line.empty())
            continue;
        self.yield(line.c_str());
    } while (true);
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

    v8::Platform* v8_platform =  v8::platform::CreateDefaultPlatform(1);
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
        v8::Local<v8::Context> context = MD_V8Wrapper::createContext(isolate);
        v8::Context::Scope context_scope(context);
        Environment* env = Environment::New(context, Scheduler::getThis());
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
                MD_V8Wrapper::execString(env, script, true, true);
                running = g_running;
            } while (running);
            std::cout<< "bye." << std::endl;
        }
        Environment::environment.reset();
    }
    isolate->Dispose();

    LineEditor* line_editor = LineEditor::Get();
    if (line_editor) line_editor->Close();

    v8::V8::Dispose();
    v8::V8::ShutdownPlatform();
    delete v8_platform;

    this->over();
}

} }  // namespace Mordor::Test
