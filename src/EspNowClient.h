#ifndef EspNowClient_h
#define EspNowClient_h
#include "Arduino.h"

#include <ArduinoJson.h>
#include <WifiEspNow.h>
#include <freeRTOS_pp.h>

void espNowClientCallback(const uint8_t mac[6], const uint8_t *buf, size_t count, void *arg);

class EspNowClient {
public:
  EspNowClient(uint8_t gatewayMAC[6], const char *clientAlias);
  bool begin();
  bool subscribe(const char *topic);
  bool unsubscribe(const char *topic);
  bool publish(const char *topic, const char *payload);
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
