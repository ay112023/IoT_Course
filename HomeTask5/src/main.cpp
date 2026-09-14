#include <Arduino.h>
#include "config.h"
#include "secrets.h"
#include "sensors.h"
#include "wifi1.h"
#include "mqtt.h"
#include "ntp.h"

// Таймер reconnect — чекаємо 5 секунд між спробами
unsigned long lastReconnectAttempt = 0;
uint8_t reconnectAttempts = 0;

// Глобальні екземпляри
DHT        dht(DHTT_PIN, DHTT_TYPE);
DHTTData   dhttpayload;
LDRData    ldrpayload;

// Флаги
bool timeNotSynchronized = false;
bool reconnectFailed = false;
bool wifiFailed = false;
volatile bool buttonPressed = false;
// ═══════════════════════════════════════════════════════════
// ТАЙМЕР ПУБЛІКАЦІЇ
// ═══════════════════════════════════════════════════════════
unsigned long lastPublish = 0;

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
// Переривання для кнопки.
void IRAM_ATTR onButtonPress() {
  buttonPressed = true; 
}
// Блокуюче блимання LED (використовуватиметься виключно тоді, коли треба сигналізувати
// про критичну помилку, що потребує втручання. В цьому випадку плата не виконуватиме 
// жодної задачи окрім блимання, тоvу допускаємо використання delay()
void blLEDBlink(uint8_t times)
{
  digitalWrite(EXT_LED_PIN, LOW);   
  for(uint8_t i = 0 ; i < times; i++)
     { 
           digitalWrite(EXT_LED_PIN, HIGH);
           delay(200);
           digitalWrite(EXT_LED_PIN, LOW);
           delay(200);           
     }
  delay(2000);
}
// ═══════════════════════════════════════════════════════════
// ПІДКЛЮЧЕННЯ ДО AWS IOT CORE
// Порядок кроків критичний: Wi-Fi → час → сертифікати → сервер
// ═══════════════════════════════════════════════════════════
boolean connectAWS() {
    // 1. Wi-Fi
    wifiFailed = !connectWifi();
    if(wifiFailed){
        Serial.println("[AWS, WiFi] WiFi недоступний. Перевірте мережу та перезавантажте пристрій!");    
        return false;
    } 

    // 2. НОВЕ: синхронізуємо час — до сертифікатів, до connect()
    // робимо 3 спроби
    for(int i = 0 ; i < 3; i++){
      timeNotSynchronized = !syncTime(); 
      if (!timeNotSynchronized) break;      
    } 

    if(timeNotSynchronized)
    {
       Serial.println("[AWS,NTP] Ліміт спроб сінхронизувати час вичерпано. Перевірте мережу та перезавантажте пристрій!");    
       return false;
    }

    // 3. НОВЕ: заряджаємо три файли зі слайда 10 в TLS-клієнт
    wifiClient.setCACert(AWS_CERT_CA);
    wifiClient.setCertificate(AWS_CERT_CRT);
    wifiClient.setPrivateKey(AWS_CERT_PRIVATE);

    // 4. Вказуємо брокер: наш AWS endpoint, порт 8883
    mqttClient.setServer(AWS_IOT_ENDPOINT, MQTT_PORT);

    // Буфер PubSubClient за замовчуванням 256 байт — замалий,
    // мовчки обрізає JSON. Збільшуємо до підключення.
    mqttClient.setBufferSize(512);

    return true;
}

// Вивід у Serial відліку часу
void printTimeStamp(unsigned long timestamp) {
    Serial.print("Час роботи: "); Serial.print(timestamp); Serial.println(" мс");
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

// Спроба реконекту
void tryReconnect()
{    
       if(reconnectAttempts < RECONNECT_ATTEMPTS)
        { 
          if (due(lastReconnectAttempt, RECONNECT_INTERVAL)) {          
             reconnectAttempts++;                         
             Serial.print("[MQTT] З'єднання втрачено — перепідключаємось... Спроба № ");            
             Serial.print(reconnectAttempts);
             Serial.println();
             wifiClient.stop();                
             if(connectMQTT()) {
              reconnectAttempts = 0;   //Якщо коннект є - скидаємо лічильник           
              reconnectFailed = false; //скидаємо флаг
             } 
           }           
        }
        else if(reconnectAttempts == RECONNECT_ATTEMPTS)
        {           
           reconnectAttempts++;     // Виводимо повідомлення тільки 1 раз  
           Serial.println("[MQTT] Ліміт спроб з'єднання перевищено. Перевірте мережу та перезавантажте пристрій!");                
           reconnectFailed = true;  // взводимо флаг
        }
}

// Обробка помилок: про критичні помилки сигналізуємо LED-ом.
uint8_t isErrorsHandled()
{    
   uint8_t errCount = 0;  
   if(wifiFailed) // Якщо WiFi не піднявся - блимаємо LED-ом 2 рази
    {                   
      blLEDBlink(2);                    
      errCount++;
    }    
    if(timeNotSynchronized) // Якщо час не синхронізовано - блмиаємо LED-ом 3 рази
    {                   
      blLEDBlink(3);             
      errCount++;
    }    
   if(reconnectFailed)     // Якщо реконнект не пройшов - блимаємо LED-ом 4 рази
    {         
       blLEDBlink(4);       
       errCount++;
    }
  return errCount;
}
// ═══════════════════════════════════════════════════════════
// SETUP
// ═══════════════════════════════════════════════════════════
void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("ESP32-A (AWS IoT Core edition) старт");
    pinMode(EXT_LED_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);  // GPIO5 — вхід з внутрішнім pull-up
    attachInterrupt(BUTTON_PIN, onButtonPress, FALLING);
    if(connectAWS())
    {
      mqttClient.setKeepAlive(100);        // PING кожні 100 секунд
      mqttClient.setSocketTimeout(30);    // таймаут TCP сокету 30 секунд
      connectMQTT();     
    } 
}
// ═══════════════════════════════════════════════════════════
// LOOP 
// ═══════════════════════════════════════════════════════════
void loop() {
    
  //Обробляємо критичні помилки
    uint8_t errCount = isErrorsHandled(); 
   
    if(buttonPressed)
    {  
      buttonPressed = false;
      // Якщо є критичні помилки та блимає LED то 
      // по натсисканню кнопки перезавантажуємо пристрій. :))
      if(errCount > 0)ESP.restart(); 
    } 
    else if (errCount > 0) 
          return;
            
    if (mqttClient.connected()) {
        // ОБОВ'ЯЗКОВО — підтримує Keep Alive з'єднання з брокером
        mqttClient.loop();       
        publishSensors();
    } else {
        tryReconnect();       
    }    
}