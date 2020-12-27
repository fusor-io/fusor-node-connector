#include "SyncOutElementConfig.h"

SyncOutElementConfig::SyncOutElementConfig(const char *varName, JsonVariant options) : accumulator(0.0f)
{
    name = varName;

    if (options.containsKey(SYNC_TYPE))
    {
        JsonVariant type = options[SYNC_TYPE];
        if (type.is<char *>())
        {
            if (strcasecmp(type, SYNC_TYPE_INSTANT) == 0)
                syncType = T_INSTANT;
            else if (strcasecmp(type, SYNC_TYPE_PREPROCESS) == 0)
                syncType = T_PREPROCESS;
            else if (strcasecmp(type, SYNC_TYPE_ON_CHANGE) == 0)
                syncType = T_ON_CHANGE;
        }
    }

    if (options.containsKey(SYNC_PREPROCESS))
    {
        JsonVariant prep = options[SYNC_PREPROCESS];
        if (prep.is<char *>())
        {
            if (strcasecmp(prep, SYNC_PREPROCESS_FIRST) == 0)
                preprocessing = P_FIRST;
            else if (strcasecmp(prep, SYNC_PREPROCESS_LAST) == 0)
                preprocessing = P_LAST;
            else if (strcasecmp(prep, SYNC_PREPROCESS_AVERAGE) == 0)
                preprocessing = P_AVERAGE;
            else if (strcasecmp(prep, SYNC_PREPROCESS_MIN) == 0)
                preprocessing = P_MIN;
            else if (strcasecmp(prep, SYNC_PREPROCESS_MAX) == 0)
                preprocessing = P_MAX;
        }
    }

    if (options.containsKey(SYNC_FRAME_TYPE))
    {
        JsonVariant frame = options[SYNC_FRAME_TYPE];
        if (frame.is<char *>())
        {
            if (strcasecmp(frame, SYNC_FRAME_TYPE_CYCLE_NUM) == 0)
                frameType = F_CYCLE_NUM;
            else if (strcasecmp(frame, SYNC_FRAME_TYPE_DURATION) == 0)
                frameType = F_DURATION;
        }
    }

    if (options.containsKey(SYNC_FRAME_LENGTH))
    {
        JsonVariant len = options[SYNC_FRAME_LENGTH];
        if (len.is<long int>())
            frameLength = len;
    }

    if (options.containsKey(SYNC_THRESHOLD))
    {
        JsonVariant th = options[SYNC_THRESHOLD];
        if (th.is<long int>())
            threshold = (float)th.as<int>();
        else if (th.is<float>())
            threshold = th;
    }
    else
    {
        threshold = 1.0f;
    }
}