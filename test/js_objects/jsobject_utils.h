
#ifndef MD_JSOBJECT_UTILS_H_
#define MD_JSOBJECT_UTILS_H_

#include "v8.h"
#include "md_v8_util_inl.h"
#include "md_env_inl.h"

namespace Mordor
{
namespace Test
{

class JSObjectUtils
{
public:

    static void setReadOnlyProperty(v8::Handle<v8::Object> object, v8::Local<v8::String> name, v8::Handle<v8::Value> value){
        object->ForceSet(name, value, v8::ReadOnly);
    }
    static void setReadOnlyProperty(v8::Isolate* isolate, v8::Handle<v8::Object> object, const char* name, v8::Handle<v8::Value> value){
        setReadOnlyProperty(object, OneByteString(isolate, name), value );
    }
    static void setReadOnlyProperty(Environment* env, v8::Handle<v8::Object> object, const char* name, v8::Handle<v8::Value> value){
        setReadOnlyProperty(env->isolate(), object, name, value);
    }

    static void setProperty(v8::Handle<v8::Object> object, v8::Local<v8::String> name, v8::Handle<v8::Value> value){
        object->Set(name, value);
    }
    static void setProperty(v8::Isolate* isolate, v8::Handle<v8::Object> object, const char* name, v8::Handle<v8::Value> value){
        setProperty(object, OneByteString(isolate, name), value);
    }
    static void setProperty(Environment* env, v8::Handle<v8::Object> object, const char* name, v8::Handle<v8::Value> value){
        setProperty(env->isolate(), object, name, value);
    }

    template <typename TypeName>
    static void setMethod(
            const TypeName& object,
            v8::Isolate* isolate,
            const char* name,
            v8::FunctionCallback callback)
    {
      v8::HandleScope handle_scope(isolate);
      v8::Local<v8::FunctionTemplate> t = v8::FunctionTemplate::New(isolate,
                                                                    callback);
      v8::Local<v8::Function> fn = t->GetFunction();
      v8::Local<v8::String> fn_name = v8::String::NewFromUtf8(isolate, name);
      fn->SetName(fn_name);
      object->Set(fn_name, fn);
    }

    template <typename TypeName>
    static void setMethod(
            const TypeName& object,
            Environment* env,
            const char* name,
            v8::FunctionCallback callback)
    {
        setMethod(object, env->isolate(), name, callback);
    }

    static const char* toCString(v8::Local<v8::String> value){
        v8::String::Utf8Value val(value);
        return toCString(val);
    }
    static const char* toCString(const v8::String::Utf8Value& value)
    {
        return *value ? *value : "<string conversion failed>";
    }

};

} } // namespace Mordor::Test

#endif // MD_JSOBJECT_UTILS_H_
