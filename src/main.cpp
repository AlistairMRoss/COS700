#include <Arduino.h>
#include "MeshNetwork.h"
#include "Ledger.h"
#include "WifiFunctions.h"

FirmwareReader firmwareReader;
const size_t CHUNK_SIZE = 256;

#define MESH_PREFIX     "join"
#define MESH_PASSWORD   "12345678"
#define MESH_PORT       5555

MeshNetwork meshNetwork(MESH_PREFIX, MESH_PASSWORD, MESH_PORT);
Ledger ledger;
WifiFunctions wifiFunctions;


#define REQUIRED_NODES 3
bool allNodesConnected = false;

void setup() {
    Serial.begin(115200);
    delay(1000);
    wifiFunctions.setup();
    delay(10000);
    meshNetwork.init();
    delay(1000);
    ledger.init();
    
    while (!allNodesConnected) {
        meshNetwork.update();
        if (meshNetwork.getConnectedNodes() >= REQUIRED_NODES - 1) {
            allNodesConnected = true;
            Serial.println("All nodes connected!");
        }
        delay(1000);
    }
    
    // String identityMessage = "IDENTITY:" + String(meshNetwork.getNodeId());
    // meshNetwork.sendMessage(identityMessage);
    // Serial.print(meshNetwork.calculateUniqueTimestampedFirmwareHash());
}

void loop() {
    meshNetwork.update();
    
    // static unsigned long lastTime = 0;
    // unsigned long currentTime = millis();
    // if (currentTime - lastTime > 5000) {
    //     lastTime = currentTime;
        
    //     // Create a new ledger entry
    //     // String entry = "Entry from " + String(meshNetwork.getNodeId()) + " at " + String(currentTime);
    //     // ledger.addEntry(entry);
        
    //     // Share the latest ledger with other nodes
    //     // String ledgerMessage = "LEDGER:" + ledger.getLatestEntry();
    //     // meshNetwork.sendMessage(ledgerMessage);
    // }
    // wifiFunctions.loop();
}
