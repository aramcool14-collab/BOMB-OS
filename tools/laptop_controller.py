# BOMB OS laptop controller
# Install once: py -m pip install pyserial pynput
# Run: py laptop_controller.py COM5
# Replace COM5 with the serial port shown for the Pi in Windows Device Manager.

import sys
import threading
import time

import serial
from pynput import mouse, keyboard

if len(sys.argv) != 2:
    print("Usage: py laptop_controller.py COM5")
    raise SystemExit(2)

port = sys.argv[1]
try:
    link = serial.Serial(port, 115200, timeout=0, write_timeout=0.1)
except Exception as exc:
    print(f"Could not open {port}: {exc}")
    raise SystemExit(1)

lock = threading.Lock()
last_xy = None
running = True

def send(packet):
    if not running:
        return
    try:
        with lock:
            link.write(bytes(packet))
    except serial.SerialException:
        pass

def clamp_delta(value):
    return max(-127, min(127, int(value)))

def on_move(x, y):
    global last_xy
    if last_xy is None:
        last_xy = (x, y)
        return
    dx = clamp_delta(x - last_xy[0])
    dy = clamp_delta(y - last_xy[1])
    last_xy = (x, y)
    if dx or dy:
        send((ord("B"), ord("I"), 1, dx & 0xff, dy & 0xff, buttons))

buttons = 0

def on_click(x, y, button, pressed):
    global buttons
    bit = 1 if button == mouse.Button.left else 2 if button == mouse.Button.right else 4
    if pressed:
        buttons |= bit
    else:
        buttons &= ~bit
    send((ord("B"), ord("I"), 1, 0, 0, buttons))

def key_number(key):
    try:
        value = key.char
        if value:
            return ord(value) & 0xff
    except AttributeError:
        pass
    try:
        return int(key.vk) & 0xff
    except Exception:
        return 0

def on_key_down(key):
    if key == keyboard.Key.esc:
        return False
    send((ord("B"), ord("I"), 2, key_number(key), 1, 0))

def on_key_up(key):
    send((ord("B"), ord("I"), 2, key_number(key), 0, 0))

print(f"BOMB OS controller started on {port}.")
print("Move the trackpad to move the Pi pointer; click to send mouse buttons.")
print("Keyboard events are forwarded. Press ESC here or stop this window to quit.")
print("This requires the Pi USB serial device to enumerate successfully.")

try:
    with mouse.Listener(on_move=on_move, on_click=on_click) as mouse_listener:
        with keyboard.Listener(on_press=on_key_down, on_release=on_key_up) as key_listener:
            key_listener.join()
finally:
    running = False
    link.close()
