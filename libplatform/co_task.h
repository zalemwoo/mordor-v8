#ifndef MORDOR_CO_TASK_H_
#define MORDOR_CO_TASK_H_

#include "v8/include/v8-platform.h"
#include "mordor/coroutine.h"
#include "mordor/fibersynchronization.h"

namespace Mordor {
namespace Platform {

struct DummyVoid;

struct CoTaskAbortedException : virtual OperationAbortedException {};

template <class Result, class Arg = DummyVoid>
class CoTask : public v8::Task, Mordor::noncopyable
{
public:
    CoTask()
    {
        m_fiber = Fiber::ptr(new Fiber(std::bind(&CoTask::run, this)));
    }

    CoTask(std::function<void (CoTask &, Arg)> dg, const Arg &arg)
        : m_dg(dg)
    {
        m_fiber = Fiber::ptr(new Fiber(std::bind(&CoTask::run, this)));
        m_arg = arg;
    }

    ~CoTask()
    {
        reset();
    }

    void reset()
    {
        if (m_fiber->state() == Fiber::HOLD) {
            try {
                throw boost::enable_current_exception(CoTaskAbortedException());
            } catch (...) {
                m_fiber->inject(boost::current_exception());
            }
        }
        m_fiber->reset(std::bind(&CoTask::run, this));
    }

    void reset(std::function<void (CoTask &, Arg)> dg)
    {
        reset();
        m_dg = dg;
    }

    void Run()
    {
        m_fiber->call();
    }

    Arg yield(const Result &result)
    {
        m_result = result;
        Fiber::yield();
        return m_arg;
    }

    Fiber::State state() const
    {
        return m_fiber->state();
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
            m_dg(*this, m_arg);
        } catch (CoTaskAbortedException &) {
        }
    }

private:
    std::function<void (CoTask &, Arg)> m_dg;
    Result m_result;
    Arg m_arg;
    Fiber::ptr m_fiber;
};


template <class Result>
class CoTask<Result, DummyVoid> : public v8::Task, Mordor::noncopyable
{
public:
    CoTask()
    {
        m_fiber = Fiber::ptr(new Fiber(std::bind(&CoTask::run, this)));
    }

    CoTask(std::function<void (CoTask &)> dg)
        : m_dg(dg)
    {
        m_fiber = Fiber::ptr(new Fiber(std::bind(&CoTask::run, this)));
    }

    ~CoTask()
    {
        reset();
    }

    void reset()
    {
        if (m_fiber->state() == Fiber::HOLD) {
            try {
                throw boost::enable_current_exception(CoTaskAbortedException());
            } catch (...) {
                m_fiber->inject(boost::current_exception());
            }
        }
        m_fiber->reset(std::bind(&CoTask::run, this));
    }

    void reset(std::function<void (CoTask &)> dg)
    {
        reset();
        m_dg = dg;
    }

    void Run()
    {
        m_fiber->call();
    }

    void yield(const Result &result)
    {
        m_result = result;
        Fiber::yield();
    }

    Fiber::State state() const
    {
        return m_fiber->state();
    }

    void setResult(const Result &result)
    {
        m_result = result;
        event_.set();
    }

    const Result & getResult()
    {
        event_.wait();
        return m_result;
    }
private:
    void run()
    {
        try {
            m_dg(*this);
        } catch (CoTaskAbortedException &) {
        }
    }

private:
    std::function<void (CoTask &)> m_dg;
    Result m_result;
    Fiber::ptr m_fiber;
    Mordor::FiberEvent event_{false};
};

template <class Arg>
class CoTask<void, Arg> : public v8::Task, Mordor::noncopyable
{
public:
    CoTask()
    {
        m_fiber = Fiber::ptr(new Fiber(std::bind(&CoTask::run, this)));
    }

    CoTask(std::function<void (CoTask &, Arg)> dg, const Arg &arg)
        : m_dg(dg)
    {
        m_fiber = Fiber::ptr(new Fiber(std::bind(&CoTask::run, this)));
        m_arg = arg;
    }

    ~CoTask()
    {
        reset();
    }

    void reset()
    {
        if (m_fiber->state() == Fiber::HOLD) {
            try {
                throw boost::enable_current_exception(CoTaskAbortedException());
            } catch (...) {
                m_fiber->inject(boost::current_exception());
            }
        }
        m_fiber->reset(std::bind(&CoTask::run, this));
    }

    void reset(std::function<void (CoTask &, Arg)> dg)
    {
        reset();
        m_dg = dg;
    }

    void Run()
    {
        m_fiber->call();
    }

    Arg yield()
    {
        Fiber::yield();
        return m_arg;
    }

    Fiber::State state() const
    {
        return m_fiber->state();
    }

private:
    void run()
    {
        try {
            m_dg(*this, m_arg);
        } catch (CoTaskAbortedException &) {
        }
    }

private:
    std::function<void (CoTask &, Arg)> m_dg;
    Arg m_arg;
    Fiber::ptr m_fiber;
};

} }  // namespace Mordor::Platform

#endif // MORDOR_CO_TASK_H_
