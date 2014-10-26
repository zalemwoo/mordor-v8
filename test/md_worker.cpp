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

namespace Mordor
{
namespace Test
{

MD_Worker* CreateWorker(IOManager* sched, int worker_pool_size)
{
    MD_Worker* mdWorker = new MD_Worker(sched);
    mdWorker->setWorkerPoolSize(worker_pool_size);
    mdWorker->ensureInitialized();
    return mdWorker;
}

const int MD_Worker::kMaxWorkerThreadSize = 4;

MD_Worker::MD_Worker(IOManager* sched) :
        initialized_(false), worker_pool_size_(0), sched_(sched)
{
}

MD_Worker::~MD_Worker()
{
    std::lock_guard<std::mutex> scopeLock(lock_);
    stop();
}

void MD_Worker::setWorkerPoolSize(int worker_pool_size)
{
    std::lock_guard<std::mutex> scopeLock(lock_);
    MORDOR_ASSERT(worker_pool_size >= 0);
    if (worker_pool_size < 1) {
        worker_pool_size = v8::base::SysInfo::NumberOfProcessors();
    }
    worker_pool_size_ = std::max(std::min(worker_pool_size, kMaxWorkerThreadSize), 1);
}

void MD_Worker::ensureInitialized()
{
    std::lock_guard<std::mutex> scopeLock(lock_);
    if (initialized_)
        return;
    initialized_ = true;

    workers_.resize(worker_pool_size_);

    for (int i = 0; i < worker_pool_size_; ++i) {
        workers_[i] = std::shared_ptr<Fiber>(new Fiber(std::bind(&MD_Worker::run, this)));
    }

    sched_->schedule(workers_.begin(), workers_.end());
}

void MD_Worker::stop()
{
    task_queue_.terminate();
    stop_lock_.wait();
}

void MD_Worker::run()
{
    Task* task = NULL;
    while (true) {
        task = task_queue_.getNext();
        if (!task){
            if(termed_workers_++ == worker_pool_size_){
                stop_lock_.notify();
            }
            return;
        }
        std::cout << "*** run on fiber:" << Fiber::getThis() << ", tid:" << Mordor::gettid() << std::endl << std::flush;
        task->Call();
    }
}

} }  // namespace Mordor::Test
