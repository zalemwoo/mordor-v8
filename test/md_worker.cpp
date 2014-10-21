// Copyright 2013 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "md_worker.h"

#include <algorithm>
#include <iostream>

#include "v8/src/base/sys-info.h"
#include "mordor/assert.h"
#include "mordor/iomanager.h"
#include "mordor/fibersynchronization.h"
#include "mordor/sleep.h"
#include "md_task.h"

namespace Mordor
{
namespace Test
{

MD_Worker* CreateWorker(int worker_pool_size)
{
    MD_Worker* mdWorker = new MD_Worker();
    mdWorker->setWorkerPoolSize(worker_pool_size);
    mdWorker->ensureInitialized();
    return mdWorker;
}

const int MD_Worker::kMaxWorkerPoolSize = 4;

MD_Worker::MD_Worker() :
        initialized_(false), worker_pool_size_(0), worker_mutex_(), worker_cond_(worker_mutex_)
{
}

MD_Worker::~MD_Worker()
{
    std::lock_guard<std::mutex> scopeLock(lock_);
}

void MD_Worker::setWorkerPoolSize(int worker_pool_size)
{
    std::lock_guard<std::mutex> scopeLock(lock_);
    MORDOR_ASSERT(worker_pool_size >= 0);
    if (worker_pool_size < 1) {
        worker_pool_size = v8::base::SysInfo::NumberOfProcessors();
    }
    worker_pool_size_ = std::max(std::min(worker_pool_size, kMaxWorkerPoolSize), 1);
}

void MD_Worker::ensureInitialized()
{
    std::lock_guard<std::mutex> scopeLock(lock_);
    if (initialized_)
        return;
    initialized_ = true;

    sched_ = std::make_shared<IOManager>(1, false);

    workers_.resize(worker_pool_size_);

    for (int i = 0; i < worker_pool_size_; ++i) {
        workers_[i] = std::shared_ptr<Fiber>(new Fiber(std::bind(&MD_Worker::idle, this)));
    }

    sched_->schedule(workers_.begin(), workers_.end());

}

void MD_Worker::idle()
{
    while(true){
        Task* task = NULL;
        {
            FiberMutex::ScopedLock lock(worker_mutex_);
            worker_cond_.wait();
            std::cout << "*** run on: " << Fiber::getThis() << std::endl;
            task = task_queue_.getNext();
        }
        if(task){
            task->Call();
        }
    }
}

void MD_Worker::doTask(Task* task)
{
    task_queue_.append(task);
    worker_cond_.signal();
    task->waitEvent();
}

}
}  // namespace Mordor::Test
