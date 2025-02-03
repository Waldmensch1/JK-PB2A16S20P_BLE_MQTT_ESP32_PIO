#include "main.h"
#include "parser.h"

void handleMQTTMessage(const char *topic, const char *command, byte *payload, unsigned int length, bool &flag)
{
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
}

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
    return;

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
      actualdata = CELLDATA;
    else if (pData[4] == DEVICEDATA)
      actualdata = DEVICEDATA;

    for (int i = 0; i < length; i++)
    {
      if (actualdata == CELLDATA)
        receivedBytes_cell[frame] = pData[i];
      else if (actualdata == DEVICEDATA)
        receivedBytes_device[frame] = pData[i];

      frame++;
    }
  }

  if (received_start && !received_start_frame && !received_complete)
  {
    //     Serial.println("Daten erweitert !");
    for (int i = 0; i < length; i++)
    {
      if (actualdata == CELLDATA)
        receivedBytes_cell[frame] = pData[i];
      else if (actualdata == DEVICEDATA)
        receivedBytes_device[frame] = pData[i];

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
    client.publish((mqttname + "/status/ble_connection").c_str(), "connected");
  }

  void onDisconnect(BLEClient *pclient)
  {
    ble_connected = false;
    // pclient->disconnect();
    client.publish((mqttname + "/status/ble_connection").c_str(), "disconnected");
    Serial.println("BLE-Disconnect");
    client.publish((mqttname + "/status/ble_connection").c_str(), "rebooting");
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
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID) && advertisedDevice.getName() == devicename)
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

  // MQTT Setup
  mqtt_buffer_maxed = client.setBufferSize(500);
  Serial.println("mqtt_buffer_maxed: " + String(mqtt_buffer_maxed));

  // BLE Setup
  BLEDevice::init("");
  pClient = BLEDevice::createClient();
  Serial.println("BLE client created");

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
      client.publish((mqttname + "/status/ble_connection").c_str(), "BLE_Connection_error!");
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

        if (!sent_once)
        {
          client.publish((mqttname + "/status/ble_connection").c_str(), "connected");
          client.publish((mqttname + "/status/status").c_str(), "online");
          client.publish((mqttname + "/status/ipaddress").c_str(), WiFi.localIP().toString().c_str());
          sent_once = true;
        }
      }
    }

    // BLE Get Device Data Trigger ...
    if (((millis() - sendingtime) > 5000) && sendingtime != 0)
    {
      sendingtime = 0;
      Serial.println("gesendet!");
      pRemoteCharacteristic->writeValue(getInfo, 20);
    }
  }

  // BLE nicht verbunden
  if ((!ble_connected && !doConnect && (millis() - bleScantime) > 15000))
  {

    Serial.println("BLE -> Reconnecting!");
    client.publish((mqttname + "/status/ble_connection").c_str(), "reconnecting");
    bleScantime = millis();
    pBLEScan->start(5, false);
    BLE_Scan_counter++;
    sent_once = false;
  }

  // BLE verbidnugn ist da aber es kommen seit X Sekunden keine neuen Daten !
  if (!doConnect && ble_connected && (millis() >= (newdatalasttime + 60000)) && newdatalasttime != 0)
  {
    ble_connected = false;
    delay(200);
    client.publish((mqttname + "/status/ble_connection").c_str(), "terminated");
    Serial.println("BLE-Disconnect/terminated");
    newdatalasttime = millis();
    pClient->disconnect();
    sent_once = false;
  }

  // checker das nach max 5 Minuten und keiner BLE Verbidung neu gestartet wird...
  if (BLE_Scan_counter > 20)
  {
    client.publish((mqttname + "/status/ble_connection").c_str(), "rebooting");
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
  client.publish((mqttname + "/status/ble_connection").c_str(), "connected");
  return true;
}

////// analyze

void analyze()
{
  if (actualdata == CELLDATA)
    readCellDataRecord();
  else if (actualdata == DEVICEDATA)
    readDeviceDataRecord();
}

////// MQTTCLIENT

boolean reconnect()
{
  if (client.connect(mqttname.c_str(), mqtt_username, mqtt_passwort, willTopic.c_str(), willQoS, willRetain, willMessage.c_str()))
  {
    // Once connected, publish an announcement...
    if (millis() < 10000)
      client.publish((mqttname + "/status/ble_connection").c_str(), "startup");

    String debug_flg_status = debug_flg ? "true" : "false";
    client.publish((mqttname + "/parameter/debugging_active").c_str(), debug_flg_status.c_str());
    client.subscribe((mqttname + "/parameter/debugging_active").c_str()); // debug_flg

    String debug_flg_full_log_status = debug_flg_full_log ? "true" : "false";
    client.publish((mqttname + "/parameter/debugging_active_full").c_str(), debug_flg_full_log_status.c_str());
    client.subscribe((mqttname + "/parameter/debugging_active_full").c_str()); // debug_flg_full_log

    client.publish((mqttname + "/status/status").c_str(), "online");

    if (debug_flg)
      Serial.println("MQTT reconnected!");
  }
  else
  {
    if (debug_flg)
      Serial.println("MQTT connection failed!");
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
