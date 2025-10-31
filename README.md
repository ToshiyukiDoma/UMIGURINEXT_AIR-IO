# UMIGURI IO Firmware
IO board for AIR-STRINGS. Modified for my air-string setup.

## Requires
- Arduino
- USB-A cable
- PCB
- AIR-STRINGS
- 5V supply
- 12V supply
- Small stuff that need for electronics

## Usage
1. Connect pins of AIR-STRINGS to the other side of the pair, arduino and power supplies.
```
6 OUT pins -> Arduino A0 - A5
TWR pins -> the opposite side of TWR pins
DI pins -> Arduino D11 and D12
5V pins -> 5V supply
12V pins -> 12V supply
GND -> ground
```
See also: https://rhythm-cons.wiki/controllers/chunithm/chunithm-air-strings/

2. Write `iofw.ino` to arduino.
3. Open UMIGURI config, Set COM port num that your arduino is recognized.
4. Done.

## License
MIT License

