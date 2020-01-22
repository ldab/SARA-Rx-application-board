# SARA-Rx application board

 SARA-R4 NB-IoT + Cat-M1 + GNSS application board - NINA-B3 with Secure MQTT

[![GitHub version](https://img.shields.io/github/release/ldab/SARA-Rx-application-board.svg)](https://github.com/ldab/SARA-Rx-application-board/releases/latest)
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](https://github.com/ldab/SARA-Rx-application-board/blob/master/LICENSE)

[![GitHub last commit](https://img.shields.io/github/last-commit/ldab/SARA-Rx-application-board.svg?style=social)](https://github.com/ldab/SARA-Rx-application-board)

## TODO

- [ ] More details about battery etc...
- [ ] !!! Better Socket close URC handler.
- [ ] Check AT response based on the last chars, in order to avoid missing URCs when after "OK" for example
- [ ] Implement Subscribe;

![](./pics/mqtt_RTT-print.png)

## Energy Budget

* 120mA active current;
* Code takes 15 seconds to execute;
* Wakes every hour;
* 2mA Sleep current *This needs to be optimized*;
* 10 hours of light available;
* V charge ready = 3.67V;
* V Over discharge = 3.6V;
* V Over charge = 4.12V;

**Source Power Required = 34mW**
**Storage Required = 42.28mAh**

## Nordic SDK

* This is based on nRF5_SDK_16.0.0_98a08e2, get yours here: https://www.nordicsemi.com/Software-and-tools/Software/nRF5-SDK/Download

## Amazon AWS and MQTT

1. Sign in to the AWS IoT Console
2. Register a Device in the Registry
3. Configure Your Device
4. View Device MQTT Messages with the AWS IoT MQTT Client

https://docs.aws.amazon.com/iot/latest/developerguide/iot-gs.html

## Running the code locally

1. Clone this repo to a local folder ```> git clone https://github.com/ldab/SARA-Rx-application-board```
2. Point the `$(SDK)` @`./code/ninab3/blank/ses/libuarte_pca10056.emProject` to where you have installed Nordic SDK, for example `C:\nRF\nRF5_SDK_16.0.0_98a08e2`
3. Like so: `macros="CMSIS_CONFIG_TOOL=C:/nRF/nRF5_SDK_16.0.0_98a08e2/external_tools/cmsisconfig/CMSIS_Configuration_Wizard.jar;SDK=C:/nRF/nRF5_SDK_16.0.0_98a08e2"`
4. Open the SEGGER Embedded Studio project file located in `./code/ninab3/blank/ses/libuarte_pca10056.emProject`

Ref. https://devzone.nordicsemi.com/f/nordic-q-a/44638/how-to-move-an-sdk-example-out-of-the-sdk-tree

## Efficiency

<img src="./pics/effchart.png" width="75%"> 

## Schematic

[![Board BOM](./pics/BOM.png)](./KiCad/BOM.csv)

## PCB

<img src="./pics/pcb.png" alt="3D PCB" width="50%"> 

## BOM

[![Board BOM](./pics/BOM.png)](./KiCad/BOM.csv)

## Enclosure

<img src="./pics/enclosure.png" alt="3D" width="50%"> 

## Kown issues, limintations

* MQTT Socket Implementaition can only **publish** QoS 0 messages.

## Final Thoughts or Improvements

* Use STATUS 2 of AEM10941 as last grasp;
* Supply NINA from the same 3.xV from the Boost Converter;
* Control boost converter EN, in order to turn SARA Off;
* SARA RESET Pin access;
* Open Drain 74LVC3G07 buffer on NINA RGB, in order to avoid leakage.

## Credits

* [MQTTBox](http://workswithweb.com/mqttbox.html) for Windows in order to communicate with the broker and check messages;
* [CloudMQTT](https://www.cloudmqtt.com/) Free Cloud Broker;
* MQTT implementation inspired and based on [knolleary PubSub Client](https://github.com/knolleary/pubsubclient);
* AT Commands implementation inspired on [vshymanskyy TinyGSM](https://github.com/vshymanskyy/TinyGSM);
* GitHub Shields and Badges created with [Shields.io](https://github.com/badges/shields/);
