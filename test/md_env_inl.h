#ifndef MORDOR_V8_ENV_INL_H_
#define MORDOR_V8_ENV_INL_H_

#include "mordor/iomanager.h"

#include "md_env.h"
#include "md_worker.h"

namespace Mordor
{
namespace Test
{

inline Environment::IsolateData* Environment::IsolateData::Get(v8::Isolate* isolate)
{
    return static_cast<IsolateData*>(isolate->GetData(kIsolateSlot));
}

inline Environment::IsolateData* Environment::IsolateData::GetOrCreate(v8::Isolate* isolate)
{
    IsolateData* isolate_data = Get(isolate);
    if (isolate_data == NULL) {
        isolate_data = new IsolateData(isolate);
        isolate->SetData(kIsolateSlot, isolate_data);
    }
    isolate_data->ref_count_ += 1;
    return isolate_data;
}

inline void Environment::IsolateData::Put()
{
    if (--ref_count_ == 0) {
        isolate()->SetData(kIsolateSlot, NULL);
        delete this;
    }
}

inline Environment::IsolateData::IsolateData(v8::Isolate* isolate) :
        isolate_(isolate),
#define V(PropertyName, StringValue)                                          \
    PropertyName ## _(isolate, FIXED_UTF8_STRING(isolate, StringValue)),
    PER_ISOLATE_STRING_PROPERTIES(V)
#undef V
        ref_count_(0)
{
}

inline v8::Isolate* Environment::IsolateData::isolate() const
{
    return isolate_;
}

inline Environment* Environment::New(v8::Local<v8::Context> context, Scheduler* scheduer)
{
    Environment::environment.reset(new Environment(context), Environment::EnvironmentDeleter());
    Environment::environment->AssignToContext(context);
    Environment::environment->worker_.reset(MD_Worker::New(scheduer, kWorkerPoolSize));
    return Environment::environment.get();
}

inline void Environment::AssignToContext(v8::Local<v8::Context> context)
{
    context->SetAlignedPointerInEmbedderData(kContextEmbedderDataIndex, this);
}

inline Environment* Environment::GetCurrent(v8::Isolate* isolate)
{
    return GetCurrent(isolate->GetCurrentContext());
}

inline Environment* Environment::GetCurrent(v8::Local<v8::Context> context)
{
    return static_cast<Environment*>(context->GetAlignedPointerFromEmbedderData(kContextEmbedderDataIndex));
}

inline MD_Worker* Environment::GetCurrentWorker(v8::Isolate* isolate)
{
    return Environment::GetCurrent(isolate)->worker();

}

inline MD_Worker* Environment::GetCurrentWorker(v8::Local<v8::Context> context)
{
    return Environment::GetCurrent(context)->worker();
}

inline Environment::Environment(v8::Local<v8::Context> context) :
        isolate_(context->GetIsolate()),
        isolate_data_(IsolateData::GetOrCreate(context->GetIsolate())),
        using_smalloc_alloc_cb_(false),
        using_domains_(false),
        printed_error_(false),
        running_(true),
        context_(context->GetIsolate(), context)
{
    // We'll be creating new objects so make sure we've entered the context.
    v8::HandleScope handle_scope(isolate());
    v8::Context::Scope context_scope(context);
    set_binding_cache_object(v8::Object::New(isolate()));
    set_module_load_list_array(v8::Array::New(isolate()));
}

inline Environment::~Environment()
{
    v8::HandleScope handle_scope(isolate());

    context()->SetAlignedPointerInEmbedderData(kContextEmbedderDataIndex, NULL);
#define V(PropertyName, TypeName) PropertyName ## _.Reset();
    ENVIRONMENT_STRONG_PERSISTENT_PROPERTIES(V)
#undef V
    isolate_data()->Put();
}

inline void Environment::Dispose()
{
    delete this;
}

inline v8::Isolate* Environment::isolate() const
{
    return isolate_;
}

inline Environment::IsolateData* Environment::isolate_data() const
{
    return isolate_data_;
}

inline bool Environment::using_smalloc_alloc_cb() const
{
    return using_smalloc_alloc_cb_;
}

inline void Environment::set_using_smalloc_alloc_cb(bool value)
{
    using_smalloc_alloc_cb_ = value;
}

inline bool Environment::using_domains() const
{
    return using_domains_;
}

inline void Environment::set_using_domains(bool value)
{
    using_domains_ = value;
}

inline bool Environment::printed_error() const
{
    return printed_error_;
}

inline void Environment::set_printed_error(bool value)
{
    printed_error_ = value;
}

template <typename V8ERROR>
inline static void ThrowErrorTmpl(V8ERROR err, v8::Isolate* isolate){
    v8::HandleScope scope(isolate);
    isolate->ThrowException(err);
}

inline void Environment::ThrowError(v8::Isolate* isolate, const char* errmsg) {
    ThrowErrorTmpl(v8::Exception::Error(OneByteString(isolate, errmsg)), isolate);
}

inline void Environment::ThrowTypeError(v8::Isolate* isolate,
                                        const char* errmsg) {
  ThrowErrorTmpl(v8::Exception::TypeError(OneByteString(isolate, errmsg)), isolate);
}

inline void Environment::ThrowRangeError(v8::Isolate* isolate,
                                         const char* errmsg) {
    ThrowErrorTmpl(v8::Exception::RangeError(OneByteString(isolate, errmsg)), isolate);
}

inline void Environment::ThrowError(const char* errmsg) {
    ThrowError(this->isolate(), errmsg);
}

inline void Environment::ThrowTypeError(const char* errmsg) {
    ThrowTypeError(this->isolate(), errmsg);
}

inline void Environment::ThrowRangeError(const char* errmsg) {
    ThrowRangeError(this->isolate(), errmsg);
}

inline void Environment::ThrowErrnoException(int errorno,
                                             const char* syscall,
                                             const char* message,
                                             const char* path) {
    isolate()->ThrowException(
      ErrnoException(this->isolate(), errorno, syscall, message, path)
    );
}

/*****************************************************************************
 *
 */
#define V(PropertyName, StringValue)                                          \
  inline                                                                      \
  v8::Local<v8::String> Environment::IsolateData::PropertyName() const {      \
    /* Strings are immutable so casting away const-ness here is okay. */      \
    return const_cast<IsolateData*>(this)->PropertyName ## _.Get(isolate());  \
  }
PER_ISOLATE_STRING_PROPERTIES(V)
#undef V

#define V(PropertyName, StringValue)                                          \
  inline v8::Local<v8::String> Environment::PropertyName() const {            \
    return isolate_data()->PropertyName();                                    \
  }
PER_ISOLATE_STRING_PROPERTIES(V)
#undef V

#define V(PropertyName, TypeName)                                             \
  inline v8::Local<TypeName> Environment::PropertyName() const {              \
    return StrongPersistentToLocal(PropertyName ## _);                        \
  }                                                                           \
  inline void Environment::set_ ## PropertyName(v8::Local<TypeName> value) {  \
    PropertyName ## _.Reset(isolate(), value);                                \
  }
ENVIRONMENT_STRONG_PERSISTENT_PROPERTIES(V)
#undef V

#undef ENVIRONMENT_STRONG_PERSISTENT_PROPERTIES
#undef PER_ISOLATE_STRING_PROPERTIES

}
} // Mordor::Test

#endif // MORDOR_V8_ENV_INL_H_
