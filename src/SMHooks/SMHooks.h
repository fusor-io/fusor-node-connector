#ifndef smhooks_h
#define smhooks_h

#include <map>

#include <ArduinoJson.h>
#include <StateMachine.h>

#include "../SyncOptions/SyncOptions.h"
#include "../Utils/Utils.h"

class SMHooks : public Hooks
{
public:
    void onVarUpdate(const char *, VarStruct *);
    void afterCycle();

    void bind(StateMachineController *, JsonVariant);
    void emit(DynamicJsonDocument *output);

private:
    JsonVariant _syncOptions;
    StateMachineController *_sm;
    std::map<const char *, SyncOptions *, KeyCompare> _registry;

    uint16_t _collectedCount();
    size_t _collectedSize();

    void _onChange(SyncOptions *, VarStruct *);
    void _preprocess(SyncOptions *, VarStruct *);
    void _preprocessCycle(SyncOptions *, VarStruct *, unsigned long);
    void _accumulate(SyncOptions *, VarStruct *, bool);

    void _collect(SyncOptions *);
    void _collect(SyncOptions *, VarStruct *);
};

#endif