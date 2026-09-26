# Домашнє завдання № 6, прошивка ESP32 для роботи у стеку:
        ESP32 - Frontend - Backend - AWS, 
		

Прошивка ESP32, яка не тільки публікує телеметрію в **AWS IoT Core** -
у топік `iot-course/yakymovich/telemetry`,
а й **слухає команди**: 
підписка на топік `iot-course/yakymovich/commands/led`
та дає зворотний зв'язок, публікучи статус led
у топік `iot-course/yakymovich/events`
MQTT over TLS (порт 8883), неблокуючий таймер на `millis()`, 
автоматичний reconnect із відновленням підписки (Arduino Framework, PlatformIO + Wokwi).

## Архітектура

   Повну архітектурну схему див. у ../README.md

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
├── dht/               - dht sensor
│    ├── dht.h
│	 └── dht.cpp│	
├── button/            — btn1
│    ├── button.h
│	 └── button.cpp
├── ldr/               — ldr1 sensor (D14 / D34) 
│    ├── ldr.h
│    └── ldr.cpp
├── tools/            - Допоміжні функції
│    ├── tools.h
│	 └── tools.cpp
├── secrets.h          — Wi-Fi, endpoint і сертифікати (У .gitignore!)
└── secrets.example.h  — шаблон secrets.h для копіювання
diagram.json           — схема підключення для Wokwi-симулятора
wokwi.toml             — конфігурація Wokwi
platformio.ini         — конфігурація PlatformIO
```

PlatformIO збирає `src/` рекурсивно — окремих налаштувань для підпапок не треба. 

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

   /net  - робота із мережею: WiFi + NTP 
   /mqtt - робота із MQTT:
             onMessage() callback, прийом повідомлень з топіку, що на нього
                               підписка (команди), первинна обробка буферу,
							   взведення флагу видання команди для основної обробки у 
                               loop()							   
             mqttBegin() перевірка конекту із WiFi, спроба синхронізувати час,
                         у випадку успіху - встановлення сертифікатів, ключів, сервера 
                         для з'єднання із AWS
             mqttConnect() підключення до AWS, підписка на топік команд
             mqtt_reconnect_tick() спроба реконнекту
             mqtt_take_command() перевірка наявності команди, повернення буфера, фкщо команда єднання
             mqtt_publish_telemetry() публікація телеметрії
             make_event_payload() збірка payload для події: якщо треба буде додати
                                  ще події окрім led-у цей функціонал можна додати у 
                                  цю функцію
             mqtt_publish_event() Публікаця події								  
    /led  - робота із led
    /dht  - робота із сенсором dht
    /button - робота із кнопкою (із DEBOUNCE) у цьому проекті кнопка не використовується
    /ldr    - робота із сенсором освітленості
    /tools  - допоміжні фуекції
               due() контроль неблокуючих таймерів
               validateSensors() перевірка статусу сенсорів, базуючись на
                                 екземплярах структур, що їх поля
                                 заповнюються під час опросу (читання) сенсорів								 
               printSensorsStatus() вивід статусу сенсорів у Serial 			   
			   
## Опис main
    
	      Якщо коннект із MQTT брокером відсутній перевіряється
	   конект із WiFi та чи синхронізовано (після 3-х спроб)
	   час із NTP сервером якщо ні - виводиться повідомлення про критичну помилку
	   що потребує перезавантаження пристрою.
	      Якщо критичних помилок нема виконується спроба реконнекту  `mqtt_reconnect_tick()`
	      Перевіряється наявність команд від стеку **фронтенд - AWS**. Якщо команда є - 
       вона виконується (вмикається чи вимикається led) після чого статус led публікується у
	   у топік `iot-course/yakymovich/events`
	       Контролюється таймер публікації та якщо він більше чи равний інтервалу `PUBLISH_INTERVAL`
		   читаються сенсори та якщо нема помилок - дані публікуються у топік `iot-course/yakymovich/telemetry`
	   