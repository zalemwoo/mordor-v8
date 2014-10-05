#ifndef MORDOR_CO_TASK_H_
#define MORDOR_CO_TASK_H_

#include "mordor/util.h"
#include "mordor/coroutine.h"
#include "mordor/fibersynchronization.h"

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
            isolate_(isolate)
    {
        m_fiber = Fiber::ptr(new Fiber(std::bind(&Task::run, this)));
    }

    virtual ~Task()
    {
        reset();
    }

    void reset()
    {
        if (m_fiber->state() == Fiber::HOLD) {
            try {
                throw boost::enable_current_exception(MdTaskAbortedException());
            } catch (...) {
                m_fiber->inject(boost::current_exception());
            }
        }
        m_fiber->reset(std::bind(&Task::run, this));
    }

    void Call()
    {
        m_fiber->call();
    }

    v8::Isolate* getIsolate()
    {
        return isolate_;
    }

    Fiber::State state() const
    {
        return m_fiber->state();
    }

    void waitEvent()
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
    Fiber::ptr m_fiber;
    v8::Isolate* isolate_;
    Mordor::FiberEvent event_ { false };
};

template<class Result, class Arg = DummyVoid>
class MD_Task: public Task, Mordor::noncopyable
{
public:
    MD_Task(v8::Isolate* isolate, std::function<void(MD_Task &, Arg)> dg, const Arg &arg) :
            Task(isolate), m_dg(dg)
    {
        m_fiber = Fiber::ptr(new Fiber(std::bind(&MD_Task::run, this)));
        m_arg = arg;
    }

    void reset(std::function<void(MD_Task &, Arg)> dg)
    {
        reset();
        m_dg = dg;
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
            setEvent();
        } catch (MdTaskAbortedException &) {
        }
    }

private:
    std::function<void(MD_Task &, Arg)> m_dg;
    Result m_result;
    Arg m_arg;
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

    void reset(std::function<void(MD_Task &)> dg)
    {
        reset();
        m_dg = dg;
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
            setEvent();
        } catch (MdTaskAbortedException &) {
        }
    }

private:
    std::function<void(MD_Task &)> m_dg;
    Result m_result;
};

template<class Arg>
class MD_Task<void, Arg> : public Task, Mordor::noncopyable
{
public:
    MD_Task(v8::Isolate* isolate, std::function<void(MD_Task &, Arg)> dg, const Arg &arg) :
            Task(isolate), m_dg(dg)
    {
        m_fiber = Fiber::ptr(new Fiber(std::bind(&MD_Task::run, this)));
        m_arg = arg;
    }

    void reset(std::function<void(MD_Task &, Arg)> dg)
    {
        reset();
        m_dg = dg;
    }

private:
    void run()
    {
        try {
            m_dg(*this, m_arg);
            setEvent();
        } catch (MdTaskAbortedException &) {
        }
    }

private:
    std::function<void(MD_Task &, Arg)> m_dg;
    Arg m_arg;
};

}
}  // namespace Mordor::Test

#endif // MORDOR_CO_TASK_H_
