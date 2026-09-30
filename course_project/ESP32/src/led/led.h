#pragma once
#include <Arduino.h>

#define LED 2   // зовнішній

void led_begin();

// Очікуваний payload: {"action":"set","value":"on"}
void led_handle_command(const char* cmd);

// Пряме керування без JSON. Використовує тінь (Заняття 16).
void led_set(const char* state);   // "on" / "off"