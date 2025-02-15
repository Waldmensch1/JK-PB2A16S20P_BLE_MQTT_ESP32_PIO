#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <Arduino.h>
#include <cstring>
#include <PubSubClient.h>
#include <WiFi.h>
#include "version.h"
#include "config.h"
#include "settings.h"
#include "led_control.h"

void mqtt_loop();
void mqtt_init();

// MQTT Setting
extern String mqttname;
extern String mqtt_main_topic;
extern long mqttpublishtime_offset;
extern PubSubClient mqtt_client;

#endif // MQTT_HANDLER_H