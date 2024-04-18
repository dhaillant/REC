REC
=======

REC is a record-and-play Eurorack module, powered by an Arduino Nano

Copyright 2024 David Haillant

Compatible hardware:
* REC V0.1 (20240315)

How does it work?
=================

In Record mode, the voltage present at "CV In" pin is sampled and stored when Clock input receives a rising edge.
Up to 512 samples can be stored.

In Play mode, the PWM output will playback the stored samples, in the same sequence as recorded, on each pulse on Clock input.

If Reset input receives a pulse, the index of stored values points back to the begining, in both Record and Play modes:
* In Play mode, the playback starts from the begining.
* In Record mode, the previous samples are overwritten.

The last sample is the last sample stored during Record mode.

In Play mode, if the last sample has been reached, the next sample to be played will be the first in the memory.

To append new recordings, change mode to Record and send pulses on Clock/push button "Step".

While recording, the CV output will reproduce immediately the last recorded value.

Hardware details
================

The core of this module is an Arduino Nano with an ATmega328.

The analog input signal is connected directly to pin A5 of the Arduino and is sampled by the internal 10-bit ADC.

The analog output uses two filtered Fast PWM signals, from pins D9 and D10.
The signal is then buffered by an single op-amp.

Buttons and clock inputs are working with Arduino internal pull-up resistors.
The input clocks are buffered through NPN transistors.

Every input and ouput is protected from reverse polarities and overvoltages with networks of series resistors and clipping diodes.

Optional LEDs can show the various modes and states.


How to setup?
=============

Download this code and open it with Arduino IDE. Compile and upload to the Arduino Nano.

See hardware informations for how to wire the circuit.

