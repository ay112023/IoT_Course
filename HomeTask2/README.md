# Лекція 4 — Heap, бітові операції, відладка

Демонстрація роботи з пам'яттю (heap), бітовими прапорами та таймерами без `delay()` на ESP32 (Arduino Framework, PlatformIO + Wokwi).

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

### 1. Таймер без `delay()`

```cpp
unsigned long lastSensorRead = 0;
#define SENSOR_INTERVAL 20000  // 20 секунд
```

Зберігаємо час останнього зчитування в глобальній змінній.  
У `loop()` перевіряємо скільки часу минуло — без блокуючого `delay()`.

```cpp
unsigned long now = millis();
if (now - lastSensorRead >= SENSOR_INTERVAL) {
    lastSensorRead = now;
    // ... зчитати і вивести
}
```

`millis()` — кількість мілісекунд з моменту старту. Різниця `now - lastSensorRead` не переповнюється навіть після перевороту `unsigned long` (~49 діб).

---

### 2. Структура даних

```cpp
struct SensorData {
  float temperature;       // 4 байти
  uint8_t status;          // 1 байт — бітові прапори
  unsigned long timestamp; // 4 байти
};
```

Структура об'єднує три пов'язані поля в один тип.  
`status` — один байт, кожен біт якого означає окремий стан пристрою.

---

### 3. Бітові прапори

```cpp
uint8_t checkWifi()  { return 0b00000001; }  // біт 0 = Wi-Fi підключено
uint8_t checkSensor(){ return 0b00000010; }  // біт 1 = сенсор OK
```

```cpp
data.status = 0b00000000 | checkWifi();   // встановлює біт 0
data.status = data.status | checkSensor();// встановлює біт 1
// результат: 0b00000011 = 3
```

Оператор `|` (бітове АБО) встановлює потрібні біти не чіпаючи решту.  
Один байт може тримати 8 незалежних булевих прапорів одночасно.

---

### 4. Передача структури через вказівник

```cpp
void printData(SensorData* data) {
    Serial.println("Temp: " + String(data->temperature) + " C");
    Serial.println("Free heap: " + String(ESP.getFreeHeap()) + " bytes");
}
```

`SensorData*` — вказівник на структуру. Замість копіювання 9+ байт передається лише 4-байтна адреса.  
`data->field` — доступ до поля через вказівник (еквівалент `(*data).field`).  
`ESP.getFreeHeap()` — повертає кількість вільних байт у heap на поточний момент.

---

### 5. `static` локальна змінна

```cpp
void printData(SensorData* data) {
    static int callCount = 0;
    callCount++;
    ...
}
```

`static` всередині функції: змінна ініціалізується один раз і зберігає значення між викликами функції. Живе в сегменті `.bss` / `.data`, а не на стеку.

---

## Де живуть змінні (огляд пам'яті)

| Сегмент | Що тут зберігається | Час життя |
|---|---|---|
| **Flash (ROM)** | Код програми, рядкові літерали | Увесь час |
| **SRAM — .data** | Глобальні та статичні змінні з початковим значенням | Увесь час |
| **SRAM — .bss** | Глобальні та статичні змінні = 0 або без явного значення | Увесь час |
| **SRAM — Stack** | Локальні змінні, аргументи функцій, адреси повернення | Під час виклику функції |
| **SRAM — Heap** | Динамічна пам'ять (`new`, `malloc`) | Від `new` до `delete` |

На ESP32 (~320 KB SRAM) невірне використання heap (витоки, фрагментація) може призвести до збоїв. `ESP.getFreeHeap()` — перший інструмент діагностики.

---

## Як запустити

1. Відкрити проєкт у VS Code з розширенням **PlatformIO**.
2. Для симуляції — встановити розширення **Wokwi for VS Code** і натиснути `F1 → Wokwi: Start Simulator`.
3. Відкрити **Serial Monitor** (швидкість: `115200`) — кожні 20 секунд з'являтиметься новий рядок з даними.

---

## Домашнє завдання

### 1. Розширити структуру

Додати до `SensorData` поля `humidity` (вологість, тип `float`) та `datetime` (дата і час, наприклад рядок `char[20]` або просто `unsigned long` з `millis()`).

```cpp
struct SensorData {
  float temperature;
  float humidity;
  char datetime[20];    // або unsigned long timestamp
  uint8_t status;
};
```

