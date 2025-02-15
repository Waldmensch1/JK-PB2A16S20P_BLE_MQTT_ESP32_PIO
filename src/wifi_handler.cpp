#include "wifi_handler.h"

// wifi Setting
const char *ssid = SSID_NAME;
const char *password = SSID_PASSWORD;

void init_wifi() {
    byte wifi_retry = 0;
    WiFi.disconnect();      // ensure WiFi is disconnected
    WiFi.persistent(false); // do not persist as WiFi.begin is not helpful then. Decreases connection speed but helps in other ways
    WiFi.mode(WIFI_STA);
    // esp_wifi_set_ps( WIFI_PS_NONE ); // do not set WiFi to sleep, increases stability
    WiFi.config(0u, 0u, 0u); // ensure settings are reset, retrieve new IP Address in any case => DHCP necessary
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi ..");
    while (WiFi.status() != WL_CONNECTED && wifi_retry < 10) {
        WiFi.begin(ssid, password);
        wifi_retry++;
        Serial.print('.');
        delay(10000);
    }
    if (wifi_retry >= 10) {
        Serial.println("\nReboot...");
        ESP.restart();
    }
    Serial.println("Ready");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}