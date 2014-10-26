#ifndef MORDOR_CO_TASK_H_
#define MORDOR_CO_TASK_H_

#include "mordor/assert.h"
#include "mordor/util.h"
#include "mordor/coroutine.h"
#include "mordor/semaphore.h"
#include "mordor/fibersynchronization.h"

#include "v8.h"
#include "v8_persistent_wrapper.h"
#include "md_v8_wrapper.h"

namespace Mordor
{
namespace Test
{

struct DummyVoid;

struct MdTaskAbortedException: virtual OperationAbortedException
{
};
template<typename TaskSignature> class MD_Task;

template<typename Result>
class TaskResult : Mordor::noncopyable
{
public:
    void set(const Result &val)
    {
        val_ = val;
    }

    Result get()
    {
        return val_;
    }
protected:
    Result val_;
};

template<>
class TaskResult<void>: Mordor::noncopyable
{
};

template<typename T>
class TaskResult<v8::Local<T>> : Mordor::noncopyable
{
public:
    void set(const v8::Local<T> &val)
    {
        val_ = PersistentHandleWrapper<T>(v8::Isolate::GetCurrent(), val);
    }

    v8::Local<T> get()
    {
        return val_.Extract();
    }
protected:
    PersistentHandleWrapper<T> val_;
};

template<typename TaskSignature> class TaskCallback;

template<typename Result, typename... ARGS>
class TaskCallback<Result(ARGS...)> : Mordor::noncopyable
{
public:
    typedef MD_Task<Result(ARGS...)> TaskType;
    typedef std::function<void(TaskType&)> CallbackType;
    TaskCallback(TaskType& task, CallbackType dg):task_ref(task), dg_(dg){}

    void run(){
        dg_(task_ref);
    }

private:
    TaskType& task_ref;
    CallbackType dg_;
};

template<typename Result>
class TaskCallback<Result(const v8::FunctionCallbackInfo<v8::Value>&)> : Mordor::noncopyable
{
public:
    typedef MD_Task<Result(const v8::FunctionCallbackInfo<v8::Value>&)> TaskType;
    typedef std::function<void(TaskType&)> CallbackType;
    TaskCallback(TaskType& task, CallbackType dg):task_ref(task), dg_(dg){}

    void setIsolate(v8::Isolate* isolate){
        this->isolate_ = isolate;
    }

    void run(){
        MORDOR_ASSERT(this->isolate_ != NULL);
        v8::Locker locker(isolate_);
        v8::Isolate::Scope isolate_scope(isolate_);
        v8::HandleScope handle_scope(isolate_);
        v8::Local<v8::Context> context = MD_V8Wrapper::s_curr_->getContext();
        v8::Context::Scope context_scope(context);
        dg_(task_ref);
    }

private:
    TaskType& task_ref;
    CallbackType dg_;
    v8::Isolate* isolate_{NULL};
};

class Task : Mordor::noncopyable
{
public:
    virtual ~Task(){}

    void Call()
    {
        try {
            this->run();
        } catch (MdTaskAbortedException &) {
        }
        setEvent();
    }

    virtual void waitEvent()
    {
        event_.wait();
    }
protected:
    virtual void run() = 0;

    void setEvent()
    {
        event_.set();
    }

protected:
    Mordor::FiberEvent event_ { false };
};


class Task_V8 : public Task
{
public:
    Task_V8(v8::Isolate* isolate): Task(), isolate_(isolate){}
    virtual ~Task_V8(){}

    v8::Isolate* getIsolate(){
        return isolate_;
    }

    virtual void waitEvent() override
    {
        v8::Unlocker unlocker(isolate_);
        Task::waitEvent();
    }

protected:
    v8::Isolate* isolate_;
};

template<typename Result, typename... ARGS>
class MD_Task<Result(ARGS...)> : public Task
{
public:
    typedef typename TaskCallback<Result(ARGS...)>::CallbackType CallbackType;
public:
    MD_Task(CallbackType dg) : Task(), dg_(*this, dg)
    {
    }

    ~MD_Task(){}

    template<typename R = Result>
    void setResult(const R &result){
        result_.set(result);
    }
    template<typename R = Result>
    R getResult(){
        return result_.get();
    }

protected:
    virtual void run()
    {
        dg_.run();
    }
protected:
    TaskCallback<Result(ARGS...)> dg_;
    TaskResult<Result> result_;
};


template<typename Result>
class MD_Task<Result(const v8::FunctionCallbackInfo<v8::Value>&)> : public Task_V8
{
public:
    typedef typename TaskCallback<Result(const v8::FunctionCallbackInfo<v8::Value>&)>::CallbackType CallbackType;
public:
    MD_Task(v8::Isolate* isolate, CallbackType dg) : Task_V8(isolate), dg_(*this, dg)
    {
        dg_.setIsolate(isolate);
    }

    ~MD_Task(){}

    template<typename R = Result>
    void setResult(const R &result){
        result_.set(result);
    }
    template<typename R = Result>
    R getResult(){
        return result_.get();
    }

protected:
    virtual void run()
    {
        dg_.run();
    }
protected:
    TaskCallback<Result(const v8::FunctionCallbackInfo<v8::Value>&)> dg_;
    TaskResult<Result> result_;
};

} }  // namespace Mordor::Test

#endif // MORDOR_CO_TASK_H_
