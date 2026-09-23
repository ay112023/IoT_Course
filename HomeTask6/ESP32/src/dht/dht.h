#pragma once
#include <Arduino.h>
#include <DHT.h>

// DHT22 (dht1 у diagram.json), SDA -> D4, живлення 3V3
#define DHT_PIN  4
#define DHTT_TYPE   DHT22

struct DHTTData {
    float temperature;
    float humidity;
};

extern DHT dht;

// Сюди йде код по датчику температури/вологості:
// dht_begin() — DHT dht(DHT_PIN, DHT22); dht.begin();
// dht_read()  — readTemperature()/readHumidity() + перевірка isnan()
// Результат віддається в mqtt_publish_telemetry()

void dht_begin();
bool dht_read(DHTTData* dhttpayload, DHT* dht);
void dht_print(DHTTData* dhttpayload);


