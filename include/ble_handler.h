#ifndef BLE_HANDLER_H
#define BLE_HANDLER_H
#include "config.h"
#include "settings.h"
#include <vector>
#include <mutex>
#include <Arduino.h>
#include "BLEDevice.h"
#include "mqtt_handler.h"
#include "parser.h"

void ble_setup();
void ble_loop();

#endif // BLE_HANDLER_H