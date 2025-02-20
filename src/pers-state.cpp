#include <Arduino.h>
#include "pers-state.h"
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "esp-now.h"

#define PERS_STATE_FILE "/state.json"

PersState state;

void stateMessageHandler(JsonDocument &doc, uint8_t *mac) {
    int otaMsParam = doc["otaMs"];
    if (otaMsParam > 0) {
        Serial.print("OTA requested for ");
        Serial.print(otaMsParam);
        Serial.println("ms. Writing to state...");
        state.otaMs = otaMsParam;
        state.saveState();
        Serial.println("Restarting...");
        ESP.restart();
    }
}

void setupPersStateAndReadState()
{
    state.lastIp[0] = '\0';
    addMessageHandler(stateMessageHandler);
    if (!LittleFS.begin())
    {
        Serial.println("Failed to mount file system");
        return;
    }
    Serial.println("File system mounted");
    if (!LittleFS.exists(PERS_STATE_FILE))
    {
        Serial.println("State file does not exist");
        return;
    }
    Serial.println("State file exists");
    state.loadState();
}

void PersState::loadState()
{
    File file = LittleFS.open(PERS_STATE_FILE, "r");
    if (!file)
    {
        Serial.println("Failed to open state file for reading");
        return;
    }
    JsonDocument stateDocument;
    DeserializationError error = deserializeJson(stateDocument, file);
    if (error)
    {
        Serial.println("Failed to read state file");
        file.close();
        return;
    }
    state.otaMs = stateDocument["otaMs"];
    // read last ip
    const char *lastIpJson = stateDocument["lastIp"];

    // read clients
    int clientCount = 0;
    for (JsonObject clientJson : stateDocument["clients"].as<JsonArray>()) {
        int counter = clientJson["counter"];
        const char* name = clientJson["name"];
        const char* macStr = clientJson["mac"];
        if (clientCount < MAX_CLIENTS) {
            Client *client = &state.clients[clientCount];
            client->counter = counter;
            strcpy(client->name, name);
            sscanf(macStr, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &client->mac[0], &client->mac[1], &client->mac[2], &client->mac[3], &client->mac[4], &client->mac[5]);
            client->channel = clientJson["channel"];
            clientCount++;
        }
    }
    state.clientCount = clientCount;
    Serial.print("State loaded from file with ");
    Serial.print(clientCount);
    Serial.println(" clients");
    strcpy(state.lastIp, lastIpJson);
    file.close();
}

String mac2string(uint8_t *mac)
{
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(macStr);
}

void PersState::saveState()
{
    JsonDocument stateDocument;
    stateDocument["lastIp"] = lastIp;
    stateDocument["otaMs"] = otaMs;
    JsonArray clients = stateDocument["clients"].to<JsonArray>();
    for (int i = 0; i < clientCount; i++)
    {
        JsonObject client = clients.add<JsonObject>();
        client["name"] = this->clients[i].name;
        client["counter"] = this->clients[i].counter;
        client["mac"] = mac2string(this->clients[i].mac);
        client["channel"] = this->clients[i].channel;
    }

    File file = LittleFS.open(PERS_STATE_FILE, "w");
    if (!file)
    {
        Serial.println("Failed to open state file for writing");
        return;
    }
    serializeJson(stateDocument, file);
    Serial.println("State written to file");
    file.close();
}

// in constructor, reset clients memory to block of zeros
PersState::PersState()
{
    clientCount = 0;
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        memset(&clients[i], 0, sizeof(Client));
    }
}