#include "rs485_handler.h"
#ifdef USE_RS485
#define RS485_BUFFER_SIZE 300

// Define the RS485 serial port
HardwareSerial RS485Serial(2); // Use UART2 (GPIO16: RX2, GPIO17: TX2)


byte message2[21] = {0x4E, 0x57, 0x00, 0x13, 0x00, 0x00, 0x00, 0x00, 0x06, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x68, 0x00, 0x00, 0x01, 0x29}; // read all
byte message1[21] = {0x4E, 0x57, 0x00, 0x13, 0x00, 0x00, 0x00, 0x00, 0x03, 0x03, 0x00, 0x83, 0x00, 0x00, 0x00, 0x00, 0x68, 0x00, 0x00, 0x01, 0xA9}; // Read total voltage
byte receivedBytes_main[RS485_BUFFER_SIZE];

void rs485_send(const byte *data, size_t length) {
    // Enable transmitter
    digitalWrite(DE_PIN, HIGH);
    digitalWrite(RE_PIN, HIGH);
    RS485Serial.write(data, length);
    RS485Serial.flush(); // Wait for the transmission to complete
    // Enable receiver
    digitalWrite(DE_PIN, LOW);
    digitalWrite(RE_PIN, LOW);
}


void init_rs485() {
    // Initialize RS485 serial port
    RS485Serial.begin(115200, SERIAL_8N1, RS485_RX, RS485_TX); // Baud rate: 115200, RX: GPIO16, TX: GPIO17
    pinMode(DE_PIN, OUTPUT);
    pinMode(RE_PIN, OUTPUT);
    digitalWrite(DE_PIN, LOW); // Enable receiver
    digitalWrite(RE_PIN, LOW); // Enable receiver
    Serial.println("RS485 Serial initialized");

    rs485_send(message2, sizeof(message2));
}

void rs485_loop() {
    static String receivedData = "";
    static unsigned long lastReceiveTime = 0;
    const unsigned long timeout = 100; // Timeout in milliseconds

    // Check if data is available
    if (RS485Serial.available()) {
        // Enable receiver
        digitalWrite(DE_PIN, LOW);
        digitalWrite(RE_PIN, LOW);
        // Read bytes from RS485 serial port
        while (RS485Serial.available()) {
            byte data = RS485Serial.read();
            if (!receivedData.isEmpty()) {
                receivedData += ",";
            }
            receivedData += String(data, HEX);
            lastReceiveTime = millis();
        }
    }

    // Check if the receive timeout has elapsed
    if (!receivedData.isEmpty() && millis() - lastReceiveTime > timeout) {
        // Publish the received data
        mqtt_client.publish((mqttname + "/rs485/data").c_str(), receivedData.c_str());
        Serial.print("Published RS485 data: ");
        Serial.println(receivedData);

        // Clear the buffer
        receivedData = "";
    }
}


#endif // USE_RS485