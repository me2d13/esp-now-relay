#include <Arduino.h>
#include "pairing.h"
#include "esp-now.h"
#include "config.h"
#include "pers-state.h"
#include <ArduinoJson.h>
#include "crypto.h"

unsigned long pairingEndMilis = 0;
int pairingChannel = -1;

bool isPairing() {
    if (pairingEndMilis > 0) {
        bool inInterval = millis() < pairingEndMilis;
        if (!inInterval) {
            pairingEndMilis = 0;
            digitalWrite(LED_PIN, HIGH);
        }
        return inInterval;
    }
    return false;
}

void sendPairingResponse(Client *client, int id) {
    JsonDocument doc;
    doc["pairMagic"] = PAIRING_MAGIC;
    doc["counter"] = client->counter;
    doc["from"] = MY_NAME;
    doc["id"] = id;
    sendJsonDocumentToEspNow(doc, client->mac);
}

void fillClient(Client *client, JsonObject& parent, int index) {
    parent["name"] = client->name;
    parent["counter"] = client->counter;
    parent["index"] = index;
    parent["channel"] = client->channel + 1;
}

void sendNewPairingLog(Client *client, int index) {
    JsonDocument doc;
    doc["log"] = "New pairing";
    JsonObject clientObject = doc["client"].to<JsonObject>();
    fillClient(client, clientObject, index);
    uint8_t gatewayMac[] = GATEWAY_MAC;
    sendJsonDocumentToEspNow(doc, gatewayMac);
}

void processActualPairingMessage(JsonDocument &doc, uint8_t *mac) {
    int pairMagic = doc["pairMagic"];
    int counter = doc["counter"];
    int id = doc["id"];
    const char *from = doc["from"];
    Serial.print("Pairing request from ");
    Serial.print(from);
    Serial.print(", magic: ");
    Serial.print(pairMagic);
    Serial.print(", counter: ");
    Serial.print(counter);
    Serial.print(", id: ");
    Serial.print(id);
    Serial.print(", current channel: ");
    Serial.println(pairingChannel);
    if (pairMagic == PAIRING_MAGIC) {
        // add to client list
        if (state.clientCount < MAX_CLIENTS) {
            Client *client = &state.clients[state.clientCount];
            memcpy(client->mac, mac, 6);
            client->counter = counter;
            strcpy(client->name, from);
            client->channel = pairingChannel;
            state.clientCount++;
            state.saveState();
            sendPairingResponse(client, id);
            sendNewPairingLog(client, state.clientCount - 1);
        } else {
            Serial.println("Client list is full");
        }
    } else {
        Serial.print("Invalid magic. Received: ");
        Serial.println(pairMagic);
    }
}

void sendPairedClients() {
    Serial.print("Sending ");
    Serial.print(state.clientCount);
    Serial.println(" paired clients");
    // send list of paired clients to gateway - one message per each client
    for (int i = 0; i < state.clientCount; i++) {
        Client *client = &state.clients[i];
        JsonDocument doc;
        doc["log"] = "Paired client";
        JsonObject clientObject = doc["client"].to<JsonObject>();
        fillClient(client, clientObject, i);
        uint8_t gatewayMac[] = GATEWAY_MAC;
        sendJsonDocumentToEspNow(doc, gatewayMac);
        delay(100); // not nice but who cares, this will be used with hands on
    }
}

void unpairClient(JsonDocument &doc) {
    int index = doc["index"] | -1;
    if (index >= 0 && index < state.clientCount) {
        for (int i = index; i < state.clientCount - 1; i++) {
            memcpy(&state.clients[i], &state.clients[i + 1], sizeof(Client));
        }
        state.clientCount--;
        state.saveState();
        Serial.print("Client at index ");
        Serial.print(index);
        Serial.println(" unpaired");
    } else if (index == -1) {
        state.clientCount = 0;
        memset(state.clients, 0, sizeof(state.clients));
        state.saveState();
        Serial.println("All clients unpaired");
    } else {
        Serial.println("Invalid index to unpair");
    }
}

void pairingMessageHandler(JsonDocument &doc, uint8_t *mac) {
    int pairMs = doc["pairMs"] | 0;
    // message to start pairing
    if (pairMs > 0) {
        int channel = doc["channel"] | -1;
        if (channel == -1) {
            Serial.println("Pairing chanel not provided, request ignored");
            return;
        }
        // compare sender's mac with gateway's mac
        byte gatewayMac[] = GATEWAY_MAC;
        if (memcmp(mac, gatewayMac, 6) != 0) {
            Serial.println("Pairing request from unknown sender");
            return;
        }
        Serial.print("Pairing requested for ");
        Serial.print(pairMs);
        Serial.print("ms ");
        Serial.print("on channel ");
        Serial.println(channel);
        pairingEndMilis = millis() + pairMs;
        pairingChannel = channel - 1;
        digitalWrite(LED_PIN, LOW);
    }
    // pairing messages
    int pairMagic = doc["pairMagic"] | 0;
    if (pairMagic == PAIRING_MAGIC) {
        int id = doc["id"] | -1;
        if (id >= 0) {
            if (isPairing()) {
                processActualPairingMessage(doc, mac);
            } else {
                Serial.println("Pairing message received outside pairing interval");
            }
        } else {
            const char *action = doc["action"];
            if (strcmp(action, "list") == 0) {
                sendPairedClients();
            } else if (strcmp(action, "unpair") == 0) {
                unpairClient(doc);
            }
        }
    }
}

void setupPairing() {
    addMessageHandler(pairingMessageHandler);
}