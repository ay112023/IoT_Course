#include <PubSubClient.h>
#include "wifi1.h"
#include "config.h"

// ═══════════════════════════════════════════════════════════
// MQTT КЛІЄНТ
// ═══════════════════════════════════════════════════════════
// WiFiClient — TCP з'єднання
// PubSubClient — MQTT протокол поверх TCP
extern PubSubClient mqttClient;

bool connectMQTT();
bool connectMQTTandSubscribe();
void publishSensorsData(float temperature, float humidity, float lux);
void publishCommand(const char* command, size_t size = MESSAGE_BUFFER_SIZE);
