// Copyright 2013 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MORDOR_LIBPLATFORM_DEFAULT_PLATFORM_H_
#define MORDOR_LIBPLATFORM_DEFAULT_PLATFORM_H_

#include <map>
#include <queue>
#include <vector>

#include "v8/include/v8-platform.h"
#include "v8/src/base/macros.h"
#include "v8/src/base/platform/mutex.h"
#include "v8/src/libplatform/task-queue.h"

namespace Mordor {
namespace platform {

class DefaultPlatform : public v8::Platform {
 public:
  DefaultPlatform();
  virtual ~DefaultPlatform();

  // v8::Platform implementation.
  virtual void CallOnBackgroundThread(
      v8::Task* task, ExpectedRuntime expected_runtime) OVERRIDE;
  virtual void CallOnForegroundThread(v8::Isolate* isolate,
          v8::Task* task) OVERRIDE;

 private:
  static const int kMaxThreadPoolSize;

  bool initialized_;
  int thread_pool_size_;

  DISALLOW_COPY_AND_ASSIGN(DefaultPlatform);
};


} }  // namespace Mordor::platform


#endif  // MORDOR_LIBPLATFORM_DEFAULT_PLATFORM_H_