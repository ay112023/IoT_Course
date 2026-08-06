#include <Arduino.h>

#define LED 2
#define EXT_LED 4

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  pinMode(EXT_LED, OUTPUT);
}

void sendS(){
 for(int i = 0; i < 3; i++)
 { 
    digitalWrite(LED, HIGH);
    digitalWrite(EXT_LED, HIGH);
    Serial.print("."); 
    delay(200);
    digitalWrite(LED, LOW);
    digitalWrite(EXT_LED, LOW);
    delay(100);
  }
  delay(100);
}

void sendO(){
 for(int i = 0; i < 3; i++)
 { 
    digitalWrite(LED, HIGH);
    digitalWrite(EXT_LED, HIGH);
    Serial.print("-");
    delay(600);
    digitalWrite(LED, LOW);
    digitalWrite(EXT_LED, LOW);
    delay(100);
  }
  delay(100);
}

void sendDivider(){
  Serial.println(); 
  delay(500);  
}

void loop() {
  sendS();
  sendO();
  sendS();
  sendDivider();     
}