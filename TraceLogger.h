#ifndef TRACE_LOGGER_H
#define TRACE_LOGGER_H

#include <Arduino.h>

class TraceLogger {
public:
    void log(const char* module, const char* message, const char* argument) {
        // Implement logging logic here, I'll move it to .cpp after making sure it works, easy fix
        Serial.print("[");
        Serial.print(module);
        Serial.print("] ");
        Serial.print(message);
        Serial.print(": ");
        Serial.println(argument);
    }

    void log(const char* module, const char* message) {
        // Implement logging logic here, I'll move it to .cpp after making sure it works, easy fix
        Serial.print("[");
        Serial.print(module);
        Serial.print("] ");
        Serial.println(message);
    }

    void log(const char* module, const char* message, long argument) {
        // Implement logging logic here, I'll move it to .cpp after making sure it works, easy fix
        Serial.print("[");
        Serial.print(module);
        Serial.print("] ");
        Serial.print(message);
        Serial.print(": ");
        Serial.println(argument);
    }
};

extern TraceLogger trace;

#endif 
