#include <Arduino.h>
#include "sensors.h"
#include "config.h"
#include "wifi1.h"
#include "httpr.h"

unsigned long lastSensorReadLDR    = 0;
unsigned long lastSensorReadDHTT   = 0;
unsigned long lastSensorsMonitor = 0;
unsigned long lastSensorsPost = 0;


// ── Глобальні екземпляри ──────────────────────────────────
DHT        dht(DHTT_PIN, DHTT_TYPE);
SensorData sensorpayload;
LDRData    ldrpayload;
DHTTData   dhttpayload;


// ── Глобальні змінні ──────────────────────────────────
unsigned long lastDebounce    = 0;
bool          lastButtonState = HIGH;
bool          buttonState     = HIGH;
bool          ledState        = LOW;
volatile bool buttonPressed   = false;
uint8_t       currentMode     = 0;


// ═══════════════════════════════════════════════════════════
// ВАЛІДАЦІЯ СЕНСОРІВ
// ═══════════════════════════════════════════════════════════
uint8_t validateSensors() {
    uint8_t status = STATUS_OK;

    if (ldrpayload.raw < 0 || ldrpayload.raw > 4095) {
        status |= STATUS_LDR_ERR;
    }

    if (isnan(dhttpayload.temperature) ||
        isnan(dhttpayload.humidity)    ||
        dhttpayload.temperature < LOW_TEMPERATURE_THRESHOLD ||
        dhttpayload.temperature > HIGH_TEMPERATURE_THRESHOLD ||
        dhttpayload.humidity    < LOW_HUMIDITY_THRESHOLD    ||
        dhttpayload.humidity    > HIGH_HUMIDITY_THRESHOLD ) {
        status |= STATUS_DHT_ERR;
    }
       
    return status;
}

uint8_t validateWiFi()
{
   uint8_t status = STATUS_OK;
   if (!isWifiConnected()) 
     status |= STATUS_WIFI_ERR;
  return status;
}

// Первірка рівня освітленності
void checkLDR()
{    
     ledState = (ldrpayload.lux < LOW_LDR_THRESHOLD)?HIGH:LOW;
     digitalWrite(EXT_LED_PIN, ledState);     
}

//  Читання сенсорів
bool readLDR(){
  unsigned long now = millis();
     if ((now - lastSensorReadLDR) > LDR_INTERVAL) { 
       lastSensorReadLDR = now;
       ldrpayload.raw    = analogRead(LDR_PIN);
       ldrpayload.lux    = adcToLux(ldrpayload.raw);
       return true;
     } 
  return false;
}

bool readDHT()
{
   unsigned long now = millis();
    if ((now - lastSensorReadDHTT) > DHTT_INTERVAL) { 
      dhttpayload.humidity    = dht.readHumidity();
      dhttpayload.temperature = dht.readTemperature();       
      return true;
    }
  return false;  
}




// Вивід у Serial відліку часу
void printTimeStamp(unsigned long timestamp) {
    Serial.print("Час роботи: "); Serial.print(timestamp); Serial.println(" мс");
}

// Вивід у Serial режиму работи
void printMode(uint8_t mode)
{
     switch(mode)
     {
       case SILENT_MODE:
       {
         Serial.println("---Silent mode activated---");
         break;
       } 
       case MONITORING_MODE:
        {
          Serial.println("---Monitoring mode activated---");
          break;
        }
     }    
}

// Переривання для кнопки
void IRAM_ATTR onButtonPress() {
    buttonPressed = true; 
}


// Перевірка стану кнопки та перемикання режимів із виводом потчоного режима у Serial
void checkMode(){
   if (buttonPressed) {
       buttonPressed = false;        
       if (millis() - lastDebounce > DEBOUNCE) {
       lastDebounce = millis();       
       currentMode = (currentMode == SILENT_MODE)?MONITORING_MODE:SILENT_MODE; 
       printMode(currentMode);
     }
  }
}

// Обробка режимів роботи
void handleMode() {
     
    
     switch(currentMode){ 
        case SILENT_MODE:
            // У режимі SILENT_MODE нічого не виводимо
            break;

        case MONITORING_MODE:
          {
            unsigned long now = millis();
            if ((now - lastSensorsMonitor) > SENSORS_MON_INTERVAL) {        
                 lastSensorsMonitor = now;                                                     
                 printTimeStamp(now);                   
                 uint8_t status = validateSensors();    // Перевіряємо статус       
                 if(status == STATUS_OK) {                  
                     printLDR(&ldrpayload);
                     printDHTT(&dhttpayload);
                 }    
                 else 
                   {   
                    // Обробка помилок сенсорів             
                    if ((status & STATUS_DHT_ERR) && !(status & STATUS_LDR_ERR) )  
                       printLDR(&ldrpayload);

                    if (!(status & STATUS_DHT_ERR) && (status & STATUS_LDR_ERR) ) 
                       printDHTT(&dhttpayload);
                    
                    printSensorStatus(status);                              
                   }  
            }
            break;
          }
        default:
          {
            // Якщо режим невідомий, встановлюємо його в SILENT_MODE  
            currentMode = SILENT_MODE;          
            break;
          }  
    
    } 
             
}
// Перевірка таймерів, сенсорів, та відправка даних на сервер
void  postData() {

 unsigned long now = millis();
    if ((now - lastSensorsPost) > HTTP_POST_INTERVAL) {
        
        lastSensorsPost = now;                   
        uint8_t status = validateWiFi(); 
        status |= validateSensors();  

         if(status == STATUS_OK) {         
           sensorpayload.dhtt         = dhttpayload;
           sensorpayload.ldr          = ldrpayload;
           sensorpayload.timestamp    = millis();
           sensorpayload.statuscheck  = status;        
           Serial.println("[SENSOR] Дані зібрані — готові до відправки");
           printSensorData(&sensorpayload);  
           sendData(dhttpayload.temperature, dhttpayload.humidity, ldrpayload.lux);                      
          } 
          else 
          {
            if(status & STATUS_WIFI_ERR)  // Обробка помилки Wi-Fi
             {
               Serial.println("[WiFi] Помилка WiFi — дані не будуть відправлені"); 
               printWiFiStatus(status);
             }

            if((status & STATUS_LDR_ERR) || (status & STATUS_DHT_ERR) ) // Обробка помилок сенсорів
             {
                Serial.println("[SENSOR] Помилка валідації сенсорів — дані не будуть відправлені"); 
               printSensorStatus(status);          
             } 
         }                             
    }  
}
 
// ═══════════════════════════════════════════════════════════
// SETUP
// ═══════════════════════════════════════════════════════════
void setup() {         
    Serial.begin(115200);
    connectWifi();
    delay(500);
    pinMode(EXT_LED_PIN, OUTPUT);   
    pinMode(BUTTON_PIN,  INPUT_PULLUP);
    pinMode(LDR_PIN,     INPUT);
    dht.begin();
    currentMode = SILENT_MODE;
    attachInterrupt(BUTTON_PIN, onButtonPress, RISING);  
    Serial.println("ESP32 старт");
}

// ═══════════════════════════════════════════════════════════
// LOOP
// ═══════════════════════════════════════════════════════════
void loop() {
    // Все в одному loop() — це один FreeRTOS Task
    // xTaskCreate() для окремих задач — поза межами курсу
    if(readLDR()) checkLDR();
    readDHT();
    checkMode();
    handleMode();
    postData(); 
}
    