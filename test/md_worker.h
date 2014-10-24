// Copyright 2013 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MORDOR_LIBPLATFORM_PLATFORM_H_
#define MORDOR_LIBPLATFORM_PLATFORM_H_

#include <mutex>
#include <memory>

#include "v8.h"
#include "mordor/fiber.h"

#include "md_task_queue.h"

namespace Mordor {

class IOManager;

namespace Test {

class MD_Worker : Mordor::noncopyable{
 public:
  MD_Worker();
  virtual ~MD_Worker();

  void setWorkerPoolSize(int worker_pool_size);
  void ensureInitialized();

  void stop();

  template <typename Result, typename... ARGS>
  void doTask(v8::Isolate* isolate, const typename MD_Task<Result(ARGS...)>::CallbackType& func, Result& ret)
  {
      MD_Task<Result(ARGS...)> task(isolate, func);
      task_queue_.append(&task);
      task.waitEvent();
      ret = task.getResult();
  }

  template <typename Result, typename... ARGS>
  void doTask(v8::Isolate* isolate, const typename MD_Task<void(ARGS...)>::CallbackType& func)
  {
      MD_Task<void(ARGS...)> task(isolate, func);
      task_queue_.append(&task);
      task.waitEvent();
  }

  template <typename Result>
  void doTask(v8::Isolate* isolate, const typename MD_Task<Result()>::CallbackType& func, Result& ret)
  {
      MD_Task<Result()> task(isolate, func);
      task_queue_.append(&task);
      task.waitEvent();
      ret = task.getResult();
  }

 private:
  void run();

 private:
  static const int kMaxWorkerThreadSize;

  std::mutex lock_;
  bool initialized_;
  int worker_pool_size_;
  std::vector<std::shared_ptr<Fiber>> workers_;

  std::shared_ptr<IOManager> sched_;

  FiberMutex worker_mutex_;
  FiberCondition worker_cond_;

  MD_TaskQueue task_queue_;
};

MD_Worker* CreateWorker(int worker_pool_size = 0);

} }  // namespace Mordor::Test


#endif  // MORDOR_LIBPLATFORM_PLATFORM_H_
