#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "httprequest.h"
#include "TraceLogger.h"
#include "relay.h"
#include "Door.h"

//Result of commands and actions
typedef enum { NO_ACTION, INVALID_RESPONSE, INVALID_REQUEST, REQUEST_FAILED, ACTIVATED_OPEN, ACTIVATED_CLOSE, IN_PROGRESS, COMPLETED } COMMAND_RESULT;

//Main states
typedef enum { WAITING, ACTIVATING_OPEN, ACTIVATING_CLOSE } STATE;

//Activation type
typedef enum { OPEN, CLOSE } ACTIVATION_TYPE;

class Controller {
  
  STATE state;
  Relay relay;
  Door door;

  void waitForInProgress(){
    while(door.getState()!=Door::MOVING){
      trace.log("CTRL", "Waiting for door to move");
      delay(1000);
    }
  }

  int executeCommandOnDoor(const char * status){
    trace.log("CTRL", "Activating relay to open/close the door...");
  
    relay.onFor(1000);

    //We wait until the door is moving to report back to the backend.
    waitForInProgress();

    if(!strcmp(status, "opening")){  
      return ACTIVATED_OPEN;
    }
  
    if(!strcmp(status, "closing")){
      return ACTIVATED_CLOSE;
    }

    //Should not 
    return NO_ACTION;
  }

  /*
   * This function notifies the backend of the completion of a action
   */
  int sendCommandStatus(){

    Door::State doorState = door.getState();
    
    if(doorState == Door::INVALID){
      error.log("CTRL", "sendCommandStatus", "Door in imvalid state. Both sensors are activated");
      return INVALID_REQUEST;
    }
  
    HTTPRequest req;
   
    sprintf(req.dataBuffer(), "{\"status\":\"%s\",\"controllerId\":\"%s\"}", doorState == Door::OPEN ? "open_complete" : ( doorState == Door::CLOSED ? "close_complete" : "in_progress"), CTRL_ID);
    auto res = req.postJSON(SERVER_URL, DOOR_PATH, 443, API_KEY, NULL);
  
    if(!res){
      return INVALID_RESPONSE;
    }
  
    if(res->statusCode == 200){
      if(doorState != Door::MOVING){
        trace.log("CTRL", "sendCommandStatus. Complete"); 
        return COMPLETED;
      } else {
        //in-progress
        trace.log("CTRL", "sendCommandStatus. Moving"); 
        return IN_PROGRESS;
      }
    }
  
    error.log("CTRL", "sendCommandStatus", res->statusCode);
    return REQUEST_FAILED;
  }

  /*
   * Checks for any pending actions. The 2 states we care are: OPENING and CLOSING
   */
  int getCommand(){
    HTTPRequest req;
  
    char apiPath[100];
    sprintf(apiPath, STATUS_PATH, CTRL_ID);
    trace.log("CTRL", "getCommand. Using path:", apiPath);
    auto res = req.get(SERVER_URL, apiPath, 443, API_KEY, NULL);
    
    if(!res){
      error.log("CTRL", "getCommand", "Invalid response");
      return INVALID_RESPONSE;
    }
    
    if(res->statusCode != 200){
      error.log("CTRL", "getCommand. Request failed:", res->statusCode);
      return REQUEST_FAILED;
    }
    
    JsonDocument doc;
    deserializeJson(doc, res->data);
    Serial.println(res->data);
    
    const char* status = doc["status"];

    //If opening or closing -> start process
    if(!strcmp(status, "opening") || !strcmp(status, "closing")){
      return executeCommandOnDoor(status);
    }

    //Nothing
    return NO_ACTION;
  }

public:
  Controller() : relay(RELAY_PIN), door(SENSOR_OPEN_PIN, SENSOR_CLOSE_PIN) {
    state = WAITING;
  }

  void Init(){
    relay.Init();
    door.Init();
  }

  /*
   * The controller can be in 3 different states:
   *  1. It is WAITING for a command to execute (OPEN/CLOSE) 
   *  2. It is activating the OPENING 
   *  3. Activating the CLOSING
   * 
   * If it is WAITING, then it checks for new commands - if none is present, it keeps WAITING.
   * If a command is present, state changes to ACTIVATING (either OPENING or CLOSING), 
   * It executes the command on the door, and waits for COMPLETION.
  */
  void Run(){
    trace.log("CTRL", "Door Status:", door.getStateStr());
    int ret;
    switch(state){
      case WAITING:
      trace.log("Controller", "run", "Getting command");
      ret=getCommand();
      if(ret==ACTIVATED_OPEN){
        trace.log("CTRL", "run", "Activating opening door");
        state=ACTIVATING_OPEN;
      }
      if(ret==ACTIVATED_CLOSE){
        trace.log("CTRL", "run", "Activating closing door");
        state=ACTIVATING_CLOSE;
      }
      break;
    case ACTIVATING_OPEN:
      ret=sendCommandStatus();
      if(ret==COMPLETED){
        trace.log("CTRL", "run", "Completed open activation");
        state=WAITING;
      }
      break;
    case ACTIVATING_CLOSE:
      ret=sendCommandStatus();
      if(ret==COMPLETED){
        trace.log("CTRL", "run", "Completed close activation");
        state=WAITING;
      }
      break;
    default:
      //Should never happen
      break;
    }
  };
};

#endif
