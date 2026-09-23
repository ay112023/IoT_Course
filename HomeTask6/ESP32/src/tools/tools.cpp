#include "tools.h"

// Контроль неблокуючих таймерів
bool due(unsigned long& last, unsigned long interval)
{
     unsigned long now = millis();
     if ((now - last) > interval)
      {       
        last = now;
        return true;
      }  
  return false;    
}

//
uint8_t validateSensors(DHTTData* dhttpayload, LDRData* ldrpayload) {
    uint8_t status = STATUS_OK;

    if(dhttpayload == nullptr || ldrpayload == nullptr)
    {
         status |= STATUS_LDR_ERR;
         status |= STATUS_DHT_ERR;
         return status;
    }

    if (ldrpayload->raw < 0 || ldrpayload->raw > 4095) {
        status |= STATUS_LDR_ERR;
    }

    if (isnan(dhttpayload->temperature) ||
        isnan(dhttpayload->humidity)    ||
        dhttpayload->temperature < LOW_TEMPERATURE_THRESHOLD ||
        dhttpayload->temperature > HIGH_TEMPERATURE_THRESHOLD ||
        dhttpayload->humidity    < LOW_HUMIDITY_THRESHOLD    ||
        dhttpayload->humidity    > HIGH_HUMIDITY_THRESHOLD ) {
        status |= STATUS_DHT_ERR;
    }
       
    return status;
}


void printSensorsStatus(uint8_t status) {
    Serial.print("Статус сенсорів: 0x");
    if (status < 16) Serial.print("0");
    Serial.println(status, HEX);
    if (status & STATUS_LDR_ERR)  Serial.println("  LDR: дані поза діапазоном");
    if (status & STATUS_DHT_ERR)  Serial.println("  DHT22: помилка читання");    
}