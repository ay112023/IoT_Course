# Лекція 6 — Analog Read (LDR + DHT22)

Демонстрація аналогового зчитування (ADC), конвертації сирого значення в люкси, зчитування температури і вологості через DHT22, бітових прапорів статусу та трьох незалежних неблокуючих таймерів на ESP32 (Arduino Framework, PlatformIO + Wokwi).

---

## Структура проєкту

```
src/
└── main.cpp      — основний файл прошивки
diagram.json      — схема підключення для Wokwi-симулятора
wokwi.toml        — конфігурація Wokwi
platformio.ini    — конфігурація PlatformIO (платформа, плата, швидкість монітора, бібліотеки)
```

---

## Залежності (platformio.ini)

```ini
lib_deps =
    adafruit/DHT sensor library
```

Бібліотека `Adafruit DHT` автоматично завантажується PlatformIO при першій збірці. Підключається у коді через `#include "DHT.h"`.

---

## Схема підключення (diagram.json)

| Компонент | Підключення |
|---|---|
| **LED** (через резистор 220 Ω) | Анод → `GPIO2`, катод → `GND` |
| **Кнопка** (`wokwi-pushbutton`) | Контакт 1 → `GPIO5`, контакт 2 → `GND` |
| **LDR** (фоторезистор) + дільник напруги | LDR → `3.3 V`, середня точка → `GPIO34`, нижній резистор → `GND` |
| **DHT22** | Data → `GPIO4`, VCC → `3.3 V`, GND → `GND` |
| **Serial Monitor** | `TX0`/`RX0` плати ESP32 |

`GPIO34` — вхід-тільки (input-only) пін ESP32, тому ідеально підходить для аналогового зчитування. Кнопка використовує внутрішній `INPUT_PULLUP` на `GPIO5`.

---

## Опис main.cpp

### 1. Піни

```cpp
#define EXT_LED_PIN 2   // зовнішній LED на GPIO2
#define BUTTON_PIN  5   // кнопка на GPIO5
#define LDR_PIN     34  // LDR (фоторезистор) на аналоговому GPIO34
#define DHTT_PIN    4   // DHT22 на GPIO4
#define DHTT_TYPE   DHT22
```

### 2. Бітові прапори статусу

```cpp
#define STATUS_OK       0b00000000
#define STATUS_LDR_ERR  0b00000001  // біт 0: LDR помилка
#define STATUS_DHT_ERR  0b00000010  // біт 1: DHT22 помилка
#define STATUS_WIFI_ERR 0b00000100  // біт 2: Wi-Fi помилка
```

Один байт `statuscheck` тримає до 8 незалежних булевих прапорів. Оператор `|` встановлює потрібні біти, не змінюючи решту. Оператор `&` — перевіряє конкретний біт.

### 3. Три незалежні неблокуючі таймери

```cpp
#define LDR_INTERVAL                    20000   // мс — зчитування LDR кожні 20 с
#define DHTT_INTERVAL                   10000   // мс — зчитування DHT22 кожні 10 с
#define SENSOR_DATA_COLLECTION_INTERVAL 300000  // мс — збір і відправка кожні 5 хв
```

```cpp
unsigned long now = millis();

if ((now - lastSensorReadLDR) > LDR_INTERVAL)   { /* зчитати LDR */ }
if ((now - lastSensorReadDHTT) > DHTT_INTERVAL) { /* зчитати DHT22 */ }
if ((now - lastSensorCollection) > SENSOR_DATA_COLLECTION_INTERVAL) { /* зібрати і вивести */ }
```

`millis()` не блокує `loop()` — усі три сенсори обробляються незалежно на власних інтервалах без `delay()`.

### 4. Структури даних

```cpp
struct DHTTData {
    float temperature;  // °C
    float humidity;     // %
};

struct LDRData {
    int   raw;  // сирий ADC 0–4095
    float lux;  // розраховані люкси
};

struct SensorData {
    DHTTData      dhtt;
    LDRData       ldr;
    unsigned long timestamp;   // мс з моменту старту
    uint8_t       statuscheck; // бітові прапори стану
};
```

### 5. Конвертація ADC → Lux

```cpp
#define GAMMA 0.7f
#define RL10  33.0f  // опір LDR при 10 lux (кОм)

float adcToLux(int adcValue) {
    float voltage    = adcValue / 4096.0f * 3.3f;
    float resistance = 2000.0f * voltage / (1.0f - voltage / 3.3f);
    float lux        = pow(RL10 * 1e3 * pow(10, GAMMA) / resistance, (1.0f / GAMMA));
    return lux;
}
```

ESP32 ADC — 12-бітний (0–4095). Формула розраховує опір LDR через дільник напруги, а потім застосовує логарифмічну модель фоторезистора (`GAMMA`, `RL10`) для отримання люксів.

### 6. Валідація сенсорів

```cpp
uint8_t validateSensors() {
    uint8_t status = STATUS_OK;

    if (ldrpayload.raw < 0 || ldrpayload.raw > 4095)
        status |= STATUS_LDR_ERR;

    if (isnan(dhttpayload.temperature) || isnan(dhttpayload.humidity) ||
        dhttpayload.temperature < -40  || dhttpayload.temperature > 80 ||
        dhttpayload.humidity    < 0    || dhttpayload.humidity    > 100)
        status |= STATUS_DHT_ERR;

    if (!isWifiConnected())
        status |= STATUS_WIFI_ERR;

    return status;
}
```

`isnan()` — перевірка на NaN, яку повертає DHT бібліотека при помилці зчитування (обрив, немає відповіді від датчика).

### 7. Вивід в Serial

```
------------------------------
Час роботи:   300000 мс
Температура:  23.5 C
Вологість:    60.2 %
Освітленість: 142.7 lux (ADC: 1820)
Статус:       0x00
------------------------------
```

При наявності помилок статус відображається як hex і виводяться текстові повідомлення для кожного встановленого біта.

---

## Як запустити

1. Відкрити проєкт у VS Code з розширенням **PlatformIO**.
2. Для симуляції — встановити розширення **Wokwi for VS Code** і натиснути `F1 → Wokwi: Start Simulator`.
3. Відкрити **Serial Monitor** (швидкість: `115200`).
4. У Serial кожні 10 с з'являються показання `[DHT]`, кожні 20 с — `[LDR]`, кожні 5 хв — повний пакет `[SENSOR]` з усіма полями та статусом.