---

### 2. Генерувати дані випадково

Замінити фіксовані значення на псевдовипадкові за допомогою `random()`:

- температура: **15–30 °C**
- вологість: **30–65 %**

```cpp
data.temperature = random(15, 31);   // [15, 30]
data.humidity    = random(30, 66);   // [30, 65]
```

---

### 3. Виводити дані раз на 20 секунд

Переконатися, що `SENSOR_INTERVAL 20000` встановлено, і вивід у `printData()` показує обидва нові поля:

```
--- Reading #3 ---
Temp:     24.0 C
Humidity: 52.0 %
Time:     60000 ms
Free heap: 271360 bytes
---
```

---

### 4. Перевірити стабільність heap

Додати окремий таймер з інтервалом 60 000 мс (1 хвилина) і виводити `ESP.getFreeHeap()` у Serial.  
Запустити симуляцію на **5 хвилин** та записати 5 значень.

**Очікуваний результат:** значення не змінюється (витоку пам'яті немає).  
Якщо значення поступово зменшується — у програмі є витік пам'яті.

```
[Heap check] Free heap: 271360 bytes
[Heap check] Free heap: 271360 bytes
[Heap check] Free heap: 271360 bytes
```

---

### 5. Пояснити результат переповнення

Що виведе наступний код і чому?

```cpp
uint8_t a = 200;
uint8_t b = 100;
uint8_t sum = a + b;
Serial.println(sum);
```

**Підказка:** `uint8_t` — беззнаковий 8-бітний тип, максимальне значення `255`.  
Сума `200 + 100 = 300`, але в 8 бітах поміщається лише `300 % 256 = 44`.  
Виведе: **44** (переповнення без помилки, результат «обертається» по модулю 256).

---

### 6. Описати де живуть змінні

Для кожної змінної з програми визначити сегмент пам'яті:

| Змінна | Сегмент | Пояснення |
|---|---|---|
| `unsigned long lastSensorRead = 0;` | `.bss` / `.data` | Глобальна, живе увесь час |
| `#define LED 2` | Flash | Макрос підставляється препроцесором, не займає RAM |
| `SensorData reading;` у `loop()` | Stack | Локальна змінна, створюється при виклику `loop()` |
| `static int callCount = 0;` | `.bss` / `.data` | `static` — живе увесь час, не на стеку |
| Рядкові літерали (`"Temp: "`) | Flash | Константи зберігаються в Flash |
| `new SensorData()` (якщо б використали) | Heap | Динамічна алокація через `new` |

---

## Рекомендована література

### Книги

- **"Programming Arduino: Getting Started with Sketches"** — Simon Monk  
  Базова книга по Arduino Framework і C++ для мікроконтролерів.

- **"Making Embedded Systems"** — Elecia White  
  Найкраща книга про embedded-розробку загалом. Рекомендована як основна literatura курсу.

- **"Effective C"** — Robert C. Seacord  
  C і C++ для embedded-розробки. Більш просунутий рівень.

### Онлайн ресурси

| Ресурс | Посилання |
|---|---|
| Пам'ять ESP32 — модель пам'яті від Espressif | [developer.espressif.com/blog/esp32-programmers-memory-model](https://developer.espressif.com/blog/esp32-programmers-memory-model) |
| ESP32 Memory Map детально | [scottyob.com/post/2025-02-27-esp32-memory](https://scottyob.com/post/2025-02-27-esp32-memory) |
| FreeRTOS — офіційна книга | [freertos.org/Documentation/RTOS_book.html](https://freertos.org/Documentation/RTOS_book.html) |
| C++ Reference — повний довідник по мові | [cppreference.com](https://cppreference.com) |
| Arduino Reference — всі функції фреймворку | [arduino.cc/reference/en](https://arduino.cc/reference/en) |
| Random Nerd Tutorials — практичні туторіали по ESP32 | [randomnerdtutorials.com](https://randomnerdtutorials.com) |

### Відео

- **ESP32 для початківців** — канал [DroneBot Workshop](https://www.youtube.com/@dronebotworkshop) на YouTube
- **C++ for Embedded** — канал [Jacob Sorber](https://www.youtube.com/@JacobSorber) на YouTube — дуже рекомендується
