# Bicycle Odometer

## Description

This is a project for a bicycle odometer. The odometer will record the front wheel turns of the bicycle. Instead of using a magnet/reed switch on the front wheel this odometer will get the information from the wheel hub generator. The generator will also provide the power for the odometer. This odometer is **not** a full fledged bicycle computer. Its sole purpose is to record the total kilometers/miles the bike has traveled. For more details please see the [docs](docs) directory.

## Hardware

* Odometer
	* Please see the [schematic](docs/Hardware.md) for details.
* AVR ISP (**I**n **S**ystem **P**rogrammer)
	* The programmer should work together with [AVRDUDE](https://www.nongnu.org/avrdude/). I e.g. use Dean Camera's [AVRISP-MKII Clone](https://www.fourwalledcubicle.com/AVRISP.php) but any other AVR ISP should also be fine.
* A USB to Serial Adapter
	* It needs to have TTL level inputs/outputs and preferably 3.3-5 Volts power output like e.g. the [Adafruit FTDI Friend](https://www.adafruit.com/product/284).

## Software

* AVR 8-bit gcc toolchain
	* available here: [Microchip](http://www.microchip.com/), [avr-gcc](https://www.microchip.com/mplab/avr-support/avr-and-arm-toolchains-c-compilers)
* GNU Make
	* available here: [GNU](http://www.gnu.org), [make](https://www.gnu.org/software/make/)
* The **AVR** **D**ownloader/**U**ploa**DE**r
	* available here: [Savannah](http://savannah.nongnu.org), [AVRDUDE](https://www.nongnu.org/avrdude/)

## Build

Clone or download the firmware. Make sure all needed binaries are in your path and then issue the following commands:

* `make clean`
* `make`
* `make flash`

This will remove any old build files, rebuild the Intel hex file and download the hex file onto the Attiny. Also remember to set the fuses:

* `make fuse`

The fuses only need to be set once. If you don't want to build the hex file yourself you can also just flash the odometer.hex.save file using AVRDUDE:

`avrdude -q -F -P usb -c avrispmkII -p attiny85 -U flash:w:odometer.hex.save`

## Usage

The odometer needs to be hooked up in parallel to the hub generator. As soon as the front wheel starts turning the Attiny will power up and start counting the AC voltage sinus waves sent by the hub generator. The number of front wheel turns will be stored in the non-volatile EEPROM. To retrieve the stored wheel turns from the EEPROM you will need to connect the odometer to a computer with a serial adapter. I have travelled **2017.0 km** up to now using the odometer.

Date: 2020-02-11
