#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RTClib.h>
#include <RotaryEncoder.h>
#include "ClockScreen.h"
#include "Debug.h"
#include "EinschaltenScreen.h"
#include <driver/ledc.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

#define CLK_PIN 26
#define DT_PIN 27
#define SW_PIN 33
#define SUMMER_PIN 19

#define SCREEN_EINSCHALTEN 0
#define SCREEN_HOME 1
#define SCREEN_MENU 2
#define SCREEN_SELECTED_TIME 3
#define SCREEN_EINWILLIGUNG 4

int selectedHour1 = 0, selectedMinute1 = 0, selectedSecond1 = 0;
int selectedHour2 = 0, selectedMinute2 = 0, selectedSecond2 = 0;
unsigned long timerEndTime1 = 0, timerEndTime2 = 0;
bool isSelectedHour1 = false, isSelectedMinute1 = false, isSelectedSecond1 = false;
bool isSelectedHour2 = false, isSelectedMinute2 = false, isSelectedSecond2 = false;

RotaryEncoder encoder(CLK_PIN, DT_PIN);

#define MENU_ANZ 10

RTC_DS3231 rtc;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
ClockScreen clockScreen(display, rtc);
EinschaltenScreen einschaltenScreen(display);

const char* menu[MENU_ANZ] = {
  "Eier",
  "Pizza",
  "Ente",
  "Ofen",
  "Soße",
  "Kuchen",
  "Kaffee",
  "Schokolade",
  "Waffel",
  "Stoppuhr",
};

struct MenuEntry {
  int hour;
  int minute;
  int second;
};

MenuEntry menuTimes[MENU_ANZ];

int currentScreen = SCREEN_EINSCHALTEN;
int selectedMenuItem = 0;
bool lastButtonState = HIGH;
bool buttonState = HIGH;
bool isSecondTimerActive = false;

unsigned long lastButtonPress = 0;
unsigned long lastMenuPress = 0;
unsigned long lastSelectedTimePress = 0;
unsigned long einschaltenStartTime = 0;
unsigned long lastActivityTime = 0;

unsigned long lastPin33Press = 0;  
unsigned long lastEinwilligungPress = 0;

bool someOtherConditionForTimer2 = false;

int lastEncoderPosition = 0;

int sensitivity = 1;

int selectedHour = 1;
int selectedMinute = 0;
int selectedSecond = 0;

int storedHour = 0;
int storedMinute = 0;
int storedSecond = 0;

bool isSelectedHour = true;
bool isSelectedMinute = false;
bool isSelectedSecond = false;

bool isTimerRunning = false;
unsigned long timerEndTime;
bool isSummerActive = false;

unsigned long selectedTime = 0;

unsigned long lastButtonPressTime = 0;
const unsigned long buttonDebounceTime = 4000;  // 2 Sekunden
unsigned long selectedTimeStored = 0;

volatile bool isButtonPressed = false;
unsigned long selectedMenuTime = 0;

void showClock();
void showSelectedTime();
void showEinwilligungScreen();
void startTimer();
void resetTimer();
void checkEncoderPosition();
void showMenu();
void checkPin33State();
void updateSelectedValue();
void handleMenuItemSelection(int menuItemIndex);
void updateSelectedTime();
void handleStateTransitions();
void printDebugInfo();

void setup() {
  Serial.begin(9600);

  // Initialisierung des LEDC
  ledcSetup(0, 1000, 10);  // Kanal 0, Frequenz 1000 Hz, 10-Bit-Auflösung
  ledcAttachPin(SUMMER_PIN, 0);

  pinMode(SW_PIN, INPUT_PULLUP);
  digitalWrite(SW_PIN, LOW);

  pinMode(CLK_PIN, INPUT_PULLUP);
  pinMode(DT_PIN, INPUT_PULLUP);
  digitalWrite(CLK_PIN, HIGH);
  digitalWrite(DT_PIN , HIGH);

  pinMode(SUMMER_PIN, OUTPUT);
  noTone(SUMMER_PIN);

  if (!rtc.begin()) {
    DEBUG_PRINTLN("Couldn't find RTC");
    while (1);
  }

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    DEBUG_PRINTLN(F("SSD1306 allocation failed"));
    for (;;);
  }

  currentScreen = SCREEN_EINSCHALTEN;
  einschaltenScreen.startScreen();

  currentScreen = SCREEN_HOME;
  showClock();

  for (int i = 0; i < MENU_ANZ; i++) {
    menuTimes[i].hour = 0;
    menuTimes[i].minute = 0;
    menuTimes[i].second = 0;
  }
}

