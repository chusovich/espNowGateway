/*!
 * @file EspNowGateway.cpp
 *
 * @page gtw_page Gateway
 *
 * @section intro_sec Introduction
 * This is an esp-now gateway
 *
 * @section examples_sec Code Examples
 * Here are some examples
 * 
 * @code
 * void setup() {
 *	 Serial.begin(9600);
 *	 gtw.begin();
 * }
 * @endcode
 *
 */

#include "EspNowGateway.h"

Queue gtwQueue(25);
const char* peerTopicFile = "peerTopicMap.txt";
const char* peerAliasFile = "peerAliasMap.txt";

/**************************************************************************/
/*! 
    @brief  This is the esp-now call back function. It's only job is to put the message it receives into the queue
*/
/**************************************************************************/
void espNowCallback(const uint8_t mac[WIFIESPNOW_ALEN], const uint8_t* buf, size_t count, void* arg) {
  JsonDocument doc;
  message_t cbMsg;
  char buffer[250];
  for (int i = 0; i < static_cast<int>(count); ++i) {
    buffer[i] = static_cast<char>(buf[i]);
  }
  DeserializationError error = deserializeJson(doc, buffer, 250);
  if (!error) {
    serializeJson(doc, cbMsg.string);  // Serial.println("Json serialized!");
    gtwQueue.enqueue(cbMsg, 100);      // Serial.println("message enquqeued from cb!");
  }
}

/**************************************************************************/
/*! 
    @brief  Startup ESP-NOw and load our mac address
*/
/**************************************************************************/
bool EspNowGateway::begin() {
  if (!SPIFFS.begin(true)) {
    return false;
  }
  // Serial.println("SPIFFS initialization done.");
  gtwQueue.create();
  WiFi.persistent(false);
  WiFi.mode(WIFI_AP);
  WiFi.disconnect();
  WiFi.softAP("ESPNOW_GATEWAY", nullptr, 3);
  WiFi.softAPdisconnect(false);
  bool initBool = WifiEspNow.begin();
  if (!initBool) {
    return false;
  }
  WifiEspNow.onReceive(espNowCallback, nullptr);
  // macAddress = WiFi.softAPmacAddress();
  return true;
}

/**************************************************************************/
/*! 
    @brief  Add a peer to our peer list
    @param  the mac address of the peer we want to add to our list
*/
/**************************************************************************/
void EspNowGateway::addPeer(uint8_t mac[6]) {
  // open the file and close it, which will create it if it hasn't already been created
  File peerFile = SPIFFS.open(macToFile(mac), FILE_WRITE);
  peerFile.close();
}

/**************************************************************************/
/*! 
    @brief  Add the peer's mac and topic to our peer/topic list file
    @param  MAC address of the peer subscribing to the topic
    @param  Topic to which the peer is subscribing
*/
/**************************************************************************/
void EspNowGateway::subPeerToTopic(uint8_t mac[6], const char* topic) {
  // append our json object to the end of the peer's file
  if (!checkPeerForTopic(mac, topic)) {
    // append to file
    File jsonlFile = SPIFFS.open(macToFile(mac), FILE_WRITE);
    JsonDocument doc;
    doc["topic"] = topic;
    serializeJson(doc, jsonlFile);
    jsonlFile.println();
    jsonlFile.close();
  }
}

/**************************************************************************/
/*! 
    @brief  Serch through out peer/topic list and forward the topic's payload to any peer subscribed to that topic
    @param  The topic which we have a payload for
    @param  The payload of the topic we're trying to forward
*/
/**************************************************************************/
void EspNowGateway::forwardMessage(const char* topic, const char* payload) {
  // creat the message string we need to forward to a client
  char buffer[250];
  JsonDocument msgDoc;
  msgDoc["topic"] = topic;
  msgDoc["payload"] = payload;
  serializeJson(msgDoc, buffer);
  File root = SPIFFS.open("/");
  File peerFile = root.openNextFile();  // for each object in the list, check if it is our topic, and then send the payload to the correct peer
  while (true) {                        // loop for each peer file in the root directory
    if (!peerFile) {
      // no more files
      break;
    }
    uint8_t peerMac[6];
    String filename = peerFile.name();
    for (int i = 0; i < 6; i++) {
      const char* str = filename.substring(i * 3, i * 3 + 1).c_str();
      peerMac[i] = strtol(str, 0, 16);
    }
    if (checkPeerForTopic(peerMac, topic)) {
      WifiEspNow.addPeer(peerMac);
      WifiEspNow.send(peerMac, reinterpret_cast<const uint8_t*>(buffer), strlen(buffer));
      WifiEspNow.removePeer(peerMac);
    }
    root.openNextFile();
  }
}

