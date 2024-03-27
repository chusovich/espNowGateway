#ifndef Client_h
#define Client_h
#include "Arduino.h"
#include <WifiEspNow.h>

#if defined(ARDUINO_ARCH_ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ARDUINO_ARCH_ESP32)
#include <WiFi.h>
#endif

#include <freeRTOS_pp.h>
#include <ArduinoJson.h>

void espNowClientCallback(const uint8_t mac[WIFIESPNOW_ALEN], const uint8_t *buf, size_t count, void *arg);

class EspNowClient {
public:
  EspNowClient(uint8_t gatewayMAC[6], const char *clientAlias);
  bool begin();
  bool subscribe(const char* topic);
  bool unsubscribe(const char* topic);
  bool publish(const char* topic, const char* payload);
  // queue methods
  bool enqueue(message_t msg, int msTimeout);
  bool dequeue(message_t *msgPtr);
  bool peek(message_t *msgPtr);
private:
  uint8_t _gtwMac[6];
  uint8_t _myMac[6];
  char _alias[20];
};
#endif
