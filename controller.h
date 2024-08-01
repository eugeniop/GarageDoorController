#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "httprequest.h"
#include "TraceLogger.h"
#include "relay.h"
#include "Door.h"

//Result of commands and actions
typedef enum { NO_ACTION, INVALID_RESPONSE, INVALID_REQUEST, REQUEST_FAILED, ACTIVATED_OPEN, ACTIVATED_CLOSE, IN_PROGRESS, COMPLETED } COMMAND_RESULT;

class Controller {
  
  Relay relay;
  Door door;

  // Once the relay is activated, we wait for a while for the the door to move
  // and deactivate either sensors (OPEN or CLOSE).
  // Total wait: tries * 1000ms
  int waitForActivation(){
    int tries = 10;
    while(tries--){
      trace.log("CTRL", "Waiting for door to move");
      auto state = door.getState();
      if(state == Door::MOVING) return COMPLETED;
      delay(1000);
    }
    error.log("CTRL", "waitForInProgress", "Door did not activate in time.");
    return REQUEST_FAILED;
  };

  /*
    Sends status to backemnd
  */
  int sendDoorStatus(){

    const char * status = door.getStateStr();
 
    HTTPRequest req(SERVER_URL, 443);
   
    sprintf(req.dataBuffer(), "{\"status\":\"%s\"}", status);
    auto res = req.putJSON(DOOR_PATH, API_KEY);
  
    if(!res){
      return INVALID_RESPONSE;
    }
  
    if(res->statusCode == 200){
      trace.log("CTRL", "sendDoorStatus. Door: ", status);
      return NO_ACTION;
    }
  
    error.log("CTRL", "sendDoorStatus. Error: ", res->statusCode);
    return REQUEST_FAILED;
  };

  static const char * serverStates[];
  enum ServerStates { CLOSED, OPENING_REQUEST, OPENING, OPEN, CLOSE_REQUEST, CLOSING };

  /*
   * Checks for any pending actions. The 2 states we care are: OPENING and CLOSING
   */
  int getServerDoorState(){ 

    HTTPRequest req(SERVER_URL, 443);
  
    auto res = req.get(DOOR_PATH, API_KEY);
    
    if(!res){
      error.log("CTRL", "getServerDoorStatus", "Invalid response");
      return INVALID_RESPONSE;
    }

    
    if(res->statusCode != 200){
      error.log("CTRL", "getServerDoorStatus. Request failed:", res->statusCode);
      return REQUEST_FAILED;
    }
    
    JsonDocument doc;
    deserializeJson(doc, res->data);
    Serial.println(res->data);
    
    const char* status = doc["status"];

    int x=0;
    while(serverStates[x]){
      if(!strcmp(status, serverStates[x])){
        return x;
      }
    }

    return -1;
  }

public:
  Controller() : relay(RELAY_PIN), 
                 door(SENSOR_OPEN_PIN, SENSOR_CLOSE_PIN)
                 {
  }

  void Init(){
    relay.Init();
    door.Init();
    //During initialization we check the door state and report it back to the backend
    //This is the "truth" 
    sendDoorStatus();
  }

  void Run(){
    trace.log("CTRL", "Door Status:", door.getStateStr());
    int ret = getServerDoorState();
    if(ret == OPENING_REQUEST || ret == CLOSE_REQUEST){
      relay.onFor(1000);    //Turn on motor
      waitForActivation();  //Wait for the door to move
    } else {
      sendDoorStatus();     //Continue sending status
    }
  };
};

const char * Controller::serverStates[] = { "closed", "opening_request", "opening", "open", "close_request", "closing", NULL };

#endif
