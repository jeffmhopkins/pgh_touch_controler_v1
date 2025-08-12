# VL1 Touch Controller Firmware

This repository contains the source code for the firmware of the VL1 Touch Controller, a Eurorack module. The firmware is Arduino-based and was originally developed for internal use by the module's creators.

## Overview

The VL1 Touch Controller is a capacitive touch-based Eurorack module for generating control voltages, gates, and sequences in modular synthesizers. This firmware (`tcFirmware_vl.ino`) handles capacitive sensing, sequencing, clocking, and output control for the module's features, including mono/duo modes, Y-axis position detection, and gate outputs.

Key features from the code:
- Capacitive touch sensing using the CapacitiveSensor library.
- Support for mono and duo note modes.
- Sequencing up to 64 steps.
- Clock division, reset handling, and external clock integration.
- Y-axis slider position calculation for CV output.
- Button handling for modes like mono, rest, and duo.
- LED and gate output control.

The code includes extensive ASCII art headers for sections like "TOUCH CONTROL" and variable declarations.

## Background

This firmware was shared by the company in response to a community request for open-source access, with permission to use, modify, and share freely. It was provided with the intent to encourage community contributions for new features or improvements to the Touch Controller. The code was originally written for internal use, so it may lack organization and documentation.

## Hardware Requirements

- **Board:** Arduino Mega (based on pin usage, e.g., pins up to 69 and analog pins like A10).
- **Libraries:** Requires the [CapacitiveSensor](https://github.com/PaulStoffregen/CapacitiveSensor) library (included via `#include <CapacitiveSensor.h>`).
- **Pins:** Configured for specific inputs/outputs (e.g., touch sensors on pins 44-52, axis on A0-A9, outputs on 3-17, etc.). See the `Pins` section in the code for details.
- **Module:** Designed for the VL1 Touch Controller hardware, including capacitive pads, LEDs, buttons, and CV/gate jacks.

## Installation and Usage

1. **Download the Code:** Clone this repository or download `tcFirmware_vl.ino`.
2. **Arduino IDE Setup:**
   - Open the `.ino` file in the Arduino IDE.
   - Install the CapacitiveSensor library via the Library Manager (Search for "CapacitiveSensor").
   - Select "Arduino Mega or Mega 2560" as the board.
   - Adjust any settings (e.g., `switchSamples`, `switchThreshold`) for calibration if needed.
3. **Compile and Upload:**
   - Verify and upload to your Arduino Mega connected to the module.
   - Note: The code uses `analogReadResolution(12)` for 12-bit ADC resolution.
4. **Calibration:** The firmware auto-calibrates capacitive sensors on startup. Touch thresholds and smoothing can be tweaked in the variables section (e.g., `switchThreshold = 400`).
5. **Testing:** Use an external clock signal on pin 68 for sequencing. Monitor outputs via oscilloscope or multimeter.

## Caveats and Warnings

- **Organization and Documentation:** The code was written for internal use only. It lacks comments, has inefficient sections, and may include unused code. Navigation can be challenging.
- **No Support:** The company cannot provide assistance, advice, or troubleshooting. Any modifications are at your own risk.
- **Potential Issues:** Capacitive sensing can be sensitive to environmental factors (e.g., humidity). Test thoroughly on hardware.
- **Truncated Code:** The provided file may appear truncated in some contexts (e.g., "...(truncated 37343 characters)..."), but the full source should be used.

If you encounter issues, refer to the Arduino forums or Eurorack communities for help.

## Community and Contributions

- **Mod Wiggler Forum:** Discussions about this module and firmware requests originated here. Share your mods or ask questions in relevant threads (search for "VL1 Touch Controller").
- **Contributions:** Feel free to fork this repo and submit pull requests for improvements, such as better documentation, optimizations, or new features (e.g., MIDI integration or advanced sequencing).
- **Sharing Mods:** If you create something cool, share it with the community or let the original creators know—they'd love to see it!

## License

No formal license is specified. The firmware was shared with permission to use and distribute freely for non-commercial purposes. If commercializing modifications, consider reaching out to the company.

For questions about this README or the repo, open an issue here. Happy hacking! 🚀
