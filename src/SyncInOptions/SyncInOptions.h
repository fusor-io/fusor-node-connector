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
 * "f" - array of field names to read from the hub and write to StateMachine store. Field - node-id.field-name
 * "d" - ms to delay between updates
 */

#define SYNC_FIELDS "f"
#define SYNC_DELAY "d"

const char HUB_REQUEST_PATH[] = "/aggregate/batch/flat";

class SyncInOptions
{
public:
    SyncInOptions();
    void init(JsonVariant, const char *);

    unsigned long delay = 60000;
    const char *requestUrl = nullptr;

private:
    void _buildRequestUrl(JsonArray, const char *);
    uint16_t _calculateUrlQuerySize(JsonArray);
};

#endif
