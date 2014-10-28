#ifndef MD_RUNNER_H_
#define MD_RUNNER_H_

#include "mordor/version.h"

#include "mordor/iomanager.h"
#include "mordor/fibersynchronization.h"

namespace Mordor {

namespace Test {

class MD_V8Wrapper;

class LineEditor
{
public:
    enum Type { DUMB = 0, READLINE = 1 };
    LineEditor(Type type, const char* name);
    virtual ~LineEditor() { }

    virtual std::string Prompt(const char* prompt) = 0;
    virtual bool Open() { return true; }
    virtual bool Close() { return true; }
    virtual void AddHistory(const char* str) { }

    const char* name() { return name_; }
    static LineEditor* Get() { return current_; }
private:
    Type type_;
    const char* name_;
    static LineEditor* current_;
};

class MD_Runner
{
public:
    MD_Runner(Scheduler& iom);
    ~MD_Runner();

    void run();

    void wait(){
        sem_.wait();
    }

    void over(){
        sem_.notify();
    }

private:
    Scheduler& sched_;
    FiberSemaphore sem_;
};

} } // namespace Mordor::Test

#endif // MD_RUNNER_H_
