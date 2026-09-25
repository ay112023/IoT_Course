#include <Arduino.h>
#include "mqtt/mqtt.h"
#include "led/led.h"
#include "dht/dht.h"
#include "ldr/ldr.h"
#include "mqtt/mqtt.h"
#include "net/net.h"
#include "button/button.h"
#include "tools/tools.h"

#define PUBLISH_INTERVAL 10000  // публікуємо раз на 10 секунд

unsigned long lastPublish = 0;
bool errorMessagePrinted = false;

void setup() {
    Serial.begin(115200);
    dht_begin();
    ldr_begin();
    button_begin();
    led_begin();

    delay(500);
    Serial.println("ESP32 — двостороння комунікація, старт");    
    if (mqtt_begin()) {
        mqtt_connect();   // перший виклик; далі — з loop()
    }
}

void loop() {
     
     if (!mqtt_connected()) {
        
        // Обробка помилок
       if(!net_wifi_connected() || !timeSynchronized)
          {
            if(!errorMessagePrinted)
            {  
                errorMessagePrinted = true;
                Serial.println("Перевірте мережу та перезавантажте пристрій.");
            } 
            return;
          }

        mqtt_reconnect_tick();
        return;
    }

    mqtt_poll();   // ОБОВʼЯЗКОВО кожну ітерацію

    // Обробка команд з фронтенду
    const char* cmd = mqtt_take_command();
    if (cmd) {
        led_handle_command(cmd);
        mqtt_publish_event(LED_EVENT, led_status());
    }

    // Неблокуюче опитування та публікация данних сенсорів

   if (due(lastPublish, PUBLISH_INTERVAL)) {  
        
        DHTTData dhttData;
        LDRData ldrData;
        bool readDHTT_OK, readLDR_OK;
        readDHTT_OK = dht_read(&dhttData, &dht);
        readLDR_OK = ldr_read(&ldrData);
         
        // Обробка помилок сенсорів
        if(readDHTT_OK && readLDR_OK)
        {
            uint8_t status = validateSensors(&dhttData, &ldrData);
            if(status != STATUS_OK )
            {
                 Serial.println("Sensors status is invalid!"); 
                 printSensorsStatus(status);
                 return;
            }

        } else {
            Serial.println("Error reading sensors!");
            return;
        }
                   
         mqtt_publish_telemetry(dhttData.temperature, dhttData.humidity, ldrData.lux);   
        
    }   
}
