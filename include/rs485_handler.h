
#ifndef RS485_HANDLER_H
#define RS485_HANDLER_H

#include "config.h"

#ifdef USE_RS485
#include "mqtt_client.h"
#include <Arduino.h>
#include <HardwareSerial.h>
#include <PubSubClient.h>

void init_rs485();
void rs485_loop();

extern String mqttname;
extern PubSubClient mqtt_client;

#endif // RS485_HANDLER_H
#endif // USE_RS485