#include "mordor/assert.h"
#include "md_task_queue.h"

namespace Mordor
{

namespace Test
{

MD_TaskQueue::MD_TaskQueue() :
        process_queue_semaphore_(0), terminated_(false)
{
}

MD_TaskQueue::~MD_TaskQueue()
{
    FiberMutex::ScopedLock lock(lock_);
    MORDOR_ASSERT(terminated_);
    MORDOR_ASSERT(task_queue_.empty());
}

void MD_TaskQueue::append(Task* task)
{
    FiberMutex::ScopedLock lock(lock_);
    MORDOR_ASSERT(!terminated_);
    task_queue_.push(task);
    process_queue_semaphore_.notify();
}

Task* MD_TaskQueue::getNext()
{
    for (;;) {
        {
            FiberMutex::ScopedLock lock(lock_);
            if (!task_queue_.empty()) {
                Task* result = task_queue_.front();
                task_queue_.pop();
                return result;
            }
            if (terminated_) {
                process_queue_semaphore_.notify();
                return NULL;
            }
        }
        process_queue_semaphore_.wait();
    }
}

void MD_TaskQueue::terminate()
{
    FiberMutex::ScopedLock lock(lock_);
    MORDOR_ASSERT(!terminated_);
    terminated_ = true;
    process_queue_semaphore_.notify();
}

}
}  // namespace Mordor::Test
