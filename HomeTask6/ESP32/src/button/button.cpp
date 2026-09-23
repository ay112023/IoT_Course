#include "button.h"
#include "../tools/tools.h"

// Порожньо. Реалізація кнопки — тут.
unsigned long lastDebounce    = 0;
bool          lastButtonState = HIGH;
bool          buttonState     = HIGH;


void button_begin()
{
   pinMode(BUTTON_PIN, INPUT_PULLUP);
} 
// Обробка кнопки 
void button_pressed()
{
     // Якщо треба - пишемо обробку натискання кнопки тут.
}
//Debounce по таймеру
void handleButton()
{
  bool reading = digitalRead(BUTTON_PIN);

  if (reading != lastButtonState) {
    lastDebounce = millis();
  }

  if (millis() - lastDebounce > DEBOUNCE) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == LOW) {           
            button_pressed();
      }
    }
  }

  lastButtonState = reading;
} 
