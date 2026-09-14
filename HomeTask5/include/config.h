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

// ═══════════════════════════════════════════════════════════
// СТАТУС ПРИСТРОЮ
// ═══════════════════════════════════════════════════════════
#define STATUS_OK       0b00000000
// ═══════════════════════════════════════════════════════════
// DEBOUNCE
// ═══════════════════════════════════════════════════════════
#define DEBOUNCE 50 // мс
// Wi-Fi
#define WIFI_SSID     "Wokwi-GUEST"
#define WIFI_PASSWORD ""
#define WIFI_TIMEOUT  10000  // мс
// ═══════════════════════════════════════════════════════════
// КОНФІГУРАЦІЯ MQTT
// ═══════════════════════════════════════════════════════════
#define MQTT_PORT      8883                                 //  TLS
#define TOPIC_TELEMETRY "iot-course/yakymovich/telemetry"
#define TOPIC_COMMANDS  "iot-course/yakymovich/commands"
#define RECONNECT_INTERVAL 5000  // мс
#define RECONNECT_ATTEMPTS 3
#define PUBLISH_INTERVAL 30000  // публікуємо раз на 30 секунд
#define MESSAGE_BUFFER_SIZE 128  // розмір буфера для публікації повідомлень
// ═══════════════════════════════════════════════════════════
// КОНФІГУРАЦІЯ NTP
//  ═══════════════════════════════════════════════════════════
#define NTP_TIMEOUT 15000  // мс
// ═══════════════════════════════════════════════════════════
// КОМАНДИ
// ═══════════════════════════════════════════════════════════
#define COMMAND_MANUAL_READ  "manual_read"  // команда для кнопки
// ═══════════════════════════════════════════════════════════
// Число блимань LED при надходженні команд 
// ═══════════════════════════════════════════════════════════
#define LED_BLINKS_MANUAL_READ 3