# JK Inverter BMS Bluetooth to MQTT gateway

## Description
This project is inspired by the Akkudoktor.net forum and especially by this thread: [JKBMS auslesen über BLE Bluetooth oder RS485 Adapter mittels EPS IoBroker](https://akkudoktor.net/t/jkbms-auslesen-uber-ble-bluetooth-oder-rs485-adapter-mittels-eps-iobroker/722). As the the code there did not work properly with my JK-PB2A16S20P (these are delivered with a lot of DIY battery boxes), this project has been raised. For details on supported JK inverter BMS models see [table below](#supported-jk-bms-models).  
The communication between the BMS and the ESP32 is primarily designed for bluetooth, thus an ESP32 model with BT support is required.  
Beside the mandatory MQTT communication, an optional InfluxDB client is included which can be activated in the [config.h file](#usage).  
If you use more than one ESP32 it is shown as a structure in MQTT:  
```
jk_ble_listener
    +---DEVICENAME1
    |
    +---DEVICENAME2
```

## Table of Contents
- [Installation](#installation)
- [Usage](#usage)
- [Supported JK BMS Models](#supported-jk-bms-models)
- [Contributing](#contributing)
- [License](#license)

## Installation
Use VS-Code with [PlatformIO](https://platformio.org/install/ide?install=vscode) Add-On to build and flash the firmware.

## Usage
- Create a config.h file in folder includes. A sample file exist, you can modify and rename it. The available configuration options are well documented in the sample file.
- Modify the platformio.ini for your needs. Especially set the `DEVICENAME` (e.g. `JK-PB2A16S20P-01`), which is the Bluetooth name **as shown by the JK-BMS smartphone app**. This overwrites the `DEVICENAME` specified in `/include/config.h`. This will allow you to build targets for multiple ESP32 boards
- Specify COM ports in platformio.ini; may be deleted to enabled auto-detect (if you only have one ESP connected to your host)
- Build and upload to your ESP32
  
**Attention:** Do note that you will not be able to connect to the BMS with your smartphone app while the ESP32 is communicating with your BMS.

## Supported JK BMS Models
This project has been primarily designed to be used for the JK-PB2A16S20P, however it may work with different models of the **JK Inverter BMS series**.  
Here is a list of currently evaluated BMS models along with the tested hard- and firmware revisions:

| Model         | HW-Rev. | FW-Ver | Status |
| ------------- | ------- | ------ | ------ |
| JK-PB2A16S20P | 15A     | 15.37  | supported |
| JK-PB2A16S15P | 15A     | 15.38  | supported |

If you are running a different BMS model, especially with different hardware (rev. 14) or different firmware, feel free to test the project and provide feedback by raising an issue.  
**Note:** The most promising precondition to successfully use this project is probably by running a **recent firmware** on your BMS. You may check [Andy's homepage](https://off-grid-garage.com/battery-management-systems-bms/) for firmware updates for the JK inverter BMS series.

## Contributing
The parsing is been done mostly aligned to this protocol documentation https://github.com/syssi/esphome-jk-bms/blob/main/docs/protocol-design-ble.md but there may be some glitches in parsing. Feel free to contribute

## License
This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.