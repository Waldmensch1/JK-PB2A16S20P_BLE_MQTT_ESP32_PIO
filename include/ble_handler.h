#ifndef BLE_HANDLER_H
#define BLE_HANDLER_H
#include "config.h"
#include <Arduino.h>
#include "BLEDevice.h"
#include "mqtt_handler.h"
#include "parser.h"

extern bool debug_flg;
extern bool debug_flg_full_log;

void ble_setup();
void ble_loop();

#endif // BLE_HANDLER_H