void EspNowGateway::refresh(Queue* queue) {
  // resubcribe to every topic in our peer/topic list
  File root = SPIFFS.open("/");
  File peerFile = root.openNextFile();
  while (true) {  // loop for each peer file in the root directory
    if (!peerFile) {
      // no more files
      break;
    }
    while (true) {  // loop for each topic in the peer file
      JsonDocument fileDoc;
      DeserializationError err = deserializeJson(fileDoc, peerFile);
      if (err) break;
      // const char* topicToAdd = fileDoc["topic"];
      fileDoc["id"] = 5;  // subscribe message id
      char msgBuf[50];
      serializeJson(fileDoc, msgBuf);
      message_t msg;
      strcpy(msg.string, msgBuf);
      queue->enqueue(msg, 500);
    }
    peerFile = root.openNextFile();  // move to the next file for the next iteration
  }
}

void EspNowGateway::removePeer(uint8_t mac[6], Queue* queue) {
  File peerFile = SPIFFS.open(macToFile(mac), FILE_READ);
  if (peerFile) {
    while (true) {  // loop for each topic in the peer file
      JsonDocument fileDoc;
      DeserializationError err = deserializeJson(fileDoc, peerFile);
      if (err) break;
      // tell the other module to unsub from these all of the peer's topics
      fileDoc["id"] = 6;  // unsub state code
      char msgBuf[50];
      serializeJson(fileDoc, msgBuf);
      message_t msg;
      strcpy(msg.string, msgBuf);
      queue->enqueue(msg, 500);
    }
    // delete the peer file so we will no longer forward messages
    SPIFFS.remove(macToFile(mac));
  }
}

bool EspNowGateway::checkPeerForTopic(uint8_t mac[6], const char* topic) {
  File peerFile = SPIFFS.open(macToFile(mac), FILE_READ);
  if (peerFile) {
    while (true) {  // loop for each topic in the peer file
      JsonDocument fileDoc;
      DeserializationError err = deserializeJson(fileDoc, peerFile);
      if (err) break;
      // if we have a match, return true
      if (strcmp(fileDoc["topic"], topic) == 0) {
        peerFile.close();
        return true;
      }
    }
    peerFile.close();
  }
  return false;
}

const char* EspNowGateway::macToFile(uint8_t mac[6]) {
  char fileName[23];
  snprintf(fileName, sizeof(fileName), "/%02X_%02X_%02X_%02X_%02X_%02X.txt",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return fileName;
}

/**************************************************************************/
/*! 
    @brief  This is a wrapper function put something into the gateway's queue
*/
/**************************************************************************/
bool EspNowGateway::enqueue(message_t msg, int msTimeout) {
  return gtwQueue.enqueue(msg, msTimeout);
}

/**************************************************************************/
/*! 
    @brief  Get the latest element in the gateway's queue
    @param  Pointer to which to pass the message to
*/
/**************************************************************************/
bool EspNowGateway::dequeue(message_t* msgPtr) {
  return gtwQueue.dequeue(msgPtr);
}

/**************************************************************************/
/*! 
    @brief  Get the latest element from the gateway's queue without removing it from the queue
    @param Pointer to which to pass the message to
*/
/**************************************************************************/
bool EspNowGateway::peek(message_t* msgPtr) {
  return gtwQueue.peek(msgPtr);
}