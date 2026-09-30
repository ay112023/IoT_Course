#ifndef MQTT_H
#define MQTT_H

#include <Arduino.h>


#define LED_EVENT 0x01 
#define BUTTON_EVENT 0x02
extern bool timeSynchronized;

// ── Життєвий цикл ──────────────────────────────────────────
bool mqtt_begin();          // Wi-Fi + час + TLS + налаштування клієнта
bool mqtt_connect();        // connect() + підписки + запит наступної роботи
bool mqtt_connected();
void mqtt_poll();           // mqttClient.loop() — кожну ітерацію
void mqtt_reconnect_tick(); // неблокуючий реконект по інтервалу

// ── Команди (Заняття 14) ───────────────────────────────────
const char* mqtt_take_command();   // NULL, якщо команди немає

// ── Роботи / OTA (Заняття 15) ──────────────────────────────
// Віддаємо СИРИЙ документ роботи. Парсить його ota.cpp.
const char* mqtt_take_job();       // NULL, якщо роботи немає

// Публікація в $next/get — «дай мою наступну роботу».
// Викликається при кожному підключенні (у mqtt_connect).
void mqtt_request_next_job();

// Звіт про статус конкретної роботи.
// status: "IN_PROGRESS" | "SUCCEEDED" | "FAILED"
void mqtt_report_status(const char* jobId, const char* status);

// Розірвати MQTT-з'єднання перед завантаженням прошивки.
// Звільняє TLS-контекст (десятки КБ купи) під другий, HTTPS до S3.
// ota.cpp не має лізти в приватний net клієнта — робимо це тут.
void mqtt_disconnect_for_ota();

// ── Телеметрія ─────────────────────────────────────────────
void mqtt_publish_telemetry(float temperature, float humidity, float lux);

// ── Активне очікування (Заняття 16) ────────────────────────
// Крутить mqtt_poll(), поки не приїде документ роботи або не вийде час.
// НЕ delay() — той не читає сокет, і відповідь просто не буде оброблена.
// Потрібне тому, що з Deep Sleep loop() більше немає: чекати мусить setup().
bool mqtt_wait_for_job(unsigned long timeoutMs);

// ── Службове ───────────────────────────────────────────────
// Публікація в довільний топік. Потрібна shadow.cpp, бо клієнт
// живе static усередині mqtt.cpp і назовні не видний.
bool mqtt_publish_raw(const char* topic, const char* payload);

#endif