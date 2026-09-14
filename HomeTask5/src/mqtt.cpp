#include "mqtt.h"
#include "config.h"
#include "secrets.h"

PubSubClient mqttClient(wifiClient);  // Використовуємо глобальний wifiClient з wifi1.h
// ═══════════════════════════════════════════════════════════
// MQTT
// ═══════════════════════════════════════════════════════════
bool connectMQTT() {
  
    Serial.print("[MQTT] Підключаємось до AWS IoT Core...");

    if (mqttClient.connect(THINGNAME)) {
        Serial.println(" OK");
        return true;
    }

    // mqttClient.state() повертає код помилки:
    // -4 = таймаут, -2 = сервер не знайдено, 5 = відмовлено в доступі
    Serial.print(" помилка: ");
    Serial.println(mqttClient.state());
    return false;
}
// 
// Коннект до MQTT з підпискою на топіки
//
bool connectMQTTandSubscribe() { 
    Serial.print("[MQTT] Підключаємось до AWS IoT Core...");
    
    if (mqttClient.connect(THINGNAME)) {
        Serial.println(" OK");

       
        mqttClient.subscribe(TOPIC_TELEMETRY);
        Serial.print("[MQTT] Підписались на: ");
        Serial.println(TOPIC_TELEMETRY);

       
        return true;
    }

  
    Serial.print(" помилка: ");
    Serial.println(mqttClient.state());
    return false;
}

// ═══════════════════════════════════════════════════════════
// ПУБЛІКАЦІЯ ДАНИХ
// ═══════════════════════════════════════════════════════════
void publishSensorsData(float temperature, float humidity, float lux) {
    if (!mqttClient.connected()) {
        Serial.println("[MQTT] Не підключено — пропускаємо");
        return;
    }   
    // snprintf замість String — безпечно для heap (Заняття 4)
    // char буфер фіксованого розміру, ніякої фрагментації
 
    char payload[MESSAGE_BUFFER_SIZE];
    snprintf(payload, sizeof(payload),
        "{\"temperature\":%.1f,\"humidity\":%.1f,\"lux\":%1.f,\"device_id\":\"%s\",\"timestamp\":%d}",
        temperature, humidity, lux, THINGNAME, millis());
    
    Serial.print("[MQTT] Публікуємо: ");
    Serial.println(payload);

    // publish() повертає true якщо повідомлення прийнято брокером
    bool ok = mqttClient.publish(TOPIC_TELEMETRY, payload);
    Serial.println(ok ? "[MQTT] OK" : "[MQTT] Помилка публікації");
    Serial.println(ESP.getFreeHeap()); // Про всяк випадок контролюємо розмір heap!
}

// Публікація команд
// Параметр size_t вводимо не використовуючи MESSAGE_BUFFER_SIZE для можливої економії трафіка
// бо для команд може бути виділений менший буфер
void publishCommand(const char* command, size_t size){

  if(size <= 0 || size > MESSAGE_BUFFER_SIZE) {
        Serial.println("[MQTT] Помилка: розмір буфера для команди некоректний");
        return;
    }   
  if (!mqttClient.connected()) {
        Serial.println("[MQTT] Не підключено — пропускаємо");
        return;
    }
    // snprintf замість String — безпечно для heap (Заняття 4)
    // char буфер фіксованого розміру, ніякої фрагментації
    char payload[size];
    snprintf(payload, sizeof(payload),
         command);
    Serial.print("[MQTT] Публікуємо: ");
    Serial.println(payload);

    // publish() повертає true якщо повідомлення прийнято брокером
    bool ok = mqttClient.publish(TOPIC_COMMANDS, payload);
    Serial.println(ok ? "[MQTT] OK" : "[MQTT] Помилка публікації");    
}
