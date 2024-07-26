#ifndef TRACE_LOGGER_H
#define TRACE_LOGGER_H

#include <Arduino.h>
#include "dumpHex.h"

class TraceLogger {

  const char * logger;

  void printHeader(){
    Serial.print(logger);
    Serial.print(" - ");   
  };

  void printModule(const char * module){
    Serial.print("[");
    Serial.print(module);
    Serial.print("] ");  
  };

public:

    TraceLogger(const char * logger): logger(logger){
    }

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

    void logHex(const char* module, const char* message, const void* data, size_t size) {
        Serial.print("[");
        Serial.print(module);
        Serial.print("] ");
        Serial.print(message);
        Serial.println(":");
        dumpHex(&Serial, data, size);
    }
};

//extern TraceLogger trace;
TraceLogger trace("DEBUG");
TraceLogger error("ERROR");

#endif // TRACE_LOGGER_H
