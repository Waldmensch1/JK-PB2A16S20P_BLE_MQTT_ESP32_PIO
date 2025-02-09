#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <Arduino.h>
#include <cstring>
#include <PubSubClient.h>
#include <WiFi.h>
#include "config.h"
#include "settings.h"

void mqtt_loop();
void mqtt_init();

// MQTT Setting
extern String mqttname;
extern long mqttpublishtime_offset;
extern PubSubClient mqtt_client;

#endif // MQTT_HANDLER_H