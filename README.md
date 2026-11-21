# BOMB OS 💣

A bare-metal operating-system experiment for the **Raspberry Pi Zero v1.3** (ARM1176JZF-S / ARMv6, 512 MB RAM).

## What is implemented

- ARMv6 startup, stack setup, and BSS clearing
- Mini-UART debug output on GPIO14/15
- VideoCore HDMI framebuffer desktop
- A 320×240 RGB565 software framebuffer, so the desktop can also render when no HDMI display is attached
- An experimental polling-mode DWC2 USB device driver with CDC-ACM descriptors
- BOMB-LINK frame packets over USB serial
- A Windows Forms viewer that reads the virtual COM port and displays the desktop

**Important:** USB enumeration and screen streaming have not yet been verified on a physical Pi Zero. The USB driver is an in-progress bare-metal implementation, not a certified or production USB stack. The GitHub Actions workflow builds the ARM image; a successful compile does not prove the hardware link works.

## Build the Pi image on Windows

Install an ARM GNU toolchain containing `arm-none-eabi-gcc` and `arm-none-eabi-objcopy`, and add its `bin` folder to PATH.

From Command Prompt in the repository folder:

```bat
build.bat
```

The image is written to `build\kernel.img`. The same build is available with GNU Make using `make`.

## Prepare the SD card

Use the existing Raspberry Pi boot/FAT partition that already contains the matching firmware files (such as `bootcode.bin`, `start.elf`, and `fixup.dat`). Copy the newly built `build\kernel.img` to the root of that partition as `kernel.img`. The repository's `boot/config.txt` selects the 32-bit kernel image.

Do not format the card or delete the firmware files just to install this kernel.

## Connect the USB viewer

1. Power the Pi Zero and boot it from the SD card.
2. Connect the laptop to the Pi Zero micro-USB port labelled **USB** (the data/OTG port), not **PWR IN**.
3. On Windows, build and run the viewer:

```bat
dotnet run --project viewer\BombViewer.csproj
```

4. Refresh the COM-port list, select the Pi's USB serial port, and press **Connect**.

The viewer expects the BOMB-LINK packet format: a 12-byte `BOMB` header followed by a 320×240 RGB565 frame. The USB serial connection is separate from HDMI; HDMI is optional for the software-framebuffer desktop path.

## Project layout

- `kernel/start.S` — ARM startup
- `kernel/kernel.c` — kernel entry, UART, USB polling, and frame streaming
- `usb/dwc2_device.c` — DWC2 USB device-mode driver
- `usb/bomb_link.c` — frame packet format
- `display/framebuffer.c` — 320×240 software framebuffer
- `display/video.c` — VideoCore HDMI framebuffer
- `desktop/desktop.c` — BOMB OS desktop rendering
- `viewer/Program.cs` — Windows screen viewer
- `build.bat` — Windows build
- `.github/workflows/build.yml` — ARM cross-build and image artifact

## Hardware target

Original Raspberry Pi Zero v1.3 (not Zero 2 W). The USB data port is the OTG port; the PWR IN port is power-only.
