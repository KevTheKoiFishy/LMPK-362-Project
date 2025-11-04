# SD Card

## Uses
- Storing and retreiving images, like for map or graphics
- Storing and retreiving audio, like for alarm
- Storing and retreiving user configurations, like alarm settings

## Hardware Interface
Using SPI, connections are as follows:

| SD Pin | Pi Pico 2 Pin |
|:-:|:-:|
| VDD (4) | 3.3V |
| VSS (6) | GND |
| CLK (5) | SCK |
| CS (2) | CS |
| DI (3) | MOSI |
| DO (7) | MISO |

## Software Interface
- SD Card's Partition 0 (FAT32) is mounted as a drive.
- You can then access the SD Card Data using standard file.h commands in C.