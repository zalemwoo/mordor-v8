#ifndef MORDOR_CO_TASK_H_
#define MORDOR_CO_TASK_H_

#include "mordor/util.h"
#include "mordor/coroutine.h"
#include "mordor/fibersynchronization.h"

#include "v8.h"

namespace Mordor
{
namespace Test
{

struct DummyVoid;

struct MdTaskAbortedException: virtual OperationAbortedException
{
};

class Task
{
public:
    Task(v8::Isolate* isolate) :
            isolate(isolate)
    {
        m_fiber = Fiber::ptr(new Fiber(std::bind(&Task::run, this)));
    }

    virtual ~Task()
    {
        // reset();
    }

    void reset()
    {
        if (m_fiber->state() == Fiber::HOLD) {
            try {
                throw MdTaskAbortedException();
            } catch (...) {
                m_fiber->inject(std::current_exception());
            }
        }
        m_fiber->reset(std::bind(&Task::run, this));
    }

    void Call()
    {
        m_fiber->call();
    }

    Fiber::State state() const
    {
        return m_fiber->state();
    }

    virtual void waitEvent()
    {
        event_.wait();
    }

public:
    v8::Isolate* isolate;

protected:
    virtual void run() = 0;

    void setEvent()
    {
        event_.set();
    }

protected:
    Fiber::ptr m_fiber;
    Mordor::FiberEvent event_ { false };
};

template<class Result, class Arg = DummyVoid>
class MD_Task: public Task, Mordor::noncopyable
{
public:
    MD_Task(v8::Isolate* isolate, std::function<void(MD_Task &, const Arg&)> dg, const Arg& arg) :
            Task(isolate), m_dg(dg), m_arg(arg)
    {
        m_fiber = Fiber::ptr(new Fiber(std::bind(&MD_Task::run, this)));
    }

    void setResult(const Result &result)
    {
        m_result = result;
    }

    const Result& getResult()
    {
        return m_result;
    }
private:
    void run()
    {
        try {
            m_dg(*this, m_arg);
        } catch (MdTaskAbortedException &) {
        }
        setEvent();
    }

private:
    std::function<void(MD_Task &, const Arg&)> m_dg;
    Result m_result;
    const Arg& m_arg;
};

template<class Result>
class MD_Task<Result, DummyVoid> : public Task, Mordor::noncopyable
{
public:
    MD_Task(v8::Isolate* isolate, std::function<void(MD_Task &)> dg) :
            Task(isolate), m_dg(dg)
    {
        m_fiber = Fiber::ptr(new Fiber(std::bind(&MD_Task::run, this)));
    }

    void setResult(const Result &result)
    {
        m_result = result;
    }

    const Result & getResult()
    {
        return m_result;
    }
private:
    void run()
    {
        try {
            m_dg(*this);
        } catch (MdTaskAbortedException &) {
        }
        setEvent();
    }

private:
    std::function<void(MD_Task &)> m_dg;
    Result m_result;
};

template<class Arg>
class MD_Task<void, Arg> : public Task, Mordor::noncopyable
{
public:
    MD_Task(v8::Isolate* isolate, std::function<void(MD_Task &, const Arg&)> dg, const Arg& arg) :
            Task(isolate), m_dg(dg), m_arg(arg)
    {
        m_fiber = Fiber::ptr(new Fiber(std::bind(&MD_Task::run, this)));
    }

private:
    void run()
    {
        try {
            m_dg(*this, m_arg);
        } catch (MdTaskAbortedException &) {
        }
        setEvent();
    }

private:
    std::function<void(MD_Task &, const Arg&)> m_dg;
    const Arg& m_arg;
};


template<class Result>
class MD_Task<Result, const v8::FunctionCallbackInfo<v8::Value> &>: public Task, Mordor::noncopyable
{
public:
    typedef const v8::FunctionCallbackInfo<v8::Value>& arg_type;
    MD_Task(v8::Isolate* isolate, std::function<void(MD_Task &, arg_type)> dg, arg_type arg) :
            Task(isolate), m_dg(dg), m_arg(arg)
    {
        m_fiber = Fiber::ptr(new Fiber(std::bind(&MD_Task::run, this)));
    }

    virtual void waitEvent() override
    {
        v8::Unlocker unlocker(isolate);
        event_.wait();
    }

    void setResult(const Result &result)
    {
        m_result = result;
    }

    const Result& getResult()
    {
        return m_result;
    }
private:
    void run()
    {
        try {
            v8::Locker locker(isolate);
            v8::Isolate::Scope isolate_scope(isolate);
            m_dg(*this, m_arg);
        } catch (MdTaskAbortedException &) {
        }
        setEvent();
    }

private:
    std::function<void(MD_Task &, arg_type)> m_dg;
    Result m_result;
    arg_type m_arg;
};


template<>
class MD_Task<void, const v8::FunctionCallbackInfo<v8::Value> &> : public Task, Mordor::noncopyable
{
public:
    typedef const v8::FunctionCallbackInfo<v8::Value>& arg_type;

    MD_Task(v8::Isolate* isolate, std::function<void(MD_Task &, arg_type)> dg, arg_type arg) :
            Task(isolate), m_dg(dg), m_arg(arg)
    {
        m_fiber = Fiber::ptr(new Fiber(std::bind(&MD_Task::run, this)));
    }

    virtual void waitEvent() override
    {
        v8::Unlocker unlocker(isolate);
        event_.wait();
    }

private:
    void run()
    {
        try {
            v8::Locker locker(isolate);
            v8::Isolate::Scope isolate_scope(isolate);
            m_dg(*this, m_arg);
        } catch (MdTaskAbortedException &) {
        }
        setEvent();
    }

private:
    std::function<void(MD_Task &, arg_type)> m_dg;
    arg_type m_arg;
};

}
}  // namespace Mordor::Test

#endif // MORDOR_CO_TASK_H_
