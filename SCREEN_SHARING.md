# BOMB OS screen-sharing milestone

This commit adds three software components:

- `display/framebuffer.c/.h`: a 320x240 RGB565 software framebuffer and test pattern.
- `usb/bomb_link.c/.h`: a framed screen-packet sender. A future USB driver supplies the byte-write callback.
- `viewer/`: a Windows Forms viewer that listens on a COM port and displays incoming frames.

## Packet format

All integers are little-endian.

| Offset | Size | Meaning |
|---|---:|---|
| 0 | 4 | ASCII `BOMB` |
| 4 | 2 | width |
| 6 | 2 | height |
| 8 | 4 | payload length in bytes |
| 12 | variable | RGB565 pixels, row-major |

The viewer currently listens at 115200 baud on a COM port.

## Important limitation

This is not yet a working USB screen-sharing release. The Raspberry Pi Zero v1.3 uses a DWC2 USB OTG controller. A bare-metal DWC2 device-mode driver, USB enumeration, and CDC-ACM interface are not implemented yet. Therefore Windows will not yet show a BOMB OS COM port from the Pi's USB socket.

The current kernel build will compile the framebuffer and packet sender, but a later milestone must connect them to a tested USB device driver and send frames from the running kernel.
