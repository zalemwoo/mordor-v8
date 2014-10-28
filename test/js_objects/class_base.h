#ifndef MD_JSOBJECT_CLASS_BASE_H_
#define MD_JSOBJECT_CLASS_BASE_H_

#include "v8.h"
#include "jsobject_utils.h"

#include "string.h" // for strlen()

namespace Mordor
{
namespace Test
{

class ClassBase
{
public:
    ClassBase(Environment* env, const char* name) :
            env_(env)
    {
        isolate_ = env->isolate();
        object_tmpl_ = v8::FunctionTemplate::New(isolate_);
        name_ = OneByteString(isolate_, name);
        setClassName(name);
        object_ = newInstance();
    }

    virtual ~ClassBase()
    {
    }

    void setClassName(const char* name)
    {
        object_tmpl_->SetClassName(Utf8String(isolate_, name, strlen(name)));
    }

    v8::Local<v8::Object> newInstance()
    {
        return object_tmpl_->GetFunction()->NewInstance();
    }

    void setReadOnlyProperty(const char* name, v8::Handle<v8::Value> value)
    {
        JSObjectUtils::setReadOnlyProperty(isolate_, object_, name, value);
    }

    void setProperty(const char* name, v8::Handle<v8::Value> value)
    {
        JSObjectUtils::setProperty(isolate_, object_, name, value);
    }

    void setMethod(const char* name, v8::FunctionCallback callback)
    {
        JSObjectUtils::setMethod(object_, isolate_, name, callback);
    }

    void setTo(v8::Local<v8::Object> target, v8::Local<v8::String> name)
    {
        JSObjectUtils::setReadOnlyProperty(target, name, object_);
    }

    void setTo(v8::Local<v8::Object> target)
    {
        setTo(target, name_);
    }

    void setToGlobal(v8::Local<v8::String> name)
    {
        v8::Local<v8::Object> global = env_->context()->Global();
        setTo(global, name);
    }

    void setToGlobal()
    {
        setToGlobal(name_);
    }

    virtual void setup() = 0;

    const char* name_cstr()
    {
        return JSObjectUtils::toCString(name_);
    }

    v8::Local<v8::String> name()
    {
        return name_;
    }

protected:
    v8::Local<v8::FunctionTemplate> object_tmpl_;
    v8::Local<v8::Object> object_;
    Environment* env_;
    v8::Isolate* isolate_;
    v8::Local<v8::String> name_;
};

} } // namespace Mordor::Test

#endif // MD_JSOBJECT_CLASS_BASE_H_
