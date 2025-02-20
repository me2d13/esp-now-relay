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

void setRelayState(int index, int state);

void relayMessageHandler(JsonDocument &doc, uint8_t *mac)
{
  int channel = doc["channel"] | 99;
  if (channel >= 0 && channel <= RELAY_NUMBER)
  {
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
      }
    }
  
    int relayIndex = channel-1;
    int state = doc["state"] | 99;
    if (state == 0 || state == 1) {
      setRelayState(relayIndex, state);
    }
    int push = doc["push"];
    if (push > 1) {
      pushReleaseTime[relayIndex] = millis() + push;
      setRelayState(relayIndex, 1);
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

void sendRelayStateToEspNow(int index, int state)
{
  JsonDocument doc;
  doc["index"] = index;
  doc["state"] = state;
  uint8_t gatewayMac[] = GATEWAY_MAC;
  sendJsonDocumentToEspNow(doc, gatewayMac);
}

void setRelayState(int index, int state)
{
  if (index < 0 || index >= RELAY_NUMBER)
    return;
  if (state != 0 && state != 1)
    return;
  digitalWrite(relayPins[index], state ? LOW : HIGH);
  sendRelayStateToEspNow(index+1, state);
}

void loopRelay() {
  // check if release time for some relay is set and reached
  for (int i = 0; i < RELAY_NUMBER; i++)
  {
    unsigned long currentMillis = millis();
    if (pushReleaseTime[i] > 0 && currentMillis >= pushReleaseTime[i])
    {
      setRelayState(i, 0);
      //digitalWrite(relayPins[i], HIGH);
      pushReleaseTime[i] = 0;
    }
  }
}

