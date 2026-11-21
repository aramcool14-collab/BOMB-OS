@echo off
setlocal
where arm-none-eabi-gcc >nul 2>nul
if errorlevel 1 (
  echo ERROR: arm-none-eabi-gcc was not found.
  echo Install the ARM GNU toolchain and add its bin folder to PATH.
  pause
  exit /b 1
)
where arm-none-eabi-objcopy >nul 2>nul
if errorlevel 1 (
  echo ERROR: arm-none-eabi-objcopy was not found.
  pause
  exit /b 1
)
if not exist build mkdir build
set CFLAGS=-mcpu=arm1176jzf-s -marm -mfloat-abi=soft -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -O2 -Wall -Wextra
arm-none-eabi-gcc %CFLAGS% -c kernel\start.S -o build\start.o
if errorlevel 1 goto failed
arm-none-eabi-gcc %CFLAGS% -c kernel\kernel.c -o build\kernel.o
if errorlevel 1 goto failed
arm-none-eabi-gcc %CFLAGS% -c display\framebuffer.c -o build\framebuffer.o
if errorlevel 1 goto failed
arm-none-eabi-gcc %CFLAGS% -c display\video.c -o build\video.o
if errorlevel 1 goto failed
arm-none-eabi-gcc %CFLAGS% -c desktop\desktop.c -o build\desktop.o
if errorlevel 1 goto failed
arm-none-eabi-gcc %CFLAGS% -c usb\bomb_link.c -o build\bomb_link.o
if errorlevel 1 goto failed
arm-none-eabi-gcc %CFLAGS% -c usb\dwc2_device.c -o build\dwc2_device.o
if errorlevel 1 goto failed
arm-none-eabi-gcc %CFLAGS% -nostdlib -nostartfiles -Wl,-T,linker.ld -Wl,--build-id=none build\start.o build\kernel.o build\framebuffer.o build\video.o build\desktop.o build\bomb_link.o build\dwc2_device.o -lgcc -o build\kernel.elf
if errorlevel 1 goto failed
arm-none-eabi-objcopy build\kernel.elf -O binary build\kernel.img
if errorlevel 1 goto failed
echo.
echo SUCCESS: BOMB OS built for Raspberry Pi Zero v1.3.
echo HDMI desktop, UART debug, USB CDC device and BOMB-LINK frame streaming are included.
pause
exit /b 0
:failed
echo.
echo BUILD FAILED. Copy the error above and send it for help.
pause
exit /b 1
