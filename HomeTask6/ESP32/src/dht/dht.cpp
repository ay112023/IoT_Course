#include "dht.h"

// Порожньо. Реалізація датчика DHT22 — тут
DHT  dht(DHT_PIN, DHTT_TYPE);


void dht_begin()
{
    dht.begin();
}
bool dht_read(DHTTData* dhttpayload, DHT* dht)
{  
    if(dhttpayload == nullptr || dht == nullptr) 
        return false;

      dhttpayload->humidity    = dht->readHumidity();
      dhttpayload->temperature = dht->readTemperature();        

    return true;  
}
void dht_print(DHTTData* dhttpayload)
{
    if(dhttpayload == nullptr) {
        Serial.println("DHTT data pointer is null.");
        return;
    }

    Serial.print("[DHT] "); Serial.print(dhttpayload->temperature, 1);
    Serial.print("C  ");    Serial.print(dhttpayload->humidity, 1); Serial.println("%");
}