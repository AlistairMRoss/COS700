#ifndef MESH_NETWORK_H
#define MESH_NETWORK_H

#include <painlessMesh.h>
#include <vector>
#include "Ledger.h"
#include "FirmwareReader.h"
#include <mbedtls/md.h>
#include <time.h>

class Ledger;
class MeshNetwork {
public:
    MeshNetwork(const char* prefix, const char* password, uint16_t port);
    void init();
    void update();
    void sendMessage(const String& msg);
    uint32_t getNodeId();
    size_t getConnectedNodes();

    void sendFirmware(const String& filename);
    // void requestFirmware();

    String calculateUniqueTimestampedFirmwareHash();
    void sendUniqueTimestampedFirmwareHash();
    
private:
    void receivedCallback(uint32_t from, String &msg);
    void newConnectionCallback(uint32_t nodeId);
    void changedConnectionCallback();
    void nodeTimeAdjustedCallback(int32_t offset);
    void processIdentityMessage(uint32_t from, const String& msg);
    void processLedgerMessage(uint32_t from, const String& msg);

    void processFirmwareChunk(uint32_t from, const String& msg);
    void sendFirmwareChunk(uint32_t to, const String& filename, size_t chunkIndex);
    void assembleFirmware();

    painlessMesh mesh;
    const char* meshPrefix;
    const char* meshPassword;
    uint16_t meshPort;
    std::vector<uint32_t> connectedNodes;

    static const size_t CHUNK_SIZE = 512;
    String incomingFirmwareFilename;
    std::vector<String> firmwareChunks;
    size_t expectedChunks;
    size_t receivedChunks;
    
    FirmwareReader firmwareReader;

};

extern Ledger ledger;

#endif