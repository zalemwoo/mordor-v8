#ifndef MD_RUNNER_H_
#define MD_RUNNER_H_

#include "mordor/version.h"

#include "mordor/iomanager.h"
#include "mordor/fibersynchronization.h"

namespace Mordor {


namespace Test {

class MD_V8Wrapper;

class MD_Runner
{
public:
    MD_Runner(Scheduler& iom);
    ~MD_Runner();

    void run();

    void wait(){
        sem_.wait();
    }

private:
    Scheduler& sched_;
    FiberSemaphore sem_;
};

} } // namespace Mordor::Test

#endif // MD_RUNNER_H_
