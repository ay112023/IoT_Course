#include "tools.h"
unsigned long lastPublish;

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


bool read_sensors(LDRData* ldrData, DHTTData* dhttData){
       
        
        bool readDHTT_OK, readLDR_OK;
        readDHTT_OK = dht_read(dhttData, &dht);
        readLDR_OK = ldr_read(ldrData);
         
        // Обробка помилок сенсорів
        if(readDHTT_OK && readLDR_OK)
        {
            uint8_t status = validateSensors(dhttData, ldrData);
            if(status != STATUS_OK )
            {
                 Serial.println("Sensors status is invalid!"); 
                 printSensorsStatus(status);
                 return false;
            }         

        } else {
            Serial.println("Error reading sensors!");
            return false;
        }
    

  return true;
}