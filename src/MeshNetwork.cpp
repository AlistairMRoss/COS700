#include "MeshNetwork.h"

MeshNetwork::MeshNetwork(const char* prefix, const char* password, uint16_t port, FirmwareUpdate* update)
    : meshPrefix(prefix), meshPassword(password), meshPort(port), firmwareUpdate(update) {}

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
    Serial.println(mesh.getNodeId());\

    if (!firmwareReader.begin()) {
        Serial.println("Failed to initialize FirmwareReader");
    }
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

    if (msg.startsWith("FIRMWARE_VERSION:")) {
        String version = msg.substring(17);
        sendMessage("FIRMWARE_REQUEST");
    } else if (msg.startsWith("FIRMWARE_CHUNK:")) {
        processFirmwareChunk(msg);
    } else if (msg == "FIRMWARE_END") {
        finalizeFirmwareUpdate();
    } else if (msg == "FIRMWARE_REQUEST") {
        startFirmwareDistribution();
    } else if (msg.startsWith("IDENTITY:")) {
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
    // ledger.addEntry(entry);
}

String MeshNetwork::calculateUniqueTimestampedFirmwareHash() {
    Serial.print("startng the hash");
    if (!firmwareReader.begin()) {
        Serial.println("Failed to initialize FirmwareReader");
        return "";
    }

    size_t firmwareSize = firmwareReader.getFirmwareSize();
    Serial.printf("Firmware size: %u bytes\n", firmwareSize);

    time_t now;
    time(&now);
    char timeStr[20];
    strftime(timeStr, sizeof(timeStr), "%Y%m%d%H%M%S", localtime(&now));

    mbedtls_md_context_t ctx;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);
    mbedtls_md_starts(&ctx);

    mbedtls_md_update(&ctx, (const unsigned char*)timeStr, strlen(timeStr));

    uint8_t* buffer = new uint8_t[CHUNK_SIZE];
    for (size_t offset = 0; offset < firmwareSize; offset += CHUNK_SIZE) {
        size_t chunkSize = min(CHUNK_SIZE, firmwareSize - offset);
        if (firmwareReader.readChunk(buffer, offset, chunkSize)) {
            mbedtls_md_update(&ctx, buffer, chunkSize);
        } else {
            Serial.printf("Failed to read chunk at offset %u\n", offset);
            delete[] buffer;
            mbedtls_md_free(&ctx);
            return "";
        }
    }
    delete[] buffer;


    uint8_t hash[32];
    mbedtls_md_finish(&ctx, hash);
    mbedtls_md_free(&ctx);

    char hashStr[65];
    for (int i = 0; i < 32; i++) {
        sprintf(&hashStr[i * 2], "%02x", hash[i]);
    }
    hashStr[64] = '\0';

    return String(timeStr) + "_" + String(hashStr);
}

void MeshNetwork::sendUniqueTimestampedFirmwareHash() {
    String hashWithTimestamp = calculateUniqueTimestampedFirmwareHash();
    if (!hashWithTimestamp.isEmpty()) {
        String message = "FIRMWARE_HASH:" + hashWithTimestamp;
        sendMessage(message);
    }
}


// =============================== Update stuff ======================================

void MeshNetwork::startFirmwareDistribution() {
    if (!firmwareUpdate || !firmwareUpdate->begin()) {
        Serial.println("Failed to start firmware distribution");
        return;
    }

    String versionMsg = "FIRMWARE_VERSION:" + firmwareUpdate->getVersion();
    sendMessage(versionMsg);

    sendNextFirmwareChunk();
}

void MeshNetwork::sendNextFirmwareChunk() {
    if (!firmwareUpdate) return;

    uint8_t buffer[CHUNK_SIZE];
    size_t bytesRead;

    if (firmwareUpdate->getNextChunk(buffer, bytesRead)) {
        String chunkMsg = "FIRMWARE_CHUNK:" + String(firmwareUpdate->getProgress()) + ":";
        chunkMsg += String((char*)buffer, bytesRead);
        sendMessage(chunkMsg);

        xTaskCreatePinnedToCore(
            [](void* param) {
                MeshNetwork* network = static_cast<MeshNetwork*>(param);
                network->sendNextFirmwareChunk();
                vTaskDelete(NULL);
            }, "sendNextChunk",3000, this, 1, NULL, 1);
    } else {
        String endMsg = "FIRMWARE_END";
        sendMessage(endMsg);
        firmwareUpdate->end();
    }
}

void MeshNetwork::processFirmwareChunk(const String& msg) {
    int firstColon = msg.indexOf(':');
    int secondColon = msg.indexOf(':', firstColon + 1);
    
    if (firstColon == -1 || secondColon == -1) {
        Serial.println("Invalid firmware chunk message");
        return;
    }

    float progress = msg.substring(firstColon + 1, secondColon).toFloat();
    String chunkData = msg.substring(secondColon + 1);

    static bool updateStarted = false;

    if (!updateStarted) {
        updateStarted = Update.begin(UPDATE_SIZE_UNKNOWN);
        if (!updateStarted) {
            Serial.println("Failed to start update");
            return;
        }
    }

    if (Update.write((uint8_t*)chunkData.c_str(), chunkData.length()) != chunkData.length()) {
        Serial.println("Failed to write firmware chunk");
        Update.abort();
        updateStarted = false;
        return;
    }

    Serial.printf("Firmware update progress: %.2f%%\n", progress * 100);
}

void MeshNetwork::finalizeFirmwareUpdate() {
    if (!Update.isFinished()) {
        Serial.println("Firmware update not finished");
        Update.abort();
        return;
    }

    if (!Update.end()) {
        Serial.println("Failed to end update");
        return;
    }

    if (Update.hasError()) {
        Serial.println("Update error: " + String(Update.getError()));
        return;
    }

    Serial.println("Firmware update successful. Rebooting...");
    ESP.restart();
}