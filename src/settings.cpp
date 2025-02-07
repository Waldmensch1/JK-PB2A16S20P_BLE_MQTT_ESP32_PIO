#include "settings.h"

Preferences prefs;

uint16_t publish_delay;

void init_settings()
{

    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    publish_delay = read_setting("publish_delay", PUBLISH_DELAY);
}

void write_setting(const char *setting_name, uint16_t value) {
    prefs.begin("storage", false);       // "storage" is the namespace
    prefs.putUShort(setting_name, value); // Save the uint16_t value
    prefs.end();
    Serial.println("Value for " + String(setting_name) + "  changed to  " + String(value));
    re_read_settings();
}

uint16_t read_setting(const char *setting_name, uint16_t default_value) {
    prefs.begin("storage", false);                                 // "storage" is the namespace
    uint16_t value = prefs.getUShort(setting_name, default_value); // Read the uint16_t value
    prefs.end();
    Serial.println("Read Value for " + String(setting_name) + " is " + String(value));
    return value;
}

void re_read_settings() {
    publish_delay = read_setting("publish_delay", PUBLISH_DELAY);
    Serial.println("Re-Read Value for publish_delay is " + String(publish_delay));
}