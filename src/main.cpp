#include <Arduino.h>
#include "config.h"
#include <ArduinoJson.h>
#include "esp-now.h"
#include "relay.h"
#include "crypto.h"
#include "pers-state.h"
#include "wifi-ota.h"
#include "pairing.h"

#define DEBUG 1

unsigned long otaEndMilis = 0;
unsigned long lastBlickMilis = 0;


void blickTimes(int times);
void sendRelayStateToEspNow(int index, int state);

void otaSetup(int ms) {
  Serial.print("OTA requested for ");
  Serial.print(ms);
  Serial.println(" ms");
  bool wifiConnected = connectWifi();
  if (wifiConnected) {
    setupOTA();
    char ip[16];
    getIp(ip);
    Serial.print("Connected, my IP: ");
    Serial.println(ip);
    strcpy(state.lastIp, ip);
    state.otaMs = 0;
    state.saveState();
    otaEndMilis = millis() + ms;
  } else {
    strcpy(state.lastIp, (char *) "Can't connect");
    state.otaMs = 0;
    state.saveState();
    delay(20000);
    ESP.restart();
  }
}

void regularSetup() {
  Serial.println("Starting in esp-now mode...");
  setupCrypto();
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  setupRelay();
  Serial.println("Starting ESP-NOW...");
  setupEspNow();
  // add message handler for led control as lambda function
  addMessageHandler([](JsonDocument &doc, uint8_t *mac) {
    int times = doc["blickTimes"];
    if (times > 0)
      blickTimes(times);
  });
  setupPairing();
  // send alive message to gateway with last IP
  JsonDocument doc;
  doc["from"] = MY_NAME;
  doc["ota_ip"] = state.lastIp;
  doc["mac_address"] = getMacAddress();
  uint8_t gatewayMac[] = GATEWAY_MAC;
  sendJsonDocumentToEspNow(doc, gatewayMac);
}

void setup()
{
  // initialize serial for debugging
  Serial.begin(115200);
  if (DEBUG) {
    delay(1000);
    Serial.println("Setup started, debug mode");
  }
  setupPersStateAndReadState();
  int ms = state.otaMs;
  if (ms > 0) {
    otaSetup(ms);
  } else {
    regularSetup();
  }
}

void loop()
{
  // get current millis
  unsigned long currentMillis = millis();
  // check if OTA is in progress
  if (otaEndMilis > 0)
  {
    handleOta();
    if (currentMillis > otaEndMilis)
    {
      Serial.println("OTA time passed, restarting...");
      ESP.restart();
    }
  } else {
    if (currentMillis - lastBlickMilis >= 60000)
    {
      lastBlickMilis = currentMillis;
      blickTimes(1);
    }
    loopRelay();
    isPairing();
  }
}

void blickTimes(int times)
{
  for (int i = 0; i < times; i++)
  {
    digitalWrite(LED_PIN, LOW);
    delay(100);
    digitalWrite(LED_PIN, HIGH);
    delay(100);
  }
}
