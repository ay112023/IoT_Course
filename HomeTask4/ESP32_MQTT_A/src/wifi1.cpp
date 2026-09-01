#include "wifi1.h"


bool connectWifi() {
    Serial.print("[Wi-Fi] Підключаємось");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD, 6);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start > WIFI_TIMEOUT) {
            Serial.println(" таймаут!");
            return false;
        }
        delay(500);
        Serial.print(".");
    }

    Serial.println(" OK");
    Serial.print("[Wi-Fi] IP: ");
    Serial.println(WiFi.localIP());
    return true;
}

bool isWifiConnected() {
    return (WiFi.status() == WL_CONNECTED);
}

void printWiFiStatus(uint8_t status) {
    Serial.print("Status WiFi :0x");
    Serial.println(status, HEX);
    if (status & STATUS_WIFI_ERR)     
       Serial.print("  Wi-Fi: немає з'єднання");
    else
       Serial.print("  Wi-Fi: Connected");  
   Serial.println();
}

uint8_t validateWiFi()
{
   uint8_t status = STATUS_OK;
   if (!isWifiConnected()) 
     status |= STATUS_WIFI_ERR;
  return status;
}

