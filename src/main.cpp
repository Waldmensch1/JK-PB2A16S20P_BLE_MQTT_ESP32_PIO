#include "BLEDevice.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include "driver/gpio.h"
#include "main.h"

void MQTTCallback(char *topic, byte *payload, unsigned int length)
{
  String Command = "";

  if (strcmp(topic, (mqttname + "/parameter/debugging_active").c_str()) == 0)
  {
    for (int i = 0; i < length; i++)
    {
      Command = Command + (char)payload[i];
    }
    if (Command == "true")
    {
      debug_flg = true;
      Serial.println("Debugging: true");
    }
    else if (Command == "false")
    {
      debug_flg = false;
      Serial.println("Debugging: false");
    }
  }

  if (strcmp(topic, (mqttname + "/parameter/debugging_active_full").c_str()) == 0)
  {
    for (int i = 0; i < length; i++)
    {
      Command = Command + (char)payload[i];
    }
    if (Command == "true")
    {
      debug_flg_full_log = true;
      Serial.println("Debugging Full Log: true");
    }
    else if (Command == "false")
    {
      debug_flg_full_log = false;
      Serial.println("Debugging Full Log: false");
    }
  }
}

// MQTT
WiFiClient espClient;
PubSubClient client(mqtt_server, mqtt_port, MQTTCallback, espClient);
long lastReconnectAttempt = 0;

static void notifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{

  if (debug_flg_full_log)
  {
    Serial.print("Notify callback for characteristic ");
    Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
    Serial.print(" of data length ");
    Serial.println(length);
    Serial.print("data: ");

    for (int i = 0; i < length; i++)
    {
      Serial.print(pData[i], HEX);
      Serial.print(", ");
    }
    Serial.println("");
  }

  if (blocked_for_parsing)
  {
    Serial.println("Blocked while parsing data !");
    return;
  }

  if (pData[0] == 0x55 && pData[1] == 0xAA && pData[2] == 0xEB && pData[3] == 0x90 && (pData[4] == CELLDATA || pData[4] == DEVICEDATA))
  {

    if (pData[4] == CELLDATA || pData[4] == DEVICEDATA)
    {
      received_start = true;
      received_start_frame = true;
      received_complete = false;
      frame = 0;
    }

    if (pData[4] == CELLDATA)
    {
      actualdata = CELLDATA;
      Serial.println("Cell Daten anerkannt !");
    }
    else if (pData[4] == DEVICEDATA)
    {
      actualdata = DEVICEDATA;
      Serial.println("Device Daten anerkannt !");
    }

    for (int i = 0; i < length; i++)
    {
      if (actualdata == CELLDATA)
      {
        receivedBytes_cell[frame] = pData[i];
      }
      else if (actualdata == DEVICEDATA)
      {
        receivedBytes_device[frame] = pData[i];
      }

      frame++;
    }
  }

  if (received_start && !received_start_frame && !received_complete)
  {
    //     Serial.println("Daten erweitert !");
    for (int i = 0; i < length; i++)
    {
      if (actualdata == CELLDATA)
      {
        receivedBytes_cell[frame] = pData[i];
      }
      else if (actualdata == DEVICEDATA)
      {
        receivedBytes_device[frame] = pData[i];
      }

      frame++;
    }

    if ((frame > 320))
    {
      Serial.println("Fehlerhafte Daten !!");
      frame = 0;
      received_start = false;
      new_data = false;
    }

    if (frame >= 300)
    {
      Serial.println("New Data for Analyse Complete...");
      received_complete = true;
      received_start = false;
      new_data = true;
      blocked_for_parsing = true;
      BLE_Scan_counter = 0;
    }
  }

  received_start_frame = false;
}

class MyClientCallback : public BLEClientCallbacks
{

  void onConnect(BLEClient *pclient)
  {
    client.publish((mqttname + "/ble_connection").c_str(), "connected");
  }

  void onDisconnect(BLEClient *pclient)
  {
    ble_connected = false;
    // pclient->disconnect();
    client.publish((mqttname + "/ble_connection").c_str(), "disconnected");
    Serial.println("BLE-Disconnect");
    client.publish((mqttname + "/ble_connection").c_str(), "Rebooting");
    Serial.println("BLE was Disconnected ... and no BLE reconnection possible, Reboot ESP...");
    ESP.restart();
  }
};

/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
  /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice)
  {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID) && advertisedDevice.getName() == Geraetename)
    {

      pBLEScan->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      pBLEScan->clearResults();

    } // Found our server
  } // onResult
}; // MyAdvertisedDeviceCallbacks

