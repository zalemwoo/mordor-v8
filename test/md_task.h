#ifndef MORDOR_CO_TASK_H_
#define MORDOR_CO_TASK_H_

#include "mordor/assert.h"
#include "mordor/util.h"
#include "mordor/coroutine.h"
#include "mordor/semaphore.h"
#include "mordor/fibersynchronization.h"

#include "v8.h"
#include "v8_persistent_wrapper.h"
#include "md_v8_util_inl.h"

namespace Mordor
{
namespace Test
{

struct MdTaskAbortedException: virtual OperationAbortedException
{
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

struct TASK;      // for normal task
struct TASK_V8;   // for task need v8 context

template<typename TaskSignature> class MD_Task;

namespace Internal {

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
class TaskCallback<Result(TASK_V8)> : Mordor::noncopyable
{
public:
    typedef MD_Task<Result(TASK_V8)> TaskType;
    typedef std::function<void(TaskType&)> CallbackType;
    TaskCallback(TaskType& task, CallbackType dg):task_ref(task), dg_(dg){}

    void run(){
        MORDOR_ASSERT(task_ref.isolate_ != NULL);
        v8::Locker locker(task_ref.isolate_);
        v8::Isolate::Scope isolate_scope(task_ref.isolate_);
        v8::HandleScope handle_scope(task_ref.isolate_);
        v8::Context::Scope context_scope(task_ref.context());
        dg_(task_ref);
    }

private:
    TaskType& task_ref;
    CallbackType dg_;
};

class TaskV8_ : public Task
{
public:
    TaskV8_(v8::Local<v8::Context> context)
        : Task(), isolate_(context->GetIsolate())
    {
        context_.Reset(isolate_, context);
    }

    virtual ~TaskV8_(){
        context_.Reset();
    }

    v8::Local<v8::Context> context(){
        return StrongPersistentToLocal(context_);
    }
    v8::Isolate* isolate(){
        return isolate_;
    }

    virtual void waitEvent() override
    {
        v8::Unlocker unlocker(isolate_);
        Task::waitEvent();
    }

protected:
    v8::Persistent<v8::Context> context_;
    v8::Isolate* isolate_;
};

template<typename Result>
class Result_
{
public:
    template<typename R = Result>
    void setResult(const R &result){
        result_.set(result);
    }
    template<typename R = Result>
    R getResult(){
        return result_.get();
    }

protected:
    TaskResult<Result> result_;
};

} // namespace Internal

template<typename Result, typename... ARGS>
class MD_Task<Result(ARGS...)> : public Task, public Internal::Result_<Result>
{
public:
    typedef typename Internal::TaskCallback<Result(ARGS...)>::CallbackType CallbackType;
public:
    MD_Task(CallbackType dg) : Task(), dg_(*this, dg)
    {}

    ~MD_Task(){}
protected:
    virtual void run()
    {
        dg_.run();
    }
protected:
    Internal::TaskCallback<Result(ARGS...)> dg_;
};

template<typename Result>
class MD_Task<Result(TASK_V8)> : public Internal::TaskV8_, public Internal::Result_<Result>
{
public:
    typedef typename Internal::TaskCallback<Result(TASK_V8)>::CallbackType CallbackType;
public:
    MD_Task(v8::Local<v8::Context> context, CallbackType dg)
        : Internal::TaskV8_(context), dg_(*this, dg)
    {}

    ~MD_Task(){}

protected:
    virtual void run()
    {
        dg_.run();
    }
protected:
    Internal::TaskCallback<Result(TASK_V8)> dg_;
    friend Internal::TaskCallback<Result(TASK_V8)>;
};

} }  // namespace Mordor::Test

#endif // MORDOR_CO_TASK_H_
