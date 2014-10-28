
#include <unistd.h>
#include <stdlib.h>

#include "mordor/version.h"

#include "process.h"
#include "jsobject_utils.h"

namespace Mordor
{
namespace Test
{

static void EnvGetter(v8::Local<v8::String> property,
        const v8::PropertyCallbackInfo<v8::Value>& info)
{
    Environment* env = Environment::GetCurrent(info.GetIsolate());
    v8::HandleScope scope(env->isolate());
#ifdef POSIX
    v8::String::Utf8Value key(property);
    const char* val = getenv(*key);
    if (val) {
        return info.GetReturnValue().Set(v8::String::NewFromUtf8(env->isolate(), val));
    }
#endif
    // Not found.  Fetch from prototype.
    info.GetReturnValue().Set(info.Data().As<v8::Object>()->Get(property));
}

static void EnvSetter(v8::Local<v8::String> property,
        v8::Local<v8::Value> value,
        const v8::PropertyCallbackInfo<v8::Value>& info)
{
    Environment* env = Environment::GetCurrent(info.GetIsolate());
    v8::HandleScope scope(env->isolate());
#ifdef POSIX
    v8::String::Utf8Value key(property);
    v8::String::Utf8Value val(value);
    setenv(*key, *val, 1);
#endif
    // Whether it worked or not, always return rval.
    info.GetReturnValue().Set(value);
}

static void EnvQuery(v8::Local<v8::String> property,
        const v8::PropertyCallbackInfo<v8::Integer>& info)
{
    Environment* env = Environment::GetCurrent(info.GetIsolate());
    v8::HandleScope scope(env->isolate());
    int32_t rc = -1;  // Not found unless proven otherwise.
#ifdef POSIX
    v8::String::Utf8Value key(property);
    if (getenv(*key))
        rc = 0;
#endif
    if (rc != -1)
        info.GetReturnValue().Set(rc);
}

static void EnvDeleter(v8::Local<v8::String> property,
        const v8::PropertyCallbackInfo<v8::Boolean>& info)
{
    Environment* env = Environment::GetCurrent(info.GetIsolate());
    v8::HandleScope scope(env->isolate());
    bool rc = true;
#ifdef POSIX
    v8::String::Utf8Value key(property);
    rc = (getenv(*key) != NULL);
    if (rc)
        unsetenv(*key);
#endif
    info.GetReturnValue().Set(rc);
}

static void EnvEnumerator(const v8::PropertyCallbackInfo<v8::Array>& info)
{
    Environment* env = Environment::GetCurrent(info.GetIsolate());
    v8::HandleScope scope(env->isolate());
#ifdef POSIX
    int size = 0;
    while (environ[size])
        size++;

    v8::Local<v8::Array> envarr = v8::Array::New(env->isolate(), size);

    for (int i = 0; i < size; ++i) {
        const char* var = environ[i];
        const char* s = strchr(var, '=');
        const int length = s ? s - var : strlen(var);
        v8::Local<v8::String> name = v8::String::NewFromUtf8(env->isolate(), var, v8::String::kNormalString, length);
        envarr->Set(i, name);
    }
#endif
    info.GetReturnValue().Set(envarr);
}

static void Exit(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    Environment* env = Environment::GetCurrent(args.GetIsolate());
    env->set_return_value(args[0]->Int32Value());
    env->set_running(false);
}

static void MemoryUsage(const v8::FunctionCallbackInfo<v8::Value>& args) {
  Environment* env = Environment::GetCurrent(args.GetIsolate());
  v8::HandleScope scope(env->isolate());
  // V8 memory usage
  v8::HeapStatistics v8_heap_stats;
  env->isolate()->GetHeapStatistics(&v8_heap_stats);

  v8::Local<v8::Integer> physical_total =
          v8::Integer::NewFromUnsigned(env->isolate(), v8_heap_stats.total_physical_size());
  v8::Local<v8::Integer> heap_total =
          v8::Integer::NewFromUnsigned(env->isolate(), v8_heap_stats.total_heap_size());
  v8::Local<v8::Integer> heap_total_exec =
          v8::Integer::NewFromUnsigned(env->isolate(), v8_heap_stats.total_heap_size_executable());
  v8::Local<v8::Integer> heap_used =
          v8::Integer::NewFromUnsigned(env->isolate(), v8_heap_stats.used_heap_size());
  v8::Local<v8::Integer> heap_limit =
          v8::Integer::NewFromUnsigned(env->isolate(), v8_heap_stats.heap_size_limit());

  v8::Local<v8::Object> info = v8::Object::New(env->isolate());
  info->Set(env->physical_total_string(), physical_total);
  info->Set(env->heap_total_string(), heap_total);
  info->Set(env->heap_total_exec_string(), heap_total_exec);
  info->Set(env->heap_used_string(), heap_used);
  info->Set(env->heap_limit_string(), heap_limit);

  args.GetReturnValue().Set(info);
}

void ProcessObject::setup()
{
    v8::HandleScope handleScope(isolate_);
    // process.version
    setReadOnlyProperty("version", OneByteString(isolate_, "0.0.1"));
    // process.versions
    v8::Local<v8::Object> versions = v8::Object::New(isolate_);
    setReadOnlyProperty("versions", versions);
    JSObjectUtils::setReadOnlyProperty(isolate_, versions, "v8", OneByteString(isolate_, v8::V8::GetVersion()));
    // process.arch
    setReadOnlyProperty("arch", OneByteString(isolate_, ARCH));
    // process.platform
#if defined(WINDOWS)
# define PLAT "win32"  // windows -> win32
#else
# define PLAT PLATFORM
#endif
    setReadOnlyProperty("platform", OneByteString(isolate_, PLAT));
#undef PLAT
    // process.env
    v8::Local<v8::ObjectTemplate> process_env_template = v8::ObjectTemplate::New(isolate_);
    process_env_template->SetNamedPropertyHandler(EnvGetter,
            EnvSetter,
            EnvQuery,
            EnvDeleter,
            EnvEnumerator,
            v8::Object::New(isolate_)
    );
    v8::Local<v8::Object> process_env = process_env_template->NewInstance();
    object_->Set(env_->env_string(), process_env);
    // process.pid
    setReadOnlyProperty("pid", v8::Integer::New(isolate_, getpid()));
    // process.quit()
    setMethod("exit", Exit);
    setMethod("reallyExit", Exit);
    // process.memoryUsage()
    setMethod("memoryUsage", MemoryUsage);

    setToGlobal();
    env_->set_process_object(object_);
}

} } // namespace Mordor::Test
