// Debug.h
#ifndef DEBUG_H
#define DEBUG_H

#include <Arduino.h>

#define DEBUG_ENABLED  // Kommentieren Sie diese Zeile aus, um die Debugging-Ausgaben zu deaktivieren

#ifdef DEBUG_ENABLED
#define DEBUG_PRINTLN(message) Serial.println(message)
#define DEBUG_PRINT(message) Serial.print(message)
#else
#define DEBUG_PRINTLN(message)
#define DEBUG_PRINT(message)
#endif

#endif  // DEBUG_H
