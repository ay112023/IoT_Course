#pragma once
#include <Arduino.h>

// Фоторезистор (ldr1 у diagram.json), живлення 3V3
#define LDR_DO_PIN  14   // цифровий: поріг спрацював / ні
#define LDR_AO_PIN  34   // аналоговий: ADC1, тільки input



// ═══════════════════════════════════════════════════════════
// КОНСТАНТИ ДЛЯ КОНВЕРТАЦІЇ LDR → LUX
// ═══════════════════════════════════════════════════════════
#define GAMMA 0.7f
#define RL10  33.0f  // опір LDR при 10 lux (кОм)

struct LDRData {
    int   raw;  // сирий ADC 0–4095
    float lux;  // розраховані люкси
};

void ldr_begin();
void ldr_print(LDRData* ldrpayload);
bool ldr_read(LDRData* ldrpayload);
float adcToLux(int adcValue);


// Сюди йде код по освітленості:
// ldr_begin() — pinMode(LDR_DO_PIN, INPUT); analogRead на 34 налаштування не треба
// ldr_read()  — analogRead(LDR_AO_PIN), 0..4095
