#ifndef WIFIHELPER_H
#define WIFIHELPER_H

//ToDo: move to general config
#define ADAFRUIT_FEATHER_M0_
#define WIFI_CONNECT_TIMEOUT_MS 10000

#ifdef ADAFRUIT_FEATHER_M0_
  #include <SPI.h>
  #include <WiFi101.h>
#else
  #include <WiFiNINA.h>
#endif

/* status:

 *  WL_NO_SHIELD        = 255,   // for compatibility with WiFi Shield library
    WL_IDLE_STATUS      = 0,
    WL_NO_SSID_AVAIL    = 1,
    WL_SCAN_COMPLETED   = 2,
    WL_CONNECTED        = 3,
    WL_CONNECT_FAILED   = 4,
    WL_CONNECTION_LOST  = 5,
    WL_DISCONNECTED     = 6
 */ 

#if defined(__cplusplus)
extern "C" {
#endif

void WiFi_Init(){
  #ifdef ADAFRUIT_FEATHER_M0_
    WiFi.setPins(8,7,4,2);
  #endif
}

int WiFi_ConnectWithParams(const char * ssid, const char * password, int retries){

  //Set WiFi connect timeout to less than WDT
  WiFi.setTimeout(WIFI_CONNECT_TIMEOUT_MS);
    
  int status;
    
//  trace.log("WiFi", "Connecting.");
  status = WiFi.status();
  if (status == WL_NO_SHIELD) {
    error.log("WiFi", "No WiFi shield.");
    // If we reach this, something bad really happened. We stop the board and wait for WDT 
    // to kick-in and reset the board.
    while(1){
    }
    return status;
  }

  if(strlen(ssid)==0){
    return status;
  }
  
  while(status != WL_CONNECTED){
    if(strlen(password) > 0){
      status = WiFi.begin(ssid, password);
    } else {
      status = WiFi.begin(ssid);  //Passwordless WiFi
    }
    if(--retries == 0){
      break;
    }
    
    // wait 1 seconds for trying again connection
    delay(1000);
  }
  
  if(status == WL_CONNECTED){
    trace.log("WiFi", "Connected WiFi");
  } else {
    error.log("WiFi", "Connect failed: ", status);
  }
  
  return status;
}

int WiFi_Status(){
  return WiFi.status();
}

unsigned long WiFi_GetTime(){
  return WiFi.getTime();
}

void WiFi_Close(){
    
  WiFi.disconnect();  //this function takes same time it seems...
  
  int retry = 5;
  while(WiFi.status()==WL_CONNECTED && retry--){
    delay(1000);  //Magic delay
  }

  WiFi.end();
}

#if defined(__cplusplus)
};
#endif

#endif
