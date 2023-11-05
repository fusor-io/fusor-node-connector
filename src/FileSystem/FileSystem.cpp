#include "FileSystem.h"
#include "../PrintWrapper/PrintWrapper.h"

/**
 * Abstraction over local file system.
 * TODO: move to LittleFS at some point
 * @see https://arduino-esp8266.readthedocs.io/en/latest/filesystem.html
 **/

bool FileSystem::begin(bool formatOnFail)
{
    if (_begin(formatOnFail))
        return true;

    unsigned long chipSize = ESP.getFlashChipSize();

#ifdef ESP32
    Serial << F("Failed accessing file system")
        << F("\nChip size: ") << chipSize 
        << "\n";
#else
    Serial << F("Failed accessing file system\nChip id: ") << ESP.getFlashChipId()
        << F("\nChip size: ") << chipSize 
        << F("\nReal size: ") << ESP.getFlashChipRealSize() << "\n";
#endif

    if (chipSize > 0)
        Serial << F("Try changing board in Arduino IDE, compile, change back, compile, upload\n");

    return false;
}

bool FileSystem::_begin(bool formatOnFail)
{
#ifdef ESP32
    return SPIFFS.begin(formatOnFail);
#else
    bool success = SPIFFS.begin();
    if (!success && formatOnFail)
    {
        Serial << F("Failed opening SPIFFS. Trying to format.\n");
        if (SPIFFS.format())
            return true;
        else
        {
            Serial << F("Formatting failed\n");
            return false;
        }
    }
    return success;
#endif
}

void FileSystem::end()
{
    SPIFFS.end();
}

bool FileSystem::exists(const char *path)
{
    return SPIFFS.exists(path);
}

size_t FileSystem::totalBytes()
{
#ifdef ESP32
    return SPIFFS.totalBytes();
#else
    FSInfo fs_info;
    SPIFFS.info(fs_info);
    return fs_info.totalBytes;
#endif
}

File FileSystem::open(const char *path, const char *mode)
{
    return SPIFFS.open(path, mode);
}

void FileSystem::remove(const char *path)
{
    SPIFFS.remove(path);
}