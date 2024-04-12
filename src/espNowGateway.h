#ifndef EspNowGateway_h
#define EspNowGateway_h
#include "Arduino.h"
#include "FS.h"
#include "SPIFFS.h"

#include <ArduinoJson.h>
#include <WifiEspNow.h>
#include <freeRTOS_pp.h>

void espNowCallback(const uint8_t mac[WIFIESPNOW_ALEN], const uint8_t* buf, size_t count, void* arg);

class EspNowGateway {
public:
  // functions
  bool begin();
  void addPeer(uint8_t mac[6]);
  void refresh(Queue* queue);
  void subPeerToTopic(uint8_t mac[6], const char* topic);
  void removePeer(uint8_t mac[6], Queue* queue);
  void forwardMessage(const char* topic, const char* payload);
  bool enqueue(message_t msg, int msTimeout);
  bool dequeue(message_t* msgPtr);
  bool peek(message_t* msgPtr);
  // data
  const char* macAddress;
private:
  bool checkPeerForTopic(uint8_t mac[6], const char* topic);
  const char* macToFile(uint8_t mac[6]);
};
#endif