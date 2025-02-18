#include "parser.h"

#define INFLUX 1
#define MQTT 0

unsigned long lastPublishTime = 0;
bool first_run = true;

uint8_t counter_last = 0;
uint32_t cells_used = 0;

float volts_old[30][2] = {
    {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}};

float cell_avg_voltage[2] = {0.0};
float cell_diff_voltage[2] = {0.0};
byte high_voltage_cell[2] = {255, 255};
byte low_voltage_cell[2] = {255, 255};

float cellResistance_old[30][2] = {
    {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}};

float temp_mosfet[2] = {0.0};
float cell_resistance_alert[2] = {255, 255};
float battery_voltage[2] = {0, 0};
float battery_power[2] = {0, 0};
float battery_charge_current[2] = {0, 0};
float battery_power_calculated[2] = {0, 0};
float temp_sensor1[2] = {0, 0};
float temp_sensor2[2] = {0, 0};
float temp_sensor3[2] = {0, 0};
float temp_sensor4[2] = {0, 0};
float temp_sensor5[2] = {0, 0};
uint32_t alarms_mask[2] = {0xFFFFFFFF, 0xFFFFFFFF};

String alarmStrings[24] = {
    "AlarmWireRes",
    "AlarmMosOTP",
    "AlarmCellQuantity",
    "AlarmCurSensorErr",
    "AlarmCellOVP",
    "AlarmBatOVP",
    "AlarmChOCP",
    "AlarmChSCP",
    "AlarmChOTP",
    "AlarmChUTP",
    "AlarmCPUAuxCommuErr",
    "AlarmCellUVP",
    "AlarmBatUVP",
    "AlarmDchOCP",
    "AlarmDchSCP",
    "AlarmDchOTP",
    "AlarmChargeMOS",
    "AlarmDischargeMOS",
    "GPSDisconneted",
    "ModifyPWDInTime",
    "DischargeOnFailed",
    "BatteryOverTempAlarm",
    "TemperatureSensorAnomaly",
    "PLCModuleAnomaly"};

float balance_current[2] = {10000, 10000};
byte balancing_action[2] = {255, 255};
uint8_t battery_soc[2] = {0, 0};
float battery_capacity_remaining[2] = {0, 0};
float battery_capacity_total[2] = {0, 0};
uint32_t battery_cycle_count[2] = {0, 0};
float battery_cycle_capacity_total[2] = {0, 0};
byte battery_soh[2] = {0, 0};
byte battery_precharge_status[2] = {255, 255};
uint16_t battery_user_alarm1[2] = {0xFFFF, 0xFFFF};
uint16_t battery_user_alarm2[2] = {0xFFFF, 0xFFFF};
byte charging_mosfet_status[2] = {255, 255};
byte discharging_mosfet_status[2] = {255, 255};
byte precharging_status[2] = {255, 255};
uint16_t timeDcOCPR[2] = {0xFFFF, 0xFFFF};
uint16_t timeDcSCPR[2] = {0xFFFF, 0xFFFF};
uint16_t timeCOCPR[2] = {0xFFFF, 0xFFFF};
uint16_t timeCSCPR[2] = {0xFFFF, 0xFFFF};
uint16_t timeUVPR[2] = {0xFFFF, 0xFFFF};
uint16_t timeOVPR[2] = {0xFFFF, 0xFFFF};
byte temp_sensor_absent_mask = 0x00;

String temp_sensors_absent[6] = {
    "MOSTempSensorAbsent",
    "BATTempSensor1Absent",
    "BATTempSensor2Absent",
    "BATTempSensor3Absent",
    "BATTempSensor4Absent",
    "BATTempSensor5Absent"};

byte battery_heating[2] = {0xFF, 0xFF};

float heat_current[2] = {0xFFFF, 0xFFFF};
float bat_vol[2] = {0xFFFF, 0xFFFF};
float bat_vol_correct[2] = {0.0, 0.0};
float vol_discharg_cur[2] = {0xFFFF, 0xFFFF};
float vol_charg_cur[2] = {0xFFFF, 0xFFFF};
u_int32_t sys_run_ticks[2] = {0x0, 0x0};
byte charger_plugged[2] = {0xFF, 0xFF};
uint16_t bat_dis_cur_correct[2] = {0xFFFF, 0xFFFF};
uint16_t time_emergency[2] = {0xFFFF, 0xFFFF};
uint32_t rtc_ticks[2] = {0x0, 0x0};
uint32_t time_enter_sleep[2] = {0x0, 0x0};
byte pcl_module_status[2] = {0xFF, 0xFF};

// Map to store the last publish time for each topic
std::map<String, unsigned long> lastPublishTimes;

template <typename T>
void publishIfChanged(T &currentValue, T newValue, const String &topic, int decimals = -1) {

    unsigned long currentTime = millis();
    unsigned long lastPublishTime = lastPublishTimes[topic];

    // Check if the value has changed or MIN_PUBLISH_TIME is greater than 0 and the time has passed since the last publish
    if (currentValue != newValue || (min_publish_time > 0 && (currentTime - lastPublishTime) >= (static_cast<unsigned long>(min_publish_time) * 1000UL))) {
        if (debug_flg) {
            DEBUG_PRINT(topic + ": ");
            if (decimals >= 0)
                DEBUG_PRINTLN(newValue, decimals);

            else
                DEBUG_PRINTLN(newValue);
        }
        String valueStr = (decimals >= 0) ? String(newValue, decimals) : String(newValue);
        mqtt_client.publish(topic.c_str(), valueStr.c_str());
        currentValue = newValue;
        lastPublishTimes[topic] = currentTime; // Update the last publish time for the topic
    }
}

#ifdef USE_INFLUXDB
template <typename T>
void publishIfChangedInflux(T &currentValue, T newValue, const String &topic, int decimals) {
    if (currentValue != newValue) {
        DEBUG_PRINT(topic + ": ");
        if (decimals >= 0) {
            DEBUG_PRINTLN(newValue, decimals);
            publishToInfluxDB(topic, newValue);
        } else {
            DEBUG_PRINTLN(newValue);
            publishToInfluxDB(topic, newValue);
        }
        currentValue = newValue;
    }
}
#endif

String toBinaryString(uint32_t value, int bits) {
    String binaryString = "";
    for (int i = bits - 1; i >= 0; i--)
        binaryString += ((value >> i) & 1) ? '1' : '0';
    return binaryString;
}

