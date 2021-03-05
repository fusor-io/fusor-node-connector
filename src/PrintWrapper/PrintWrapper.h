#ifndef ncprintwrapper_h
#define ncprintwrapper_h

#include <Arduino.h>

extern bool __nc_serial_enabled;

template <class T>
inline Print &operator<<(Print &stream, T arg)
{
    if (__nc_serial_enabled)
        stream.print(arg);
    return stream;
}

#endif