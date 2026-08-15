
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

