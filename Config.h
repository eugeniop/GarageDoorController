#ifndef CONFIG_H
#define CONFIG_H

#define RELAY_PIN 3
#define SENSOR_CLOSE_PIN 1 
#define SENSOR_OPEN_PIN 3
#define WIFI_SSID     "1"
#define WIFI_PASSWORD "2"
#define SERVER_URL    "garage-door-c2bec621cb00.herokuapp.com"
#define STATUS_PATH "/api/controller/command/%s"
#define DOOR_PATH "/api/door"
#define CTRL_ID "USER-CTRL-1"

#include "secrets.h"

#endif
