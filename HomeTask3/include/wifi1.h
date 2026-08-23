#include <WiFi.h>
#include "config.h"

#define STATUS_WIFI_ERR 0b00000100  // біт 2: Wi-Fi помилка


bool isWifiConnected();
void printWiFiStatus(uint8_t status);
bool connectWifi();