# shooplera
USB HID interface for digital calipers.

## shusb
Firmware for STM32F103 MCU.

## shoop
PC program sending HID command to the MCU. 

Unexpected difficulties with Windows HID keyboard policy preventing usage on this platform.
The device behaves as a keyboard which can accept "output HID report" as well. Unfortunately Windows do not allow doing that. 
Unfortunately again, I failed to find a solution for that problem within one week, so I'm leaving it for future opensource magic, when someone send me a merge request which I'll happily accept.

## hw
Description of the modification of the Chinese st-link clone PCB.
