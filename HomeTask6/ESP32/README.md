# Лекція 14 — Двостороння комунікація: команди з вебсторінки на ESP32

Прошивка ESP32, яка не тільки публікує телеметрію в **AWS IoT Core**, а й **слухає команди**: підписка на топік `iot-course/demo/commands/led`, керування світлодіодом із браузера через FastAPI. MQTT over TLS (порт 8883), неблокуючий таймер на `millis()`, автоматичний reconnect із відновленням підписки (Arduino Framework, PlatformIO + Wokwi).

Розвиток Заняття 11: канал «вгору» лишається без змін, додається канал «вниз» — `subscribe()`, callback і буфер команди. Код розкладено по модулях: `net/`, `mqtt/`, `led/` замість одного `main.cpp`.

---

## Архітектура

```
Браузер (HTML/) ──POST /command──► FastAPI ──publish──► AWS IoT Core
                                                             │
                                        topic: .../commands/led
                                                             ▼
                                                          ESP32  → LED
                                                             │
                                        topic: .../telemetry │
                                                             ▼
                                              Rules Engine → DynamoDB
```

---

## Структура проєкту

```
src/
├── main.cpp           — setup/loop, таймер публікації
├── net/               — Wi-Fi + NTP
│   ├── net.h
│   └── net.cpp
├── mqtt/              — TLS, connect/subscribe, publish, reconnect, callback
│   ├── mqtt.h
│   └── mqtt.cpp
├── led/               — led1 (D2), розбір команди
│   ├── led.h
│   └── led.cpp
├── dht/               — dht22 (D4)          ЗАГЛУШКА
├── button/            — btn1 (D5)           ЗАГЛУШКА
├── ldr/               — ldr1 (D14 / D34)    ЗАГЛУШКА
├── secrets.h          — Wi-Fi, endpoint і сертифікати (У .gitignore!)
└── secrets.example.h  — шаблон secrets.h для копіювання
diagram.json           — схема підключення для Wokwi-симулятора
wokwi.toml             — конфігурація Wokwi
platformio.ini         — конфігурація PlatformIO
```

PlatformIO збирає `src/` рекурсивно — окремих налаштувань для підпапок не треба. Заглушки містять тільки піни й коментар «сюди йде код по…»; компіляції не заважають.

---

## Налаштування secrets.h

`secrets.h` містить приватний ключ і **до git не потрапляє** (у `.gitignore`).

