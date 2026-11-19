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

build/kernel.o: kernel/kernel.c | build
	$(CC) $(CFLAGS) -c $< -o $@

build/kernel.elf: build/start.o build/kernel.o linker.ld
	$(CC) $(CFLAGS) $(LDFLAGS) build/start.o build/kernel.o -o $@

build/kernel.img: build/kernel.elf
	$(OBJCOPY) $< -O binary $@

clean:
	rm -rf build

.PHONY: all clean
