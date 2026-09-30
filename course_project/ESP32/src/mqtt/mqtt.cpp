#include "mqtt.h"
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "../net/net.h"
#include "../secrets.h"
#include "../version.h"
#include "../shadow/shadow.h"
#include "../tools/tools.h"

#define MQTT_PORT        8883                                 // MQTT over TLS, НЕ 1883!
#define TOPIC_TELEMETRY  "iot-course/yakymovich/telemetry2"    // вгору: публікуємо
#define TOPIC_EVENTS     "iot-course/yakymovich/events"       // вгору:  публікуємо після події
#define TOPIC_CMD        "iot-course/yakymovich/commands/led" //  вниз: слухаємо

// ═══ ТОПІКИ РОБІТ (Заняття 15) ═══
// Зарезервовані топіки AWS — починаються з $aws, ми їх не вигадуємо.
// Складаємо через THINGNAME, бо в них зашите ім'я Thing.
// $next — службове слово «наступна робота в черзі», jobId знати не треба.
#define TOPIC_JOBS_NOTIFY  "$aws/things/" THINGNAME "/jobs/notify-next"
#define TOPIC_JOBS_GET     "$aws/things/" THINGNAME "/jobs/$next/get"
#define TOPIC_JOBS_ACCEPT  "$aws/things/" THINGNAME "/jobs/$next/get/accepted"

// Топіки тіні — у shadow.h, поруч із логікою, яка їх обслуговує.

#define RECONNECT_INTERVAL 5000  // мс між спробами

static WiFiClientSecure net;
static PubSubClient     mqttClient(net);

static unsigned long lastReconnectAttempt = 0;

// Команда: пише callback, читає setup() — тому volatile на прапорці
static char          commandBuf[64];
static volatile bool commandReady = false;

// Робота: окремий буфер, ВЕЛИКИЙ.
// Документ з presigned URL — це не 256 байт. 3 КБ із запасом.
static char          jobBuf[3072];
static volatile bool jobReady = false;

bool timeSynchronized;

// ═══════════════════════════════════════════════════════════
// CALLBACK — ДИСПЕТЧЕР на чотири види топіка.
// Правило те саме, що на Занятті 14: скопіювати, підняти прапорець, вийти.
// Жодної роботи тут: буфер PubSubClient спільний на вхід і вихід,
// а поки ми в callback — не летить PING і брокер рахує нас мертвими.
// Завантаження прошивки тим паче тут робити НЕ МОЖНА — це десятки секунд.
// ═══════════════════════════════════════════════════════════
static void onMessage(char* topic, byte* payload, unsigned int length) {

    // ── Тінь (Заняття 16) ──
    // Віддаємо в свій модуль. Він поверне true, якщо топік був його.
    if (shadow_on_message(topic, payload, length)) {
        return;
    }

    // ── Команда на світлодіод (Заняття 14) ──
    if (strstr(topic, "/commands/led") != NULL) {
        unsigned int n = length;
        // sizeof(commandBuf), а НЕ sizeof(payload):
        // payload — вказівник, sizeof від нього дасть 4
        if (n > sizeof(commandBuf) - 1) {
            n = sizeof(commandBuf) - 1;      // -1 — місце під '\0'
        }
        memcpy(commandBuf, payload, n);
        commandBuf[n] = '\0';                // payload прийшов без термінатора
        commandReady = true;
        return;
    }

    // ── Сповіщення «є нова робота» ──
    // Нічого не копіюємо: одразу просимо повний документ.
    // publish із callback — коротка операція, це прийнятно.
    if (strstr(topic, "/jobs/notify-next") != NULL) {
        mqttClient.publish(TOPIC_JOBS_GET, "{}");
        return;
    }

    // ── Приїхав документ роботи ──
    if (strstr(topic, "/jobs/$next/get/accepted") != NULL) {
        if (length >= sizeof(jobBuf)) return;   // не вліз — ігноруємо
        memcpy(jobBuf, payload, length);
        jobBuf[length] = '\0';
        jobReady = true;
        return;
    }
}

