#include <Arduino.h>
#include <PubSubClient.h>
#include "wifi1.h"

// Таймер reconnect — чекаємо 5 секунд між спробами
// щоб не штурмувати брокер при нестабільному з'єднанні
unsigned long lastReconnectAttempt = 0;
uint8_t reconnectAttempts= 0;


// ═══════════════════════════════════════════════════════════
// MQTT КЛІЄНТ
// ═══════════════════════════════════════════════════════════
// WiFiClient — TCP з'єднання
// PubSubClient — MQTT протокол поверх TCP
WiFiClient   wifiClient;
PubSubClient mqttClient(wifiClient);

volatile bool manTriggerReceived = false;

// ═══════════════════════════════════════════════════════════
// CALLBACK — викликається автоматично при вхідному повідомленні
// Викликається всередині mqttClient.loop()
// Не використовувати delay() і publish() всередині
// ═══════════════════════════════════════════════════════════
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

   if(strstr(topic, "sensors") != NULL) 
   {
    char* tempPtr = strstr(message, "\"temperature\":");
    if (tempPtr != NULL) {           
           
     // Зсуваємось на 14 символів — довжина "temperature":
      float temperature = atof(tempPtr + 14);
      Serial.print("[MQTT] Температура: ");
      Serial.println(temperature);

      // Керуємо LED на основі температури
      // Замінити пороги 26.0 і 20.0 на власні значення
      if (temperature > 26.0) {
         digitalWrite(EXT_LED_PIN, HIGH);
         Serial.println("[LED] ON — вище 26°C");
      } else if (temperature < 20.0) {
         digitalWrite(EXT_LED_PIN, LOW);
         Serial.println("[LED] OFF — нижче 20°C");
      } else {
         Serial.println("[LED] Без змін — температура в нормі");
      } 
     }else
         Serial.println("[MQTT] Температура не знайдена");  
      
        return;

    }  
    
    if(strstr(topic, "commands") != NULL)
    {
         char* commandPtr = strstr(message, "manual_read"); 
         if(commandPtr != NULL)
         {            
             manTriggerReceived = true;            
         }else
             Serial.println("[MQTT] Команда не знайдена");                        
    }
  }


void doLedBlinks(uint8_t times)
{
    uint8_t state = digitalRead(EXT_LED_PIN);

    for(int i = 0 ;i < times * 2; i++)
    {
      state = (state == HIGH)?LOW:HIGH;
      digitalWrite(EXT_LED_PIN, state);
      delay(200); 
    }
}
// ═══════════════════════════════════════════════════════════
// MQTT
// ═══════════════════════════════════════════════════════════
bool connectMQTT() {
    Serial.print("[MQTT] Підключаємось до ");
    Serial.print(MQTT_BROKER);
    Serial.print("...");

    if (mqttClient.connect(MQTT_CLIENT_ID)) {
        Serial.println(" OK");

        // Підписуємось всередині connectMQTT — не в setup()
        // Бо Clean Session = true скидає підписки при кожному відключенні
        // Так підписка автоматично відновлюється після reconnect
        mqttClient.subscribe(TOPIC_SENSORS);
        Serial.print("[MQTT] Підписались на: ");
        Serial.println(TOPIC_SENSORS);

        mqttClient.subscribe(TOPIC_COMMANDS);
        Serial.print("[MQTT] Підписались на: ");
        Serial.println(TOPIC_COMMANDS);
        return true;
    }

    // mqttClient.state() повертає код помилки:
    // -4 = таймаут, -2 = сервер не знайдено, 5 = відмовлено в доступі
    Serial.print(" помилка: ");
    Serial.println(mqttClient.state());
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
    pinMode(EXT_LED_PIN, OUTPUT);
    digitalWrite(EXT_LED_PIN, LOW);  // LED вимкнено при старті
    Serial.println("ESP32-B старт");

    connectWifi();
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    mqttClient.setCallback(onMessage);  // реєструємо callback до підключення
    mqttClient.setKeepAlive(60);        // PING кожні 60 секунд
    mqttClient.setSocketTimeout(30);    // таймаут TCP сокету 30 секунд
    connectMQTT();
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
            Serial.println("Manual trigger received.");
            doLedBlinks(3);
            manTriggerReceived = false; 
        }

    } else {        
         tryReconnect();
    }
}