// Diese Variable speichert den Zeitpunkt der letzten Tasterbetätigung
unsigned long lastButtonDebounceTime = 0;

// Diese Funktion überprüft und entprellt den Taster
bool debounceButton() {
  static bool lastButtonState = HIGH;  // Static verwenden, um den Zustand zwischen den Aufrufen zu speichern
  static unsigned long lastButtonDebounceTime = 0;

  // Lesen Sie den aktuellen Zustand des Tasters
  bool buttonState = digitalRead(SW_PIN);

  // Überprüfen Sie, ob der Tasterzustand seit dem letzten Mal geändert wurde
  if (buttonState != lastButtonState) {
    // Aktualisieren Sie den Zeitpunkt der letzten Tasterbetätigung
    lastButtonDebounceTime = millis();
  }

  // Überprüfen Sie, ob genügend Zeit seit der letzten Tasterbetätigung vergangen ist
  if (millis() - lastButtonDebounceTime > 1000) {
    // Wenn ja, aktualisieren Sie den letzten Tasterzustand und geben Sie true zurück
    lastButtonState = buttonState;
    return true;
  }

  // Andernfalls geben Sie false zurück, da der Taster noch entprellt wird
  return false;
}

void handleMenuItemSelection(int menuItemIndex) {
  menuTimes[menuItemIndex].hour = selectedHour;
  menuTimes[menuItemIndex].minute = selectedMinute;
  menuTimes[menuItemIndex].second = selectedSecond;

  selectedMenuTime = selectedTime;
}

void updateSelectedTime() {
  selectedTime = selectedHour * 3600000UL + selectedMinute * 60000UL + selectedSecond * 1000UL;

  // Aktualisiere die ausgewählte Zeit nur für den aktuellen Menüpunkt
  if (currentScreen == SCREEN_SELECTED_TIME) {
    menuTimes[selectedMenuItem].hour = selectedHour;
    menuTimes[selectedMenuItem].minute = selectedMinute;
    menuTimes[selectedMenuItem].second = selectedSecond;
    selectedTimeStored = selectedTime;
  }
}

void updateSelectedValue() {
  if (isSelectedHour) {
    if (selectedHour < 99) {
      selectedHour++;
    } else {
      selectedHour = 0;
    }
    showSelectedTime();
  } else if (isSelectedMinute) {
    if (selectedMinute < 59) {
      selectedMinute++;
    } else {
      selectedMinute = 0;
    }
    showSelectedTime();
  } else if (isSelectedSecond) {
    if (selectedSecond < 59) {
      selectedSecond++;
    } else {
      selectedSecond = 0;
    }
    showSelectedTime();
  }
}

void handleStateTransitions() {
  if (currentScreen == SCREEN_EINSCHALTEN) {
    // Initial state, move to SCREEN_HOME after 5 seconds
    if (millis() - einschaltenStartTime > 5000) {
      currentScreen = SCREEN_HOME;
      showClock();
    }
  } else if (currentScreen == SCREEN_HOME) {
    // Pressing the button in SCREEN_HOME transitions to SCREEN_MENU
    if (digitalRead(SW_PIN) == LOW) {
      currentScreen = SCREEN_MENU;
      lastActivityTime = millis(); // Reset the inactivity timer
      showMenu();
    }
  } else if (currentScreen == SCREEN_MENU) {
    // Pressing the button in SCREEN_MENU transitions to SCREEN_SELECTED_TIME
    if (isButtonPressed && millis() - lastButtonPress > 2000) {
      currentScreen = SCREEN_SELECTED_TIME;
      isSelectedHour = true;
      isSelectedMinute = false;
      isSelectedSecond = false;
      showSelectedTime();
      lastActivityTime = millis(); // Reset the inactivity timer
    }

    // Check for inactivity timeout to transition to SCREEN_HOME
    if (millis() - lastActivityTime > 180000) {
      currentScreen = SCREEN_HOME;
      showClock();
    }
   } else if (currentScreen == SCREEN_SELECTED_TIME) {
    // Handle time selection logic
    if (isButtonPressed && millis() - lastButtonPress > 2000) {
      if (isSelectedHour) {
        isSelectedHour = false;
        isSelectedMinute = true;
        showSelectedTime();
      } else if (isSelectedMinute) {
        isSelectedMinute = false;
        isSelectedSecond = true;
        showSelectedTime();
      } else if (isSelectedSecond) {
        // Hier wird die ausgewählte Zeit im Menü überschrieben
        handleMenuItemSelection(selectedMenuItem);

        currentScreen = SCREEN_EINWILLIGUNG;
        showEinwilligungScreen();
      }
      lastActivityTime = millis(); // Reset the inactivity timer
    }
  } else if (currentScreen == SCREEN_EINWILLIGUNG) {
    // Handle "Ja" or "Nein" selection logic
    if (isButtonPressed && millis() - lastButtonPress > 2000) {
      if (isSelectedHour || isSelectedMinute || isSelectedSecond) {
        startTimer();
      }
      currentScreen = SCREEN_MENU;
      showMenu();
    }
  }
}

