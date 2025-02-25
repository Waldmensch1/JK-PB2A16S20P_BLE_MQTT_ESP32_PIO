#ifndef WIFI_HANDLER_H
#define WIFI_HANDLER_H
#include "config.h"
#include <Arduino.h>
#include <WiFi.h>
#include "macros.h"
#include "mqtt_handler.h"

void init_wifi();
void wifi_loop();

// DHCP Hostname
#define WIFI_DHCPNAME TEXTIFY(CLTNAME)

#endif