void readDeviceDataRecord(void *message, const char *devicename) {
    uint8_t *receivedBytes_device = static_cast<uint8_t *>(message);

    String str_base_topic = mqtt_main_topic + devicename;

    DEBUG_PRINTLN("Parse DeviceDataRecord");

    size_t index = 5; // Skip the first 5 bytes
    uint8_t counter = receivedBytes_device[index++];
    // DEBUG_PRINT("counter: ");
    // DEBUG_PRINTLN(counter);

    String str_value = String(counter);
    mqtt_client.publish((str_base_topic + "/device/read_count").c_str(), str_value.c_str());

    // Read vendorID
    char vendorID[50];
    size_t vendorIDIndex = 0;
    while (index < 300 && receivedBytes_device[index] != 0x0)
        vendorID[vendorIDIndex++] = receivedBytes_device[index++];

    vendorID[vendorIDIndex] = '\0';
    // DEBUG_PRINT("vendorID: ");
    // DEBUG_PRINTLN(vendorID);

    str_value = String(vendorID);
    mqtt_client.publish((str_base_topic + "/device/vendor_id").c_str(), str_value.c_str());

    // Skip the 0x0
    while (index < 300 && receivedBytes_device[index] == 0x0)
        index++;

    // Read hardwareVersion
    char hardwareVersion[50];
    size_t hardwareVersionIndex = 0;
    while (index < 300 && receivedBytes_device[index] != 0x0)
        hardwareVersion[hardwareVersionIndex++] = receivedBytes_device[index++];

    hardwareVersion[hardwareVersionIndex] = '\0';
    // DEBUG_PRINT("hardwareVersion: ");
    // DEBUG_PRINTLN(hardwareVersion);
    str_value = String(hardwareVersion);
    mqtt_client.publish((str_base_topic + "/device/hw_revision").c_str(), str_value.c_str());

    // Skip the 0x0
    while (index < 300 && receivedBytes_device[index] == 0x0)
        index++;

    // Read softwareVersion
    char softwareVersion[50];
    size_t softwareVersionIndex = 0;
    while (index < 300 && receivedBytes_device[index] != 0x0)
        softwareVersion[softwareVersionIndex++] = receivedBytes_device[index++];

    softwareVersion[softwareVersionIndex] = '\0';
    // DEBUG_PRINT("softwareVersion: ");
    // DEBUG_PRINTLN(softwareVersion);

    str_value = String(softwareVersion);
    mqtt_client.publish((str_base_topic + "/device/sw_version").c_str(), str_value.c_str());

    // Skip the 0x0
    while (index < 300 && receivedBytes_device[index] == 0x0)
        index++;

    // Read uptime
    uint32_t uptime = 0;
    size_t uptimePos = 0;
    while (index < 300 && receivedBytes_device[index] != 0x0)
        uptime += receivedBytes_device[index++] * pow(256, uptimePos++);

    // DEBUG_PRINT("uptime: ");
    // DEBUG_PRINTLN(uptime);

    str_value = String(uptime);
    mqtt_client.publish((str_base_topic + "/device/uptime").c_str(), str_value.c_str());

    byte sec = uptime % 60;
    uptime /= 60;
    byte mi = uptime % 60;
    uptime /= 60;
    byte hr = uptime % 24;
    byte days = uptime /= 24;
    mqtt_client.publish((str_base_topic + "/device/uptime_fmt").c_str(), (String(days) + " days " + String(hr) + ":" + String(mi) + ":" + String(sec)).c_str());

    // Skip the 0x0
    while (index < 300 && receivedBytes_device[index] == 0x0)
        index++;

    // Read powerUpTimes
    uint8_t powerUpTimes = receivedBytes_device[index++];
    // DEBUG_PRINT("powerUpTimes: ");
    // DEBUG_PRINTLN(powerUpTimes);

    str_value = String(powerUpTimes);
    mqtt_client.publish((str_base_topic + "/device/power_up_times").c_str(), str_value.c_str());

    // Skip the 0x0
    while (index < 300 && receivedBytes_device[index] == 0x0)
        index++;

    // Read deviceName
    char deviceName[50];
    size_t deviceNameIndex = 0;
    while (index < 300 && receivedBytes_device[index] != 0x0)
        deviceName[deviceNameIndex++] = receivedBytes_device[index++];

    deviceName[deviceNameIndex] = '\0';
    // DEBUG_PRINT("deviceName: ");
    // DEBUG_PRINTLN(deviceName);

    str_value = String(deviceName);
    mqtt_client.publish((str_base_topic + "/device/device_name").c_str(), str_value.c_str());

    // Skip the 0x0
    while (index < 300 && receivedBytes_device[index] == 0x0)
        index++;

    // Read passCode
    char passCode[50];
    size_t passCodeIndex = 0;
    while (index < 300 && receivedBytes_device[index] != 0x0)
        passCode[passCodeIndex++] = receivedBytes_device[index++];

    passCode[passCodeIndex] = '\0';
    // DEBUG_PRINT("passCode: ");
    // DEBUG_PRINTLN(passCode);

    str_value = String(passCode);
    mqtt_client.publish((str_base_topic + "/device/device_passwd").c_str(), str_value.c_str());

    while (index < 300 && receivedBytes_device[index] == 0x0)
        index++;

    // Read manufacturingDate
    char manufacturingDate[20];
    size_t manufacturingDateIndex = 0;
    while (index < 300 && receivedBytes_device[index] != 0x0)
        manufacturingDate[manufacturingDateIndex++] = receivedBytes_device[index++];

    manufacturingDate[manufacturingDateIndex] = '\0';
    // DEBUG_PRINT("manufacturingDate: ");
    // DEBUG_PRINTLN(manufacturingDate);

    str_value = String(manufacturingDate);
    mqtt_client.publish((str_base_topic + "/device/manufacturing_date").c_str(), str_value.c_str());

    while (index < 300 && receivedBytes_device[index] == 0x0)
        index++;

    // Read serialNumber
    char serialNumber[50];
    size_t serialNumberIndex = 0;
    while (index < 300 && receivedBytes_device[index] != 0x0)
        serialNumber[serialNumberIndex++] = receivedBytes_device[index++];
    serialNumber[serialNumberIndex] = '\0';
    // DEBUG_PRINT("serialNumber: ");
    // DEBUG_PRINTLN(serialNumber);

    str_value = String(serialNumber);
    mqtt_client.publish((str_base_topic + "/device/serial_number").c_str(), str_value.c_str());

    while (index < 300 && receivedBytes_device[index] == 0x0)
        index++;

    // Read passCode2
    char passCode2[50];
    size_t passCode2Index = 0;
    while (index < 300 && receivedBytes_device[index] != 0x0)
        passCode2[passCode2Index++] = receivedBytes_device[index++];
    passCode2[passCode2Index] = '\0';
    // DEBUG_PRINT("passCode2: ");
    // DEBUG_PRINTLN(passCode2);

    str_value = String(passCode2);
    mqtt_client.publish((str_base_topic + "/device/device_passwd2").c_str(), str_value.c_str());

    while (index < 300 && receivedBytes_device[index] == 0x0)
        index++;

    // Read userData
    char userData[50];
    size_t userDataIndex = 0;
    while (index < 300 && receivedBytes_device[index] != 0x0)
        userData[userDataIndex++] = receivedBytes_device[index++];
    userData[userDataIndex] = '\0';
    // DEBUG_PRINT("userData: ");
    // DEBUG_PRINTLN(userData);

    str_value = String(userData);
    mqtt_client.publish((str_base_topic + "/device/user_data").c_str(), str_value.c_str());

    while (index < 300 && receivedBytes_device[index] == 0x0)
        index++;

    // Read setupPasscode
    char setupPasscode[50];
    size_t setupPasscodeIndex = 0;
    while (index < 300 && receivedBytes_device[index] != 0x0)
        setupPasscode[setupPasscodeIndex++] = receivedBytes_device[index++];
    setupPasscode[setupPasscodeIndex] = '\0';
    // DEBUG_PRINT("setupPasscode: ");
    // DEBUG_PRINTLN(setupPasscode);

    str_value = String(setupPasscode);
    mqtt_client.publish((str_base_topic + "/device/setup_passcode").c_str(), str_value.c_str());

    // blocked_for_parsing = false;
}

