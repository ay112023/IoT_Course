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

     
   char payload[128];
   snprintf(payload, sizeof(payload),"{\"temperature\":%.1f,\"humidity\":%.1f,\"lux\":%.1f}",
                                                                   temperature, humidity, lux);
    
    Serial.print("[HTTP] Відправляємо: ");                                                                       
    int code = http.POST((uint8_t *)payload, strlen(payload));                         
    Serial.println(payload);   

    if (code == 200) {
        Serial.println("[HTTP] OK — сервер отримав дані");
    } else {
        Serial.print("[HTTP] Помилка: ");
        Serial.println(code);
    }
    Serial.println("---------------------");
    http.end();
}