#!/usr/bin/env python3
"""The board's log over its USB serial (USB-Serial-JTAG): resets it first unless --noreset.
    python3 tools/esp32_log.py [SECONDS] [--noreset] [--port /dev/ttyACM0]"""
import sys, time
import serial

secs = float(next((a for a in sys.argv[1:] if not a.startswith("-")), 15))
port = sys.argv[sys.argv.index("--port") + 1] if "--port" in sys.argv else "/dev/ttyACM0"
s = serial.Serial()
s.port, s.baudrate, s.timeout = port, 115200, 0.2
s.open()                     # (both lines high as the kernel leaves them: no reset;
s.rts = False; s.dtr = False  #  RTS dropped first: DTR low with RTS high would reset the chip)
if "--noreset" not in sys.argv:
    s.dtr = False; s.rts = True; time.sleep(0.1); s.rts = False   # EN pulse via USB-JTAG
end = time.time() + secs
while time.time() < end:
    d = s.read(4096)
    if d:
        sys.stdout.write(d.decode("utf-8", "replace")); sys.stdout.flush()
