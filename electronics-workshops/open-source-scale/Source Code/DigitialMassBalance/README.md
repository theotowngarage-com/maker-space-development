# OS_Nano_Balance
Firmware and design files for a digital mass balance with 3D printable components.
([Project Page](https://www.appropedia.org/3-D_Printable_Digital_Balance))

This balance is powered and controlled by an Arduino Nano. 
Using the digital I/O pins, the Nano powers and reads information from an HX711
32-bit load cell amplifier.
The readout from this loadcell can be calibrated using a standard mass.
Calibration mode is entered using a push button.
The calibrated value is distributed via two media: Serial and an LCD display.
When connected to the Nano over serial (115200 baud), the raw (tared) and calibrated 
readout are displayed.
Depending on the SSH client in use, this data can be logged and plotted -
primarily for scientific uses.
When an LCD is included in the system, the calibrated value is displayed with units.

**Power**

The entire system is powered by the USB input to the Nano.
This can be supplied by a computer or a 5V wall adapter.
* The HX711 and LCD are powered by digital I/O pins, meaning they can be independently turned on and off if desired.
* This works because the HX711 requires a maximum of 1.5 mA @ 5V (see data sheet in ``doc``).
* Similarly, the LCD requires around 2.5 mA @ 5V (see data sheet in ``doc``). 
* Arduino I/O pins can supply 40 mA @ 5V (recommended max is 20 mA continuous) 
(see [Arduino Forum](https://forum.arduino.cc/index.php?topic=121675.0)

## Wishlist
* Send a calibration value over serial, or offer a selection on the display (select with Tare)
* ~~Improve readout on the display to offer more information and work with calibration`~~
* ~~Condense readout commands into functions to make the code 
[DRY](https://pragprog.com/the-pragmatic-programmer/extracts/tips)~~
* ~~Improved averaging algorithm to improve Signal-to-Noise-Ratio~~
* Add compatibility for different types of displays
* Update readout resolution (number of decimals) to match the resolution of the load cell.
This could be adaptive based on the calibration value. 
(eg a 5000g load cell offers a different level of precision than a 50kg load cell).

## Printable Parts
The bed, base, and face are parametrically designed in FreeCAD.
Old designs created in OpenSCAD are available as well - 
these accommodate a TAL220 and TAL221 loadcell, but not an LCD.
These parts can be printed for a few dollars (depending on the cost of your filament).
If you are using an LCD, check the dimensions on ``Face.FCStd`` before printing.

## Wiring
Wiring diagram produced using Fritzing and included in the `doc` folder. 
Pin locations can be verified by looking at the .ino source code.

**LCD**
Wiring is based on the 
[Hello World](https://www.arduino.cc/en/Tutorial/HelloWorld)
example.

**Load Cell --> HX711:**  
* red --> E+  
* blk --> E-  
* grn --> A+  
* wht --> A-  

**HX711 --> Arduino:**  
The HX711 is powered by the Arduino's digital pin. 
This is possible because the Arduino can supply 20 mA of current, while the 
HX711 only requires 1.5 mA. 
* GND --> GND  
* DT  --> D2  
* SCK --> D3  
* VCC --> D4  

**Buttons --> Arduino:**  
* GND --> Cal  --> D7
* GND --> Tare --> D8

## Firmware
The firmware sends a readout to the Serial Monitor for interaction.
Arduino's Serial Plotter can be used to get a live readout of changing mass on
the balance.

### Tare
To tare, simply hit the tare button (on pin 8). This will zero the output.

### Calibrate
To calibrate, press and hold the tare button (on pin 8) for 3 seconds
(this time can be changed in config.hpp).
This will send the scale into its calibrate state.
The scale will automatically tare when in calibration mode.
The scale will take several averages and then calculate a new sensitivity.
At this time, the device is set to calibrate with 0.2359 kg, the mass of 1 US Cup of water
(this can be changed in config.hpp).

## Data Logging
To log data, use a third-party client such as PuTTY, which offers session-logging
capabilities.
