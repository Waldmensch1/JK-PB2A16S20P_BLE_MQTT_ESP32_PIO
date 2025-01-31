# JK-PB2A16S20P_BLE_MQTT_ESP32_PIO

## Description
This project is inspired by the Akkudoktor.net forum and especially by this thread: [JKBMS auslesen über BLE Bluetooth oder RS485 Adapter mittels EPS IoBroker](https://akkudoktor.net/t/jkbms-auslesen-uber-ble-bluetooth-oder-rs485-adapter-mittels-eps-iobroker/722). As the the code there does not work properly with JK-PB2A16S20P (these are delivered with a lot of DIY battery boxes).

I tested it with HW Revision 15A and Firmware 15.37 and it does so far. If you use more than one ESP32 it is shown as a structure in MQTT

jk_ble_listener
    +---DEVICENAME1
    |
    +---DEVICENAME2

## Table of Contents
- [Installation](#installation)
- [Usage](#usage)
- [Contributing](#contributing)
- [License](#license)

## Installation
- It's a normal PIO project. So use VS-Code with PIO Ad-On to build and flash. 

## Usage
- Create a config.h file in folder includes. A sample file exist, you can modify and rename it.
- Modify the platformio.ini for your needs. Especially set the DEVICENAME e.g. JK-PB2A16S20P-01. This overwrites the DEVICENAME specified in /include/config.h. I did that because I have 2 build targets for 2 different ESP32 boards
- Specify COM ports in platformio.ini
- Upload to your ESP32

## Contributing
The parsing is been done mostly aligned to this protocol documentation https://github.com/syssi/esphome-jk-bms/blob/main/docs/protocol-design-ble.md but there may be some glitches in parsing. Feel free to contribute

## License
This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.