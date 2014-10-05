// Copyright 2013 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "md_worker.h"

#include <algorithm>

#include "v8/src/base/sys-info.h"
#include "mordor/iomanager.h"
#include "md_task.h"

namespace Mordor
{
namespace Test
{

MD_Worker* CreateWorker(int thread_pool_size)
{
    MD_Worker* mdWorker = new MD_Worker();
    mdWorker->SetThreadPoolSize(thread_pool_size);
    mdWorker->EnsureInitialized();
    return mdWorker;
}

const int MD_Worker::kMaxThreadPoolSize = 4;

MD_Worker::MD_Worker() :
        initialized_(false), thread_pool_size_(0)
{
}

MD_Worker::~MD_Worker()
{
    std::lock_guard<std::mutex> scopeLock(lock_);
}

void MD_Worker::SetThreadPoolSize(int thread_pool_size)
{
    std::lock_guard<std::mutex> scopeLock(lock_);
    DCHECK(thread_pool_size >= 0);
    if (thread_pool_size < 1) {
        thread_pool_size = v8::base::SysInfo::NumberOfProcessors();
    }
    thread_pool_size_ = std::max(std::min(thread_pool_size, kMaxThreadPoolSize), 1);
}

void MD_Worker::EnsureInitialized()
{
    std::lock_guard<std::mutex> scopeLock(lock_);
    if (initialized_)
        return;
    initialized_ = true;

    scheduler_.reset(new IOManager(thread_pool_size_, false));
}

void MD_Worker::run(Task *task)
{
    v8::Locker locker(task->getIsolate());
    task->Call();
}

void MD_Worker::doTask(Task* task)
{
    scheduler_->schedule(std::bind(&MD_Worker::run, this, task));
    v8::Unlocker unlocker(task->getIsolate());
    task->waitEvent();
}

}
}  // namespace Mordor::Test
