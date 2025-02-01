#include <Arduino.h>
#include "BLEDevice.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include "config.h"

void analyze();
boolean reconnect();
void initWiFi();
void readDeviceDataRecord();
void readCellDataRecord();
String toBinaryString(uint32_t value, int bits);

// W-LAN Setting
const char *ssid = SSID_NAME;
const char *password = SSID_PASSWORD;
WiFiClient espClient;

// BMS-BLE Settings
const char *Geraetename = DEVICENAME; // JK-B2A24S20P JK-B2A24S15P
static void notifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify);
bool connectToBLEServer();

// MQTT Setting
const char *mqtt_server = MQTT_SERVER;                      // -> hier die IP des MQTT Server eingeben
const int mqtt_port = MQTT_PORT;                            // -> hier den Port einstellen für den MQTT Server
const char *mqtt_username = MQTT_USERNAME;                  // -> hier MQTT Benutzername eintragen
const char *mqtt_passwort = MQTT_PASSWORD;                  // -> hier MQTT Passwort eingeben
String mqttname = String("jk_ble_listener/") + Geraetename; // -> hier wird der MQTT Gerätename festgelegt
const int mqttpublishtime_offset = 1000;                    //-> hier einstellen wie oft Danten gesnedet werden sollen 1000 = jede Sekunde
bool mqtt_buffer_maxed = 0;
long lastReconnectAttempt = 0;
String willTopic = String("jk_ble_listener/") + String(Geraetename) + String("/status");
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

static BLEUUID serviceUUID("ffe0"); // The remote service we wish to connect to.
static BLEUUID charUUID("ffe1");    // ffe1 // The characteristic of the remote service we are interested in.
BLEClient *pClient;
BLEScan *pBLEScan;
byte getdeviceInfo[20] = {0xaa, 0x55, 0x90, 0xeb, 0x97, 0x00, 0xdf, 0x52, 0x88, 0x67, 0x9d, 0x0a, 0x09, 0x6b, 0x9a, 0xf6, 0x70, 0x9a, 0x17, 0xfd}; // Device Infos
byte getInfo[20] = {0xaa, 0x55, 0x90, 0xeb, 0x96, 0x00, 0x79, 0x62, 0x96, 0xed, 0xe3, 0xd0, 0x82, 0xa1, 0x9b, 0x5b, 0x3c, 0x9c, 0x4b, 0x5d};

unsigned long sendingtime = 0;
unsigned long bleScantime = 0;
unsigned long mqttpublishtime = 0;
unsigned long newdatalasttime = 0;
unsigned long ble_connection_time = 0;
bool blocked_for_parsing = false;
bool sent_once = false;

byte receivedBytes_cell[320];
byte receivedBytes_device[320];

int frame = 0;
bool received_start = false;
bool received_start_frame = false;
bool received_complete = false;
bool new_data = false;
byte BLE_Scan_counter = 0;

static bool doConnect = false;
static bool ble_connected = false;
static BLERemoteCharacteristic *pRemoteCharacteristic;
static BLEAdvertisedDevice *myDevice;

uint8_t counter_last = 0;
uint32_t cells_used = 0;
float volts_old[30] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
float cell_avg_voltage = 0;
float cell_diff_voltage = 0;
float cellResistance_old[30] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
float temp_pwr_tube = 0;
float cell_resistance_alert = 255;
float battery_voltage = 0;
float battery_power = 0;    
float battery_charge_current = 0;
float temp_sensor1 = 0;
float temp_sensor2 = 0;
float temp_sensor3 = 0;
float temp_sensor4 = 0;
float temp_sensor5 = 0;
int16_t errors_mask = 255;
float balance_current = 10000;
byte balancing_action = 255;
uint8_t battery_soc = 0;
float capacity_remaining = 0;
float capacity_total = 0;   
uint32_t battery_cycle_count = 0;
float cycle_capacity_total = 0;
byte battery_soh = 0;
byte battery_precharge_status = 255;
byte charging_mosfet_status = 255;
byte discharging_mosfet_status = 255;
byte precharging_status = 255;