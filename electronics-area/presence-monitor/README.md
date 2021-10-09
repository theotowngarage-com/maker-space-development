# WiFi presence monitor
Wireless presence monitor based on ESP8266 and PIR sensor.
Data is sent and processed by ThingSpeak platform.
[Public sensor dashboard](https://thingspeak.com/channels/1169359)

# Dependencies
DallasTemperature library (requires OneWire library)
ThingSpeak library

# Quick start
1. Select your ESP8266 module board in Arduino.
1. Install required libraries (look at Dependencies above)
1. Optionally: enable debugg by uncommenting '#define DEBUG'
1. Compile sketch
1. Connect board with USB cable, and upload sketch

# System architecture
ESP8266 module
Cheap PIR module connected to pin D7
Maxim DS18B20 temperature sensor connected to pin D6

Powered by 2S LiIon battery. Battery voltage is dropped to 5V by LDO linear regulator. 
5V is needed to power PIR, and temperature sensors. Voltage is further dropped from 5V to 3.3V to power ESP8266 module.

# Theory of operation
## Sensors
PIR sensor module signals high when motion is detected. PIR module is powered from 5V

Temperature sensor is read before each data package is sent. Sensor is connected over 1wire bus, and powered from 5V. 
In sleep mode current consumption does not exceed few uA. Conversion time is exponentialy dependent on resolution.
Conversion is done before wifi is connected.

ESP8266 is kept in deep sleep with WiFi radio disabled for as long as possible.
State of PIR sensor is kept high for few seconds after motion was detected. This gives module time to wake up and read state
every 3 seconds.

## Power considerations
Device is designed to operate from battery power.
Power calculations based on measurements done with oscilloscope are documented in excel files.
Every 3s device wakes up with WiFi module disabled, and monitors state of PIR input.
If input is high, then FLAG_LAST_PIR_STATE flag is set in NVRAM.

Every 20s state of the flags in NVRAM is checked, and if set then message transmission is requested.

Heartbeat message is sent every 30min even if motion was not detected.

### Low battery
Battery voltage is affected heavily by current draw, especially at low temperatures.
Moving average is implemented to smooth out voltage measurements.

When moving average drops below 6.2V then data transmission is inhibited, and duration of deep sleep cycle is 
increased to 30s.

### Battery not connected
When battery voltage measured is below 1V, then module assumes it is powered with external power supply.
In this mode extreme power saving options are disabled.

### Communication errors handling
Further battery savings are done when wifi or internet connection is down in more than 8 consecutive transmission attempts.
When this condition is detected, then data transmission interval is increased to 30min, and deep sleep interval in increased to 30s.
Error counters are cleared after successful data transmission.

# TODO

 - Visual low battery indicator
 - Humidity measurements 

# Copyright
Author: Michal Milkowski

# Credits
Based on 
- [ThingSpeak example](https://github.com/nothans/thingspeak-esp-examples/blob/master/examples/RSSI_to_ThingSpeak.ino) by Hans Scharler (http://nothans.com)
- [DS18B20 example](https://github.com/milesburton/Arduino-Temperature-Control-Library/blob/master/examples/WaitForConversion2/WaitForConversion2.ino) by milesburton

License: GPLv3

