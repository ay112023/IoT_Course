#include "DHT.h"

// ═══════════════════════════════════════════════════════════
// СТАТУС СЕНСОРІВ
// ═══════════════════════════════════════════════════════════
#define STATUS_LDR_ERR  0b00000001  // біт 0: LDR помилка
#define STATUS_DHT_ERR  0b00000010  // біт 1: DHT22 помилка

// ═══════════════════════════════════════════════════════════
// КОНСТАНТИ ДЛЯ КОНВЕРТАЦІЇ LDR → LUX
// ═══════════════════════════════════════════════════════════
#define GAMMA 0.7f
#define RL10  33.0f  // опір LDR при 10 lux (кОм)

// ═══════════════════════════════════════════════════════════
// СТРУКТУРИ ДАНИХ
// ═══════════════════════════════════════════════════════════
struct DHTTData {
    float temperature;
    float humidity;
};

struct LDRData {
    int   raw;  // сирий ADC 0–4095
    float lux;  // розраховані люкси
};

struct SensorData {
    DHTTData      dhtt;
    LDRData       ldr;
    unsigned long timestamp;
    uint8_t       statuscheck;
};

float adcToLux(int adcValue);
void printSensorStatus(uint8_t status);
void printSensorData(SensorData* sensorpayload);
void printLDR(LDRData* ldrpayload);
void printDHTT(DHTTData* dhttpayload);




















