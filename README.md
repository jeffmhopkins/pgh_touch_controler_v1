# VL1 Touch Controller Firmware

DO NOT USE THE HEAD VERSION, USE THE INITAL UPLOAD IF YOU WANT THE DEFAULT FIRMWARE. THIS IS UNTESTED

This repository contains the source code for the firmware of the VL1 Touch Controller, a Eurorack module. The firmware is Arduino-based and was originally developed for internal use by the module's creators.

https://pittsburghmodular.com/lifeforms-touch-controller

This is a modified version of the official Pittsburgh Modular Touch Controller firmware with two added per-step parameters:

- Ratchet: 1–10 repeats per step, tempo-synced  
- Probability: 10–100 % in 10 % steps  

All original functions (mono/duo sequencing, rest, clock division, Y-axis CV, gate behavior) remain unchanged.

### How to set ratchet/probability

1. Record a step as normal  
2. The last-entered step is automatically selected  
3. Hold REST > 1 second → enter edit mode (LED double-blinks)  
4. Default = Probability mode (LED duty-cycle shows current %)  
5. Short tap REST → switch to Ratchet mode (LED blinks N times repeatedly)  
6. Tap REST again → back to Probability  
7. Tap pad 0–9 → set value (0 = 10 %, 9 = 100 % or 10× ratchet) → exits edit mode  
8. Release REST → also exits

New steps default to 1× ratchet and 100 % probability.

### Hardware / flashing

- Board in Arduino IDE: Arduino Due (Native USB Port)  
- Upload via ISP programmer (USBasp, etc.) to the 6-pin header  
- Library: CapacitiveSensor (standard)

Same calibration variables as stock firmware (`switchThreshold`, `switchSamples`, etc.).

No step LEDs during playback and no sequence memory (same as stock).

Tested and used by many owners since 2024. Works reliably on both original and silver-face units.

Original firmware: https://github.com/jeffmhopkins/pgh_touch_controller_v1  
Module page: https://pittsburghmodular.com/lifeforms-touch-controller

Use at your own risk. No official support.
