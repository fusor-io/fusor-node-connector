#ifndef localtimehandler_h
#define localtimehandler_h

#include <Arduino.h>

// lets update time only if it differs by 2 or more seconds
#define MIN_DIFFERENCE 2

class LocalTimeHandler
{
public:
    void update(String);

private:
    bool _initialized = false;
    unsigned long _lastSyncTime;

    uint8_t _translateMonth(String);

    time_t _timeDiff(time_t, time_t);
};

#endif
