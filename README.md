# Meat Loaf 64

Commodore 64/128 WiFi Modem and IEC Serial Floppy Drive simulator device

Code is based on and inspired by the following:
Paul Rickard's ESP8266 Modem (https://github.com/RolandJuno/esp8266_modem) 
Lars Wadefalk's UNO2IEC (https://github.com/Larswad/uno2iec)
Steve White's Pi1541 (https://github.com/pi1541/Pi1541)


To setup your own MeatLoaf64 server check out this code.
(https://github.com/idolpx/meatloaf-svr)


Key Features
------------

* WiFi modem for connecting to telnet BBSs
* IEC Bus interface for loading data directly from flash memory or via HTTP
* Can be configured to simulate multiple IEC devices (IDs 4-30)
* Each device's configuration is switched out and persisted on access
* Firmware can be updated via HTTP


To Do
-----

* Standardize all Hayes Commands and add extended commands
* Complete CBM DOS support
* EXtend CBM DOS with device specific features
* Support all different CBM file, disk, tape, cart media image files from local flash file system
* Add support for Fast Loaders
* Port all code to ESP32 IDF
* Add SD card interface
* Add Cassette tape interface
* Add virtual printer/plotter interface


References
----------

* https://www.pagetable.com/?p=1018
* http://www.zimmers.net/anonftp/pub/cbm/programming/serial-bus.pdf
