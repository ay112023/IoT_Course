# Заняття 2 — Знайомство з ESP32 та середовищем розробки

## Опис
Перший проєкт на ESP32 з використанням PlatformIO та Wokwi Simulator.
Програма блимає вбудованим світлодіодом (GPIO2) та зовнішнім світлодіодом (GPIO4) і виводить повідомлення в Serial Monitor.

## Інструменти
- [VS Code](https://code.visualstudio.com/)
- [PlatformIO IDE](https://platformio.org/) — розширення для VS Code
- [Wokwi Simulator](https://wokwi.com/) — розширення для VS Code

## Встановлення розширень у VS Code
1. Відкрити VS Code
2. Extensions (Ctrl+Shift+X)
3. Знайти та встановити **PlatformIO IDE**
4. Знайти та встановити **Wokwi Simulator**
5. Отримати безкоштовну ліцензію Wokwi: wokwi.com/license
6. Перезапустити VS Code → зачекати ~2-3 хв

## Створення проєкту
1. Іконка PlatformIO зліва (мурашка)
2. Quick Access → PIO Home → New Project
3. Board: ESP32 Dev Module
4. Framework: Arduino
5. Finish

## Структура проєкту

    my-project/
    ├── src/
    │   └── main.cpp          Основний код програми
    ├── include/              Заголовкові файли (.h)
    ├── lib/                  Локальні бібліотеки
    ├── test/                 Тести
    ├── diagram.json          Схема Wokwi — компоненти та з'єднання
    ├── wokwi.toml            Конфіг симулятора — шлях до прошивки
    └── platformio.ini        Конфіг проєкту — плата, платформа, швидкість порту

## platformio.ini

    [env:esp32dev]
    platform = espressif32     ; Платформа — чіпи від Espressif
    board = esp32dev           ; Конкретна плата — ESP32 DevKit V1
    framework = arduino        ; Фреймворк — використовуємо Arduino API
    monitor_speed = 115200     ; Швидкість Serial Monitor

## wokwi.toml

    [wokwi]
    version = 1
    firmware = '.pio/build/esp32dev/firmware.bin'
    elf = '.pio/build/esp32dev/firmware.elf'

    ; version   — версія конфігураційного файлу, завжди 1
    ; firmware  — скомпільована програма, генерується після Ctrl+Alt+B
    ; elf       — потрібен для дебагера

## diagram.json

    {
      "version": 1,
      "author": "Your Name",
      "editor": "wokwi",
      "parts": [
        {
          "type": "board-esp32-devkit-v1",  -- тип компонента
          "id": "esp",                      -- унікальний ID, використовується в connections
          "top": 0,                         -- позиція на схемі по вертикалі
          "left": 0,                        -- позиція на схемі по горизонталі
          "attrs": {}                       -- додаткові параметри
        },
        {
          "type": "wokwi-resistor",         -- резистор 220 Ом
          "id": "r1",
          "top": 80,
          "left": 200,
          "attrs": { "value": "220" }
        },
        {
          "type": "wokwi-led",              -- зовнішній світлодіод
          "id": "led1",
          "top": 140,
          "left": 208,
          "attrs": { "color": "red" }       -- колір: red, green, blue, yellow, white, orange, purple
        }
      ],
      "connections": [
        ["esp:TX0", "$serialMonitor:RX", "", []],   -- Serial TX → монітор
        ["esp:RX0", "$serialMonitor:TX", "", []],   -- Serial RX → монітор
        ["esp:D4",  "r1:1",             "green", []], -- GPIO4 → резистор
        ["r1:2",    "led1:A",           "green", []], -- резистор → анод LED
        ["led1:C",  "esp:GND.1",        "black", []]  -- катод LED → GND
      ],
      "dependencies": {}
    }

## Serial в ESP32

ESP32 має три UART порти:

| UART  | Arduino об'єкт | TX (за замовч.) | RX (за замовч.) | Примітка                                      |
|-------|----------------|-----------------|-----------------|-----------------------------------------------|
| UART0 | `Serial`       | GPIO 1          | GPIO 3          | Serial Monitor і завантаження коду            |
| UART1 | `Serial1`      | GPIO 10         | GPIO 9          | Зайнятий SPI Flash — потрібні кастомні піни   |
| UART2 | `Serial2`      | GPIO 17         | GPIO 16         | Вільний, можна перепризначити будь-які GPIO   |

`Serial.println()` завжди використовує UART0 → TX0 (GPIO1) / RX0 (GPIO3) у Wokwi

Кастомні піни (для Serial1/Serial2):

    HardwareSerial mySerial(1);
    mySerial.begin(9600, SERIAL_8N1, RX_GPIO, TX_GPIO);

Джерело: [Random Nerd Tutorials — ESP32 UART](https://randomnerdtutorials.com/esp32-uart-communication-serial-arduino/)

## Як запустити

1. Зібрати проєкт — Ctrl+Alt+B
2. Запустити симулятор — F1 → Wokwi: Start Simulator
3. Serial Monitor відкривається автоматично внизу симулятора

## Код

    #include <Arduino.h>

    #define LED 2      // вбудований світлодіод
    #define EXT_LED 4  // зовнішній світлодіод

    void setup() {
      Serial.begin(115200);
      pinMode(LED, OUTPUT);
      pinMode(EXT_LED, OUTPUT);
    }

    void loop() {
      Serial.println("Hello World");
      digitalWrite(LED, HIGH);
      digitalWrite(EXT_LED, HIGH);
      delay(100);
      digitalWrite(LED, LOW);
      digitalWrite(EXT_LED, LOW);
      delay(100);
    }

## Корисні матеріали

1. [PlatformIO](https://docs.platformio.org)
2. [Wokwi](https://wokwi.com/docs)
3. [Arduino Documentation](https://docs.arduino.cc/language-reference/)
4. [ESP32 документація](https://randomnerdtutorials.com/esp32-pinout-reference-gpios)
5. [UART / Serial](https://randomnerdtutorials.com/esp32-uart-communication-serial-arduino)

## Результат
- Serial Monitor виводить "Hello World"
- Вбудований світлодіод (GPIO2) та зовнішній (GPIO4) блимають синхронно
- Зовнішня схема: GPIO4 → резистор 220 Ом → анод LED → катод LED → GND