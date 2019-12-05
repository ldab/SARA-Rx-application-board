# SARA-R5 application board

 SARA-R5 NB-IoT + Cat-M1 + GNSS application board

[![GitHub version](https://img.shields.io/github/release/ldab/SARA-R5-application-board.svg)](https://github.com/ldab/SARA-R5-application-board/releases/latest)
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](https://github.com/ldab/SARA-R5-application-board/blob/master/LICENSE)

[![GitHub last commit](https://img.shields.io/github/last-commit/ldab/SARA-R5-application-board.svg?style=social)](https://github.com/ldab/SARA-R5-application-board)

![PCB](./pics/esp_pmc_pcb.png)

## TODO

- [ ] More details about battery etc...

## Amazon AWS and MQTT

https://docs.aws.amazon.com/iot/latest/developerguide/iot-gs.html

## Running the code locally

https://devzone.nordicsemi.com/f/nordic-q-a/44638/how-to-move-an-sdk-example-out-of-the-sdk-tree

~~macros="CMSIS_CONFIG_TOOL=.../../../../../../external_tools/cmsisconfig/CMSIS_Configuration_Wizard.jar;"~~

`macros="CMSIS_CONFIG_TOOL=C:/nRF/nRF5_SDK_15.2.0_9412b96/external_tools/cmsisconfig/CMSIS_Configuration_Wizard.jar;SDK=C:/nRF/nRF5_SDK_15.2.0_9412b96"`

I changed ALL occurrences of "../../../../../.." to $(SDK)

## ATSAMD21E Variant

* When using I2C, it may conflict with SERCOM2 PA 14 and 15, change handler to SERCOM0 at `\.platformio\packages\framework-arduinosam\variants\trinket_m0\variant.h`

```
#define PERIPH_WIRE          sercom0
#define WIRE_IT_HANDLER      SERCOM0_Handler
```

* And disable SERCOM0 at the end of `\.platformio\packages\framework-arduinosam\variants\trinket_m0\variant.cpp` SERCOM0 -> PA 06 and 07 are used for something else.

```
/*Uart Serial1( &sercom0, PIN_SERIAL1_RX, PIN_SERIAL1_TX, PAD_SERIAL1_RX, PAD_SERIAL1_TX ) ;

void SERCOM0_Handler()
{
  Serial1.IrqHandler();
}*/
```

## Efficiency

<img src="./pics/effchart.png" width="75%"> 

## Schematic

<img src="./schematic.png" width="75%"> 

## BOM

[ESP Battery pmb](./KiCad/BOM.csv)

## Enclosure

<img src="./pics/enclosure.png" alt="3D" width="50%"> 

## Final Thoughts or Improvements

* Use STATUS 2 of AEM10941 as lst grasp;
* Supply NINA from the same 3.xV from the Boost Converter;
* Control boost converter EN, in order to turn SARA Off;
* SARA RESET Pin access;
* Open Drain 74LVC3G07 buffer on NINA RGB, in order to avoid leak.

## Credits

GitHub Shields and Badges created with [Shields.io](https://github.com/badges/shields/)
