#ifndef MD_V8_WRAPPER_H_
#define MD_V8_WRAPPER_H_

#include <memory>

#include <v8.h>

namespace Mordor {
namespace Test {

class MD_Worker;

class MD_V8Wrapper : public std::enable_shared_from_this<MD_V8Wrapper>
{
public:
    MD_V8Wrapper(const char* name):name_(name){
        isolate_ = v8::Isolate::New();
        isolate_->Enter();
    }

    virtual ~MD_V8Wrapper(){
        isolate_->Exit();
        isolate_->Dispose();
    }

    // Creates a new execution environment containing the built-in
    // functions.
    virtual v8::Handle<v8::Context> createContext();

    void setName(const char* name){
        name_ = name;
    }
    bool execString(
            v8::Handle<v8::String> source,
            bool print_result,
            bool report_exceptions);
    bool execString(const char* str, bool print_result, bool report_exceptions);
    bool execString(const std::string& str, bool print_result, bool report_exceptions);

    v8::Isolate* getIsolate(){
        return this->isolate_;
    }
    v8::Handle<v8::Context> getContext(){
        return this->context_;
    }

    bool isRunning(){
        return running_;
    }

    template <typename T>
    v8::Handle<v8::String> toV8String(T str){
        return MD_V8Wrapper::toV8String(isolate_, str);
    }

public:
    static v8::Handle<v8::String> toV8String(v8::Isolate* isolate, const char* str){
        return v8::String::NewFromUtf8(isolate, str);
    }

    static v8::Handle<v8::String> toV8String(v8::Isolate* isolate, const std::string& str){
        return MD_V8Wrapper::toV8String(isolate, str.c_str());
    }

protected:
    static void init(int argc, char** argv);
    static void shutdown();

    static std::shared_ptr<MD_V8Wrapper> getWrapper(){
        if(!s_curr_.get())
            s_curr_ = std::make_shared<MD_V8Wrapper>("md_shell");
        return s_curr_->shared_from_this();
    }

public:
    class Scope{
    public:
        Scope(int argc, char** argv){
            MD_V8Wrapper::init(argc, argv);
        }
        ~Scope(){
            MD_V8Wrapper::shutdown();
        }

        std::shared_ptr<MD_V8Wrapper> getWrapper(){
            return MD_V8Wrapper::getWrapper();
        }
    };

protected:

    // The callback that is invoked by v8 whenever the JavaScript 'print'
    // function is called.  Prints its arguments on stdout separated by
    // spaces and ending with a newline.
    static void Print(const v8::FunctionCallbackInfo<v8::Value>& args);

    // The callback that is invoked by v8 whenever the JavaScript 'read'
    // function is called.  This function loads the content of the file named in
    // the argument into a JavaScript string.
    static void Read(const v8::FunctionCallbackInfo<v8::Value>& args);

    // The callback that is invoked by v8 whenever the JavaScript 'load'
    // function is called.  Loads, compiles and executes its argument
    // JavaScript file.
    static void Load(const v8::FunctionCallbackInfo<v8::Value>& args);

    // The callback that is invoked by v8 whenever the JavaScript 'quit'
    // function is called.  Quits.
    static void Quit(const v8::FunctionCallbackInfo<v8::Value>& args);

    static void Version(const v8::FunctionCallbackInfo<v8::Value>& args);

public:
    static std::shared_ptr<MD_V8Wrapper> s_curr_;
private:
    std::string name_;
    v8::Handle<v8::Context> context_;
    v8::Isolate* isolate_;
    bool running_{true};

    static bool s_inited_;
    static int s_argc_;
    static char** s_argv_;
    static std::unique_ptr<v8::Platform> s_platform_;
    static std::unique_ptr<MD_Worker> s_worker_;
};

} } // namespace Mordor::Test

#endif // MD_V8_WRAPPER_H_
