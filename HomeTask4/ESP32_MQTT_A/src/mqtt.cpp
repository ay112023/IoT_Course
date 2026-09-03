#include "mqtt.h"
#include "config.h"

PubSubClient mqttClient(wifiClient);  // Використовуємо глобальний wifiClient з wifi1.h
// ═══════════════════════════════════════════════════════════
// MQTT
// ═══════════════════════════════════════════════════════════
bool connectMQTT() {
    Serial.print("[MQTT] Підключаємось до ");
    Serial.print(MQTT_BROKER);
    Serial.print("...");

    if (mqttClient.connect(MQTT_CLIENT_ID)) {
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
bool connectMQTTandSubscribe(uint8_t qos) {
    Serial.print("[MQTT] Підключаємось до ");
    Serial.print(MQTT_BROKER);
    Serial.print("...");

    if (mqttClient.connect(MQTT_CLIENT_ID)) {
        Serial.println(" OK");

        // Підписуємось всередині connectMQTT — не в setup()
        // Бо Clean Session = true скидає підписки при кожному відключенні
        // Так підписка автоматично відновлюється після reconnect
        mqttClient.subscribe(TOPIC_SENSORS, qos);
        Serial.print("[MQTT] Підписались на: ");
        Serial.println(TOPIC_SENSORS);

        mqttClient.subscribe(TOPIC_COMMANDS, qos);
        Serial.print("[MQTT] Підписались на: ");
        Serial.println(TOPIC_COMMANDS);
        return true;
    }
    // mqttClient.state() повертає код помилки:
    // -4 = таймаут, -2 = сервер не знайдено, 5 = відмовлено в доступі
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
        "{\"temperature\":%.1f,\"humidity\":%.1f,\"lux\":%1.f}",
        temperature, humidity, lux);
    
    Serial.print("[MQTT] Публікуємо: ");
    Serial.println(payload);

    // publish() повертає true якщо повідомлення прийнято брокером
    bool ok = mqttClient.publish(TOPIC_SENSORS, payload);
    Serial.println(ok ? "[MQTT] OK" : "[MQTT] Помилка публікації");
}

// Публікація команд
// size = MESSAGE_BUFFER_SIZE за замовчуванням, але можна передати менший розмір
// А раптом для команди достатньо меншого розміру буфера, ніж для сенсорних даних?
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
