// Copyright 2013 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "md_platform.h"

#include <algorithm>

#include "v8/src/base/logging.h"
#include "v8/src/base/platform/platform.h"
#include "v8/src/base/sys-info.h"
#include "v8/src/libplatform/worker-thread.h"

#include "mordor/iomanager.h"

namespace Mordor
{
namespace Platform
{

using namespace v8;

v8::Platform* CreatePlatform(int thread_pool_size)
{
    DefaultPlatform* platform = new DefaultPlatform();
    platform->SetThreadPoolSize(thread_pool_size);
    platform->EnsureInitialized();
    return platform;
}

bool PumpMessageLoop(v8::Platform* platform, v8::Isolate* isolate)
{
    return reinterpret_cast<DefaultPlatform*>(platform)->PumpMessageLoop(isolate);
}

const int DefaultPlatform::kMaxThreadPoolSize = 4;

DefaultPlatform::DefaultPlatform() :
        initialized_(false), thread_pool_size_(0)
{
}

DefaultPlatform::~DefaultPlatform()
{
    std::lock_guard<std::mutex> scopeLock(lock_);
}

void DefaultPlatform::SetThreadPoolSize(int thread_pool_size)
{
    std::lock_guard<std::mutex> scopeLock(lock_);
    DCHECK(thread_pool_size >= 0);
    if (thread_pool_size < 1) {
        thread_pool_size = base::SysInfo::NumberOfProcessors();
    }
    thread_pool_size_ = std::max(std::min(thread_pool_size, kMaxThreadPoolSize), 1);
}

void DefaultPlatform::EnsureInitialized()
{
    std::lock_guard<std::mutex> scopeLock(lock_);
    if (initialized_)
        return;
    initialized_ = true;

    scheduler_.reset(new IOManager(thread_pool_size_, false));
}

void DefaultPlatform::runOnBackground(v8::Task *task)
{
    std::unique_ptr<Task> t(task);
    t->Run();
}

bool DefaultPlatform::PumpMessageLoop(v8::Isolate* isolate)
{
    // TODO
//    Task* task = NULL;
//    task->Run();
//    delete task;
    return true;
}

void DefaultPlatform::CallOnBackgroundThread(v8::Task *task, v8::Platform::ExpectedRuntime expected_runtime)
{
    EnsureInitialized();
    scheduler_->schedule(std::bind(&DefaultPlatform::runOnBackground, this, task));
}

void DefaultPlatform::CallOnForegroundThread(v8::Isolate* isolate, v8::Task* task)
{
    // TODO
}

}
}  // namespace Mordor::Platform
