#include "PersistentStorage.h"
#include "../Utils/Utils.h"

PersistentStorage::PersistentStorage() : _tracker()
{
}

void PersistentStorage::init(JsonVariant options, Store *store)
{
    if (!options.is<JsonObject>())
        return;

    _options = options.as<JsonObject>();
    _store = store;

    _initialized = true;
    Serial.println(F("Persistent storage initialized"));
}

void PersistentStorage::save(const char *varName)
{
    // check if timeout reached and var is updated
    if (!_canSave(varName, false))
        return;

    Serial.print(F("Writing variable to flash: "));
    Serial.println(varName);

    _save();
}

void PersistentStorage::saveOnReboot()
{
    // check if timeout reached and var is updated
    if (!_canSaveOnReboot())
        return;

    Serial.println(F("Writing variables to flash"));

    _save();
}

void PersistentStorage::load()
{
    if (!_initialized)
        return;

    Serial.println(F("Loading variables from flash"));

    bool success = SPIFFS.begin();
    if (!success)
        return;

    if (!SPIFFS.exists(STORAGE_FILE))
        return;

    File file = SPIFFS.open(STORAGE_FILE, "r");

    int size = file.size();
    Serial.print(F("File size: "));
    Serial.println(size);

    size_t nameLen;
    VarStruct var;
    while (file.available())
    {
        file.read((uint8_t *)&nameLen, sizeof(nameLen));
        char varName[nameLen];
        file.read((uint8_t *)varName, nameLen);
        size_t successBytes = file.read((uint8_t *)&var, sizeof(var));

        if (successBytes == sizeof(var))
        {
            Serial.print(varName);
            Serial.print("=");
            if (var.type == VAR_TYPE_FLOAT)
            {
                _store->setVar(varName, var.vFloat, false);
                Serial.println(var.vFloat);
            }
            else
            {
                _store->setVar(varName, var.vInt, false);
                Serial.println(var.vInt);
            }
        }
    }
    file.close();

    SPIFFS.end();
}

bool PersistentStorage::_canSave(const char *varName, bool onReboot)
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

    // we need different check on reboot
    if (onReboot)
    {
        JsonVariant onRestart = _options[varName][ON_RESTART];
        if (onRestart.isNull() || !onRestart.is<bool>() || !onRestart.as<bool>())
            return false;
    }
    else
    {
        // check if timeout elapsed
        if (getTimeout(trackedValue->updatedAt) < _minTimeout(varName))
            return false;
    }

    // check if value actually is changed
    return oldValue->vInt != newValue->vInt || oldValue->vFloat != newValue->vFloat;
}

bool PersistentStorage::_canSaveOnReboot()
{
    if (!_initialized)
        return false;

    // check if at least one variable needs saving
    for (JsonPair option : _options)
    {
        // check if it is eligible and was updated
        if (_canSave(option.key().c_str(), true))
            return true;
    }

    return false;
}

void PersistentStorage::_save()
{
    if (!_initialized)
        return;

    // prepare for saving to EEPROM
    bool success = SPIFFS.begin();
    if (!success)
        return;

    File file = SPIFFS.open(STORAGE_FILE, "w");

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

    SPIFFS.end();
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