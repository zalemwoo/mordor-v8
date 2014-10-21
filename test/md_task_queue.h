// Copyright 2013 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_LIBPLATFORM_TASK_QUEUE_H_
#define V8_LIBPLATFORM_TASK_QUEUE_H_

#include <queue>

#include "mordor/fibersynchronization.h"
#include "md_task.h"

namespace Mordor {

namespace Test {

class MD_TaskQueue : Mordor::noncopyable{
 public:
  MD_TaskQueue();
  ~MD_TaskQueue();

  // Appends a task to the queue. The queue takes ownership of |task|.
  void append(Task* task);

  // Returns the next task to process. Blocks if no task is available. Returns
  // NULL if the queue is terminated.
  Task* getNext();

  // Terminate the queue.
  void terminate();

 private:
  FiberMutex lock_;
  FiberSemaphore process_queue_semaphore_;
  std::queue<Task*> task_queue_;
  bool terminated_;

};

}
}  // namespace Mordor::Test


#endif  // V8_LIBPLATFORM_TASK_QUEUE_H_
