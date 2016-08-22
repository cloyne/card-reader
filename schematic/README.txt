Schematic README

A quick rundown of how the circuit works:

Power from ethernet cable J1: +6V, GND, +12V, -12V. +6V powers the transistors, +/-12V powers the LF356 Op Amps U1A/U2A/U3A/U4A. Capacitors C5, C6, and C7 attenuate 60Hz noise.

Arduino A1 provides a 125kHz 5V/0V square wave that acts as the transmitter wave. U1A/R1/R2 is an attenuator that steps it down to -0.2V. This is connected to the external inductor (~ 0.16mH) and C1, an LC circuit roughly tuned to 125kHz. The Cal1ID card communicates through inductive coupling with the external inductor. 

U2A/R3/R4 and U3A/R7/R8 act as voltage followers. D1, C2, and R5 act as a peak follower. R6 and C3 act as a low pass filter. C4 and R9 act as a high pass filter. U4A acts as comparator that returns +/- 12V. Resistors R12, R13, and R14 steps down +/- 12V to +5V/0V. This signal is then fed into Arduino A2. 

A2 records, decodes the filtered Cal1ID card signal. Upon receiving a signal, it checks whether it is stored on the SD card, while blinking the red LED. If so, it blinks the green LED, and enables transistor Q1, which triggers the door to open.