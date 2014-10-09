// Copyright 2013 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MORDOR_LIBPLATFORM_PLATFORM_H_
#define MORDOR_LIBPLATFORM_PLATFORM_H_

#include <mutex>
#include <memory>

#include "v8/include/v8-platform.h"
#include "mordor/util.h"

namespace Mordor {

class WorkerPool;

namespace Platform {

class DefaultPlatform : public v8::Platform, Mordor::noncopyable {
 public:
  DefaultPlatform();
  virtual ~DefaultPlatform();

  void SetThreadPoolSize(int thread_pool_size);

  void EnsureInitialized();

  bool PumpMessageLoop(v8::Isolate* isolate);

  // v8::Platform implementation.
  virtual void CallOnBackgroundThread(
      v8::Task* task,  v8::Platform::ExpectedRuntime expected_runtime) override;
  virtual void CallOnForegroundThread(v8::Isolate* isolate,
          v8::Task* task) override;
  virtual double MonotonicallyIncreasingTime() override;

 private:
  void runOnBackground(v8::Task *task);

 private:
  static const int kMaxThreadPoolSize;

  std::mutex lock_;
  bool initialized_;
  int thread_pool_size_;
  std::unique_ptr<WorkerPool> scheduler_;
};

} }  // namespace Mordor::Platform


#endif  // MORDOR_LIBPLATFORM_PLATFORM_H_
