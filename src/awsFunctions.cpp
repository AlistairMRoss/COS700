#include "awsFunctions.h"

AwsFunctions::AwsFunctions(void (*messageHandler)(char*, byte*, unsigned int)) {
    client.setClient(net);
    client.setCallback(messageHandler);
}

void AwsFunctions::publishMessage(const char *message) {
  StaticJsonDocument<512> doc;
  doc["reply"] = message;
  String pubtopic = TOPIC_BASE + deviceId + "/info";
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);
  Serial.print("publish to ");
  Serial.print(pubtopic);
  Serial.print(" message: ");
  Serial.println(jsonBuffer);
  client.publish(pubtopic.c_str(), jsonBuffer);
}

bool AwsFunctions::connectSubscribe()
{
  if (client.connected()) {
    Serial.println("Already Connected");
    return true;
  } else if (millis() - last_connect_attempt < AWS_CONNECT_TIMEOUT) {
    delay(100);
    return false;
  } else if (retries > 5) {
    retries = 0;
  }
  Serial.println("Connecting to AWS IOT");
  client.connect(deviceId.c_str());
  last_connect_attempt = millis();
  Serial.println("Connecting");
  if (client.connected())
  {
    Serial.println("AWS IoT Connected!");
    String subtopic = TOPIC_BASE + deviceId + "/commands";
    client.subscribe(subtopic.c_str());
    Serial.print("Subscribing:");
    Serial.println(subtopic);
    AWSConnected = true;
    publishMessage("Connected to AWS");
    return true;
  }
  else
  {
    Serial.println("Not Connected to AWS yet");
    retries++;
    return false;
  }
}

bool AwsFunctions::connectAWS() {
  Serial.print("Device Id: ");
  Serial.println(deviceId);
  net.setCACert(ca_cert.c_str());
  net.setCertificate(certificate.c_str());
  net.setPrivateKey(privateKey.c_str());

  client.setServer(endPoint.c_str(), 8883);

  if (WiFi.status() != WL_CONNECTED)
  {
    return false;
  }
  connectSubscribe();
  return false;
}

bool AwsFunctions::getAwsConnected() {
  return AWSConnected;
}
