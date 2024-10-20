#ifndef FIRMWARE_UPDATE_H
#define FIRMWARE_UPDATE_H

#include <Arduino.h>
#include <SPIFFS.h>

class FirmwareUpdate {
public:
    FirmwareUpdate(const char* filename, const char* version);
    bool begin();
    void end();
    String getVersion() const;
    size_t getFileSize() const;
    size_t getTotalChunks() const;
    float getProgress() const;
    bool getNextChunk(uint8_t* buffer, size_t& bytesRead);

private:
    const char* filename;
    const char* version;
    File file;
    size_t fileSize;
    size_t totalChunks;
    size_t sentChunks;
    const size_t CHUNK_SIZE = 512;
};

#endif 