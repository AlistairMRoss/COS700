#ifndef AWS_FUNCTIONS_H
#define AWS_FUNCTIONS_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "WiFi.h"

#define TOPIC_BASE "cos700/"
#define AWS_MAX_RECONNECT_TRIES 5
#define AWS_CONNECT_TIMEOUT 10000

class AwsFunctions {
    public:
        AwsFunctions(void (*messageHandler)(char*, byte*, unsigned int));
        bool connectSubscribe();
        bool connectAWS();
        void publishMessage(const char *message);;
        bool getAwsConnected();
    private:
        WiFiClientSecure net = WiFiClientSecure();
        PubSubClient client;
        String endPoint = "";
        String ca_cert = "";
        String certificate = "";
        String privateKey = "";
        String deviceId = "";
        String token = "";
        bool AWSConnected = false;
        int retries = 0;
        unsigned long last_connect_attempt = 0;
};

#endif