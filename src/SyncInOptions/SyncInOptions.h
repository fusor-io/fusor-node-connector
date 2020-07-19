#ifndef syncinoptions_h
#define syncinoptions_h

#include <ArduinoJson.h>

/**
 * SyncIn options:
 * {
 *   "f": string[]
 *   "d": number
 * }
 * 
 * "f" - array of field names to read from gateway and write to StateMachine store. Field - node-id.field-name
 * "d" - ms to delay between updates
 */

class SyncInOptions
{
public:
    SyncInOptions(const char *, JsonVariant);
};

#endif
