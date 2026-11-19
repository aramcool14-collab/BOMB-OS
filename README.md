# BOMB OS 💣

A from-scratch operating-system project for the **Raspberry Pi Zero v1.3** (ARM1176JZF-S / ARMv6), written in C with a small ARM assembly startup file.

## Current stage

**Milestone 0.01 — UART kernel starter**

- Starts at the Raspberry Pi's legacy ARM kernel load address.
- Sets up a stack and clears the BSS section.
- Initializes the Pi Zero mini UART.
- Prints a BOMB OS startup message.

This is an early kernel starter, **not a finished operating system** and not yet a ready-to-flash SD-card image. It currently reports through the UART pins; it does not show a display over the normal USB cable.

## Project files

- `kernel/start.S` — ARM startup code
- `kernel/kernel.c` — freestanding C kernel and UART output
- `linker.ld` — kernel memory layout
- `Makefile` — build rules for a GNU Make environment
- `build.bat` — Windows Command Prompt build script
- `boot/config.txt` — Raspberry Pi boot configuration

## Build on Windows

A Windows ARM cross-compiler is required. It must provide:

- `arm-none-eabi-gcc`
- `arm-none-eabi-objcopy`

Install a Windows ARM GNU toolchain, add its `bin` folder to PATH, open Command Prompt in this folder, and run:

```bat
build.bat
```

The output should be `build\kernel.img`.

The source folder alone is **not** bootable. A working SD-card boot setup also needs the correct Raspberry Pi firmware files and a properly prepared FAT boot partition. Do not format your SD card for this starter yet.

## Hardware target

Raspberry Pi Zero v1.3 (original non-W model), ARMv6, 512 MB RAM.
