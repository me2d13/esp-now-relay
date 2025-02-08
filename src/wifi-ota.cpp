#include <Arduino.h>
#include <ArduinoOTA.h>
#include "config.h"
#include "wifi-ota.h"

#define LOGGER Serial

WiFiClient espClient;

bool connectWifi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.waitForConnectResult() != WL_CONNECTED) {
        LOGGER.println("Connection Failed! Rebooting...");
        return false;
    }
    LOGGER.println("Ready");
    char message[100];
    sprintf(message, "IP address: %d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
    LOGGER.println(message);
    return true;
}

void setupOTA() {
    ArduinoOTA.setHostname("ESP32-ESPNOW-GW-OTA");
    //ArduinoOTA.setPassword("admin");
    ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) {
            type = "sketch";
        } else { // U_SPIFFS
            type = "filesystem";
        }
        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
        LOGGER.print("Start updating ");
        LOGGER.println(type.c_str());
    });
    ArduinoOTA.onEnd([]() {
        LOGGER.println("End");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        LOGGER.print("Ota error: ");
        if (error == OTA_AUTH_ERROR) {
            LOGGER.println("Auth Failed");
        } else if (error == OTA_BEGIN_ERROR) {
            LOGGER.println("Begin Failed");
        } else if (error == OTA_CONNECT_ERROR) {
            LOGGER.println("Connect Failed");
        } else if (error == OTA_RECEIVE_ERROR) {
            LOGGER.println("Receive Failed");
        } else if (error == OTA_END_ERROR) {
            LOGGER.println("End Failed");
        }
    });
    ArduinoOTA.begin();
}

void getIp(char *ip) {
    sprintf(ip, "%d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
}

void handleOta() {
    ArduinoOTA.handle();
}

String getMacAddress() {
    return WiFi.macAddress();
}