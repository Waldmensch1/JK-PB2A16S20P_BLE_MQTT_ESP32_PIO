#include "influxdb_handler.h"

#ifdef USE_INFLUXDB
#include <InfluxDbClient.h>

// Assuming you have an InfluxDB client instance
InfluxDBClient influx_client;
const char *influx_devicename = DEVICENAME; // JK-B2A24S20P JK-B2A24S15P
String influx_prefix = influx_devicename + String("_");


void init_influxdb()
{
    Serial.println("Init InfluxDB client");
    // Set up the client
    influx_client.setInsecure(true); // Set to true if you are using a self-signed certificate
    influx_client.setConnectionParams(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN);
    influx_client.setWriteOptions(WriteOptions().writePrecision(WritePrecision::S));

    // Accurate time is necessary for certificate validation
    // For the fastest time sync find NTP servers in your area: https://www.pool.ntp.org/zone/
    // Syncing progress and the time will be printed to Serial
    timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");

    // Check server connection
    if (influx_client.validateConnection())
    {
        Serial.print("Connected to InfluxDB: ");
        Serial.println(influx_client.getServerUrl());
    }
    else
    {
        Serial.print("InfluxDB connection failed: ");
        Serial.println(influx_client.getLastErrorMessage());
    }
}

void publishToInfluxDB(const String &topic, int value)
{
    String valueStr = String(value);
    Point point(influx_prefix + topic);
    point.addField("value", value);
    influx_client.writePoint(point);
}

void publishToInfluxDB(const String &topic, float value)
{
    String valueStr = String(value, 3); // Default to 3 decimal places for floats
    Point point(influx_prefix + topic);
    point.addField("value", value);
    influx_client.writePoint(point);
}

#endif // USE_INFLUXDB