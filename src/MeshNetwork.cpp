#include "MeshNetwork.h"


MeshNetwork::MeshNetwork(const char* prefix, const char* password, uint16_t port)
    : meshPrefix(prefix), meshPassword(password), meshPort(port), firmwareReader() {}

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
    if (msg.startsWith("IDENTITY:")) {
        processIdentityMessage(from, msg);
    } else if (msg.startsWith("LEDGER:")) {
        processLedgerMessage(from, msg);
    } else if (msg.startsWith("FIRMWARE_CHUNK:")) {
        processFirmwareChunk(from, msg);
    } else if (msg == "REQUEST_FIRMWARE") {
        sendFirmware("/firmware.bin"); // Assuming the firmware file is named firmware.bin
    }
}

void MeshNetwork::sendFirmware(const String& filename) {
    if (!SPIFFS.begin(true)) {
        Serial.println("An error has occurred while mounting SPIFFS");
        return;
    }

    File file = SPIFFS.open(filename, "r");
    if (!file) {
        Serial.println("Failed to open file for reading");
        return;
    }

    size_t fileSize = file.size();
    size_t chunks = (fileSize + CHUNK_SIZE - 1) / CHUNK_SIZE;

    for (size_t i = 0; i < chunks; i++) {
        sendFirmwareChunk(0, filename, i); // 0 means broadcast to all nodes
    }

    file.close();
}

void MeshNetwork::sendFirmwareChunk(uint32_t to, const String& filename, size_t chunkIndex) {
    File file = SPIFFS.open(filename, "r");
    if (!file) {
        Serial.println("Failed to open file for reading");
        return;
    }

    file.seek(chunkIndex * CHUNK_SIZE);
    String chunk = file.readString();
    file.close();

    String msg = "FIRMWARE_CHUNK:" + String(chunkIndex) + ":" + String(chunk.length()) + ":" + chunk;
    if (to == 0) {
        mesh.sendBroadcast(msg);
    } else {
        mesh.sendSingle(to, msg);
    }
}

void MeshNetwork::processFirmwareChunk(uint32_t from, const String& msg) {
    int firstColon = msg.indexOf(':', 15); // Start after "FIRMWARE_CHUNK:"
    int secondColon = msg.indexOf(':', firstColon + 1);

    size_t chunkIndex = msg.substring(15, firstColon).toInt();
    size_t chunkSize = msg.substring(firstColon + 1, secondColon).toInt();
    String chunkData = msg.substring(secondColon + 1);

    if (chunkIndex == 0) {
        incomingFirmwareFilename = "/incoming_firmware.bin";
        firmwareChunks.clear();
        expectedChunks = (chunkSize + CHUNK_SIZE - 1) / CHUNK_SIZE;
        receivedChunks = 0;
    }

    if (chunkIndex < expectedChunks) {
        firmwareChunks.push_back(chunkData);
        receivedChunks++;

        if (receivedChunks == expectedChunks) {
            assembleFirmware();
        }
    }
}

void MeshNetwork::assembleFirmware() {
    if (!SPIFFS.begin(true)) {
        Serial.println("An error has occurred while mounting SPIFFS");
        return;
    }

    File file = SPIFFS.open(incomingFirmwareFilename, "w");
    if (!file) {
        Serial.println("Failed to open file for writing");
        return;
    }

    for (const auto& chunk : firmwareChunks) {
        file.print(chunk);
    }

    file.close();
    Serial.println("Firmware file assembled and saved");
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