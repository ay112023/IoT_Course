# ДЗ № 5: MQTT over TLS до AWS IoT Core (ESP32)

Демонстрація захищеного підключення ESP32 до **AWS IoT Core** через MQTT over TLS (порт 8883) із взаємною автентифікацією за сертифікатами (mTLS). Неблокуючий таймер на `millis()` публікує телеметрію кожні 30 секунд; автоматичний reconnect відновлює з'єднання без `delay()` (Arduino Framework, PlatformIO + Wokwi).


## Архітектура
  
┌──────────────┐
│  ESP32       │  Wokwi
│  (DHT22 sim) │
└──────┬───────┘
       │ MQTT over TLS, порт 8883
       │ топік: iot-course/yakymovich/telemetry
       │ payload: {"temperature","humidity","lux","device_id","timestamp"}
       ▼
┌───────────────────────────────────────────────────────────────────────────┐
│  AWS IoT Core (eu-north-1)                                                | 
│  ┌────────────────────────────────┐   ┌──────────────────────────────┐    │  
│  │  Rules Engine                  │   | Rules Engine                 │    |
│  │  rule_iot_telemetry            │   | rule_iot_telemetry_alert     |    │
│  │  SELECT * + timestamp()        │   |  SELECT * ...                |    │
│  │           + clientid()         │   |   where temperature > 28     |    │
│  │           + topic(2)           │   |                              |    │
│  └────────────┬───────────────────┘   └────────┬─────────────────────┘    |
└───────────────┼────────────────────────────────┼──────────────────────────┘
                │  Action dynamoDBv2             | Action CloudWatch Logs                  
                │  IAM-роль:                     |  IAM-роль: iot_telemetry_alert
                |     iot_telemetry_add,         |
                |  Error Action CloudWatch Logs  |    
                |  IAM-роль:                     | 
                |     iot_telemetry_error_action |
                ▼                                ▼
        ┌───────────────────┐           ┌───────────────────┐  
        │  DynamoDB         │           |                   |
        │  iot_telemetry    │           |  CloudWatch Logs  | 
        |                   |           |                   |
        │  pk: device_id    │           └───────────────────┘ 
        │  sk: received_at  │
        |  --------------   | 
        |  CloudWatch Logs  |  
        └───────────────────┘
                
                

## Пояснення до архітектури

   ESP32:     
      Сенсори опитуються із інтервалом у 30 секунд
      із-за чого KeepAlive нв MQTT клієнті було встановлено у 100 секунд бо
      при значенні = 60 коннект із AWS відвалювався та постійно відбувався реконнект.
      Обробка помилок: при винивненні критичних помлок LED блимає
          - Не піднявся WiFi                        - 2 рази
          - За 3 спроби не синхронізувався час      - 3 рази
          - За 3 спроби не пройшов реконнект MQTT   - 4 рази
         блимання LED-ом блокуюче, бо якщо виникають критичні помилки 
         ніякі інші дії не виконуються. У стані обробки критичних помилок
         доступне "аварійне" перезавантаження пристрою кнопкою, для чого 
         використовється переривання. :) 
         У поле timestamp -> transmitted_at() передається значення millis() а не time()
         бо так зручніше контролювати рядки у таблиці iot_telemetry у DynamoDB та LogStreams у CloudWatch.


     AWS:
       Cтворено Thing що обмежується
       політикою.
      
       Створена таблиця DynamoDBv2: 
             iot_telemetry
       
       Створені rules у Rules Engine:
         - rule_iot_telemetry із IAM-role iot_telemetry_add                    
           та Action для додавання записів у таблицю iot_telemetry DynamoDBv2
           й Error Action для додавання логів у CloudWatch
         
         - rule_iot_telemetry_alert із IAM-role iot_telemetry_alert
           для додавання логів у CloulWatch, якщо температура > 28 С.


## Структура проєкту

