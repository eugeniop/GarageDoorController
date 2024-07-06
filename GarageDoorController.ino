#include <ArduinoJson.h>

#include "HTTPRequest.h"

#include "Config.h" // Configuration file for WiFi, server, and access token

#define ACCESS_TOKEN  "TOKEN"

//ToDo: initialize all components (WiFi, Serial, etc)
void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); // Ensure relay is off

  //ToDo: None of this is needed since HTTP and WiFiHelper do all this
  // you could just call WiFi_Init(); and then have HTTPRequest take care of the details
  
  //WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
//
//  while (WiFi.status() != WL_CONNECTED) {
//    delay(1000);
//    Serial.println("Connecting to WiFi...");
//  }
//  Serial.println("Connected to WiFi");
  
}

typedef enum { NO_ACTION, INVALID_RESPONSE, INVALID_REQUEST, REQUEST_FAILED, ACTIVATED, COMPLETED } COMMAND_RESULT;


/*
 * This function needs to activate the relay that will simulate the pressing of the button on the garage door existing control
 * 
 */
int executeCommandOnDoor(const char * status){
  Serial.println("Activating relay to open/close the door...");
  digitalWrite(RELAY_PIN, HIGH); // Activate relay
  delay(1000); // Simulate button press duration
  digitalWrite(RELAY_PIN, LOW); // Deactivate relay
  return ACTIVATED;
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
  StaticJsonDocument<200> doc;
  JsonDocument doc;
  doc["status"] = sensorOpen ? "open_complete" : (sensorClose ? "close_complete" : "in_progress");
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);
  
  auto res = req.postJSON(SERVER_URL, "/api/door", 443, ACCESS_TOKEN, NULL);

  if (!res) {
    return INVALID_RESPONSE;
  }

  if (res->statusCode == 200) {
    return COMPLETED;
  }

  return REQUEST_FAILED;
}

int checkCommandComplete(){
  int sensorOpen = digitalRead(SENSOR_OPEN_PIN);
  int sensorClose = digitalRead(SENSOR_CLOSE_PIN);

  if (sensorOpen || sensorClose) {
    return sendCommandCompletion(sensorOpen, sensorClose);
  }
  return NO_ACTION;
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
