////////////////////////////// Settings //////////////////////////////

// if devicename is not defined in platformio.ini, use this default
#ifndef DEVICENAME
#define DEVICENAME "JK-PB2A16S20P"
#endif

#define SSID_NAME "SSID"
#define SSID_PASSWORD "pass"

#define MQTT_SERVER "192.168.178.195"
#define MQTT_PORT 1883
#define MQTT_USERNAME ""
#define MQTT_PASSWORD ""

// this attached a unique id to the mqtt client name. This is useful if 
// you have multiple devices with the same name. It collides with the
// retained messages on the broker, so last will messages are not working
// Uncomment only if you have problems connecting to the broker
//#define USE_RANDOM_CLIENT_ID

// reboot after BLE scans without success
#define REBOOT_AFTER_BLE_RETRY 20

// only publish every this seconds
#define PUBLISH_DELAY 0

// publish values also if they are not changed. 0 = off, n = seconds 
#define MIN_PUBLISH_TIME 300



//#define USE_INFLUXDB

#ifdef USE_INFLUXDB
// InfluxDB v2 server url, e.g. https://eu-central-1-1.aws.cloud2.influxdata.com (Use: InfluxDB UI -> Load Data -> Client Libraries)
// InfluxDB 1.8+  (v2 compatibility API) server url, e.g. http://192.168.1.48:8086
#define INFLUXDB_URL "http://192.168.178.195:8086"
// InfluxDB v2 server or cloud API authentication token (Use: InfluxDB UI -> Load Data -> Tokens -> <select token>)
// InfluxDB 1.8+ (v2 compatibility API) use form user:password, eg. admin:adminpass
#define INFLUXDB_TOKEN "your token"
// InfluxDB v2 organization name or id (Use: InfluxDB UI -> Settings -> Profile -> <name under tile> )
// InfluxDB 1.8+ (v2 compatibility API) use any non empty string
#define INFLUXDB_ORG "your org"
// InfluxDB v2 bucket name (Use: InfluxDB UI -> Load Data -> Buckets)
// InfluxDB 1.8+ (v2 compatibility API) use database name
#define INFLUXDB_BUCKET "your bucket"

// Set timezone string according to https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html
// Examples:
//  Pacific Time: "PST8PDT"
//  Eastern: "EST5EDT"
//  Japanesse: "JST-9"
//  Central Europe: "CET-1CEST,M3.5.0,M10.5.0/3"
#define TZ_INFO "CET-1CEST,M3.5.0,M10.5.0/3"

//#define INFLUX_TEMP_SENSOR_1
//#define INFLUX_TEMP_SENSOR_2
//#define INFLUX_TEMP_SENSOR_3
//#define INFLUX_TEMP_SENSOR_4
//#define INFLUX_TEMP_SENSOR_5
//#define INFLUX_TEMP_SENSOR_MOSFET
//#define INFLUX_BATTERY_VOLTAGE
//#define INFLUX_BATTERY_CURRENT
//#define INFLUX_BATTERY_POWER
//#define INFLUX_BATTERY_SOC
//#define INFLUX_CELLS_VOLTAGE

#endif

// This is not ready and todo. Leave that commented out
//#define USE_RS485

#ifdef USE_RS485
#define RS485_RX 16
#define RS485_TX 17
#define DE_PIN 4
#define RE_PIN 2
#endif