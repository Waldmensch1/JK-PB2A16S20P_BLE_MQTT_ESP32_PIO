#include "ble_client.h"

// strings
const char *devicename = DEVICENAME;

// status flags
bool do_connect = false;
bool ble_connected = false;
bool capturing = false;
bool initial_send_done = false; // Flag to indicate if the initial send has occurred

// counter
int count_ble_scans = 0;

// BLE
static NimBLEUUID serviceUUID("ffe0"); // The remote service we wish to connect to.
static NimBLEUUID charUUID("ffe1");    // ffe1 // The characteristic of the remote service we are interested in.
NimBLEAdvertisedDevice *myDevice;
NimBLEScan *pBLEScan;
NimBLEClient *pClient;
NimBLERemoteCharacteristic *pRemoteCharacteristic;

// messages
byte getdeviceInfo[20] = {0xaa, 0x55, 0x90, 0xeb, 0x97, 0x00, 0xdf, 0x52, 0x88, 0x67, 0x9d, 0x0a, 0x09, 0x6b, 0x9a, 0xf6, 0x70, 0x9a, 0x17, 0xfd}; // Device Infos
byte getInfo[20] = {0xaa, 0x55, 0x90, 0xeb, 0x96, 0x00, 0x79, 0x62, 0x96, 0xed, 0xe3, 0xd0, 0x82, 0xa1, 0x9b, 0x5b, 0x3c, 0x9c, 0x4b, 0x5d};

// Buffer
std::mutex bufferMutex;
uint8_t ble_buffer[BUFFER_SIZE];
int ble_buffer_index = 0;

// Time variables
unsigned long last_received_notification = 0;
unsigned long last_sending_time = 0;
unsigned long last_ble_connection_attempt_time = 0;
unsigned long last_ble_scan_time = 0;
unsigned long last_data_received_time = 0;

#ifdef DUALCORE
// Define the queue handle
QueueHandle_t bleQueue;
#endif

void parser01(void *message) {
    DEBUG_PRINTLN("Parser1");
    readConfigDataRecord(message, devicename);
}

void parser02(void *message) {
    DEBUG_PRINTLN("Parser2");
    readCellDataRecord(message, devicename);
}

void parser03(void *message) {
    DEBUG_PRINTLN("Parser3");
    readDeviceDataRecord(message, devicename);
}

// forks into message parser by message type
void parser(void *message) {
    uint8_t *msg = static_cast<uint8_t *>(message);
    uint8_t type = msg[4]; // 4. Byte decides the message type

    switch (type) {
    case 0x01:
        parser01(message);
        break;
    case 0x02:
        parser02(message);
        break;
    case 0x03:
        parser03(message);
        break;
    default:
        DEBUG_PRINTLN("Unbekannter Typ in message[3]!");
        break;
    }
}

class MyClientCallback : public NimBLEClientCallbacks {
    void onConnect(NimBLEClient *pclient) {
        DEBUG_PRINTLN("Connected");
        ble_connected = true;
    }

    void onDisconnect(NimBLEClient *pclient) {
        DEBUG_PRINTLN("Disconnected");
        ble_connected = false;
        ESP.restart();
    }
};

class MyAdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks {
    void onResult(NimBLEAdvertisedDevice *advertisedDevice) {
        DEBUG_PRINT("BLE Advertised Device found: ");
        DEBUG_PRINTLN(advertisedDevice->toString().c_str());

        // We have found a device, let us now see if it contains the service we are looking for.
        if (advertisedDevice->haveServiceUUID() && advertisedDevice->isAdvertisingService(NimBLEUUID(serviceUUID)) && advertisedDevice->getName() == devicename) {
            DEBUG_PRINTLN("Found our server " + String(devicename));
            pBLEScan->stop();
            myDevice = advertisedDevice;
            do_connect = true;
            // pBLEScan->clearResults(); // that seems to kill the pointer to the device
        } // Found our server
    } // onResult
}; // MyAdvertisedDeviceCallbacks

