#ifndef smhooks_h
#define smhooks_h

#include <map>

#include <ArduinoJson.h>
#include <StateMachine.h>

#include "../SyncOutElementConfig/SyncOutElementConfig.h"
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
    JsonVariant _options;
    StateMachineController *_sm;
    GatewayClient *_gateway;
    const char *_postUrl;

    std::map<const char *, SyncOutElementConfig *, KeyCompare> _registry;

    uint16_t _collectedCount();
    size_t _collectedSize();

    void _onChange(SyncOutElementConfig *, VarStruct *);
    void _preprocess(SyncOutElementConfig *, VarStruct *);
    void _preprocessCycle(SyncOutElementConfig *, VarStruct *, unsigned long);
    void _accumulate(SyncOutElementConfig *, VarStruct *, bool);

    void _collect(SyncOutElementConfig *);
    void _collect(SyncOutElementConfig *, VarStruct *);
};

#endif