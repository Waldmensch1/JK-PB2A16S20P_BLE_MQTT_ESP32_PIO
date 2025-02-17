#include "rs485_handler.h"
#ifdef USE_RS485
#define RS485_BUFFER_SIZE 320

// Define the RS485 serial port
HardwareSerial RS485Serial(2); // Use UART2 (GPIO16: RX2, GPIO17: TX2)

byte message2[21] = {0x4E, 0x57, 0x00, 0x13, 0x00, 0x00, 0x00, 0x00, 0x06, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x68, 0x00, 0x00, 0x01, 0x29}; // read all
byte message1[21] = {0x4E, 0x57, 0x00, 0x13, 0x00, 0x00, 0x00, 0x00, 0x03, 0x03, 0x00, 0x83, 0x00, 0x00, 0x00, 0x00, 0x68, 0x00, 0x00, 0x01, 0xA9}; // Read total voltage
byte buffer[RS485_BUFFER_SIZE];
int receivedLength = 0;

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
    DEBUG_PRINTLN("RS485 Serial initialized");

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
            if (receivedLength < RS485_BUFFER_SIZE) {
                buffer[receivedLength++] = data;
            }
            lastReceiveTime = millis();
        }
    }

    // Check if the receive timeout has elapsed
    if (!receivedData.isEmpty() && millis() - lastReceiveTime > timeout) {
        // ignore too short messages
        if (receivedLength > 11) {
            // Create a fresh array for further processing
            byte freshArray[receivedLength];
            memcpy(freshArray, buffer, receivedLength);

            // Process the fresh array
            DEBUG_PRINT("Received data: ");
            for (int i = 0; i < receivedLength; i++) {
                DEBUG_PRINT(freshArray[i], HEX);
                if (i < receivedLength - 1) {
                    DEBUG_PRINT(",");
                }
            }
            DEBUG_PRINTLN();
        }

        // Reset the buffer
        receivedLength = 0;
    }
}

#endif // USE_RS485