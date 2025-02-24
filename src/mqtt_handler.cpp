#include "mqtt_handler.h"

constexpr unsigned long RECONNECT_DELAY = 5000;
unsigned long lastReconnectAttempt = 0;
constexpr int MQTT_BUFFER_SIZE = 500;

bool mqtt_buffer_maxed = false;

const char *mqtt_server = MQTT_SERVER;
const int mqtt_port = MQTT_PORT;
const char *mqtt_username = MQTT_USERNAME;
const char *mqtt_passwort = MQTT_PASSWORD;
const char *mqtt_devicename = DEVICENAME;
String mqtt_main_topic = String(TOPIC_BASE);
String mqttname = mqtt_main_topic + mqtt_devicename;

String willTopic = mqttname + String("/status/status");
String willMessage = "offline";
byte willQoS = 0;
boolean willRetain = true;

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

        time_t now = time(nullptr);
        time_t bootTime = now - esp_timer_get_time() / 1000000;
        setState("uptime", formatUptime(now - bootTime), false);

        if (mqtt_client.connected()) {
            publishStates();
        }
        vTaskDelay(min_publish_time * 1000 / portTICK_PERIOD_MS); // Delay for min_publish_time seconds
    }
}

void handleMQTTSettingsMessage(const char *topic, const char *command, byte *payload, unsigned int length, bool &flag) {
    // Find the last occurrence of '/'
    const char *lastSlash = strrchr(topic, '/');
    if (lastSlash != nullptr) {
        // Move past the last '/'
        const char *lastPart = lastSlash + 1;
        // Compare the last part of the topic with the command
        if (strcmp(lastPart, command) == 0) {
            // Check if the payload is numeric
            bool isNumeric = true;
            for (unsigned int i = 0; i < length; i++) {
                if (!isdigit(payload[i])) {
                    isNumeric = false;
                    break;
                }
            }

            // If the payload is numeric, convert it to uint16_t and call write_setting
            if (isNumeric) {
                // Ensure the payload is null-terminated
                char payloadStr[length + 1];
                memcpy(payloadStr, payload, length);
                payloadStr[length] = '\0';

                uint16_t value = atoi(payloadStr);
                DEBUG_PRINT("Converted value: ");
                DEBUG_PRINTLN(value);
                write_setting(command, value);
            } else {
                mqtt_client.publish(String(topic).c_str(), String(0).c_str());
            }
        }
    }
}

void handleMQTTMessage(const char *topic, const char *command, byte *payload, unsigned int length, bool &flag, bool setting = false) {

    if (setting) {
        return handleMQTTSettingsMessage(topic, command, payload, length, flag);
    }

    if (strcmp(topic, command) == 0) {
        String Command = String((char *)payload, length);
        flag = (Command == "true");
        write_setting("debug_flg", debug_flg);
        write_setting("debug_flg_full_log", debug_flg_full_log);
    }
}

// handle Subscriptions
void MQTTCallback(char *topic, byte *payload, unsigned int length) {
    handleMQTTMessage(topic, (mqttname + "/parameter/debugging_active").c_str(), payload, length, debug_flg);
    handleMQTTMessage(topic, (mqttname + "/parameter/debugging_active_full").c_str(), payload, length, debug_flg_full_log);

    handleMQTTMessage(topic, String("publish_delay").c_str(), payload, length, debug_flg_full_log, true);
    handleMQTTMessage(topic, String("min_publish_time").c_str(), payload, length, debug_flg_full_log, true);
}

WiFiClient wifi_client;
PubSubClient mqtt_client(mqtt_server, mqtt_port, MQTTCallback, wifi_client);

// Reconnect to MQTT broker
boolean reconnect() {
    mqtt_buffer_maxed = mqtt_client.setBufferSize(MQTT_BUFFER_SIZE);

#ifdef USE_RANDOM_CLIENT_ID
    String random_client_id = mqtt_devicename + String("-");
    random_client_id += String(random(0xffff), HEX);
#else
    String random_client_id = mqtt_devicename;
#endif

    DEBUG_PRINTLN("Attempting MQTT connection... " + random_client_id);

    // Attempt to reconnect to the MQTT broker
    if (mqtt_client.connect(random_client_id.c_str(), mqtt_username, mqtt_passwort, willTopic.c_str(), willQoS, willRetain, willMessage.c_str())) {

        String debug_flg_status = debug_flg ? "true" : "false";
        mqtt_client.publish((mqttname + "/parameter/debugging_active").c_str(), debug_flg_status.c_str());
        mqtt_client.subscribe((mqttname + "/parameter/debugging_active").c_str()); // debug_flg

        String debug_flg_full_log_status = debug_flg_full_log ? "true" : "false";
        mqtt_client.publish((mqttname + "/parameter/debugging_active_full").c_str(), debug_flg_full_log_status.c_str());
        mqtt_client.subscribe((mqttname + "/parameter/debugging_active_full").c_str()); // debug_flg_full_log

        mqtt_client.publish((mqttname + "/parameter/publish_delay").c_str(), String(publish_delay).c_str());
        mqtt_client.subscribe((mqttname + "/parameter/publish_delay").c_str()); // debug_flg_full_log

        mqtt_client.publish((mqttname + "/parameter/min_publish_time").c_str(), String(min_publish_time).c_str());
        mqtt_client.subscribe((mqttname + "/parameter/min_publish_time").c_str()); // min_publish_time

#ifdef USELED
        // Send LED_ON state to the LED task
        set_led(LedState::LED_FLASH);
#endif
    }

    return mqtt_client.connected();
}

// MQTT Check
void mqtt_loop() {
    if (!mqtt_client.connected()) {
        DEBUG_PRINTLN("MQTT mqtt_client not connected, attempting to reconnect...");
        unsigned long now = millis();
        if (now - lastReconnectAttempt > RECONNECT_DELAY) { // 5 seconds delay

            lastReconnectAttempt = now;
            if (reconnect()) {
                lastReconnectAttempt = 0;
                DEBUG_PRINTLN("MQTT Reconnected.");
                mqtt_client.loop();
            } else {
                DEBUG_PRINTLN("MQTT Reconnect failed.");
                // lastReconnectAttempt = 0;
            }
        }
    } else {
        mqtt_client.loop();
    }
}

void mqtt_init() {
    DEBUG_PRINTLN("Connecting MQTT ...");
    if (reconnect()) {
        DEBUG_PRINTLN("MQTT Connected.");
        // Create the task to call publishStates() every min_publish_time seconds
        xTaskCreate(publishStatesTask, "Publish States Task", 2048, NULL, 0, NULL);

        setState("version", VERSION, true);
        setState("ipaddress", WiFi.localIP().toString(), true);
        setState("ble_connection", "startup", true);
        setState("status", "online", true);

    } else
        DEBUG_PRINTLN("MQTT Connect failed.");
}