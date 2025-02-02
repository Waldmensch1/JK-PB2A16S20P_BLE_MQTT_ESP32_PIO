#include <Arduino.h>
#include <PubSubClient.h>
#include "arduino_base64.hpp"

void readDeviceDataRecord();
void readCellDataRecord();
String toBinaryString(uint32_t value, int bits);

extern byte receivedBytes_cell[320];
extern byte receivedBytes_device[320];
extern String mqttname;
extern bool blocked_for_parsing;
extern bool debug_flg_full_log;

extern PubSubClient client;