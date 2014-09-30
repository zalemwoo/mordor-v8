
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

#include <functional>

#include "md_runner.h"

#include "mordor/type_name.h"
#include "mordor/workerpool.h"
#include "mordor/thread.h"
#include "mordor/fiber.h"
#include "mordor/streams/file.h"
#include "mordor/streams/std.h"
#include "mordor/streams/buffer.h"
#include "mordor/sleep.h"

#include "v8.h"
#include "md_v8_wrapper.h"

extern int g_argc;
extern char** g_argv;

namespace Mordor {
namespace Test {

MD_Runner::MD_Runner(Scheduler& iom):iom_(iom){
    std::function<void()> fiber_func = std::bind(&MD_Runner::run, this);
    Fiber::ptr fiber(new Fiber(fiber_func));
    iom_.schedule(fiber);
}

MD_Runner::~MD_Runner(){
    runtime_.reset();
}

void MD_Runner::run()
{
    MD_V8Wrapper::Scope warpper_scope(g_argc, g_argv);

    runtime_ = MD_V8Wrapper::getWrapper();

    fprintf(stderr, "V8 version %s [sample shell]\n", v8::V8::GetVersion());

    v8::Isolate* isolate = runtime_->getIsolate();
    {
      v8::Locker locker(isolate);
      v8::HandleScope handle_scope(isolate);
      v8::Local<v8::Context> context = runtime_->createContext();
      runtime_->setName("md_sample");
      {
        v8::Context::Scope context_scope(context);
        bool more = true;
        do {
            char buffer[256];
            fprintf(stderr, "> ");
            char* str = fgets(buffer, 256, stdin);
            if (str == NULL)
                break;
            v8::HandleScope handle_scope(context->GetIsolate());
            runtime_->execString(str,true, true);
        } while (more && runtime_->isRunning() == true);
      }
    }
    fprintf(stderr, "\n");
}

} }  // namespace Mordor::Test
