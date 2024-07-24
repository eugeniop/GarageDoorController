#include <ArduinoJson.h>

#include "HTTPRequest.h"

#include "Config.h" // Configuration file for WiFi, server, etc

//ToDo: initialize all components (WiFi, Serial, etc)
void setup() {

  //Enable for debugging
  Serial.begin(115200);
  while(!Serial);

  //Initialize WiFi
  WiFi_Init();
  
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); // Ensure relay is off
}

typedef enum { NO_ACTION, INVALID_RESPONSE, INVALID_REQUEST, REQUEST_FAILED, ACTIVATED, COMPLETED } COMMAND_RESULT;

/*
 * This function needs to activate the relay that will simulate the pressing of the button on the garage door existing control
 * 
 */

// For simulation
typedef enum { OPENING, CLOSING } doorState;
int doorStatus;
int counter;
 
int executeCommandOnDoor(const char * status){
  Serial.println("Activating relay to open/close the door...");
  //digitalWrite(RELAY_PIN, HIGH); // Activate relay
  delay(1000); // Simulate button press duration
  //digitalWrite(RELAY_PIN, LOW); // Deactivate relay

  int ret = NO_ACTION;

  if(!strcmp(status, "opening")){
    doorStatus = OPENING;
    counter = 3;
    ret = ACTIVATED;
  }

  if(!strcmp(status, "closing")){
    doorStatus = CLOSING;
    counter = 3;
    ret = ACTIVATED;
  }
  
  return ret;
}

/*
 * This function notifies the backend of the completion of a action
 */
int sendCommandCompletion(int sensorOpen, int sensorClose){
  if (sensorOpen && sensorClose) {
    return INVALID_REQUEST;
  }

  HTTPRequest req;
  
  /*
   * StaticJsonDocument is deprecated, and replaced by JsonDocument. In addition to that, I like using ArduinoJson for 
   * "parsing" JSON. Building it is really straightforward and sprintf is your friend. The original code is simpler/faster
   *  and more compact:
   *   sprintf(req.dataBuffer(), "{\"status\":\"%s\"}", sensorOpen ? "open_complete" : ( sensorClose ? "close_complete" : "in_progress"));
   *   
   *  Also, the way request.postJSON works is by expecting the data to be ready in the response.data buffer (which you can get a pointer to 
   *  through request.dataBuffer()
   */
  sprintf(req.dataBuffer(), "{\"status\":\"%s\",\"controllerId\":\"%s\"}", sensorOpen ? "open_complete" : ( sensorClose ? "close_complete" : "in_progress"), CTRL_ID);
  Serial.println(req.dataBuffer());
  auto res = req.postJSON(SERVER_URL, DOOR_PATH, 443, API_KEY, NULL);

  if(!res){
    return INVALID_RESPONSE;
  }

  if(res->statusCode == 200) {
    Serial.println("Completed");
    Serial.println(res->statusCode);
    return COMPLETED;
  }

  Serial.println("Failed");
  Serial.println(res->statusCode);
  return REQUEST_FAILED;
}

int checkCommandComplete(){

  if(counter == 0){
    Serial.println("Completion");
    return sendCommandCompletion(doorStatus==OPENING, doorStatus==CLOSING);
  } else {
    Serial.println(counter);
    counter--;
    return NO_ACTION;
  }
}


/*
 * Checks for any pending actions. The 2 states we care (initially), is OPENING and CLOSING
 * 
 */
int getCommand(){

  //ToDo: move all parameters to configuration
  HTTPRequest req;

  char apiPath[100];
  sprintf(apiPath, STATUS_PATH, CTRL_ID);
  Serial.println(apiPath);
  auto res = req.get(SERVER_URL, apiPath, 443, API_KEY, NULL);
  
  if(!res){
    Serial.println("Invalid response");
    return INVALID_RESPONSE;
  }
  
  if(res->statusCode != 200){
    Serial.println("Request failed:");
    Serial.println(res->statusCode);
    return REQUEST_FAILED;
  }
  
  JsonDocument doc;
  deserializeJson(doc, res->data);
  Serial.println(res->data);
  
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
      Serial.println("Getting command");
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

  delay(2000);  //We check every 2 seconds
}
