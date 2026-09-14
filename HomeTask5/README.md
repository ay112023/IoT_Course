# Upd По виконанню завдання:
  
   HARDWARE:
     
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


     У AWS:
       Cтворено Thing esp32_yakymovich що обмежується
       політикою  my_policy1.
      
       Створена таблиця DynamoDBv2: 
             iot_telemetry
       
       Створені rules у Rules Engine:
         - rule_iot_telemetry із IAM-role iot_telemetry_add                    
           та Action для додавання записів у таблицю iot_telemetry DynamoDBv2
           й Error Action для додавання логів у CloudWatch
         
         - rule_iot_telemetry_alert із IAM-role iot_telemetry_alert
           для додавання логів у CloulWatch, якщо температура > 28 С.

Зроблено скріншоти:
   1 - Налаштування Thing
   2 - Налаштування сертифікату із policy         
   3 - Permissions для  policy my_policy1
   4 - Rules, що створені
   5 - Налаштування rule_iot_telemetry
   6 - Налаштування rule_iot_telemetry
   7 - Налаштування rule_iot_telemetry_alert
   8 - IAM Role iot_telemetry_add
   9 - Permissions IAM Role iot_telemetry_add
  10 - Permissions IAM Role iot_telemetry_error_action
  11 - IAM Role iot_telemetry_alert
  12 - Permissions IAM Role iot_telemetry_alert
  14 - Робота у WOKWI, публікація топіків
  15 - Таблиця iot_telemetry
  16 - Log groups
  17 - Log streams iot_telemetry_alert
  18 - Log events  iot_telemetry_alert
   


# Лекція 10 — MQTT over TLS до AWS IoT Core (ESP32)

Демонстрація захищеного підключення ESP32 до **AWS IoT Core** через MQTT over TLS (порт 8883) із взаємною автентифікацією за сертифікатами (mTLS). Неблокуючий таймер на `millis()` публікує телеметрію кожні 10 секунд; автоматичний reconnect відновлює з'єднання без `delay()` (Arduino Framework, PlatformIO + Wokwi).

Розвиток Заняття 8: логіка MQTT ідентична, змінюється лише транспорт — `WiFiClient` → `WiFiClientSecure`, брокер HiveMQ → AWS IoT Core, порт 1883 → 8883.

---

## Структура проєкту

```
src/
├── main.cpp           — основний файл прошивки
├── secrets.h          — Wi-Fi, endpoint і сертифікати (У .gitignore!)
└── secrets.example.h  — шаблон secrets.h для копіювання
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

### 1. Wi-Fi — `connectWifi()`

Без змін із Заняття 8. Канал 6 (`WiFi.begin(..., 6)`) пропускає сканування — економить ~4 секунди в Wokwi. Повертає `false` при таймауті `WIFI_TIMEOUT`.

### 2. NTP-синхронізація часу — `syncTime()` (НОВЕ, слайд 18)

```cpp
configTime(0, 0, "pool.ntp.org");  // зсув 0, DST 0 — для TLS достатньо
```

**Без цього TLS впаде**, навіть із правильними сертифікатами: ESP32 стартує з 1970 року, і handshake вважає сертифікат AWS «ще не дійсним» (1970 < дата видачі). Час треба синхронізувати **до** `connect()`.

### 3. Підключення до AWS — `connectAWS()`

Порядок кроків критичний:

```cpp
connectWifi();                          // 1. Wi-Fi
syncTime();                             // 2. час (до сертифікатів!)
net.setCACert(AWS_CERT_CA);             // 3. три файли зі слайда 10
net.setCertificate(AWS_CERT_CRT);
net.setPrivateKey(AWS_CERT_PRIVATE);
mqttClient.setServer(AWS_IOT_ENDPOINT, 8883);
mqttClient.setBufferSize(512);          // 256 замало — мовчки обрізає JSON
```

`WiFiClientSecure net` замість `WiFiClient` — єдина зміна на рівні транспорту (слайд 17). `PubSubClient` той самий, що й у Занятті 8.

### 4. MQTT Connect — `connectMQTT()`

```cpp
mqttClient.connect(THINGNAME);  // Client ID = THINGNAME (слайд 16)
```

Коди `mqttClient.state()` при невдачі:
- `-2` — помилка TLS/handshake → перевір **час** і **endpoint**
- `5` — відмовлено в доступі → перевір **AWS Policy**

### 5. Публікація — `publishData(temperature, humidity)`

Без змін із Заняття 8. `snprintf()` замість Arduino `String` — уникаємо фрагментації heap (Заняття 4). Топік: `iot-course/demo/telemetry`.

```json
{"temperature":24.5,"humidity":55.0}
```

### 6. Неблокуючий таймер і reconnect

`mqttClient.loop()` викликається лише коли підключено — без нього брокер не отримує PING і відключає клієнта. Reconnect — раз на 5 секунд без `delay()`.

---

## Вивід у Serial

```
ESP32-A (AWS IoT Core edition) старт
[Wi-Fi] Підключаємось.... OK
[Wi-Fi] IP: 10.13.37.2
[NTP] Синхронізація часу.. OK
[MQTT] Підключаємось до AWS IoT Core... OK
[MQTT] Публікуємо: {"temperature":27.0,"humidity":55.0}
[MQTT] OK
```

При помилці:

```
[MQTT] Підключаємось до AWS IoT Core... помилка: -2
```

---

## Моніторинг через AWS IoT Console

1. AWS IoT Console → **MQTT test client**.
2. **Subscribe to a topic** → `iot-course/demo/telemetry` (або `iot-course/demo/#`).
3. Запустити симуляцію в Wokwi — повідомлення з'являться у списку.

