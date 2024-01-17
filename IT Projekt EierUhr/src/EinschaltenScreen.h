#ifndef EINSCHALTEN_SCREEN_H
#define EINSCHALTEN_SCREEN_H

#include <Arduino.h>

#define SCREEN_EINSCHALTEN_DURATION 2000

class EinschaltenScreen {
public:
  EinschaltenScreen(Adafruit_SSD1306 &display) : display(display) {}
  void startScreen();

private:
  Adafruit_SSD1306 &display;
  unsigned long startTime;
};

#endif