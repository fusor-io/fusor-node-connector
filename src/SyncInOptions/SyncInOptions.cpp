#include "SyncInOptions.h"

SyncInOptions::SyncInOptions(JsonVariant options, const char *baseUrl)
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
    char *url = new char[urlLen];

    strcpy((char *)baseUrl, url);

    uint8_t count = 0;
    for (JsonVariant field : fields)
    {
        if (!field.is<char *>())
            continue;

        strcat((char *)(count++ ? "&" : "?"), url);
        char *fieldName = (char *)field.as<char *>();
        strcat(fieldName, url);
    }

    _requestUrl = (const char *)url;
}

uint16_t SyncInOptions::_calculateUrlQuerySize(JsonArray fields)
{
    uint16_t size = 0;
    uint16_t fieldCount = 0;

    for (JsonVariant field : fields)
    {
        if (!field.is<char *>())
            continue;

        fieldCount++;
        size += strlen(field.as<char *>());
    }

    if (!fieldCount)
        return 0;

    size += fieldCount; // ? and & inside query path, eg. ?node1.param1&node2.param3

    return size;
}
