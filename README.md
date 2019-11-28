# eepromutil
### EEPROM writting utility for SST39 NOR flash, using Arduino

I wrote this to help me flash SST39SF0*0A chips with help of arduino, as I couldn't find anything that tackled specifically those chips
Maybe someone will find it useful. 

## Wiring

A0  - A7  : PORTA   : (Arduino pins 22, 23, 24, 25, 26, 27, 28, 29)
A8  - A15 : PORTC   : (Arduino pins 37, 36, 35, 34, 33, 32, 31, 30)
A16 - A18 : PORTG   : (Arduino pins 41, 40, 39)
D0  - D7  : PORTL   : (Arduino pins 49, 48, 47, 46, 45, 44, 43, 42)
CE#       : GND
WE#       : PORTD.7 : (Arduino pin 38)
OE#       : PORTB.0 : (Arduino pin 53)


