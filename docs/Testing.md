# Testing

## Low Voltage

First tests show that the odometer stops working when the generator voltage comes down to about 8 Volts peak to peak (about 3.3 Volts after being rectified). At this wheel speed the AC frequency is down to about 4 Hz. If I did my maths right this means the bike needs to move with at least about 2.5 km/h for the odometer to work. I will try to improve on power consumption to get this value down even further.

<img src="images/hub-generator-slow.jpeg" width="200">

Here is what the voltage looks like once it's failing after the LDO voltage regulator:

<img src="images/power_starts_failing.jpeg" width="200">

## Wheel Turns

The two pictures below show the signal processing used to count the half waves. The first image shows the signal at the gate of the transistor after being rectified by the diode. And the second one shows the clean, inverted signal at the drain/input into timer/counter 0.

<img src="images/signal_half_waves.jpeg" width="200">

<img src="images/signal_after_transistor.jpeg" width="200">

Date: 2019-11-17
