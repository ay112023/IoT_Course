#include <Arduino.h>
#include "sensors.h"
#include "config.h"
#include "wifi1.h"
#include "httpr.h"

unsigned long lastSensorReadLDR    = 0;
unsigned long lastSensorReadDHTT   = 0;
unsigned long lastSensorsRead = 0;
unsigned long lastSensorsPost = 0;


// ── Глобальні екземпляри ──────────────────────────────────
DHT        dht(DHTT_PIN, DHTT_TYPE);
SensorData sensorpayload;
LDRData    ldrpayload;
DHTTData   dhttpayload;

// ═══════════════════════════════════════════════════════════
// DEBOUNCE
// ═══════════════════════════════════════════════════════════
#define DEBOUNCE 50  // мс


// ── Глобальні змінні ──────────────────────────────────
unsigned long lastDebounce   = 0;
bool          lastButtonState = HIGH;
bool          buttonState     = HIGH;
bool          ledState        = LOW;
volatile bool buttonPressed   = false;
uint8_t       currentMode;


// ═══════════════════════════════════════════════════════════
// ВАЛІДАЦІЯ СЕНСОРІВ
// ═══════════════════════════════════════════════════════════
uint8_t validateSensorsAndWiFi() {
    uint8_t status = STATUS_OK;

    if (ldrpayload.raw < 0 || ldrpayload.raw > 4095) {
        status |= STATUS_LDR_ERR;
    }

    if (isnan(dhttpayload.temperature) ||
        isnan(dhttpayload.humidity)    ||
        dhttpayload.temperature < LOW_TEMPERATURE_THRESHOLD ||
        dhttpayload.temperature > HIGH_TEMPERATURE_THRESHOLD ||
        dhttpayload.humidity    < LOW_HUMIDITY_THRESHOLD    ||
        dhttpayload.humidity    > HIGH_HUMIDITY_THRESHOLD) {
        status |= STATUS_DHT_ERR;
    }

    if (!isWifiConnected()) {
        status |= STATUS_WIFI_ERR;
    }

    return status;
}

//  Читання сенсорів

void readLDR(){
    ldrpayload.raw    = analogRead(LDR_PIN);
    ldrpayload.lux    = adcToLux(ldrpayload.raw);       
}

void readDHT()
{
    dhttpayload.humidity    = dht.readHumidity();
    dhttpayload.temperature = dht.readTemperature();       
}

void printTimeStamp(unsigned long timestamp) {
    Serial.print("Час роботи: "); Serial.print(timestamp); Serial.println(" мс");
}

// Переривання для кнопки
void IRAM_ATTR onButtonPress() {
    buttonPressed = true; 
}

// Перевірка стану кнопки та перемикання режимів
void checkMode(){
   if (buttonPressed) {
       buttonPressed = false;  
      
       if (millis() - lastDebounce > DEBOUNCE) {
       lastDebounce = millis();
       if(currentMode == SILENT_MODE)
           currentMode = MONITORING_MODE;
       else 
           currentMode = SILENT_MODE;    
     }
  }
}

// Обробка режимів роботи

void handleMode() {
     unsigned long now = millis();
    
     switch(currentMode){ 
        case SILENT_MODE:
            // У режимі SILENT_MODE нічого не виводимо
            break;

        case MONITORING_MODE:
          {
           
            if ((now - lastSensorsRead) > SENSORS_COLLECTION_INTERVAL) {        
                 lastSensorsRead = now;                                    
                 uint8_t status = validateSensorsAndWiFi();
                 printTimeStamp(now);                
                 if(status == STATUS_OK) {
                     readLDR();
                     readDHT();
                     printLDR(&ldrpayload);
                     printDHTT(&dhttpayload);
                 }    
                 else
                 {
                     printSensorStatus(status);
                     printWiFiStatus(status);
                 }  
            }
            break;
          }
        default:
            // Якщо режим невідомий, встановлюємо його в SILENT_MODE            
            break;
    
    } 
             
}


// Перевірка таймерів та відправка даних на сервер
void checkAndPost() {

 unsigned long now = millis();
    if ((now - lastSensorsPost) > SENSORS_COLLECTION_INTERVAL) {
        lastSensorsPost            = now;            
        uint8_t status = validateSensorsAndWiFi();
        
        if(status == STATUS_OK) {
          readLDR();
          readDHT();
          sensorpayload.dhtt         = dhttpayload;
          sensorpayload.ldr          = ldrpayload;
          sensorpayload.timestamp    = millis();
          sensorpayload.statuscheck  = status;        
          Serial.println("[SENSOR] Дані зібрані — готові до відправки");
          printSensorData(&sensorpayload);                        
        } else{
           Serial.println("[SENSOR] Помилка валідації сенсорів або WiFi — дані не будуть відправлені"); 
           printSensorStatus(status);
           printWiFiStatus(status);
           sendData(dhttpayload.temperature, dhttpayload.humidity, ldrpayload.lux);
        }
    }  
}
   
   




// ═══════════════════════════════════════════════════════════
// SETUP
// ═══════════════════════════════════════════════════════════
void setup() {
    Serial.begin(115200);
    delay(500);
    pinMode(EXT_LED_PIN, OUTPUT);
    pinMode(BUTTON_PIN,  INPUT_PULLUP);
    pinMode(LDR_PIN,     INPUT);
    dht.begin();
    attachInterrupt(BUTTON_PIN, onButtonPress, FALLING);
    Serial.println("ESP32 старт");
}

// ═══════════════════════════════════════════════════════════
// LOOP
// ═══════════════════════════════════════════════════════════
void loop() {
    // Все в одному loop() — це один FreeRTOS Task
    // xTaskCreate() для окремих задач — поза межами курсу
    checkMode();
    handleMode();
    checkAndPost();        
}
    