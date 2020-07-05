#ifndef smhooks_h
#define smhooks_h

#include <map>

#include <ArduinoJson.h>
#include <StateMachine.h>

#include "../SyncOptions/SyncOptions.h"
#include "../Utils/Utils.h"
#include "../GatewayClient/GatewayClient.h"

class SMHooks : public Hooks
{
public:
    void init(GatewayClient *, const char *, StateMachineController *, JsonVariant);
    void emit(DynamicJsonDocument *output);

    void onVarUpdate(const char *, VarStruct *);
    void afterCycle();

private:
    JsonVariant _syncOptions;
    StateMachineController *_sm;
    GatewayClient *_gateway;
    const char *_postUrl;

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