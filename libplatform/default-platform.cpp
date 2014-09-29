// Copyright 2013 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "default-platform.h"

namespace Mordor {
namespace platform {

v8::Platform* CreateDefaultPlatform(int thread_pool_size) {
  DefaultPlatform* platform = new DefaultPlatform();
  return platform;
}

const int DefaultPlatform::kMaxThreadPoolSize = 4;

DefaultPlatform::DefaultPlatform()
    : initialized_(false), thread_pool_size_(0) {}


DefaultPlatform::~DefaultPlatform() {
}


void DefaultPlatform::CallOnBackgroundThread(v8::Task *task,
                                             ExpectedRuntime expected_runtime) {
}


void DefaultPlatform::CallOnForegroundThread(v8::Isolate* isolate, v8::Task* task) {
}

} }  // namespace Mordor::platform
