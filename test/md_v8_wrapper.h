#ifndef MD_V8_WRAPPER_H_
#define MD_V8_WRAPPER_H_

#include <memory>

#include <v8.h>

namespace Mordor {
namespace Test {

class MD_V8Wrapper : public std::enable_shared_from_this<MD_V8Wrapper>
{
public:
    static std::shared_ptr<MD_V8Wrapper> getWrapper(){
        if(!s_curr_wrapper_.get())
            s_curr_wrapper_.reset(new MD_V8Wrapper());

        return s_curr_wrapper_->shared_from_this();
    }

    MD_V8Wrapper(){
        isolate_ = v8::Isolate::New();
        isolate_->Enter();
        name_= v8::String::Empty(isolate_);
    }

    virtual ~MD_V8Wrapper(){
        isolate_->Exit();
        isolate_->Dispose();
    }

    // Creates a new execution environment containing the built-in
    // functions.
    virtual v8::Handle<v8::Context> createContext();

    void setName(const char* name){
        name_ = toV8String(name);
    }

    bool execString(const char* str, bool print_result, bool report_exceptions);

    v8::Isolate* getIsolate(){
        return this->isolate_;
    }
    v8::Handle<v8::Context> getContext(){
        return this->curr_context_;
    }

    bool isRunning(){
        return running_;
    }

protected:
    static void init(int argc, char** argv);
    static void shutdown();

public:
    class Scope{
    public:
        Scope(int argc, char** argv){
            MD_V8Wrapper::init(argc, argv);
        }
        ~Scope(){
            MD_V8Wrapper::shutdown();
        }
    };

protected:

    v8::Handle<v8::String> toV8String(const char* str){
        return v8::String::NewFromUtf8(isolate_, str);
    }

    v8::Handle<v8::String> toV8String(const std::string& str){
        return toV8String(str.c_str());
    }

    // The callback that is invoked by v8 whenever the JavaScript 'print'
    // function is called.  Prints its arguments on stdout separated by
    // spaces and ending with a newline.
    void Print(const v8::FunctionCallbackInfo<v8::Value>& args);

    // The callback that is invoked by v8 whenever the JavaScript 'read'
    // function is called.  This function loads the content of the file named in
    // the argument into a JavaScript string.
    void Read(const v8::FunctionCallbackInfo<v8::Value>& args);

    // The callback that is invoked by v8 whenever the JavaScript 'load'
    // function is called.  Loads, compiles and executes its argument
    // JavaScript file.
    void Load(const v8::FunctionCallbackInfo<v8::Value>& args);

    // The callback that is invoked by v8 whenever the JavaScript 'quit'
    // function is called.  Quits.
    static void Quit(const v8::FunctionCallbackInfo<v8::Value>& args);

    static void Version(const v8::FunctionCallbackInfo<v8::Value>& args);

private:
    v8::Handle<v8::String> name_;
    v8::Handle<v8::Context> curr_context_;
    v8::Isolate* isolate_;
    bool running_{true};

    static bool s_inited_;
    static int s_argc_;
    static char** s_argv_;
    static std::shared_ptr<MD_V8Wrapper> s_curr_wrapper_;
    static std::unique_ptr<v8::Platform> s_platform_;
};

} } // namespace Mordor::Test

#endif // MD_V8_WRAPPER_H_
