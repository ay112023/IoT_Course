#include <WiFi.h>
#include "net.h"
#include "../secrets.h"

#define WIFI_TIMEOUT  10000   // максимум 10 секунд на підключення
#define NTP_TIMEOUT   15000

bool net_wifi_connected() {
    return WiFi.status() == WL_CONNECTED;
}

bool net_wifi_connect() {
    Serial.print("[Wi-Fi] Підключаємось");
    // Канал 6 — пропускає сканування, економить ~4 секунди в Wokwi
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD, 6);

    unsigned long start = millis();
    while (!net_wifi_connected()) {
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

bool net_time_sync() {
    Serial.print("[NTP] Синхронізація часу");
    configTime(0, 0, "pool.ntp.org");  // зсув 0, DST 0 — для TLS достатньо

    struct tm timeinfo;
    unsigned long start = millis();
    while (!getLocalTime(&timeinfo)) {
        if (millis() - start > NTP_TIMEOUT) {
            Serial.println(" таймаут!");
            return false;
        }
        delay(500);
        Serial.print(".");
    }

    Serial.println(" OK");
    return true;
}
