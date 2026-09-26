#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "mqtt.h"
#include "../net/net.h"
#include "../tools/tools.h"
#include "../secrets.h"

#define MQTT_PORT        8883                           // MQTT over TLS, НЕ 1883!
#define TOPIC_TELEMETRY  "iot-course/yakymovich/telemetry"    // вгору: публікуємо
#define TOPIC_CMD        "iot-course/yakymovich/commands/led" // вниз: слухаємо
#define TOPIC_EVENTS     "iot-course/yakymovich/events"       // Зворотний зв'зок після виконання команди

#define RECONNECT_INTERVAL 5000  // мс між спробами


//Флаги для обробки критичних помилок  
bool timeSynchronized = false;

static WiFiClientSecure net;
static PubSubClient     mqttClient(net);

static unsigned long lastReconnectAttempt = 0;

// Пише callback, читає loop() — тому volatile на прапорці
static char          commandBuf[64];
static volatile bool commandReady = false;

// ═══════════════════════════════════════════════════════════
// CALLBACK — смикає його mqttClient.loop(), коли прийшло повідомлення
// Правило: скопіювати, підняти прапорець, вийти.
// Жодної роботи тут: буфер PubSubClient спільний на вхід і вихід,
// а поки ми в callback — не летить PING і брокер рахує нас мертвими
// ═══════════════════════════════════════════════════════════
static void onMessage(char* topic, byte* payload, unsigned int length) {
    // topic не розбираємо: підписка одна.
    // Зʼявиться commands/fan — роутинг по topic буде саме тут
    
    Serial.println("Callback");

    unsigned int n = length;

    // sizeof(commandBuf), а НЕ sizeof(payload):
    // payload — вказівник, sizeof від нього дасть 4
    if (n > sizeof(commandBuf) - 1) {
        n = sizeof(commandBuf) - 1;      // -1 — місце під '\0'
    }

    memcpy(commandBuf, payload, n);
    commandBuf[n] = '\0';                // payload прийшов без термінатора
            
    commandReady = true;                 // роботу зробить loop()
}

bool mqtt_begin() {
    // 1. Wi-Fi
    if (!net_wifi_connect()) {
        Serial.println("[AWS] Немає Wi-Fi — далі йти немає сенсу");
        return false;
    }

    // 2. Час — до сертифікатів, до connect()
    // Робимо 3 спроби синхронізації часу
    for(uint8_t i = 0 ; i < 3 ; i++) 
    {
        timeSynchronized = net_time_sync();
        if(timeSynchronized)break;
    }

    if (!timeSynchronized) {
        Serial.println("[AWS] Час не синхронізовано — TLS впаде");
        return false;
    }

    // 3. Три файли з Заняття 10 у TLS-клієнт
    net.setCACert(AWS_CERT_CA);
    net.setCertificate(AWS_CERT_CRT);
    net.setPrivateKey(AWS_CERT_PRIVATE);

    // 4. Брокер: наш AWS endpoint, порт 8883
    mqttClient.setServer(AWS_IOT_ENDPOINT, MQTT_PORT);

    // Буфер за замовчуванням 256 байт — замалий, мовчки обрізає JSON.
    // Один і той самий буфер обслуговує і вхід, і вихід
    mqttClient.setBufferSize(512);

    // Callback ставимо ДО connect() — щоб не проґавити повідомлення
    mqttClient.setCallback(onMessage);

    mqttClient.setKeepAlive(60);       // PING кожні 60 секунд
    mqttClient.setSocketTimeout(30);   // таймаут TCP сокету

    return true;
}

