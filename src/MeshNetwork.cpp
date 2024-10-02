#include "MeshNetwork.h"
#include "Ledger.h"

extern Ledger ledger;

MeshNetwork::MeshNetwork(const char* prefix, const char* password, uint16_t port)
    : meshPrefix(prefix), meshPassword(password), meshPort(port) {}

void MeshNetwork::init() {
    WiFi.mode(WIFI_AP_STA);
    mesh.setDebugMsgTypes(ERROR | STARTUP);
    mesh.init(meshPrefix, meshPassword, meshPort);
    mesh.onReceive(std::bind(&MeshNetwork::receivedCallback, this, std::placeholders::_1, std::placeholders::_2));
    mesh.onNewConnection(std::bind(&MeshNetwork::newConnectionCallback, this, std::placeholders::_1));
    mesh.onChangedConnections(std::bind(&MeshNetwork::changedConnectionCallback, this));
    mesh.onNodeTimeAdjusted(std::bind(&MeshNetwork::nodeTimeAdjustedCallback, this, std::placeholders::_1));

    Serial.println("Mesh initialized");
    Serial.print("Node ID: ");
    Serial.println(mesh.getNodeId());
}

void MeshNetwork::update() {
    mesh.update();
}

void MeshNetwork::sendMessage(const String& msg) {
    mesh.sendBroadcast(msg);
}

uint32_t MeshNetwork::getNodeId() {
    return mesh.getNodeId();
}

size_t MeshNetwork::getConnectedNodes() {
    return connectedNodes.size();
}

void MeshNetwork::receivedCallback(uint32_t from, String &msg) {
    Serial.printf("Received from %u msg=%s\n", from, msg.c_str());
    
    if (msg.startsWith("IDENTITY:")) {
        processIdentityMessage(from, msg);
    } else if (msg.startsWith("LEDGER:")) {
        processLedgerMessage(from, msg);
    }
}

void MeshNetwork::newConnectionCallback(uint32_t nodeId) {
    Serial.printf("New Connection, nodeId = %u\n", nodeId);
    connectedNodes.push_back(nodeId);
}

void MeshNetwork::changedConnectionCallback() {
    Serial.printf("Changed connections\n");
    auto nodes = mesh.getNodeList();
    connectedNodes.clear();
    for (auto it = nodes.begin(); it != nodes.end(); ++it) {
        connectedNodes.push_back(*it);
    }
}

void MeshNetwork::nodeTimeAdjustedCallback(int32_t offset) {
    Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}

void MeshNetwork::processIdentityMessage(uint32_t from, const String& msg) {
    // Process identity message if needed
    Serial.printf("Received identity from node %u\n", from);
}

void MeshNetwork::processLedgerMessage(uint32_t from, const String& msg) {
    String entry = msg.substring(7); // Remove "LEDGER:" prefix
    ledger.addEntry(entry);
}