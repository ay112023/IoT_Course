#pragma once
#include <Arduino.h>

#define LED_EVENT 0x01  // Хай event_id  == 1 це буде зміна стану LED-a


extern bool timeSynchronized;

// Wi-Fi → час → сертифікати → сервер. Порядок критичний.
bool mqtt_begin();

// connect + subscribe. Підписка живе в сесії брокера:
// реконект = нова сесія = нуль підписок, тому вона всередині.
bool mqtt_connect();

bool mqtt_connected();

// Викликати кожну ітерацію loop(): читає вхідні байти й тримає Keep Alive.
void mqtt_poll();

// Реконект з паузою RECONNECT_INTERVAL. Сам підніме Wi-Fi і закриє стару TLS-сесію.
void mqtt_reconnect_tick();

void mqtt_publish_telemetry(float temperature, float humidity, float lux);

// NULL, якщо команди нема. Інакше — payload, дійсний до наступного mqtt_poll().
const char* mqtt_take_command();


bool make_event_payload(char* payload, uint8_t length, uint8_t event_id, uint8_t value );

// Публікація event-а
void mqtt_publish_event(uint8_t event_id, uint8_t value);
