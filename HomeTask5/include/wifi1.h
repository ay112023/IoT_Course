#include <WiFi.h>
#include <WiFiClientSecure.h>


#define STATUS_WIFI_ERR 0b00000100  // біт 2: Wi-Fi помилка

#pragma once
extern WiFiClientSecure  wifiClient;

bool isWifiConnected();
void printWiFiStatus(uint8_t status);
bool connectWifi();
uint8_t validateWiFi();