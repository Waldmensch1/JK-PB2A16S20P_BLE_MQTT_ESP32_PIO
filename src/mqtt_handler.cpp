#include "mqtt_handler.h"

constexpr long RECONNECT_DELAY = 5000;
constexpr int MQTT_BUFFER_SIZE = 500;

bool mqtt_buffer_maxed = false;

const char *mqtt_server = MQTT_SERVER;
const int mqtt_port = MQTT_PORT;
const char *mqtt_username = MQTT_USERNAME;
const char *mqtt_passwort = MQTT_PASSWORD;
const char *mqtt_devicename = DEVICENAME;
String mqttname = String("jk_ble_listener/") + mqtt_devicename;

long lastReconnectAttempt = 0;
long mqttpublishtime_offset = 1000;

bool debug_flg = false;
bool debug_flg_full_log = false;

String willTopic = String("jk_ble_listener/") + String(mqtt_devicename) + String("/status/status");
String willMessage = "offline";
byte willQoS = 0;
boolean willRetain = true;

void handleMQTTSettingsMessage(const char *topic, const char *command, byte *payload, unsigned int length, bool &flag)
{
    // Find the last occurrence of '/'
    const char *lastSlash = strrchr(topic, '/');
    if (lastSlash != nullptr)
    {
        // Move past the last '/'
        const char *lastPart = lastSlash + 1;
        // Compare the last part of the topic with the command
        if (strcmp(lastPart, command) == 0)
        {
            // Check if the payload is numeric
            bool isNumeric = true;
            for (unsigned int i = 0; i < length; i++)
            {
                if (!isdigit(payload[i]))
                {
                    isNumeric = false;
                    break;
                }
            }

            // If the payload is numeric, convert it to uint16_t and call write_setting
            if (isNumeric)
            {
                // Ensure the payload is null-terminated
                char payloadStr[length + 1];
                memcpy(payloadStr, payload, length);
                payloadStr[length] = '\0';

                uint16_t value = atoi(payloadStr);
                Serial.print("Converted value: ");
                Serial.println(value);
                write_setting(command, value);
            }
            else
            {
                mqtt_client.publish(String(topic).c_str(), String(0).c_str());
            }
        }
    }
}

void handleMQTTMessage(const char *topic, const char *command, byte *payload, unsigned int length, bool &flag, bool setting = false)
{

    if (setting)
    {
        return handleMQTTSettingsMessage(topic, command, payload, length, flag);
    }

    if (strcmp(topic, command) == 0)
    {
        String Command = String((char *)payload, length);
        flag = (Command == "true");
    }
}

// handle Subscriptions
void MQTTCallback(char *topic, byte *payload, unsigned int length)
{
    handleMQTTMessage(topic, (mqttname + "/parameter/debugging_active").c_str(), payload, length, debug_flg);
    handleMQTTMessage(topic, (mqttname + "/parameter/debugging_active_full").c_str(), payload, length, debug_flg_full_log);

    handleMQTTMessage(topic, String("publish_delay").c_str(), payload, length, debug_flg_full_log, true);
}

WiFiClient wifi_client;
PubSubClient mqtt_client(mqtt_server, mqtt_port, MQTTCallback, wifi_client);

// Reconnect to MQTT broker
boolean reconnect()
{
    mqtt_buffer_maxed = mqtt_client.setBufferSize(MQTT_BUFFER_SIZE);
    // Attempt to reconnect to the MQTT broker
    if (mqtt_client.connect(String(mqtt_devicename).c_str(), mqtt_username, mqtt_passwort, willTopic.c_str(), willQoS, willRetain, willMessage.c_str()))
    {
        // Once connected, publish an announcement...
        if (millis() < 10000)
            mqtt_client.publish((mqttname + "/status/ble_connection").c_str(), "startup");

        String debug_flg_status = debug_flg ? "true" : "false";
        mqtt_client.publish((mqttname + "/parameter/debugging_active").c_str(), debug_flg_status.c_str());
        mqtt_client.subscribe((mqttname + "/parameter/debugging_active").c_str()); // debug_flg

        String debug_flg_full_log_status = debug_flg_full_log ? "true" : "false";
        mqtt_client.publish((mqttname + "/parameter/debugging_active_full").c_str(), debug_flg_full_log_status.c_str());
        mqtt_client.subscribe((mqttname + "/parameter/debugging_active_full").c_str()); // debug_flg_full_log

        mqtt_client.publish((mqttname + "/parameter/publish_delay").c_str(), String(publish_delay).c_str());
        mqtt_client.subscribe((mqttname + "/parameter/publish_delay").c_str()); // debug_flg_full_log

        mqtt_client.publish((mqttname + "/status/status").c_str(), "online");
        mqtt_client.publish((mqttname + "/status/ipaddress").c_str(), WiFi.localIP().toString().c_str());
    }

    return mqtt_client.connected();
}

// MQTT Check
void mqtt_loop()
{
    if (!mqtt_client.connected())
    {
        Serial.println("MQTT mqtt_client not connected, attempting to reconnect...");
        long now = millis();
        if (now - lastReconnectAttempt > RECONNECT_DELAY)
        { // 5 seconds delay

            lastReconnectAttempt = now;
            if (reconnect())
            {
                lastReconnectAttempt = 0;
                Serial.println("MQTT Reconnected.");
            }
            else
            {
                Serial.println("MQTT Reconnect failed.");
                // lastReconnectAttempt = 0;
            }
        }
    }
    else
    {
        mqtt_client.loop();
    }
}

void mqtt_init()
{
    Serial.println("Connecting MQTT ...");
    if (reconnect())
        Serial.println("MQTT Connected.");
    else
        Serial.println("MQTT Connect failed.");
}