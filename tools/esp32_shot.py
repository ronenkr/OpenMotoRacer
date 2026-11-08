#!/usr/bin/env python3
"""A picture of the game's own frame from the board over its USB serial (omr_esp32.c's console),
and keys typed to it.
    python3 tools/esp32_shot.py OUT.png          the picture
    python3 tools/esp32_shot.py --click 320,250     a touch at that game pixel
    python3 tools/esp32_shot.py --env "OMR_PAGE=1 OMR_CHOICES=1,0,4,0,0 OMR_AUTOPILOT=1"
                                                 restart with test settings (once)
    python3 tools/esp32_shot.py --keys "nnr" [--wait 0.5]   type keys (n Return, e Escape, u d l r,
                                                 1..9 F1..F9, a/b/t hold accelerate/brake/turbo)"""
import base64, sys, time
import serial
from PIL import Image

port = sys.argv[sys.argv.index("--port") + 1] if "--port" in sys.argv else "/dev/ttyACM0"
s = serial.Serial()          # (opened with DTR and RTS low: opening must not reset the board)
s.port, s.baudrate, s.timeout = port, 115200, 0.5
s.open()                     # (both lines high as the kernel leaves them: no reset;
s.rts = False; s.dtr = False  #  RTS dropped first: DTR low with RTS high would reset the chip)
if "--keys" in sys.argv:
    wait = float(sys.argv[sys.argv.index("--wait") + 1]) if "--wait" in sys.argv else 0.4
    for k in sys.argv[sys.argv.index("--keys") + 1]:
        s.write(k.encode()); s.flush(); time.sleep(wait)
    sys.exit(0)
if "--env" in sys.argv:                         # --env "OMR_PAGE=1 ...": restart with them
    s.write(b"X" + sys.argv[sys.argv.index("--env") + 1].encode() + b"\n"); s.flush(); time.sleep(0.5)
    sys.exit(0)
if "--click" in sys.argv:                       # --click X,Y (game pixels, 640x480)
    x, y = map(int, sys.argv[sys.argv.index("--click") + 1].split(","))
    s.write(b"c%03d%03d" % (x, y)); s.flush(); time.sleep(0.3)
    sys.exit(0)
out = sys.argv[1]
s.reset_input_buffer()
s.write(b"S"); s.flush()
end = time.time() + 20
w = h = None
rows = []
buf = b""
while time.time() < end:
    buf += s.read(65536)
    while b"\n" in buf:
        line, buf = buf.split(b"\n", 1)
        line = line.strip()
        if w is None:
            if line.startswith(b"SHOT "):
                w, h = map(int, line.split()[1:3])
        elif line == b"END":
            end = 0
            break
        else:
            try:
                r = base64.b64decode(line, validate=True)
            except Exception:
                r = b""
            rows.append(r if len(r) == 2 * w else rows[-1] if rows else bytes(2 * w))
if w is None or len(rows) < h:
    sys.exit(f"no picture ({len(rows)} rows)")
img = Image.new("RGB", (w, h))
px = img.load()
for y in range(h):
    r = rows[y]
    for x in range(w):
        c = r[2 * x] | r[2 * x + 1] << 8
        px[x, y] = ((c >> 11) * 255 // 31, (c >> 5 & 63) * 255 // 63, (c & 31) * 255 // 31)
img.save(out)
print(out, w, h)
