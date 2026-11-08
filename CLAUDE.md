# OpenMotoRacer — *Moto Racer* (1997, Delphine Software, Windows 95) for Linux and the MiSTer

Reverse engineering and a hand-written reimplementation of `game.EXE`, the Windows program
of *Moto Racer* (the 1998 "polygons version 3.22" patch, as GOG sells it). A native SDL2
port of the **software renderer** (Linux x86, Windows, then ARM), then a MiSTer hybrid core
(game on the ARM, the FPGA for 15 kHz video, audio and input).

**This file is the summary. `docs/NOTES.md` is the full record**: addresses, proofs,
formats, dead ends. New findings go there in detail and here as one line.
**`docs/HANDOVER.md` is where the work stands**: read it first in a new session.

---

## Working agreement — read this first

**Danik does not write code or decompile.** He knows the MiSTer and these ports, and he
does the verification and judges whether it plays like the original. Don't hand him code
to review, patches to apply or compiler errors to sort out.

- **Claude:** all code, tooling, decompilation, builds, debugging. This machine has
  Ghidra 12.1.2 (`/opt/ghidra`), python3 (pefile, PIL), wine/proton, objdump. The `Read`
  tool opens PNGs: look at the pictures you make yourself.
- **Danik:** real hardware, whether it looks and feels like the original, and decisions
  about scope, licences and targets.

"Here is a build and a screenshot" beats "please check this function". Everything builds
with one command. Plain language, not jargon.

**The method is the skill `reverse-engineer-dos-game`** (`~/.claude/skills/`), as on
ReNFS (`~/renfs`) and OpenScreamer (`~/openscreamer`). This game is a **Windows** program
(MSVC, PE), not DOS: Ghidra's own PE loader replaces `unle.py`, and the original is run
under wine instead of DOSBox. Their "Hard-won rules" hold here too, above all: **when a
format question stalls, find the code that reads the field.**

---

## Decisions (Danik, 2026-10-26)

- Engine name **OpenMotoRacer** (`~/openmotoracer`); MiSTer id `MotoRacer`.
- **The player provides the GOG copy as installed** (`Moto Racer/`: `game.EXE`, `data/`,
  the music `02.wav`–`13.wav`), **and the original 1997 CD is to be supported too** (its
  image in `CD/`, its files extracted to `Moto Racer CD/`, neither committed). The port reads the
  game's tables from the player's `game.EXE`, each known build by its hash.
- **Scope: the software renderer, one or two players on one PC.** No Direct3D renderer, no
  network or modem play (DirectPlay).
- **Hand-written from the start.** Decompiled code is read and clean C is written that does
  the same. Machine-converted code only as an unpublished reference under `build/`.
- **Public since 2026-11-04** (Danik, for the MiSTer release): `github.com/ItsDanik/OpenMotoRacer`, GPL-3.0
  (`LICENSE`), branches `master` and `mister` only; commit after each finished step.
- **PC releases** as ReNFS's: a GitHub pre-release `v<version>` with `tools/release_linux.sh` and
  `tools/release_windows.sh`'s packages (`dist/`), the version from `VERSION` and `tools/version.sh`;
  `tools/shots.sh`, `tools/replaycheck.sh` and `tools/windows.sh check` first. 0.6.0b the first
  (2026-11-04, from master after mister was merged into it; mister kept equal to master since).

## Legal

The game is still its publisher's. Never publish `Moto Racer/` or anything made from it:
the flat image, the Ghidra project, `build/decomp`, unpacked banks, pictures, converted code.
All of that stays in the git-ignored `build/`. Decompiled text is never pasted into `port/`.
`git status` before every commit.

## The binary and how to decompile it

`Moto Racer/game.EXE` (SHA-1 `164b4175…`, linked 1998-07-09): PE32, MSVC C++ (sources
`D:\Dsed\mr320\...`), base 0x400000, entry 0x4b6420. DirectDraw, Direct3D (immediate mode),
DirectSound, DirectPlay, WinMM (CD audio through MCI). `MotoRacer.exe` is only GOG's
launcher (Delphi): it writes `motoracer.ini` and starts `game.EXE`.

- `tools/rebuild.sh`: Ghidra imports the PE, analyses, seeds functions from the relocation
  records, renames from `tools/names.txt`, decompiles everything into `build/decomp/`.
  `tools/rebuild.sh --decomp` after new names.
- `tools/pe2flat.py`: the program laid out flat at its addresses (`build/mr_flat.bin`) and
  the layout map with every pointer site (`build/mr_layout.json`) for `strmap.py`,
  `seeds.py`, `strrefs.py`.

