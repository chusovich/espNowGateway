#ifndef Gateway_h
#define Gateway_h
#include "Arduino.h"
#include <freeRTOS_pp.h>
#include <WifiEspNow.h>
#include <ArduinoJson.h>

#define NUM_PEERS 10
#define NUM_TOPICS 12

void espNowCallback(const uint8_t mac[WIFIESPNOW_ALEN], const uint8_t *buf, size_t count, void *arg);

struct PeerData {
  bool active = false;
  uint8_t mac[6] = { 0, 0, 0, 0, 0, 0 };
  String topics[NUM_TOPICS];
  String alias = "PeerAlias";
};

class EspNowGateway {
public:
  // functions
  bool enqueue(message_t msg, int msTimeout);
  bool dequeue(message_t *msgPtr);
  bool peek(message_t *msgPtr);
  bool begin();
  void addPeer(uint8_t mac[6]);
  void refresh();
  void subPeerToTopic(const uint8_t mac[6], String topic);
  // removePeer();
  void forwardMessage(String topic, String payload);
  // data
  String macAddress;
};
#endif