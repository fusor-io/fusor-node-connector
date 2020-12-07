#include "SyncInOptions.h"

SyncInOptions::SyncInOptions()
{
}

void SyncInOptions::init(JsonVariant options, const char *baseUrl)
{
    if (!options.is<JsonObject>())
        return;

    if (options.containsKey(SYNC_DELAY))
    {
        JsonVariant delayVar = options[SYNC_DELAY];
        if (delayVar.is<unsigned long>())
            delay = delayVar.as<unsigned long>();
    }

    if (!options.containsKey(SYNC_FIELDS))
        return;

    JsonVariant fields = options[SYNC_FIELDS];
    if (!fields.is<JsonArray>())
        return;

    _buildRequestUrl(fields.as<JsonArray>(), baseUrl);
}

void SyncInOptions::_buildRequestUrl(JsonArray fields, const char *baseUrl)
{
    uint16_t urlLen = _calculateUrlQuerySize(fields) + strlen(baseUrl);

    // allocate memory (one-time action, no fragmentation risk)
    char *url = new char[urlLen];

    strcpy((char *)baseUrl, url);

    uint8_t count = 0;
    for (JsonVariant field : fields)
    {
        if (!field.is<char *>())
            continue;

        char *fieldName = (char *)field.as<char *>();

        if (strlen(fieldName) > 0)
        {
            strcat((char *)(count++ ? "&" : "?"), url);
            strcat(fieldName, url);
        }
    }

    requestUrl = (const char *)url;
}

uint16_t SyncInOptions::_calculateUrlQuerySize(JsonArray fields)
{
    uint16_t size = 0;
    uint16_t fieldCount = 0;

    // sum up lengths of all field names
    for (JsonVariant field : fields)
    {
        if (!field.is<char *>())
            continue;

        fieldCount++;
        size += strlen(field.as<char *>());
    }

    if (!fieldCount)
        return 0;

    // add '?' and '&' inside query path, eg. ?node1.param1&node2.param3
    size += fieldCount;

    return size;
}
