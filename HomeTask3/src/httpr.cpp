#include "httpr.h"
#include "config.h"
#include "wifi1.h"


void sendData(float temperature, float humidity, float lux) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[HTTP] Wi-Fi не підключено — пропускаємо");
        return;
    }

    HTTPClient http;
    http.begin(SERVER_URL);
    http.addHeader("Content-Type", "application/json");

    // TODO ДЗ: замінити хардкодені дані на реальні з сенсорів
    String payload = "{\"temperature\":" + String(temperature, 1) +
                     ",\"humidity\":"    + String(humidity, 1)    +
                     ",\"lux\":"         + String(lux, 1)         + "}";

    Serial.print("[HTTP] Відправляємо: ");
    Serial.println(payload);

    int httpCode = http.POST(payload);

    if (httpCode == 200) {
        Serial.println("[HTTP] OK — сервер отримав дані");
    } else {
        Serial.print("[HTTP] Помилка: ");
        Serial.println(httpCode);
    }
    Serial.println("---------------------");
    http.end();
}