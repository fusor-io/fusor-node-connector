#ifndef PersistentStorage_h
#define PersistentStorage_h

#include <map>
#include <ArduinoJson.h>
#include <StateMachine.h>
#include <Arduino.h>
#ifdef ESP32
#include <SPIFFS.h>
#else
#include <FS.h>
#endif

#include "RecordStruct.h"

/*
 * Persistent storage is used to save choosen variables to ERPROM and load them
 * as default values after the restart,
 *
 * Options for single field to be stored into EEPROM
 *  {
 *    DEFAULT_VALUE: int | float,
 *    MIN_TIMEOUT: int, // Minimal time in seconds between updates to EEPROM. If not set - defaults to 6 hours.
 *                      // ESP32 should survive at least 10.000 rewrites.
 *                      // Rewriting once per 6 hours will result in ~7 years of service
 *                      // Note: timer is reset on each restart (no RTC)
 *    AFTER_FIRST_CYCLE: bool, // saves after the first StateMachine cycle
 *    ON_RESTART: bool, // saves before restart initiated by NodeConnector
 *  }
 */

// TODO implement list for saving update times

#define DEFAULT_VALUE "d"
#define MIN_TIMEOUT "t"
#define AFTER_FIRST_CYCLE "f"
#define ON_RESTART "r"

// 6 hours in seconds
#define DEFAULT_MIN_TIMEOUT 21600ul

const char STORAGE_FILE[] = "/variables.bin";

class PersistentStorage
{
public:
    PersistentStorage();
    void init(JsonVariant, Store *);

    void save(const char *);
    void saveOnReboot();
    void load();

private:
    bool _initialized = false;
    JsonObject _options;
    Store *_store;

    std::map<char *, RecordStruct *, KeyCompare> _tracker;
    KeyCreate _keyCreator;

    bool _canSave(const char *, bool);
    bool _canSaveOnReboot();
    unsigned long _minTimeout(const char *);
    void _save();
};

#endif