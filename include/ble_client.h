#ifndef BLE_CLIENT_H
#define BLE_CLIENT_H
#include <vector>
#include <mutex>
#include <Arduino.h>
#include <NimBLEDevice.h>
#include <vector>
#include <mutex>
#include "config.h"
#include "settings.h"
#include "mqtt_handler.h"
#include "parser.h"
#include "led_control.h"
#include "state_handler.h"

#define BLE_RECONNECT 2000UL
#define REBOOT_AFTER_BLE_RETRY 20
#define TIMEOUT_NO_DATA 60000UL
#define SCAN_REPEAT_TIME 15000UL
#define PAUSE_SENDING 5000UL
#define BUFFER_TIMEOUT 1000UL
#define BUFFER_SIZE 300
#define REPEAT_SEND_INTERVAL 3600000UL // Define the interval for sending data (1 hour)
#define INITIAL_SEND_INTERVAL 5000UL   // Define the interval for the initial send (5 seconds)
#define RSSI_INTERVAL 60000UL


void ble_setup();
void ble_loop();

#endif // BLE_CLIENT_H

