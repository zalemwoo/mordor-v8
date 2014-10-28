#ifndef MORDOR_V8_ENV_H_
#define MORDOR_V8_ENV_H_

#include <memory>

#include "v8.h"
#include "mordor/util.h"
#include "md_v8_util_inl.h"

namespace Mordor
{

class Scheduler;

namespace Test
{

#define MD_V8_WORKERPOOL_SIZE               4
#define MD_V8_CONTEXT_EMBEDDER_DATA_INDEX   32
#define MD_V8_ISOLATE_SLOT                  3

// Strings are per-isolate primitives but Environment proxies them
// for the sake of convenience.
#define PER_ISOLATE_STRING_PROPERTIES(V)                                      \
  V(args_string, "args")                                                      \
  V(argv_string, "argv")                                                      \
  V(code_string, "code")                                                      \
  V(env_string, "env")                                                        \
  V(errno_string, "errno")                                                    \
  V(error_string, "error")                                                    \
  V(path_string, "path")                                                      \
  V(syscall_string, "syscall")                                                \


#define ENVIRONMENT_STRONG_PERSISTENT_PROPERTIES(V)                           \
  V(context, v8::Context)                                                     \
  V(binding_cache_object, v8::Object)                                         \
  V(module_load_list_array, v8::Array)                                        \


class MD_Worker;

class Environment: Mordor::noncopyable
{
public:
    static inline Environment* GetCurrent(v8::Isolate* isolate);
    static inline Environment* GetCurrent(v8::Local<v8::Context> context);
    static inline Environment* New(v8::Local<v8::Context> context, Scheduler* scheduer);
    inline void Dispose();

    static inline MD_Worker* GetCurrentWorker(v8::Isolate* isolate);
    static inline MD_Worker* GetCurrentWorker(v8::Local<v8::Context> context);

    MD_Worker* worker(){
        return worker_.get();
    }

    void AssignToContext(v8::Local<v8::Context> context);
    inline v8::Isolate* isolate() const;

    // Strings are shared across shared contexts. The getters simply proxy to
    // the per-isolate primitive.
#define V(PropertyName, StringValue)                                          \
    inline v8::Local<v8::String> PropertyName() const;
    PER_ISOLATE_STRING_PROPERTIES(V)
#undef V

#define V(PropertyName, TypeName)                                         \
    inline v8::Local<TypeName> PropertyName() const;                      \
    inline void set_ ## PropertyName(v8::Local<TypeName> value);
    ENVIRONMENT_STRONG_PERSISTENT_PROPERTIES    (V)
#undef V

    inline bool using_smalloc_alloc_cb() const;
    inline void set_using_smalloc_alloc_cb(bool value);

    inline bool using_domains() const;
    inline void set_using_domains(bool value);

    inline bool printed_error() const;
    inline void set_printed_error(bool value);

    inline void ThrowError(const char* errmsg);
    inline void ThrowTypeError(const char* errmsg);
    inline void ThrowRangeError(const char* errmsg);
    inline void ThrowErrnoException(int errorno,
                                    const char* syscall = NULL,
                                    const char* message = NULL,
                                    const char* path = NULL);

    inline static void ThrowError(v8::Isolate* isolate, const char* errmsg);
    inline static void ThrowTypeError(v8::Isolate* isolate, const char* errmsg);
    inline static void ThrowRangeError(v8::Isolate* isolate, const char* errmsg);

public:
    static std::shared_ptr<Environment> environment;
private:
    class IsolateData;

    static const int kWorkerPoolSize = MD_V8_WORKERPOOL_SIZE;
    static const int kIsolateSlot = MD_V8_ISOLATE_SLOT;
    inline explicit Environment(v8::Local<v8::Context> context);
    inline ~Environment();

    inline IsolateData* isolate_data() const;

private:
    v8::Isolate* const isolate_;
    IsolateData* const isolate_data_;

    enum ContextEmbedderDataIndex {
        kContextEmbedderDataIndex = MD_V8_CONTEXT_EMBEDDER_DATA_INDEX
    };

    bool using_smalloc_alloc_cb_;
    bool using_domains_;
    bool printed_error_;

    std::unique_ptr<MD_Worker> worker_;

#define V(PropertyName, TypeName)                                             \
  v8::Persistent<TypeName> PropertyName ## _;
    ENVIRONMENT_STRONG_PERSISTENT_PROPERTIES(V)
#undef V

    struct EnvironmentDeleter
    {
        void operator()(Environment* env){
            env->Dispose();
        }
    };

    friend EnvironmentDeleter;

    // Per-thread, reference-counted singleton.
    class IsolateData {
    public:
        static inline IsolateData* GetOrCreate(v8::Isolate* isolate);
        inline void Put();

#define V(PropertyName, StringValue)                                          \
      inline v8::Local<v8::String> PropertyName() const;
        PER_ISOLATE_STRING_PROPERTIES(V)
#undef V

    private:
        inline static IsolateData* Get(v8::Isolate* isolate);
        inline explicit IsolateData(v8::Isolate* isolate);
        inline v8::Isolate* isolate() const;
        v8::Isolate* const isolate_;

#define V(PropertyName, StringValue)                                          \
      v8::Eternal<v8::String> PropertyName ## _;
        PER_ISOLATE_STRING_PROPERTIES(V)
#undef V

        unsigned int ref_count_;
    };  // class IsolateData

}; // class Environment


v8::Local<v8::Value> ErrnoException(v8::Isolate* isolate,
                                    int errorno,
                                    const char* syscall = NULL,
                                    const char* message = NULL,
                                    const char* path = NULL);

} } // namespace Mordor::Test

#endif //  MORDOR_V8_ENV_H_
