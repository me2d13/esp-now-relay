#pragma once

#include <Arduino.h>

bool connectWifi();
void setupOTA();
void getIp(char *ip);
void handleOta();
String getMacAddress();