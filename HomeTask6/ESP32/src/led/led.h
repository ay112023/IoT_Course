#pragma once
#include <Arduino.h>

#define LED 2   // зовнішній

void led_begin();

// Очікуваний payload: {"action":"set","value":"on"}
void led_handle_command(const char* cmd);
bool led_status();
