#include "led.h"

void led_begin() {
    pinMode(LED, OUTPUT);
    digitalWrite(LED, LOW);
}

void led_handle_command(const char* cmd) {
    Serial.print("[CMD] Отримано: ");
    Serial.println(cmd);

    // Шукаємо саме значення в лапках, а не пару "ключ":"значення" —
    // тоді пробіли після двокрапки не мають значення.
    // "on" не збігається всередині "off": лапки роблять токени різними
    if (strstr(cmd, "\"on\"") != NULL) {
        digitalWrite(LED, HIGH);
        Serial.println("[CMD] LED увімкнено");

    } else if (strstr(cmd, "\"off\"") != NULL) {
        digitalWrite(LED, LOW);
        Serial.println("[CMD] LED вимкнено");

    } else {
        Serial.println("[CMD] Невідома команда — ігноруємо");
    }
}

bool led_status()
{
     return (digitalRead(LED) == HIGH);       
}