## The port (`port/`)
C11, CMake, SDL2 linked dynamically. Built with one command:
`cmake -S port -B port/build && cmake --build port/build` → `port/build/openmotoracer`.
- `-home DIR` / `OMR_HOME`: where saves are written (`saves.c`; default the working directory,
  Danik's call), the game's format; read from there, else `~/.config/openmotoracer` where they used to go, else the game's data/.
- `-data DIR` / `OMR_DATA`: the player's Moto Racer folder (game.EXE + data/; default
  `./Moto Racer`). Names matched in any case. Files through the banks first (`files.c`).
- **The game's tables are read from the player's game.EXE** at their addresses
  (`exe.c`: the PE placed at 0x400000, the build known by its FNV-1a sum; unknown builds
  are refused with a message).
- The CD: `-data` at the CD's files works alone, `MOTO.EXE` read from `DISK1/data.z` (`exe.c`, NOTES §3.5a), the 1997 program's behaviour by default; title and main menu equal to MOTO.EXE's; its music from the CD's image (`cdaudio.c`, §4.10.5.3a), or from the player's own files of it, `music/track02.ogg`... (.ogg, .mp3, .wav; `sound.c`, §4.10.5.3b). MOTO.EXE runs a whole race under wine (`-noD3D`, §3.5a); it differs from GOG's in 92 routines (§3.5b).
- The screen is the game's: RGB565 (`video.c`), shown 4:3; menus 640x480, the race also 512x384 and 320x240 and 16:9 at the top (F2, F3, F4, the F12 column, Shift+F12; `OMR_SCREEN=MODE[,WIDE]`; NOTES §4.20 §1b, §4.19 §5b).
- `--shot FILE N` (`OMR_SHOT`): no window, N frames, the screen as a BMP. `--help` lists
  the test switches (`OMR_*`).
- `clock.c`: the game's millisecond clock (timeGetTime): the real time in a window, 1000/60 a
  picture under `--shot`. What the game counts in pictures (tiles, blinking, drifting texts)
  counts 60ths of a second (`g_ticks`), so any refresh rate gives the original's speed.
- `tools/shots.sh`: the test pictures' sums; `tools/shots.sh | diff build/shots.txt -` after
  every change.
- `menu.c`: the menu core (NOTES §4.4): pages by the game's numbers, page change, buttons,
  keys (SDL keys turned into Windows key codes) and mouse, the game's own pointer.
- Pages written: 25 logo and 14 title (`page_intro.c`), 15 name entry (`page_name.c`, its
  3D letters through `obj3d.c`, NOTES §4.11), 3 main menu (`page_main.c`), 4 quit,
  5 SELECT MODE (`page_mode.c`), 6 SELECT RACE (`page_track.c`), 10 CHOOSE BIKE
  (`page_bike.c`, `bikes.c`, its turning 3D bike `bike3d.c`, NOTES §4.15), 9 TOP 5 (`page_top5.c`, `scores.c`:
  bestscor.dat), 1 loading (`page_load.c`: back to page 3 until the race exists), 20/21
  championship Load/Save (`page_champ.c`), 11 Options, 12 controls, 13 diagnostic, 23 data
  disk (`page_options.c`, NOTES §4.14); the race
  settings and track lists in `choices.c` (NOTES §4.12). A page not written yet ends the
  program with a message. Starts on 25, as the game.
