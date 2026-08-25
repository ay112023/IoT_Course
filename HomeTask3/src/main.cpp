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
uint8_t       currentMode     = SILENT_MODE;

// Первірка рівня освітленності
void checkLDR()
{    
     ledState = (ldrpayload.lux < LOW_LDR_THRESHOLD)?HIGH:LOW;
     digitalWrite(EXT_LED_PIN, ledState);     
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
// не використовую, бо, як показала практика, воно працює 
// тільки для швидквих коротких натискань кнопки
// а якщо натискання довге - брязкіт, коли відпускаємо кнопку, все одно не пропадає...
void IRAM_ATTR onButtonPress() {
    buttonPressed = true;
}

// Перевірка стану кнопки та перемикання режимів із виводом потчоного режима у Serial
void handleButtonAndSwitchMode()
{
  bool reading = digitalRead(BUTTON_PIN);

  if (reading != lastButtonState) {
    lastDebounce = millis();
  }

  if (millis() - lastDebounce > DEBOUNCE) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == LOW) {
       currentMode = (currentMode == SILENT_MODE)?MONITORING_MODE:SILENT_MODE; 
       printMode(currentMode);
      }
    }
  }

  lastButtonState = reading;
} 
void doMonitoring()  
{
    unsigned long now = millis();
   if ((now - lastSensorsMonitor) > SENSORS_MON_INTERVAL) {        
      lastSensorsMonitor = now;                                                     
      printTimeStamp(now);      
      readDHT(&dhttpayload,&dht);
      readLDR(&ldrpayload);
      checkLDR();             
      uint8_t status = validateSensors(&dhttpayload, &ldrpayload);    // Перевіряємо статус       
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
}
// Перевірка таймерів, сенсорів, та відправка даних на сервер
void  postData() {

 unsigned long now = millis();
    if ((now - lastSensorsPost) > HTTP_POST_INTERVAL) {
        
        lastSensorsPost = now;                   
        uint8_t status = validateWiFi(); 
        status |= validateSensors(&dhttpayload,&ldrpayload);  

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
// Обробка режимів роботи
void handleMode() {
         
     switch(currentMode){ 
        case SILENT_MODE:
            // У режимі SILENT_MODE нічого не виводимо
            break;

        case MONITORING_MODE:
          {
            doMonitoring();  
            postData();          
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
    Serial.println("ESP32 старт");
    printMode(currentMode);
}

// ═══════════════════════════════════════════════════════════
// LOOP
// ═══════════════════════════════════════════════════════════
void loop() {
    // Все в одному loop() — це один FreeRTOS Task
    // xTaskCreate() для окремих задач — поза межами курсу
    handleButtonAndSwitchMode();
    handleMode();  
}
    