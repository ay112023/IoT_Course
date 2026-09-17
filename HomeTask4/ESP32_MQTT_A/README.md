# Publisher ESP32-A
  
   Демонстрація підключення до публічного MQTT брокера, публікація
   даних із сенсорів, що опитуються та команд у топіки

## Структура проєкту

```
src/
└── main.cpp      — основний файл прошивки
    mqtt.cpp        протокол MQTT
    sensors.cpp     работа із сенсорами
    wifi1.cpp       работа із WiFi  
diagram.json      — схема підключення для Wokwi-симулятора
wokwi.toml        — конфігурація Wokwi
platformio.ini    — конфігурація PlatformIO (платформа, плата, швидкість монітора)
```

---

## Залежності

```ini
lib_deps =
    knolleary/PubSubClient
```

`WiFi.h` входить до складу ESP32 Arduino core — додаткових бібліотек не потрібно.

---

## Конфігурація

```cpp
#define WIFI_SSID      "Wokwi-GUEST"                     // SSID мережі (Wokwi симулятор)
#define WIFI_PASSWORD  ""                                // пароль (порожній для Wokwi-GUEST)
#define WIFI_TIMEOUT   10000                             // таймаут підключення, мс

#define MQTT_BROKER    "broker.hivemq.com"                 // публічний MQTT брокер
#define MQTT_PORT      1883                                // plain TCP, без TLS
#define MQTT_CLIENT_ID "esp32-yakomovich-a"                // унікальний Client ID
#define TOPICS_ALL     "iot-course/yakymovich/#"           //Гілка із wildcard для доступу до усіх топіків
#define TOPIC_SENSORS  "iot-course/yakymovich/sensors"     // топік для публікації сенсорів
#define TOPIC_COMMANDS  "iot-course/yakymovich/commands"   // топік для публікації сенсорів
#define COMMAND_MANUAL_READ  "manual_read"                 // строкова константа для команди натискання на  
                                                           //  кнопку

#define PUBLISH_INTERVAL  10000                            // інтервал публікації, мс
#define RECONNECT_INTERVAL 5000                            // інтервал між спробами reconnect, мс
```

---

## Опис main.cpp

### 1 Контроль неблокуючих таймерів  `bool due(unsigned long& last, unsigned long interval)`
       Викликається для контролю інтервалів часу


### 2 Вивід часу`void printTimeStamp(unsigned long timestamp)`
       Виводе у Serial timestamp

### 3  Обробка натискання кнопки `void handleButton()`
        Обробка натискання кнопки для публікації команди
        використовує функцію void publishCommand(const char* command, size_t size)
        з файла mqtt.cpp

## 4  Читання та публікація даних з сенсорів `void publishSensors()`  
      


## 5 Неблокуючий reconnect  `void tryReconnect()`