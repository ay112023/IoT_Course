#include <Arduino.h>

#define LED 2
#define EXT_LED 4

#define DASH_DELAY 600
#define DOT_DELAY  200
#define PAUSE_DELAY 100
#define DIVIDER_DELAY 500

void dash(){
    digitalWrite(LED, HIGH);
    digitalWrite(EXT_LED, HIGH);
    Serial.print("-");
    delay(DASH_DELAY);
    digitalWrite(LED, LOW);
    digitalWrite(EXT_LED, LOW);
    delay(PAUSE_DELAY);
}

void dot(){
    digitalWrite(LED, HIGH);
    digitalWrite(EXT_LED, HIGH);
    Serial.print(".");
    delay(DOT_DELAY);
    digitalWrite(LED, LOW);
    digitalWrite(EXT_LED, LOW);
    delay(PAUSE_DELAY);
}

void sendS(){
 for(int i = 0; i < 3; i++)
 { 
   dot();
 }
 delay(PAUSE_DELAY);
}

void sendO(){
 for(int i = 0; i < 3; i++)
 { 
   dash();
 }
 delay(PAUSE_DELAY);
}

void sendDivider(){
  Serial.println(); 
  delay(DIVIDER_DELAY);  
}

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  pinMode(EXT_LED, OUTPUT);
}

void loop() {
  sendS();
  sendO();
  sendS();
  sendDivider();     
}