void readCellDataRecord(void *message, const char *devicename) {
    uint8_t *receivedBytes_cell = static_cast<uint8_t *>(message);

    String str_base_topic = mqtt_main_topic + devicename;

    DEBUG_PRINTLN("Parse CellDataRecord");

    unsigned long currentTime = millis();
    // mqtt_client.publish((str_base_topic + "/debug/publish_delay").c_str(), String(publish_delay).c_str());

    unsigned long timeDiff = currentTime - lastPublishTime;
    // mqtt_client.publish((str_base_topic + "/debug/timeDiff").c_str(), String(timeDiff).c_str());

    if (!first_run && (publish_delay > 0 && (currentTime - lastPublishTime) < (publish_delay * 1000))) {
        // mqtt_client.publish((str_base_topic + "/debug/indelay").c_str(), "true");
        //  Do nothing if the delay has not passed
        // blocked_for_parsing = false;
        return;
    }
    first_run = false;

    // Update the last publish time
    lastPublishTime = currentTime;

    size_t index = 5; // Skip the first 5 bytes
    uint8_t counter = receivedBytes_cell[index++];
    if (counter == counter_last)
        return;
    else
        counter_last = counter;

    // DEBUG_PRINT("counter: ");
    // DEBUG_PRINTLN(counter);

    String str_topic;
    String str_value;

    str_topic = String(str_base_topic + "/data/readcount");
    str_value = String(counter);
    mqtt_client.publish(str_topic.c_str(), str_value.c_str());

    if (debug_flg_full_log) {
        uint16_t inputLength = sizeof(receivedBytes_cell);
        char output[base64::encodeLength(inputLength)];
        base64::encode(receivedBytes_cell, inputLength, output);
        str_topic = String(str_base_topic + "/debug/rawdata");
        str_value = String(output);
        mqtt_client.publish(str_topic.c_str(), str_value.c_str());
        mqtt_client.publish((str_base_topic + "/debug/enabled").c_str(), "true");
    } else {
        if (debug_flg) {
            str_topic = String(str_base_topic + "/debug/rawdata");
            str_value = "not published";
            mqtt_client.publish(str_topic.c_str(), str_value.c_str());
            mqtt_client.publish((str_base_topic + "/debug/enabled").c_str(), "false");
        }
    }

    // Read cell voltages
    float volts[30];
    for (int i = 0; i < 30; i++) {
        uint16_t volt = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8);
        volts[i] = volt * 0.001;
    }

    // DEBUG_PRINT("volts: ");
    str_topic = String(str_base_topic + "/data/cells/voltage/cell_v_");
    for (uint8_t i = 0; i < 30; i++) {
        // DEBUG_PRINT(volts[i]);
        // DEBUG_PRINT(" ");
        if (volts[i] != volts_old[i][MQTT]) {
            str_value = String(volts[i], 3);
            String topic;
            if (i < 9)
                topic = str_topic + String("0") + String(i + 1);
            else
                topic = str_topic + String(i + 1);
            if (volts[i] != 0) {
                mqtt_client.publish(topic.c_str(), str_value.c_str());
#ifdef INFLUX_CELLS_VOLTAGE
                publishToInfluxDB("cell_" + String(i + 1), volts[i]);
#endif
            }
        }
        volts_old[i][MQTT] = volts[i];
    }
    // DEBUG_PRINTLN();

    // ignore 4 bytes
    index += 4;

    // read the mask
    uint32_t uint32_t_value = (receivedBytes_cell[index++] << 24) |
                              (receivedBytes_cell[index++] << 16) |
                              (receivedBytes_cell[index++] << 8) |
                              (receivedBytes_cell[index++]);

    if (debug_flg) {
        if (uint32_t_value != cells_used || cells_used == 0) {
            DEBUG_PRINT("mask: ");
            String cellsUsedMaskStr = toBinaryString(uint32_t_value, 32);
            DEBUG_PRINTLN(cellsUsedMaskStr);

            str_topic = String(str_base_topic + "/data/cells_used");
            mqtt_client.publish(str_topic.c_str(), cellsUsedMaskStr.c_str());
            cells_used = uint32_t_value;
        }
    }

    // Read cell average voltage
    float fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8) * 0.001;
    publishIfChanged(cell_avg_voltage[MQTT], fl_value, str_base_topic + "/data/cells/voltage/cell_avg_voltage", 3);

    // Read cell voltage difference
    fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8) * 0.001;
    publishIfChanged(cell_diff_voltage[MQTT], fl_value, str_base_topic + "/data/cells/voltage/cell_diff_voltage", 3);

    // high_voltage_cell
    byte byte_value = receivedBytes_cell[index++];
    publishIfChanged(high_voltage_cell[MQTT], byte_value, str_base_topic + "/data/cells/voltage/high_voltage_cell");

    // low_voltage_cell
    byte_value = receivedBytes_cell[index++];
    publishIfChanged(low_voltage_cell[MQTT], byte_value, str_base_topic + "/data/cells/voltage/low_voltage_cell");

    // Read cell resistances
    float cellResistance[30];
    for (int i = 0; i < 30; i++) {
        uint16_t resistance = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8);
        cellResistance[i] = resistance * 0.001;
    }

    DEBUG_PRINT("cellResistance: ");

    str_topic = String(str_base_topic + "/data/cells/resistance/cell_r_");
    for (uint8_t i = 0; i < 30; i++) {
        // DEBUG_PRINT(cellResistance[i]);
        // DEBUG_PRINT(" ");

        if (cellResistance[i] != cellResistance_old[i][MQTT] || cellResistance_old[i][MQTT] == 0) {

            str_value = String(cellResistance[i], 3);
            String topic;
            if (i < 9)
                topic = str_topic + String("0") + String(i + 1);
            else
                topic = str_topic + String(i + 1);
            if (cellResistance[i] != 0) {
                mqtt_client.publish(topic.c_str(), str_value.c_str());
            }
            cellResistance_old[i][MQTT] = cellResistance[i];
        }
    }
    // DEBUG_PRINTLN();

    // ignore 4 bytes
    index += 4;

    // temp_mosfet
    fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8) * 0.1;
    publishIfChanged(temp_mosfet[MQTT], fl_value, str_base_topic + "/data/temperatures/temp_mosfet");
#ifdef INFLUX_TEMP_SENSOR_MOSFET
    publishIfChangedInflux(temp_mosfet[INFLUX], fl_value, "temp_mosfet");
#endif

    // read the mask
    uint32_t_value = (receivedBytes_cell[index++] << 24) |
                     (receivedBytes_cell[index++] << 16) |
                     (receivedBytes_cell[index++] << 8) |
                     (receivedBytes_cell[index++]);

    if (debug_flg) {
        if (uint32_t_value != cell_resistance_alert[MQTT] || cell_resistance_alert[MQTT] == 255) {
            DEBUG_PRINT("resistanceAlertMask: ");
            String resistanceAlertMaskStr = toBinaryString(uint32_t_value, 32);
            DEBUG_PRINTLN(resistanceAlertMaskStr);
            str_topic = String(str_base_topic + "/data/cell_resistance_alert");
            mqtt_client.publish(str_topic.c_str(), resistanceAlertMaskStr.c_str());
            cell_resistance_alert[MQTT] = uint32_t_value;
        }
    }

    // battery_voltage
    fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8 | receivedBytes_cell[index++] << 16 | receivedBytes_cell[index++] << 24) * 0.001;
    publishIfChanged(battery_voltage[MQTT], fl_value, str_base_topic + "/data/battery_voltage");