- Settings: `config.c` reads CONFIG.BPT (the player's home copy first, else the game's)
  and writes it to the home folder when the program ends, as the game does (NOTES §4.14.5).
  The track list too (`choices.c`: LIST.BPT, LIST01.BPT, what's unlocked); `tools/unlock_all.sh` every track on, `--undo` back.
- 3D (NOTES §4.9): `r3d.c` the software renderer (float transform, depth buckets, s-buffer,
  affine triangles; the kind drawn lives in the model's face, shared by its copies, §4.13.4), `model.c` 3DF+MAP, `rand.c` the game's random streams (seeds from
  game.EXE). The glass transition is `menu.c`'s.
- Race's sound measured against the original (NOTES §4.10.5b): levels within 0.1 dB; lost voices and the commentary 0x407420 fixed.
- Sound (NOTES §4.10): `sound.c`, the effects by the game's id tables, its queues of
  delayed voices, the music as GOG plays it (NN.wav, looping). `OMR_WAV=FILE` records it.
- Text (NOTES §4.8): `script.c` reads BPT, `font.c` the 35 fonts (FONTS.BPT + .FNT),
  `text.c` the 48 text objects (blink, fades, see-through, drift).
- The race's track (NOTES §4.13): `land.c` (Level.Spt, .trk, section walk, what a camera
  sees), `horizon.c`, and `r3d_race()` (near 64, the detail remap, the sub-6 perspective
  spans). `OMR_RACEVIEW=TRACK,FOCAL,X,Y,Z,C00..C22` draws a track from a still camera.
  `signs.c` (§4.13.5) the turn signs (F8, DisplaySignal) and the place's figure; `land.c` moves
  the boats and gulls (AnimDecor). The .trk's "billboards" (+0x5c) are dead code in the game.
- The race (NOTES §4.16): `race.c` (load, grid, frame: track, sky, riders), page 1 hands
  over; `rules.c` (§4.17: countdown, time limit, laps, places with the game's qsort);
  `race_sound.c` (§4.10.6) the engines, skids, crowd and voices, and the music; `hud.c` (§4.20 §3) the dashboard's texts, countdown and
  messages and the dials (equal to the captures to the pixel), GAME OVER a sprite object
  (`sprite.c`: the Texts .RAW sprites, keyed blit; §4.20 §3.5); `dialog.c` (§4.19) the
  in-race dialogs (pause, results, continue); the end of the race and bestscor.dat.
  `pose.c` the rider's body each step (NOTES §4.18 8.16a): speed, lean, jumps, stunts, looks, salute, fall.
  `shadow.c` the bikes' shadows and the crash's second one, `effects.c` smoke, spray, tyre marks and the
  crash's debris (NOTES §4.15.9, §4.15.10); lit and dark bikes through `bike3d_lighting`.
  `bikephys.c` the player's bike (bit-exact on recorded steps, `tools/physcheck.sh`),
  `camera.c` the chase camera 0x46f840 (F5 distance, Tab looks back) and the on-board view
  (F6, mode 6 0x4700e0) with the cockpit (`bike3d_cockpit`, page 7, §4.20 mode 6).
  `cpu.c` the computer riders (§4.21, bit-exact on recordings, `tools/cpucheck.sh`).
  Smooth pictures between the 62.5 Hz steps (F10, the port's own settings file).
  SmallFonts (CONFIG, default 1) picks the race's small menu font.
  The race's keys: F1 the help (`generic.c`, §5a), F11/F12 the columns and Ctrl+S/V/A (`race_sound.c`, §5b),
  the dialogs' mouse and the game's own pointer (`pointer.c`, §4.19 §2.6a), the CD's sprite messages on the
  CD's data (§4.20 §3.5), texts keeping out of each other's way (`text.c`, §4.8.3.8), the music's pause (§4.10.5.3).
- Replays, demos, the TV, the winner's lap (NOTES §4.21b): `replay.c` the recording, played back
  to the bit (`tools/replaycheck.sh`), the demos after 30 s on the title, `camera.c`'s director,
  the record split line in `rules.c`; `generic.c` the .GEN scripts and the victory credits.
  `OMR_AUTOPILOT=1` the computer rides the player's bike (`=win`: and the others crawl, no time
  limit, to reach the winner's lap), `OMR_RACELOG=FILE` a sum a step, `OMR_WATCH_AT=N` replay at
  step N.
- The time attack's ghost (NOTES §4.21c): `ghost.c`, the game's file `<track>/timatak[R_][_]<laps>.bin`
  in the home folder (the original reads the port's, and the reverse), played as rider 2, Reset.
- Joysticks (NOTES §4.22): `joy.c`, SDL's joysticks as the game's WinMM ones (dead zone, rescale,
  the bindings 0x411b30, the dialogs' arrows, Ctrl+J/K); `-jN` / `OMR_JOYDEAD` the dead
  zone, `OMR_DEVICE` the input device, `OMR_INPUT` `F:joy:X,Y,B` a pretend joystick.
- A gamepad, the port's own (NOTES §4.22): `joy.c` through SDL_GameController, triggers bindable and by degrees (key bits 8..23), RT and LT accelerate and brake without being set (`joy_spare_trigger`), `tools/padtest.sh` what a real pad sends, the settings page's Analog and Rumble (openmotoracer.ini, both on), `pad.c` the cross and stick in the menus and dialogs, Start the pause; `OMR_INPUT` `F:pad:X,Y,B,LT,RT,CROSS`.
- The MiSTer's ARM (NOTES §4.23): `tools/arm.sh` the static build, `tools/arm.sh check` all pictures and replays on it
  under qemu (the same as the PC's); `OMR_PERF=1` the speed readout.
- The MiSTer core (NOTES §4.25, branch `mister`, `~/MiSTer-openmotoracer`): both CPUs draw the race (`R3D_TWO`, every other row: the same pixels), 320x240 and 640x240 a steady 60 on the real core on every track; `OMR_ONE_CPU=1` one CPU; `-DOMR_PROF=ON` the profile, `tools/prof.py`; the ARM's build in Thumb with LTO.
- The race's small keys (NOTES §4.20 §2.6): F9 the map (`trackmap.c`), numpad + and − the detail, * and / the volumes, Ctrl+F the
  fonts, Shift+F1 the size (`racekeys.c`), Ctrl+U; T, A, Shift+A, Ctrl+H left out. Losing the focus (§4.10.5.3): music held, no texts,
  race goes on. `-windowed`, `-fullscreen`, `-NoSound`, Alt+Return (the window starts as CONFIG says). Link stays on page 3.
- Windows and the soak (NOTES §4.24): `tools/windows.sh` the MinGW build with SDL2.dll in `build/win/`, `tools/windows.sh check` all pictures, replays and
  checks on it under wine (5-6-5: SDL2.dll rounds its BMPs differently); home `%APPDATA%\OpenMotoRacer`. `python3 tools/soak.py` the sanitizers over every track, mode and page.
- A bike is drawn only near the view and on a section the camera sees (`bike_seen`, 0x458b80), each bike has its own shadow faces; every way of drawing a face in the game's tables is written (`r3d.c`, NOTES §4.24). `build/shots.txt` is the port against itself: compare the race's picture against `build/ref/` too.
- Game Behaviour (Options, `behaviour_cd()`, `OMR_BEHAVIOUR=cd|gog`; NOTES §3.5c): which program the port acts like, with any copy's files. What follows the files and what the setting is listed there. The bike under the setting checked by `tools/physcheck_cd.sh` against MOTO.EXE's steps (16547 of 16549, §3.5c).
- GO's test uses the seconds of the last clock split (0x4f0d8c): the lap panel's, not the time left (NOTES §4.17 §6; West Way on easy once lost its GO). `OMR_DIFFICULTY` for tests.
- Test switches: `OMR_PAGE`, `OMR_LIST` (the track list), `OMR_CHOICES="MODE,TRACK,LAPS[,REV,POC]"`, `OMR_TILE=SX,SY` (the chequered tile's scroll, to match a capture;
  `tools/cmpshot.py` finds it), `OMR_INPUT="F:key F:hold:key F:release:key F:move:X,Y F:click F:down F:up"` (input at frame F),
  `OMR_TEXT="FONT,X,Y,ALIGN,EFFECT,BLINK,FORE,BACK,TEXT"` (one text object).
- The ESP32-S3 (branch `esp32`, NOTES §4.26): `esp32/` (ESP-IDF v6) for the Waveshare ESP32-S3-Touch-LCD-1.83, an SDL subset on FreeRTOS, `OMR_ESP32`; tilt, touch, BOOT, PWR; ~25 a second (single-precision physics: not bit-exact there), `tools/esp32_card.py`, `tools/esp32_shot.py`.
- Progress tracker for Danik: https://claude.ai/artifact/CCupuhZmsCRgz7FA5eFMxz

## File formats

| Ext | What | Status |
|-----|------|--------|
| .BKF | bank: u32 count, 0x2c-byte records (name[0x24], offset, size), files | `tools/bkf.py`, all 28 unpack |
| .RAW | Image Alchemy "mhwanh" picture (BE header, palette, 8-bit pixels) | `tools/raw.py` |
| .LEZ | packed 8-bit TGA (`LEZ1`, LZ with 10-bit distances) | `tools/lez.py`, all 755 |
| .FNT | bitmap font: first, count, height, palette size, palette, widths, glyphs | `tools/fnt.py`, all 34; NOTES §4.8 |
| .BPT | the .Spt scripts as tokens, XOR with Park-Miller from 0x1a2b3c4d | `tools/bpt.py`, all |
| .3DF .MAP | model (float vertices x96, faces) + per-face texture mapping | read from 0x4216a0, NOTES §4.5 |
| .SKL .ROT | the rider + bike skeleton (17 bones), poses (3 angles a bone) | `tools/bikeparts.py`, all 10 + 106; NOTES §4.15.3 (.PIV, .SNI: never read) |
| .trk | track (`TRK\0`, v5: sections, road blocks, decor library and objects) | `tools/track.py`, all 10; NOTES §4.13 |
| .WAV | effects and voices in SFX.BKF: RIFF, enciphered (byte − 4(i+8) per block), mono | `tools/sfx.py`, all 250; NOTES §4.10 |
| .dem | demo list (text: track, time, picture) | read |
| data.z | the CD's InstallShield 3 archive (files stored) | `tools/is3.py` |

## How the game works (the summary; NOTES §4)
- MFC app; `App_OnIdle` 0x415960 is the frame: the menu (page table 0x4dbb38) or the race
  (`Game_Frame` 0x47d660). One step a frame of the measured time (ms x 0.1, at most 200 ms):
  the port runs fixed 1/60 s steps.
- The picture is **16-bit RGB565** (320x240, 512x384, 640x480).
- Running the original: `tools/orig.sh start|shot|key|stop`, unseen (headless Weston +
  Xwayland; the game only moves while its window has focus). `ORIG_DISPLAY=:1` is Danik's
  own screen (allowed). `tools/win/memtool.c` reads its memory (NOTES §5).
