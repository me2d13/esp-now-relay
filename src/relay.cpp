#include <Arduino.h>
#include "relay.h"
#include "esp-now.h"
#include "config.h"
#include "pers-state.h"

#define RELAY_NUMBER 2
#define NO_CLIENT 100

const uint8_t relayPins[RELAY_NUMBER] = {D5, D6};
unsigned long pushReleaseTime[RELAY_NUMBER] = {0, 0};

byte gatewayMac[] = GATEWAY_MAC;

void setRelayState(int index, int state, const JsonDocument* meta = NULL);

void relayMessageHandler(JsonDocument &doc, uint8_t *mac)
{
  int channel = doc["channel"] | 99;
  if (channel >= 0 && channel <= RELAY_NUMBER)
  {
    JsonDocument meta;
    uint8_t fromClientIndex = NO_CLIENT;
    // validate sender's mac
    if (memcmp(mac, gatewayMac, 6) != 0)
    {
      // check also paired devices
      for (int i = 0; i < state.clientCount; i++)
      {
        if (memcmp(mac, state.clients[i].mac, 6) == 0)
        {
          fromClientIndex = i;
          break;
        }
      }
      if (fromClientIndex == NO_CLIENT)
      {
        Serial.println("Relay message from unknown sender, ignoring");
        return;
      } else {
        // validate counter
        auto client = state.clients[fromClientIndex];
        meta["from"] = client.name;
        auto lastUsedCounter = client.counter;
        unsigned int incomingCounter = doc["counter"];
        // incoming counter must be greater than last used counter, but maximum 50 ahead
        if (incomingCounter <= lastUsedCounter || incomingCounter > lastUsedCounter + 50)
        {
          // make message also as String to be sent out
          String message = "Invalid counter value " + String(incomingCounter) + " from client, expected between " 
            + String(lastUsedCounter + 1) + " and " + String(lastUsedCounter + 50);
          Serial.println(message);
          meta["error"] = message;
          uint8_t gatewayMac[] = GATEWAY_MAC;
          sendJsonDocumentToEspNow(meta, gatewayMac);
          return;
        } else {
          // update client's counter
          state.clients[fromClientIndex].counter = incomingCounter;
          state.saveState();
          meta["battery"] = doc["battery"];
        }
      }
    } else {
      meta["from"] = "gateway";
    }
  
    int relayIndex = channel-1;
    int state = doc["state"] | 99;
    if (state == 0 || state == 1) {
      setRelayState(relayIndex, state, &meta);
    }
    int push = doc["push"];
    if (push > 1) {
      pushReleaseTime[relayIndex] = millis() + push;
      setRelayState(relayIndex, 1, &meta);
    }
  }
}

void setupRelay() {
  for (int i = 0; i < RELAY_NUMBER; i++)
  {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], HIGH);
  }
  addMessageHandler(relayMessageHandler);
}

void sendRelayStateToEspNow(int index, int state, const JsonDocument* meta = NULL)
{
  JsonDocument doc;
  doc["index"] = index;
  doc["state"] = state;
  if (meta != NULL)
  {
    JsonObject metaOutputObject = doc["meta"].to<JsonObject>();
    JsonObjectConst metaInputObject = meta->as<JsonObjectConst>();
    // add every element from meta to doc
    for (JsonPairConst kv : metaInputObject)
    {
      metaOutputObject[kv.key().c_str()] = kv.value();
    }
  }
  uint8_t gatewayMac[] = GATEWAY_MAC;
  sendJsonDocumentToEspNow(doc, gatewayMac);
}

void setRelayState(int index, int state, const JsonDocument* meta)
{
  if (index < 0 || index >= RELAY_NUMBER)
    return;
  if (state != 0 && state != 1)
    return;
  digitalWrite(relayPins[index], state ? LOW : HIGH);
  sendRelayStateToEspNow(index+1, state, meta);
}

void loopRelay() {
  // check if release time for some relay is set and reached
  for (int i = 0; i < RELAY_NUMBER; i++)
  {
    unsigned long currentMillis = millis();
    if (pushReleaseTime[i] > 0 && currentMillis >= pushReleaseTime[i])
    {
      setRelayState(i, 0, NULL);
      //digitalWrite(relayPins[i], HIGH);
      pushReleaseTime[i] = 0;
    }
  }
}

