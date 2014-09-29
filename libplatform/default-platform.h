// Copyright 2013 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MORDOR_LIBPLATFORM_DEFAULT_PLATFORM_H_
#define MORDOR_LIBPLATFORM_DEFAULT_PLATFORM_H_

#include <mutex>
#include <memory>

#include "v8/include/v8-platform.h"
#include "v8/src/base/macros.h"
#include "v8/src/base/platform/mutex.h"
#include "v8/src/libplatform/task-queue.h"

namespace Mordor {

class Scheduler;

namespace platform {

class DefaultPlatform : public v8::Platform {
 public:
  DefaultPlatform();
  virtual ~DefaultPlatform();

  void SetThreadPoolSize(int thread_pool_size);

  void EnsureInitialized();

  bool PumpMessageLoop(v8::Isolate* isolate);

  // v8::Platform implementation.
  virtual void CallOnBackgroundThread(
      v8::Task* task,  v8::Platform::ExpectedRuntime expected_runtime) OVERRIDE;
  virtual void CallOnForegroundThread(v8::Isolate* isolate,
          v8::Task* task) OVERRIDE;

 private:
  void runOnBackground(v8::Task *task);

 private:
  static const int kMaxThreadPoolSize;

  std::mutex lock_;
  bool initialized_;
  int thread_pool_size_;
  std::unique_ptr<Scheduler> scheduler_;

  DISALLOW_COPY_AND_ASSIGN(DefaultPlatform);
};

} }  // namespace Mordor::platform


#endif  // MORDOR_LIBPLATFORM_DEFAULT_PLATFORM_H_
