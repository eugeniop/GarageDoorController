#ifndef TRACE_LOGGER_H
#define TRACE_LOGGER_H

#include <Arduino.h>

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
        printHeader();
        printModule(module);
        Serial.print(message);
        Serial.print(": ");
        Serial.println(argument);
    }

    void log(const char* module, const char* message) {
        // Implement logging logic here, I'll move it to .cpp after making sure it works, easy fix
        printHeader();
        printModule(module);
        Serial.println(message);
    }

    void log(const char* module, const char* message, long argument) {
        // Implement logging logic here, I'll move it to .cpp after making sure it works, easy fix
        printHeader();
        printModule(module);
        Serial.print(message);
        Serial.print(": ");
        Serial.println(argument);
    }
};

//extern TraceLogger trace;
TraceLogger trace("DEBUG");
TraceLogger error("ERROR");

#endif 
