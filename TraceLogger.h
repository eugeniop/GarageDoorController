#ifndef TRACE_LOGGER_H
#define TRACE_LOGGER_H

#include <Arduino.h>
#include "dumpHex.h"

class TraceLogger {
public:
    void log(const char* module, const char* message, const char* argument) {
        Serial.print("[");
        Serial.print(module);
        Serial.print("] ");
        Serial.print(message);
        Serial.print(": ");
        Serial.println(argument);
    }

    void log(const char* module, const char* message) {
        Serial.print("[");
        Serial.print(module);
        Serial.print("] ");
        Serial.println(message);
    }

    void log(const char* module, const char* message, long argument) {
        Serial.print("[");
        Serial.print(module);
        Serial.print("] ");
        Serial.print(message);
        Serial.print(": ");
        Serial.println(argument);
    }

    void logHex(const char* module, const char* message, const void* data, size_t size) {
        Serial.print("[");
        Serial.print(module);
        Serial.print("] ");
        Serial.print(message);
        Serial.println(":");
        dumpHex(&Serial, data, size);
    }
};

extern TraceLogger trace;

#endif // TRACE_LOGGER_H
