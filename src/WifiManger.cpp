#include "WiFiManager.h"
#include <SPIFFS.h>

WiFiManager::WiFiManager() : WiFiOffline(true), last_connected(millis()), last_force_connected(0), last_query_time(0), queried(false) {}

void WiFiManager::setup() {
    Serial.println("WiFi Setup");
    WiFi.onEvent(std::bind(&WiFiManager::WiFiGotIP, this, std::placeholders::_1, std::placeholders::_2), 
                 WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
    WiFi.onEvent(std::bind(&WiFiManager::WiFiDisconnected, this, std::placeholders::_1, std::placeholders::_2), 
                 WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    force_connect();
}

void WiFiManager::loop() {
    if(WiFi.status() != WL_CONNECTED) {
        if (last_connected + REBOOT_INTERVAL < millis()) {
            Serial.println("Rebooting");
            ESP.restart();
        }
        if (last_force_connected + FORCE_CONNECT_INTERVAL < millis()) {
            force_connect();
        }
    } else {
      if (!queried) {
        queryEndpoint();
        queried = true;
      }
        if (millis() - last_query_time >= QUERY_INTERVAL) {
            
        }
    }
}

void WiFiManager::queryEndpoint() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        prefs.begin(PREFS_NAMESPACE, false);
        String auth = prefs.getString("auth", "");
        prefs.end();

        if (auth.isEmpty()) {
            Serial.println("No authentication token found.");
            return;
        }
        // String checkEndpoint = "http://your-check-endpoint.com/check-update";
        
        // http.begin(checkEndpoint);
        // http.addHeader("Authorization", "Bearer " + auth);
        // int httpResponseCode = http.GET();
        
        // if (httpResponseCode == 200) {
            // String response = http.getString();
            // http.end();
            bool response = true;
            if (response) {
                String fileEndpoint = "https://5n84t2twwg.execute-api.us-east-1.amazonaws.com//v1/firmware/getFirmware";
                http.begin(fileEndpoint);
                // http.addHeader("Authorization", "Bearer " + auth);
                int httpResponseCode = http.GET();

                if (httpResponseCode == 200) {
                    if(!SPIFFS.begin(true)){
                        Serial.println("An Error has occurred while mounting SPIFFS");
                        return;
                    }

                    File file = SPIFFS.open("/update.bin", FILE_WRITE);
                    if(!file){
                        Serial.println("Failed to open file for writing");
                        return;
                    }

                    int len = http.getSize();
                    uint8_t buff[128] = { 0 };
                    WiFiClient * stream = http.getStreamPtr();
                    while(http.connected() && (len > 0 || len == -1)) {
                        size_t size = stream->available();
                        if(size) {
                            int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
                            file.write(buff, c);
                            if(len > 0) {
                                len -= c;
                            }
                        }
                        delay(1);
                    }
                    file.close();
                    Serial.println("File downloaded and saved");
                } else {
                    Serial.print("Error on file download. Error code: ");
                    Serial.println(httpResponseCode);
                }
            } else {
                Serial.println("No update available");
            }
        // } else {
        //     Serial.print("Error checking for update. Error code: ");
        //     Serial.println(httpResponseCode);
        // }
        
        http.end();
        last_query_time = millis();
    } else {
        Serial.println("WiFi not connected. Cannot query endpoint.");
    }
}

bool WiFiManager::isOffline() const {
    return WiFiOffline;
}

void WiFiManager::connectToNetwork(uint8_t idx, const char* password) {
    prefs.begin(PREFS_NAMESPACE, false);
    prefs.putString("ssid", WiFi.SSID(idx).c_str());
    prefs.putString("pw", password);
    prefs.end();
    WiFi.setAutoReconnect(true);
    WiFi.begin(WiFi.SSID(idx).c_str(), password);
}

void WiFiManager::scanForNetworks() {
    WiFi.disconnect();
    delay(100);
    Serial.print("Scanning for networks...");
    int n = WiFi.scanNetworks();
    Serial.println("done");
    if (n == 0) {
        Serial.println("no networks found");
    } else {
        Serial.print(n);
        Serial.println(" networks found");
        for (uint8_t i = 0; i < n; ++i) {
            Serial.print(i + 1);
            Serial.print(": ");
            Serial.print(WiFi.SSID(i));
            Serial.print(" (");
            Serial.print(WiFi.RSSI(i));
            Serial.print(")");
            Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");
            delay(10);
        }
    }
    Serial.println("");
}

void WiFiManager::WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.print("Wifi Connected: ");
    last_connected = millis();
    configTime(GMT_OFFSET, DAYLIGHT_OFFSET, NTP_SERVER);
    delay(1000);
    WiFiOffline = false;
    char str[100];
    sprintf(str, "Connected to %s @ %i dBm", WiFi.SSID().c_str(), WiFi.RSSI());
    Serial.println(str);
}

void WiFiManager::WiFiDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.print("Wifi Disconnected: ");
    if (info.wifi_sta_disconnected.reason == 15) {
        Serial.println("Wrong Password");
    } else if (info.wifi_sta_disconnected.reason == 201) {
        Serial.println("No AP Found");
    }
    if (!WiFiOffline) {
        WiFiOffline = true;
        Serial.print("Wifi Disconnected: ");
        Serial.println(info.wifi_sta_disconnected.reason);
        char str[100];
        sprintf(str, "Wifi Disconnected: %i ", info.wifi_sta_disconnected.reason);
        Serial.println(str);
    }
}

void WiFiManager::force_connect() {
    if (last_force_connected + FORCE_CONNECT_INTERVAL < millis()) {
        return;
    }
    last_force_connected = millis();
    Serial.println("Manually Connect");
    WiFi.setTxPower(WIFI_POWER_19_5dBm);
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true);
    prefs.begin(PREFS_NAMESPACE, true);
    String SSID = prefs.getString("ssid", "");
    if (SSID != "") {
        String PW = prefs.getString("pw", "");
        Serial.print("Connecting to ");
        Serial.println(SSID);
        WiFi.begin(SSID.c_str(), PW.c_str());
    } else {
        Serial.print("No networks saved");
    }
    prefs.end();
}