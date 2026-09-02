#include <Arduino.h>
#include "sensors.h"
#include "mqtt.h"


// Стан світлодіода та кнопки
unsigned long lastDebounce    = 0;
bool          lastButtonState = HIGH;
bool          buttonState     = HIGH;
bool          ledState        = LOW;

// Таймер опитування сенсорів
unsigned long lastSensorsMonitor = 0;
DHT        dht(DHTT_PIN, DHTT_TYPE);

// Таймер reconnect — чекаємо 5 секунд між спробами
// щоб не штурмувати брокер при нестабільному з'єднанні
unsigned long lastReconnectAttempt = 0;
// Лічильник спроб реконнекту
uint8_t reconnectAttempts = 0;


// ═══════════════════════════════════════════════════════════
// ТАЙМЕР ПУБЛІКАЦІЇ
// ═══════════════════════════════════════════════════════════
unsigned long lastPublish = 0;


DHTTData dhttpayload;
LDRData ldrpayload;

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

// Вивід у Serial відліку часу
void printTimeStamp(unsigned long timestamp) {
    Serial.print("Час роботи: "); Serial.print(timestamp); Serial.println(" мс");
}

// Обробка кнопки — публікуємо команду manual_read, переривання не використовуємо
void handleButton()
{
  bool reading = digitalRead(BUTTON_PIN);

  if (reading != lastButtonState) {
    lastDebounce = millis();
  }

  if (millis() - lastDebounce > DEBOUNCE) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == LOW) {
            publishCommand(COMMAND_MANUAL_READ);
      }
    }
  }

  lastButtonState = reading;
} 

// Обробка таймера, читання сенсорів та публікація
void publishSensors()  
{
   if (due(lastPublish, PUBLISH_INTERVAL)) {                                                           
      printTimeStamp(millis());      
      readDHT(&dhttpayload,&dht);
      readLDR(&ldrpayload);

      uint8_t status = validateSensors(&dhttpayload, &ldrpayload);    // Перевіряємо статус       
      if(status == STATUS_OK) {                                       
           publishSensorsData(dhttpayload.temperature, dhttpayload.humidity, ldrpayload.lux);           
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

void tryReconnect()
{
     // Не штурмуємо брокер — чекаємо RECONNECT_INTERVAL мс
       if(reconnectAttempts < RECONNECT_ATTEMPTS)
        { 
          if (due(lastReconnectAttempt, RECONNECT_INTERVAL)) {          
             reconnectAttempts++;                         
             Serial.print("[MQTT] З'єднання втрачено — перепідключаємось... Спроба № ");            
             Serial.print(reconnectAttempts);
             Serial.println();
             if(connectMQTT())reconnectAttempts = 0; //Якщо коннект є - скидаємо лічильник           
           }           
        }
        else if(reconnectAttempts == RECONNECT_ATTEMPTS)
        {
           Serial.println("[MQTT] Ліміт спроб з'єднання перевищено. Перевірте мережу та перезавантажте пристрій!");           
           reconnectAttempts++;   // Виводимо повідомлення тільки 1 раз       
        }
}

// ═══════════════════════════════════════════════════════════
// SETUP
// ═══════════════════════════════════════════════════════════
void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("ESP32-A старт");

    connectWifi();
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    mqttClient.setKeepAlive(60);        // PING кожні 60 секунд
    mqttClient.setSocketTimeout(30);    // таймаут TCP сокету 30 секунд
    connectMQTT();
}

// ═══════════════════════════════════════════════════════════
// LOOP
// ═══════════════════════════════════════════════════════════
void loop() {
    if (mqttClient.connected()) {
        // ОБОВ'ЯЗКОВО — підтримує Keep Alive з'єднання з брокером
        mqttClient.loop();
        handleButton();
        publishSensors();
    } else {
        tryReconnect();       
    }    
}