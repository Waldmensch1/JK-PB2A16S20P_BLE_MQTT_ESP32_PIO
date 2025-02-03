#include <Arduino.h>
#include "parser.h"

uint8_t counter_last = 0;
uint32_t cells_used = 0;
float volts_old[30] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
float cell_avg_voltage = 0;
float cell_diff_voltage = 0;
byte high_voltage_cell = 255;
byte low_voltage_cell = 255;
float cellResistance_old[30] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
float temp_mosfet = 0;
float cell_resistance_alert = 255;
float battery_voltage = 0;
float battery_power = 0;
float battery_charge_current = 0;
float temp_sensor1 = 0;
float temp_sensor2 = 0;
float temp_sensor3 = 0;
float temp_sensor4 = 0;
float temp_sensor5 = 0;
uint32_t alarms_mask = 0xFFFFFFFF;

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

float balance_current = 10000;
byte balancing_action = 255;
uint8_t battery_soc = 0;
float battery_capacity_remaining = 0;
float battery_capacity_total = 0;
uint32_t battery_cycle_count = 0;
float battery_cycle_capacity_total = 0;
byte battery_soh = 0;
byte battery_precharge_status = 255;
uint16_t battery_user_alarm1 = 0xFFFF;
uint16_t battery_user_alarm2 = 0xFFFF;
byte charging_mosfet_status = 255;
byte discharging_mosfet_status = 255;
byte precharging_status = 255;
uint16_t timeDcOCPR = 0xFFFF;
uint16_t timeDcSCPR = 0xFFFF;
uint16_t timeCOCPR = 0xFFFF;
uint16_t timeCSCPR = 0xFFFF;
uint16_t timeUVPR = 0xFFFF;
uint16_t timeOVPR = 0xFFFF;
byte temp_sensor_absent_mask = 0x00;

String temp_sensors_absent[6] = {
    "MOSTempSensorAbsent",
    "BATTempSensor1Absent",
    "BATTempSensor2Absent",
    "BATTempSensor3Absent",
    "BATTempSensor4Absent",
    "BATTempSensor5Absent"};

byte battery_heating = 0xFF;

float heat_current = 0xFFFF;
float bat_vol = 0xFFFF;
float bat_vol_correct = 0xFFFFFFFF;
float vol_discharg_cur = 0xFFFF;
float vol_charg_cur = 0xFFFF;
u_int32_t sys_run_ticks = 0x0;
byte charger_plugged = 0xFF;
uint16_t bat_dis_cur_correct = 0xFFFF;
uint16_t time_emergency = 0xFFFF;
uint32_t rtc_ticks = 0x0;
uint32_t time_enter_sleep = 0x0;
byte pcl_module_status = 0xFF;

template <typename T>
void publishIfChanged(T &currentValue, T newValue, const String &topic, int decimals = -1)
{
    if (currentValue != newValue)
    {
        Serial.print(topic + ": ");
        if (decimals >= 0)
        {
            Serial.println(newValue, decimals);
        }
        else
        {
            Serial.println(newValue);
        }
        String valueStr = (decimals >= 0) ? String(newValue, decimals) : String(newValue);
        client.publish(topic.c_str(), valueStr.c_str());
        currentValue = newValue;
    }
}

String toBinaryString(uint32_t value, int bits)
{
    String binaryString = "";
    for (int i = bits - 1; i >= 0; i--)
        binaryString += ((value >> i) & 1) ? '1' : '0';
    return binaryString;
}