// Client ID = THINGNAME — Policy обмежує Connect саме по ньому
bool mqtt_connect() {
    Serial.print("[MQTT] Підключаємось до AWS IoT Core...");

    if (mqttClient.connect(THINGNAME)) {
        Serial.println(" OK");

        // QoS 1: команда унікальна, повторити її нікому.
        // Телеметрію женемо QoS 0 — там наступний пакет перекриє втрачений
        if (mqttClient.subscribe(TOPIC_CMD, 1)) {
            Serial.println("[MQTT] Підписані на commands/led");
        } else {
            Serial.println("[MQTT] ПІДПИСКА НЕ ВДАЛАСЬ");
        } 
        return true;
    }

    // state(): -2 = TLS/handshake (перевір час і endpoint),
    //           5 = відмовлено (перевір Policy)
    Serial.print(" помилка: ");
    Serial.println(mqttClient.state());
    return false;
}

bool mqtt_connected() {
    return mqttClient.connected();
}

void mqtt_poll() {
    mqttClient.loop();
}

void mqtt_reconnect_tick() {

    if(!due(lastReconnectAttempt, RECONNECT_INTERVAL))
    return;

    Serial.println("[MQTT] Зʼєднання втрачено — перепідключаємось...");

    // Спершу мережа: без Wi-Fi реконект MQTT безнадійний
    if (!net_wifi_connected()) {
        net_wifi_connect();
    }

    // Явно закриваємо стару TLS-сесію перед новою спробою —
    // інакше mbedTLS-контекст лишається "напівживим" і з часом зʼїдає купу
    net.stop();

    mqtt_connect();   // subscribe() всередині — підписка відновиться
}

const char* mqtt_take_command() {
    if (!commandReady) return NULL;
    commandReady = false;      // скидаємо ДО обробки
    return commandBuf;
}

// received_at тут НЕМА — його дописує правило в хмарі
void mqtt_publish_telemetry(float temperature, float humidity, float lux) {
    if (!mqttClient.connected()) {
        Serial.println("[MQTT] Не підключено — пропускаємо");
        return;
    }

    time_t now;
    time(&now);

    // snprintf обріже по межі й не впаде — але JSON прилетить
    // битий, і правило його не розбере
    char payload[160];
   

    snprintf(payload, sizeof(payload),
        "{\"temperature\":%.1f,\"humidity\":%.1f,\"lux\":%1.f,\"device_id\":\"%s\",\"timestamp\":%lu}",
        temperature, humidity, lux, THINGNAME, (unsigned long)now);

    Serial.print("[MQTT] Публікуємо: ");
    Serial.println(payload);

    bool ok = mqttClient.publish(TOPIC_TELEMETRY, payload);
    Serial.println(ok ? "[MQTT] OK" : "[MQTT] Помилка публікації");
}

// Збирання payload для event-у
bool make_event_payload(char* payload, uint8_t length, uint8_t event_id, uint8_t value )
{
    
    if(payload == NULL || length == 0 || length > 254) 
    { 
       Serial.println("[Device] Хибні параметри буфера!");  
       return false;
    } 
    
    char event[50];

  // Якщо треба публікувати інші EVENT-и - додаємо сюди.
  // Або замість switch використовуємо якусь іншу обробку
    switch(event_id)
    {
        case LED_EVENT: 
         {                                                       
                if (value == 1) 
                    snprintf(event, sizeof(event), "\"event\":\"led_changed\",\"value\":\"on\"");
                else
                    snprintf(event, sizeof(event), "\"event\":\"led_cahanged\",\"value\":\"off\"");
            break;  
         }
         default:
         {            
            Serial.println("[Device] Невідома подія!");
            return false;
         }
    }
    
    time_t now;
    time(&now);

    snprintf(payload, length, 
            "{\"device_id\":\"%s\",\"timestamp\":\"%lu\",%s}", 
            THINGNAME, (unsigned long)now, event); 
            
  return true;
}


// Публікація event-а
void mqtt_publish_event(uint8_t event_id, uint8_t value){
       
    char payload[160];
    if(make_event_payload(payload, sizeof(payload), event_id, value))
    { 
        //  Звісно, публікувати event треба із QoS = 1 але - ...
        bool ok = mqttClient.publish(TOPIC_EVENTS, payload);
        Serial.print("Публікуємо: ");
        Serial.println(payload);
        Serial.println(ok ? "[MQTT] OK" : "[MQTT] Помилка публікації");
    }     
}