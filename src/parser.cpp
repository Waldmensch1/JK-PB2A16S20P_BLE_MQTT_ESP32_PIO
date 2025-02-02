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
float temp_pwr_tube = 0;
float cell_resistance_alert = 255;
float battery_voltage = 0;
float battery_power = 0;    
float battery_charge_current = 0;
float temp_sensor1 = 0;
float temp_sensor2 = 0;
float temp_sensor3 = 0;
float temp_sensor4 = 0;
float temp_sensor5 = 0;
uint32_t errors_mask = 0xFFFFFFFF;
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
uint16_t temp_sensor_absent_mask = 0xFFFF;




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
  if (fl_value != cell_avg_voltage || cell_avg_voltage == 0)
  {
    Serial.print("cellAvgVoltage: ");
    Serial.println(fl_value);
    cellStr = String(fl_value, 3);
    newTopic = String(mqttname + "/data/cells/voltage/cell_avg_voltage");
    client.publish(newTopic.c_str(), cellStr.c_str());
    cell_avg_voltage = fl_value;
  }

  // Read cell voltage difference
  fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8) * 0.001;
  if (fl_value != cell_diff_voltage || cell_diff_voltage == 0)
  {
    Serial.print("cellVoltageDiff: ");
    Serial.println(fl_value);
    cellStr = String(fl_value, 3);
    newTopic = String(mqttname + "/data/cells/voltage/cell_diff_voltage");
    client.publish(newTopic.c_str(), cellStr.c_str());
    cell_diff_voltage = fl_value;
  }

  // high_voltage_cell
  byte byte_value = receivedBytes_cell[index++];
  if (byte_value != high_voltage_cell || high_voltage_cell == 255)
  {

    Serial.print("Cell with highest voltage: ");
    Serial.println(byte_value);
    cellStr = String(byte_value);
    newTopic = String(mqttname + "/data/cells/voltage/high_voltage_cell");
    client.publish(newTopic.c_str(), cellStr.c_str());
    high_voltage_cell = byte_value;
  }

  // low_voltage_cell
  byte_value = receivedBytes_cell[index++];
  if (byte_value != low_voltage_cell || low_voltage_cell == 255)
  {
    Serial.print("Cell with lowest voltage: ");
    Serial.println(byte_value);
    cellStr = String(byte_value);
    newTopic = String(mqttname + "/data/cells/voltage/low_voltage_cell");
    client.publish(newTopic.c_str(), cellStr.c_str());
    low_voltage_cell = byte_value;
  }

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

  fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8) * 0.1;
  if (fl_value != temp_pwr_tube || temp_pwr_tube == 0)
  {
    Serial.print("powerTubeTemp: ");
    Serial.println(fl_value);
    cellStr = String(fl_value, 3);
    newTopic = String(mqttname + "/data/temperatures/temp_pwr_tube");
    client.publish(newTopic.c_str(), cellStr.c_str());
    temp_pwr_tube = fl_value;
  }

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

  fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8 | receivedBytes_cell[index++] << 16 | receivedBytes_cell[index++] << 24) * 0.001;
  if (fl_value != battery_voltage || battery_voltage == 0)
  {
    Serial.print("cellVoltage: ");
    Serial.println(fl_value);
    cellStr = String(fl_value, 3);
    newTopic = String(mqttname + "/data/battery_voltage");
    client.publish(newTopic.c_str(), cellStr.c_str());
    battery_voltage = fl_value;
  }

  fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8 | receivedBytes_cell[index++] << 16 | receivedBytes_cell[index++] << 24) * 0.001;
  if (fl_value != battery_power || battery_power == 0)
  {
    Serial.print("batteryPower: ");
    Serial.println(fl_value);
    cellStr = String(fl_value, 3);
    newTopic = String(mqttname + "/data/battery_power");
    client.publish(newTopic.c_str(), cellStr.c_str());
    battery_power = fl_value;
  }

  fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8 | receivedBytes_cell[index++] << 16 | receivedBytes_cell[index++] << 24) * 0.001;
  if (fl_value != battery_charge_current || battery_charge_current == 0)
  {
    Serial.print("chargeCurrent: ");
    Serial.println(fl_value);
    cellStr = String(fl_value, 3);
    newTopic = String(mqttname + "/data/battery_charge_current");
    client.publish(newTopic.c_str(), cellStr.c_str());
    battery_charge_current = fl_value;
  }

  fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8) * 0.1;
  if (fl_value != temp_sensor1 || temp_sensor1 == 0)
  {
    Serial.print("tempSensor1: ");
    Serial.println(fl_value);
    cellStr = String(fl_value, 3);
    newTopic = String(mqttname + "/data/temperatures/temp_sensor1");
    client.publish(newTopic.c_str(), cellStr.c_str());
    temp_sensor1 = fl_value;
  }

  fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8) * 0.1;
  if (fl_value != temp_sensor2 || temp_sensor2 == 0)
  {
    Serial.print("tempSensor2: ");
    Serial.println(fl_value);
    cellStr = String(fl_value, 3);
    newTopic = String(mqttname + "/data/temperatures/temp_sensor2");
    client.publish(newTopic.c_str(), cellStr.c_str());
    temp_sensor2 = fl_value;
  }

  uint32_t_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8 | receivedBytes_cell[index++] << 16 | receivedBytes_cell[index++] << 24);
  if (uint32_t_value != errors_mask || errors_mask == 255)
  {
    Serial.print("errorsMask: ");
    String errorsMaskStr = toBinaryString(uint32_t_value, 32);
    Serial.println(errorsMaskStr);
    newTopic = String(mqttname + "/data/errors_mask");
    client.publish(newTopic.c_str(), errorsMaskStr.c_str());
    errors_mask = uint32_t_value;
  }

  fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8) * 0.1;
  if (fl_value != balance_current || balance_current == 10000)
  {
    Serial.print("balanceCurrent: ");
    Serial.println(fl_value);
    Serial.println(fl_value);
    cellStr = String(fl_value, 3);
    newTopic = String(mqttname + "/data/balance_current");
    client.publish(newTopic.c_str(), cellStr.c_str());
    balance_current = fl_value;
  }

  byte_value = receivedBytes_cell[index++];
  if (byte_value != balancing_action || balancing_action == 255)
  {
    Serial.print("Balancing_action: ");
    Serial.println(byte_value);
    cellStr = String(byte_value);
    newTopic = String(mqttname + "/data/balancing_action");
    client.publish(newTopic.c_str(), cellStr.c_str());
    balancing_action = byte_value;
  }

  uint8_t uint8_t_value = receivedBytes_cell[index++];
  if (uint8_t_value != battery_soc || battery_soc == 0)
  {
    Serial.print("soc: ");
    Serial.println(uint8_t_value);
    cellStr = String(uint8_t_value);
    newTopic = String(mqttname + "/data/battery_soc");
    client.publish(newTopic.c_str(), cellStr.c_str());
    battery_soc = uint8_t_value;
  }

  fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8 | receivedBytes_cell[index++] << 16 | receivedBytes_cell[index++] << 24) * 0.001;
  if (fl_value != battery_capacity_remaining || battery_capacity_remaining == 0)
  {
    Serial.print("battery_capacity_remaining: ");
    Serial.println(fl_value);
    cellStr = String(fl_value, 3);
    newTopic = String(mqttname + "/data/battery_capacity_remaining");
    client.publish(newTopic.c_str(), cellStr.c_str());
    battery_capacity_remaining = fl_value;
  }

  fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8 | receivedBytes_cell[index++] << 16 | receivedBytes_cell[index++] << 24) * 0.001;
  if (fl_value != battery_capacity_total || battery_capacity_total == 0)
  {
    Serial.print("capacity_Full: ");
    Serial.println(fl_value);
    cellStr = String(fl_value, 3);
    newTopic = String(mqttname + "/data/battery_capacity_total");
    client.publish(newTopic.c_str(), cellStr.c_str());
    battery_capacity_total = fl_value;
  }

  uint32_t_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8 | receivedBytes_cell[index++] << 16 | receivedBytes_cell[index++] << 24);
  if (uint32_t_value != battery_cycle_count || battery_cycle_count == 0)
  {
    Serial.print("cycleCount: ");
    Serial.println(uint32_t_value);
    cellStr = String(uint32_t_value);
    newTopic = String(mqttname + "/data/battery_cycle_count");
    client.publish(newTopic.c_str(), cellStr.c_str());
    battery_cycle_count = uint32_t_value;
  }

  fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8 | receivedBytes_cell[index++] << 16 | receivedBytes_cell[index++] << 24) * 0.001;
  if (fl_value != battery_cycle_capacity_total || battery_cycle_capacity_total == 0)
  {
    Serial.print("total_cycle_capacity: ");
    Serial.println(fl_value);
    cellStr = String(fl_value, 3);
    newTopic = String(mqttname + "/data/battery_cycle_capacity_total");
    client.publish(newTopic.c_str(), cellStr.c_str());
    battery_cycle_capacity_total = fl_value;
  }

  byte_value = receivedBytes_cell[index++];
  if (byte_value != battery_soh || battery_soh == 255)
  {
    Serial.print("soh: ");
    Serial.println(byte_value);
    cellStr = String(byte_value);
    newTopic = String(mqttname + "/data/battery_soh");
    client.publish(newTopic.c_str(), cellStr.c_str());
    battery_soh = byte_value;
  }

  byte_value = receivedBytes_cell[index++];
  if (byte_value != battery_precharge_status || battery_precharge_status == 255)
  {
    Serial.print("prechargeStatus: ");
    Serial.println(byte_value);
    cellStr = String(byte_value);
    newTopic = String(mqttname + "/data/battery_precharge_status");
    client.publish(newTopic.c_str(), cellStr.c_str());
    battery_precharge_status = byte_value;
  }

  uint16_t uint16_t_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8);
  if (uint16_t_value != battery_user_alarm1 || battery_user_alarm1 == 0)
  {
    Serial.print("userAlarm1: ");
    Serial.println(uint16_t_value);
    cellStr = String(uint16_t_value);
    newTopic = String(mqttname + "/data/battery_user_alarm1");
    client.publish(newTopic.c_str(), cellStr.c_str());
    battery_user_alarm1 = uint16_t_value;
  }

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

  byte_value = receivedBytes_cell[index++];
  if (byte_value != charging_mosfet_status || charging_mosfet_status == 255)
  {
    Serial.print("chargingMosfetStatus: ");
    Serial.println(byte_value);
    cellStr = String(byte_value);
    newTopic = String(mqttname + "/data/charging_mosfet_status");
    client.publish(newTopic.c_str(), cellStr.c_str());
    charging_mosfet_status = byte_value;
  }

  byte_value = receivedBytes_cell[index++];
  if (byte_value != discharging_mosfet_status || discharging_mosfet_status == 255)
  {
    Serial.print("dischargingMosfetStatus: ");
    Serial.println(byte_value);
    cellStr = String(byte_value);
    newTopic = String(mqttname + "/data/discharging_mosfet_status");
    client.publish(newTopic.c_str(), cellStr.c_str());
    discharging_mosfet_status = byte_value;
  }

  uint16_t_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8);
  if (uint16_t_value != battery_user_alarm2 || battery_user_alarm2 == 0xFFFF)
  {
    Serial.print("userAlarm2: ");
    Serial.println(uint16_t_value);
    cellStr = String(uint16_t_value);
    newTopic = String(mqttname + "/data/battery_user_alarm2");
    client.publish(newTopic.c_str(), cellStr.c_str());
    battery_user_alarm2 = uint16_t_value;
  }

  uint16_t_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8);
  if (uint16_t_value != timeDcOCPR || timeDcOCPR == 0xFFFF)
  {
    Serial.print("timeDcOCPR: ");
    Serial.println(uint16_t_value);
    cellStr = String(uint16_t_value);
    newTopic = String(mqttname + "/data/timeDcOCPR");
    client.publish(newTopic.c_str(), cellStr.c_str());
    timeDcOCPR = uint16_t_value;
  }

  uint16_t_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8);
  if (uint16_t_value != timeDcSCPR || timeDcSCPR == 0xFFFF)
  {
    Serial.print("timeDcSCPR: ");
    Serial.println(uint16_t_value);
    cellStr = String(uint16_t_value);
    newTopic = String(mqttname + "/data/timeDcSCPR");
    client.publish(newTopic.c_str(), cellStr.c_str());
    timeDcSCPR = uint16_t_value;
  }

  uint16_t_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8);
  if (uint16_t_value != timeCOCPR || timeCOCPR == 0xFFFF)
  {
    Serial.print("timeCOCPR: ");
    Serial.println(uint16_t_value);
    cellStr = String(uint16_t_value);
    newTopic = String(mqttname + "/data/timeCOCPR");
    client.publish(newTopic.c_str(), cellStr.c_str());
    timeCOCPR = uint16_t_value;
  }

  uint16_t_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8);
  if (uint16_t_value != timeCSCPR || timeCSCPR == 0xFFFF)
  {
    Serial.print("timeCSCPR: ");
    Serial.println(uint16_t_value);
    cellStr = String(uint16_t_value);
    newTopic = String(mqttname + "/data/timeCSCPR");
    client.publish(newTopic.c_str(), cellStr.c_str());
    timeCSCPR = uint16_t_value;
  }

  uint16_t_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8);
  if (uint16_t_value != timeUVPR || timeUVPR == 0xFFFF)
  {
    Serial.print("timeUVPR: ");
    Serial.println(uint16_t_value);
    cellStr = String(uint16_t_value);
    newTopic = String(mqttname + "/data/timeUVPR");
    client.publish(newTopic.c_str(), cellStr.c_str());
    timeUVPR = uint16_t_value;
  }

  uint16_t_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8);
  if (uint16_t_value != timeOVPR || timeOVPR == 0xFFFF)
  {
    Serial.print("timeOVPR: ");
    Serial.println(uint16_t_value);
    cellStr = String(uint16_t_value);
    newTopic = String(mqttname + "/data/timeOVPR");
    client.publish(newTopic.c_str(), cellStr.c_str());
    timeOVPR = uint16_t_value;
  }

  uint16_t_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8);
  if (uint16_t_value != temp_sensor_absent_mask || temp_sensor_absent_mask == 0xFFFF)
  {
    Serial.print("temp_sensor_absent_mask: ");
    Serial.println(uint16_t_value);
    String temp_sensor_absent_mask_str = toBinaryString(uint16_t_value, 16);
    cellStr = String(temp_sensor_absent_mask_str);
    newTopic = String(mqttname + "/data/temp_sensor_absent_mask");
    client.publish(newTopic.c_str(), cellStr.c_str());
    temp_sensor_absent_mask = uint16_t_value;
  }

  // index is 216 here

  // according to Jihang's MODBUS documentation, the following fields can be implemented in the future

  // 0x00D2 210 UINT16 2 R Reserved
  // 0x00D4 212 UINT16 2 R 应急开关时间TimeEmergency S
  // 0x00D6 214 UINT16 2 R 放电电流修正因子BatDisCurCorrect
  // 0x00D8 216 UINT16 2 R 充电电流传感器电压VolChargCur mV
  // 0x00DA 218 UINT16 2 R 放电电流传感器电压VolDischargCur mV
  // 0x00DC 220 FLOAT 4 R 电池电压修正因子BatVolCorrect
  // 0x00E4 228 UINT16 2 R 电池电压BatVol 0.01V
  // 0x00E6 230 INT16 2 R 加热电流HeatCurrent mA
  // UINT8 R 保留RVD
  // UINT8 R 充电器状态ChargerPlugged 1：插入; 0：未插入
  // 0x00F0 240 UINT32 4 R 系统节拍SysRunTicks 0.1S
  // 0x00F8 248 INT16 2 R 电池温度TempBat 3 0.1℃
  // 0x00FA 250 INT16 2 R 电池温度TempBat 4 0.1℃
  // 0x00FC 252 INT16 2 R 电池温度TempBat 5 0.1℃
  // 0x0100 256 UINT32 4 R RTC计数器RTCTicks 自
  // 0x0108 264 UINT32 4 R 进入休眠时间TimeEnterSleep S
  // 0x010C 268 UINT8 并联限流模块状态PCLModuleSta 1：打开; 0：关闭
  //             UINT8 保留RVD

  // for now we will skip the above fields
  index += 38;

  // send current index to debug topic
  // cellStr = String(index);
  // newTopic = String(mqttname + "/debug/last_index");
  // client.publish(newTopic.c_str(), cellStr.c_str());

  // index is 254 here

  fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8) * 0.1;
  if (fl_value != temp_sensor3 || temp_sensor3 == 0)
  {
    Serial.print("tempSensor3: ");
    Serial.println(fl_value);
    cellStr = String(fl_value, 3);
    newTopic = String(mqttname + "/data/temperatures/temp_sensor3");
    client.publish(newTopic.c_str(), cellStr.c_str());
    temp_sensor3 = fl_value;
  }

  fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8) * 0.1;
  if (fl_value != temp_sensor4 || temp_sensor4 == 0)
  {
    Serial.print("tempSensor4: ");
    Serial.println(fl_value);
    cellStr = String(fl_value, 3);
    newTopic = String(mqttname + "/data/temperatures/temp_sensor4");
    client.publish(newTopic.c_str(), cellStr.c_str());
    temp_sensor4 = fl_value;
  }

  fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8) * 0.1;
  if (fl_value != temp_sensor5 || temp_sensor5 == 0)
  {
    Serial.print("tempSensor5: ");
    Serial.println(fl_value);
    cellStr = String(fl_value, 3);
    newTopic = String(mqttname + "/data/temperatures/temp_sensor5");
    client.publish(newTopic.c_str(), cellStr.c_str());
    temp_sensor5 = fl_value;
  }

  Serial.print("Index: ");
  Serial.println(index);

  blocked_for_parsing = false;
}

String toBinaryString(uint32_t value, int bits)
{
  String binaryString = "";
  for (int i = bits - 1; i >= 0; i--)
  {
    binaryString += ((value >> i) & 1) ? '1' : '0';
  }
  return binaryString;
}
