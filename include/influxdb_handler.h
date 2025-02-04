#include "config.h"

#ifdef USE_INFLUXDB
#ifndef INFLUXDB_HANDLER_H
#define INFLUXDB_HANDLER_H

#include <Arduino.h>
#include "config.h"

void setupInfluxDB();
void publishToInfluxDB(const String &topic, int value);
void publishToInfluxDB(const String &topic, float value);

#endif // INFLUXDB_HANDLER_H
#endif // USE_INFLUXDB