#ifdef INFLUX_BATTERY_VOLTAGE
    publishIfChangedInflux(battery_voltage[INFLUX], fl_value, "battery_voltage");
#endif

    // battery_power
    fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8 | receivedBytes_cell[index++] << 16 | receivedBytes_cell[index++] << 24) * 0.001;
    publishIfChanged(battery_power[MQTT], fl_value, str_base_topic + "/data/battery_power");

    // battery_charge_current
    fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8 | receivedBytes_cell[index++] << 16 | receivedBytes_cell[index++] << 24) * 0.001;
    publishIfChanged(battery_charge_current[MQTT], fl_value, str_base_topic + "/data/battery_charge_current");
#ifdef INFLUX_BATTERY_CURRENT
    publishIfChangedInflux(battery_charge_current[INFLUX], fl_value, "battery_current");
#endif

    if (battery_charge_current[MQTT] < 0)
        fl_value = 0 - battery_power[MQTT];
    else
        fl_value = battery_power[MQTT];

    // this is to have the correct sign for the power (discharging "-" or charging)
    publishIfChanged(battery_power_calculated[MQTT], fl_value, str_base_topic + "/data/battery_power_calculated");
#ifdef INFLUX_BATTERY_POWER
    publishIfChangedInflux(battery_power_calculated[INFLUX], fl_value, "battery_power");
#endif

    // temp_sensor1
    fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8) * 0.1;
    publishIfChanged(temp_sensor1[MQTT], fl_value, str_base_topic + "/data/temperatures/temp_sensor1");
#ifdef INFLUX_TEMP_SENSOR_1
    publishIfChangedInflux(temp_sensor1[INFLUX], fl_value, "temp_sensor1");
#endif

    // temp_sensor2
    fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8) * 0.1;
    publishIfChanged(temp_sensor2[MQTT], fl_value, str_base_topic + "/data/temperatures/temp_sensor2");
#ifdef INFLUX_TEMP_SENSOR_2
    publishIfChangedInflux(temp_sensor2[INFLUX], fl_value, "temp_sensor2");
#endif

    uint32_t_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8 | receivedBytes_cell[index++] << 16 | receivedBytes_cell[index++] << 24);
    if (uint32_t_value != alarms_mask[MQTT] || alarms_mask[MQTT] == 0xFFFFFFFF) {

        if (debug_flg) {
            DEBUG_PRINT("errorsMask: ");
            String errorsMaskStr = toBinaryString(uint32_t_value, 32);
            DEBUG_PRINTLN(errorsMaskStr);
            str_topic = String(str_base_topic + "/data/alarms/alarms_mask");
            mqtt_client.publish(str_topic.c_str(), errorsMaskStr.c_str());
        }

        // resolve alarms according to 极空BMS RS485 Modbus通用协议V1.1 2024.02 Page 10

        // AlarmWireRes; 1：ON; 0：OFF;  BIT0
        // AlarmMosOTP; 1：ON; 0：OFF;  BIT1
        // AlarmCellQuantity; 1：ON; 0：OFF;  BIT2
        // AlarmCurSensorErr; 1：ON; 0：OFF;  BIT3
        // AlarmCellOVP; 1：ON; 0：OFF;  BIT4
        // AlarmBatOVP; 1：ON; 0：OFF;  BIT5
        // AlarmChOCP; 1：ON; 0：OFF;  BIT6
        // AlarmChSCP; 1：ON; 0：OFF;  BIT7
        // AlarmChOTP; 1：ON; 0：OFF;  BIT8
        // AlarmChUTP; 1：ON; 0：OFF;  BIT9
        // AlarmCPUAuxCommuErr; 1：ON; 0：OFF;  BIT10
        // AlarmCellUVP; 1：ON; 0：OFF;  BIT11
        // AlarmBatUVP; 1：ON; 0：OFF;  BIT12
        // AlarmDchOCP; 1：ON; 0：OFF;  BIT13
        // AlarmDchSCP; 1：ON; 0：OFF;  BIT14
        // AlarmDchOTP; 1：ON; 0：OFF;  BIT15
        // AlarmChargeMOS; 1：ON; 0：OFF;  BIT16
        // AlarmDischargeMOS; 1：ON; 0：OFF;  BIT17
        // GPSDisconneted; 1：ON; 0：OFF;  BIT18
        // Modify PWD. in time; 1：ON; 0：OFF;  BIT19
        // Discharge On Failed; 1：ON; 0：OFF;  BIT20
        // Battery Over Temp Alarm; 1：ON; 0：OFF;  BIT21
        // Temperature sensor anomaly; 1：ON; 0：OFF;  BIT22
        // PLCModule anomaly; 1：ON; 0：OFF;  BIT23

        for (int i = 0; i < 24; ++i) {
            String status = (uint32_t_value & (1 << i)) ? "ON" : "OFF";
            str_topic = str_base_topic + "/data/alarms/" + alarmStrings[i];
            mqtt_client.publish(str_topic.c_str(), status.c_str());
        }

        alarms_mask[MQTT] = uint32_t_value;
    }

    fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8) * 0.1;
    publishIfChanged(balance_current[MQTT], fl_value, str_base_topic + "/data/balance_current");

    byte_value = receivedBytes_cell[index++];
    publishIfChanged(balancing_action[MQTT], byte_value, str_base_topic + "/data/balancing_action");

    uint8_t uint8_t_value = receivedBytes_cell[index++];
    publishIfChanged(battery_soc[MQTT], uint8_t_value, str_base_topic + "/data/battery_soc");
#ifdef INFLUX_BATTERY_SOC
    publishIfChangedInflux(battery_soc[INFLUX], uint8_t_value, "battery_soc");
