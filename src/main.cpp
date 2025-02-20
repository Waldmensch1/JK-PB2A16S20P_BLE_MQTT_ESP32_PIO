#include "main.h"

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;  // Deutschland: MEZ (UTC+1)
const int   daylightOffset_sec = 3600;  // Sommerzeit (UTC+2)


void setup() {
#ifdef SERIAL_OUT
    Serial.begin(115200);
#endif
    init_settings();

    DEBUG_PRINTLN("");
    DEBUG_PRINTLN("");
    DEBUG_PRINT("JK-BMS Listener V ");
    DEBUG_PRINTLN(VERSION);
    DEBUG_PRINTLN("Starting");

#ifdef USELED
    init_led();
    // Send LED_FLASH state to the LED task
    set_led(LedState::LED_DOUBLE_FLASH);
#endif

    // WIFI Setup
    init_wifi();

    // Get the current time
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    DEBUG_PRINTLN("NTP-Time synced");

    mqtt_init();

#ifdef USE_RS485
    init_rs485();
#endif

#ifdef USE_INFLUXDB
    // InfluxDB Setup
    init_influxdb();
#endif

    ble_setup();
}

void loop() {
    if (WiFi.status() != WL_CONNECTED) {
        DEBUG_PRINTLN("WIFI Connection is Lost! Try to Reconnect...");
        init_wifi();
    } else {
        mqtt_loop();

        ble_loop();

#ifdef USE_RS485
        rs485_loop();
#endif
    }
}
