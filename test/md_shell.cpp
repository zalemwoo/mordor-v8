// Copyright 2012 the V8 project authors. All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//     * Neither the name of Google Inc. nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <functional>

#include <v8.h>

#include "mordor/config.h"
#include "mordor/main.h"
#include "mordor/type_name.h"
#include "mordor/iomanager.h"
#include "mordor/workerpool.h"
#include "mordor/thread.h"
#include "mordor/fiber.h"
#include "mordor/sleep.h"

#include "libplatform/libplatform.h"

#include "md_runner.h"

#ifdef COMPRESS_STARTUP_DATA_BZ2
#error Using compressed startup data is not supported for this sample
#endif

/**
 * This sample program shows how to implement a simple javascript shell
 * based on V8.  This includes initializing V8 with command line options,
 * creating global functions, compiling and executing strings.
 *
 * For a more sophisticated shell, consider using the debug shell D8.
 */
using namespace Mordor;

int g_argc = 0;
char** g_argv = NULL;

MORDOR_MAIN(int argc, char* argv[])
{
    g_argc = argc;
    g_argv = argv;

    try {
        Config::loadFromCommandLine(argc, argv);
    } catch (std::invalid_argument &ex) {
        ConfigVarBase::ptr configVar = Config::lookup(ex.what());
        MORDOR_ASSERT(configVar);
        std::cerr << "Invalid value for " << type_name(*configVar) << ' ' << configVar->name() << ": "
                << configVar->description() << std::endl;
        return 1;
    }
    Config::loadFromEnvironment();

    int result = 0;

    WorkerPool pool;
    Mordor::Test::MD_Runner runner(pool);

    return result;
}

