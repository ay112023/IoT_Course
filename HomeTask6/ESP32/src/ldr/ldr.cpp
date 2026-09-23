#include "ldr.h"

// Порожньо. Реалізація фоторезистора — тут.
void ldr_begin(){
   pinMode(LDR_DO_PIN, INPUT); 
}

float adcToLux(int adcValue) {
    float voltage    = adcValue / 4096.0f * 3.3f;
    float resistance = 2000.0f * voltage / (1.0f - voltage / 3.3f);
    float lux        = pow(RL10 * 1e3 * pow(10, GAMMA) / resistance, (1.0f / GAMMA));
    return lux;
}

bool  ldr_read(LDRData* ldrpayload){
     if(ldrpayload == NULL) 
      return false;
   
       ldrpayload->raw    = analogRead(LDR_AO_PIN);
       ldrpayload->lux    = adcToLux(ldrpayload->raw);
    return true;   
}

void ldr_print(LDRData* ldrpayload)
{
    if(ldrpayload == NULL) {
        Serial.println("LDR data pointer is null.");
        return;
    }

    Serial.print("[LDR] ADC: ");  Serial.print(ldrpayload->raw);
    Serial.print("  Lux: ");      Serial.println(ldrpayload->lux, 1);
}