1. Скопіювати `src/secrets.example.h` → `src/secrets.h`.
2. Вписати `THINGNAME` (= ім'я Thing, = `device_id`) та `AWS_IOT_ENDPOINT`
   (AWS IoT Console → Settings → Device data endpoint).
3. Вставити вміст трьох `.pem` файлів, завантажених у Занятті 9:
   - `AmazonRootCA1.pem` → `AWS_CERT_CA` — перевірка сервера («це справді AWS?»)
   - `certificate.pem.crt` → `AWS_CERT_CRT` — паспорт пристрою («ось хто я»)
   - `private.pem.key` → `AWS_CERT_PRIVATE` — секретний доказ («паспорт справді мій»)

> **Client ID має дорівнювати `THINGNAME`** — AWS Policy обмежує Connect саме по ньому (слайд 16).

**Policy має дозволяти не тільки Publish.** Для команд потрібні ще `iot:Subscribe` і `iot:Receive` на `iot-course/demo/commands/led`. Без них `connect()` пройде, а підписка мовчки не вдасться.

---

## Залежності

```ini
lib_deps =
    knolleary/PubSubClient
    adafruit/DHT sensor library
```

`WiFi.h` і `WiFiClientSecure.h` входять до складу ESP32 Arduino core — додаткових бібліотек для TLS не потрібно.

---

## Опис модулів

### 1. `net/` — Wi-Fi та час

`net_wifi_connect()` — без змін із Заняття 8. Канал 6 (`WiFi.begin(..., 6)`) пропускає сканування, економить ~4 секунди в Wokwi. Повертає `false` при таймауті `WIFI_TIMEOUT`.

`net_time_sync()`:

```cpp
configTime(0, 0, "pool.ntp.org");  // зсув 0, DST 0 — для TLS достатньо
```

**Без цього TLS впаде**, навіть із правильними сертифікатами: ESP32 стартує з 1970 року, і handshake вважає сертифікат AWS «ще не дійсним». Час треба синхронізувати **до** `connect()`.

### 2. `mqtt/` — підключення, підписка, публікація

`mqtt_begin()` — порядок кроків критичний:

```cpp
net_wifi_connect();                     // 1. Wi-Fi
net_time_sync();                        // 2. час (до сертифікатів!)
net.setCACert(AWS_CERT_CA);             // 3. три файли зі слайда 10
net.setCertificate(AWS_CERT_CRT);
net.setPrivateKey(AWS_CERT_PRIVATE);
mqttClient.setServer(AWS_IOT_ENDPOINT, 8883);
mqttClient.setBufferSize(512);          // 256 замало — мовчки обрізає JSON
mqttClient.setCallback(onMessage);      // ДО connect() — щоб не проґавити
```

`WiFiClientSecure net` і `PubSubClient mqttClient` — `static` усередині `mqtt.cpp`. Назовні модуль віддає лише функції, TLS-клієнта ніхто інший не чіпає.

### 3. MQTT Connect + Subscribe — `mqtt_connect()`

```cpp
if (mqttClient.connect(THINGNAME)) {        // Client ID = THINGNAME (слайд 16)
    mqttClient.subscribe(TOPIC_CMD, 1);     // QoS 1 — команду повторити нікому
}
```

`subscribe()` стоїть **усередині** `if` — і це не косметика. Підписка живе в сесії брокера, не на ESP32. Реконект = нова сесія = нуль підписок. Тут вона відновлюється автоматично, бо є частиною підключення, а не окремою дією.

Телеметрію женемо QoS 0: наступний пакет перекриє втрачений.

Коди `mqttClient.state()` при невдачі:
- `-2` — помилка TLS/handshake → перевір **час** і **endpoint**
- `5` — відмовлено в доступі → перевір **AWS Policy**

### 4. Callback — `onMessage()`

```cpp
memcpy(commandBuf, payload, n);
commandBuf[n] = '\0';
commandReady = true;      // роботу зробить loop()
```

Правило: **скопіювати, підняти прапорець, вийти.** Жодної роботи в callback: буфер PubSubClient спільний на вхід і вихід, а поки ми всередині — не летить PING і брокер рахує нас мертвими.

Дві пастки:
- обрізаємо по `sizeof(commandBuf)`, а **не** `sizeof(payload)` — `payload` це вказівник, `sizeof` від нього дасть 4;
- `payload` приходить **без** нуль-термінатора, його ставимо самі.

`commandReady` — `volatile`: пише callback, читає `loop()`.

### 5. `led/` — виконання команди

Викликається з `loop()`, не з callback. Очікуваний payload:

```json
{"action":"set","value":"on"}
```

```cpp
if (strstr(cmd, "\"on\"") != NULL)  digitalWrite(LED, HIGH);
else if (strstr(cmd, "\"off\"") != NULL) digitalWrite(LED, LOW);
```

Шукаємо **значення в лапках**, а не пару `"ключ":"значення"` — тоді пробіли після двокрапки не мають значення. І `"on"` не збігається всередині `"off"`: лапки роблять токени різними.

### 6. Публікація — `mqtt_publish_telemetry()`

`snprintf()` замість Arduino `String` — уникаємо фрагментації heap (Заняття 4). `received_at` тут НЕМА — його дописує правило в хмарі.

```json
{"device_id":"esp32lecture10","timestamp":1784301652,"temperature":27.0,"humidity":55.0}
```

### 7. `loop()` — хто кого смикає

```cpp
if (!mqtt_connected()) { mqtt_reconnect_tick(); return; }

mqtt_poll();                                  // кожну ітерацію
const char* cmd = mqtt_take_command();        // NULL, якщо команди нема
if (cmd) led_handle_command(cmd);
```

`mqtt_poll()` **обовʼязково кожну ітерацію**: читає вхідні байти й тримає Keep Alive. Без нього команди не приходять узагалі.

`mqtt_reconnect_tick()` — раз на 5 секунд без `delay()`: спершу піднімає Wi-Fi (без мережі реконект MQTT безнадійний), потім `net.stop()` — інакше mbedTLS-контекст лишається «напівживим» і з часом зʼїдає купу — і аж тоді `mqtt_connect()`.

---

## Вивід у Serial

```
ESP32 — двостороння комунікація, старт
[Wi-Fi] Підключаємось.... OK
[Wi-Fi] IP: 10.13.37.2
[NTP] Синхронізація часу.. OK
[MQTT] Підключаємось до AWS IoT Core... OK
[MQTT] Підписані на commands/led
[MQTT] Публікуємо: {"device_id":"esp32lecture10","timestamp":1784301652,"temperature":27.0,"humidity":55.0}
[MQTT] OK
[CMD] Отримано: {"action":"set","value":"on"}
[CMD] LED увімкнено
```

При помилці:

```
[MQTT] Підключаємось до AWS IoT Core... помилка: -2
[MQTT] ПІДПИСКА НЕ ВДАЛАСЬ
```

---

## Перевірка через AWS IoT Console

1. AWS IoT Console → **MQTT test client**.
2. **Subscribe to a topic** → `iot-course/demo/telemetry` — телеметрія має падати раз на 10 с.
3. **Publish to a topic** → `iot-course/demo/commands/led`, payload:

```json
{"action":"set","value":"on"}
```

4. У Wokwi світлодіод має засвітитись, у Serial — `[CMD] LED увімкнено`.

Так перевіряється саме прошивка, без FastAPI. Не працює тут — далі йти немає сенсу.

---

## Як запустити

1. Скопіювати `secrets.example.h` → `secrets.h` і заповнити (див. вище).
2. Відкрити проєкт у VS Code з розширенням **PlatformIO**.
3. Для симуляції — розширення **Wokwi for VS Code**, `F1 → Wokwi: Start Simulator`.
4. Відкрити **Serial Monitor** (швидкість `115200`).
5. Кожні 10 с у Serial — рядок `[MQTT] Публікуємо: ...`; команда з консолі або з `HTML/index.html` вмикає LED.

---

## Troubleshooting

| Симптом | Куди дивитись |
|---|---|
| `mqttClient.state()` = -2 | TLS/handshake: перевірити час і endpoint |
| `mqttClient.state()` = 5 | Відмовлено: перевірити Policy та Client ID |
| Connect OK, `ПІДПИСКА НЕ ВДАЛАСЬ` | У Policy немає `iot:Subscribe` / `iot:Receive` на топік команд |
| Телеметрія йде, команди не приходять | Топік у публікації не збігається з `TOPIC_CMD` побуквенно |
| Команди приходили, після реконекту зникли | `subscribe()` винесли з `connectMQTT()` — підписка не відновилась |
| `[CMD] Невідома команда` | У payload немає `"on"` / `"off"` **у лапках** |
| Команда приходить обрізана | `setBufferSize(512)` або `commandBuf[64]` замалі |

---

## Ключові факти заняття

- Підписка живе **в сесії брокера**, не на пристрої. Реконект стирає її — тому `subscribe()` частина `connect()`
- У callback — тільки копіювання. Робота в callback ламає Keep Alive і топче спільний буфер
- `payload` приходить без `'\0'`, а `sizeof` від вказівника — це 4, а не розмір даних
- Команда — QoS 1 (повторити нікому), телеметрія — QoS 0 (наступний пакет перекриє)
- `mqttClient.loop()` кожну ітерацію, інакше вхідного каналу просто немає
- Модулі ділять код, а не логіку: `mqtt.cpp` тримає TLS-клієнт приватним і віддає назовні лише функції

---

## Рекомендована література

| Ресурс | Посилання |
|---|---|
| AWS IoT Core — Developer Guide | [docs.aws.amazon.com/iot](https://docs.aws.amazon.com/iot/latest/developerguide/what-is-aws-iot.html) |
| AWS IoT — publish/subscribe policy actions | [docs.aws.amazon.com/iot/…/iot-policy-actions](https://docs.aws.amazon.com/iot/latest/developerguide/iot-policy-actions.html) |
| AWS IoT — MQTT protocol support (QoS) | [docs.aws.amazon.com/iot/…/mqtt](https://docs.aws.amazon.com/iot/latest/developerguide/mqtt.html) |
| AWS IoT — device certificates (mTLS) | [docs.aws.amazon.com/iot/…/x509-client-certs](https://docs.aws.amazon.com/iot/latest/developerguide/x509-client-certs.html) |
| PubSubClient — офіційна документація | [pubsubclient.knolleary.net](https://pubsubclient.knolleary.net) |
| WiFiClientSecure (ESP32 Arduino core) | [github.com/espressif/arduino-esp32/…/WiFiClientSecure](https://github.com/espressif/arduino-esp32/tree/master/libraries/WiFiClientSecure) |
| PlatformIO — структура проєкту та `src_dir` | [docs.platformio.org/…/projectconf](https://docs.platformio.org/en/latest/projectconf/index.html) |
| Wokwi — ESP32 Wi-Fi у симуляторі | [docs.wokwi.com/guides/esp32-wifi](https://docs.wokwi.com/guides/esp32-wifi) |
| Random Nerd Tutorials — ESP32 + AWS IoT | [randomnerdtutorials.com/esp32-aws-iot-core](https://randomnerdtutorials.com/esp32-aws-iot-core-mqtt-arduino/) |

### Додаткові матеріали

- **AWS — Using Device Time to Validate AWS IoT Server Certificates** — [aws.amazon.com/blogs/iot/using-device-time-to-validate-aws-iot-server-certificates](https://aws.amazon.com/blogs/iot/using-device-time-to-validate-aws-iot-server-certificates/)
- **AWS — Security best practices in AWS IoT Core** — [docs.aws.amazon.com/iot/latest/developerguide/security-best-practices.html](https://docs.aws.amazon.com/iot/latest/developerguide/security-best-practices.html)
- **PubSubClient (knolleary)** — та сама бібліотека з Заняття 8 — [github.com/knolleary/pubsubclient](https://github.com/knolleary/pubsubclient)
- **WiFiClientSecure** — довідник ESP32 Arduino Core — [docs.espressif.com](https://docs.espressif.com) (пошук: WiFiClientSecure ESP32)
