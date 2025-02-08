#include <Arduino.h>
#include "pers-state.h"
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "esp-now.h"

#define PERS_STATE_FILE "/state.json"

int otaMs = 0;
char lastIp[16];

void stateMessageHandler(JsonDocument &doc, uint8_t *mac) {
    int otaMsParam = doc["otaMs"];
    if (otaMsParam > 0) {
        Serial.print("OTA requested for ");
        Serial.print(otaMsParam);
        Serial.println("ms. Writing to state...");
        writeState((char *) "OTA requested", otaMsParam);
        Serial.println("Restarting...");
        ESP.restart();
    }
}

void setupPersStateAndReadState()
{
    lastIp[0] = '\0';
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
    readState();
}

void readState()
{
    File file = LittleFS.open(PERS_STATE_FILE, "r");
    if (!file)
    {
        Serial.println("Failed to open state file for reading");
        return;
    }
    JsonDocument state;
    DeserializationError error = deserializeJson(state, file);
    if (error)
    {
        Serial.println("Failed to read state file");
        file.close();
        return;
    }
    otaMs = state["otaMs"];
    // read last ip
    const char *lastIpJson = state["lastIp"];
    strcpy(lastIp, lastIpJson);
    file.close();
}

int shouldOtaMs()
{
    return otaMs;
}

char *getLastIp()
{
    return lastIp;
}

void writeState(char *lastIp, int otaMsParam)
{
    JsonDocument state;
    state["lastIp"] = lastIp;
    state["otaMs"] = otaMsParam;
    File file = LittleFS.open(PERS_STATE_FILE, "w");
    if (!file)
    {
        Serial.println("Failed to open state file for writing");
        return;
    }
    serializeJson(state, file);
    Serial.println("State written to file");
    file.close();
}