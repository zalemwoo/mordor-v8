// Copyright 2013 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MORDOR_LIBPLATFORM_PLATFORM_H_
#define MORDOR_LIBPLATFORM_PLATFORM_H_

#include <mutex>
#include <memory>

#include "v8.h"

#include "md_task.h"

namespace Mordor {

class IOManager;

namespace Test {

class MD_Worker : Mordor::noncopyable{
 public:
  MD_Worker();
  virtual ~MD_Worker();

  void setThreadPoolSize(int thread_pool_size);

  void ensureInitialized();

  void doTask(Task* task);

 private:
  void run(Task *task);

 private:
  static const int kMaxThreadPoolSize;

  std::mutex lock_;
  bool initialized_;
  int thread_pool_size_;
  std::unique_ptr<IOManager> scheduler_;
};

MD_Worker* CreateWorker(int thread_pool_size = 0);

} }  // namespace Mordor::Test


#endif  // MORDOR_LIBPLATFORM_PLATFORM_H_