```
include/
├── config.h           — основний файл конфігурації
├── secrets.h          — Wi-Fi, endpoint і сертифікати (У .gitignore!)
└── secrets.example.h  — шаблон secrets.h для копіювання
    mqtt.h             - header бібліотеки для роботи із MQTT
    ntp.h              - header бібліотеки для роботи із NTP
    sensors.h          - header бібліотеки для роботи із сенсорами
    wifi1.h            - header бібліотеки для роботи із WiFi 
src/
├── main.cpp             — основний файл прошивки
    mqtt.cpp             - бібліотека для роботи із MQTT
    ntp.cpp              - бібліотека для роботи із NTP
    sensors.cpp          - бібліотека для роботи із сенсорами
    wifi1.cpp            - бібліотека для роботи із сенсорами 
screeenshots/            - скріншоти, що демнострують роботу:
 ├──    1.jpg - Налаштування Thing
        2.jpg - Налаштування сертифікату із policy         
        3.jpg - Permissions для  policy my_policy1
        4.jpg - Rules, що створені
        5.jpg - Налаштування rule_iot_telemetry
        6.jpg - Налаштування rule_iot_telemetry
        7.jpg - Налаштування rule_iot_telemetry_alert
        8.jpg - IAM Role iot_telemetry_add
        9.jpg - Permissions IAM Role iot_telemetry_add
       10.jpg - Permissions IAM Role iot_telemetry_error_action
       11.jpg - IAM Role iot_telemetry_alert
       12.jpg - Permissions IAM Role iot_telemetry_alert
       14.jpg - Робота у WOKWI, публікація топіків
       15.jpg - Таблиця iot_telemetry
       16.jpg - Log groups
       17.jpg - Log streams iot_telemetry_alert
       18.jpg - Log events  iot_telemetry_alert
diagram.json           — схема підключення для Wokwi-симулятора
wokwi.toml             — конфігурація Wokwi
platformio.ini         — конфігурація PlatformIO
```

---

## Налаштування secrets.h

`secrets.h` містить приватний ключ і **до git не потрапляє** (у `.gitignore`).

1. Скопіювати `src/secrets.example.h` → `src/secrets.h`.
2. Вписати `THINGNAME` (= ім'я Thing) та `AWS_IOT_ENDPOINT`
   (AWS IoT Console → Settings → Device data endpoint).
3. Вставити вміст трьох `.pem` файлів, завантажених у Занятті 9:
   - `AmazonRootCA1.pem` → `AWS_CERT_CA` — перевірка сервера («це справді AWS?»)
   - `certificate.pem.crt` → `AWS_CERT_CRT` — паспорт пристрою («ось хто я»)
   - `private.pem.key` → `AWS_CERT_PRIVATE` — секретний доказ («паспорт справді мій»)

> **Client ID має дорівнювати `THINGNAME`** — AWS Policy обмежує Connect саме по ньому (слайд 16).

---
## Залежності

```ini
lib_deps =
    knolleary/PubSubClient
    adafruit/DHT sensor library
```

`WiFi.h` і `WiFiClientSecure.h` входять до складу ESP32 Arduino core — додаткових бібліотек для TLS не потрібно.

---

## Опис main.cpp

### 1 Контроль неблокуючих таймерів  `bool due(unsigned long& last, unsigned long interval)`
       Викликається для контролю інтервалів часу

### 2 Переривання для кнопки `void IRAM_ATTR onButtonPress()`
       Використовується для обробки натискання кнопки для аваріного перезавантаження пристрою
       при критичних помилках

### 3 Блокуюче блимання LED `void blLEDBlink(uint8_t times)`
      Використовується для індикації критичних помилок, що виникли

### 3  Коннект з AWS IoT Core `boolean connectAWS()`
      З'єднання зі хмарою (Конyект із WiFi, синхронізація часу по NTP, встановлення сертифікатів та ключів)

### 4  Вивід у Serial відміток часу `void printTimeStamp(unsigned long timestamp) `

### 5  Публікація сенсорів `void publishSensors()` 

### 6  Неблокуючий реконнект `void  tryReconnect()`

### 7  Обробка помилок `uint8_t isErrorsHandled()`
      Блокуюче блимання LED-ом:
          - Не піднявся WiFi                        - 2 рази
          - За 3 спроби не синхронізувався час      - 3 рази
          - За 3 спроби не пройшов реконнект MQTT   - 4 рази
### 8   loop()
     
      Перевіряємо чи є критичні помилки та
      якщо вони є - LED блимає, по
      натисканню кнопки можемо перезавантажити пристрій.
      Якщо помилок нема - штатна робота пристрою.