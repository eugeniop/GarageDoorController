#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "httprequest.h"
#include "TraceLogger.h"
#include "relay.h"
#include "Door.h"

//Result of commands and actions
typedef enum { NO_ACTION, INVALID_RESPONSE, INVALID_REQUEST, REQUEST_FAILED, COMPLETED } COMMAND_RESULT;

class Controller {

  enum State { IDLE, ACTIVATING };

  State state;
  
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
      return COMPLETED;
    }
  
    error.log("CTRL", "sendDoorStatus. Error: ", res->statusCode);
    return REQUEST_FAILED;
  };

  static const char * serverStates[];
  enum ServerStates { CLOSED, OPENING_REQUEST, OPENING, OPEN, CLOSING_REQUEST, CLOSING };

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

    trace.log("CTRL", "getServerDoorStatus. Status - ", status); 

    int x=0;
    while(serverStates[x]){
      if(!strcmp(status, serverStates[x])){
        return x;
      }
      x++;
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

    //If the door is MOVING, then we need to wait until that is complete before we get commands
    if(door.getState() == Door::MOVING){
      state = ACTIVATING;
    } else {
      state = IDLE;
    }
  }

  void Run(){
    trace.log("CTRL", "Run. Door Status", door.getStateStr());
    int ret;
    auto doorState = door.getState();
    
    switch(state){
      //If controller is in IDLE, we are waiting for either commands from the backend or the door moving on its own
      //if the door is MOVING (on its own) we switch to ACTIVATING (waiting for completion)
      //if the door is NOT MOVING, but there's a request to move from the backend, we activate the relay and go to ACTIVATING
      //if neither, we just report state
      case IDLE:
        if(doorState == Door::MOVING){
          trace.log("CTRL", "Run. Door is moving. Wait for completion");
          state = ACTIVATING;
          return;
        }

        ret = getServerDoorState();
  
        if(ret == OPENING_REQUEST && doorState == Door::OPEN || 
           ret == CLOSING_REQUEST && doorState == Door::CLOSED){
          state = IDLE;
          sendDoorStatus();
          return; 
        }

        if(ret == OPENING_REQUEST || ret == CLOSING_REQUEST ){
          trace.log("CTRL", "Run. Received open or close request", ret);
          relay.onFor(1000);    //Turn on motor
          if(waitForActivation() == COMPLETED){
            state = ACTIVATING;
            return;
          }
        } else {
          //The door is NOT moving on its own and there are no requests to open or close, synch status
          if( (doorState == Door::OPEN && ret == OPEN) || doorState == Door::CLOSED && ret == CLOSED){
            trace.log("CTRL", "Run. Backend and Door in synch");
            return; //Nothing to report
          }
          sendDoorStatus();
        }
        break;

      case ACTIVATING:
        // The door is MOVING (on its own or by a command). We report back to the backend 
        // the status. If the door is closed or open, we finished and we go back to IDLE.
        if( sendDoorStatus() == COMPLETED ){
          auto doorState = door.getState();
          if(doorState == Door::OPEN || doorState == Door::CLOSED){
            trace.log("CTRL", "Run. Operation completed", door.getStateStr());
            state = IDLE;
          }
        }
        break;
    }
  };
};

//These strings need to be in THE SAME order as the enum:
//enum ServerStates { CLOSED, OPENING_REQUEST, OPENING, OPEN, CLOSING_REQUEST, CLOSING };
const char * Controller::serverStates[] = { "closed", "opening_request", "opening", "open", "closing_request", "closing", NULL };

#endif