void readDeviceDataRecord()
{
    size_t index = 5; // Skip the first 5 bytes
    uint8_t counter = receivedBytes_device[index++];
    // Serial.print("counter: ");
    // Serial.println(counter);

    String cellStr = String(counter);
    client.publish((mqttname + "/device/read_count").c_str(), cellStr.c_str());

    // Read vendorID
    char vendorID[50];
    size_t vendorIDIndex = 0;
    while (index < 300 && receivedBytes_device[index] != 0x0)
        vendorID[vendorIDIndex++] = receivedBytes_device[index++];

    vendorID[vendorIDIndex] = '\0';
    // Serial.print("vendorID: ");
    // Serial.println(vendorID);

    cellStr = String(vendorID);
    client.publish((mqttname + "/device/vendor_id").c_str(), cellStr.c_str());

    // Skip the 0x0
    while (index < 300 && receivedBytes_device[index] == 0x0)
        index++;

    // Read hardwareVersion
    char hardwareVersion[50];
    size_t hardwareVersionIndex = 0;
    while (index < 300 && receivedBytes_device[index] != 0x0)
        hardwareVersion[hardwareVersionIndex++] = receivedBytes_device[index++];

    hardwareVersion[hardwareVersionIndex] = '\0';
    // Serial.print("hardwareVersion: ");
    // Serial.println(hardwareVersion);
    cellStr = String(hardwareVersion);
    client.publish((mqttname + "/device/hw_revision").c_str(), cellStr.c_str());

    // Skip the 0x0
    while (index < 300 && receivedBytes_device[index] == 0x0)
        index++;

    // Read softwareVersion
    char softwareVersion[50];
    size_t softwareVersionIndex = 0;
    while (index < 300 && receivedBytes_device[index] != 0x0)
        softwareVersion[softwareVersionIndex++] = receivedBytes_device[index++];

    softwareVersion[softwareVersionIndex] = '\0';
    // Serial.print("softwareVersion: ");
    // Serial.println(softwareVersion);

    cellStr = String(softwareVersion);
    client.publish((mqttname + "/device/sw_version").c_str(), cellStr.c_str());

    // Skip the 0x0
    while (index < 300 && receivedBytes_device[index] == 0x0)
        index++;

    // Read uptime
    uint32_t uptime = 0;
    size_t uptimePos = 0;
    while (index < 300 && receivedBytes_device[index] != 0x0)
        uptime += receivedBytes_device[index++] * pow(256, uptimePos++);

    // Serial.print("uptime: ");
    // Serial.println(uptime);

    cellStr = String(uptime);
    client.publish((mqttname + "/device/uptime").c_str(), cellStr.c_str());

    byte sec = uptime % 60;
    uptime /= 60;
    byte mi = uptime % 60;
    uptime /= 60;
    byte hr = uptime % 24;
    byte days = uptime /= 24;
    client.publish((mqttname + "/device/uptime_fmt").c_str(), (String(days) + " days " + String(hr) + ":" + String(mi) + ":" + String(sec)).c_str());

    // Skip the 0x0
    while (index < 300 && receivedBytes_device[index] == 0x0)
        index++;

    // Read powerUpTimes
    uint8_t powerUpTimes = receivedBytes_device[index++];
    // Serial.print("powerUpTimes: ");
    // Serial.println(powerUpTimes);

    cellStr = String(powerUpTimes);
    client.publish((mqttname + "/device/power_up_times").c_str(), cellStr.c_str());

    // Skip the 0x0
    while (index < 300 && receivedBytes_device[index] == 0x0)
        index++;

    // Read deviceName
    char deviceName[50];
    size_t deviceNameIndex = 0;
    while (index < 300 && receivedBytes_device[index] != 0x0)
        deviceName[deviceNameIndex++] = receivedBytes_device[index++];

    deviceName[deviceNameIndex] = '\0';
    // Serial.print("deviceName: ");
    // Serial.println(deviceName);

    cellStr = String(deviceName);
    client.publish((mqttname + "/device/device_name").c_str(), cellStr.c_str());

    // Skip the 0x0
    while (index < 300 && receivedBytes_device[index] == 0x0)
        index++;

    // Read passCode
    char passCode[50];
    size_t passCodeIndex = 0;
    while (index < 300 && receivedBytes_device[index] != 0x0)
        passCode[passCodeIndex++] = receivedBytes_device[index++];

    passCode[passCodeIndex] = '\0';
    // Serial.print("passCode: ");
    // Serial.println(passCode);

    cellStr = String(passCode);
    client.publish((mqttname + "/device/device_passwd").c_str(), cellStr.c_str());

    while (index < 300 && receivedBytes_device[index] == 0x0)
        index++;

    // Read manufacturingDate
    char manufacturingDate[20];
    size_t manufacturingDateIndex = 0;
    while (index < 300 && receivedBytes_device[index] != 0x0)
        manufacturingDate[manufacturingDateIndex++] = receivedBytes_device[index++];

    manufacturingDate[manufacturingDateIndex] = '\0';
    // Serial.print("manufacturingDate: ");
    // Serial.println(manufacturingDate);

    cellStr = String(manufacturingDate);
    client.publish((mqttname + "/device/manufacturing_date").c_str(), cellStr.c_str());

    while (index < 300 && receivedBytes_device[index] == 0x0)
        index++;

    // Read serialNumber
    char serialNumber[50];
    size_t serialNumberIndex = 0;
    while (index < 300 && receivedBytes_device[index] != 0x0)
        serialNumber[serialNumberIndex++] = receivedBytes_device[index++];
    serialNumber[serialNumberIndex] = '\0';
    // Serial.print("serialNumber: ");
    // Serial.println(serialNumber);

    cellStr = String(serialNumber);
    client.publish((mqttname + "/device/serial_number").c_str(), cellStr.c_str());

    while (index < 300 && receivedBytes_device[index] == 0x0)
        index++;

    // Read passCode2
    char passCode2[50];
    size_t passCode2Index = 0;
    while (index < 300 && receivedBytes_device[index] != 0x0)
        passCode2[passCode2Index++] = receivedBytes_device[index++];
    passCode2[passCode2Index] = '\0';
    // Serial.print("passCode2: ");
    // Serial.println(passCode2);

    cellStr = String(passCode2);
    client.publish((mqttname + "/device/device_passwd2").c_str(), cellStr.c_str());

    while (index < 300 && receivedBytes_device[index] == 0x0)
        index++;

    // Read userData
    char userData[50];
    size_t userDataIndex = 0;
    while (index < 300 && receivedBytes_device[index] != 0x0)
        userData[userDataIndex++] = receivedBytes_device[index++];
    userData[userDataIndex] = '\0';
    // Serial.print("userData: ");
    // Serial.println(userData);

    cellStr = String(userData);
    client.publish((mqttname + "/device/user_data").c_str(), cellStr.c_str());

    while (index < 300 && receivedBytes_device[index] == 0x0)
        index++;

    // Read setupPasscode
    char setupPasscode[50];
    size_t setupPasscodeIndex = 0;
    while (index < 300 && receivedBytes_device[index] != 0x0)
        setupPasscode[setupPasscodeIndex++] = receivedBytes_device[index++];
    setupPasscode[setupPasscodeIndex] = '\0';
    // Serial.print("setupPasscode: ");
    // Serial.println(setupPasscode);

    cellStr = String(setupPasscode);
    client.publish((mqttname + "/device/setup_passcode").c_str(), cellStr.c_str());

    blocked_for_parsing = false;
}

