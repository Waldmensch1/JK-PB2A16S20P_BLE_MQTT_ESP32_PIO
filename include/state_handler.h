#ifndef STATE_HANDLER_H
#define STATE_HANDLER_H
#include <WiFi.h>
#include "mqtt_handler.h"
#include <map>

void state_handler();
void setState(String key, String value, bool publish);

#endif //STATE_HANDLER_H