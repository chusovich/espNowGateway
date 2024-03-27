#include "Client.h"
// globals
Queue espNowQueue(25);

void espNowClientCallback(const uint8_t mac[WIFIESPNOW_ALEN], const uint8_t* buf, size_t count, void* arg) {
  JsonDocument doc;
  message_t cbMsg;
  char buffer[250];
  for (int i = 0; i < static_cast<int>(count); ++i) {
    buffer[i] = static_cast<char>(buf[i]);
  }
  DeserializationError error = deserializeJson(doc, buffer, 250);
  if (!error) {
    serializeJson(doc, cbMsg.string);
    espNowQueue.enqueue(cbMsg, 100);
    Serial.println("message recieved!");
  }
}

// class methods
EspNowClient::EspNowClient(uint8_t gatewayMAC[6], const char* clientAlias) {
  for (int i = 0; i < 6; i++) {
    _gtwMac[i] = gatewayMAC[i];
  }
  strcpy(_alias, clientAlias);
}

bool EspNowClient::enqueue(message_t msg, int msTimeout) {
  return espNowQueue.enqueue(msg, msTimeout);
}

bool EspNowClient::dequeue(message_t* msgPtr) {
  return espNowQueue.dequeue(msgPtr);
}

bool EspNowClient::peek(message_t* msgPtr) {
  return espNowQueue.peek(msgPtr);
}

bool EspNowClient::begin() {
  espNowQueue.create();
  WiFi.persistent(false);
  WiFi.mode(WIFI_AP);
  WiFi.disconnect();
  WiFi.softAP("ESPNOW", nullptr, 3);
  WiFi.softAPdisconnect(false);
  bool initBool = WifiEspNow.begin();
  if (!initBool) {
    return false;
  }
  initBool = WifiEspNow.addPeer(_gtwMac);
  if (!initBool) {
    return false;
  }
  WifiEspNow.onReceive(espNowClientCallback, nullptr);
  WiFi.softAPmacAddress(_myMac);
  char msgBuf[245];
  JsonDocument doc;
  for (int i = 0; i < 6; i++) {
    doc["mac"][i] = _myMac[i];
  }
  doc["alias"] = _alias;
  doc["id"] = 3;
  serializeJson(doc, msgBuf);
  Serial.printf("Sending msg: %s\n", msgBuf);
  Serial.printf("Gatway MAC:%02X:%02X:%02X:%02X:%02X:%02X\n", _gtwMac[0], _gtwMac[1], _gtwMac[2], _gtwMac[3], _gtwMac[4], _gtwMac[5]);
  if (WifiEspNow.send(_gtwMac, reinterpret_cast<const uint8_t*>(msgBuf), strlen(msgBuf))) {
    return true;
  }
  return false;
}

bool EspNowClient::subscribe(const char* topic) {
  JsonDocument doc;
  doc["id"] = 4;  // subscribe msg code
  for (int i = 0; i < 6; i++) {
    doc["mac"][i] = _myMac[i];
  }
  doc["topic"] = topic;
  char msgBuf[250];
  serializeJson(doc, msgBuf);
  Serial.printf("Sending msg: %s\n", msgBuf);
  if (WifiEspNow.send(_gtwMac, reinterpret_cast<const uint8_t*>(msgBuf), strlen(msgBuf))) {
    return true;
  }
  return false;
}

bool EspNowClient::unsubscribe(const char* topic) {
  JsonDocument doc;
  doc["id"] = 6;  // unsubscribe msg code
  for (int i = 0; i < 6; i++) {
    doc["mac"][i] = _myMac[i];
  }
  doc["topic"] = topic;
  char msgBuf[250];
  serializeJson(doc, msgBuf);
  Serial.printf("Sending msg: %s\n", msgBuf);
  if (WifiEspNow.send(_gtwMac, reinterpret_cast<const uint8_t*>(msgBuf), strlen(msgBuf))) {
    return true;
  }
  return false;
}

bool EspNowClient::publish(const char* topic, const char* payload) {
  JsonDocument doc;
  doc["id"] = 5;  // publish msg code
  doc["topic"] = topic;
  doc["payload"] = payload;
  char msgBuf[250];
  serializeJson(doc, msgBuf);
  Serial.printf("Sending msg: %s\n", msgBuf);
  if (WifiEspNow.send(_gtwMac, reinterpret_cast<const uint8_t*>(msgBuf), strlen(msgBuf))) {
    return true;
  }
  return false;
}
