# pi0neopixel

Test development of application to control NeoPixels (aka WS2812B) from a Raspberry Pi Zero.

The source code is based on a post from [@danjperron](https://github.com/danjperron)
in the Raspberry Pi forums.


# platform

Hardware:
* Raspberry Pi Zero W.
* WS2812B-8 RGB LED PCB Stick

Operating System:
* Raspberry Pi OS Lite
* System: 32-bit.
* Debian version: 11(bullseye)
* Kernel version: 5.10


# build

A Makefile is provided to simplify the build process.


# notes

## /boot/config.txt

Enable SPI interface by adding the following line to /boot/config.txt

> dtparam=spi=on


## RGB vs RGBW

While this project is using a NeoPixel with WS2812B LEDs that require an RGB
signal for each pixel there are some variants that use different LED hardware
and require an RGBW signal. The source code includes a WS2812B define that is
used to toggle RGB / RGBW.


## RGBW to data signal T0H-T0L / T1H-T1L

The color data is stored as 4 bytes for the four color channels R, G, B, W in
each pixel. However the data signal sent to the pixels is a width modulated
clock signal where each bit value in the color channel bytes is represented by
the differential in the data signal high and low length.

Thus the color channel bytes must be converted into a data signal that clocks
high and low with different pulse widths based on the bit values in the color
channel byte.

This process of conversion is documented in the source code and the data sheet
for the NeoPixels.

Example: R channel byte value 255 converted to data signal...
```
R (1 byte): 255  ==  11111111
                /                 \
               /                   \
              /                     \
         1111                        1111
        /     \                    /      \
      /         \                 /         \
    11            11            11            11
   /   \         /   \         /   \         /   \
1110 | 1110   1110 | 1110   1110 | 1110   1110 | 1110

Data (4 bytes): 238 238 238 238
```
