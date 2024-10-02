#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <HTTPClient.h>

#define PREFS_NAMESPACE "experiement"
#define WIFI_MAX_RECONNECT_TRIES 25
#define CONNECT_TIMEOUT 10000
#define AP_NAME "GEM"
#define NTP_SERVER "pool.ntp.org"
#define GMT_OFFSET 3600
#define DAYLIGHT_OFFSET 3600
#define LONG_PRESS_TIME 500
#define FORCE_CONNECT_INTERVAL 1000 * 60 * 5 
#define REBOOT_INTERVAL 1000 * 60 * 30
#define QUERY_INTERVAL 1000 * 60 * 60

class WiFiManager {
public:
    WiFiManager();
    void setup();
    void loop();
    bool isOffline() const;

private:
    void connectToNetwork(uint8_t idx, const char* password);
    void scanForNetworks();
    void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info);
    void WiFiDisconnected(WiFiEvent_t event, WiFiEventInfo_t info);
    void force_connect();
    void queryEndpoint();

    bool WiFiOffline;
    unsigned long last_connected;
    unsigned long last_force_connected;
    unsigned long last_query_time;
    Preferences prefs;
    bool queried;
};

#endif