#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "EinschaltenScreen.h"

// Grafik mit 3 größeren runden Eiern
static const uint8_t PROGMEM eggSymbol[] = {
  B00000000, B01111110, B11111111, B11011011, B11011011, B11011011, B11111111, B01111110,
};

void EinschaltenScreen::startScreen() {
  startTime = millis();
  while (millis() - startTime < SCREEN_EINSCHALTEN_DURATION) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.print("Willkommen Nutzer*in.");
    display.setCursor(0, 10);
    display.print("The Boys Eieruhr-App");
    
    // Zeige die drei runden Ei-Symbole an
    for (int i = 0; i < 3; i++) {
      display.drawBitmap(20 + i * 40, 40, eggSymbol, 8, 8, WHITE);
    }
    
    display.display();
    // Kein delay(100) mehr, um nicht zu blockieren
  }
}
