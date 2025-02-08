#include <Arduino.h>
#include "pairing.h"
#include "esp-now.h"
#include "config.h"

unsigned long pairingEndMilis = 0;

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

void pairingMessageHandler(JsonDocument &doc, uint8_t *mac) {
    int pairMs = doc["pairMs"];
    // message to start pairing
    if (pairMs > 0) {
        Serial.print("Pairing requested for ");
        Serial.print(pairMs);
        Serial.println("ms");
        pairingEndMilis = millis() + pairMs;
        digitalWrite(LED_PIN, LOW);
    }
    // actual pairing message
    if (doc.containsKey("pairMagic") && isPairing()) {
        int pairMagic = doc["pairMagic"];
        int counter = doc["counter"];
        const char *from = doc["from"];
        Serial.print("Pairing request from ");
        Serial.print(from);
        Serial.print(", magic: ");
        Serial.print(pairMagic);
        Serial.print(", counter: ");
        Serial.println(counter);
        if (pairMagic == PAIRING_MAGIC) {
            // add to client list
        } else {
            Serial.print("Invalid magic. Received: ");
            Serial.println(pairMagic);
        }
        
    }

}

void setupPairing() {
    addMessageHandler(pairingMessageHandler);

}