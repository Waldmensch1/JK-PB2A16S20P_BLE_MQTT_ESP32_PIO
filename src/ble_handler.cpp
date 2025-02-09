#include "ble_handler.h"

// BMS-BLE Settings
const char *devicename = DEVICENAME; // JK-B2A24S20P JK-B2A24S15P

// BLE Data
byte receivedBytes_cell[320];
byte receivedBytes_device[320];

int frame = 0;
bool received_start = false;
bool received_start_frame = false;
bool received_complete = false;
bool new_data = false;
byte BLE_Scan_counter = 0;

const byte CONFIGDATA = 0x01;
const byte CELLDATA = 0x02;
const byte DEVICEDATA = 0x03;
byte actualdata = 0x00;

// BLE
static BLERemoteCharacteristic *pRemoteCharacteristic;
static BLEAdvertisedDevice *myDevice;
static BLEUUID serviceUUID("ffe0"); // The remote service we wish to connect to.
static BLEUUID charUUID("ffe1");    // ffe1 // The characteristic of the remote service we are interested in.
BLEClient *pClient;
BLEScan *pBLEScan;
byte getdeviceInfo[20] = {0xaa, 0x55, 0x90, 0xeb, 0x97, 0x00, 0xdf, 0x52, 0x88, 0x67, 0x9d, 0x0a, 0x09, 0x6b, 0x9a, 0xf6, 0x70, 0x9a, 0x17, 0xfd}; // Device Infos
byte getInfo[20] = {0xaa, 0x55, 0x90, 0xeb, 0x96, 0x00, 0x79, 0x62, 0x96, 0xed, 0xe3, 0xd0, 0x82, 0xa1, 0x9b, 0x5b, 0x3c, 0x9c, 0x4b, 0x5d};

static bool doConnect = false;
static bool ble_connected = false;
unsigned long sendingtime = 0;
unsigned long bleScantime = 0;
unsigned long mqttpublishtime = 0;
unsigned long newdatalasttime = 0;
unsigned long ble_connection_time = 0;
bool blocked_for_parsing = false;
bool sent_once = false;

void analyze()
{
    if (actualdata == CELLDATA)
        readCellDataRecord();
    else if (actualdata == DEVICEDATA)
        readDeviceDataRecord();
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
        mqtt_client.publish((mqttname + "/status/ble_connection").c_str(), "connected");
    }

    void onDisconnect(BLEClient *pclient)
    {
        ble_connected = false;
        // pclient->disconnect();
        mqtt_client.publish((mqttname + "/status/ble_connection").c_str(), "disconnected");
        Serial.println("BLE-Disconnect");
        mqtt_client.publish((mqttname + "/status/ble_connection").c_str(), "rebooting");
        Serial.println("BLE was Disconnected ... and no BLE reconnection possible, Reboot ESP...");
        ESP.restart();
    }
};

// Scan for BLE servers and find the first one that advertises the service we are looking for.
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{

    // Called for each advertising BLE server.

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

void ble_setup()
{
    BLEDevice::init("");
    pClient = BLEDevice::createClient();
    Serial.println("BLE client started");

    pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setInterval(1349);
    pBLEScan->setWindow(449);
    pBLEScan->setActiveScan(true);
}

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
        if (pClient != nullptr)
        {
            pClient->disconnect();
        }
        else
        {
            Serial.println("Error: pClient is NULL");
        }
        return false;
    }
    Serial.println(" - Found our service");

    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr)
    {
        Serial.print("Failed to find our characteristic UUID: ");
        Serial.println(charUUID.toString().c_str());
        if (pClient != nullptr)
        {
            pClient->disconnect();
        }
        else
        {
            Serial.println("Error: pClient is NULL");
        }
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
    mqtt_client.publish((mqttname + "/status/ble_connection").c_str(), "connected");
    return true;
}

void ble_loop()
{
    if (doConnect == true)
    {
        if (!connectToBLEServer())
        {
            Serial.println("We have failed to connect to the server; there is nothin more we will do.");
            mqtt_client.publish((mqttname + "/status/ble_connection").c_str(), "BLE_Connection_error!");
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
                    mqtt_client.publish((mqttname + "/status/ble_connection").c_str(), "connected");
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

    // BLE not connected
    if ((!ble_connected && !doConnect && (millis() - bleScantime) > 15000))
    {

        Serial.println("BLE -> Reconnecting ... " + String(BLE_Scan_counter));
        mqtt_client.publish((mqttname + "/status/ble_connection").c_str(), "reconnecting");
        bleScantime = millis();
        if (pBLEScan != nullptr)
        {
            pBLEScan->start(5, false);
        }
        else
        {
            Serial.println("Error: pBLEScan is NULL");
        }
        BLE_Scan_counter++;
        sent_once = false;
    }

    // BLE connected but no new data since 60 seconds
    if (!doConnect && ble_connected && (millis() >= (newdatalasttime + 60000)) && newdatalasttime != 0)
    {
        ble_connected = false;
        delay(200);
        mqtt_client.publish((mqttname + "/status/ble_connection").c_str(), "terminated");
        Serial.println("BLE-Disconnect/terminated");
        newdatalasttime = millis();
        if (pClient != nullptr)
        {
            pClient->disconnect();
        }
        else
        {
            Serial.println("Error: pClient is NULL");
        }
        sent_once = false;
    }

    // ESP restart after REBOOT_AFTER_BLE_RETRY BLE Scans without success
    if (BLE_Scan_counter > REBOOT_AFTER_BLE_RETRY)
    {
        mqtt_client.publish((mqttname + "/status/ble_connection").c_str(), "rebooting");
        Serial.println("BLE isn´t receiving new Data form BMS... and no BLE reconnection possible, Reboot ESP...");
        ESP.restart();
    }
}
