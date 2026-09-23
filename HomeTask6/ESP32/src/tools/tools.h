#pragma once
#include <Arduino.h>
#include "dht/dht.h"
#include "ldr/ldr.h"

// ═══════════════════════════════════════════════════════════
// СТАТУС ПРИСТРОЮ
// ═══════════════════════════════════════════════════════════
#define STATUS_OK       0b00000000

// ═══════════════════════════════════════════════════════════
// ПОРОГОВІ ЗНАЧЕННЯ
// ═══════════════════════════════════════════════════════════
#define HIGH_TEMPERATURE_THRESHOLD    80
#define LOW_TEMPERATURE_THRESHOLD    -40
#define HIGH_HUMIDITY_THRESHOLD      100
#define LOW_HUMIDITY_THRESHOLD         0
#define LOW_LDR_THRESHOLD            200

// ═══════════════════════════════════════════════════════════
// СТАТУС СЕНСОРІВ
// ═══════════════════════════════════════════════════════════
#define STATUS_LDR_ERR  0b00000001  // біт 0: LDR помилка
#define STATUS_DHT_ERR  0b00000010  // біт 1: DHT22 помилка

// Контроль неблокуючих таймерів
bool due(unsigned long& last, unsigned long interval);
// 
uint8_t validateSensors(DHTTData* dhttpayload, LDRData* ldrpayload);
void printSensorsStatus(uint8_t status);