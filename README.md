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


## Laptop input bridge (experimental)

The Pi's USB CDC device driver now has a receive queue for input packets. The kernel parses a small BOMB Input (BI) packet and draws a pointer on HDMI.

On Windows, install the helper dependencies:

```bat
py -m pip install pyserial pynput
```

After the Pi appears as a COM port in Device Manager, run:

```bat
py tools\laptop_controller.py COM5
```

Replace COM5 with the actual port. Move the laptop trackpad to move the Pi pointer, click to send mouse-button state, and keyboard events are forwarded. Press ESC to stop the helper.

**Important:** this is experimental. The Pi USB CDC connection has not yet been verified on the physical Raspberry Pi Zero. The pointer is currently a basic overlay; desktop windows, app clicking, and keyboard-driven editor controls are not connected yet. If Windows does not show a COM port, the bridge cannot connect.

## BOMB language preview

BOMB is being developed as a small English-like language. Current interpreter commands include:

```text
say "Hello!"
set score 10
add score 5
subtract score 2
print score
make app MyFirstApp
label "Welcome!"
button "Press me"
end
```

See `examples/hello.bomb` and `examples/my_first_app.bomb`. Variable storage is currently limited to eight simple integer variables. App layout commands are parsed as app events; the visual app builder/runtime is still under development.
