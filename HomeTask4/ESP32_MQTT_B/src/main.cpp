#include <Arduino.h>
#include "mqtt.h"

// Таймер reconnect — чекаємо 5 секунд між спробами
// щоб не штурмувати брокер при нестабільному з'єднанні
unsigned long lastReconnectAttempt = 0;
uint8_t reconnectAttempts= 0;
unsigned long lastLedStateChange = 0;
int ledStateChangesCount = 0;


volatile bool manTriggerReceived = false;


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
// ═══════════════════════════════════════════════════════════
// CALLBACK — викликається автоматично при вхідному повідомленні
// Викликається всередині mqttClient.loop()
// Не використовувати delay() і publish() всередині
// ═══════════════════════════════════════════════════════════
void onMessage(char* topic, byte* payload, unsigned int length) {
    // payload — масив байтів без термінатора '\0'
    // треба самостійно перетворити в C-рядок
    char message[length + 1];
    memcpy(message, payload, length);
    message[length] = '\0';  // додаємо термінатор рядка

    Serial.print("[MQTT] Топік: ");
    Serial.println(topic);
    Serial.print("[MQTT] Payload: ");
    Serial.println(message);

    // Простий парсинг JSON через strstr + atof
    // Без бібліотеки ArduinoJson — достатньо для відомої структури payload

   if(strstr(topic, TOPIC_SENSORS) != NULL) 
   {
    char* tempPtr = strstr(message, "\"temperature\":");
    if (tempPtr != NULL) {           
           
     // Зсуваємось на 14 символів — довжина "temperature":
      float temperature = atof(tempPtr + 14);
      Serial.print("[MQTT] Температура: ");
      Serial.println(temperature);

      // Керуємо LED на основі температури
      // Замінити пороги 26.0 і 20.0 на власні значення
      if (temperature > HIGH_TEMPERATURE_THRESHOLD) {
         digitalWrite(EXT_LED_PIN, HIGH);
         Serial.print("[LED] ON — вище");
         Serial.print(HIGH_TEMPERATURE_THRESHOLD);
         Serial.println("°C");

      } else if (temperature < LOW_TEMPERATURE_THRESHOLD) {
         digitalWrite(EXT_LED_PIN, LOW);
         Serial.print("[LED] OFF — нижче ");
         Serial.print(LOW_TEMPERATURE_THRESHOLD);
         Serial.println("°C");

      } else {
         Serial.println("[LED] Без змін — температура в нормі");
      } 
     }else
         Serial.println("[MQTT] Температура не знайдена");  
      
        return;
    }  
    
    if(strstr(topic, TOPIC_COMMANDS) != NULL)
    {        
         if(strstr(message, COMMAND_MANUAL_READ) != NULL)
         {            
             manTriggerReceived = true;            
         }else
             Serial.println("[MQTT] Команда не знайдена");                        
    }
  }

// Зміна стану LED з неблокуючим таймером
bool changeLedState(uint8_t times)
{
      if(due(lastLedStateChange, LED_STATE_INTERVAL))
      { 
        if(ledStateChangesCount < times) 
        {
           uint8_t state;
           state = digitalRead(EXT_LED_PIN);
           state = (state == HIGH) ? LOW : HIGH;  
           digitalWrite(EXT_LED_PIN, state);     
           ledStateChangesCount++;    
        } else {
           ledStateChangesCount = 0;  // Скидаємо лічильник після завершення блимання      
           return true;
        }            
       }
   return false;      
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
             if(connectMQTTandSubscribe())reconnectAttempts = 0; //Якщо коннект є - скидаємо лічильник           
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
    pinMode(EXT_LED_PIN, OUTPUT);
    digitalWrite(EXT_LED_PIN, LOW);  // LED вимкнено при старті
    Serial.println("ESP32-B старт");
    connectWifi();
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    mqttClient.setCallback(onMessage);  // реєструємо callback до підключення
    mqttClient.setKeepAlive(60);        // PING кожні 60 секунд
    mqttClient.setSocketTimeout(30);    // таймаут TCP сокету 30 секунд
    connectMQTTandSubscribe();
}

// ═══════════════════════════════════════════════════════════
// LOOP
// ═══════════════════════════════════════════════════════════
void loop() {
    if (mqttClient.connected()) {                 

        // ⚠️ ОБОВ'ЯЗКОВО — без цього callback не викликається
        // і брокер не отримує PING → відключає клієнта       
        mqttClient.loop();

        if(manTriggerReceived)
        {
           if(ledStateChangesCount == 0){ 
               Serial.println("Manual trigger received.");           
           }                       
           
           // Неблокуюче блимання LED 
           if(changeLedState(LED_BLINKS_MANUAL_READ * 2)){ 
              manTriggerReceived = false; 
          } 
        }

    } else {        
         tryReconnect();
    }
}