void setup()
{
  Serial.begin(115200);
  Serial.print("BMS Watchdog V ");
  Serial.println(VERSION);
  Serial.println("Booting");

  // WIFI Setup
  initWiFi();

  // BLE Setup
  BLEDevice::init("");
  pClient = BLEDevice::createClient();
  Serial.println(" - Created client");

  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);

} // End of setup.

void loop()
{

  // WIFI Check
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("WIFI Connection is Lost! Try to Reconnect...");
    initWiFi();
  }
  else
  {

    // MQTT Check if WIFI is There
    if (!client.connected())
    {
      if ((millis() - lastReconnectAttempt) > 5000)
      { // reconnect nach 5 sekunden
        if (debug_flg)
        {
          Serial.println("MQTT Client not connected");
          Serial.println("MQTT time for reconnect");
        }
        lastReconnectAttempt = millis();
        // Attempt to reconnect
        if (reconnect())
        {
          lastReconnectAttempt = 0;
        }
        else
        {
          if (debug_flg)
          {
            Serial.println("MQTT reconnect Error");
          }
        }
      }
    }
    else
    {
      client.loop();
    }
  }

  // BLE
  if (doConnect == true)
  {
    if (!connectToBLEServer())
    {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
      client.publish((mqttname + "/ble_connection").c_str(), "BLE_Connection_error!");
      delay(500);
      ble_connected = false;
      doConnect = false;
    }
  }

  if (ble_connected)
  {

    if (received_complete)
    {

      if (new_data)
      {
        analyze();
        newdatalasttime = millis();
      }

      if (mqttpublishtime == 0 || (millis() >= (mqttpublishtime + mqttpublishtime_offset)))
      {
        mqttpublishtime = millis();
        client.publish((mqttname + "/ble_connection").c_str(), "connected");
        client.publish((mqttname + "/status").c_str(), "online");
      }
    }

    // BLE Get Device Data Trigger ...
    if (((millis() - sendingtime) > 5000) && sendingtime != 0)
    {
      sendingtime = 0;
      Serial.println("gesendet!");
      // aktive_sending = false;
      pRemoteCharacteristic->writeValue(getInfo, 20);
    }
  }

  // BLE nicht verbunden
  if ((!ble_connected && !doConnect && (millis() - bleScantime) > 15000))
  {

    Serial.println("BLE -> Reconnecting!");
    client.publish((mqttname + "/ble_connection").c_str(), "Reconnecting");
    bleScantime = millis();
    pBLEScan->start(5, false);
    BLE_Scan_counter++;
  }

  // BLE verbidnugn ist da aber es kommen seit X Sekunden keine neuen Daten !
  if (!doConnect && ble_connected && (millis() >= (newdatalasttime + 60000)) && newdatalasttime != 0)
  {
    ble_connected = false;
    delay(200);
    client.publish((mqttname + "/ble_connection").c_str(), "terminated");
    Serial.println("BLE-Disconnect/terminated");
    newdatalasttime = millis();
    pClient->disconnect();
  }

  // checker das nach max 5 Minuten und keiner BLE Verbidung neu gestartet wird...
  if (BLE_Scan_counter > 20)
  {
    client.publish((mqttname + "/ble_connection").c_str(), "Rebooting");
    Serial.println("BLE isn´t receiving new Data form BMS... and no BLE reconnection possible, Reboot ESP...");
    ESP.restart();
  }

} // End of loop

////// BLE CLIENT

