#include "settings.h"

Preferences prefs;

uint16_t publish_delay;
bool debug_flg = false;
bool debug_flg_full_log = false;

void write_setting(const char *setting_name, uint16_t value) {
    // "storage" is the namespace
    prefs.begin("storage", false);
    // Save the uint16_t value
    prefs.putUShort(setting_name, value);
    prefs.end();
    DEBUG_PRINTLN("Value for " + String(setting_name) + "  changed to  " + String(value));
    re_read_settings();
}

void write_setting(const char *setting_name, bool value) {
    prefs.begin("storage", false);
    prefs.putUChar(setting_name, value ? 1 : 0);
    prefs.end();
    DEBUG_PRINTLN("Value for " + String(setting_name) + "  changed to  " + String(value));
    re_read_settings();
}

uint16_t read_setting(const char *setting_name, uint16_t default_value) {
    prefs.begin("storage", true);
    uint16_t value = prefs.getUShort(setting_name, default_value);
    prefs.end();
    DEBUG_PRINTLN("Read Value for " + String(setting_name) + " is " + String(value));
    return value;
}

bool read_setting(const char *setting_name, bool default_value) {
    prefs.begin("storage", true);
    bool value = prefs.getUChar(setting_name, default_value ? 1 : 0) == 1;
    prefs.end();
    DEBUG_PRINTLN("Read Value for " + String(setting_name) + " is " + String(value));
    return value;
}

void re_read_settings() {
    publish_delay = read_setting("publish_delay", (uint16_t)PUBLISH_DELAY);
    // Limit to 1000 seconds as sometimes the value is corrupted 0xFFFF
    publish_delay = publish_delay > 1000 ? (uint16_t)PUBLISH_DELAY : publish_delay;
    DEBUG_PRINTLN("Re-Read Value for publish_delay is " + String(publish_delay));

    debug_flg = read_setting("debug_flg", false);
    DEBUG_PRINTLN("Re-Read Value for debug_flg is " + String(debug_flg));

    debug_flg_full_log = read_setting("debug_flg_full_log", false);
    DEBUG_PRINTLN("Re-Read Value for debug_flg_full_log is " + String(debug_flg_full_log));
}

void init_settings() {

    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    re_read_settings();
}