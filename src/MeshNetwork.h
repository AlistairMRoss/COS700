#ifndef MESH_NETWORK_H
#define MESH_NETWORK_H

#include <painlessMesh.h>
#include <vector>
#include "Ledger.h"
#include "FirmwareReader.h"
#include <mbedtls/md.h>
#include <time.h>
#include <Update.h>
#include "FirmwareUpdate.h"

class Ledger;
class MeshNetwork {
public:
    MeshNetwork(const char* prefix, const char* password, uint16_t port, FirmwareUpdate* update);
    void init();
    void update();
    void sendMessage(const String& msg);
    uint32_t getNodeId();
    size_t getConnectedNodes();

    String calculateUniqueTimestampedFirmwareHash();
    void sendUniqueTimestampedFirmwareHash();

    void startFirmwareDistribution();
    
private:
    void receivedCallback(uint32_t from, String &msg);
    void newConnectionCallback(uint32_t nodeId);
    void changedConnectionCallback();
    void nodeTimeAdjustedCallback(int32_t offset);
    void processIdentityMessage(uint32_t from, const String& msg);
    void processLedgerMessage(uint32_t from, const String& msg);
    void sendNextFirmwareChunk();
    void processFirmwareChunk(const String& msg);
    void finalizeFirmwareUpdate();

    painlessMesh mesh;
    const char* meshPrefix;
    const char* meshPassword;
    uint16_t meshPort;
    std::vector<uint32_t> connectedNodes;

    const size_t CHUNK_SIZE = 512;
    String incomingFirmwareFilename;
    std::vector<String> firmwareChunks;
    size_t expectedChunks;
    size_t receivedChunks;
    FirmwareUpdate* firmwareUpdate;

    FirmwareReader firmwareReader;

};

extern Ledger ledger;

#endif