#ifndef CONFIG_H
#define CONFIG_H

#define RELAY_PIN 3
#define SENSOR_CLOSE_PIN 1 
#define SENSOR_OPEN_PIN 3

#define SERVER_URL    "{heroku URL *without* https:.//}"
#define STATUS_PATH "/api/controller/command/%s"
#define DOOR_PATH "/api/door"

#define CTRL_ID "USER-CTRL-1"

#include "secrets.h"

#endif
