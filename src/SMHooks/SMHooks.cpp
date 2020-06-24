#include <Arduino.h>

#include <math.h>
#include <Arduino.h>

#include "SMHooks.h"

void SMHooks::bind(StateMachineController *sm, JsonVariant syncOptions)
{
    _sm = sm;
    _syncOptions = syncOptions;
    _sm->setHooks(this);

    if (!syncOptions.is<JsonObject>())
        return;

    for (JsonPair option : (JsonObject)syncOptions)
    {
        JsonVariant optionConfig = option.value();
        if (!optionConfig.is<JsonObject>())
            continue;

        const char *varName = option.key().c_str();

        SyncOptions *syncOptions = new SyncOptions(varName, optionConfig);
        _registry[varName] = syncOptions;
    }
}

void SMHooks::onVarUpdate(const char *name, VarStruct *value)
{
    if (!_registry.count(name))
        return;

    SyncOptions *options = _registry[name];

    switch (options->syncType)
    {

    case T_INSTANT:
        _collect(options, value);
        break;

    case T_ON_CHANGE:
        _onChange(options, value);
        break;

    case T_PREPROCESS:

        break;
    }

    options->updateCounter++;
}

void SMHooks::emit(DynamicJsonDocument *output)
{
    for (auto const &item : _registry)
    {
        const char *name = item.first;
        SyncOptions *options = item.second;
        if (options->canEmit)
        {
            options->updateCounter = 0;
            options->canEmit = false;
            options->lastEmitTime = millis();
            if (options->accumulator.type == VAR_TYPE_FLOAT)
                (*output)[name] = options->accumulator.vFloat;
            else
                (*output)[name] = options->accumulator.vInt;
        }
    }
}

void SMHooks::_collect(SyncOptions *options, VarStruct *value)
{
    options->accumulator = *value;
    _collect(options);
}

void SMHooks::_collect(SyncOptions *options)
{
    options->canEmit = true;

    Serial.print("Collected ");
    Serial.print(options->name);
    Serial.print(" = ");
    Serial.println(options->accumulator.vInt);
}

void SMHooks::_onChange(SyncOptions *options, VarStruct *value)
{
    if (options->updateCounter)
    {
        if (value->type == VAR_TYPE_FLOAT)
        {
            if (abs(value->vFloat - options->accumulator.vFloat) >= options->threshold)
                _collect(options, value);
        }
        else
        {
            if ((float)abs(value->vInt - options->accumulator.vInt) >= options->threshold)
                _collect(options, value);
        }
    }
    else
    {
        _collect(options, value);
    }
}

void SMHooks::_preprocess(SyncOptions *options, VarStruct *value)
{
}