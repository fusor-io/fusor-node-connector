#include "FileSystem.h"

/**
 * Abstraction over local file system.
 * TODO: move to LittleFS at some point
 * @see https://arduino-esp8266.readthedocs.io/en/latest/filesystem.html
 **/

bool FileSystem::begin(bool formatOnFail)
{
#ifdef ESP32
    return SPIFFS.begin(formatOnFail);
#else
    bool success = SPIFFS.begin();
    if (!success && formatOnFail)
    {
        Serial.println(F("Failed opening SPIFFS. Trying to format."));
        if (SPIFFS.format())
            return true;
        else
        {
            Serial.println(F("Formatting failed"));
            Serial.print(F("Chip size: "));
            Serial.println(ESP.getFlashChipSize());
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