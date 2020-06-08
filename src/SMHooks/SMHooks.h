#ifndef smhooks_h
#define smhooks_h

#include <StateMachine.h>

class SMHooks : public Hooks
{
public:
    void onVarUpdate(const char *, VarStruct *);
};

#endif