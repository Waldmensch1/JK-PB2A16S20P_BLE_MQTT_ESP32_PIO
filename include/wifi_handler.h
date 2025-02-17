#ifndef WIFI_HANDLER_H
#define WIFI_HANDLER_H
#include "config.h"
#include <Arduino.h>
#include <WiFi.h>
#include "macros.h"

void init_wifi();

// DHCP Hostname
#define WIFI_DHCPNAME TEXTIFY(CLTNAME)

#endif