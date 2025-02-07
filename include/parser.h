#ifndef PARSER_H
#define PARSER_H

#include "config.h"
#include "settings.h"
#include <Arduino.h>
#include "mqtt_handler.h"

#include "arduino_base64.hpp"

#ifdef USE_INFLUXDB
#include <InfluxDbClient.h>
#include "influxdb_handler.h"
extern InfluxDBClient influx_client;
#endif

void readDeviceDataRecord();
void readCellDataRecord();

extern byte receivedBytes_cell[320];
extern byte receivedBytes_device[320];
extern bool blocked_for_parsing;
extern bool debug_flg_full_log;
extern uint16_t publish_delay;

#endif // PARSER_H