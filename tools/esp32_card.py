#!/usr/bin/env python3
"""The ESP32 board's SD card from the player's 1997 CD image (a .cue with its .bin files).

    python3 -I tools/esp32_card.py data/"Moto Racer (USA).cue" build/sdcard
    then copy build/sdcard/* to the card's root (FAT32): MOTO/ and OMR/

MOTO/ is the game folder the port is given (-data): the CD's DATA/ as it is, MOTO.EXE taken
out of DISK1/data.z (exe.c reads moto.exe first, so the 75 MB archive stays off the card),
and music/trackNN.wav: the CD's audio tracks at 22050 Hz mono 16-bit, the board's mixer
rate and its one speaker, so the music costs a file read and nothing else on the board.
OMR/ is the home folder (settings, saves), made empty.
"""
import re, shutil, struct, subprocess, sys, tempfile
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
import is3  # noqa: E402  (tools/is3.py: the InstallShield 3 archive)


def parse_cue(cue):
    """[(file, track, mode, index01 sector within its file)]"""
    out, cur_file = [], None
    for line in cue.read_text().splitlines():
        m = re.match(r'\s*FILE "(.+)" ', line)
        if m:
            cur_file = cue.parent / m[1]
        m = re.match(r"\s*TRACK (\d+) (\S+)", line)
        if m:
            out.append([cur_file, int(m[1]), m[2], 0])
        m = re.match(r"\s*INDEX 01 (\d+):(\d+):(\d+)", line)
        if m:
            out[-1][3] = (int(m[1]) * 60 + int(m[2])) * 75 + int(m[3])
    return out


def half_band(n=63):
    """a windowed-sinc low-pass at a quarter of the rate (44.1 kHz -> 22.05 kHz)"""
    k = np.arange(n) - (n - 1) / 2
    h = np.sinc(k / 2) / 2 * np.blackman(n)
    return h / h.sum()


def write_wav(path, pcm16, rate):
    data = pcm16.astype("<i2").tobytes()
    hdr = b"RIFF" + struct.pack("<I", 36 + len(data)) + b"WAVEfmt " + struct.pack(
        "<IHHIIHH", 16, 1, 1, rate, rate * 2, 2, 16) + b"data" + struct.pack("<I", len(data))
    path.write_bytes(hdr + data)


def main():
    cue, out = Path(sys.argv[1]), Path(sys.argv[2])
    tracks = parse_cue(cue)
    moto = out / "MOTO"
    (moto / "music").mkdir(parents=True, exist_ok=True)
    (out / "OMR").mkdir(parents=True, exist_ok=True)
    h = half_band()
    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)
        for f, n, mode, start in tracks:
            if mode.startswith("MODE1"):
                iso = tmp / "data.iso"
                with open(f, "rb") as i, open(iso, "wb") as o:
                    i.seek(start * 2352)
                    while True:
                        s = i.read(2352)
                        if len(s) < 2352:
                            break
                        o.write(s[16:16 + 2048])
                subprocess.run(["7z", "x", "-y", f"-o{tmp / 'iso'}", str(iso), "DATA", "DISK1/data.z"],
                               check=True, stdout=subprocess.DEVNULL)
                if (moto / "DATA").exists():
                    shutil.rmtree(moto / "DATA")
                shutil.copytree(tmp / "iso" / "DATA", moto / "DATA")
                exe = [packed if len(packed) == usize else is3.explode(packed)
                       for folder, name, usize, packed in is3.read_is3(tmp / "iso" / "DISK1" / "data.z")
                       if name.lower() == "moto.exe"]
                if not exe:
                    sys.exit("MOTO.EXE not found in DISK1/data.z")
                (moto / "MOTO.EXE").write_bytes(exe[0])
                print(f"track {n:02d}: data, DATA/ and MOTO.EXE ({len(exe[0])} bytes)")
            else:
                raw = Path(f).read_bytes()[start * 2352:]
                st = np.frombuffer(raw[:len(raw) // 4 * 4], dtype="<i2").reshape(-1, 2).astype(np.float64)
                mono = (st[:, 0] + st[:, 1]) * 0.5
                lo = np.convolve(mono, h, mode="same")[::2]
                write_wav(moto / "music" / f"track{n:02d}.wav",
                          np.clip(np.round(lo), -32768, 32767).astype(np.int16), 22050)
                print(f"track {n:02d}: audio, {len(lo) / 22050:.1f} s")
    print("banks:")
    for b in sorted((moto / "DATA").glob("*.BKF")):
        print(f"  {b.name:14s} {b.stat().st_size:9d}")
    total = sum(p.stat().st_size for p in out.rglob("*") if p.is_file())
    print(f"{out}: {total / 1e6:.1f} MB")


if __name__ == "__main__":
    main()