void showClock() {
  display.clearDisplay();
  clockScreen.show();
  display.display();
}

void updateFixedTimeBasedOnMenu();
// Feste Zeiten für jeden Menüeintrag (in Millisekunden)
unsigned long fixedTimes[MENU_ANZ] = {
  30000,   // Beispiel: 30 Sekunden für "Eier"
  60000,   // Beispiel: 1 Minute für "Pizza"
  120000,  // Beispiel: 2 Minuten für "Ente"
  180000,  // Beispiel: 3 Minuten für "Ofen"
  240000,  // Beispiel: 4 Minuten für "Soße"
  300000,  // Beispiel: 5 Minuten für "Kuchen"
  360000,  // Beispiel: 6 Minuten für "Kaffee"
  420000,  // Beispiel: 7 Minuten für "Schokolade"
  480000,  // Beispiel: 8 Minuten für "Waffel"
  540000,  // Beispiel: 9 Minuten für "Stoppuhr"
};

void showMenu() {
  display.clearDisplay();
  display.setTextSize(1);

  int startIndex = selectedMenuItem > 5 ? selectedMenuItem - 5 : 0;

  for (int i = startIndex; i < startIndex + 6 && i < MENU_ANZ; i++) {
    if (i == selectedMenuItem && currentScreen == SCREEN_MENU) {
      display.setTextColor(BLACK, WHITE);
    } else {
      display.setTextColor(WHITE);
    }
    display.setCursor(0, (i - startIndex) * 10);
    display.print(menu[i]);

    // Anzeigen der zugehörigen Uhrzeit (rechts)
    display.setCursor(64, (i - startIndex) * 10);

    // Änderung hier: Wenn es eine vordefinierte Zeit gibt, zeige sie an, sonst die ausgewählte Zeit
    if (fixedTimes[i] > 0) {
      display.print(fixedTimes[i] / 3600000);
      display.print("h ");
      display.print((fixedTimes[i] % 3600000) / 60000);
      display.print("m ");
      display.print((fixedTimes[i] % 60000) / 1000);
      display.print("s");
    } else {
      display.print(menuTimes[i].hour);
      display.print("h ");
      display.print(menuTimes[i].minute);
      display.print("m ");
      display.print(menuTimes[i].second);
      display.print("s");
    }
  }

  display.display();
}

// Funktion zum Aktualisieren der festen Zeit basierend auf der ausgewählten Zeit und Menüindex
void updateFixedTime() {
  fixedTimes[selectedMenuItem] = selectedHour * 3600000UL + selectedMinute * 60000UL + selectedSecond * 1000UL;
  updateFixedTimeBasedOnMenu();  // Rufe die neue Funktion auf
}

// Funktion zum Aktualisieren der festen Zeit im Menü
void updateMenuFixedTime() {
  for (int i = 0; i < MENU_ANZ; i++) {
    if (i < MENU_ANZ - 1) {  // Korrekte Anzahl von Elementen überprüfen
      menuTimes[i].hour = fixedTimes[i] / 3600000;   // Stunden
      menuTimes[i].minute = (fixedTimes[i] % 3600000) / 60000;  // Minuten
      menuTimes[i].second = (fixedTimes[i] % 60000) / 1000;  // Sekunden
    }
  }
}

void updateFixedTimeBasedOnMenu() {
  if (selectedMenuItem < MENU_ANZ - 1) {  // Überprüfe, ob der Menüindex gültig ist
    fixedTimes[selectedMenuItem] = menuTimes[selectedMenuItem].hour * 3600000UL + 
                                   menuTimes[selectedMenuItem].minute * 60000UL + 
                                   menuTimes[selectedMenuItem].second * 1000UL;
  }
}

