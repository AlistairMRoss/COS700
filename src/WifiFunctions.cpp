#include "WifiFunctions.h"

WifiFunctions* WifiFunctions::instance = nullptr;

WifiFunctions::WifiFunctions() 
    : connected(false), WiFiOffline(true), last_connected(millis()), 
      last_force_connected(0), last_query_time(0), queried(false),
      awsFunctions(staticAwsMessageCallback) {
    instance = this;
}

void WifiFunctions::setup() {
    delay(1000);
    Serial.println("WiFi Setup");

    WiFi.onEvent(std::bind(&WifiFunctions::WiFiGotIP, this, std::placeholders::_1, std::placeholders::_2), 
                 WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
    WiFi.onEvent(std::bind(&WifiFunctions::WiFiDisconnected, this, std::placeholders::_1, std::placeholders::_2), 
                 WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

    WiFiManager wm;
    wm.setConfigPortalTimeout(PORTAL_TIMEOUT);

    if (!wm.autoConnect(AP_NAME)) {
        Serial.println("Failed to connect");
        ESP.restart();
    } else {
        connected = true;
        WiFiOffline = false;
        Serial.println("Connection successful!");

        Serial.println("attempting MQTT");
        if (awsFunctions.connectAWS()) {
            Serial.println("AWS connection successful!");
        } else {
            Serial.println("AWS connection failed!");
        }
    }
}

void WifiFunctions::loop() {
    if (WiFi.status() != WL_CONNECTED) {
        if (last_connected + REBOOT_INTERVAL < millis()) {
            Serial.println("Rebooting");
            ESP.restart();
        }
        if (last_force_connected + FORCE_CONNECT_INTERVAL < millis()) {
            force_connect();
        }
    }
}

void WifiFunctions::queryEndpoint() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        prefs.begin(PREFS_NAMESPACE, false);
        String auth = prefs.getString("auth", "");
        prefs.end();
        bool response = true;
        if (response) {
            String fileEndpoint = "https://5n84t2twwg.execute-api.us-east-1.amazonaws.com/v1/firmware/getFirmware";
            http.begin(fileEndpoint);
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
                queried = true;
            } else {
                Serial.print("Error on file download. Error code: ");
                Serial.println(httpResponseCode);
            }
        } else {
            Serial.println("No update available");
        }
        
        http.end();
        last_query_time = millis();
    } else {
        Serial.println("WiFi not connected. Cannot query endpoint.");
    }
}

bool WifiFunctions::checkWifi() {
    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < WIFI_MAX_RECONNECT_TRIES) {
        delay(500);
        Serial.print(".");
        retries++;
    }
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected");
        return false;
    } else {
        return true;
    }
}

void WifiFunctions::tryConnect() {
    WiFi.disconnect();
    WiFi.reconnect();
    esp_sleep_enable_timer_wakeup(1 * 60L * 1000000L);
    esp_deep_sleep_start();
}

bool WifiFunctions::getConnected() {
    return connected;
}

void WifiFunctions::force_connect() {
    if (last_force_connected + FORCE_CONNECT_INTERVAL < millis()) {
        return;
    }
    last_force_connected = millis();
    Serial.println("Manually Connect");
    WiFi.setTxPower(WIFI_POWER_19_5dBm);
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true);
    prefs.begin("wifi", true);
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

void WifiFunctions::WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.print("WiFi Connected: ");
    connected = true;
    last_connected = millis();
    
    // Configure and get time from NTP
    configTime(GMT_OFFSET, DAYLIGHT_OFFSET, NTP_SERVER);
    
    // Wait for time to be set
    int retry = 0;
    const int retry_count = 10;
    while (time(nullptr) < 1000000000L && ++retry < retry_count) {
        Serial.print(".");
        delay(1000);
    }
    Serial.println();
    
    if (retry < retry_count) {
        Serial.println("NTP time set successfully");
        time_t now;
        struct tm timeinfo;
        if (getLocalTime(&timeinfo)) {
            time(&now);
            setenv("TZ", "GMT0BST,M3.5.0/1,M10.5.0", 1);
            tzset();
            
            // Set ESP32's RTC time
            struct timeval tv = { .tv_sec = now };
            settimeofday(&tv, NULL);
            
            Serial.print("Time set to: ");
            Serial.println(getCurrentTime());
        } else {
            Serial.println("Failed to obtain time");
        }
    } else {
        Serial.println("Failed to set time using NTP");
    }
    
    WiFiOffline = false;
    connected = true;
    char str[100];
    sprintf(str, "Connected to %s @ %i dBm", WiFi.SSID().c_str(), WiFi.RSSI());
    Serial.println(str);
}

String WifiFunctions::getCurrentTime() {
    if (!connected) {
        return "WiFi not connected";
    }

    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)){
        Serial.println("Failed to obtain time");
        return "Failed to obtain time";
    }

    char timeString[30];
    strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", &timeinfo);
    return String(timeString);
}

void WifiFunctions::WiFiDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.print("WiFi Disconnected: ");
    if (info.wifi_sta_disconnected.reason == 15) {
        Serial.println("Wrong Password");
    } else if (info.wifi_sta_disconnected.reason == 201) {
        Serial.println("No AP Found");
    }
    if (!WiFiOffline) {
        WiFiOffline = true;
        connected = false;
        Serial.print("WiFi Disconnected: ");
        Serial.println(info.wifi_sta_disconnected.reason);
        char str[100];
        sprintf(str, "WiFi Disconnected: %i ", info.wifi_sta_disconnected.reason);
        Serial.println(str);
    }
}

void WifiFunctions::staticAwsMessageCallback(char* topic, byte* payload, unsigned int length) {
    if (instance) {
        instance->awsMessageCallback(topic, payload, length);
    }
}

void WifiFunctions::awsMessageCallback(char* topic, byte* payload, unsigned int length) {
    char message[length + 1];
    memcpy(message, payload, length);
    message[length] = '\0';

    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    Serial.println(message);

    if (strcmp(message, "update") == 0) {
        Serial.println("Update command received. Querying endpoint...");
        queryEndpoint();
    }
}