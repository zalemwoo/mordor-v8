// Copyright 2013 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MORDOR_LIBPLATFORM_PLATFORM_H_
#define MORDOR_LIBPLATFORM_PLATFORM_H_

#include <mutex>
#include <memory>

#include "v8.h"
#include "mordor/fiber.h"
#include "mordor/fibersynchronization.h"

#include "md_task_queue.h"

namespace Mordor
{

class IOManager;

namespace Test
{

class MD_Worker: Mordor::noncopyable
{
public:
    virtual ~MD_Worker();

    static MD_Worker* New(Scheduler* sched, int worker_pool_size = 0);

    template<typename Result, typename ... ARGS>
    void doTask(const typename MD_Task<Result(ARGS...)>::CallbackType& func, Result& ret)
    {
        MD_Task<Result(ARGS...)> task(func);
        task_queue_.append(&task);
        task.waitEvent();
        ret = task.getResult();
    }

    template<typename Result, typename ... ARGS>
    void doTask(const typename MD_Task<void(ARGS...)>::CallbackType& func)
    {
        MD_Task<void(ARGS...)> task(func);
        task_queue_.append(&task);
        task.waitEvent();
    }

    template<typename Result>
    void doTask(const typename MD_Task<Result()>::CallbackType& func, Result& ret)
    {
        MD_Task<Result()> task(func);
        task_queue_.append(&task);
        task.waitEvent();
        ret = task.getResult();
    }

    template<typename Result, typename ... ARGS>
    void doTask(v8::Local<v8::Context> context, const typename MD_Task<Result(ARGS...)>::CallbackType& func,
            Result& ret)
    {
        MD_Task<Result(ARGS...)> task(context, func);
        task_queue_.append(&task);
        task.waitEvent();
        ret = task.getResult();
    }

    template<typename Result, typename ... ARGS>
    void doTask(v8::Local<v8::Context> context, const typename MD_Task<void(ARGS...)>::CallbackType& func)
    {
        MD_Task<void(ARGS...)> task(context, func);
        task_queue_.append(&task);
        task.waitEvent();
    }

    template<typename Result>
    void doTask(v8::Local<v8::Context> context, const typename MD_Task<Result()>::CallbackType& func, Result& ret)
    {
        MD_Task<Result()> task(context, func);
        task_queue_.append(&task);
        task.waitEvent();
        ret = task.getResult();
    }

private:
    MD_Worker(Scheduler* sched);
    void setWorkerPoolSize(int worker_pool_size);
    void ensureInitialized();

    void stop();
    void run();

private:
    static const int kMaxWorkerFiberSize;

    std::mutex lock_;
    bool initialized_ { false };
    int worker_pool_size_ { 0 };
    int termed_workers_ { 0 };
    FiberSemaphore stop_lock_ { 0 };
    std::vector<Fiber::ptr> workers_;
    Scheduler* sched_;
    MD_TaskQueue task_queue_;
};

}
}  // namespace Mordor::Test

#endif  // MORDOR_LIBPLATFORM_PLATFORM_H_