bool connectToBLEServer()
{
  Serial.print("Forming a connection to ");
  Serial.println(myDevice->getAddress().toString().c_str());

  pClient->setClientCallbacks(new MyClientCallback());
  delay(500); // hope it helps against ->  lld_pdu_get_tx_flush_nb HCI packet count mismatch (0, 1)

  // Connect to the remove BLE Server.
  pClient->connect(myDevice); // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
  delay(500);                 // hope it helps against ->  lld_pdu_get_tx_flush_nb HCI packet count mismatch (0, 1)

  Serial.println(" - Connected to server");

  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService *pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr)
  {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(serviceUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our service");

  // Obtain a reference to the characteristic in the service of the remote BLE server.
  pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
  if (pRemoteCharacteristic == nullptr)
  {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(charUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }

  Serial.println(" - Found our characteristic");

  // Read the value of the characteristic.
  if (pRemoteCharacteristic->canRead())
  {
    std::string value = pRemoteCharacteristic->readValue();
    Serial.print("The characteristic value was: ");
    Serial.println(value.c_str());
  }

  if (pRemoteCharacteristic->canNotify())
  {
    pRemoteCharacteristic->registerForNotify(notifyCallback);
    Serial.println("Notify the characteristic");
  }

  // Sending getdevice info
  pRemoteCharacteristic->writeValue(getdeviceInfo, 20);
  sendingtime = millis();
  Serial.println("Sending device Info");

  ble_connected = true;
  doConnect = false;
  Serial.println("We are now connected to the BLE Server.");
  client.publish((mqttname + "/ble_connection").c_str(), "connected");
  return true;
}

////// analyze

void analyze()
{

  if (actualdata == CELLDATA)
  {
    readCellDataRecord();
  }
  else if (actualdata == DEVICEDATA)
  {
    readDeviceDataRecord();
  }
}

////// MQTTCLIENT

boolean reconnect()
{
  if (client.connect(mqttname.c_str(), mqtt_username, mqtt_passwort, willTopic.c_str(), willQoS, willRetain, willMessage.c_str()))
  {
    // Once connected, publish an announcement...
    if (millis() < 10000)
    {
      client.publish((mqttname + "/ble_connection").c_str(), "Startup");
    }

    client.subscribe((mqttname + "/parameter/debugging_active").c_str());
    client.subscribe((mqttname + "/parameter/debugging_active_full").c_str()); // debug_flg_full_log
    client.publish((mqttname + "/status").c_str(), "online");
    if (debug_flg)
    {
      Serial.println("MQTT reconnected!");
    }
  }
  else
  {
    if (debug_flg)
    {
      Serial.println("MQTT connection failed!");
    }
  }
  return client.connected();
}

///////// WIFI

void initWiFi()
{
  byte wifi_retry = 0;
  WiFi.disconnect();      // ensure WiFi is disconnected
  WiFi.persistent(false); // do not persist as WiFi.begin is not helpful then. Decreases connection speed but helps in other ways
  WiFi.mode(WIFI_STA);
  // esp_wifi_set_ps( WIFI_PS_NONE ); // do not set WiFi to sleep, increases stability
  WiFi.config(0u, 0u, 0u); // ensure settings are reset, retrieve new IP Address in any case => DHCP necessary
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED && wifi_retry < 10)
  {
    WiFi.begin(ssid, password);
    wifi_retry++;
    Serial.print('.');
    delay(10000);
  }
  if (wifi_retry >= 10)
  {
    Serial.println("\nReboot...");
    ESP.restart();
  }
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
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
  Serial.print("counter: ");
  Serial.println(counter);

  String newTopic = String(mqttname + "/data/readcount");
  String cellStr = String(counter);
  client.publish(newTopic.c_str(), cellStr.c_str());

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
    newTopic = String(mqttname + "/data/cell_avg_voltage");
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
    newTopic = String(mqttname + "/data/cell_diff_voltage");
    client.publish(newTopic.c_str(), cellStr.c_str());
    cell_diff_voltage = fl_value;
  }

  // Read balance current
  fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8) * 0.001;
  Serial.print("???: ");
  Serial.println(fl_value);

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
    newTopic = String(mqttname + "/data/temp_pwr_tube");
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
    newTopic = String(mqttname + "/data/temp_sensor1");
    client.publish(newTopic.c_str(), cellStr.c_str());
    temp_sensor1 = fl_value;
  }

  fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8) * 0.1;
  if (fl_value != temp_sensor2 || temp_sensor2 == 0)
  {
    Serial.print("tempSensor2: ");
    Serial.println(fl_value);
    cellStr = String(fl_value, 3);
    newTopic = String(mqttname + "/data/temp_sensor2");
    client.publish(newTopic.c_str(), cellStr.c_str());
    temp_sensor2 = fl_value;
  }

  int16_t int16_t_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8);
  if (int16_t_value != errors_mask || errors_mask == 255)
  {
    Serial.print("errorsMask: ");
    String errorsMaskStr = toBinaryString(int16_t_value, 16);
    Serial.println(errorsMaskStr);
    newTopic = String(mqttname + "/data/errors_mask");
    client.publish(newTopic.c_str(), errorsMaskStr.c_str());
    errors_mask = int16_t_value;
  }

  fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8) * 0.1;
  if (fl_value != balance_current || balance_current == 0)
  {
    Serial.print("balanceCurrent: ");
    Serial.println(fl_value);
    Serial.println(fl_value);
    cellStr = String(fl_value, 3);
    newTopic = String(mqttname + "/data/balance_current");
    client.publish(newTopic.c_str(), cellStr.c_str());
    balance_current = fl_value;
  }

  byte byte_value = receivedBytes_cell[index++];
  if (byte_value != balancing_action || balancing_action == 255)
  {
    Serial.print("Balancing_action: ");
    Serial.println(byte_value);
    cellStr = String(byte_value);
    newTopic = String(mqttname + "/data/balancing_action");
    client.publish(newTopic.c_str(), cellStr.c_str());
    balancing_action = byte_value;
  }

  // ignore
  index += 2;

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
  if (fl_value != capacity_remaining || capacity_remaining == 0)
  {
    Serial.print("capacity_Remaining: ");
    Serial.println(fl_value);
    cellStr = String(fl_value, 3);
    newTopic = String(mqttname + "/data/capacity_remaining");
    client.publish(newTopic.c_str(), cellStr.c_str());
    capacity_remaining = fl_value;
  }

  fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8 | receivedBytes_cell[index++] << 16 | receivedBytes_cell[index++] << 24) * 0.001;
  if (fl_value != capacity_total || capacity_total == 0)
  {
    Serial.print("capacity_Full: ");
    Serial.println(fl_value);
    cellStr = String(fl_value, 3);
    newTopic = String(mqttname + "/data/capacity_total");
    client.publish(newTopic.c_str(), cellStr.c_str());
    capacity_total = fl_value;
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
  if (fl_value != cycle_capacity_total || cycle_capacity_total == 0)
  {
    Serial.print("total_cycle_capacity: ");
    Serial.println(fl_value);
    cellStr = String(fl_value, 3);
    newTopic = String(mqttname + "/data/cycle_capacity_total");
    client.publish(newTopic.c_str(), cellStr.c_str());
    cycle_capacity_total = fl_value;
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

  // absolutely no clue how to parse this
  uint32_t_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8 | receivedBytes_cell[index++] << 16 | receivedBytes_cell[index++] << 24);
  Serial.print("totalRuntime: ");
  Serial.println(uint32_t_value);
  cellStr = String(uint32_t_value);
  newTopic = String(mqttname + "/data/battery_total_runtime");
  // client.publish(newTopic.c_str(), cellStr.c_str());

  byte sec = uint32_t_value % 60;
  uint32_t_value /= 60;
  byte mi = uint32_t_value % 60;
  uint32_t_value /= 60;
  byte hr = uint32_t_value % 24;
  byte days = uint32_t_value /= 24;
  Serial.println(String(days) + " days " + String(hr) + ":" + String(mi) + ":" + String(sec));
  newTopic = String(mqttname + "/data/battery_total_runtime_fmt");
  // client.publish(newTopic.c_str(), (String(days) + " days " + String(hr) + ":" + String(mi) + ":" + String(sec)).c_str());

  // ignore 1 byte
  index += 1;

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

  byte_value = receivedBytes_cell[index++];
  if (byte_value != precharging_status || precharging_status == 255)
  {
    Serial.print("prechargingStatus: ");
    Serial.println(byte_value);
    cellStr = String(byte_value);
    newTopic = String(mqttname + "/data/precharging_status");
    client.publish(newTopic.c_str(), cellStr.c_str());
    precharging_status = byte_value;
  }

  // ignore 54 byte
  index += 54;

  fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8) * 0.1;
  if (fl_value != temp_sensor5 || temp_sensor5 == 0)
  {
    Serial.print("tempSensor5: ");
    Serial.println(fl_value);
    cellStr = String(fl_value, 3);
    newTopic = String(mqttname + "/data/temp_sensor5");
    client.publish(newTopic.c_str(), cellStr.c_str());
    temp_sensor5 = fl_value;
  }

  fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8) * 0.1;
  if (fl_value != temp_sensor4 || temp_sensor4 == 0)
  {
    Serial.print("tempSensor4: ");
    Serial.println(fl_value);
    cellStr = String(fl_value, 3);
    newTopic = String(mqttname + "/data/temp_sensor4");
    client.publish(newTopic.c_str(), cellStr.c_str());
    temp_sensor4 = fl_value;
  }

  fl_value = (receivedBytes_cell[index++] | receivedBytes_cell[index++] << 8) * 0.1;
  if (fl_value != temp_sensor3 || temp_sensor3 == 0)
  {
    Serial.print("tempSensor3: ");
    Serial.println(fl_value);
    cellStr = String(fl_value, 3);
    newTopic = String(mqttname + "/data/temp_sensor3");
    client.publish(newTopic.c_str(), cellStr.c_str());
    temp_sensor3 = fl_value;
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
