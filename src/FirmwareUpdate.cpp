#include "FirmwareUpdate.h"

FirmwareUpdate::FirmwareUpdate(const char* filename, const char* version)
    : filename(filename), version(version), fileSize(0), sentChunks(0) {}

bool FirmwareUpdate::begin() {
    if (!SPIFFS.begin(true)) {
        Serial.println("Failed to mount SPIFFS");
        return false;
    }

    file = SPIFFS.open(filename, "r");
    if (!file) {
        Serial.println("Failed to open firmware file");
        return false;
    }

    fileSize = file.size();
    totalChunks = (fileSize + CHUNK_SIZE - 1) / CHUNK_SIZE;
    return true;
}

void FirmwareUpdate::end() {
    if (file) file.close();
}

String FirmwareUpdate::getVersion() const { return version; }

size_t FirmwareUpdate::getFileSize() const { return fileSize; }

size_t FirmwareUpdate::getTotalChunks() const { return totalChunks; }

float FirmwareUpdate::getProgress() const { return static_cast<float>(sentChunks) / totalChunks; }

bool FirmwareUpdate::getNextChunk(uint8_t* buffer, size_t& bytesRead) {
    if (!file || file.available() == 0) return false;
    bytesRead = file.read(buffer, CHUNK_SIZE);
    sentChunks++;
    return true;
}