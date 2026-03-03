# Description
Simple BMS controller implementation for esp32-s2
Modify used pins in config.h if soldered differently

## Hardware Needed
For firmware to work properly there are a few parts needed

*Quadrature Encoder for turntable
*11 Buttons with Microswitches
*Esp32-s2 board, i used esp32-s2 Pico by waveshare

### What does the code do ?

*hid gamepad using TinyUsb library
*State machine for quadrature encoders
*Very basic debounce, use hardware debouncing if you can.