#endif

    fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8 | receivedBytes_cell[index++] << 16 | receivedBytes_cell[index++] << 24) * 0.001;
    publishIfChanged(battery_capacity_remaining[MQTT], fl_value, str_base_topic + "/data/battery_capacity_remaining");

    fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8 | receivedBytes_cell[index++] << 16 | receivedBytes_cell[index++] << 24) * 0.001;
    publishIfChanged(battery_capacity_total[MQTT], fl_value, str_base_topic + "/data/battery_capacity_total");

    uint32_t_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8 | receivedBytes_cell[index++] << 16 | receivedBytes_cell[index++] << 24);
    publishIfChanged(battery_cycle_count[MQTT], uint32_t_value, str_base_topic + "/data/battery_cycle_count");

    fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8 | receivedBytes_cell[index++] << 16 | receivedBytes_cell[index++] << 24) * 0.001;
    publishIfChanged(battery_cycle_capacity_total[MQTT], fl_value, str_base_topic + "/data/battery_cycle_capacity_total");

    byte_value = receivedBytes_cell[index++];
    publishIfChanged(battery_soh[MQTT], byte_value, str_base_topic + "/data/battery_soh");

    byte_value = receivedBytes_cell[index++];
    publishIfChanged(battery_precharge_status[MQTT], byte_value, str_base_topic + "/data/battery_precharge_status");

    uint16_t uint16_t_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8);
    publishIfChanged(battery_user_alarm1[MQTT], uint16_t_value, str_base_topic + "/data/battery_user_alarm1");

    uint32_t_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8 | receivedBytes_cell[index++] << 16 | receivedBytes_cell[index++] << 24);
    // DEBUG_PRINT("totalRuntime: ");
    // DEBUG_PRINTLN(uint32_t_value);
    str_value = String(uint32_t_value);
    str_topic = String(str_base_topic + "/data/battery_total_runtime");
    mqtt_client.publish(str_topic.c_str(), str_value.c_str());

    byte sec = uint32_t_value % 60;
    uint32_t_value /= 60;
    byte mi = uint32_t_value % 60;
    uint32_t_value /= 60;
    byte hr = uint32_t_value % 24;
    byte days = uint32_t_value /= 24;
    // DEBUG_PRINTLN(String(days) + " days " + String(hr) + ":" + String(mi) + ":" + String(sec));
    str_topic = String(str_base_topic + "/data/battery_total_runtime_fmt");
    mqtt_client.publish(str_topic.c_str(), (String(days) + " days " + String(hr) + ":" + String(mi) + ":" + String(sec)).c_str());

    // charging_mosfet_status
    byte_value = receivedBytes_cell[index++];
    publishIfChanged(charging_mosfet_status[MQTT], byte_value, str_base_topic + "/data/charging_mosfet_status");

    // discharging_mosfet_status
    byte_value = receivedBytes_cell[index++];
    publishIfChanged(discharging_mosfet_status[MQTT], byte_value, str_base_topic + "/data/discharging_mosfet_status");

    // battery_user_alarm2
    uint16_t_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8);
    publishIfChanged(battery_user_alarm2[MQTT], uint16_t_value, str_base_topic + "/data/battery_user_alarm2");

    // timeDcOCPR
    uint16_t_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8);
    publishIfChanged(timeDcOCPR[MQTT], uint16_t_value, str_base_topic + "/data/alarms/timeDcOCPR");

    // timeDcSCPR
    uint16_t_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8);
    publishIfChanged(timeDcSCPR[MQTT], uint16_t_value, str_base_topic + "/data/alarms/timeDcSCPR");

    // timeCOCPR
    uint16_t_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8);
    publishIfChanged(timeCOCPR[MQTT], uint16_t_value, str_base_topic + "/data/alarms/timeCOCPR");

    // timeCSCPR
    uint16_t_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8);
    publishIfChanged(timeCSCPR[MQTT], uint16_t_value, str_base_topic + "/data/alarms/timeCSCPR");

    // timeUVPR
    uint16_t_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8);
    publishIfChanged(timeUVPR[MQTT], uint16_t_value, str_base_topic + "/data/alarms/timeUVPR");

    // timeOVPR
    uint16_t_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8);
    publishIfChanged(timeOVPR[MQTT], uint16_t_value, str_base_topic + "/data/alarms/timeOVPR");

    byte_value = (receivedBytes_cell[index++]);
    if (byte_value != temp_sensor_absent_mask || temp_sensor_absent_mask == 0x00) {
        if (debug_flg) {
            DEBUG_PRINT("temp_sensor_absent_mask: ");
            DEBUG_PRINTLN(byte_value);
            String temp_sensor_absent_mask_str = toBinaryString(byte_value, 8);
            str_value = String(temp_sensor_absent_mask_str);
            str_topic = String(str_base_topic + "/data/temperatures/temp_sensor_absent_mask");
            mqtt_client.publish(str_topic.c_str(), str_value.c_str());
        }

        // resolve temp sensor absent mask according to 极空BMS RS485 Modbus通用协议V1.1 2024.02 Page 11
        // MOS TempSensorAbsent; 1：Normal; 0：Missing; BIT0
        // BATTempSensor1Absent; 1：Normal; 0：Missing; BIT1
        // BATTempSensor2Absent; 1：Normal; 0：Missing; BIT2
        // BATTempSensor3Absent; 1：Normal; 0：Missing; BIT3
        // BATTempSensor4Absent; 1：Normal; 0：Missing; BIT4
        // BATTempSensor5Absent; 1：Normal; 0：Missing; BIT5

        for (int i = 0; i < 6; ++i) {
            String status = (byte_value & (1 << i)) ? "Normal" : "Missing";
            String str_topic = str_base_topic + "/data/temperatures/" + temp_sensors_absent[i];
            mqtt_client.publish(str_topic.c_str(), status.c_str());
        }

        temp_sensor_absent_mask = byte_value;
    }

    // battery_heating
    byte_value = (receivedBytes_cell[index++]);
    publishIfChanged(battery_heating[MQTT], byte_value, str_base_topic + "/data/temperatures/battery_heating");

    // send current index to debug topic
    if (debug_flg) {
        str_value = String(index);
        str_topic = String(str_base_topic + "/debug/last_index_1");
        mqtt_client.publish(str_topic.c_str(), str_value.c_str());
    }

    // index 216
    index += 1; // skip 1 reserved byte

    // index 217
    // suspect: time_emergency
    uint16_t_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8);
    if (debug_flg) {
        DEBUG_PRINT("time_emergency: ");
        DEBUG_PRINTLN(uint16_t_value);
        publishIfChanged(time_emergency[MQTT], uint16_t_value, str_base_topic + "/data/sus/time_emergency");
    }

    // index 219
    // suspect:  Discharge current correction factor
    uint16_t_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8);
    if (debug_flg) {
        DEBUG_PRINT("bat_dis_cur_correct: ");
        DEBUG_PRINTLN(uint16_t_value);
        publishIfChanged(bat_dis_cur_correct[MQTT], uint16_t_value, str_base_topic + "/data/sus/bat_dis_cur_correct");
    }

    // index 221
    // suspect: Charging current sensor voltage
    fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8) * 0.001;
    if (debug_flg) {
        DEBUG_PRINT("vol_charg_cur: ");
        DEBUG_PRINTLN(fl_value);
        publishIfChanged(vol_charg_cur[MQTT], fl_value, str_base_topic + "/data/sus/vol_charg_cur");
    }

    // index 223
    // suspect: Discharge current sensor voltage.
    fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8) * 0.001;
    if (debug_flg) {
        DEBUG_PRINT("vol_discharg_cur: ");
        DEBUG_PRINTLN(fl_value);
        publishIfChanged(vol_discharg_cur[MQTT], fl_value, str_base_topic + "/data/sus/vol_discharg_cur");
    }

    // index 225
    // suspect: Battery voltage correction factor
    fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8 | receivedBytes_cell[index++] << 16 | receivedBytes_cell[index++] << 24) * 0.001;
    if (debug_flg) {
        DEBUG_PRINT("bat_vol_correct: ");
        DEBUG_PRINTLN(fl_value);
        publishIfChanged(bat_vol_correct[MQTT], fl_value, str_base_topic + "/data/sus/bat_vol_correct");
    }

    // index 229
    index += 4; // skip 4 reserved bytes

    // index 233
    // suspect: Battery voltage
    fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8) * 0.01;
    if (debug_flg) {
        DEBUG_PRINT("bat_vol: ");
        DEBUG_PRINTLN(fl_value);
        publishIfChanged(bat_vol[MQTT], fl_value, str_base_topic + "/data/sus/bat_vol");
    }

    // index 235
    // suspect: Heating current
    fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8) * 0.001;
    if (debug_flg) {
        DEBUG_PRINT("heat_current: ");
        DEBUG_PRINTLN(fl_value);
        publishIfChanged(heat_current[MQTT], fl_value, str_base_topic + "/data/sus/heat_current");
    }

    // index 237
    //  skip 7 sus bytes
    index += 8;

    // index 245
    byte_value = receivedBytes_cell[index++];
    if (debug_flg) {
        DEBUG_PRINT("charger_plugged: ");
        DEBUG_PRINTLN(byte_value);
        publishIfChanged(charger_plugged[MQTT], byte_value, str_base_topic + "/data/sus/charger_plugged");
    }

    // index 246
    // 0x00F0 240 UINT32 4 R 系统节拍SysRunTicks 0.1S
    uint32_t_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8 | receivedBytes_cell[index++] << 16 | receivedBytes_cell[index++] << 24);
    if (debug_flg) {
        DEBUG_PRINT("sys_run_ticks: ");
        DEBUG_PRINTLN(uint32_t_value);
        publishIfChanged(sys_run_ticks[MQTT], uint32_t_value, str_base_topic + "/data/sus/sys_run_ticks");
    }

    // index 250
    index += 2; // skip 2 sus bytes

    // index 252
    index += 2; // skip 2 sus bytes

    // index 254
    // send current index to debug topic
    if (debug_flg) {
        str_value = String(index);
        str_topic = String(str_base_topic + "/debug/last_index_2");
        mqtt_client.publish(str_topic.c_str(), str_value.c_str());
    }

    // temp_sensor3
    fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8) * 0.1;
    publishIfChanged(temp_sensor3[MQTT], fl_value, str_base_topic + "/data/temperatures/temp_sensor3");
