# Лекція 3 — Структури та Serial

Демонстрація використання структур (`struct`) у C++ та виведення даних через Serial на ESP32 (Arduino Framework, PlatformIO + Wokwi).

---

## Структура проєкту

```
src/
└── main.cpp      — основний файл прошивки
diagram.json      — схема підключення для Wokwi-симулятора
wokwi.toml        — конфігурація Wokwi
platformio.ini    — конфігурація PlatformIO (платформа, плата, швидкість монітора)
```

---

## Опис main.cpp

### 1. Підключення бібліотеки Arduino

```cpp
#include <Arduino.h>
```

Підключає Arduino Framework — набір функцій для роботи з GPIO, Serial, таймерами тощо.

---

### 2. Визначення пінів (макроси)

```cpp
#define LED 2
#define EXT_LED 4
```

`#define` — директива препроцесора. Замінює ім'я константи на числове значення **до** компіляції.  
`LED 2` — вбудований світлодіод ESP32 (пін GPIO2).  
`EXT_LED 4` — зовнішній світлодіод (пін GPIO4).

---

### 3. Структура даних

```cpp
struct SensorData {
  float temperature;
};
```

`struct` — спосіб згрупувати пов'язані змінні в один тип.  
Тут `SensorData` містить одне поле — `temperature` (температура, тип `float`).

> Структури зручні, коли функція повинна повернути кілька значень або коли дані логічно пов'язані між собою.

---

### 4. Функція зчитування сенсора

```cpp
SensorData readSensor() {
  SensorData data;
  data.temperature = random(15, 30);
  return data;
}
```

Повертає об'єкт типу `SensorData` зі згенерованою псевдовипадковою температурою від 15 до 29 °C.  
`random(min, max)` — вбудована Arduino-функція, повертає ціле число в діапазоні `[min, max)`.

---

### 5. Функція `setup()`

```cpp
void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  pinMode(EXT_LED, OUTPUT);
  delay(500);
}
```

Виконується один раз при старті мікроконтролера.

| Виклик | Призначення |
|---|---|
| `Serial.begin(115200)` | Ініціалізує UART-з'єднання зі швидкістю 115200 бод |
| `pinMode(пін, OUTPUT)` | Налаштовує пін як вихід |
| `delay(500)` | Пауза 500 мс для стабілізації після старту |

---

### 6. Функція `loop()`

```cpp
void loop() {
  SensorData reading;
  reading = readSensor();
  Serial.printf("Temp: %.1f C\n", reading.temperature);
  Serial.println();
  digitalWrite(LED, HIGH);
  digitalWrite(EXT_LED, HIGH);
  delay(100);
  digitalWrite(LED, LOW);
  digitalWrite(EXT_LED, LOW);
  delay(100);
}
```

Виконується нескінченно після `setup()`.

1. Зчитує дані з сенсора.
2. Виводить температуру в Serial Monitor.
3. Вмикає обидва світлодіоди на 100 мс, потім вимикає на 100 мс (блимання).

---

## Serial: функції виведення

Об'єкт `Serial` забезпечує комунікацію між мікроконтролером і комп'ютером через UART/USB.  
Перед використанням необхідно викликати `Serial.begin(baudRate)` у `setup()`.

---

### `Serial.print(value)`

Виводить значення **без** переходу на новий рядок.

```cpp
Serial.print("Температура: ");
Serial.print(25.3);
// Виведе: Температура: 25.3
```

Підтримує: рядки (`String`, `char*`), числа (`int`, `float`), символи (`char`).

---

### `Serial.println(value)`

Виводить значення **з** переходом на новий рядок (`\r\n`).

```cpp
Serial.println("Привіт!");
Serial.println(42);
// Виведе:
// Привіт!
// 42
```

Можна викликати без аргументів — тоді виводить лише порожній рядок (перехід на новий рядок).

```cpp
Serial.println(); // лише \r\n
```

---

### `Serial.printf(format, ...)`

Виводить відформатований рядок у стилі C-функції `printf`. Підтримує специфікатори формату.

```cpp
float temp = 23.7;
int humidity = 65;
Serial.printf("Температура: %.1f C, Вологість: %d%%\n", temp, humidity);
// Виведе: Температура: 23.7 C, Вологість: 65%
```

Найчастіші специфікатори:

| Специфікатор | Тип | Приклад |
|---|---|---|
| `%d` | ціле число (`int`) | `42` |
| `%f` | число з плаваючою комою | `3.140000` |
| `%.2f` | float, 2 знаки після коми | `3.14` |
| `%s` | рядок (`char*`) | `hello` |
| `%c` | символ (`char`) | `A` |
| `%%` | знак `%` | `%` |

> `Serial.printf` — найзручніша функція для дебагу: дозволяє виводити кілька змінних в одному рядку з форматуванням.

---

## Порівняння функцій Serial

| Функція | Новий рядок | Форматування | Типовий сценарій |
|---|---|---|---|
| `Serial.print()` | Ні | Ні | Частини рядка по черзі |
| `Serial.println()` | Так | Ні | Прості значення з переносом |
| `Serial.printf()` | За потреби (`\n`) | Так | Складний вивід із кількома змінними |

---

## Як запустити

1. Відкрити проєкт у VS Code з розширенням **PlatformIO**.
2. Для симуляції — встановити розширення **Wokwi for VS Code** і натиснути `F1 → Wokwi: Start Simulator`.
3. Відкрити **Serial Monitor** (швидкість: `115200`) — там з'явиться вивід температури.

---

## Рекомендована література

### Книги

- **"Programming Arduino: Getting Started with Sketches"** — Simon Monk  
  Базова книга по Arduino Framework і C++ для мікроконтролерів. Доступна на Amazon.

- **"Effective C"** — Robert C. Seacord  
  C і C++ для embedded-розробки. Більш просунутий рівень.

- **"Making Embedded Systems"** — Elecia White  
  Найкраща книга про embedded-розробку загалом. Рекомендована як основна literatura курсу.

### Онлайн ресурси

| Ресурс | Посилання |
|---|---|
| C++ Reference — повний довідник по мові | [cppreference.com](https://cppreference.com) |
| Arduino Reference — всі функції фреймворку | [arduino.cc/reference/en](https://arduino.cc/reference/en) |
| Random Nerd Tutorials — практичні туторіали по ESP32 | [randomnerdtutorials.com](https://randomnerdtutorials.com) |
| ESP32 офіційна документація | [docs.espressif.com](https://docs.espressif.com/projects/esp-idf/en/latest) |

### Відео

- **ESP32 для початківців** — канал [DroneBot Workshop](https://www.youtube.com/@dronebotworkshop) на YouTube
- **C++ for Embedded** — канал [Jacob Sorber](https://www.youtube.com/@JacobSorber) на YouTube — дуже рекомендується
