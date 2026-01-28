# 34hp 16 Step Sequencer

![](https://raw.githubusercontent.com/Fihdi/16Sequencer/refs/heads/main/16-Sequencer.jpeg)

A 16-Step sequencer with individual CV knobs and 4 programmable gate outputs, including a probability gate output. Input gates can control the Clock, Reset, Direction and Pause/Play.

To program a gate channel, hold down the channel button (A through D) and select or deselect the steps are active. 

The functionality is similar to the famous "Baby 8" sequencers but instead of a CD4017, a TCA9555 IO-expander sends 5 or 0V to the potentiometers, which are then summed together. Another TCA9555 is used for the LEDs and push-buttons, both of which are set up in a 4x4 Matrix, allowing both of them to be handled by a single expander.

ToDo:

- Individual gate channel lengths: The length of every gate channel should be set individually with the LENGTH knob. Turning the LENGTH knob while holding down a channel button will change the length of that gate channel. Turning the LENGTH knob on its own only changes the length of the CV channel.

- Ratcheting: Pushing on a step button multiple times should allow for re-triggering (2, 4 or 8 times that of the CLK signal)
