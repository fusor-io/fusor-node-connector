#ifndef filesystem_h
#define filesystem_h

#include <Arduino.h>
#ifdef ESP32
#include <SPIFFS.h>
#else
#include <FS.h>
#endif
// https://github.com/espressif/arduino-esp32/blob/master/libraries/SPIFFS/src/SPIFFS.cpp

class FileSystem
{
public:
    bool begin(bool formatOnFail = false);
    void end();
    bool exists(const char *);
    size_t totalBytes();
    File open(const char *, const char *);
    void remove(const char *);
};

#endif