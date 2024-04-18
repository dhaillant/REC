REC
=======

REC is a record-and-play Eurorack module

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
