#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <iostream>
#include <functional>

#include "md_runner.h"

#include "mordor/type_name.h"
#include "mordor/thread.h"
#include "mordor/fiber.h"
#include "mordor/assert.h"
#include "mordor/coroutine.h"
#include "mordor/streams/file.h"
#include "mordor/streams/std.h"
#include "mordor/streams/buffer.h"

#include "v8.h"
#include "md_v8_wrapper.h"

extern int g_argc;
extern char** g_argv;

namespace Mordor
{
namespace Test
{

static void readScript(Coroutine<const char*>& self)
{
    atexit(clear_history);

    Buffer buf;
    char* line = NULL;
    do {
        buf.clear(true);
        line = readline("> ");
        if (!line)
            break;
        if (*line) {
            add_history(line);
            buf.copyIn(line);
        }
        free(line);
        self.yield(buf.toString().c_str());
    } while (true);
}

MD_Runner::MD_Runner(Scheduler& sched) :
        sched_(sched)
{
    sched_.schedule(std::bind(&MD_Runner::run, this), gettid());
    this->wait();
}

MD_Runner::~MD_Runner()
{
}

void MD_Runner::run()
{
    MD_V8Wrapper::Scope warpper_scope(g_argc, g_argv);
    fprintf(stderr, "V8 version %s [sample shell]\n", v8::V8::GetVersion());

    std::shared_ptr<MD_V8Wrapper> runtime(warpper_scope.getWrapper());
    MD_V8Wrapper::s_curr_ = runtime.get();

    v8::Isolate* isolate = runtime->getIsolate();
    {
        v8::Locker locker(isolate);
        v8::HandleScope handle_scope(isolate);
        v8::Local<v8::Context> context = runtime->createContext();
        {
            Coroutine<const char*> coReadScript(&readScript);
            v8::Context::Scope context_scope(context);
            const char* script;
            bool running = true;
            do {
                script = coReadScript.call();
                if (coReadScript.state() == Fiber::State::TERM) {
                    break;
                }
                v8::HandleScope handle_scope(context->GetIsolate());
                runtime->execString(script, true, true);
                running = runtime->isRunning();
            } while (running);
        }
    }

    fputs("bye.\n", stderr);
    runtime.reset();
    sem_.notify();
}

} }  // namespace Mordor::Test