void notifyCallback(NimBLERemoteCharacteristic *pRemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify) {

    // DEBUG_PRINT("Notification received: ");
    // for (size_t i = 0; i < length; i++) {
    //     DEBUG_PRINT(pData[i], HEX);
    //     DEBUG_PRINT(" ");
    // }
    // DEBUG_PRINTLN();

    std::lock_guard<std::mutex> lock(bufferMutex);
    last_received_notification = millis(); // Zeitstempel aktualisieren

    for (size_t i = 0; i < length; i++) {
        uint8_t byte = pData[i];

        if (!capturing) {
            // check for start sequence
            if (ble_buffer_index == 0 && byte == 0x55) {
                ble_buffer[ble_buffer_index++] = byte;
            } else if (ble_buffer_index == 1 && byte == 0xaa) {
                ble_buffer[ble_buffer_index++] = byte;
            } else if (ble_buffer_index == 2 && byte == 0xeb) {
                ble_buffer[ble_buffer_index++] = byte;
            } else if (ble_buffer_index == 3 && byte == 0x90) {
                ble_buffer[ble_buffer_index++] = byte;
                DEBUG_PRINTLN("Startsequence detected! Message type: " + String(ble_buffer[ble_buffer_index]));
                capturing = true; // from now on, data will be stored in buffer
            } else {
                ble_buffer_index = 0; // no start sequence detected, reset buffer
            }
        } else {
            // write byte to buffer
            ble_buffer[ble_buffer_index++] = byte;

            // if 320 bytes received, call parser
            if (ble_buffer_index >= BUFFER_SIZE) {
                // copy buffer to message
                std::vector<uint8_t> message(ble_buffer, ble_buffer + BUFFER_SIZE);
                ble_buffer_index = 0;
                capturing = false; // waiting for next start sequence
                last_data_received_time = millis();

                // .. and call parser
                DEBUG_PRINTLN("Calling parser");
#ifdef DUALCORE
                // Add message to queue
                if (xQueueSend(bleQueue, message.data(), portMAX_DELAY) != pdTRUE) {
                    DEBUG_PRINTLN("Failed to send message to queue");
                }
#else
                parser(static_cast<void *>(message.data()));
#endif
            }
        }
    }
}

bool connectToBLEServer() {
    DEBUG_PRINT("Forming a connection to ");
    DEBUG_PRINTLN(myDevice->getAddress().toString().c_str());

    pClient->setClientCallbacks(new MyClientCallback());
    delay(500);

    // Connect to the remote BLE Server.
    if (pClient->connect(myDevice)) {

        mqtt_client.publish((mqttname + "/status/ble_device_mac").c_str(), myDevice->getAddress().toString().c_str());
        mqtt_client.publish((mqttname + "/status/ble_device_rssi").c_str(), String(myDevice->getRSSI()).c_str());

        DEBUG_PRINTLN(" - Connected to server");

        // Obtain a reference to the service we are after in the remote BLE server.
        NimBLERemoteService *pRemoteService = pClient->getService(NimBLEUUID(serviceUUID));
        if (pRemoteService == nullptr) {
            DEBUG_PRINT("Failed to find our service UUID: ");
            DEBUG_PRINTLN(serviceUUID.toString().c_str());
            pClient->disconnect();
            return false;
        }
        DEBUG_PRINTLN(" - Found our service");

        // Obtain a reference to the characteristic in the service of the remote BLE server.
        pRemoteCharacteristic = pRemoteService->getCharacteristic(NimBLEUUID(charUUID));
        if (pRemoteCharacteristic == nullptr) {
            DEBUG_PRINT("Failed to find our characteristic UUID: ");
            DEBUG_PRINTLN(charUUID.toString().c_str());
            pClient->disconnect();
            return false;
        }
        DEBUG_PRINTLN(" - Found our characteristic");

        // Set the notification callback
        if (pRemoteCharacteristic->canNotify()) {
            pRemoteCharacteristic->subscribe(true, notifyCallback);
            DEBUG_PRINTLN("Subscribed to notifications");
        }

        // Sending getdevice info
        pRemoteCharacteristic->writeValue(getdeviceInfo, 20);
        initial_send_done = false;
        last_sending_time = millis();
        DEBUG_PRINTLN("Sending device Info");

        mqtt_client.publish((mqttname + "/status/ble_connection").c_str(), "connected");

#ifdef USELED
        // Send LED_BLINK_SLOW state to the LED task
        set_led(LedState::LED_BLINK_SLOW);
#endif

        return true;
    } else {
        DEBUG_PRINTLN(" - Failed to connect to server");
        mqtt_client.publish((mqttname + "/status/ble_connection").c_str(), "failed");
        return false;
    }
}

