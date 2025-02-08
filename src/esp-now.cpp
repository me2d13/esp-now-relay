#include "esp-now.h"
#include <ESP8266WiFi.h>
#include <espnow.h>
#include "config.h"
#include "crypto.h"

byte gatewayMac[] = GATEWAY_MAC;

#define BUFFER_SIZE 250

#define ENABLE_ENCRYPTION 1

byte messageBuffer[BUFFER_SIZE];

const byte key[32] = CRYPTO_KEY;

// array of up to 5 message handlers
void (*messageHandlers[5])(JsonDocument &doc, uint8_t *mac);
int messageHandlerCount = 0;

void onDataSent(uint8_t *mac, uint8_t sendStatus);
void onDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len);

void setupEspNow() {
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    // wait 1 minute and reboot
    delay(60000);
    ESP.restart();
    return;
  }
  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataRecv);
  esp_now_add_peer(gatewayMac, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
  Serial.println("Gw peer added");
}

// Callback when data is sent
void onDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.print("Last espnow send status: ");
  if (sendStatus == 0){
    Serial.println("Delivery success");
  }
  else{
    Serial.println("Delivery fail");
  }
}

void addMessageHandler(void (*handler)(JsonDocument &doc, uint8_t *mac)) {
    if (messageHandlerCount < 5) {
        messageHandlers[messageHandlerCount] = handler;
        messageHandlerCount++;
    }
}

void onDataRecv(uint8_t *mac, uint8_t *incomingData, uint8_t len) {
  memcpy(messageBuffer, incomingData, BUFFER_SIZE);
  Serial.print("Received ");
  logMessageToSerial(incomingData, len, ENABLE_ENCRYPTION);
  if (ENABLE_ENCRYPTION) {
    inPlaceDecryptAndLog(messageBuffer, len);
  }
 // Allocate the JSON document
  JsonDocument doc;
  // Parse the JSON document
  DeserializationError error = deserializeJson(doc, messageBuffer);
  // Test if parsing succeeds.
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }
  // call all message handlers
  for (int i = 0; i < messageHandlerCount; i++) {
    messageHandlers[i](doc, mac);
  }
}

void sendJsonDocumentToEspNow(JsonDocument &doc) {
  // Serialize the JSON document
  size_t len = serializeJson(doc, messageBuffer);
  char textBuffer[250];
  byte dataBytes[250];
  serializeJson(doc, textBuffer);
  Serial.print("Sending status message: ");
  Serial.println(textBuffer);
  int length = messageToByteArray(textBuffer, dataBytes, ENABLE_ENCRYPTION);
  if (length > 0) {
    Serial.print("Sending ");
    logMessageToSerial(dataBytes, length, ENABLE_ENCRYPTION);
    esp_now_send(gatewayMac, dataBytes, length);
  }
}