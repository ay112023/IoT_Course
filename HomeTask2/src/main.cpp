#include <Arduino.h>

#define LED_BLINK_INTERVAL 200
#define EXT_LED 4

#define SENSOR_INTERVAL 20000  // 20 секунд
#define TEMP_LOW 15
#define TEMP_HIGH 30
#define HUMIDITY_LOW 30
#define HUMIDITY_HIGH 65

#define MEMORY_ANALYSIS_INTERVAL 60000  // 60 секунд

// Таймер для сенсора
unsigned long lastSensorRead = 0;
// Таймер для аналізу пам'яті
unsigned long lastMemoryAnalyzed = 0;
// Таймер для LED
unsigned long lastBlinkTime = 0;

// Структура для зберігання даних сенсора
struct SensorData {
  float temperature;       // 4 байти
  float humidity;
  uint8_t status;          // 1 байт — бітові флаги
  unsigned long timestamp; // 4 байти
};

// Біт 0 = Wi-Fi connected
uint8_t checkWifi() {
  return 0b00000001;
}

// Біт 1 = sensor ok
uint8_t checkSensor() {
  return 0b00000010;
}


// Приймає вказівник — не копію структури
void printSensorData(SensorData* data) {
  static int callCount = 0;
  callCount++;
  Serial.printf("Reading # %d: Temp: %.1f C, Humidity: %.1f%%, Status: 0x%02X, Timestamp: %lu ms", callCount, data->temperature, data->humidity, data->status, data->timestamp);  
  Serial.println();
}


void printMemoryAnalysis() { 
  unsigned long now = millis();  
  Serial.printf("--- Free heap: %lu bytes, Up time:  %.3f min.", ESP.getFreeHeap(), now / 60000.0);
  Serial.println();
}

SensorData readSensor() {
  SensorData data;

  data.temperature = random(TEMP_LOW, TEMP_HIGH);
  data.humidity = random(HUMIDITY_LOW, HUMIDITY_HIGH);
  data.timestamp = millis();

  // Бітові флаги
  data.status = 0b00000000 | checkWifi();
  data.status = data.status | checkSensor();

  return data;
}

void setup() {
  Serial.begin(115200);  
  pinMode(EXT_LED, OUTPUT);
  delay(500);
  Serial.println("Starting sensor reading and memory analysis...");    
}

void loop() {
  unsigned long now = millis();

   // Мигає світлодіодом кожні 200 мс
  if(now - lastBlinkTime >= LED_BLINK_INTERVAL) {
    lastBlinkTime = now;
    digitalWrite(EXT_LED, !digitalRead(EXT_LED)); 
  }
  
  // Читає й виводе дані сенсора кожні 20 секунд
  if (now - lastSensorRead >= SENSOR_INTERVAL) {
    lastSensorRead = now;
    SensorData reading;
    reading = readSensor();
    printSensorData(&reading);
  }
  
  // Виводе FreeHeap кожні 60 секунд
  if(now - lastMemoryAnalyzed >= MEMORY_ANALYSIS_INTERVAL) {
    lastMemoryAnalyzed = now;
    printMemoryAnalysis();
  }
    
  }

