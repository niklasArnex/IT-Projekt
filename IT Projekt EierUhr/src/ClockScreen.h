// ClockScreen.h
#ifndef CLOCK_SCREEN_H
#define CLOCK_SCREEN_H

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RTClib.h>

class ClockScreen {
public:
    ClockScreen(Adafruit_SSD1306& display, RTC_DS3231& rtc);
    void show();

private:
    Adafruit_SSD1306& display;
    RTC_DS3231& rtc;
    char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
};

#endif // CLOCK_SCREEN_H