void showTimers() {
  display.clearDisplay();
  display.setTextSize(1);

  for (int i = 0; i < MENU_ANZ; i++) {
    display.setCursor(0, i * 10);
    display.print(menu[i]);

    if (menuTimes[i].hour > 0 || menuTimes[i].minute > 0 || menuTimes[i].second > 0) {
      display.print(menuTimes[i].hour);   // Stunden
      display.print(F("h "));
      display.print(menuTimes[i].minute); // Minuten
      display.print(F("m "));
      display.print(menuTimes[i].second); // Sekunden
      display.print(F("s"));
    }
  }

  display.setCursor(0, selectedMenuItem * 10);
  display.print("Selected: ");
  display.print(selectedTime / 3600000);
  display.print("h ");
  display.print((selectedTime % 3600000) / 60000);
  display.print("m ");
  display.print((selectedTime % 60000) / 1000);
  display.print("s ");

  display.display();
}

void showEinwilligungScreen() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.setCursor(22, 10);
  display.print("Willst du den");

  display.setCursor(22, 20);
  display.print("Timer starten?");

  display.setCursor(30, 40);

  // Check if "Ja" is selected for Timer 1
  if (isSelectedHour || isSelectedMinute || isSelectedSecond) {
    display.print("[Ja] - Nein");

  } else {
    display.print("Ja - [Nein]");

  display.display();
  }
}

void showSelectedTime() {
  if (!isTimerRunning) {
    noTone(SUMMER_PIN);
  }

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);

  display.setCursor(0, 0);
  display.print(isSelectedHour ? "[H]" : " H. ");

  display.setCursor(40, 0);
  display.print(isSelectedMinute ? "[M]" : " M. ");

  display.setCursor(80, 0);
  display.print(isSelectedSecond ? "[S]" : " S. ");

  display.setCursor(0, 30);
  display.print(selectedHour < 10 ? "0" : "");
  display.print(selectedHour);

  display.setCursor(45, 30);
  display.print(selectedMinute < 10 ? "0" : "");
  display.print(selectedMinute);

  display.setCursor(90, 30);
  display.print(selectedSecond < 10 ? "0" : "");
  display.print(selectedSecond);

  display.display();
}

void startTimer() {
  isTimerRunning = true;
  timerEndTime = millis() + selectedHour * 3600000UL + selectedMinute * 60000UL + selectedSecond * 1000UL;
}

void resetTimer() {
  isTimerRunning = false;
  selectedHour = storedHour;
  selectedMinute = storedMinute;
  selectedSecond = storedSecond;
}

void checkButtonPress() {
  buttonState = digitalRead(SW_PIN);

  if (buttonState != lastButtonState) {
    if (buttonState == LOW) {
      if (currentScreen == SCREEN_EINSCHALTEN) {
        if (millis() - einschaltenStartTime > 3000) {
          currentScreen = SCREEN_HOME;
          resetTimer();
          showClock();
        }
      } else if (currentScreen == SCREEN_HOME) {
        currentScreen = SCREEN_MENU;
        showMenu();
      } else if (currentScreen == SCREEN_MENU) {
        currentScreen = SCREEN_SELECTED_TIME;
        showSelectedTime();
      } else if (currentScreen == SCREEN_SELECTED_TIME) {
        storedHour = selectedHour;
        storedMinute = selectedMinute;
        storedSecond = selectedSecond;
        startTimer();
        showClock();
      } else if (currentScreen == SCREEN_EINWILLIGUNG) {
        if (isSelectedHour || isSelectedMinute || isSelectedSecond) {
          startTimer();
        } else {
          resetTimer();
        }
        currentScreen = SCREEN_MENU;
        showMenu();
      }
    }

    lastButtonState = buttonState;
  }
}

void checkMenuPress() {
  int encoderButtonState = digitalRead(SW_PIN);

  // Überprüfe, ob der Encoder-Button gedrückt ist und genügend Zeit seit dem letzten Drücken vergangen ist
  if (encoderButtonState == LOW && millis() - lastButtonPress > 2000) {
    lastButtonPress = millis(); // Aktualisiere den Zeitpunkt des letzten Encoder-Button-Drucks

    if (currentScreen == SCREEN_MENU) {
      currentScreen = SCREEN_SELECTED_TIME;
      showSelectedTime();
    } else if (currentScreen == SCREEN_SELECTED_TIME) {
      storedHour = selectedHour;
      storedMinute = selectedMinute;
      storedSecond = selectedSecond;
      startTimer();
      showClock();
    }
  }
}


