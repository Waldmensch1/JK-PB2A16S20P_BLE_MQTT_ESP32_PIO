#include "main.h"

void setup() {
    Serial.begin(115200);

    init_settings();

    Serial.println("");
    Serial.println("");
    Serial.print("JK-BMS Listener V ");
    Serial.println(VERSION);
    Serial.println("Starting");

    init_led();
    // Send LED_FLASH state to the LED task
    set_led(LedState::LED_DOUBLE_FLASH);

    // WIFI Setup
    init_wifi();
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
        Serial.println("WIFI Connection is Lost! Try to Reconnect...");
        init_wifi();
    } else {
        mqtt_loop();

        ble_loop();

#ifdef USE_RS485
        rs485_loop();
#endif
    }
}
