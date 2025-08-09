# ESP32_S3_MIDI_keyboard
ESP32_S3_MIDI_Keyboard and how to use a standard USB PC keyboard as a MIDI synthesizer and chord player.

Overview
This project turns a regular USB computer keyboard into a fully functional MIDI instrument.
It supports both:

Polyphonic synthesizer mode — play multiple notes simultaneously.

Chord mode — trigger full chords with just one or two key presses.

The keyboard acts as a MIDI event trigger, sending notes and other MIDI commands directly to an external MIDI device.
It runs on the ATOM S3 (ESP32-S3 with native USB host support), allowing USB keyboard input to be read without additional hardware.

Features
-Plug-and-play USB keyboard support (USB-C or USB-A with adapter).
-Configurable chord mappings — one key can trigger a complete chord.
-Works with any MIDI-compatible synthesizer or sound module.
-Minimal learning curve — great for quick chord playing without advanced piano skills.
Some instant key functions:
-Up/down arrows - shift octave up or down
-Left/Right arrows - change instrument from 0...127 instruments
-PageUp PageDown - increase or decrease Master Volume (velocity)
-Seven predefined instruments for quick change numeric keys 0,1,2,3,4,5 and 6
-Seven chord modes for now but can be extended
-Set everithing default and all notes off; when Atom display is pressed

Hardware Used
-Universal USB keyboard (USB-C connector or USB-A with A-to-C adapter).
-ATOM S3 Dev Kit with 0.85-inch screen from M5STACK.
-UNIT MIDI from M5STACK — audio output can connect directly to any amplifier.

Dependencies
-EspUsbHost library by TANAKA Masayuki — for USB host support on ESP32-S3.
-M5STACK libraries — for ATOM S3 and UNIT MIDI control.
-Arduino IDE 2.3.5 — for compiling and uploading firmware.

Background
This project started as a simple single-key MIDI instrument.
Then came the idea: press one key → play a whole chord. It worked so well that the project evolved into a compact and versatile chord player, perfect for jamming along to songs without extensive piano training.

I drew inspiration from:
Volos’ basic synthesizer project.
The excellent synthesizer, piano, and organ work of Marcel Licence on ESP and Raspberry Pi boards.
The EspUsbHost library by TANAKA Masayuki.
Rich M5STACK libraries available on GitHub.
My own guitar-playing background and newfound enthusiasm for Arduino.

Links
https://github.com/VolosR/m5StackSynth
https://github.com/tanakamasayuki/EspUsbHost
https://github.com/marcel-licence
https://github.com/m5stack/M5Unit-Synth

Demo Video
...

Acknowledgements
Thanks to all developers and creators mentioned above, to GitHub for providing an excellent platform, and to the Arduino IDE team for making such a versatile development tool. 
