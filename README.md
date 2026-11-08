# OpenMotoRacer

*Moto Racer* (Delphine Software, 1997, Windows 95), written again in C by reading the
original program, for **Linux and Windows**, and as a hybrid port for the
**[MiSTer](https://github.com/ItsDanik/OpenMotoRacer_MiSTer)**.

It is the game's software renderer, one player on one PC: every track and bike, practice,
single race and the championship, the time attack's ghost, the replays and the TV director,
the game's menus, sounds, voices and music. It plays the 1998 GOG version
(game.EXE, "polygons version 3.22") and the original 1997 CD (MOTO.EXE), each one's own way
(*Options*, *Game Behaviour*). Keyboard, mouse and gamepad; the triggers accelerate and brake
by degrees and a pad rumbles.

Not in it: the Direct3D renderer and the network and modem race.

## Download

Built programs are on the [releases page](https://github.com/ItsDanik/OpenMotoRacer/releases):

- **Windows** (10 or 11, 64-bit): `openmotoracer-<version>-windows-x86_64.zip`. Unpack it, put
  your `Moto Racer` folder next to `openmotoracer.exe`, double-click `openmotoracer.exe`.
- **Linux** (x86-64): `openmotoracer-<version>-linux-x86_64.tar.gz`. It needs SDL2, which most
  systems have (`sudo apt install libsdl2-2.0-0` on Debian, Ubuntu and Mint, `sudo dnf
  install SDL2` on Fedora, `sudo pacman -S sdl2` on Arch). Unpack it, put your `Moto Racer`
  folder next to `openmotoracer`, run `./openmotoracer`.

## You need the game

The port brings none of the game's files. Use your copy of *Moto Racer*, either one of these:

- **GOG**: the folder as installed (`game.EXE`, `data/` and the music `02.wav` to `13.wav`).
- **The 1997 CD**: the files of the CD, and for its music either the CD's image (`.cue` and
  `.bin`, in the same folder, the folder above it, or a folder named `CD` there) or its audio
  tracks as files in a `music` folder there: `track02.ogg` to `track13.ogg` (Ogg Vorbis; `.mp3`
  and `.wav` do as well, and `Track02`, `track_02` or `02` as the name).

Name the folder `Moto Racer` and put it next to the program, or start the program with
`-data <folder>`.

The port reads the game's tables from your `game.EXE` or `MOTO.EXE`, each known build by its
checksum.

## On the MiSTer

[OpenMotoRacer_MiSTer](https://github.com/ItsDanik/OpenMotoRacer_MiSTer) is the hybrid port:
the game on the MiSTer's ARM, a small FPGA core for the 15 kHz picture, the sound and the
controllers. Install and play instructions are there. The MiSTer's code here is the branch
`mister`, behind the `MISTER_HYBRID` build option.

## On the ESP32 (Waveshare ESP32-S3-Touch-LCD-1.83)

The branch `esp32` runs the game on the [Waveshare ESP32-S3-Touch-LCD-1.83](https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-1.83)
(ESP32-S3, 8 MB PSRAM, 240x284 touch LCD, ES8311 speaker), held sideways with the USB connector on
top: a race at about 25 pictures a second, with its music and sounds. It plays the 1997 CD's data.

**Controls.** Tilt the board to steer (the way it is held when a race starts is straight). BOOT
accelerates, the touch screen's left half brakes and its right half is turbo, PWR pauses. In the
menus a tap is the mouse (the nearest button takes it, pressed when the finger lifts), BOOT is
Return and PWR is Back; a tap skips the logo and starts from the title.

**The SD card** (FAT32), made from your CD's image (`.cue` with its `.bin` files) with Python 3,
numpy and 7-Zip:

```sh
python3 tools/esp32_card.py "path/to/Moto Racer (USA).cue" build/sdcard
cp -r build/sdcard/MOTO build/sdcard/OMR /media/<your card>/
```

`MOTO/` holds the game's `DATA/`, `MOTO.EXE` and the music as `music/trackNN.wav` (22 kHz mono);
`OMR/` is where settings and saves go.

**Building and flashing** with [ESP-IDF](https://docs.espressif.com/projects/esp-idf/) v6.0:

```sh
. ~/esp/v6.0/esp-idf/export.sh
idf.py -C esp32 build
idf.py -C esp32 -p /dev/ttyACM0 flash
```

How it was fitted into 8 MB and made fast is in `docs/NOTES.md` §4.26.

## Building

C11, CMake and SDL2:

```sh
cmake -S port -B port/build && cmake --build port/build
port/build/openmotoracer -data "/path/to/Moto Racer"
```

`-data` (or `OMR_DATA`) is your Moto Racer folder; `-home` (or `OMR_HOME`) is where settings
and saved games go, by default the folder it runs in. `--help` lists the rest.
`tools/release_linux.sh` and `tools/release_windows.sh` make the release packages (in `dist/`),
and `tools/arm.sh` makes the MiSTer's ARM build.

## How it was made

The original program was read with Ghidra, and every part of it was written again by hand as
clean C that does the same thing. No machine-converted code is in this repository, and none of
the game's own files are. `docs/NOTES.md` is the full record: the program's map, the file
formats, and how each part was checked against the running original. `tools/` holds the
scripts used for that; they need your own copy of the game.

## Licence

GPL-3.0 (`LICENSE`). *Moto Racer* is Delphine Software's; OpenMotoRacer is not connected
with them or with its publishers, and the game itself remains theirs.
