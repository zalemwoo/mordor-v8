#ifndef MD_RUNNER_H_
#define MD_RUNNER_H_

#include "mordor/iomanager.h"

namespace Mordor {
namespace Test {

class MD_V8Wrapper;

class MD_Runner
{
public:
    MD_Runner(Scheduler& iom);
    ~MD_Runner();

    void run();

    int getResult(){
        return result_;
    }
private:
    Scheduler& iom_;
    std::shared_ptr<MD_V8Wrapper> runtime_;
    int result_{0};
};

} } // namespace Mordor::Test

#endif // MD_RUNNER_H_
