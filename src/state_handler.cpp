#include "state_handler.h"

// Define the map to store key-value pairs
std::map<String, String> stateMap;

String formatUptime(time_t uptime) {
    int days = uptime / 86400;
    int hours = (uptime % 86400) / 3600;
    int minutes = (uptime % 3600) / 60;
    int secs = uptime % 60;

    char buffer[20];
    snprintf(buffer, sizeof(buffer), "%dd %02d:%02d:%02d", days, hours, minutes, secs);
    return String(buffer);
}

void setState(String key, String value, bool publish) {
    stateMap[key] = value;
    String fullTopic = mqttname + "/status/" + key;
    if (publish && mqtt_client.connected()) {
        mqtt_client.publish(fullTopic.c_str(), value.c_str());
    }
}

void publishStates() {
    for (const auto &kv : stateMap) {
        String fullTopic = mqttname + "/status/" + kv.first;
        mqtt_client.publish(fullTopic.c_str(), kv.second.c_str());
    }
}

void publishStatesTask(void *pvParameters) {
    while (true) {
        if (mqtt_client.connected()) {
            publishStates();
        }
        vTaskDelay(min_publish_time * 1000 / portTICK_PERIOD_MS); // Delay for min_publish_time seconds
    }
}

void state_handler() {
    static bool OnFirstRun = true;
    time_t now = time(nullptr);
    time_t bootTime = now - esp_timer_get_time() / 1000000;
    setState("uptime", formatUptime(now - bootTime), OnFirstRun);
    setState("wifi_rssi", String(WiFi.RSSI()), OnFirstRun);
    if (OnFirstRun) {
        // Create the task to call publishStates() every min_publish_time seconds
        xTaskCreate(publishStatesTask, "Publish States Task", 2048, NULL, 0, NULL);
        setState("version", VERSION, true);
        setState("ipaddress", WiFi.localIP().toString(), true);
        setState("ble_connection", "startup", true);
        setState("status", "online", true);        
    }
    OnFirstRun = false;
}