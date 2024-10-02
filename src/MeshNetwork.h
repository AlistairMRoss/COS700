#ifndef MESH_NETWORK_H
#define MESH_NETWORK_H

#include <painlessMesh.h>
#include <vector>

class MeshNetwork {
public:
    MeshNetwork(const char* prefix, const char* password, uint16_t port);
    void init();
    void update();
    void sendMessage(const String& msg);
    uint32_t getNodeId();
    size_t getConnectedNodes();
    
private:
    void receivedCallback(uint32_t from, String &msg);
    void newConnectionCallback(uint32_t nodeId);
    void changedConnectionCallback();
    void nodeTimeAdjustedCallback(int32_t offset);
    void processIdentityMessage(uint32_t from, const String& msg);
    void processLedgerMessage(uint32_t from, const String& msg);

    painlessMesh mesh;
    const char* meshPrefix;
    const char* meshPassword;
    uint16_t meshPort;
    std::vector<uint32_t> connectedNodes;
};
#endif