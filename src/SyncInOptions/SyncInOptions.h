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

#define SYNC_FIELDS "f"
#define SYNC_DELAY "d"

class SyncInOptions
{
public:
    SyncInOptions(JsonVariant, const char *);

    unsigned long delay = 60000;

private:
    void _buildRequestUrl(JsonArray, const char *);
    uint16_t _calculateUrlQuerySize(JsonArray);
    const char *_requestUrl = nullptr;
};

#endif