void checkEncoderClick() {
  int encoderButtonState = digitalRead(SW_PIN);

  if (encoderButtonState == LOW) {
    if (currentScreen == SCREEN_MENU) {
      currentScreen = SCREEN_SELECTED_TIME;
      isSelectedHour = true;
      isSelectedMinute = false;
      isSelectedSecond = false;
      showSelectedTime();
      lastActivityTime = millis(); // Reset the inactivity timer
    } else if (currentScreen == SCREEN_SELECTED_TIME) {
      isSelectedHour = !isSelectedHour;
      isSelectedMinute = !isSelectedHour && isSelectedMinute;
      isSelectedSecond = !isSelectedHour && !isSelectedMinute;
      showSelectedTime();

      // Aktualisiere die Variable für den Encoder-Klick
      lastButtonPress = millis();
    } else if (currentScreen == SCREEN_EINWILLIGUNG) {
      if (millis() - lastEinwilligungPress > 1000) {
        lastEinwilligungPress = millis();
        // Hier können Sie weitere Aktionen hinzufügen, die beim Klick im Einwilligungs-Bildschirm durchgeführt werden sollen
      }
    }
  }
}

void checkEncoderPosition() {
  int encoderPosition = encoder.getPosition();

  if (currentScreen == SCREEN_MENU) {
    if (encoderPosition > lastEncoderPosition) {
      // Scrolling nach unten
      selectedMenuItem = (selectedMenuItem + 1) % MENU_ANZ;
      showMenu();
    } else if (encoderPosition < lastEncoderPosition) {
      // Scrolling nach oben
      selectedMenuItem = (selectedMenuItem - 1 + MENU_ANZ) % MENU_ANZ;
      showMenu();
    }

    lastEncoderPosition = encoderPosition;
  } else if (currentScreen == SCREEN_SELECTED_TIME) {
    if (encoderPosition > lastEncoderPosition) {
      // Rechtsdrehung: Inkrementiere die ausgewählte Zeit
      if (isSelectedHour) {
        if (selectedHour < 99) {
          selectedHour++;
        } else {
          selectedHour = 0;
        }
      } else if (isSelectedMinute) {
        if (selectedMinute < 59) {
          selectedMinute++;
        } else {
          selectedMinute = 0;
        }
      } else if (isSelectedSecond) {
        if (selectedSecond < 59) {
          selectedSecond++;
        } else {
          selectedSecond = 0;
        }
      }
    } else if (encoderPosition < lastEncoderPosition) {
      // Linksdrehung: Dekrementiere die ausgewählte Zeit
      if (isSelectedHour) {
        if (selectedHour > 0) {
          selectedHour--;
        } else {
          selectedHour = 99;
        }
      } else if (isSelectedMinute) {
        if (selectedMinute > 0) {
          selectedMinute--;
        } else {
          selectedMinute = 59;
        }
      } else if (isSelectedSecond) {
        if (selectedSecond > 0) {
          selectedSecond--;
        } else {
          selectedSecond = 59;
        }
      }
    }

    lastEncoderPosition = encoderPosition;
  } else if (currentScreen == SCREEN_EINWILLIGUNG) {
    if (encoderPosition > lastEncoderPosition) {
      // Rechtsdrehung: Auswahl zu "Nein"
      isSelectedHour = false;
      isSelectedMinute = false;
      isSelectedSecond = false;
      showEinwilligungScreen();
    } else if (encoderPosition < lastEncoderPosition) {
      // Linksdrehung: Auswahl zu "Ja"
      isSelectedHour = true;
      isSelectedMinute = true;
      isSelectedSecond = true;
      showEinwilligungScreen();
    }

    // Aktualisiere die ausgewählte Zeit-Anzeige auf dem Bildschirm
    showSelectedTime();

    lastEncoderPosition = encoderPosition;
  }
}

