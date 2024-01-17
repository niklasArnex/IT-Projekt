// ClockScreen.cpp
#include "ClockScreen.h"

ClockScreen::ClockScreen(Adafruit_SSD1306& display, RTC_DS3231& rtc) : display(display), rtc(rtc) {}

void ClockScreen::show() {
    DateTime now = rtc.now();

    display.clearDisplay();

    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(75, 0);
    display.println(now.second(), DEC);

    display.setTextSize(2);
    display.setCursor(25, 0);
    display.println(":");

    display.setTextSize(2);
    display.setCursor(65, 0);
    display.println(":");

    display.setTextSize(2);
    display.setCursor(40, 0);
    display.println(now.minute(), DEC);

    display.setTextSize(2);
    display.setCursor(0, 0);
    display.println(now.hour(), DEC);

    display.setTextSize(2);
    display.setCursor(0, 20);
    display.println(now.day(), DEC);

    display.setTextSize(2);
    display.setCursor(25, 20);
    display.println("-");

    display.setTextSize(2);
    display.setCursor(40, 20);
    display.println(now.month(), DEC);

    display.setTextSize(2);
    display.setCursor(55, 20);
    display.println("-");

    display.setTextSize(2);
    display.setCursor(70, 20);
    display.println(now.year(), DEC);

    display.setTextSize(2);
    display.setCursor(0, 40);
    display.print(daysOfTheWeek[now.dayOfTheWeek()]);

    display.display();
}