#include <Arduino.h>
#include "FirmwareReader.h"
#include "MeshNetwork.h"
#include "Ledger.h"
#include "WifiManager.h"

FirmwareReader firmwareReader;
const size_t CHUNK_SIZE = 256;

#define MESH_PREFIX     "join"
#define MESH_PASSWORD   "12345678"
#define MESH_PORT       5555

MeshNetwork meshNetwork(MESH_PREFIX, MESH_PASSWORD, MESH_PORT);
Ledger ledger;
WiFiManager wifiManager;


#define REQUIRED_NODES 3
bool allNodesConnected = false;

void setup() {
    Serial.begin(115200);
    wifiManager.setup();
    meshNetwork.init();
    ledger.init();
    
    while (!allNodesConnected) {
        meshNetwork.update();
        if (meshNetwork.getConnectedNodes() >= REQUIRED_NODES - 1) {
            allNodesConnected = true;
            Serial.println("All nodes connected!");
        }
        delay(1000);
    }
    
    String identityMessage = "IDENTITY:" + String(meshNetwork.getNodeId());
    meshNetwork.sendMessage(identityMessage);
}

void loop() {
    meshNetwork.update();
    
    static unsigned long lastTime = 0;
    unsigned long currentTime = millis();
    if (currentTime - lastTime > 5000) {
        lastTime = currentTime;
        
        // Create a new ledger entry
        String entry = "Entry from " + String(meshNetwork.getNodeId()) + " at " + String(currentTime);
        ledger.addEntry(entry);
        
        // Share the latest ledger with other nodes
        String ledgerMessage = "LEDGER:" + ledger.getLatestEntry();
        meshNetwork.sendMessage(ledgerMessage);
    }
    wifiManager.loop();
}


    // if (!firmwareReader.begin()) {
    //     Serial.println("Failed to initialize FirmwareReader");
    //     return;
    // }

    // size_t firmwareSize = firmwareReader.getFirmwareSize();
    // Serial.printf("Firmware size: %u bytes\n", firmwareSize);

    // uint8_t* buffer = new uint8_t[CHUNK_SIZE];
    // for (size_t offset = 0; offset < firmwareSize; offset += CHUNK_SIZE) {
    //     size_t chunkSize = min(CHUNK_SIZE, firmwareSize - offset);
    //     if (firmwareReader.readChunk(buffer, offset, chunkSize)) {
    //         Serial.printf("Offset %u:\n", offset);
    //         firmwareReader.printHex(buffer, chunkSize);
    //     } else {
    //         Serial.printf("Failed to read chunk at offset %u\n", offset);
    //     }
    // }
    // delete[] buffer;