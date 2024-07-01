#include <ArduinoJson.h>

#include "HTTPRequest.h"

//ToDo: move to a configuration file
#define ACCESS_TOKEN  "TOKEN"

//ToDo: initialize all components (WiFi, Serial, etc)
void setup() {
  
}

typedef enum { NO_ACTION, INVALID_RESPONSE, INVALID_REQUEST, REQUEST_FAILED, ACTIVATED, COMPLETED } COMMAND_RESULT;


/*
 * This function needs to activate the relay that will simulate the pressing of the button on the garage door existing control
 * 
 */
int executeCommandOnDoor(const char * status){
  //ToDo: complete with actual execution ("press the button") - Will use the relay shield from Adafruit
  return ACTIVATED;
}

/*
 * This function notifies the backend of the completion of a action
 */
int sendCommandCompletion(int sensorOpen, int sensorClose){

  if(sensorOpen && sensorClose){  //Should never happen
    return INVALID_REQUEST;
  }
  
  HTTPRequest req;
  sprintf(req.dataBuffer(), "{\"status\":\"%s\"}", sensorOpen ? "open_complete" : ( sensorClose ? "close_complete" : "in_progress"));

  //ToDo: change URL and PATH move to config
  auto res = req.postJSON("https://herokuapp.com", "/api/door", 443, ACCESS_TOKEN, NULL);

  if(!res){
    return INVALID_RESPONSE;
  }

  if(res->statusCode == 200){
    return COMPLETED;
  }

  return REQUEST_FAILED;
}

int checkCommandComplete(){
  //ToDo: check sensors to be signaling completion
  //we will have 2 sensors detecting when the door is closed and open
  // Open=true  Closed=False -> Open
  // Open=false Closed=False -> Door is moving (in between open and closed)
  // Open=false Closed=true  -> Closed
  // Open=true  Closed=true  -> Should never happen
  // When either sensor is true, we send an event to the backend to signal "completion"
  //sendCommandCompletion(sensorOpen, sensorClose);
  return COMPLETED;
}

/*
 * Checks for any pending actions. The 2 states we care (initially), is OPENING and CLOSING
 * 
 */
int getCommand(){

  //ToDo: move all parameters to configuration
  HTTPRequest req;
  auto res = req.get("https://herokuapp.com", "/api/door", 443, ACCESS_TOKEN, NULL);
  
  if(!res){
    return INVALID_RESPONSE;
  }
  
  if(res->statusCode != 200){
    return REQUEST_FAILED;
  }
  
  JsonDocument doc;
  deserializeJson(doc, res->data);
  
  const char* status = doc["status"];
  
  if(!strcmp(status, "opening") || !strcmp(status, "closing")){
    return executeCommandOnDoor(status);
  }

  return NO_ACTION;
}

/*
 * The controller can be in 2 different states: it is WAITING for a command to execute (OPEN/CLOSE) or it is in the middle of executing one (ACTIVATING)
 * If it is WAITING, then it checks for new commands - if none is present, it keeps WAITING.
 * If a command is present, state changes to ACTIVATING, it executes the command on the door, and waits for COMPLETION.
*/
typedef enum { WAITING, ACTIVATING } STATE;

STATE state = WAITING;

void loop() {
  int ret;
  switch(state){
    case WAITING:
      ret=getCommand();
      if(ret==ACTIVATED){
        state=ACTIVATING;
      }
      break;
    case ACTIVATING:
      ret=checkCommandComplete();
      if(ret==COMPLETED){
        state=WAITING;
      }
      break;
    default:
      //Should never happen
      break;
  }

  delay(1000);  //We check ever second
}
