@echo off
setlocal
where arm-none-eabi-gcc >nul 2>nul
if errorlevel 1 (
  echo ERROR: arm-none-eabi-gcc was not found.
  echo Install a Windows ARM GNU toolchain and add its bin folder to PATH.
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
arm-none-eabi-gcc -mcpu=arm1176jzf-s -marm -mfloat-abi=soft -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -O2 -Wall -Wextra -c kernel\start.S -o build\start.o
if errorlevel 1 goto failed
arm-none-eabi-gcc -mcpu=arm1176jzf-s -marm -mfloat-abi=soft -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -O2 -Wall -Wextra -c kernel\kernel.c -o build\kernel.o
if errorlevel 1 goto failed
arm-none-eabi-gcc -mcpu=arm1176jzf-s -marm -mfloat-abi=soft -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -O2 -Wall -Wextra -nostdlib -nostartfiles -Wl,-T,linker.ld -Wl,--build-id=none build\start.o build\kernel.o -o build\kernel.elf
if errorlevel 1 goto failed
arm-none-eabi-objcopy build\kernel.elf -O binary build\kernel.img
if errorlevel 1 goto failed
echo.
echo SUCCESS: build\kernel.img created.
pause
exit /b 0
:failed
echo.
echo BUILD FAILED. Copy the error above and send it for help.
pause
exit /b 1
