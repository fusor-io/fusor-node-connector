#ifndef smhooks_h
#define smhooks_h

#include <map>

#include <ArduinoJson.h>
#include <StateMachine.h>

#include "../SyncOptions/SyncOptions.h"

class SMHooks : public Hooks
{
public:
    void bind(StateMachineController *, JsonVariant);
    void onVarUpdate(const char *, VarStruct *);

    void emit(DynamicJsonDocument *output);

private:
    JsonVariant _syncOptions;
    StateMachineController *_sm;
    std::map<const char *, SyncOptions *, KeyCompare> _registry;

    void _onChange(SyncOptions *, VarStruct *);
    void _preprocess(SyncOptions *, VarStruct *);

    void _collect(SyncOptions *);
    void _collect(SyncOptions *, VarStruct *);
};

#endif