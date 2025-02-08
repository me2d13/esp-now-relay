#pragma once

#include <ArduinoJson.h>

void setupEspNow();
void addMessageHandler(void (*handler)(JsonDocument &doc, uint8_t *mac));
void sendJsonDocumentToEspNow(JsonDocument &doc);