# Testing

## Low Voltage

First tests show that the odometer stops working when the generator voltage comes down to about 8 Volts peak to peak (about 3.3 Volts after being rectified). At this wheel speed the AC frequency is down to about 4 Hz. If I did my maths right this means the bike needs to move with at least about 2.5 km/h for the odometer to work. I will try to improve on power consumption to get this value down even further.

<img src="images/hub-generator-slow.jpeg" width="200">

Here is what the voltage looks like once it's failing on the output side of the LDO voltage regulator:

<img src="images/power_starts_failing.jpeg" width="200">

## Wheel Turns

The two pictures below show the signal processing used to count the half waves. The first image shows the signal at the gate of the transistor after being rectified by the diode. And the second one shows the clean, inverted signal at the drain/input into timer/counter 0.

<img src="images/signal_half_waves.jpeg" width="200">

<img src="images/signal_after_transistor.jpeg" width="200">

## Transistor

I replaced the BSS123 transistor with a 2N7002 as a precaution. The 2N7002 from NXP has a higher gate-source voltage limit (±30 Volts compared to ±20 Volts). Otherwise the specs are pretty much the same.

## Power Usage

Total power consumption is at about 0.46 milli Amps on average (messured at the hub generator input). The power usage fluctuates when the LED comes on. Even more power could be saved by turning off the LED but it is hardly worth it. The main power consumer is the Attiny.

## Serial Port

If you run into issues with the serial connection try adjusting the USI_BAUD_DELAY value in the odometer.h file. By increasing or decreasing the value by 1 or 2 you can adjust the baud rate to slightly different hardware like e.g. the internal oscillator or another serial adapter. Please remember the the USI is strobed in software (not hardware) and therefore is not as precise as it could be.

## High Voltage

I'm beginning to learn more and more about generators. One thing I was not expecting is the voltage the generator produces while running idle. Under load the generator produces the 6 Volts AC. I was expecting something like maybe 12 Volts when it is running idle but this is not the case. The voltage rises almost linear depending on the revolutions per minute of the generator. The SON generators voltage I use increases by about 17.5 Volts for every 20 km/h increase in speed on my bicycle. Since I use my bicycle mostly without the lights on the odometer will have to deal with these high voltages. I have replaced the LDO regulator (again) with an LT3014 which can handle up to 80 Volts of input. And I have also replaced the capacitor behind the rectifier with a 100 Volt Ceramic. I don't want to shunt the high voltage and thus produce load on the generator. The odometer should use as little power as possible (less or equal to 1 milli Watt).

Date: 2019-11-17
