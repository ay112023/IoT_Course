// ═══════════════════════════════════════════════════════════
// КОНФІГУРАЦІЯ ПІНІВ
// ═══════════════════════════════════════════════════════════
#define EXT_LED_PIN 2
#define BUTTON_PIN  5
#define LDR_PIN     34
#define DHTT_PIN    4
#define DHTT_TYPE   DHT22
// ═══════════════════════════════════════════════════════════
// ПОРОГОВІ ЗНАЧЕННЯ
// ═══════════════════════════════════════════════════════════
#define HIGH_TEMPERATURE_THRESHOLD    80
#define LOW_TEMPERATURE_THRESHOLD    -40
#define HIGH_HUMIDITY_THRESHOLD      100
#define LOW_HUMIDITY_THRESHOLD         0
#define LOW_LDR_THRESHOLD            200
//═══════════════════════════════════════════════════════════
// РЕЖИИМИ РОБОТИ
// ═══════════════════════════════════════════════════════════
#define SILENT_MODE      0x10  
#define MONITORING_MODE  0x12
// ═══════════════════════════════════════════════════════════
// ТАЙМЕРИ
// ═══════════════════════════════════════════════════════════
#define LDR_INTERVAL                    5000   // мс
#define DHTT_INTERVAL                   5000   // мс
#define SENSORS_MON_INTERVAL            10000   // мс
#define HTTP_POST_INTERVAL              30000  // мс
// ═══════════════════════════════════════════════════════════
// СТАТУС ПРИСТРОЮ
// ═══════════════════════════════════════════════════════════
#define STATUS_OK       0b00000000
// ═══════════════════════════════════════════════════════════
// DEBOUNCE
// ═══════════════════════════════════════════════════════════
#define DEBOUNCE 50 // мс
// ═══════════════════════════════════════════════════════════
// КОНФІГУРАЦІЯ WI-FI
// ═══════════════════════════════════════════════════════════
#define WIFI_SSID     "Wokwi-GUEST"
#define WIFI_PASSWORD ""
#define WIFI_TIMEOUT  10000  // мс
// ═══════════════════════════════════════════════════════════
// КОНФІГУРАЦІЯ HTTP
// ═══════════════════════════════════════════════════════════
#define SERVER_URL "http://httpbun.com/post"
// ═══════════════════════════════════════════════════════════
// КОНФІГУРАЦІЯ MQTT
// ═══════════════════════════════════════════════════════════
#define MQTT_BROKER    "broker.hivemq.com"       // публічний брокер HiveMQ
#define MQTT_PORT      1883                       // plain TCP, без TLS
#define MQTT_CLIENT_ID "esp32-demo-a"             // унікальний — не як у ESP32-B!
#define TOPICS_ALL     "iot-course/yakymovich/#"  //Усі топіки
#define TOPIC_SENSORS  "iot-course/yakymovich/sensors"    // топік для публікації сенсорів
#define TOPIC_COMMANDS "iot-course/yakymovich/commands"  //       для публікації команд
#define RECONNECT_INTERVAL 5000  // мс
#define RECONNECT_ATTEMPTS 3
#define PUBLISH_INTERVAL 10000  // публікуємо раз на 10 секунд
#define MESSAGE_BUFFER_SIZE 128  // розмір буфера для публікації повідомлень
// ═══════════════════════════════════════════════════════════
// КОМАНДИ
// ═══════════════════════════════════════════════════════════
#define COMMAND_MANUAL_READ  "manual_read"  // команда для кнопки