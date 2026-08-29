#include <Arduino.h>
#include <PubSubClient.h>
#include "wifi1.h"
#include "sensors.h"


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
uint8_t reconnectAttempts = 0;


// ═══════════════════════════════════════════════════════════
// ТАЙМЕР ПУБЛІКАЦІЇ
// ═══════════════════════════════════════════════════════════
unsigned long lastPublish = 0;

// ═══════════════════════════════════════════════════════════
// MQTT КЛІЄНТ
// ═══════════════════════════════════════════════════════════
// WiFiClient — TCP з'єднання
// PubSubClient — MQTT протокол поверх TCP
WiFiClient   wifiClient;
PubSubClient mqttClient(wifiClient);

DHTTData dhttpayload;
LDRData ldrpayload;

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

// Прототип фунції
void publishSensorsData(float temperature, float humidity, float lux);

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

// Прототип фунції
void publishCommand(const char* command, size_t size);

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
            publishCommand("manual_read",12);
      }
    }
  }

  lastButtonState = reading;
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
        return true;
    }

    // mqttClient.state() повертає код помилки:
    // -4 = таймаут, -2 = сервер не знайдено, 5 = відмовлено в доступі
    Serial.print(" помилка: ");
    Serial.println(mqttClient.state());
    return false;
}

// ═══════════════════════════════════════════════════════════
// ПУБЛІКАЦІЯ ДАНИХ
// ═══════════════════════════════════════════════════════════
void publishSensorsData(float temperature, float humidity, float lux) {
    if (!mqttClient.connected()) {
        Serial.println("[MQTT] Не підключено — пропускаємо");
        return;
    }

    // snprintf замість String — безпечно для heap (Заняття 4)
    // char буфер фіксованого розміру, ніякої фрагментації
    char payload[100];
    snprintf(payload, sizeof(payload),
        "{\"temperature\":%.1f,\"humidity\":%.1f,\"lux\":%1.f}",
        temperature, humidity, lux);
    
    Serial.print("[MQTT] Публікуємо: ");
    Serial.println(payload);

    // publish() повертає true якщо повідомлення прийнято брокером
    bool ok = mqttClient.publish(TOPIC_SENSORS, payload);
    Serial.println(ok ? "[MQTT] OK" : "[MQTT] Помилка публікації");
}

// Публікація команд
void publishCommand(const char* command, size_t size){
  if (!mqttClient.connected()) {
        Serial.println("[MQTT] Не підключено — пропускаємо");
        return;
    }
    // snprintf замість String — безпечно для heap (Заняття 4)
    // char буфер фіксованого розміру, ніякої фрагментації
    char payload[size];
    snprintf(payload, sizeof(payload),
         command);
    Serial.print("[MQTT] Публікуємо: ");
    Serial.println(payload);

    // publish() повертає true якщо повідомлення прийнято брокером
    bool ok = mqttClient.publish(TOPIC_COMMANDS, payload);
    Serial.println(ok ? "[MQTT] OK" : "[MQTT] Помилка публікації");
}

// Спроба реконнекту
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

        // Публікуємо дані кожні PUBLISH_INTERVAL мс
        // random(15, 40) — симулюємо зміну температури для демонстрації
        // Замінити на реальні дані з DHT22
        handleButton();
        publishSensors();
    } else {
        tryReconnect();       
    }    
}