#include "main.h"

#ifdef NTPSERVER
const char* ntpServer = NTPSERVER;
#ifdef TIMEZONE
const char *time_zone = TIMEZONE;
#else
const long  gmtOffset_sec = GMTOFFSET;
const int   daylightOffset_sec = DLOFFSET;
#endif
#endif //NTPSERVER

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

#ifdef NTPSERVER
    // NTP Setup
#ifdef TIMEZONE
    configTzTime(time_zone, ntpServer);
#else
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
#endif
    DEBUG_PRINTLN("NTP-Time synced");
#endif //NTPSERVER
    
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
    wifi_loop();
    state_handler();
    mqtt_loop();
    ble_loop();

#ifdef USE_RS485
    rs485_loop();
#endif
}
