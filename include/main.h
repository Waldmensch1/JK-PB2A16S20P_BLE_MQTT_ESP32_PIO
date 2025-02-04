#include <Arduino.h>
#include "BLEDevice.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include "config.h"
#ifdef USE_INFLUXDB
#include "influxdb_handler.h"
#endif


void handleMQTTMessage(const char* topic, const char* command, byte* payload, unsigned int length, bool& flag);
void analyze();
boolean reconnect();
void initWiFi();


// W-LAN Setting
const char *ssid = SSID_NAME;
const char *password = SSID_PASSWORD;
WiFiClient espClient;

// BMS-BLE Settings
const char *devicename = DEVICENAME; // JK-B2A24S20P JK-B2A24S15P
static void notifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify);
bool connectToBLEServer();

// MQTT Setting
const char *mqtt_server = MQTT_SERVER; 
const int mqtt_port = MQTT_PORT;
const char *mqtt_username = MQTT_USERNAME; 
const char *mqtt_passwort = MQTT_PASSWORD; 
String mqttname = String("jk_ble_listener/") + devicename; 
bool mqtt_buffer_maxed = 0;
long lastReconnectAttempt = 0;
long mqttpublishtime_offset = 1000;

// MQTT Last Will
String willTopic = String("jk_ble_listener/") + String(devicename) + String("/status");
String willMessage = "offline";
byte willQoS = 0;
boolean willRetain = true;

void MQTTCallback(char *topic, byte *payload, unsigned int length);
PubSubClient client(mqtt_server, mqtt_port, MQTTCallback, espClient);

const byte CONFIGDATA = 0x01;
const byte CELLDATA = 0x02;
const byte DEVICEDATA = 0x03;
byte actualdata = 0x00;

bool debug_flg = false;
bool debug_flg_full_log = false;

// BLE
static BLERemoteCharacteristic *pRemoteCharacteristic;
static BLEAdvertisedDevice *myDevice;
static BLEUUID serviceUUID("ffe0"); // The remote service we wish to connect to.
static BLEUUID charUUID("ffe1");    // ffe1 // The characteristic of the remote service we are interested in.
BLEClient *pClient;
BLEScan *pBLEScan;
byte getdeviceInfo[20] = {0xaa, 0x55, 0x90, 0xeb, 0x97, 0x00, 0xdf, 0x52, 0x88, 0x67, 0x9d, 0x0a, 0x09, 0x6b, 0x9a, 0xf6, 0x70, 0x9a, 0x17, 0xfd}; // Device Infos
byte getInfo[20] = {0xaa, 0x55, 0x90, 0xeb, 0x96, 0x00, 0x79, 0x62, 0x96, 0xed, 0xe3, 0xd0, 0x82, 0xa1, 0x9b, 0x5b, 0x3c, 0x9c, 0x4b, 0x5d};

static bool doConnect = false;
static bool ble_connected = false;
unsigned long sendingtime = 0;
unsigned long bleScantime = 0;
unsigned long mqttpublishtime = 0;
unsigned long newdatalasttime = 0;
unsigned long ble_connection_time = 0;
bool blocked_for_parsing = false;
bool sent_once = false;

// BLE Data
byte receivedBytes_cell[320];
byte receivedBytes_device[320];

int frame = 0;
bool received_start = false;
bool received_start_frame = false;
bool received_complete = false;
bool new_data = false;
byte BLE_Scan_counter = 0;