#ifdef INFLUX_TEMP_SENSOR_3
    publishIfChangedInflux(temp_sensor3[INFLUX], fl_value, "temp_sensor3");
#endif

    // index 256
    // temp_sensor4
    fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8) * 0.1;
    publishIfChanged(temp_sensor4[MQTT], fl_value, str_base_topic + "/data/temperatures/temp_sensor4");
#ifdef INFLUX_TEMP_SENSOR_4
    publishIfChangedInflux(temp_sensor4[INFLUX], fl_value, "temp_sensor4");
#endif

    // index 258
    // temp_sensor5
    fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8) * 0.1;
    publishIfChanged(temp_sensor5[MQTT], fl_value, str_base_topic + "/data/temperatures/temp_sensor5");
#ifdef INFLUX_TEMP_SENSOR_5
    publishIfChangedInflux(temp_sensor5[INFLUX], fl_value, "temp_sensor5");
#endif

    // index 260
    index += 2; // skip 2 sus bytes

    // index 262
    // suspect: rtc_ticks
    uint32_t_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8 | receivedBytes_cell[index++] << 16 | receivedBytes_cell[index++] << 24);
    if (debug_flg) {
        DEBUG_PRINT("rtc_ticks: ");
        DEBUG_PRINTLN(uint32_t_value);
        publishIfChanged(rtc_ticks[MQTT], uint32_t_value, str_base_topic + "/data/sus/rtc_ticks");
    }

    // index 266
    index += 4; // skip 4 sus bytes

    // index 270
    // suspect: time_enter_sleep
    uint32_t_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8 | receivedBytes_cell[index++] << 16 | receivedBytes_cell[index++] << 24);
    if (debug_flg) {
        DEBUG_PRINT("time_enter_sleep: ");
        DEBUG_PRINTLN(uint32_t_value);
        publishIfChanged(time_enter_sleep[MQTT], uint32_t_value, str_base_topic + "/data/sus/time_enter_sleep");
    }

    // index 274
    // suspect: PCLModuleSta Parallel current limiting module status 1: on; 0: off
    byte_value = receivedBytes_cell[index++];
    if (debug_flg) {
        DEBUG_PRINT("pcl_module_status: ");
        DEBUG_PRINTLN(byte_value);
        publishIfChanged(pcl_module_status[MQTT], byte_value, str_base_topic + "/data/sus/pcl_module_status");
    }

    // index 275
    DEBUG_PRINT("Index: ");
    DEBUG_PRINTLN(index);
}

