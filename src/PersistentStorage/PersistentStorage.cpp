#include "../PrintWrapper/PrintWrapper.h"
#include "../Utils/Utils.h"

#include "PersistentStorage.h"

PersistentStorage::PersistentStorage() : _tracker(), _fs()
{
}

void PersistentStorage::init(JsonVariant options, Store *store)
{
    if (!options.is<JsonObject>())
        return;

    _options = options.as<JsonObject>();
    _store = store;

    _initialized = true;
    Serial << F("Persistent storage initialized\n");
}

void PersistentStorage::saveOnUpdate(const char *varName)
{
    // check if timeout reached and var is updated
    if (!_canSave(varName, onUpdate))
        return;

    Serial << F("Storing variable: ") << varName << "\n";
    _save();
}

void PersistentStorage::saveOnReboot()
{
    // check if at least one variable should be saved
    if (!_canSaveAnyOnEvent(onReboot))
        return;

    Serial << F("Storing storage on reboot\n");
    _save();
}

void PersistentStorage::saveOnFirstCycle()
{
    // check if at least one variable should be saved
    if (!_canSaveAnyOnEvent(onFirstCycle))
        return;

    Serial << F("Storing storage on first cycle\n");
    _save();
}

void PersistentStorage::load()
{
    if (!_initialized)
        return;

    Serial << F("Loading variables from flash\n");

    bool success = _fs.begin();
    if (!success)
        return;

    if (!_fs.exists(STORAGE_FILE))
        return;

    File file = _fs.open(STORAGE_FILE, "r");

    int size = file.size();
    Serial << F("File size: ") << size << "\n";

    size_t nameLen;
    VarStruct var;
    while (file.available())
    {
        file.read((uint8_t *)&nameLen, sizeof(nameLen));
        char varName[nameLen + 1];
        file.read((uint8_t *)varName, nameLen);
        varName[nameLen] = 0;

        size_t successBytes = file.read((uint8_t *)&var, sizeof(var));

        if (successBytes == sizeof(var))
        {
            Serial << varName << "=";
            if (var.type == VAR_TYPE_FLOAT)
            {
                _store->setVar(varName, var.vFloat, false);
                Serial << var.vFloat;
            }
            else
            {
                _store->setVar(varName, var.vInt, false);
                Serial << var.vInt;
            }
            Serial << "\n";
        }
    }
    file.close();

    _fs.end();
}

bool PersistentStorage::_canSave(const char *varName, StorageEvent event)
{
    if (!_initialized)
        return false;

    // check if this variable is tracked
    if (_options[varName].isNull())
        return false;

    // check if variable has value
    VarStruct *newValue = _store->getVar(varName);
    if (!newValue)
        return false;

    // check if this is the very first update
    // if yes, init value in our tracker
    if (!_tracker.count((char *)varName))
        _tracker[(char *)(_keyCreator.createKey(varName))] = new RecordStruct(newValue);

    // get old value from our tracker
    RecordStruct *trackedValue = _tracker[(char *)varName];
    VarStruct *oldValue = &(trackedValue->var);

    // we need different check on different event
    switch (event)
    {
    case onUpdate:
        // check if timeout elapsed (it is measured in seconds, so we divide timeout by 1000)
        if (getTimeout(trackedValue->updatedAt) / 1000 < _minTimeout(varName))
            return false;
        break;
    case onFirstCycle:
        if (!_isFlagOn(varName, AFTER_FIRST_CYCLE))
            return false;
        break;
    case onReboot:
        if (!_isFlagOn(varName, ON_RESTART))
            return false;
        break;
    }

    // check if value actually is changed
    return oldValue->vInt != newValue->vInt || oldValue->vFloat != newValue->vFloat;
}

bool PersistentStorage::_canSaveAnyOnEvent(StorageEvent event)
{
    if (!_initialized)
        return false;

    // check if at least one variable needs saving
    for (JsonPair option : _options)
    {
        // check if it is eligible and was updated
        if (_canSave(option.key().c_str(), event))
            return true;
    }

    return false;
}

void PersistentStorage::_save()
{
    Serial << F("Writing storage to flash\n");

    if (!_initialized)
        return;

    // prepare for saving to EEPROM
    bool success = _fs.begin();
    if (!success)
        return;

    File file = _fs.open(STORAGE_FILE, "w");

    // as long as we save all variables in one file, lets iterate over whole config
    unsigned long now = millis();
    for (JsonPair option : _options)
    {
        char *name = (char *)option.key().c_str();
        size_t nameLen = strlen(name);

        VarStruct *var = _store->getVar(name);
        if (var)
        {
            file.write((uint8_t *)(&nameLen), sizeof(nameLen));
            file.write((uint8_t *)name, nameLen);
            file.write((uint8_t *)var, sizeof(VarStruct));
        }

        _tracker[name]->updatedAt = now;
    }

    file.close();

    _fs.end();
    Serial << "Done\n";
}

bool PersistentStorage::_isFlagOn(const char *varName, const char *flagName)
{
    JsonVariant flag = _options[varName][flagName];
    return !flag.isNull() && flag.is<bool>() && flag.as<bool>();
}

unsigned long PersistentStorage::_minTimeout(const char *varName)
{
    JsonVariant settings = _options[varName][MIN_TIMEOUT];
    if (settings.isNull())
        return DEFAULT_MIN_TIMEOUT;
    if (!settings.is<unsigned long>())
        return DEFAULT_MIN_TIMEOUT;
    return settings.as<unsigned long>();
}