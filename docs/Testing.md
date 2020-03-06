# Testing

Below you can see the depletion mode MOSFET Q2 keeping any excess voltage greater than 13 Volts away from the voltage regulator. Channel 1 shows the signal at the drain and channel 2 the signal at the source of transistor Q2. You can see the voltage being smoothed by capacitor C1 when the transistor is conductive and the rectified half-waves greater than 13 Volts at the drain when the transistor is non conductive.

<img src="images/Q2-signal.jpeg" width="200">

---

The two pictures below show the signal processing used to count the half-waves. The first image shows the signal before and after the **C**urrent **R**egulating **D**iode (CRD). You can see the CRD pinching off the excess voltage at around 7 Volts. And the second one shows the signal at the gate (channel 1) and drain (channel 2) of the transistor Q1. Channel 2 shows the cleaned, voltage adjusted, inverted signal feed into timer/counter 0.

<img src="images/CDR-signal.jpeg" width="200">

<img src="images/Q1-signal.jpeg" width="200">

---

The image below shows the signal being feed into timer/counter 0 (channel 1). And the second signal shows the 2.1 Volts used to power the tiny (channel 2). The image was taken when the power started failing which can be seen by the little dents in the supply voltage. The full-wave period of the timer/counter 0 signal is at about 240 milli seconds. Using this you can calculate the speed the front wheel was moving with when the power started failing (assuming a 26" wheel):

((2.075 m / (0.24 s x 13)) x 60 x 60) / 1000 ≈ 2.39 km/h  

<img src="images/power-starts-failing.jpeg" width="200">

---

Total power consumption is at about 0.46 milli Amps on average (measured at the hub generator input). The peak usage measured is 1.17 milli Amps. The power usage fluctuates when the LED comes on. Even more power could be saved by turning off the LED but it is hardly worth it. The main power consumer is the Attiny.

---

If you run into issues with the serial connection try calibrating the Attinys internal oscillator. The calibration can be set using the INTERNAL\_OSCILLATOR\_CALIBRATION macro which is added to the OSCCAL register in the usi\_init function. The internal oscillator is a little bit different for every tiny produced. If you want to know the value added to OSCCAL in an already programmed Attiny disassemble the flash:

<pre>
mini% make disassemble
</pre>

and check what is being done to register 49 or 0x31 (OSCCAL) in the odometer.asm file. I have tried doing the timing in the USI functions with a well calibrated tiny. The USI functions have also been tested with rates of 1200 to 19200 Baud (see the USI\_BAUD\_DELAY macro). 

Date: 2020-02-04