bool mqtt_begin() {
    // 1. Wi-Fi
    if (!net_wifi_connect()) {
        Serial.println("[AWS] Немає Wi-Fi — далі йти немає сенсу");
        return false;
    }

    // 2. Час — до сертифікатів, до connect()
    for(uint8_t i = 0 ; i < 3 ; i++) 
    {
        timeSynchronized = net_time_sync();
        if(timeSynchronized)break;
    }

    // 3. Три файли з Заняття 10 у TLS-клієнт
    net.setCACert(AWS_CERT_CA);
    net.setCertificate(AWS_CERT_CRT);
    net.setPrivateKey(AWS_CERT_PRIVATE);

    // 4. Брокер: наш AWS endpoint, порт 8883
    mqttClient.setServer(AWS_IOT_ENDPOINT, MQTT_PORT);

    // Буфер підняли з 512 до 3072!
    // Документ роботи з presigned URL у 512 НЕ ВЛІЗЕ — обріжеться МОВЧКИ,
    // а ArduinoJson поверне IncompleteInput і виглядатиме як баг парсера.
    // Документ тіні з get/accepted теж чималий: крім state там їде
    // metadata з мітками часу на кожне поле.
    mqttClient.setBufferSize(3072);

    // Callback ставимо ДО connect() — щоб не проґавити повідомлення
    mqttClient.setCallback(onMessage);

    mqttClient.setKeepAlive(60);       // PING кожні 60 секунд
    mqttClient.setSocketTimeout(30);   // таймаут TCP сокету

    return true;
}

// Client ID = THINGNAME — Policy обмежує Connect саме по ньому
bool mqtt_connect() {
    Serial.print("[MQTT] Підключаємось до AWS IoT Core...");

    // Коротка форма connect() = cleanSession true.
    // Тобто персистентної сесії немає: команди, надіслані поки ми спали,
    // не чекають на брокері. Це свідомий вибір — керованість під час сну
    // вирішує Device Shadow, а не черга подій (Заняття 16).
    if (!mqttClient.connect(THINGNAME)) {
        // state(): -2 = TLS/handshake (перевір час і endpoint),
        //           5 = відмовлено (перевір Policy)
        Serial.print(" помилка: ");
        Serial.println(mqttClient.state());
        return false;
    }

    Serial.println(" OK");
     


    // ── Підписка на команди (Заняття 14) ──
    // QoS 1: команда унікальна, повторити її нікому.
    if (mqttClient.subscribe(TOPIC_CMD, 1)) {
        Serial.println("[MQTT] Підписані на commands/led");
    } else {
        Serial.println("[MQTT] ПІДПИСКА commands НЕ ВДАЛАСЬ");
    }
 


    // ── Підписки на роботи (Заняття 15) ──
    // Всі підписки ТУТ, а не в setup(): реконект = нова сесія = нуль підписок.
    // Після Deep Sleep setup() виконується заново — тому підписки
    // відновлюються самі, безкоштовно.
    mqttClient.subscribe(TOPIC_JOBS_NOTIFY, 1);
    mqttClient.subscribe(TOPIC_JOBS_ACCEPT, 1);
    Serial.println("[MQTT] Підписані на jobs");
    

   
    // ── Підписки на тінь (Заняття 16) ──
    // Перелічуємо ЯВНО. Не shadow/# — AWS час від часу додає нові топіки
    // в цю структуру, і решітка почне тягнути те, чого ми не просили,
    // а кожне зайве повідомлення — це радіо.
    // ЯКЩО ТУТ МОВЧАННЯ: перевір Policy. Без shadow-топіків у політиці
    // підписка «вдається», але дельта не приходить ніколи. Помилки не буде.
    mqttClient.subscribe(TOPIC_SHADOW_DELTA, 1);
    mqttClient.subscribe(TOPIC_SHADOW_GET_ACC, 1);
  
     
    // Відповідь на НАШ запис у shadow/update. Без цих двох звіт летить
    // у порожнечу: ми не дізнаємось ні що reported оновлено, ні що
    // документ відхилено — і дельта повертатиметься щопробудження.
    mqttClient.subscribe(TOPIC_SHADOW_UPD_ACC, 1);
    mqttClient.subscribe(TOPIC_SHADOW_UPD_REJ, 1);
    Serial.println("[MQTT] Підписані на shadow");
  

 
    // ── КРИТИЧНО: одразу питаємо, чи є робота ──
    // Поки нас не було, роботу могли створити. notify-next тоді не прилетить —
    // її треба забрати самим. Без цього рядка пропустимо оновлення після сну.
   
    mqtt_request_next_job();
    
    return true;
}

bool mqtt_connected() {
    return mqttClient.connected();
}

void mqtt_poll() {
    mqttClient.loop();
}

