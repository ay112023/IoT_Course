#include "sensors.h"

// ═══════════════════════════════════════════════════════════
// КОНВЕРТАЦІЯ ADC → LUX
// ═══════════════════════════════════════════════════════════
float adcToLux(int adcValue) {
    float voltage    = adcValue / 4096.0f * 3.3f;
    float resistance = 2000.0f * voltage / (1.0f - voltage / 3.3f);
    float lux        = pow(RL10 * 1e3 * pow(10, GAMMA) / resistance, (1.0f / GAMMA));
    return lux;
}

// ═══════════════════════════════════════════════════════════
// ВИВІД В SERIAL
// ═══════════════════════════════════════════════════════════
void printSensorStatus(uint8_t status) {
    Serial.print("Статус сенсорів: 0x");
    if (status < 16) Serial.print("0");
    Serial.println(status, HEX);
    if (status & STATUS_LDR_ERR)  Serial.println("  LDR: дані поза діапазоном");
    if (status & STATUS_DHT_ERR)  Serial.println("  DHT22: помилка читання");    
}

void printSensorData(SensorData* sensorpayload) {

    if(sensorpayload == nullptr) {
        Serial.println("Sensor data pointer is null.");
        return;
    }
    
    Serial.println("------------------------------");
    Serial.print("Час роботи:   "); Serial.print(sensorpayload->timestamp);   Serial.println(" мс");
    Serial.print("Температура:  "); Serial.print(sensorpayload->dhtt.temperature, 1); Serial.println(" C");
    Serial.print("Вологість:    "); Serial.print(sensorpayload->dhtt.humidity, 1);    Serial.println(" %");
    Serial.print("Освітленість: "); Serial.print(sensorpayload->ldr.lux, 1);
    Serial.print(" lux (ADC: ");    Serial.print(sensorpayload->ldr.raw); Serial.println(")");   
    Serial.println("------------------------------");     
}
void printLDR(LDRData* ldrpayload)
{
    if(ldrpayload == nullptr) {
        Serial.println("LDR data pointer is null.");
        return;
    }

    Serial.print("[LDR] ADC: ");  Serial.print(ldrpayload->raw);
    Serial.print("  Lux: ");      Serial.println(ldrpayload->lux, 1);
}
void printDHTT(DHTTData* dhttpayload)
{
    if(dhttpayload == nullptr) {
        Serial.println("DHTT data pointer is null.");
        return;
    }

    Serial.print("[DHT] "); Serial.print(dhttpayload->temperature, 1);
    Serial.print("C  ");    Serial.print(dhttpayload->humidity, 1); Serial.println("%");
}