void ble_loop() {
    // BLE not connected
    if (!ble_connected && !do_connect && (millis() - last_ble_scan_time) > SCAN_REPEAT_TIME) {
        DEBUG_PRINTLN("BLE -> Scanning ... " + String(count_ble_scans));
        mqtt_client.publish((mqttname + "/status/ble_connection").c_str(), String("scanning " + String(count_ble_scans)).c_str());
        last_ble_scan_time = millis();
        if (pBLEScan != nullptr) {
            pBLEScan->start(5, false);

#ifdef USELED
            // Send LED_BLINK_FAST state to the LED task
            set_led(LedState::LED_BLINK_FAST);
#endif

        } else {
            DEBUG_PRINTLN("Error: pBLEScan is NULL");
        }
        count_ble_scans++;
    }

    // BLE connected but no new data since 60 seconds
    if (!do_connect && ble_connected && (millis() >= (last_data_received_time + TIMEOUT_NO_DATA)) && last_data_received_time != 0) {
        ble_connected = false;
        delay(200);
        mqtt_client.publish((mqttname + "/status/ble_connection").c_str(), "terminated");
        DEBUG_PRINTLN("BLE-Disconnect/terminated");
        last_data_received_time = millis();
        if (pClient != nullptr) {
            pClient->disconnect();
        } else {
            DEBUG_PRINTLN("Error: pClient is NULL");
        }
    }

    // ESP restart after REBOOT_AFTER_BLE_RETRY BLE Scans without success
    if (count_ble_scans > REBOOT_AFTER_BLE_RETRY) {
        mqtt_client.publish((mqttname + "/status/ble_connection").c_str(), "rebooting");
        DEBUG_PRINTLN("BLE not receiving new Data from BMS... and no BLE reconnection possible, Reboot ESP...");
        ESP.restart();
    }

    // Attempt to connect if a device is found
    if (do_connect) {
        unsigned long currentMillis = millis();
        if (currentMillis - last_ble_connection_attempt_time >= BLE_RECONNECT) {
            last_ble_connection_attempt_time = currentMillis;
            if (connectToBLEServer()) {
                DEBUG_PRINTLN("We are now connected to the BLE Server.");
                do_connect = false;
            } else {
                DEBUG_PRINTLN("Failed to connect to the BLE Server.");
            }
        }
    }

    if (ble_connected) {
        // request cell data 5 seconds after request for device data

        if (!initial_send_done) {
            // Initial send after 5 seconds
            if ((millis() - last_sending_time) >= INITIAL_SEND_INTERVAL) {
                DEBUG_PRINTLN("Send getInfo (initial)");
                pRemoteCharacteristic->writeValue(getInfo, 20);
                last_sending_time = millis(); // Update the last sending time to the current time
                initial_send_done = true;     // Set the flag to indicate the initial send is done
            }
        } else {
            // Subsequent sends every hour
            if ((millis() - last_sending_time) >= REPEAT_SEND_INTERVAL) {
                DEBUG_PRINTLN("Send getInfo (hourly)");
                pRemoteCharacteristic->writeValue(getInfo, 20);
                last_sending_time = millis(); // Update the last sending time to the current time
            }
        }
    }
}

// Watchdog for buffer timeout, runs in a separate thread
void bufferTimeoutCheck() {
    while (true) {
        delay(100); // check every 100ms

        std::lock_guard<std::mutex> lock(bufferMutex);
        if (capturing && (millis() - last_received_notification > BUFFER_TIMEOUT)) {
            // Timeout reached
            ble_buffer_index = 0;
            capturing = false;
            DEBUG_PRINTLN("Notfication Timeout reached! Buffer reset.");
        }
    }
}

#ifdef DUALCORE
// Define the BLE client task
void bleClientTask(void *pvParameters) {
    NimBLEDevice::init("");
    pClient = NimBLEDevice::createClient();
    pClient->setClientCallbacks(new MyClientCallback());
    DEBUG_PRINTLN("BLE client started");

    pBLEScan = NimBLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setInterval(1349);
    pBLEScan->setWindow(449);
    pBLEScan->setActiveScan(true);

    DEBUG_PRINTLN("Scan for our Server " + String(devicename));

    // Keep the task running
    while (true) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

// Define the parser task
void parserTask(void *pvParameters) {
    uint8_t message[BUFFER_SIZE];

    while (true) {
        // Receive data from the queue
        if (xQueueReceive(bleQueue, &message, portMAX_DELAY) == pdTRUE) {
            // Call the parser function
            parser(message);
        }
    }
}
#endif

void ble_setup() {

#ifdef DUALCORE
    // Create the queue
    bleQueue = xQueueCreate(10, sizeof(uint8_t[BUFFER_SIZE]));
    DEBUG_PRINTLN("ble queue created");

    // Create the BLE client task on core 1
    xTaskCreatePinnedToCore(bleClientTask, "BLE Client Task", 8192, NULL, 1, NULL, 1);
    DEBUG_PRINTLN("BLE Client Task created");

    // Create the parser task on core 0
    xTaskCreatePinnedToCore(parserTask, "Parser Task", 4096, NULL, 1, NULL, 0);
    DEBUG_PRINTLN("Parser Task created");

#else

    NimBLEDevice::init("");
    pClient = NimBLEDevice::createClient();
    pClient->setClientCallbacks(new MyClientCallback());
    DEBUG_PRINTLN("BLE client started");

    pBLEScan = NimBLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setInterval(1349);
    pBLEScan->setWindow(449);
    pBLEScan->setActiveScan(true);

    DEBUG_PRINTLN("Scan for our Server " + String(devicename));

#endif

    // start timeout check in separate thread
    xTaskCreatePinnedToCore(
        [](void *) { bufferTimeoutCheck(); }, // function as lambda
        "BufferTimeout",                      // taskname
        2048,                                 // stacksize
        nullptr,
        1, // priority
        nullptr,
        1 // use core 1, so BLE can run on core 0
    );
}