void readConfigDataRecord(void *message, const char *devicename) {
    uint8_t *data = static_cast<uint8_t *>(message);
    DEBUG_PRINTLN("readConfigDataRecord");

    String str_base_topic = mqtt_main_topic + devicename;
    String str_topic;
    String str_value;

    str_topic = str_base_topic + "/config/device_id";
    str_value = String(1);
    // DEBUG_PRINTLN(str_topic + ": " + str_value);

    size_t index = 4; // Skip the first 5 bytes
    uint8_t uint8_t_value = data[index++];
    // DEBUG_PRINT("message_type: ");
    // DEBUG_PRINTLN(uint8_t_value);

    uint8_t_value = data[index++];
    // DEBUG_PRINT("count: ");
    // DEBUG_PRINTLN(uint8_t_value);

    float float_value = (data[index++] | data[index++] << 8 | data[index++] << 16 | data[index++] << 24) * 0.001;
    // DEBUG_PRINT(str_base_topic + "/config/vol_smart_sleep: ");
    // DEBUG_PRINTLN(float_value);
    str_topic = str_base_topic + "/config/vol_smart_sleep";
    str_value = String(float_value, 3);
    mqtt_client.publish(str_topic.c_str(), str_value.c_str());

    float_value = (data[index++] | data[index++] << 8 | data[index++] << 16 | data[index++] << 24) * 0.001;
    // DEBUG_PRINT(str_base_topic + "/config/vol_cell_uv: ");
    // DEBUG_PRINTLN(float_value);
    str_topic = str_base_topic + "/config/vol_cell_uv";
    str_value = String(float_value, 3);
    mqtt_client.publish(str_topic.c_str(), str_value.c_str());

    float_value = (data[index++] | data[index++] << 8 | data[index++] << 16 | data[index++] << 24) * 0.001;
    // DEBUG_PRINT(str_base_topic + "/config/vol_cell_uvpr: ");
    // DEBUG_PRINTLN(float_value);
    str_topic = str_base_topic + "/config/vol_cell_uvpr";
    str_value = String(float_value, 3);
    mqtt_client.publish(str_topic.c_str(), str_value.c_str());

    float_value = (data[index++] | data[index++] << 8 | data[index++] << 16 | data[index++] << 24) * 0.001;
    // DEBUG_PRINT(str_base_topic + "/config/vol_cell_ov: ");
    // DEBUG_PRINTLN(float_value);
    str_topic = str_base_topic + "/config/vol_cell_ov";
    str_value = String(float_value, 3);
    mqtt_client.publish(str_topic.c_str(), str_value.c_str());

    float_value = (data[index++] | data[index++] << 8 | data[index++] << 16 | data[index++] << 24) * 0.001;
    // DEBUG_PRINT(str_base_topic + "/config/vol_cell_ovpr: ");
    // DEBUG_PRINTLN(float_value);
    str_topic = str_base_topic + "/config/vol_cell_ovpr";
    str_value = String(float_value, 3);
    mqtt_client.publish(str_topic.c_str(), str_value.c_str());

    float_value = (data[index++] | data[index++] << 8 | data[index++] << 16 | data[index++] << 24) * 0.001;
    // DEBUG_PRINT(str_base_topic + "/config/vol_balan_trig: ");
    // DEBUG_PRINTLN(float_value);
    str_topic = str_base_topic + "/config/vol_balan_trig";
    str_value = String(float_value, 3);
    mqtt_client.publish(str_topic.c_str(), str_value.c_str());

    float_value = (data[index++] | data[index++] << 8 | data[index++] << 16 | data[index++] << 24) * 0.001;
    // DEBUG_PRINT(str_base_topic + "/config/vol_100_percent: ");
    // DEBUG_PRINTLN(float_value);
    str_topic = str_base_topic + "/config/vol_100_percent";
    str_value = String(float_value, 3);
    mqtt_client.publish(str_topic.c_str(), str_value.c_str());

    float_value = (data[index++] | data[index++] << 8 | data[index++] << 16 | data[index++] << 24) * 0.001;
    // DEBUG_PRINT(str_base_topic + "/config/vol_0_percent: ");
    // DEBUG_PRINTLN(float_value);
    str_topic = str_base_topic + "/config/vol_0_percent";
    str_value = String(float_value, 3);
    mqtt_client.publish(str_topic.c_str(), str_value.c_str());

    float_value = (data[index++] | data[index++] << 8 | data[index++] << 16 | data[index++] << 24) * 0.001;
    // DEBUG_PRINT(str_base_topic + "/config/vol_cell_rcv: ");
    // DEBUG_PRINTLN(float_value);
    str_topic = str_base_topic + "/config/vol_cell_rcv";
    str_value = String(float_value, 3);
    mqtt_client.publish(str_topic.c_str(), str_value.c_str());

    float_value = (data[index++] | data[index++] << 8 | data[index++] << 16 | data[index++] << 24) * 0.001;
    // DEBUG_PRINT(str_base_topic + "/config/vol_cell_rfv: ");
    // DEBUG_PRINTLN(float_value);
    str_topic = str_base_topic + "/config/vol_cell_rfv";
    str_value = String(float_value, 3);
    mqtt_client.publish(str_topic.c_str(), str_value.c_str());

    float_value = (data[index++] | data[index++] << 8 | data[index++] << 16 | data[index++] << 24) * 0.001;
    // DEBUG_PRINT(str_base_topic + "/config/vol_sys_pwr_off: ");
    // DEBUG_PRINTLN(float_value);
    str_topic = str_base_topic + "/config/vol_sys_pwr_off";
    str_value = String(float_value, 3);
    mqtt_client.publish(str_topic.c_str(), str_value.c_str());

    float_value = (data[index++] | data[index++] << 8 | data[index++] << 16 | data[index++] << 24) * 0.001;
    // DEBUG_PRINT(str_base_topic + "/config/cur_bat_coc: ");
    // DEBUG_PRINTLN(float_value);
    str_topic = str_base_topic + "/config/cur_bat_coc";
    str_value = String(float_value, 3);
    mqtt_client.publish(str_topic.c_str(), str_value.c_str());

    uint32_t uint32_t_value = (data[index++] | data[index++] << 8 | data[index++] << 16 | data[index++] << 24) * 0.001;
    // DEBUG_PRINT(str_base_topic + "/config/time_bat_cocp_delay: ");
    // DEBUG_PRINTLN(uint32_t_value);
    str_topic = str_base_topic + "/config/time_bat_cocp_delay";
    str_value = String(uint32_t_value);
    mqtt_client.publish(str_topic.c_str(), str_value.c_str());

    uint32_t_value = (data[index++] | data[index++] << 8 | data[index++] << 16 | data[index++] << 24) * 0.001;
    // DEBUG_PRINT(str_base_topic + "/config/time_bat_cocprd_delay: ");
    // DEBUG_PRINTLN(uint32_t_value);
    str_topic = str_base_topic + "/config/time_bat_cocprd_delay";
    str_value = String(uint32_t_value);
    mqtt_client.publish(str_topic.c_str(), str_value.c_str());

    float_value = (data[index++] | data[index++] << 8 | data[index++] << 16 | data[index++] << 24) * 0.001;
    // DEBUG_PRINT(str_base_topic + "/config/cur_bat_dc_oc: ");
    // DEBUG_PRINTLN(float_value);
    str_topic = str_base_topic + "/config/cur_bat_dc_oc";
    str_value = String(float_value, 3);
    mqtt_client.publish(str_topic.c_str(), str_value.c_str());

    uint32_t_value = (data[index++] | data[index++] << 8 | data[index++] << 16 | data[index++] << 24) * 0.001;
    // DEBUG_PRINT(str_base_topic + "/config/time_bat_dc_ocp_delay: ");
    // DEBUG_PRINTLN(uint32_t_value);
    str_topic = str_base_topic + "/config/time_bat_dc_ocp_delay";
    str_value = String(uint32_t_value);
    mqtt_client.publish(str_topic.c_str(), str_value.c_str());

    uint32_t_value = (data[index++] | data[index++] << 8 | data[index++] << 16 | data[index++] << 24) * 0.001;
    // DEBUG_PRINT(str_base_topic + "/config/time_bat_dc_oprd_delay: ");
    // DEBUG_PRINTLN(uint32_t_value);
    str_topic = str_base_topic + "/config/time_bat_dc_oprd_delay";
    str_value = String(uint32_t_value);
    mqtt_client.publish(str_topic.c_str(), str_value.c_str());

    uint32_t_value = (data[index++] | data[index++] << 8 | data[index++] << 16 | data[index++] << 24) * 0.001;
    // DEBUG_PRINT(str_base_topic + "/config/time_bat_scpr_delay: ");
    // DEBUG_PRINTLN(uint32_t_value);
    str_topic = str_base_topic + "/config/time_bat_scpr_delay";
    str_value = String(uint32_t_value);
    mqtt_client.publish(str_topic.c_str(), str_value.c_str());

    float_value = (data[index++] | data[index++] << 8 | data[index++] << 16 | data[index++] << 24) * 0.001;
    // DEBUG_PRINT(str_base_topic + "/config/cur_balance_max: ");
    // DEBUG_PRINTLN(float_value);
    str_topic = str_base_topic + "/config/cur_balance_max";
    str_value = String(float_value, 3);
    mqtt_client.publish(str_topic.c_str(), str_value.c_str());

    float_value = (data[index++] | data[index++] << 8 | data[index++] << 16 | data[index++] << 24) * 0.1;
    // DEBUG_PRINT(str_base_topic + "/config/tmp_bat_cot: ");
    // DEBUG_PRINTLN(float_value);
    str_topic = str_base_topic + "/config/tmp_bat_cot";
    str_value = String(float_value);
    mqtt_client.publish(str_topic.c_str(), str_value.c_str());

    float_value = (data[index++] | data[index++] << 8 | data[index++] << 16 | data[index++] << 24) * 0.1;
    // DEBUG_PRINT(str_base_topic + "/config/tmp_bat_cotpr: ");
    // DEBUG_PRINTLN(float_value);
    str_topic = str_base_topic + "/config/tmp_bat_cotpr";
    str_value = String(float_value);
    mqtt_client.publish(str_topic.c_str(), str_value.c_str());

    float_value = (data[index++] | data[index++] << 8 | data[index++] << 16 | data[index++] << 24) * 0.1;
    // DEBUG_PRINT(str_base_topic + "/config/tmp_bat_dc_ot: ");
    // DEBUG_PRINTLN(float_value);
    str_topic = str_base_topic + "/config/tmp_bat_dc_ot";
    str_value = String(float_value);
    mqtt_client.publish(str_topic.c_str(), str_value.c_str());

    float_value = (data[index++] | data[index++] << 8 | data[index++] << 16 | data[index++] << 24) * 0.1;
    // DEBUG_PRINT(str_base_topic + "/config/tmp_bat_dc_otpr: ");
    // DEBUG_PRINTLN(float_value);
    str_topic = str_base_topic + "/config/tmp_bat_dc_otpr";
    str_value = String(float_value);
    mqtt_client.publish(str_topic.c_str(), str_value.c_str());

    float_value = (data[index++] | data[index++] << 8 | data[index++] << 16 | data[index++] << 24) * 0.1;
    // DEBUG_PRINT(str_base_topic + "/config/tmp_bat_cut: ");
    // DEBUG_PRINTLN(float_value);
    str_topic = str_base_topic + "/config/tmp_bat_cut";
    str_value = String(float_value);
    mqtt_client.publish(str_topic.c_str(), str_value.c_str());

    float_value = (data[index++] | data[index++] << 8 | data[index++] << 16 | data[index++] << 24) * 0.1;
    // DEBUG_PRINT(str_base_topic + "/config/tmp_bat_cutpr: ");
    // DEBUG_PRINTLN(float_value);
    str_topic = str_base_topic + "/config/tmp_bat_cutpr";
    str_value = String(float_value);
    mqtt_client.publish(str_topic.c_str(), str_value.c_str());

    float_value = (data[index++] | data[index++] << 8 | data[index++] << 16 | data[index++] << 24) * 0.1;
    // DEBUG_PRINT(str_base_topic + "/config/tmp_mos_ot: ");
    // DEBUG_PRINTLN(float_value);
    str_topic = str_base_topic + "/config/tmp_mos_ot";
    str_value = String(float_value);
    mqtt_client.publish(str_topic.c_str(), str_value.c_str());

    float_value = (data[index++] | data[index++] << 8 | data[index++] << 16 | data[index++] << 24) * 0.1;
    // DEBUG_PRINT(str_base_topic + "/config/tmp_mos_otpr: ");
    // DEBUG_PRINTLN(float_value);
    str_topic = str_base_topic + "/config/tmp_mos_otpr";
    str_value = String(float_value);
    mqtt_client.publish(str_topic.c_str(), str_value.c_str());

    uint32_t_value = (data[index++] | data[index++] << 8 | data[index++] << 16 | data[index++] << 24);
    // DEBUG_PRINT(str_base_topic + "/config/cell_count: ");
    // DEBUG_PRINTLN(uint32_t_value);
    str_topic = str_base_topic + "/config/cell_count";
    str_value = String(uint32_t_value);
    mqtt_client.publish(str_topic.c_str(), str_value.c_str());

    uint32_t_value = (data[index++] | data[index++] << 8 | data[index++] << 16 | data[index++] << 24);
    // DEBUG_PRINT(str_base_topic + "/config/bat_charge_enabled: ");
    // DEBUG_PRINTLN(uint32_t_value);
    str_topic = str_base_topic + "/config/bat_charge_enabled";
    str_value = String(uint32_t_value);
    mqtt_client.publish(str_topic.c_str(), str_value.c_str());

    uint32_t_value = (data[index++] | data[index++] << 8 | data[index++] << 16 | data[index++] << 24);
    // DEBUG_PRINT(str_base_topic + "/config/bat_discharge_enabled: ");
    // DEBUG_PRINTLN(uint32_t_value);
    str_topic = str_base_topic + "/config/bat_discharge_enabled";
    str_value = String(uint32_t_value);
    mqtt_client.publish(str_topic.c_str(), str_value.c_str());

    uint32_t_value = (data[index++] | data[index++] << 8 | data[index++] << 16 | data[index++] << 24);
    // DEBUG_PRINT(str_base_topic + "/config/bat_balance_enabled: ");
    // DEBUG_PRINTLN(uint32_t_value);
    str_topic = str_base_topic + "/config/bat_balance_enabled";
    str_value = String(uint32_t_value);
    mqtt_client.publish(str_topic.c_str(), str_value.c_str());

    float_value = (data[index++] | data[index++] << 8 | data[index++] << 16 | data[index++] << 24) * 0.001;
    // DEBUG_PRINT(str_base_topic + "/config/cap_bat_cell: ");
    // DEBUG_PRINTLN(float_value);
    str_topic = str_base_topic + "/config/cap_bat_cell";
    str_value = String(float_value);
    mqtt_client.publish(str_topic.c_str(), str_value.c_str());

    uint32_t_value = (data[index++] | data[index++] << 8 | data[index++] << 16 | data[index++] << 24);
    // DEBUG_PRINT(str_base_topic + "/config/scp_delay: ");
    // DEBUG_PRINTLN(uint32_t_value);
    str_topic = str_base_topic + "/config/scp_delay";
    str_value = String(uint32_t_value);
    mqtt_client.publish(str_topic.c_str(), str_value.c_str());

    float_value = (data[index++] | data[index++] << 8 | data[index++] << 16 | data[index++] << 24) * 0.001;
    // DEBUG_PRINT(str_base_topic + "/config/vol_start_balance: ");
    // DEBUG_PRINTLN(float_value);
    str_topic = str_base_topic + "/config/vol_start_balance";
    str_value = String(float_value, 3);
    mqtt_client.publish(str_topic.c_str(), str_value.c_str());

    // jump over resistance correction settings
    index = 270;

    uint32_t_value = (data[index++] | data[index++] << 8 | data[index++] << 16 | data[index++] << 24);
    // DEBUG_PRINT(str_base_topic + "/config/dev_address: ");
    // DEBUG_PRINTLN(uint32_t_value);
    str_topic = str_base_topic + "/config/dev_address";
    str_value = String(uint32_t_value);
    mqtt_client.publish(str_topic.c_str(), str_value.c_str());

    uint32_t_value = (data[index++] | data[index++] << 8 | data[index++] << 16 | data[index++] << 24);
    // DEBUG_PRINT(str_base_topic + "/config/tim_pro_discharge: ");
    // DEBUG_PRINTLN(uint32_t_value);
    str_topic = str_base_topic + "/config/tim_pro_discharge";
    str_value = String(uint32_t_value);
    mqtt_client.publish(str_topic.c_str(), str_value.c_str());

    DEBUG_PRINT("Index <---------- ");
    DEBUG_PRINTLN(index);
}
