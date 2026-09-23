#pragma once
#include <Arduino.h>

// Кнопка (btn1 у diagram.json), один контакт -> D5, другий -> GND
// Тому INPUT_PULLUP: натиснуто == LOW
#define BUTTON_PIN  5 

// ═══════════════════════════════════════════════════════════
// DEBOUNCE
// ═══════════════════════════════════════════════════════════
#define DEBOUNCE 50 // мс

// Сюди йде код по кнопці:
// button_begin()   — pinMode(BUTTON_PIN, INPUT_PULLUP)
// button_pressed() — антидребезг по millis(), фронт натискання
void button_begin();  
void button_pressed();