#include "mordor/assert.h"
#include "md_task_queue.h"

namespace Mordor
{

namespace Test
{

MD_TaskQueue::MD_TaskQueue() :
        lock_(), condition_(lock_), terminated_(false)
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
    {
        FiberMutex::ScopedLock lock(lock_);
        MORDOR_ASSERT(!terminated_);
        task_queue_.push(task);
    }
    condition_.signal();
}

Task* MD_TaskQueue::getNext()
{
    while (true) {
        FiberMutex::ScopedLock lock(lock_);
        if (!task_queue_.empty()) {
            Task* task = task_queue_.front();
            task_queue_.pop();
            return task;
        }
        if (terminated_) {
            condition_.signal();
            return NULL;
        }
        condition_.wait();
    }
}

void MD_TaskQueue::terminate()
{
    {
        FiberMutex::ScopedLock lock(lock_);
        MORDOR_ASSERT(!terminated_);
        terminated_ = true;
    }
    condition_.broadcast();
}

} }  // namespace Mordor::Test
