# rpiMavToLTM

Small tool to read Mavlink stream from Serial gpio (made for Arducopter on Pixhawk, but should work with others). On stdout it produces LTM data with one additional frame (C-frame like custom-frame) - it is back-compatible with normal LTM tools. Also, it controls NeoPixel LEDs, attached to 12 gpio (pin 18), depending on arm state and flight mode.

Depends on next libraries:
git://git.drogon.net/wiringPi  - for serial port functionality
https://github.com/jgarff/rpi_ws281x.git - for NeoPixel support

Special thanks:
<b>ScottFlys</b> for mavsky project https://github.com/scottflys/mavsky.git
<b>Guillaume S aka KipK</b> for GhettoProxy project</b> https://github.com/KipK/Ghettostation/tree/master/GhettoProxy
