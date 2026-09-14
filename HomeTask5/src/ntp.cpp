#include <Arduino.h>
#include "ntp.h"

bool syncTime() {
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