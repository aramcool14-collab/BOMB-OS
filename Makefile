CROSS   = arm-none-eabi-
CC      = $(CROSS)gcc
OBJCOPY = $(CROSS)objcopy

CFLAGS  = -mcpu=arm1176jzf-s -marm -mfloat-abi=soft -ffreestanding \
          -fno-builtin -fno-stack-protector -fno-pic -O2 -Wall -Wextra
LDFLAGS = -nostdlib -nostartfiles -Wl,-T,linker.ld -Wl,--build-id=none

all: build/kernel.img

build:
	mkdir -p build

build/start.o: kernel/start.S | build
	$(CC) $(CFLAGS) -c $< -o $@

build/kernel.o: kernel/kernel.c display/video.h desktop/desktop.h display/framebuffer.h | build
	$(CC) $(CFLAGS) -c $< -o $@

build/framebuffer.o: display/framebuffer.c display/framebuffer.h | build
	$(CC) $(CFLAGS) -c $< -o $@

build/video.o: display/video.c display/video.h | build
	$(CC) $(CFLAGS) -c $< -o $@

build/desktop.o: desktop/desktop.c desktop/desktop.h display/video.h | build
	$(CC) $(CFLAGS) -c $< -o $@

build/bomb_link.o: usb/bomb_link.c usb/bomb_link.h | build
	$(CC) $(CFLAGS) -c $< -o $@

build/dwc2_device.o: usb/dwc2_device.c usb/dwc2_device.h | build
	$(CC) $(CFLAGS) -c $< -o $@

build/kernel.elf: build/start.o build/kernel.o build/framebuffer.o build/video.o build/desktop.o build/bomb_link.o build/dwc2_device.o linker.ld
	$(CC) $(CFLAGS) $(LDFLAGS) build/start.o build/kernel.o build/framebuffer.o build/video.o build/desktop.o build/bomb_link.o build/dwc2_device.o -o $@

build/kernel.img: build/kernel.elf
	$(OBJCOPY) $< -O binary $@

clean:
	rm -rf build

.PHONY: all clean
