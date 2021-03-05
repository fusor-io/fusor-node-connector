#include <math.h>
#include <Arduino.h>
#include <ArduinoJson.h>

#include "../PrintWrapper/PrintWrapper.h"
#include "SMHooks.h"

void SMHooks::init(HubClient *hub,
                   PersistentStorage *persistentStorage,
                   const char *postUrl,
                   StateMachineController *sm,
                   JsonVariant options)
{
    _hub = hub;
    _persistentStorage = persistentStorage;
    _postUrl = postUrl;
    _sm = sm;
    _options = options;
    _sm->setHooks(this);

    if (!options.is<JsonObject>())
        return;

    for (JsonPair option : (JsonObject)options)
    {
        JsonVariant optionConfig = option.value();
        if (!optionConfig.is<JsonObject>())
            continue;

        const char *varName = option.key().c_str();

        SyncOutElementConfig *elementConfig = new SyncOutElementConfig(varName, optionConfig);
        _registry[varName] = elementConfig;
    }
}

void SMHooks::onVarUpdate(const char *name, VarStruct *value)
{
    // handle persistent value saving
    _persistentStorage->saveOnUpdate(name);

    // check if variable is tracked
    if (!_registry.count(name))
        return;

    SyncOutElementConfig *options = _registry[name];

    switch (options->syncType)
    {

    case T_INSTANT:
        _collect(options, value);
        break;

    case T_ON_CHANGE:
        _onChange(options, value);
        break;

    case T_PREPROCESS:
        _preprocess(options, value);
        break;
    }
}

void SMHooks::setVar(const char *varName, float value)
{
    if (_sm->getVarFloat(varName) != value)
        _sm->setVar(varName, value, false);
}

void SMHooks::setVar(const char *varName, long int value)
{
    if (_sm->getVarInt(varName) != value)
        _sm->setVar(varName, value, false);
}

void SMHooks::afterCycle(unsigned long cycleNum)
{
    if (cycleNum == 1)
        _persistentStorage->saveOnFirstCycle();

    size_t jsonSize = _collectedSize();

    if (jsonSize)
    {
        DynamicJsonDocument output(jsonSize);
        emit(&output);

        size_t size = measureMsgPack(output);
        uint8_t buffer[size + 1];
        // size + 1 because serializeMsgPack sometimes eats last byte
        serializeMsgPack(output, (char *)buffer, size + 1);

        _hub->postMsgPack(_postUrl, buffer, size);
    }
}

void SMHooks::emit(DynamicJsonDocument *output)
{
    Serial << F("Emitting\n");

    for (auto const &item : _registry)
    {
        const char *name = item.first;
        SyncOutElementConfig *options = item.second;
        if (options->canEmit)
        {
            options->canEmit = false;
            if (options->accumulator.type == VAR_TYPE_FLOAT)
                (*output)[name] = options->accumulator.vFloat;
            else
                (*output)[name] = options->accumulator.vInt;

            Serial << options->name << " = " << options->accumulator.vInt << "\n";
        }
    }
}

size_t SMHooks::_collectedSize()
{
    uint16_t count = _collectedCount();
    return count ? JSON_OBJECT_SIZE(count) : 0;
}

uint16_t SMHooks::_collectedCount()
{
    uint16_t count = 0;

    for (auto const &item : _registry)
    {
        const char *name = item.first;

        SyncOutElementConfig *options = item.second;
        if (options->canEmit)
            count++;
    }
    return count;
}

void SMHooks::_collect(SyncOutElementConfig *options, VarStruct *value)
{
    options->accumulator = *value;
    _collect(options);
}

void SMHooks::_collect(SyncOutElementConfig *options)
{
    options->canEmit = true;
}

void SMHooks::_onChange(SyncOutElementConfig *options, VarStruct *value)
{
    if (options->updateCounter)
    {
        if (value->type == VAR_TYPE_FLOAT)
        {
            if (abs(value->vFloat - options->accumulator.vFloat) >= options->threshold)
            {
                _collect(options, value);
                options->accumulator = *value;
            }
        }
        else
        {
            if ((float)abs(value->vInt - options->accumulator.vInt) >= options->threshold)
            {
                _collect(options, value);
                options->accumulator = *value;
            }
        }
    }
    else
    {
        _collect(options, value);
        options->updateCounter++;
    }
}

void SMHooks::_preprocess(SyncOutElementConfig *options, VarStruct *value)
{
    if (options->frameType == F_CYCLE_NUM)
    {
        _preprocessCycle(options, value, _sm->cycleNum);
    }
    else if (options->frameType == F_DURATION)
    {
        _preprocessCycle(options, value, millis());
    }
}

void SMHooks::_preprocessCycle(SyncOutElementConfig *options, VarStruct *value, unsigned long cycle)
{
    if (options->frameNum++ == 0)
    {
        options->lastEmit = cycle;
        options->updateCounter == 0;
        _accumulate(options, value, false);
        return;
    }
    if (diff(options->lastEmit, cycle) >= options->frameLength)
    {
        _accumulate(options, value, true);
        _collect(options);
        options->lastEmit = cycle;
        options->updateCounter == 0;
    }
    else
    {
        _accumulate(options, value, false);
    }
}

void SMHooks::_accumulate(SyncOutElementConfig *options, VarStruct *value, bool finalize)
{
    switch (options->preprocessing)
    {
    case P_FIRST:
        if (options->updateCounter == 0)
            options->accumulator = *value;
        break;

    case P_LAST:
        options->accumulator = *value;
        break;

    case P_AVERAGE:
        if (options->updateCounter == 0)
        {
            options->accumulator = *value;
            break;
        }
        options->accumulator.vFloat += value->vFloat;
        if (finalize)
        {
            options->accumulator.vFloat = options->accumulator.vFloat / (float)options->updateCounter;
            options->accumulator.vInt = round(options->accumulator.vFloat);
        }
        break;

    case P_MIN:
        if (options->updateCounter == 0)
        {
            options->accumulator = *value;
            break;
        }
        if (options->accumulator.vFloat > value->vFloat)
            options->accumulator.vFloat = value->vFloat;
        break;

    case P_MAX:
        if (options->updateCounter == 0)
        {
            options->accumulator = *value;
            break;
        }
        if (options->accumulator.vFloat < value->vFloat)
            options->accumulator.vFloat = value->vFloat;
        break;
    }

    if (finalize)
        options->updateCounter = 0;
    else
        options->updateCounter++;
}
