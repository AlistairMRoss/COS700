#ifndef FIRMWARE_READER_H
#define FIRMWARE_READER_H

#include <Arduino.h>
#include "esp_ota_ops.h"
#include "esp_partition.h"

class FirmwareReader {
public:
    FirmwareReader();
    ~FirmwareReader();

    bool begin();
    size_t getFirmwareSize();
    bool readChunk(uint8_t* buffer, size_t offset, size_t length);
    void printHex(uint8_t* buffer, size_t length);

private:
    const esp_partition_t* runningPartition;
    size_t firmwareSize;
};

#endif