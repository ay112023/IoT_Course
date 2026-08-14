#include <Arduino.h>

#define LED 2
#define EXT_LED 4


struct SensorData {
  float temperature;
};

SensorData readSensor() {
  SensorData data;
  data.temperature = random(15, 30);
  return data;
}

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  pinMode(EXT_LED, OUTPUT);
  delay(500);
}

void loop() {
  SensorData reading;
  reading = readSensor();
  Serial.printf("Temp: %.1f C\n", reading.temperature);
  Serial.println();
  digitalWrite(LED, HIGH);
  digitalWrite(EXT_LED, HIGH);
  delay(100);
  digitalWrite(LED, LOW);
  digitalWrite(EXT_LED, LOW);
  delay(100);
}