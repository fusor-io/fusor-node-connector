#include <TimeLib.h>
#include "LocalTimeHandler.h"

/**
 * Handles local system time by interpreting Date header form the HTTP requests.
 * Time is in GMT (0 offset). Please adjust to your device time zone,
 * if providing for the user.
 */

void LocalTimeHandler::update(String timeStamp)
{
    // Time format: "Wed, 21 Oct 2015 07:28:00 GMT"
    //               01234567890123456789012345678

    // All time related types and functions are from TimeLib.h
    // https://github.com/PaulStoffregen/Time

    tmElements_t tm;

    tm.Day = (uint8_t)timeStamp.substring(5, 7).toInt();
    tm.Month = _translateMonth(timeStamp.substring(8, 11));
    tm.Year = (uint8_t)timeStamp.substring(12, 16).toInt();
    tm.Hour = (uint8_t)timeStamp.substring(17, 19).toInt();
    tm.Minute = (uint8_t)timeStamp.substring(20, 22).toInt();
    tm.Second = (uint8_t)timeStamp.substring(23, 25).toInt();

    time_t time = makeTime(tm);

    if (_timeDiff(now(), time) >= MIN_DIFFERENCE || !_initialized)
    {
        Serial.print(F("Local system time updated to: "));
        Serial.println(timeStamp.substring(5, 25));
        setTime(time);
        _initialized = true;
    }
}

time_t LocalTimeHandler::_timeDiff(time_t a, time_t b)
{
    return a > b ? a - b : b - a;
}

uint8_t LocalTimeHandler::_translateMonth(String monthStr)
{
    String months = "janfebmaraprmayjunjulaugsepoctnovdec";
    monthStr.toLowerCase();
    return (uint8_t)(months.indexOf(monthStr) / 3 + 1);
}