void readCellDataRecord()
{
    size_t index = 5; // Skip the first 5 bytes
    uint8_t counter = receivedBytes_cell[index++];
    if (counter == counter_last)
        return;
    else
        counter_last = counter;

    Serial.print("counter: ");
    Serial.println(counter);

    String newTopic;
    String cellStr;

    newTopic = String(mqttname + "/data/readcount");
    cellStr = String(counter);
    client.publish(newTopic.c_str(), cellStr.c_str());

    if (debug_flg_full_log)
    {
        uint16_t inputLength = sizeof(receivedBytes_cell);
        char output[base64::encodeLength(inputLength)];
        base64::encode(receivedBytes_cell, inputLength, output);
        newTopic = String(mqttname + "/debug/rawdata");
        cellStr = String(output);
        client.publish(newTopic.c_str(), cellStr.c_str());
        client.publish((mqttname + "/debug/enabled").c_str(), "true");
    }
    else
    {
        newTopic = String(mqttname + "/debug/rawdata");
        cellStr = "not published";
        client.publish(newTopic.c_str(), cellStr.c_str());
        client.publish((mqttname + "/debug/enabled").c_str(), "false");
    }

    // Read cell voltages
    float volts[30];
    for (int i = 0; i < 30; i++)
    {
        uint16_t volt = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8);
        volts[i] = volt * 0.001;
    }

    Serial.print("volts: ");
    newTopic = String(mqttname + "/data/cells/voltage/cell_v_");
    for (uint8_t i = 0; i < 30; i++)
    {
        Serial.print(volts[i]);
        Serial.print(" ");
        if (volts[i] != volts_old[i])
        {
            cellStr = String(volts[i], 3);
            String topic;
            if (i < 9)
                topic = newTopic + String("0") + String(i + 1);
            else
                topic = newTopic + String(i + 1);
            if (volts[i] != 0)
            {
                client.publish(topic.c_str(), cellStr.c_str());
            }
        }
        volts_old[i] = volts[i];
    }
    Serial.println();

    // ignore 4 bytes
    index += 4;

    // read the mask
    uint32_t uint32_t_value = (receivedBytes_cell[index++] << 24) |
                              (receivedBytes_cell[index++] << 16) |
                              (receivedBytes_cell[index++] << 8) |
                              (receivedBytes_cell[index++]);

    if (uint32_t_value != cells_used || cells_used == 0)
    {
        Serial.print("mask: ");
        String cellsUsedMaskStr = toBinaryString(uint32_t_value, 32);
        Serial.println(cellsUsedMaskStr);

        newTopic = String(mqttname + "/data/cells_used");
        client.publish(newTopic.c_str(), cellsUsedMaskStr.c_str());
        cells_used = uint32_t_value;
    }

    // Read cell average voltage
    float fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8) * 0.001;
    publishIfChanged(cell_avg_voltage, fl_value, mqttname + "/data/cells/voltage/cell_avg_voltage", 3);

    // Read cell voltage difference
    fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8) * 0.001;
    publishIfChanged(cell_diff_voltage, fl_value, mqttname + "/data/cells/voltage/cell_diff_voltage", 3);

    // high_voltage_cell
    byte byte_value = receivedBytes_cell[index++];
    publishIfChanged(high_voltage_cell, byte_value, mqttname + "/data/cells/voltage/high_voltage_cell");

    // low_voltage_cell
    byte_value = receivedBytes_cell[index++];
    publishIfChanged(low_voltage_cell, byte_value, mqttname + "/data/cells/voltage/low_voltage_cell");

    // Read cell resistances
    float cellResistance[30];
    for (int i = 0; i < 30; i++)
    {
        uint16_t resistance = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8);
        cellResistance[i] = resistance * 0.001;
    }

    Serial.print("cellResistance: ");

    newTopic = String(mqttname + "/data/cells/resistance/cell_r_");
    for (uint8_t i = 0; i < 30; i++)
    {
        Serial.print(cellResistance[i]);
        Serial.print(" ");

        if (cellResistance[i] != cellResistance_old[i] || cellResistance_old[i] == 0)
        {

            cellStr = String(cellResistance[i], 3);
            String topic;
            if (i < 9)
                topic = newTopic + String("0") + String(i + 1);
            else
                topic = newTopic + String(i + 1);
            if (cellResistance[i] != 0)
            {
                client.publish(topic.c_str(), cellStr.c_str());
            }
            cellResistance_old[i] = cellResistance[i];
        }
    }
    Serial.println();

    // ignore 4 bytes
    index += 4;

    // temp_mosfet
    fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8) * 0.1;
    publishIfChanged(temp_mosfet, fl_value, mqttname + "/data/temperatures/temp_mosfet");

    // read the mask
    uint32_t_value = (receivedBytes_cell[index++] << 24) |
                     (receivedBytes_cell[index++] << 16) |
                     (receivedBytes_cell[index++] << 8) |
                     (receivedBytes_cell[index++]);

    if (uint32_t_value != cell_resistance_alert || cell_resistance_alert == 255)
    {
        Serial.print("resistanceAlertMask: ");
        String resistanceAlertMaskStr = toBinaryString(uint32_t_value, 32);
        Serial.println(resistanceAlertMaskStr);
        newTopic = String(mqttname + "/data/cell_resistance_alert");
        client.publish(newTopic.c_str(), resistanceAlertMaskStr.c_str());
        cell_resistance_alert = uint32_t_value;
    }

    // battery_voltage
    fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8 | receivedBytes_cell[index++] << 16 | receivedBytes_cell[index++] << 24) * 0.001;
    publishIfChanged(battery_voltage, fl_value, mqttname + "/data/battery_voltage");

    // battery_power
    fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8 | receivedBytes_cell[index++] << 16 | receivedBytes_cell[index++] << 24) * 0.001;
    publishIfChanged(battery_power, fl_value, mqttname + "/data/battery_power");

    // battery_charge_current
    fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8 | receivedBytes_cell[index++] << 16 | receivedBytes_cell[index++] << 24) * 0.001;
    publishIfChanged(battery_charge_current, fl_value, mqttname + "/data/battery_charge_current");

    // temp_sensor1
    fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8) * 0.1;
    publishIfChanged(temp_sensor1, fl_value, mqttname + "/data/temperatures/temp_sensor1");

    // temp_sensor2
    fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8) * 0.1;
    publishIfChanged(temp_sensor2, fl_value, mqttname + "/data/temperatures/temp_sensor2");

    uint32_t_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8 | receivedBytes_cell[index++] << 16 | receivedBytes_cell[index++] << 24);
    if (uint32_t_value != alarms_mask || alarms_mask == 0xFFFFFFFF)
    {
        Serial.print("errorsMask: ");
        String errorsMaskStr = toBinaryString(uint32_t_value, 32);
        Serial.println(errorsMaskStr);
        newTopic = String(mqttname + "/data/alarms/alarms_mask");
        client.publish(newTopic.c_str(), errorsMaskStr.c_str());

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

        for (int i = 0; i < 24; ++i)
        {
            String status = (uint32_t_value & (1 << i)) ? "ON" : "OFF";
            newTopic = mqttname + "/data/alarms/" + alarmStrings[i];
            client.publish(newTopic.c_str(), status.c_str());
        }

        alarms_mask = uint32_t_value;
    }

    fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8) * 0.1;
    publishIfChanged(balance_current, fl_value, mqttname + "/data/balance_current");

    byte_value = receivedBytes_cell[index++];
    publishIfChanged(balancing_action, byte_value, mqttname + "/data/balancing_action");

    uint8_t uint8_t_value = receivedBytes_cell[index++];
    publishIfChanged(battery_soc, uint8_t_value, mqttname + "/data/battery_soc");

    fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8 | receivedBytes_cell[index++] << 16 | receivedBytes_cell[index++] << 24) * 0.001;
    publishIfChanged(battery_capacity_remaining, fl_value, mqttname + "/data/battery_capacity_remaining");

    fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8 | receivedBytes_cell[index++] << 16 | receivedBytes_cell[index++] << 24) * 0.001;
    publishIfChanged(battery_capacity_total, fl_value, mqttname + "/data/battery_capacity_total");

    uint32_t_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8 | receivedBytes_cell[index++] << 16 | receivedBytes_cell[index++] << 24);
    publishIfChanged(battery_cycle_count, uint32_t_value, mqttname + "/data/battery_cycle_count");

    fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8 | receivedBytes_cell[index++] << 16 | receivedBytes_cell[index++] << 24) * 0.001;
    publishIfChanged(battery_cycle_capacity_total, fl_value, mqttname + "/data/battery_cycle_capacity_total");

    byte_value = receivedBytes_cell[index++];
    publishIfChanged(battery_soh, byte_value, mqttname + "/data/battery_soh");

    byte_value = receivedBytes_cell[index++];
    publishIfChanged(battery_precharge_status, byte_value, mqttname + "/data/battery_precharge_status");

    uint16_t uint16_t_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8);
    publishIfChanged(battery_user_alarm1, uint16_t_value, mqttname + "/data/battery_user_alarm1");

    uint32_t_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8 | receivedBytes_cell[index++] << 16 | receivedBytes_cell[index++] << 24);
    Serial.print("totalRuntime: ");
    Serial.println(uint32_t_value);
    cellStr = String(uint32_t_value);
    newTopic = String(mqttname + "/data/battery_total_runtime");
    client.publish(newTopic.c_str(), cellStr.c_str());

    byte sec = uint32_t_value % 60;
    uint32_t_value /= 60;
    byte mi = uint32_t_value % 60;
    uint32_t_value /= 60;
    byte hr = uint32_t_value % 24;
    byte days = uint32_t_value /= 24;
    Serial.println(String(days) + " days " + String(hr) + ":" + String(mi) + ":" + String(sec));
    newTopic = String(mqttname + "/data/battery_total_runtime_fmt");
    client.publish(newTopic.c_str(), (String(days) + " days " + String(hr) + ":" + String(mi) + ":" + String(sec)).c_str());

    // charging_mosfet_status
    byte_value = receivedBytes_cell[index++];
    publishIfChanged(charging_mosfet_status, byte_value, mqttname + "/data/charging_mosfet_status");

    // discharging_mosfet_status
    byte_value = receivedBytes_cell[index++];
    publishIfChanged(discharging_mosfet_status, byte_value, mqttname + "/data/discharging_mosfet_status");

    // battery_user_alarm2
    uint16_t_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8);
    publishIfChanged(battery_user_alarm2, uint16_t_value, mqttname + "/data/battery_user_alarm2");

    // timeDcOCPR
    uint16_t_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8);
    publishIfChanged(timeDcOCPR, uint16_t_value, mqttname + "/data/alarms/timeDcOCPR");

    // timeDcSCPR
    uint16_t_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8);
    publishIfChanged(timeDcSCPR, uint16_t_value, mqttname + "/data/alarms/timeDcSCPR");

    // timeCOCPR
    uint16_t_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8);
    publishIfChanged(timeCOCPR, uint16_t_value, mqttname + "/data/alarms/timeCOCPR");

    // timeCSCPR
    uint16_t_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8);
    publishIfChanged(timeCSCPR, uint16_t_value, mqttname + "/data/alarms/timeCSCPR");

    // timeUVPR
    uint16_t_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8);
    publishIfChanged(timeUVPR, uint16_t_value, mqttname + "/data/alarms/timeUVPR");

    // timeOVPR
    uint16_t_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8);
    publishIfChanged(timeOVPR, uint16_t_value, mqttname + "/data/alarms/timeOVPR");

    byte_value = (receivedBytes_cell[index++]);
    if (byte_value != temp_sensor_absent_mask || temp_sensor_absent_mask == 0x00)
    {
        Serial.print("temp_sensor_absent_mask: ");
        Serial.println(byte_value);
        String temp_sensor_absent_mask_str = toBinaryString(byte_value, 8);
        cellStr = String(temp_sensor_absent_mask_str);
        newTopic = String(mqttname + "/data/temperatures/temp_sensor_absent_mask");
        client.publish(newTopic.c_str(), cellStr.c_str());

        // resolve temp sensor absent mask according to 极空BMS RS485 Modbus通用协议V1.1 2024.02 Page 11
        // MOS TempSensorAbsent; 1：Normal; 0：Missing; BIT0
        // BATTempSensor1Absent; 1：Normal; 0：Missing; BIT1
        // BATTempSensor2Absent; 1：Normal; 0：Missing; BIT2
        // BATTempSensor3Absent; 1：Normal; 0：Missing; BIT3
        // BATTempSensor4Absent; 1：Normal; 0：Missing; BIT4
        // BATTempSensor5Absent; 1：Normal; 0：Missing; BIT5

        for (int i = 0; i < 6; ++i)
        {
            String status = (byte_value & (1 << i)) ? "Normal" : "Missing";
            String newTopic = mqttname + "/data/temperatures/" + temp_sensors_absent[i];
            client.publish(newTopic.c_str(), status.c_str());
        }

        temp_sensor_absent_mask = byte_value;
    }

    // battery_heating
    byte_value = (receivedBytes_cell[index++]);
    publishIfChanged(battery_heating, byte_value, mqttname + "/data/temperatures/battery_heating");

    // send current index to debug topic
    cellStr = String(index);
    newTopic = String(mqttname + "/debug/last_index_1");
    client.publish(newTopic.c_str(), cellStr.c_str());

    // index 216
    index += 1; // skip 1 reserved byte

    // index 217
    // suspect: time_emergency
    uint16_t_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8);
    publishIfChanged(time_emergency, uint16_t_value, mqttname + "/data/sus/time_emergency");

    // index 219
    // suspect:  Discharge current correction factor
    uint16_t_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8);
    publishIfChanged(bat_dis_cur_correct, uint16_t_value, mqttname + "/data/sus/bat_dis_cur_correct");

    // index 221
    // suspect: Charging current sensor voltage
    fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8) * 0.001;
    publishIfChanged(vol_charg_cur, fl_value, mqttname + "/data/sus/vol_charg_cur");

    // index 223
    // suspect: Discharge current sensor voltage.
    fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8) * 0.001;
    publishIfChanged(vol_discharg_cur, fl_value, mqttname + "/data/sus/vol_discharg_cur");

    // index 225
    // suspect: Battery voltage correction factor
    fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8 | receivedBytes_cell[index++] << 16 | receivedBytes_cell[index++] << 24) * 0.001;
    publishIfChanged(bat_vol_correct, fl_value, mqttname + "/data/sus/bat_vol_correct");

    // index 229
    index += 4; // skip 4 reserved bytes

    // index 233
    // suspect: Battery voltage
    fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8) * 0.01;
    publishIfChanged(bat_vol, fl_value, mqttname + "/data/sus/bat_vol");

    // index 235
    // suspect: Heating current
    fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8) * 0.001;
    publishIfChanged(heat_current, fl_value, mqttname + "/data/sus/heat_current");

    // index 237
    //  skip 7 sus bytes
    index += 8;

    // index 245
    byte_value = receivedBytes_cell[index++];
    publishIfChanged(charger_plugged, byte_value, mqttname + "/data/sus/charger_plugged");

    // index 246
    // 0x00F0 240 UINT32 4 R 系统节拍SysRunTicks 0.1S
    uint32_t_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8 | receivedBytes_cell[index++] << 16 | receivedBytes_cell[index++] << 24);
    publishIfChanged(sys_run_ticks, uint32_t_value, mqttname + "/data/sus/sys_run_ticks");

    // index 250
    index += 2; // skip 2 sus bytes

    // index 252
    index += 2; // skip 2 sus bytes

    // index 254
    // send current index to debug topic
    cellStr = String(index);
    newTopic = String(mqttname + "/debug/last_index_2");
    client.publish(newTopic.c_str(), cellStr.c_str());

    // temp_sensor3
    fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8) * 0.1;
    publishIfChanged(temp_sensor3, fl_value, mqttname + "/data/temperatures/temp_sensor3");

    // index 256
    // temp_sensor4
    fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8) * 0.1;
    publishIfChanged(temp_sensor4, fl_value, mqttname + "/data/temperatures/temp_sensor4");

    // index 258
    // temp_sensor5
    fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8) * 0.1;
    publishIfChanged(temp_sensor5, fl_value, mqttname + "/data/temperatures/temp_sensor5");

    // index 260
    index += 2; // skip 2 sus bytes

    // index 262
    // suspect: rtc_ticks
    uint32_t_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8 | receivedBytes_cell[index++] << 16 | receivedBytes_cell[index++] << 24);
    publishIfChanged(rtc_ticks, uint32_t_value, mqttname + "/data/sus/rtc_ticks");

    // index 266
    index += 4; // skip 4 sus bytes

    // index 270
    // suspect: time_enter_sleep
    uint32_t_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8 | receivedBytes_cell[index++] << 16 | receivedBytes_cell[index++] << 24);
    publishIfChanged(time_enter_sleep, uint32_t_value, mqttname + "/data/sus/time_enter_sleep");

    // index 274
    // suspect: PCLModuleSta Parallel current limiting module status 1: on; 0: off
    byte_value = receivedBytes_cell[index++];
    publishIfChanged(pcl_module_status, byte_value, mqttname + "/data/sus/pcl_module_status");

    // index 275
    Serial.print("Index: ");
    Serial.println(index);

    blocked_for_parsing = false;
}

