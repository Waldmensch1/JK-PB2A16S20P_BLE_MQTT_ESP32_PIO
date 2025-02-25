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
#include "macros.h"
#include <map>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

void setState(String key, String value, bool publish);
void mqtt_loop();
void mqtt_init();

// MQTT Setting
// MQTT Client name used when connecting to broker
#define MQTT_CLTNAME TEXTIFY(CLTNAME)
extern String mqttname;
extern String mqtt_main_topic;
extern PubSubClient mqtt_client;
extern SemaphoreHandle_t mqttMutex;

#endif // MQTT_HANDLER_H