---

## Як запустити

1. Скопіювати `secrets.example.h` → `secrets.h` і заповнити (див. вище).
2. Відкрити проєкт у VS Code з розширенням **PlatformIO**.
3. Для симуляції — розширення **Wokwi for VS Code**, `F1 → Wokwi: Start Simulator`.
4. Відкрити **Serial Monitor** (швидкість `115200`).
5. Кожні 10 с у Serial — рядок `[MQTT] Публікуємо: ...` з підтвердженням.

---

## Рекомендована література

| Ресурс | Посилання |
|---|---|
| AWS IoT Core — Developer Guide | [docs.aws.amazon.com/iot](https://docs.aws.amazon.com/iot/latest/developerguide/what-is-aws-iot.html) |
| AWS IoT — device certificates (mTLS) | [docs.aws.amazon.com/iot/…/x509-client-certs](https://docs.aws.amazon.com/iot/latest/developerguide/x509-client-certs.html) |
| PubSubClient — офіційна документація | [pubsubclient.knolleary.net](https://pubsubclient.knolleary.net) |
| WiFiClientSecure (ESP32 Arduino core) | [github.com/espressif/arduino-esp32/…/WiFiClientSecure](https://github.com/espressif/arduino-esp32/tree/master/libraries/WiFiClientSecure) |
| Wokwi — ESP32 Wi-Fi у симуляторі | [docs.wokwi.com/guides/esp32-wifi](https://docs.wokwi.com/guides/esp32-wifi) |
| Random Nerd Tutorials — ESP32 + AWS IoT | [randomnerdtutorials.com/esp32-aws-iot-core](https://randomnerdtutorials.com/esp32-aws-iot-core-mqtt-arduino/) |

### Додаткові матеріали

-  **AWS — Using Device Time to Validate AWS IoT Server Certificates** — [aws.amazon.com/blogs/iot/using-device-time-to-validate-aws-iot-server-certificates](https://aws.amazon.com/blogs/iot/using-device-time-to-validate-aws-iot-server-certificates/)
-  **AWS — Server authentication** (офіційна документація) — [docs.aws.amazon.com/iot/latest/developerguide/server-authentication.html](https://docs.aws.amazon.com/iot/latest/developerguide/server-authentication.html)
-  **AWS — Security best practices in AWS IoT Core** — [docs.aws.amazon.com/iot/latest/developerguide/security-best-practices.html](https://docs.aws.amazon.com/iot/latest/developerguide/security-best-practices.html)
-  **PubSubClient (knolleary)** — та сама бібліотека з Заняття 8 — [github.com/knolleary/pubsubclient](https://github.com/knolleary/pubsubclient)
-  **WiFiClientSecure** — довідник ESP32 Arduino Core — [docs.espressif.com](https://docs.espressif.com) (пошук: WiFiClientSecure ESP32)
