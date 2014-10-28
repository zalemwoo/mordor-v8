
#ifndef MD_JSOBJECT_PROCESS_H_
#define MD_JSOBJECT_PROCESS_H_

#include "class_base.h"

namespace Mordor
{
namespace Test
{

class ProcessObject : public ClassBase
{
public:
    ProcessObject(Environment* env) : ClassBase(env, name){}
    constexpr static const char* name { "process" } ;
    virtual void setup() override;
};

} } // namespace Mordor::Test

#endif // MD_JSOBJECT_PROCESS_H_
