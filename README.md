# ChickenDoor

This is a chicken door automation designed for our two Eglu Cube and one Eglu Go chicken houses. Operated by an ESP8266 microprocessor featuring a RESTless API, it can be easily integrated into a home automation system.

# Features
- can be added (almost) without modifying the chicken house
- controls a motor to open / close the door
- controls a light to attract chickens to move in
- supports several commands using a RESTfull API:
- door status
- open door
- close door
- light status
- turn light on
- turn light off
- button to open and close the door manually
- displays door status using two LEDs

## Other Sources

Besides this repository, the 3D printable parts are provided on [Harry's Prusa Prints](https://www.prusaprinters.org/social/92858-harry/prints).

## Electronics

The plan is provided in `plan.svg`. It combines an ESP8266 with a stepper motor driver, two LEDs for status display and a button allowing manual operation. In addition, a 12V LED can be added.

## Software Installation

Start with setting up your Arduino environment. When using the electronic parts I have used, here are the basic steps to follow:

- install support for your ESP8266 board
- install RemoteDebug library required for our own libraries

chickendoor is based on the **Bolbro library** my projects are based on. It comes with a number of convenience functions including WiFi, preference, time, web server, and log handling. 

-  copy `library/Bolbro` to your Arduino library directory; on macOS, this is `~/Documents/Arduino/libraries`
-  restart Arduino IDE afterwards

Now, you are ready to compile and flash the sketch. `chickendoor` requires a customization to work:

- rename `chickendoor.ino.customize` to `chickendoor.ino`
- edit the call to `Bolbro.addWiFi()` to add the local WiFi to connect to
- search for other entries marked with "customize" and change as required

By adding a WiFi made up from an SSID and a password, it will be considered as a connection point of your local network. In case you have more than one, call `addWiFi()` multiple times. The best one will be choosen.

Finally, the sketch:

- compile and flash `chickendoor`

See the [3D Model description](https://www.prusaprinters.org/prints/75842-chicken-door-automation-for-omlet) for instructions how to operate the system. 
