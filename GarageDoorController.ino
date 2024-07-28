#include <ArduinoJson.h>

#include "Config.h" // Configuration file for WiFi, server, etc
#include "controller.h"

Controller controller;

//ToDo: initialize all components (WiFi, Serial, etc)
void setup() {

  //Enable for debugging
  Serial.begin(115200);
  while(!Serial);

  //Initialize WiFi
  WiFi_Init();
  controller.Init();
 
  trace.log("MAIN", "setup", "Controller started");
}

void loop(){
  trace.log("MAIN", "loop", "Checking for commands");
  controller.Run();
  delay(5000);
}
