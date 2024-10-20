#include "FirmwareReader.h"

FirmwareReader::FirmwareReader() : runningPartition(nullptr), firmwareSize(0) {}

FirmwareReader::~FirmwareReader() {}

bool FirmwareReader::begin() {
    runningPartition = esp_ota_get_running_partition();
    if (runningPartition == nullptr) {
        Serial.println("Failed to get running partition");
        return false;
    }
    
    firmwareSize = runningPartition->size;
    return true;
}

size_t FirmwareReader::getFirmwareSize() {
    return firmwareSize;
}

bool FirmwareReader::readChunk(uint8_t* buffer, size_t offset, size_t length) {
    if (runningPartition == nullptr) {
        Serial.println("Partition not initialized");
        return false;
    }
    
    if (offset + length > firmwareSize) {
        Serial.println("Read request exceeds firmware size");
        return false;
    }
    
    esp_err_t result = esp_partition_read(runningPartition, offset, buffer, length);
    if (result != ESP_OK) {
        Serial.printf("Failed to read partition: %s\n", esp_err_to_name(result));
        return false;
    }
    
    return true;
}
