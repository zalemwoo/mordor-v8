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

  void doTask(Task* task);

 private:
  void run(Task *task);
  void idle();

 private:
  static const int kMaxWorkerPoolSize;

  std::mutex lock_;
  bool initialized_;
  int worker_pool_size_;
  std::vector<std::shared_ptr<Fiber>> workers_;
  Fiber* curr_fiber_{nullptr};

  std::shared_ptr<IOManager> sched_;

  FiberMutex worker_mutex_;
  FiberCondition worker_cond_;

  MD_TaskQueue task_queue_;
};

MD_Worker* CreateWorker(int worker_pool_size = 0);

} }  // namespace Mordor::Test


#endif  // MORDOR_LIBPLATFORM_PLATFORM_H_
