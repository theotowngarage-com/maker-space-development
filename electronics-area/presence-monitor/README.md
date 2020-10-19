# WiFi presence monitor
Wireless presence monitor based on ESP8266 and PIR sensor.
Data is sent and processed by ThingSpeak platform.
[Public sensor dashboard](https://thingspeak.com/channels/1169359)

# System architecture
ESP8266 module
Cheap PIR module
Powered by 2S LiIon battery. Battery voltage is dropped to 5V by LDO linear regulator. 
5V is needed to power PIR sensor. Voltage is further dropped from 5V to 3.3V to power ESP8266 module.

# Theory of operation
## Sensors
PIR sensor module signals high when motion is detected. PIR module is powered from 5V

ESP8266 is kept in deep sleep with WiFi radio disabled for as long as possible.
State of PIR sensor is kept high for few seconds after motion was detected.

## Power considerations
Device is designed to operate from battery power.
Power calculations based on measurements done with oscilloscope are documented in excel files.
Every 3s device wakes up with WiFi module disabled, and monitors state of PIR input.
If input is high, then FLAG_LAST_PIR_STATE flag is set in NVRAM.

Every 20s state of the flag is checked, and if set then message transmission is requested.

Heartbeat message is sent every 30min even if motion was not detected.

# TODO

 - Temperature and humidity measurements 
 - Shutdown when battery voltage drops too low 
 - Visual low battery indicator

# Copyright
Author: Michal Milkowski

License: GPLv3