void checkPin33State() {
  // Überprüfe, ob der Pin 33 gedrückt ist und genügend Zeit seit dem letzten Drücken vergangen ist
  if (digitalRead(33) == LOW && millis() - lastPin33Press > 500) {
    lastPin33Press = millis(); // Aktualisiere den Zeitpunkt des letzten Pin-33-Drucks

    switch (currentScreen) {
      case SCREEN_HOME:
        // Wechsle zum Bildschirm SCREEN_MENU, wenn Pin 33 im Bildschirm SCREEN_HOME gedrückt wird
        currentScreen = SCREEN_MENU;
        lastActivityTime = millis();
        showMenu();
        checkEncoderPosition(); // Überwache Encoder-Position im Menü-Bildschirm
        break;


      case SCREEN_MENU:
        // Überprüfe, ob es der erste Druck nach dem Betreten von SCREEN_MENU ist
        if (millis() - lastPin33Press > 500) {
          lastPin33Press = millis(); // Aktualisiere den Zeitpunkt des letzten Pin-33-Drucks
          currentScreen = SCREEN_SELECTED_TIME; // Wechsle zu SCREEN_SELECTED_TIME
          isSelectedHour = true;
          isSelectedMinute = false;
          isSelectedSecond = false;
          showSelectedTime(); // Zeige die ausgewählte Zeit an
        }
        break;

      case SCREEN_SELECTED_TIME:
        // Behandle jegliche Logik, die beim Übergang von SCREEN_SELECTED_TIME zu einem anderen Bildschirm benötigt wird
        // Zum Beispiel könnte hier updateSelectedTime() aufgerufen werden
        updateSelectedTime();
        storedHour = selectedHour;
        storedMinute = selectedMinute;
        storedSecond = selectedSecond;
        currentScreen = SCREEN_EINWILLIGUNG; // Wechsle zu SCREEN_EINWILLIGUNG
        showEinwilligungScreen(); // Zeige den Einwilligungs-Bildschirm an
        break;

      case SCREEN_EINWILLIGUNG:
        // Überprüfe, ob Stunde, Minute oder Sekunde ausgewählt ist
        if (isSelectedHour || isSelectedMinute || isSelectedSecond) {
          startTimer(); // Starte den Timer
          menuTimes[selectedMenuItem].hour = selectedHour;    // Aktualisiere die Zeit im Menü
          menuTimes[selectedMenuItem].minute = selectedMinute;
          menuTimes[selectedMenuItem].second = selectedSecond;
        } else {
          resetTimer(); // Setze den Timer zurück
        }
        currentScreen = SCREEN_MENU; // Wechsle zu SCREEN_MENU
        showMenu(); // Zeige das Menü an
        isSelectedHour = true; // Setze die Auswahl auf Stunde
        isSelectedMinute = false;
        isSelectedSecond = false;
        isButtonPressed = false; // Setze den Flag zurück, um weitere Bestätigungen zu ermöglichen
        break;
    }
  }    
}







void loop() {
  encoder.tick();

  // Überprüfen, ob der Timer abgelaufen ist
  if (isTimerRunning && millis() >= timerEndTime) {
    resetTimer();
    showMenu();
  }

  // Überprüfen Sie den Tastendruck mit Entprellung
  if (debounceButton() || isButtonPressed) {
    // Setzen Sie das Flag zurück
    isButtonPressed = false;
    
    // Überprüfen Sie, ob genügend Zeit seit dem letzten Tastendruck vergangen ist
    if (millis() - lastButtonPressTime >= buttonDebounceTime) {
      // Logik für Zustandsübergänge
      handleStateTransitions();

      // Aktualisieren Sie die Zeit des letzten Tastendrucks
      lastButtonPressTime = millis();
    }
  }

  // Überwachen der Position des Drehgebers
  int encoderPosition = encoder.getPosition();
  checkEncoderPosition();

  // Aktualisieren der festen Zeiten basierend auf der ausgewählten Zeit
  updateFixedTime();
  updateMenuFixedTime();  // Aktualisieren Sie die fest eingestellten Zeiten im Menü

  // Überprüfen Sie den Status von Pin 33 unabhängig vom Bildschirmzustand
  checkPin33State();

  // Debug-Informationen (falls erforderlich)
  printDebugInfo();
}

void printDebugInfo() {
  // Debug information (if needed)
  static unsigned long lastDebugPrint = 0;
  static unsigned long lastPin25Print = 0;

  if (millis() - lastDebugPrint > 500) {
    lastDebugPrint = millis();

    DEBUG_PRINT("CLK_PIN state: ");
    DEBUG_PRINTLN(digitalRead(CLK_PIN));

    DEBUG_PRINT("DT_PIN state: ");
    DEBUG_PRINTLN(digitalRead(DT_PIN));
  }

  if (millis() - lastPin25Print > 500) {
    lastPin25Print = millis();

    if (digitalRead(33) == LOW) {
      DEBUG_PRINTLN("Pin 33 pressed: 1");
      // Wenn Pin 33 gedrückt wird, wechsle von SCREEN_HOME zu SCREEN_MENU
      currentScreen = SCREEN_MENU;
      lastActivityTime = millis();
      showMenu();
    } else {
      DEBUG_PRINTLN("Pin 33 not pressed: 0");
    }
  }
}