// ═══════════════════════════════════════════════════════════
// АКТИВНЕ ОЧІКУВАННЯ — серце моделі «прокинувся-зробив-заснув».
//
// MQTT не має запиту й відповіді. Ми публікуємо запит у $next/get —
// і функція повертається за мілісекунди, нічого не принісши. Відповідь
// прилетить окремим повідомленням, у інший топік, і потрапить до нас
// РІВНО одним шляхом: через callback, який викликає mqttClient.loop().
//
// Тому delay() тут не працює: він не читає сокет. Час минув — і що?
// Повідомлення обробляється не коли минув час, а коли ми крутнули poll.
//
// Раніше цього не було потрібно: loop() крутився вічно й сам ловив
// документ. З Deep Sleep loop() порожній — чекати мусить setup().
//
// Вихід достроковий, і це не косметика: дві секунди чекання на ~100 мА
// коштують дорожче, ніж хвилина сну. Відповідь зазвичай приходить
// за 200-400 мс. Півтори зекономлені секунди на кожному пробудженні —
// це різниця в кілька діб автономної роботи.
// ═══════════════════════════════════════════════════════════
bool mqtt_wait_for_job(unsigned long timeoutMs) {
    unsigned long start = millis();

    while (millis() - start < timeoutMs) {
        mqttClient.loop();          // ОСЬ ЗАРАДИ ЧОГО ВСЕ

        if (jobReady) {
            Serial.print("[JOBS] Відповідь за ");
            Serial.print(millis() - start);
            Serial.println(" мс");
            return true;            // прийшло — не палимо радіо далі
        }

        delay(10);                  // не молотимо CPU даремно
    }

    Serial.println("[JOBS] Таймаут — робіт у черзі немає");
    return false;
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
    // інакше mbedTLS-контекст лишається «напівживим» і з часом зʼїдає купу
    net.stop();

    mqtt_connect();   // підписки + запит роботи всередині
}

const char* mqtt_take_command() {
    if (!commandReady) return NULL;
    commandReady = false;      // скидаємо ДО обробки
    return commandBuf;
}

// Віддаємо сирий документ. Парсить ota.cpp (варіант А).
const char* mqtt_take_job() {
    if (!jobReady) return NULL;
    jobReady = false;
    return jobBuf;
}

void mqtt_disconnect_for_ota() {
    mqttClient.disconnect();
    net.stop();   // саме net.stop() — інакше контекст mbedTLS лишиться в купі
    Serial.println("[MQTT] Зʼєднання закрито на час OTA");
}

void mqtt_request_next_job() {
    // Порожнє тіло: сам факт публікації в get — це і є запит
    mqttClient.publish(TOPIC_JOBS_GET, "{}");
}

void mqtt_report_status(const char* jobId, const char* status) {
    // Топік звіту містить jobId усередині себе — ось навіщо ota його зберігає
    char topic[160];
    snprintf(topic, sizeof(topic),
             "$aws/things/" THINGNAME "/jobs/%s/update", jobId);

    char payload[64];
    snprintf(payload, sizeof(payload), "{\"status\":\"%s\"}", status);

    mqttClient.publish(topic, payload);
    Serial.print("[JOBS] Статус ");
    Serial.print(status);
    Serial.print(" для ");
    Serial.println(jobId);
}

// received_at тут НЕМА — його дописує правило в хмарі
void mqtt_publish_telemetry(float temperature, float humidity, float lux) {
    if (!mqttClient.connected()) {
        Serial.println("[MQTT] Не підключено — пропускаємо");
        return;
    }

    time_t now;
    time(&now);

     // firmware_version — щоб у хмарі було видно, хто на якій версії.
    // Після OTA це єдиний доказ, що оновлення відбулось.
    char payload[200];
    snprintf(payload, sizeof(payload),
        "{\"device_id\":\"%s\",\"timestamp\":\"%lu\",\"temperature\":\"%.1f\",\"humidity\":\"%.1f\",\"light_level\":\"%.1f\",\"firmware_version\":\"%s\",\"uptime_sec\":\"%lu\",\"free_heap\":\"%lu\"}",
        THINGNAME, (unsigned long)now, temperature, humidity, lux,
        FIRMWARE_VERSION, millis(), ESP.getFreeHeap());

    Serial.print("[MQTT] Публікуємо: ");
    Serial.println(payload);

    bool ok = mqttClient.publish(TOPIC_TELEMETRY, payload);
    Serial.println(ok ? "[MQTT] OK" : "[MQTT] Помилка публікації");
}

// Потрібна shadow.cpp: клієнт живе static тут і назовні не видний.
bool mqtt_publish_raw(const char* topic, const char* payload) {
    return mqttClient.publish(topic, payload);
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
         case BUTTON_EVENT:
         {
            snprintf(event, sizeof(event), "\"event\":\"manual_trigger\",\"value\":\"device up\""); 
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

