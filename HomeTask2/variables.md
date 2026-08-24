
   Розміщення змінних у пам'яті


   Символічні константи, подставляються препроцесором
   у місце виклику, не використоують RAM

#define LED_BLINK_INTERVAL 200
#define EXT_LED 4

#define SENSOR_INTERVAL 20000  // 20 секунд
#define TEMP_LOW 15
#define TEMP_HIGH 30
#define HUMIDITY_LOW 30
#define HUMIDITY_HIGH 65

#define MEMORY_ANALYSIS_INTERVAL 60000 

    Глобальні змінні, що ініціалізуються значеннями по замочуваню (0),
    розміщуються у секції .bss, живуть весь час, доступні усюди
  
unsigned long lastSensorRead = 0;
unsigned long lastMemoryAnalyzed = 0;
unsigned long lastBlinkTime = 0;

     Статична змінна, живе весь час, розміщується у секції .bss,  доступна усюди

static int callCount = 0;

     Локальні рядкові константи у методах, розміщуються у Flash пам'яті

 "Starting sensor reading and memory analysis..."
 "Reading # %d: Temp: %.1f C, Humidity: %.1f%%, Status: 0x%02X, Time: %lu ms"
 "--- Free heap: %lu bytes"    
     
     Локальні змінні, живуть тільки у методах, де вони створюються, розміщуються у стеку

SensorData data;
unsigned long now;
SensorData reading;
      
      Upd: Heap 

Heap не використовується явно, бо не викроистовуються оператор new та виклики malloс(),
також немає локальних, та глобальних змінних типу String та їх використання,
 що їх внутрішній буфер розміщується в динамічній пам'яті. 
Але heap у данному випадку використовується під внутрішні буфери бібліотек, 
зокрема фунція Serial.printf() розміщує там буфер для копіювання туди 
локальних рядкових констант із регулярними виразами для подстановки значень та 
форматованого виводу у порт.
Місце під буфер виділяється при першому виклику функції та зберігається, 
судячи із усього, глобально.Значення FreeHeapSize виводиться для
контролю можливих витіків пам'яті про що буде казати його зменшення із часом.
При роботі програми значення FreeHeapSize за 5 хвилин роботи не зменшилося:


Reading # 1: Temp: 25.0 C, Humidity: 57.0%, Status: 0x03, Timestamp: 20000 ms
Reading # 2: Temp: 16.0 C, Humidity: 59.0%, Status: 0x03, Timestamp: 40000 ms
Reading # 3: Temp: 17.0 C, Humidity: 60.0%, Status: 0x03, Timestamp: 60000 ms
--- Free heap: 350820 bytes, Up time:  1.000 min.
Reading # 4: Temp: 28.0 C, Humidity: 49.0%, Status: 0x03, Timestamp: 80000 ms
Reading # 5: Temp: 15.0 C, Humidity: 35.0%, Status: 0x03, Timestamp: 100000 ms
Reading # 6: Temp: 18.0 C, Humidity: 56.0%, Status: 0x03, Timestamp: 120000 ms
--- Free heap: 350820 bytes, Up time:  2.000 min.
Reading # 7: Temp: 29.0 C, Humidity: 32.0%, Status: 0x03, Timestamp: 140000 ms
Reading # 8: Temp: 27.0 C, Humidity: 48.0%, Status: 0x03, Timestamp: 160000 ms
Reading # 9: Temp: 29.0 C, Humidity: 51.0%, Status: 0x03, Timestamp: 180000 ms
--- Free heap: 350820 bytes, Up time:  3.000 min.
Reading # 10: Temp: 25.0 C, Humidity: 54.0%, Status: 0x03, Timestamp: 200000 ms
Reading # 11: Temp: 23.0 C, Humidity: 45.0%, Status: 0x03, Timestamp: 220000 ms
Reading # 12: Temp: 24.0 C, Humidity: 56.0%, Status: 0x03, Timestamp: 240000 ms
--- Free heap: 350820 bytes, Up time:  4.000 min.
Reading # 13: Temp: 26.0 C, Humidity: 34.0%, Status: 0x03, Timestamp: 260000 ms
Reading # 14: Temp: 28.0 C, Humidity: 54.0%, Status: 0x03, Timestamp: 280000 ms
Reading # 15: Temp: 24.0 C, Humidity: 33.0%, Status: 0x03, Timestamp: 300000 ms
--- Free heap: 350820 bytes, Up time:  5.000 min.
   
     Аналіз коду:

uint8_t a = 200;
uint8_t b = 100;
uint8_t sum = a + b;

Якщо скласти 2 беззнакових цілих длиною 8 біт та присвоїти третьому
беззнаковому цілому може статися переповнення розрядної сітки, що й відбувається
у даному випадку:

a = 0x64(100)
b = 0xC8(200)
a + b = 0x12C(300)

Длина змінної, що зберігахє суму теж становить 8 біт,
тому одиниця у числі 0x12C(300) губиться, бо для неї немає старшої тетради й
залишаються тільки дві молодші тетради 0x2C(44).
Тому для sum = a + b, що мають тип uint8_t та a = 100 й b = 200 фактично виповнюється:
sum = ((100 + 200) % 256) = 44.

