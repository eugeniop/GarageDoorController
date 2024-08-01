#ifndef CONFIG_H
#define CONFIG_H

#define RELAY_PIN         9
#define SENSOR_CLOSE_PIN  5
#define SENSOR_OPEN_PIN   6

#define SERVER_URL  "garage-door-c2bec621cb00.herokuapp.com"
//#define STATUS_PATH "/api/controller/command/%s"
#define DOOR_PATH   "/api/door/USER-CTRL-1"

#include "secrets.h"

#endif
