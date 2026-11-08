# OpenMotoRacer — the full record

Every claim carries its address and its proof. Dead ends are written down too.

## 1. The program

- `Moto Racer/game.EXE`, 1,133,056 bytes, SHA-1 `164b417553a8df4284b908405b66c269cc6d94e5`,
  PE timestamp 1998-07-09 14:15:27. GOG's copy; the readme calls it "Polygons version 3.22"
  (the patch that fixed Voodoo 2 freezes). MSVC C++ (`Microsoft Visual C++ Runtime
  Library`), source paths `D:\Dsed\mr320\` (`screen3d\bankw.Cpp`, `menu\menu.cpp`,
  `Dsi_d3d.cpp`, `frame3d.cpp`, `event.cpp`, `player.cpp`, `animDecor.cpp`, ...).
- Sections: `.text` 0x401000 (0xce600), `.rdata` 0x4d0000, `.data` 0x4db000 (0x1a400 in the
  file, 0x231568 in memory: a 2.2 MB BSS), `.idata` 0x70d000, `.rsrc`, `.reloc` (35,560
  pointer sites). Entry 0x4b6420.
- Imports: WINMM, DDRAW, DPLAYX, DSOUND, KERNEL32, USER32, GDI32, comdlg32, WINSPOOL,
  ADVAPI32, SHELL32, COMCTL32.
- Command-line switches (strings): `-NoD3D`, `-windowed`, `-fullscreen`, `-NoSound`,
  `-Pal16`, `-ZBuffer`, `-Alpha`, `-SnapShot`, `-saverecord`, `-savpath`, `-frameratemax`,
  `-nbcomp`, `-Modem`, `-fr -gr -it -sp -sw -us` (language), and many Direct3D tuning ones.
  In the port: `-windowed`, `-fullscreen` (0x4dc1f0: over the settings' FullScreen, 0x42b0d9, and then
  saved like any other value), `-NoSound` (0x4dc1a8, any case, §4.10), each also `OMR_WINDOWED`,
  `OMR_FULLSCREEN`, `OMR_NOSOUND`. The port's window now starts as the settings say: GOG's CONFIG has 1,
  so a first run is on the full screen, like the game's.
- GOG adds: `MotoRacer.exe` (Delphi launcher: `motoracer.ini` → switches for `game.EXE`),
  `DDraw.dll`/`D3DImm.dll`/`dgVoodooCpl.exe` (dgVoodoo 2), `winmm.dll` (CD audio from
  `NN.wav`), `goggame-*.dll`.
- Other builds: the 1997 retail CD (to be added; Danik will provide it).

## 2. The data (`Moto Racer/data/`)

- 28 `.BKF` banks: `MENU` (+ `MENUFR/GR/IT/SP/SW/US` per language), `PAGES`, `PODIUM`,
  `DEMO`, `DESIGN`, `FONTES`, `GENERIC`, `SFX`, the bikes (`SPEED`, `SPEEDPKT`, `KTMCROSS`,
  `KTMPKT`), and the tracks `ILE01`–`ILE11` (no 07).
- `.BPT` loose: `CONFIG`, `LIST`/`LIST01`, `CHAMPA/B/C` (+`01`), `FONTS`, `MOTOKTM`,
  `MOTOSPD`, `PKTKTM`, `PKTSPEED`. Enciphered (CONFIG.BPT and LIST.BPT share a long run of
  identical bytes after their counts, so a fixed key stream, §3.4).
- `data/ileNN/*.trk` (one a track, `TRK\0` + floats), `data/demo/EA.dem`, `BMG.dem`
  (text lists of two demo runs: track name, 90000, a picture), `bestscor.dat`,
  `config.txt` (`-us`), `delphine.err` (the game's error log).
- Music: `02.wav`–`13.wav` beside the program (the CD's audio tracks, through GOG's winmm).
  They are the CD's tracks 2–13 sample for sample, the same length, shifted by 532 samples
  (2128 bytes: a ripping offset). `tools/cdbin.py` cuts the CD image into `build/cd/NN.wav`
  and `data.iso`.

- **The 1997 CD's `DATA/` against GOG's** (2026-10-26, file by file inside the banks): the
  same files apart from: `FONTES` (22 of the CD's 34 fonts differ, GOG has 2 more),
  `DESIGN` (`TEXTS\LOWRES/HIRES/GAMEOVER.RAW` differ, GOG has one more), `KTMCROSS`
  (`CARAC2/4.BPT` differ, GOG 2 more), `ILE09` (`BNK00_02.RAW`, `BNKD3D02.RAW` differ; the
  CD has 3 more), `DEMO` (the CD has 2 more), `MENU` (`BITMAP\EALOGO.LEZ`), `CONFIG.BPT`,
  `FONTS.BPT`. GOG alone has `ILE10`, `ILE11` (the patch's two tracks?) and the `*01.BPT`
  lists. Banks of equal size still differ as files: the junk after each name's zero
  differs. So the CD's program reads data the GOG program also reads, nearly all of it
  the same; its own tables (MOTO.EXE, 1997-07-21) are at other addresses (to line up).
- **MOTO.EXE's tables against game.EXE's** (2026-10-26, pefile images, `build/moto.asm`):
  the menu pictures 0x4d9840 (209 records, as GOG's 0x4e3e20), the language folders
  0x4da8a8 (GOG 0x4e8b60), the random seeds 0x4d2bb8 (GOG 0x4f2590; the first 7 equal,
  **the 8th is 0** where GOG has 0x73616c43). **No table of named colours** (GOG 0x4f0dd4):
  the 1997 build pushes R, G, B as three immediates before its colour call (e.g. 0x46723c:
  255,255,255 then 0x423620), a different font interface. So the CD's program is not
  GOG's with tables moved: its text code differs (and 22 of its 34 fonts). Open: run the
  CD build as GOG's logic with the CD's data (colours from GOG's values?), or ask CD
  players for the official 3.22 patch (whose game.EXE may be GOG's own, to check).
  (Done for now, the first of the two: §3.5a.)

## 3. Formats

### 3.1 BKF banks
u32 count; count records of 0x2c bytes: name (0x24 bytes, zero-ended; the rest is old
memory), u32 offset from the file's start, u32 size. Files follow in order without gaps
(MENU.BKF has 0x4419 unreferenced bytes after its records, and two names, EALOGO and
LODANIM, share one file). Every bank ends exactly where its last file does.
`tools/bkf.py` lists and unpacks (`-x build/bkf`). 2,200 files: 755 LEZ, 418 RAW, 250 WAV,
132 MAP, 128 3DF, 106 ROT, 52 BPT, 34 FNT, 25 GEN, 10 SKL, 9 PIV, ...

### 3.2 RAW pictures
Image Alchemy's "mhwanh": 32-byte header, big-endian: magic, u16 version (4), u16 width,
u16 height, u16 palette entries; then the palette (3 bytes an entry), then 8-bit pixels.
402 of 418 have no palette (the track and bike texture pages, 256x256 and 128x128: the
palette comes from elsewhere). `tools/raw.py`.

### 3.3 LEZ pictures
A packed TGA (unpacker `Lez_Unpack` 0x43eb00, the file read whole by 0x43e700). Header:
`LEZ1`, u32 unpacked size, u32 offset of the copy words, u32 offset of the literal bytes,
u32 unused, u32 the first flag word; further flag words from 0x18. Flags are taken from
the top bit down with a sentinel 1 (`word*2+1`): 1 = a literal byte, 0 = a u16 copy word,
distance = low 10 bits, length = high 6 bits + 3. The TGA: palette length at +5, width
+12, height +14, palette (B,G,R) at 0x12, 8-bit pixels top row first (whatever the TGA
descriptor says). The palette becomes 16-bit screen colours (shift/position globals
0x4ee9b8..0x4ee9cc set from the DirectDraw pixel format); a 0-entry palette uses a grey
ramp. Mode 4 (`this+0x14 == 4`) makes a 256x128 texture with every row written twice
(0x100 apart). One file is 24-bit (MENU/BITMAP/AUTORUN.LEZ, the CD autorun picture).
`tools/lez.py` (all 755 unpack; checked by eye: ILE01/LOAD, MENU/BITMAP/CHOOSEFO).

### 3.4 BPT binary scripts
The `.Spt` scripts the code names (`Data\Config.Spt`, `%sLevel.Spt`) are opened as
`.BPT` (`Script_Open` 0x4242a0, which swaps the extension). A BPT is the token stream of the
text script without its keywords: a string is a plain u32 length then its characters
(`Script_ReadString` 0x424e60), a number 4 bytes read as a float (0x424c00) or an int
(0x425080). Characters and numbers are enciphered by `Script_Decipher` 0x45a4a0: XOR with
the low byte of `Rand_Next(6)` 0x4b1160 (Park-Miller x16807 in Schrage form, a result
above 0x7fff taken mod 0x7fff), seeded with 0x1a2b3c4d when the file opens (0x4b1140).
Lengths are plain and do not advance the key. `tools/bpt.py` prints the tokens (guessing
string or number). Every BPT in data/ and the banks reads through to its end.
- `LIST.BPT`: 'PLUGIN', 0, 0, 8, then per track: name, folder, ON/OFF, a number
  (SPEED_BAY ile01, SNOW_RIDE ile05, ROCK_FOREST ile03, LOST_RUINS ile04, WEST_WAY ile02,
  GREAT_WALL ile06, RED_CITY ile08, DIRT_ARENA ile09).
- `ILEnn/LEVEL.BPT`: the track's files (`ile01.trk`, `Frames3d.Spt`, `Decor.Spt`,
  `DecorInv.Spt`, `Moto.net`, `Map.raw`, the bike scripts, `balise.spt`, `Map1.raw`, the
  20 texture pages `bank\bnk00_NN.raw` with their Direct3D twins) and numbers (the
  fields named in 0x427e80: StepGlobal, YScaleLevel, StartPosition, AI tuning, NbComputer,
  NbCheckPoint, TimeCheckPoint, AudioTrack ...).
- `CONFIG.BPT`: the settings in the order of 0x42ae30.. (DefName 'DSI', DrawLand,
  ScreenMode, FullScreen, Mode_169, ...).

### 3.5 The CD's installer
`DISK1/data.z` is InstallShield 3 (`tools/is3.py`): 58 files, all stored (packed size =
size). It holds `MOTO.EXE` (the CD's program, §1) and the same data as the CD's `DATA/`.
The CD's `DATA/` against GOG's `data/` (bank by bank, `build/cmpbkf.py`): GENERIC, ILE01-08,
KTMPKT, MENU*, PAGES, PODIUM, SFX, SPEED, SPEEDPKT hold the same files (the bank files
differ only in the junk after the names). GOG changes DEMO (2 demo files fewer),
DESIGN (3 pictures, + TEMP.ACT), FONTES (22 fonts, + DEBUG.RAW, SYMBOL.RAW), ILE09 (2
texture pages, 3 fewer), KTMCROSS (CARAC2/4.BPT, + 2 leftovers), MENU (EALOGO.LEZ), and
adds ILE10, ILE11 (not on the CD; not in LIST.BPT either).

### 3.5a The CD's MOTO.EXE in the port (2026-10-30)
The port takes `game.EXE`, else `MOTO.EXE`, else `MOTO.EXE` out of `DISK1/data.z` (stored in the archive,
`exe.c` `archive_read`; only the table and that file are read). Its tables, found by following GOG's strings to
the pointers that hold them, then compared record for record: pictures 0x4d9840 (209, no differences), language
folders 0x4da8a8, seeds 0x4d2bb8 (8th 0; the port never draws stream 7), sounds 0x4de830, 0x4de7d8, 0x4de7c8,
0x4de868, 0x4de8f8, 0x4de978, 0x4de6d8 (ids and names all equal), podium's sound objects 0x4e4a68, cheats 0x4e7530
(count 0x4e7528, switches 0x4d2350 for GOG's 0x4dc050), words 0x4d8340 (191, two wordings differ: 86 and 160, the
port doesn't format with either), scripts 0x4e73f0, 0x4e7450; the cursors' resources have the same numbers.
**Named colours (corrected 2026-10-31):** an earlier line here said there were none: wrong. 0x422fa0 is `Font_Get` (249 calls), and the
"colours" taken for its arguments were what lay on the stack under them. The table is at **0x4e7790**: GOG's 0x4f0dd4 without its grey,
10 entries [read from the file and from the running program]; grey is 127 as a number. The two programs colour alike nearly everywhere:
the only difference with a pixel to it is page 8's grey rows, 128 against 127, and the port has no page 8. So the CD has no look of its own in colours. `font.c` reads the table.
Which program's logic the port runs is the Game Behaviour setting, whichever files it reads (§3.5c).
"FULL D3D VERSION" is the patch's: not on this build. Registry value `Version` 1 makes the program use the
`title*GP.lez` pictures (0x4d247c) ("Moto Racer GP"); the port doesn't read it and shows the plain title.
The CD's audio tracks come from its image (§4.10.5.3a), or from files the player made from it (§4.10.5.3b).

**Running MOTO.EXE under wine** (build/origcd: MOTO.EXE, DATA, GOG's DDraw.dll, D3DImm.dll, dgVoodoo.conf; drive
e: at that folder with `.windows-label` moto_rac and `HKLM\Software\Wine\Drives` e: cdrom). Without the install's
registry it says PROGRAM INCORRECTLY INSTALLED. Under `HKLM\SOFTWARE\Wow6432Node\DelphineSoft\MotoRacer\CurrentVersion\Moto`
(the 32-bit view: reg.exe's plain path doesn't reach it): strings DataPath `e:\`, SavingPath `e:\`, Language ENGLISH,
dwords Version 0, InstallType 3, PId = the sum of 0x47bf90 over Windows' ProductId (`e = 2e ^ c`; here 140469129).
Title and main menu: the port on the CD's files differs from it in the pointer only (175 and 179 pixels, box of
the pointer; main with OMR_TILE=89,89). The name page wasn't compared (its letters turn). **Pressing Solo did nothing** was the copy check 0x466920 (track 2's length through MCI, 4:12:62): no disc under wine, so page 26.

**A whole race on MOTO.EXE under wine, since 2026-10-31.** Two things: its switch is spelled **`-noD3D`**, and it is case-sensitive: with `-NoD3D`
it stays on the Direct3D path and the 3D draws black; and the check's two jumps, 0x466941 and 0x466959, are overwritten with NOPs in the running
program's memory, for this comparison on our own copy (a memtool with the name moto.exe). No file of the game is changed, and the port has no check.
`build/origcd.sh` (start, shot, key, poke, read, stop, its own prefix `build/wineprefix_cd` and hidden display, sound into a null sink; `build/origcd_reg.sh` the registry and drive) stays git-ignored. Not capped by -FrameRateMax60 unless asked.

### 3.5b MOTO.EXE against game.EXE, function by function (2026-10-31)
Full report in `build/notes/cd_logic_diff.md` (made from the programs, so git-ignored). 3738 pairs by masked instructions: 2788 equal, 950 differ (910 the game's own, all read: 646 noise, 179 out of scope, 85 logic); 472 in one program only (7 logic). All from reading [asm, decomp], none run yet, physics figures by hand. **Physics: the player's bike only; the computer riders' numbers are the same.**
On the CD: lean by the computer's rule (0x48ad50), about half GOG's; keyboard steering limited to carac+0x28 (0x487043); dirt steering not clamped (0x4978c0); ground covered in turns by |velocity·forward|, not 1−R/|r| (0x41cad0); speed x 0.994 on a big lateral change (0x47ebc2); engine braking one gear off (0x41e510); crash bounce 0.1 not 0.01 (0x4894a0).
Seen: podium's 3D drawn first, so the panel shows, no Escape, glass to Top 5 (0x476f40, 0x476180); Choose Bike's panels over the bike (0x4730b0; **seen in a capture**); big messages not forced Direct (0x4939ee); 4 flat shadow points (0x487aa0); key messages at y 0, not 20; replay limit 10000, not 20400. Copy check and "ILLEGAL VERS" (0x417660): never in the port.
None of these is in the port: it runs GOG's logic on the CD's data until Danik says otherwise.

### 3.5c Game Behaviour: the program the port acts like (Danik, 2026-10-31)
Options, the row under Sessions Refresh Rate: GAME BEHAVIOUR : 1997 CD / GOG RELEASE. `behaviour_cd()` (config.c); `behaviour=` in
openmotoracer.ini once the player has chosen, until then what the files are (`exe_is_cd`); `OMR_BEHAVIOUR=cd|gog` for tests. Any data with
either setting; the differences' numbers are written in the port, because the other program may not be there to read them from.

**Follows the files, not the setting:** the colour table's form (font.c, the exe's own), the HUD messages' form and key colour (hud.c: GOG's
Texts picture holds only GAME OVER, so the 1997 program's sprites cannot come from it), where the music comes from (NN.wav if the
folder has them, else the CD's image), the sprites' palette test (near-black entries as holes are a mismatch of program and files, not behaviour).
**Follows the setting, screen and sound:** title's FULL D3D VERSION; music at a track's end (GOG loops, a disc stops, repeats by its timer) and
at Pause and F1 (GOG plays straight through, its winmm.dll never hears the request; a disc holds). Losing the focus holds it in both: the port's choice.
Podium: 3D first, results over it (0x47710b), no Escape, the glass on to Top 5, Alt+Return works. Choose Bike: the bike before the panels and buttons
(0x473479); the title's letters before its buttons (0x46b25e); the other pages named by §3.5b's renders hold no 3D. GameMsg fonts not forced to Direct,
set again at each race (`font_follow_behaviour`). Shadow: 4 corners on the crash's two faces, one height from the bike's section, raised 2
(0x487aa0, 0x47d550). Sky: tile size is the rectangle's corner, not +1 (0x41a956), fills from the one corner pixel (0x465a02), palette as it is. Messages at y 0
(Ctrl+S, V, A, J, K, F8), replay 10000 records, text snap < 3.0, map marker 50 in mode 3, low readout + 1, Shift+F1's VIDEO 8M line, diagnostic's video
line gone, F12 opens the column with any modifier and no Shift+F12 list, Ctrl+U with Shift, Ctrl+F not with Shift.
**Follows the setting, the player's bike** (`g_phys_cd`, set at a race's load; full record build/notes/cd_physics.md): lean by the computer's rule, steering rate limit, dirt bikes unclamped, ground
covered by the slide's angle, forward speed x 0.994 when lateral speed changes by over 0.1, engine braking a gear off, no gearbox above speed 3 under braking, crash bounce 0.1, slope pull always, pitch spring
with no floor, the pose's constants and next stunt by the draw (reading only); also found on the way: an extra turn through the cosine table (0x487135), a road bike's turn radius left at none (0x47e850), MOTO's sums in their own order.
Checked by `tools/physcheck_cd.sh` against MOTO.EXE itself: `tools/win/stepwatch.c` records as a debugger, 10 runs on 4 tracks in build/phys_cd/, one step from "before" compared with "after", 70 fields, 16547 of 16549
steps all equal (one a crash's draw before the recorder read rand's state, one the recorder's own height poke). `tools/physcheck_cd_off.py`: each difference turned back fails its steps, so the recordings do hold it. Not recorded: a natural jump (air by poking the height only), a computer rider under MOTO, two players.
Winner's lap hand-back (32) left out: at most a frame of engine note, by reading. Steps depend on the frame in MOTO (no cap): at the port's 1.6 the landing spring kicks about twice as hard as at dt 3 (a guessed 1997 PC's 20-30 fps); for Danik's judgment by feel.
**Whole pictures against MOTO.EXE** (Speed Bay, Practice, standing, the CD's files, the port's frame found by the lap clock): race 1 pixel of 640x480 at the matching frame (gull);
on-board view 0, F9 map 8, pause dialog 38 (clock and gulls), Choose Bike 0 below the bike, against 25725 for the GOG setting. Two real differences found that way: a bone's one-texel face keeps bit 5 of its palette index on the CD (0x483d40; GOG loses it,
which turned parts of the bike and exhaust red and light), the needles' black shadow shows on the CD (0x4a6230: depth 2048, moved 3 or 2 pixels; GOG's has depth 1, behind the near plane).
**Things that move are right** (all programs): a scratch build with every decor animation moved by a fraction of a step against captures of the original: Dirt Arena's helicopter 0 and 7 pixels of its box at -0.1 and -0.4 units (1 and 4 ms), Fun Fair's flag 0 at -0.25..-1. The differences in §3.5d are the moment of the capture between two 16 ms steps.
**Left out on purpose:** copy check and page 26 (never in the port); Ctrl+F1's three letters and stats line (0x4a5e50, 0x4a5cb0: DirectDraw flip counters and
capabilities, nothing they could say here); dialogs' clear to black (A9, drawn over at once); the colour helper's green limit (31): nothing traced it to anything shown. A replay is keys: it plays as what recorded it, in the same session always; demos are
computer riders, the same in both programs.

### 3.5d The second list's starts against the original: four faults (2026-10-31)
build/notes/list2_starts.md. **Road painter** 0x4a5ab0 (also 0x4a4dc0, 0x4a4f30): u, v turned round `ror 13`, added with `adc`: a slope below zero runs 1-2 lowest bits
above a plain sum (Fun Fair reversed (223,152), the texel of row 18 at v 147455). **Section planes**: +0x40 is rebuilt at load by 0x40e5c0 (the section's Z row through its
position), differs from the file's on 912 of 919 sections on Red City reversed; `land_walk` used the file's, so rules' walk, the riders' grid headings and what they aim at; a race log parts
from the old one at step 376-526. tools/cpucheck.sh builds no rules, so could not see it. **First step**: 0x411010's clock, measured from the rear wheel over 33 runs: 1 ms 15 times, 2 ms 3,
30 ms once. **Camera**: above. The port as committed gives the same 66 pictures, pixel for pixel, as the debug copy these were tested on (4705 outside the clock and countdown).

## 4. Map of the program

### 4.1 Start and the frame
- MFC application. `entry` 0x4b6420 → `_WinMain@16` 0x4c39e0 → `AfxWinMain`. The app's
  vtable 0x4d0420 (installed at 0x413aa0): `App_InitInstance` 0x414080 (Ghidra missed it:
  it starts 0x22 bytes before its first block 0x4140a2), `App_PreTranslateMessage`
  0x4159c0, `App_OnIdle` 0x415960, `App_ExitInstance` 0x415500.
- `App_OnIdle` is the frame: `Menu_Frame` 0x409c00 while `*0x4dc0c4` (the menu) is set,
  else `Race_Frame` 0x4583d0 = the race object's (0x52c478, vtable 0x4d04c8, installed
  0x413f30) slot +0x14 = `Game_Frame` 0x47d660.
- Bikes: records of 0x3209 bytes from about 0x583fe0, 24 of them (loops end at
  0x5cf4f9).

### 4.2 The clock: a variable step
`timeGetTime` everywhere. 0x4120e0 each frame: elapsed ms since the last frame
(0x459fa0/0x459fc0) → `0x4e980c` (int) and `0x4e9808` (float); above 200 → 200, below 1 → 30.
0x47df40 then scales `0x4e9808` by 0.1 (float 0x4d1970) (it compares the result with 0.0,
float 0x4d1900, not 1.0: any frame of 1–200 ms is simply ms x 0.1) and steps every bike
by it (0x45d540 the player, 0x45e940 the others). So **one step a frame, of the measured
time**. `-frameratemax N` (0x4dc200) only waits in 0x459fd0 until 1000/N ms have passed
(integer division); GOG's launcher passes `-FrameRateMax60`, so **the race runs at 62.5
frames a second, every step 16 ms, dt = 1.6** [run: 813 frames in a row, §4.18.1]. The
port's race runs those steps: one of 16 ms for every 16 ms of the clock.

**The menu's speed** (2026-10-26): the menu times its slides, 3D objects and texts by
`timeGetTime`, but some things move a fixed amount each picture: the tile scroll
(0x43f840), the button blink (0x4dbd08), the drifting texts (0x416b40) and frame-counted
blinks, the title's "FULL D3D VERSION" every 21 pictures. Started without
`-FrameRateMax60`, under wine on an unsynchronised display, the original ran its main menu
at ~95 pictures a second (the tile's scroll read from memory, 0x504880 → +0x18). The
limiter 0x459fd0 is called only by `Game_Frame`, so `-FrameRateMax60` holds the race at 60
but not the menu: there the original's counts ran at the screen's refresh (1997's 60-75 Hz
monitors). The port takes 60, the race's rate. The port's clock is the real time in a window, and those counts go
by 60ths of a second (`g_ticks`), so the port has the original's speed at any refresh
rate (Danik's 180 Hz screen ran the old port 3 times too fast). Measured: 23 pixels of
scroll in 0.386 s (23.1 expected). Under `--shot` the clock is 1000/60 a picture, one
tick each, as before.

### 4.3 The screen
- **16-bit colour.** The software renderer draws RGB565 (wine's trace: the 640x480 surface,
  pitch 1280, R 0xf800 G 0x07e0 B 0x1f); the game refuses a desktop that is not 16-bit
  (0x45a7c2: `GetDisplayMode` bits != 16 → "16 bit screen mode required !"). LEZ palettes
  and texture palettes become 16-bit colours through the shift globals 0x4ee9b8..0x4ee9cc.
  So the port's framebuffer is 16-bit, not palette numbers.
- Resolutions offered: 320*240, 512*384, 640*480 (0x443200).
- Cdd (screen3d/dd.cpp): the DirectDraw object 0x5d922c, width 0x5d93bc, height 0x5d9c20,
  bits 0x5d8944; the back surface 0x5d4320 locked by 0x476700 (address → 0x4eea14).

### 4.4 The menu (menu\menu.cpp, menuTools.cpp; mapped 2026-10-26)


Everything here was read from build/decomp and checked against build/mr.asm where the
arithmetic or the layout mattered. Confidence marks: **[asm]** = checked in the machine
code, **[decomp]** = read from Ghidra's text only, **[guess]** = an interpretation.
"now" always means `timeGetTime()` (import 0x70da04, confirmed with pefile: WINMM
timeGetTime). Other imports confirmed the same way: 0x70d99c ClientToScreen, 0x70d9a0
SetCapture, 0x70d9a4 ReleaseCapture, 0x70d9a8 DefWindowProcA, 0x70d9cc PostMessageA.

#### 4.4.1 The menu object

One object, 0xd4c bytes, `operator new(0xd4c)` in 0x472c40 (start-up), pointer kept in
**0x4dc0c4**. Constructor 0x4098e0. App_OnIdle 0x415960 calls `Menu_Frame` 0x409c00 while
`menu+0x00` is non-zero, otherwise `Race_Frame`.

| Off | Meaning | Who writes / reads |
|-----|---------|--------------------|
| +0x000 | **active** (1 = the menu runs, 0 = the race runs) | set 1 by 0x409f70 on first entry; 0 by ctor, by 0x409990, and by the loading page 0x40b3f0 when the race starts. Menu_Frame aborts with "MENU-Actived error" (menu.cpp line 0x141) if it is 0 |
| +0x004 | **mouse mode**: 1 = selection follows the pointer, 0 = keyboard selection kept | 1: page change (0x409eb0), WM_MOUSEMOVE (0x4336c8). 0: first entry (0x409f70), any WM_KEYDOWN except Shift (0x10) and Ctrl (0x11) (0x40a0f0) [asm/decomp] |
| +0x008 | mouse button bits of the previous Menu_Frame (copy of 0x5083b8) | end of Menu_Frame; 0 on page change |
| +0x00c | **last key**: Windows virtual-key code of the last WM_KEYDOWN | 0x40a0f0 stores it; consumed (0) by a hotkey match, by 0x40c1e0, by page change, by Alt+Enter; pages read it directly (name entry 0x487bb0 does its own typing from it) |
| +0x010 | **selected** (highlighted) button index, -1 none | hover (0x40c0c0), Tab/arrows, hotkeys; -1 on page change and in ctor |
| +0x014 | previous selected (to play the hover sound once) | 0x4099a0 |
| +0x018 | **pressed** button index, -1 none (the "action" the page reads) | click (0x40a460), held repeat (Menu_Frame), Return/Space, hotkey; read via 0x40c1b0, cleared by 0x40c1e0 |
| +0x01c | transition start time (ms) | Menu_Frame |
| +0x020 | transition end time = start + 0x4b0 (1200 ms) | Menu_Frame |
| +0x024 | **last activity time** (ms) for the 30 s demo timeout | page change (0x409f70), mouse move, left click (0x40a460), left button held (Menu_Frame), any key (0x40a0f0) |
| +0x028 | set 0 at transition start; no reader found in the menu routines | |
| +0x02c | transition progress, float 0..1 | 0x40c500 |
| +0x030 | redraw/flip request | set 1 by many; 0x49b1c0 flips only if set, then clears it. 0x4099a0 sets it to 1 unconditionally every frame [asm 0x4099e7], so in practice **every frame is flipped** |
| +0x034 | "force redraw" | 0x409bf0 sets +0x34 and +0x30; 0x4099a0 clears +0x34 |
| +0x03c | **current page** (index into 0x4dbb38) | 0x409f70 |
| +0x040 | **button count** (0..32) | 0x40bd40 clears; 0x40bd60/0x40bef0 add |
| +0x044 | 32 button records of 0x58 bytes (+0x044 .. +0xb44) | see §4.4.3 |
| +0xb44 | set 0 at every page start; no reader found [asm: only write 0x409cc7] | |
| +0xd48 | **transition running** | Menu_Frame sets, 0x40c500 clears after 1200 ms |

Constructor 0x4098e0 [decomp]: constructs the 32 records (0x409980 is an empty ctor),
active=0, +4=0, page=0; fills the 2048 bytes 0x5da298..0x5daa98 with `i>>3` (each value
0..255 eight times: a divide-by-8 table); allocates 0x2108 bytes into 0x504e70
(0x43e1c0); loads the language strings/pictures table (0x415a70 with the language index
0x4dbce8); +0x30=1, +0xd48=0, +0x34=0, +0x40=0, +0x10=-1.

Globals of the menu (outside the object):
- **0x5083b8** mouse buttons: bit 1 (value 2) = left button down, bit 0 (value 1) = right
  button down (§4.4.4) [asm].
- **0x519fe8 / 0x519fea** mouse x / y, signed 16-bit (all readers use `movsx WORD`) [asm].
- **0x4dbfc8** modifier bits: 1 = Alt, 2 = Shift, 4 = Ctrl (set on key down in
  0x40a0f0, cleared on key up in 0x432700; WM_SYSKEYUP clears the whole word). Alt never
  arrives as WM_KEYDOWN (it is a WM_SYSKEYDOWN, and App_PreTranslateMessage 0x4159c0
  swallows WM_SYSKEYDOWN with wParam 0x12), so bit 1 is probably never set [guess].
- **0x504e78** "the keyboard navigated on this page" (Tab/arrows set 1; page change and a
  hotkey match clear it).
- **0x504e74** "a page change is in progress": 1 from 0x409eb0 until the new page's start
  routine has run. While 1, page frames throw the 3D list away (0x49ad70) instead of
  drawing it (0x49ad90).
- **0x4dbd00** "capture taken, do the transition on the next page start" (set by
  0x40c4a0, consumed by Menu_Frame).
- **0x504e84**, **0x504e7c**: every write found stores 0 (ctor, 0x40bd40, Menu_Frame,
  0x40a0f0, 0x40a460) [asm grep], so the tests `0x504e84 != 0 → no input` never fire in
  this build. **0x504878** = ftol(now + 300.0) (double 0x4d02a0) is written at every
  press and never read [asm grep]. The port can drop all three.
- **0x504880..0x504bc4**: the 209 menu picture pointers by id (§4.7). Picture
  object fields used here: +0 x, +4 y, +8 w, +0xc h, +0x10 "slide animation running",
  +0x2c unpacked pixels, +0x30 packed file, +0x44..+0x60 the slide (§4.4.3.6).

#### 4.4.2 Pages

##### 4.4.2.1 The page table 0x4dbb38 (dumped from mr_flat.bin)

16 bytes a page: `u32 started, frame routine, start routine, end routine`
(Menu_Frame reads +4 as the per-frame routine, +8 as start, +0xc as end) [asm/decomp].
Entry 27 would start at 0x4dbce8, which is the language index: 27 pages, 0..26.

| # | addr | frame | start | end | what (strings / exits) |
|---|------|-------|-------|-----|------|
| 0 | 4dbb38 | 0 | 0 | 0 | idle (page 0 = nothing) |
| 1 | 4dbb48 | 40b3f0 | 0 | 0 | loading (`%sload.lez`); ends the menu: page 0, `menu+0 = 0` → race |
| 2 | 4dbb58 | 486a60 | 486860 | 486a40 | network (DirectPlay) |
| 3 | 4dbb68 | 442790 | 4422c0 | 442730 | **main menu**: Solo / Link / Options / Exit (see §4.4.5) |
| 4 | 4dbb78 | 40bc40 | 0 | 0 | quit: tears the menu down, PostMessage(hwnd, WM_CLOSE) |
| 5 | 4dbb88 | 447710 | 447250 | 0 | ("PRESENTS"; reached by Solo) |
| 6 | 4dbb98 | 448110 | 447a30 | 4480d0 | track choice (`map.lez`, `photo.lez`) |
| 7 | 4dbba8 | 449680 | 449100 | 44a370 | |
| 8 | 4dbbb8 | 44aee0 | 44a930 | 44aeb0 | (`smallmap.lez`) |
| 9 | 4dbbc8 | 44d540 | 44cfd0 | 44d510 | times, Reverse, Pocket |
| 10 | 4dbbd8 | 44ed30 | 44e480 | 44ecf0 | |
| 11 | 4dbbe8 | 443200 | 443000 | 0 | options (screen sizes, `List*.bpt`); main menu "Options" goes here |
| 12 | 4dbbf8 | 444f30 | 444b10 | 0 | |
| 13 | 4dbc08 | 4459d0 | 4458a0 | 0 | 3D card info |
| 14 | 4dbc18 | 446b80 | 4465f0 | 446b40 | **title** |
| 15 | 4dbc28 | 487bb0 | 487070 | 488790 | **name entry** |
| 16 | 4dbc38 | 44fdb0 | 44ffd0 | 450050 | |
| 17 | 4dbc48 | 450080 | 450290 | 450310 | |
| 18 | 4dbc58 | 450340 | 450550 | 4505d0 | |
| 19 | 4dbc68 | 488970 | 488850 | 488840 | |
| 20 | 4dbc78 | 450800 | 450600 | 0 | championship saves |
| 21 | 4dbc88 | 450f40 | 450d20 | 0 | championship saves |
| 22 | 4dbc98 | 44a4f0 | 44a390 | 44a4e0 | |
| 23 | 4dbca8 | 4514e0 | 451d10 | 451fc0 | |
| 24 | 4dbcb8 | 452f80 | 452240 | 452eb0 | podium |
| 25 | 4dbcc8 | 453c50 | 4537d0 | 453bf0 | **Delphine logo intro** (`pages\titleDsi`) |
| 26 | 4dbcd8 | 447710 | 447250 | 0 | same routines as 5 (main menu Solo when 0x442740 fails) |

The old page list called page 3 "championship": page 3 is the main menu (its buttons are SOLO_D/SOLO_OK,
LINK_D/LINK_OK, options.lez, exit.lez), not "championship"; its start routine only reads
`Data\champ%c.spt` [decomp, picture names from the table 0x4e3e20].

##### 4.4.2.2 Changing page: 0x409eb0 (menu, page) [decomp, matches asm]

`ChangePage` = 0x409eb0, called by every page that leaves (thiscall, menu in ECX):
1. 0x45a190(0x4dbf84): sets the Windows cursor back to the arrow (port: nothing).
2. 0x417750(0x53b8e8): deletes every object of the 48-slot text/sprite list (§4.4.6).
3. For every picture pointer in 0x504880..0x504bc4: if it has a packed file (+0x30) and
   unpacked pixels (+0x2c), the pixels are freed and +0x2c = 0 (pictures are re-unpacked
   on demand: memory only, invisible).
4. selected (+0x10) = -1; 0x504e78 = 0; mouse mode (+4) = 1; 0x504e74 = 1.
5. If the *current* page's started flag is set: call its end routine (if any), clear
   the flag.
6. If 0x4ee954 (Direct3D only): 0x4dc070 = 1, 0x4e9cf8 = 1.
7. 0x409f70(menu, page).

0x409f70 (menu, page) — "SetPage" [asm checked for the order]:
- **If the menu is not active** (first entry, or coming back from a race): if a frame is
  locked (0x4f1a20) end it (0x49b0e0(0,0)); save 0x4dc0d4 → 0x504e80, 0x4dc0c8 → 0x504bc4,
  0x4dc028 → 0x50487c and set 0x4dc0d4 = 0, **0x4dc0c8 = 2** (the 640x480 mode index),
  0x4dc028 = 0; viewport 0x499ba0(0,0,640,480,0,0); far plane 0x6d1f70 = 40000.0
  (0x471c4000); 0x62e444 = 1; focal lengths 0x49a0e0(1911.0), 0x49a100(1911.0); 0x625774 =
  256.0; re-lock if it was locked; **0x40a9a0 loads the menu's resources** (pictures, the
  vitre mesh 0x504c70, textures); active = 1; selected = previous = pressed = -1; mouse
  mode = 0; 0x4961d0(0); 0x404d50().
- Else, in Direct3D mode only, may set 0x4dbcec (display reset) [decomp].
- Always: +0x24 = now; started[page] = 0; **+0x3c = page**; +0xc = 0; **0x5083b8 = 0**;
  +8 = 0. (So the mouse button state is forgotten at a page change: holding the button
  across a change needs a new click.)

The new page's start routine does not run here: it runs at the next Menu_Frame.

0x409a30 (demo start, §4.4.5.2) and 0x40bc40 (page 4, quit) contain inlined copies of
0x409eb0. Quit = the same teardown, SetPage(0), then the end routine of **every** page
whose started flag is set, then `PostMessageA(hwnd 0x68b028, WM_CLOSE 0x10, 0, 0)`.

##### 4.4.2.3 Menu_Frame 0x409c00 (fastcall, ECX = menu) [asm]

1. If active == 0 → fatal error, exit.
2. 0x50bbc8 = now (the sounds' clock); 0x4178c0(now): 0x4e4ed0 = now (the animation
   clock of the text/sprite objects).
3. If 0x4e8918 != 0: 0x4587e0(0x4e8918) (network/session event flags; may change page:
   0x18, 0xe, 5) — out of scope.
4. **Page start** (if started[page] == 0): button count = 0; 0x504e84 = 0; +0x30 = 1;
   every picture's +0x10 (slide running) = 0; +0xb44 = 0; call the start routine if any,
   and then 0x504e74 = 0 (only when there is a start routine: pages 1 and 4 leave it 1);
   started = 1; **if 0x4dbd00**: start the transition (§4.4.2.4) and clear 0x4dbd00.
5. Call the page's frame routine (if any).
6. **Demo timer**: if page == 14 and the demo list has entries (0x53e5f2, a field of the
   demo list object 0x53e4d0) and (now − menu+0x24 > 30000 (unsigned) or 0x4dbf80, the
   `-d` switch): 0x409a30; if that fails 0x53e5f2 = 0 (never tried again).
7. If 0x4e9ce4 (Alt+Enter was pressed): clear it, 0x499ed0(0x4dbff0, 0) (re-set the
   display mode, windowed/fullscreen), +0xc = 0.
8. **Held left button repeat**: if (0x5083b8 & 2) and (+8 & 2) (down now *and* last frame):
   +0x24 = now; and if mouse mode and selected != -1 and that button's flags (+0x54 in
   the record, menu+0x98+i*0x58) bit 0: pressed = selected (every frame while held).
9. +8 = 0x5083b8; 0x404eb0 (sound channels update).

##### 4.4.2.4 The transition: a pane of glass shattering towards you

Taken in two halves:
- **Leaving**: a page calls 0x40c4a0 just after its last frame is drawn (and before or
  after 0x409eb0). 0x40c4a0: 0x4dbd00 = 1; 0x479260(0x4dc0c0, 2) copies the back buffer
  into texture slot 2, **640x480 → 256x192**: from every 5 pixels of a row it keeps the 1st
  and 3rd (x = 0,2,5,7,10,...), from every 5 rows the 1st and 3rd (rows 0,2,5,7,...) [asm
  loop 0x479260: 10-byte step, two rows per 5]; then 0x41b5b0(2, {0,0,256,256}, 0x4f9c00)
  hands it to the texture system. Pages that change *without* calling 0x40c4a0 get no
  transition (title 14 → 15 has none; 25 → 14 and 15 → 3 have one).
- **New page start** (Menu_Frame step 4): +0xd48 = 1; +0x1c = now; +0x20 = now + 1200;
  0x47b1d0(0x504bc8, (0,0,0)); 0x47a340(*0x504c70, (0,0,0), (0,0,1911)); +0x28 = 0;
  sound 0x2a (0x4049f0(0x2a, 0)).
- **Each frame**, the page calls 0x40c500 (after drawing its buttons, before the 3D list
  is drawn): if +0xd48: if now < +0x20: `t = (now − start) × 0.000833333` (float
  0x4d02b4 = 1/1200) → +0x2c, and 0x479c10 adds the shards to the 3D list; else +0xd48 =
  0. While +0xd48 is set no button can be pressed (0x40c1b0 returns -1, hotkeys are
  ignored).

The shards (0x504c70, a 0x94-byte object, created in 0x40a9a0 from
`data\menu\meshes\vitre.3df`, prepared by 0x479630 in menuTools.cpp):
- vitre.3df: 59 vertices, **99 triangles**, x −320.4..320.4, y −240.2..240.2, z −0.69
  (loader 0x420590 does *not* multiply by 96 [asm: no 0x4d0690]). It is the 640x480
  screen cut into triangles.
- For each triangle: centre `(cx, cy) = (sum of the 3 x, sum of the 3 y) × 1/3`, z = 0
  → obj+0x8c (12 bytes a piece); the vertices become relative to the centre (z = 0).
  Spin speeds (obj+0x90, 3 floats) from three `Rand_Next(1)` draws r1, r2, r3 (0x458cd0;
  stream 1, seeded from the program's table 0x4f2590 = 0x89696798, **0xbadebeaf**,
  0x39859835, 0x33243094, 0x1a3d5e7f, 0x45f8a6c1, 0x12345678, 0x73616c43; before these the
  mesh loader 0x420590 has drawn one number per face (99, at 0x42089f) and nothing else
  draws from stream 1 before the first transition; the port with that sequence matches the
  original's captures of 25 -> 14, the pieces each turning their own way):
  `spin[2] = (r1 − 32767) × (1/32767) × 5.340707511` (1.7π),
  `spin[1] = (r2 − 32767) × (1/32767) × 4.712388980` (1.5π),
  `spin[0] = (r3 − 32767) × (1/32767) × 5.654866776` (1.8π) [asm 0x479931..0x479985].
- Texture coordinates, per vertex, truncated to integers: `u = (x + 320) × 256 × 0.0015625`
  (= 0..256), `v = (y + 240) × 192 × (−0.00208333) + 192` (= 192..0, y up) [asm,
  floats 0x4d1850..0x4d1864]. Each triangle becomes two faces (both windings, so it is
  seen from both sides), texture slot 2.
- Drawing at progress t (0x479c10) [asm, FPU stack traced]: angles `a_k = t × spin[k]`;
  `idx_k = ftol(a_k × 2607.594587618272)` (16384/2π); the table 0x5fcb80 is a 16384-entry
  **cosine** table (built at 0x4901e0 with `fcos`), so `c_k = T[idx_k & 0x3fff]`,
  `s_k = T[(idx_k − 0x1000) & 0x3fff]`. Matrix written to 0x5daa98 (row-major as stored):
  `[c1c2, s0s1c2 + c0s2, s0s2 − c0s1c2;  −c1s2, c0c2 − s0s1s2, s0c2 + c0s1s2;  s1, −s0c1,
  c0c1]`, translation `(cx, cy, 1911 − 1850 t)` (floats 0x4d1870 = 1911, 0x4d186c =
  −1850). 0x49b2c0(matrix, the 3 vertices, 3, 0x1f) transforms, 0x49b7f0 queues the face
  (into the same z-sorted list as the page's 3D objects). With the focal length 1911 a
  piece at z = 1911 covers exactly its screen pixels; over 1.2 s the pieces come towards
  the viewer (z 1911 → 61) while tumbling. ftol = truncation towards zero.
- The matrix goes straight to 0x49b2c0 with no camera (the pieces are in the view's frame)
  and is the same as 0x48c030's rotation, here with the constant 2607.594587618272
  (0x4d1878); translation (cx, cy, 1911 - 1850 t). The faces (0x422f80): kind 16 (opaque),
  page 2, corners (0,1,2) and (0,2,1). The loader does not x96 nor mirror. [asm, px:
  captures of the original fitted to the port's frames, 3-13 % of pixels differ, all
  sub-frame timing of pieces moving fast]

#### 4.4.3 Buttons

##### 4.4.3.1 The record (0x58 bytes, record i at menu + 0x44 + i×0x58, at most 32)

Offsets relative to the record (absolute menu offsets for i = 0 in brackets) [decomp of
both creators, the drawing and the hit test, asm for the drawing]:

| Rel | (abs) | Field |
|-----|-------|-------|
| +0x00..+0x0c | (+0x44..+0x50) | text button rectangle x0, y0, x1, y1 (filled by 0x492a90 when created) |
| +0x10 | (+0x54) | **hotkey**: virtual-key code, -1 = none |
| +0x14 | (+0x58) | **kind**: 1 = text, 2 = picture |
| +0x18 | (+0x5c) | text (char*) — text buttons |
| +0x1c | (+0x60) | font object — text buttons |
| +0x20 | (+0x64) | **normal picture** — picture buttons |
| +0x24 | (+0x68) | **highlighted picture**, 0 = none |
| +0x28 | (+0x6c) | text style flags (passed to the measuring and the drawing) |
| +0x2c, +0x30 | (+0x70, +0x74) | text x, y (0 for picture buttons) |
| +0x34, +0x38 | (+0x78, +0x7c) | picture position x, y; -1 = leave the picture where it is (not written by the text creator) |
| +0x3c | (+0x80) | always -1; no reader found |
| +0x40 | (+0x84) | **neighbours** for the arrows: byte 0 = Left, byte 1 = Right, byte 2 = Down, byte 3 = Up; 0xff = none; -1 = no arrows |
| +0x44 | (+0x88) | **group**, -1 = none |
| +0x48 | (+0x8c) | pointer to an int option variable, 0 = none |
| +0x4c | (+0x90) | value: the button shows "on" when `*var == value` (radio buttons) |
| +0x50 | (+0x94) | **blink** flag (not written by the text creator) |
| +0x54 | (+0x98) | **flags**: bit 0 = repeats while the left mouse button is held |

##### 4.4.3.2 Creating buttons

- 0x40bd40 (menu) = clear the page's buttons: count = 0, 0x504e84 = 0, +0x30 = 1. Every
  start routine calls it first.
- **0x40bef0** (menu, pic, hlpic, neighbours, hotkey, group, value, varptr, blink, x, y,
  flags) — picture button (kind 2). Fatal "MAX BUTTON reached" (line 0x682) if count > 31.
  Unpacks the LEZ of both pictures now if needed. Writes rect/text/font/style/textpos 0,
  +0x3c = -1.
- **0x40bd60** (menu, int pos[2], text, font, neighbours, style, hotkey, flags) — text
  button (kind 1): 0x492a90(text, &rect, x, y, style) measures it into the rectangle;
  pictures 0, group -1, varptr 0, value 0; +0x34/+0x38/+0x50 are not written (stale).
  Used by options (11) and network (2) only.

Examples [decomp]: title (0x4465f0): start.lez (id 0x4c, *0x5049b0) placed at (0, 430)
with alignment 1 (x centred: (640 − w) >> 1), one button, no highlight, hotkey Return
(0xd). Delphine intro (0x4537d0): continue.lez (id 0x4d, *0x5049b4) placed at (−300, −300)
(off screen, never drawn visibly, never hovered), hotkey Escape (0x1b). Main menu
(0x4422c0): Solo (SOLO_D id 4 / SOLO_OK id 5, neighbours 0xff0201ff), Link (LINK_D 6 /
LINK_OK 7, 0xff03ff00), Options (id 0x4b, no highlight, 0x00ff03ff), Exit (id 0x49, no
highlight, 0x01ffff02, hotkey Escape): a 2x2 grid Solo|Link over Options|Exit; plus up to
two optional extra buttons without neighbours: Pocket (poc_D id 0x59 / poc_L id 0x58,
when 0x4dc03c or 0x4dc054) and Reverse (rev_D 0x5b / rev_L 0x5a, when 0x4dc040 or
0x4dc050) — probably unlocked by the name-entry cheats [guess].

Picture placing helpers: 0x43fa40(pic, int pt[2], align) sets pic x,y: align 0 = as
given, 1 = x = (640 − w) >> 1 (y as given), 2 = centred on the point (x − w>>1, y − h>>1);
it also writes the adjusted point back. 0x43faa0(pic) returns pic itself (= a pointer to
its x,y).

##### 4.4.3.3 Selecting with the mouse (every frame)

0x49a160(0,0) (frame begin, called by every page) ends with 0x4099a0(menu) when the menu
is active [asm]:
- +0x30 = 1 (always); +0x34 = 0.
- `sel = 0x40c0c0(menu)`; +0x10 = sel; if sel != +0x14: +0x14 = sel, and if sel != -1
  play sound **0x29** (the hover tick).

0x40c0c0 [decomp, coordinates asm]: in keyboard mode (+4 == 0) returns +0x10 unchanged.
In mouse mode: for i = 0.. count−1, the first button under the pointer wins:
- picture button: first moves its normal picture to (+0x34, +0x38) if both != −1; hit if
  `pic.x <= mx <= pic.x + pic.w` and `pic.y <= my <= pic.y + pic.h` (inclusive both ends);
- other kinds: hit if `x0 <= mx <= x1` and `y0 <= my <= y1` (the text rectangle).
No hit → selected = −1. So in mouse mode the highlight disappears when the pointer is
off every button.

##### 4.4.3.4 Pressing

Pressing = writing the index into **pressed (+0x18)**. Ways [decomp; asm where marked]:
- **Left button down** (WM_LBUTTONDOWN → 0x40a460, called from the window procedure only
  when the menu is active): +0x24 = now; if mouse mode and selected != −1: pressed =
  selected, sound **3**. The press happens on the button going *down*. [asm 0x4336fa]
- **Held**: Menu_Frame step 8, only for buttons with flags bit 0, every frame from the
  second frame of holding.
- **Keys** (0x40a0f0, §4.4.4.2): a button's hotkey; Return or Space on the selected button.

Pages read the press with **0x40c1b0** (menu): returns −1 while the transition runs
(+0xd48) or if 0x504e84, else +0x18. After acting they call **0x40c1e0** (menu): last key
(+0xc) = 0 and pressed = −1. A press stays pending until the page clears it.

##### 4.4.3.5 Drawing: 0x40c1f0 (menu), once a frame by every page [asm for the state table]

1. For every picture button: 0x43fab0(normal picture, 0,0,0,0) = step its slide (§4.4.3.6).
2. `g` = group of the selected button (−1 if none selected).
3. Only if +0x30 (always, see §4.4.1): toggle the blink phase 0x4dbd08 (^= 1, so blinking
   buttons blink at half the frame rate), then for each button:
   - **text** (kind 1): selected → font colours (255,255,0) (yellow, two calls 0x490a60 and
     0x490a30), else (255,255,255) white; font vtable slot 1 (1, 0xff); 0x493ee0(font,
     text, x, y, style, 0,0,0,0) draws it (the text system is described elsewhere).
   - **picture** (kind 2): move the normal picture to (+0x34,+0x38) if both != −1. State
     bits: 4 if g != −1 and this button's group == g (includes the selected one itself);
     2 if this is the selected one; 1 if varptr and *varptr == value. If the blink flag is
     set and the blink phase is 1: state = 0 (`and ecx,0xfffffff0`). Then (jump table
     0x40c474, mode numbers of 0x43ede0, §4.7):

| state | drawn |
|-------|-------|
| 0 | normal picture: mode 5 (its own) if the button has a highlight picture, else mode **3** (every other pixel, colour 0 transparent = dimmed) |
| 1, 6, 7 | highlight picture moved to the normal picture's x,y, mode 5 |
| 2 | highlight picture at the normal's x,y, mode 5; without a highlight picture: normal, mode 5 |
| 3 | **nothing** (selected and on, no group: never drawn — literal reading of the table) |
| 4 | normal picture, mode **2** (every other pixel, colour 0 drawn) |
| 5 | highlight picture at the normal's x,y, mode 2 |

All drawn on the menu surface 0x4dc0c0 with pct_h = 100, pct_w = 0 (whole picture).
So with highlight pictures (Solo/Link) the plain one is shown until hovered; without
(Options/Exit/Start) the button is shown half-dissolved and becomes solid when selected.

##### 4.4.3.6 Picture slides (buttons sliding in)

0x43f9a0(pic, int target[2], float seconds, kind, align): start = now (+0x54), duration
= ftol(seconds × 1000.0) (+0x58), end (+0x5c); target adjusted by align like 0x43fa40;
delta (+0x4c, +0x50) = target − current; from (+0x44, +0x48) = current; +0x10 = 1 (running);
+0x60 = kind. 0x43fab0 each frame (for mode-4 tile pictures it scrolls the tile instead):
`n = min(now, end)`, `f = (n − start) / duration` (float); kind 0:
`x = ftol(from + delta × f)`; kind 1: `x = ftol(from + delta × (sin(f ×
π − π/2) + 1) × 0.5)` (ease in-out); same for y; when n == end, +0x10 = 0 [asm 0x43fb04..].
The main menu starts slides for its pictures with 1.0 s, kind 1 (0x4422c0).

#### 4.4.4 Input from Windows

Window procedure at 0x4329a5 (the message switch from 0x432ecc; jump tables 0x433880 for
0x101..0x105 and 0x433894 for 0x200..0x205) [asm]:

##### 4.4.4.1 Mouse
- **WM_MOUSEMOVE** (0x433637): 0x519fe8 (dword) = lParam (x low word, y high word);
  ClientToScreen into 0x51be94/0x51be96 (screen x,y; previous kept in 0x509220); deltas
  0x519ff4/0x519ff6; reset of the cursor timers 0x4dbfb4 = 3000, 0x4dbfb0 = 0. If the menu
  is active: mouse mode (+4) = 1, +0x24 = now. If fullscreen (0x4dbff0): 0x519fe8 =
  0x51be94 (screen coordinates = game pixels).
- **WM_LBUTTONDOWN** (0x4336fa): 0x516aa4 = old bits; 0x5083b8 |= 2; 0x53eb18 = 1,
  0x53ead8 = 0; SetCapture; if the menu is active: 0x40a460 (press, §4.4.3.4).
- **WM_LBUTTONUP** (0x43375a): 0x53eb18 = 0; 0x5083b8 &= ~2; ReleaseCapture.
- **WM_RBUTTONDOWN** (0x43377e): 0x5083b8 |= 1; 0x53ead8 = 1; SetCapture.
  **WM_RBUTTONUP** (0x4337bd): 0x5083b8 &= ~1; ReleaseCapture. The menu core ignores the
  right button.

##### 4.4.4.2 Keyboard
- **WM_KEYDOWN** (0x43351f) → 0x431250(hwnd, wParam, lParam): if the menu is active it
  only calls **0x40a0f0(menu, vk, 0)** and returns [asm 0x431250..0x431275]; otherwise the
  race's key table. Windows' auto-repeat makes held keys repeat.
- **WM_KEYUP** (0x433535) → 0x432700: menu active → clear Shift/Ctrl/Alt bits of 0x4dbfc8.
- **WM_CHAR** (0x433547): 0x51be90 = the character, 0x50bbb0 = scan code (lParam >> 16).
- **WM_SYSKEYDOWN** (0x43355f): only Return, and only if 0x4e8934 == 1: toggles 0x4dbff0
  (fullscreen ↔ window) and sets 0x4e9ce4 (Menu_Frame step 7 re-sets the display).
  **WM_SYSKEYUP**: 0x4dbfc8 = 0.

0x40a0f0 (menu, vk) [decomp, branch order matches the asm]:
1. +0x24 = now; +0xc = vk; unless vk is Shift (0x10) or Ctrl (0x11): mouse mode = 0.
2. (0x504e84 test: never true.)
3. **Hotkeys**, only if no transition and nothing pressed yet (+0x18 == −1): for every
   button whose hotkey == vk: if 0x504e78 (arrows/Tab used) and vk == Return and Alt not
   held → stop looking (Return then acts on the selection, step 4); else selected =
   pressed = i, +0xc = 0, 0x504e78 = 0. The loop does not stop at a match (the last
   matching button wins). No sound.
4. Then on +0xc (0 after a hotkey match, so nothing more happens):
   - **Tab (9)**: Shift held → selected − 1 (below 0 → count − 1), else selected + 1
     (== count → 0); 0x504e78 = 1.
   - **Return (0xd) / Space (0x20)**: if no transition, Alt not held and selected != −1:
     pressed = selected (no sound here).
   - **Shift (0x10)** / **Ctrl (0x11)** / **Alt (0x12)**: set bit 2 / 4 / 1 of 0x4dbfc8.
   - **Left (0x25) / Right (0x27) / Down (0x28) / Up (0x26)**: if selected == −1: selected
     = (count == 0) (i.e. 0 when there are buttons); else n = byte 0 / 1 / 2 / 3 of the
     selected button's neighbours; n == 0xff → unchanged, else selected = n. 0x504e78 = 1.
   - **Insert (0x2d)**: if 0x4e8928 (a debug switch) 0x471110(0x5d2dd8,1,0): screenshot
     to TGA files.
   - **Escape** has no code of its own: pages give a button the hotkey 0x1b.

#### 4.4.5 Start-up and the first pages

##### 4.4.5.1 How the menu starts [decomp]
App_InitInstance 0x414080 → 0x473520 → 0x472c40: after the screen and sound set-up it
creates the menu object (new 0xd4c, ctor 0x4098e0) into 0x4dc0c4, 0x4311c0, 0x4e9834 = 0,
loads the demo list `Data\Demo\EA.dem` (`Data\Demo\BMG.dem` for the GP edition 0x4dc1c4)
into 0x53e4d0 (0x42e370; 0x42e6d0 if it has entries), then **ChangePage(menu, 25)**. As the
menu is not active yet, 0x409f70 does the first-entry set-up (§4.4.2.2). App_OnIdle then runs
Menu_Frame every idle loop. (0x415b20, also called from InitInstance, builds a temporary
menu object only to load the language tables, then deletes it.)

##### 4.4.5.2 The chain 25 → 14 → 15 → 3 (every exit found by scanning all 0x409eb0 calls)

- **25, Delphine logo** (0x453c50): six 3D pieces (`titleDsi` camera, D, S, I, DsInt)
  animate; CD track 13 (0x404990(0xd, 0)) starts in its start routine. Each frame
  `dt = now − last`, if dt > 500 → 100 (float 0x4d0bf4 = 500.0); each piece's progress
  += dt / (its duration × 1000.0). Ends when **all 6 have arrived** (0x53f788 == 6) or its
  only button is pressed: the invisible "continue" button, hotkey **Escape** (also
  Tab/arrow then Return/Space; it cannot be clicked). → `0x409eb0(14)`, then after the
  frame is finished `0x40c4a0` (so 25 → 14 **has the glass transition**).
- **14, title** (0x446b80): the three logo pieces fly in, then title4 with blinking
  "FULL D3D VERSION" (§4.7); CD track 3 (0x404990(3,2)) and sound 0x2d at its start.
  One button "Start" (Return hotkey, clickable). Pressed → `0x40c1e0`, `0x409eb0(15)`
  with **no transition** (no 0x40c4a0). Idle 30 s (no mouse move, click or key; the
  clock restarts at each page change) with demos available, or `-d`: **demo** (0x409a30):
  opens the demo list, takes the next entry (0x42e580 → track/time/picture into 0x508430,
  name into 0x542891), sets up the race (0x457bf0), 0x4dbffc = 1, and does ChangePage to
  **page 1** (loading) inline. On failure: list closed, 0x4e8944 = 1, and with `-d` the
  box "Error in demo script". Escape does nothing on the title.
- **15, name entry** (0x487bb0): a 3D grid of 28 cells, 7 a row (26 letters, 26 = delete,
  27 = OK; cursor 0x5f647c — an int that Ghidra shows as a float: 3.64338e-44 = 26,
  3.78351e-44 = 27); name in 0x5f6518, length 0x5f1a44 (max 8). It reads menu+0xc itself
  (not buttons): first PageUp/PageDown/End/Home (0x21–0x24), numpad and F-keys
  (0x60–0x6b, 0x6f–0x7b) are discarded; **Backspace (8)** deletes the last letter;
  **Return** (no Alt) confirms if the name is not empty; **Space** = act on the cell under
  the cursor (letter, delete, OK); **arrows** move the cursor (Left/Right ±1 wrapping
  0..27, Up/Down ±7 wrapping by ±21); **A–Z** (0x41–0x5a, also 0x61–0x7a) type the letter.
  On confirm the name goes to 0x5161b0 and is compared (case-insensitive) with the cheat
  table 0x4f1250 (0x4f1248 entries of {name, int *var, value}, first "CESREVER"): a cheat
  sets its variable and the page restarts (`0x409eb0(15)`, no transition); a real name
  → `0x40c4a0` then `0x409eb0(3)` (with transition). Sound 3 on each accepted key.
- **3, main menu** (0x442790): Solo (button 0): voice 0x55 (once, 0x53f554 = 1), then
  nothing happens until now > 0x53f78c = ftol(now + 1000.0) (double 0x4d0b68) [asm]
  (the press stays pending meanwhile); then 0x4dbffc = 1, glass capture, a CD-audio check loop (0x4723c0 asks for
  the CD, may drop out of fullscreen), then page **5** if 0x442740 succeeds, else **26**.
  Link (1): voice 0x56, the same 1000 ms wait, then 0x4dbffc = 0, transition, page **2**. Options (2):
  transition, page **11**. Exit (3, also **Escape**): page **4** (quit, no transition).
  Arrows move in the 2x2 grid; Tab cycles. The two optional buttons toggle 0x4dc078
  (Pocket) / 0x4dc068 (Reverse) in place (0x40c1e0 + 0x409bf0, no page change).

##### 4.4.5.3 The main menu's layout (0x4422c0, 0x442790) [asm for the press handling]
- Start: title bar (id 3) at (0,-80) slides to (0,0); Solo (4/5) from (-140,116) and Link
  (6/7) from (780,116) slide to (56,177) and (363,177) — y 116 when Pocket or Reverse is
  unlocked; all 1.0 s eased. Options (0x4b) at (240,500), Exit (0x49) at (580,500); the
  frame after Solo's slide has ended they slide to (240,420) and (510,430) in 0.7 s
  (= 699 ms: the float 0.7 times the double 1000.0, truncated). Pocket button (0x59/0x58)
  at (227,288) or (352,288) with Reverse; Reverse (0x5b/0x5a) at (202,293) or (72,293)
  with Pocket. Their switch labels (0x5c+0x5f off, 0x5d+0x5e on) each frame at
  (248|373, 362) + (328|453, 362) for Pocket and (250|120, 363) + (330|200, 363) for
  Reverse.
- Frame: hover; Options/Exit rise; tile (id 0) scrolled (1,1) a frame and drawn; title
  bar; buttons; switches; glass; pointer. Presses (esi = 0x40c1b0): if the voice's wait
  0x53f78c is set (not -1) and now <= it, nothing more. Else 0 Solo → done, 0x4dbffc = 1,
  then (after the toggles' tests) glass capture, the CD check loop, 0x442740 (the copy
  protection: the CD's track 2 must be 0x3e0c04 long) → page 5, else 26; if the CD check
  loop gives up, no page change and the capture is dropped. 1 Link → 0x4dbffc = 0, glass,
  page 2. 2 Options → glass, page 11. 3 Exit → page 4. Then pressed == 0x53f848 (Pocket's
  index, -2 none) toggles 0x4dc078, == 0x53f4e8 toggles 0x4dc068. The end routine
  0x442730 is an empty `ret`.
- The tile picture's step (0x43f840) adds (dx, dy) to its scroll (& 0x7f) and moves a
  cosine "wobble" (+0x20/+0x24 from counters +0x64..+0x70) that only the MMX copy path
  reads (0x43f292); the plain path (0x4dbcf4 = 0) ignores it. The scroll is per frame: its
  speed follows the frame rate.
- Checked: the port's settled main menu (frame 120) against the original's
  (build/ref/orig_mainmenu.png): 99.94 % of pixels equal, the rest the two mouse pointers.

#### 4.4.6 The common calls of a page frame (what each is, briefly)

The usual order of a page's frame routine (title 0x446b80, intro 0x453c50, main menu
0x442790 all follow it) [decomp]:

1. **0x49b1c0** — present the previous frame: if the menu is active and +0x30 is 0 do
   nothing, else clear +0x30 and flip (0x419900 → 0x474860); then display-mode
   housekeeping (0x4dbcec → 0x499ed0; 0x4e8940 → 0x4f1824). Since +0x30 is always 1,
   every frame is presented here, one frame late (the port presents at the end of the
   frame: same picture sequence).
2. **0x49a160(0,0)** — begin the frame ("BeginDisplay", screen3d.cpp): lock the back
   surface (0x476700; fatal "BeginDisplay failed"), 0x624f5c = pixels + (pitch × 0x6d1040 +
   0x624374) × 2 (the viewport origin), 0x4f1a20 = 1 (locked); reset the 3D renderer's per-
   row span lists (0x623bd8, 0x68b008 rows); then **0x4099a0** (menu hover, §4.4.3.3).
3. The page draws its pictures (0x43ede0) and queues 3D objects (0x48c030 etc.).
4. **0x40c1f0** buttons, **0x40c500** transition shards (the intro calls 0x40c500 after
   the sprites; order between them does not matter for 2D since the shards go to the 3D
   list).
5. **0x49ad90** — draw the queued 3D polygons (z-sorted, 8000 buckets at 0x65da34 down,
   each emptied); only when 0x504e74 == 0. **0x49ad70** just empties the 8000 buckets
   (used instead when 0x504e74 == 1). In Direct3D mode (0x4dc06c && !0x4dc070) 0x49ad90 is
   called after the unlock instead.
6. Text/sprite list **0x53b8e8** (48 object pointers with a vtable: slot 0 destroy, +8
   update, +0xc "still alive", +0x10 draw): **0x418790** = time-outs (objects whose
   timer +0x8c has expired change state 3→1 or 4→2) and removal of dead ones; **0x417840**
   = 1 when the list is empty/all dead; **0x418860** = update + draw every live object
   (called only if 0x417840 says something is alive). **0x417750** = delete them all
   (page change). The text system itself is documented elsewhere.
7. **Mouse pointer**: if fullscreen (0x4dbff0) and DirectDraw is up (0x53c524): picture
   **0x5049d4** (id 0x55 `Pointeur.lez`, language 0) placed at (mx, my) = (0x519fe8,
   0x519fea) with 0x43fa40 align 0 (its top-left corner on the hot spot), drawn mode 5.
   In a window the Windows cursor is used instead. 0x4dbff0 is forced to 1 whenever
   DirectDraw was created (0x42b0cb), so normally the game draws its own pointer.
8. **0x49b0e0(0,0)** — end the frame: if 0x625748 (a clear is pending) fill each row's
   remaining span list with the colour 0x65f0d8; unlock (0x4767b0); 0x624f5c = 0,
   0x4f1a20 = 0 [decomp; what the spans mean is the renderer's business].
9. Read the press (0x40c1b0), act, 0x40c1e0, maybe 0x40c4a0 + 0x409eb0.

Menu_Frame itself then does the demo timer, Alt+Enter, the held-button repeat and
**0x404eb0** = update the sound channels (0x404b30 on the 8 records of 100 bytes at
0x504240..0x504560 and on 0x5028c8: queued sounds started when their delay has passed).

Sounds used by the core: **0x4049f0(id, flags)** plays sound effect `id`: ids < 0x2f or
> 0x5c on the effects channel (if effects are on, 0x4db664), 0x2f..0x5c (voices) on the
second channel (0x4db668); flag 4 = don't restart if it is the last one played. Menu ids:
0x29 hover, 3 click/accept, 0x2a transition start, 0x2d title start, 0x55/0x56 the "Solo"/
"Link" voices. **0x404990(track, x)** plays a CD audio track through MCI (0x503528).

#### 4.4.7 Notes for the port

- Fixed 60 Hz steps: everything here runs on real milliseconds (now), so the port can
  keep a millisecond clock advanced by 16.667 per step and feed the same formulas
  (1200 ms transition, 30000 ms demo timeout, slide durations, per-frame dt capped 500 →
  100 in the intro/title).
- The selection model to copy: mouse mode follows the pointer each frame (−1 off the
  buttons); any key (other than Shift/Ctrl) switches to keyboard mode where the selection
  stays put; a mouse move switches back.
- Press on left button *down*, Return/Space on the selection, or a hotkey; read once by
  the page, then cleared. No presses during the 1.2 s transition.
- Open questions: the row/column convention of 0x49b2c0 for the shard matrix; Rand_Next's
  range (assumed 0..65534 from the (r − 32767)/32767 scaling); what 0x4d0bf4/0x540ae0
  durations of the intro are (page 25 is described only as far as its exit).

### 4.5 3D models: 3DF + MAP
Loader 0x4216a0 (and 0x420a50, its twin for the race). **3DF**: u16 version (1), u16
vertex count, vertices as 3 floats each, multiplied by 96.0 (float 0x4d0690; Ghidra shows
an int-to-float conversion, the machine code is `fld dword` — a decompiler trap), then
faces: u16 point count n, u16 attribute, n u16 vertex numbers; a count of 0 ends the list.
In memory a face is 0x40 bytes. **MAP**: 0x200-byte header (byte 1 bit 7 must be set:
"version trop ancienne" otherwise), then 0x80 bytes a face in the 3DF's order: u16 kind
(<3 untextured; 3 triangle, 4 quad: must equal the face's point count), texture (u,v) as
ints at (+0x22,+0x32) (+0x26,+0x36) (+0x2a,+0x3a) (+0x2e,+0x3e), flags u32 at +0x48 (low
16 bits: the texture page; 0x10000, 0x20000, 0x40000 ... 0x400000 set the face's drawing
kind), u16 colour at +0x4e (RGB565). After the faces an `ANI!` block (animation).

### 4.6 Ghidra and MSVC
Routines with C++ exception handling set up their frame, end the prologue with a `nop`,
and Ghidra starts a second function after it: 81 such pairs, found by `tools/splits.py`
and joined by `JoinFunc.java` (now a step of `rebuild.sh`). Catch blocks stay separate
small functions (they call 0x4c4f71). No RTTI for the game's own classes:
`tools/vtables.py` lists the virtual tables and who installs them.

### 4.7 Menu pictures
- **The table** at 0x4e3e20: 209 records of 20 bytes, `u32 ?, u32 language, u32 id,
  char *name, u32 mode`, ended by id -1. 0x415ad2 copies it out by id into 0x53a8d8 (16
  bytes an id: ?, language, name, mode). The loaded pictures are pointers by id at
  0x504880 (0x504b70 = id 0xbc).
- **Loading** (0x43e8e0): language 0 → `data\menu\bitmap\<name>`; language 1 → the menu
  language's folder from 0x4e8b60 (`data\MenuUs\`, `MenuFr`, `MenuGr`, `MenuIt`, `MenuSp`,
  `MenuSw`; index 0x4dbce8, set by `-us`/`-fr`/...), and if that file is missing,
  `data\MenuUs\`. A `.lez` is kept packed and unpacked at its first drawing; anything else
  is a 24-bit TGA (0x43e280, rows flipped). All 209 names are `.lez`.
- **The object** (0x40 bytes): +0 x, +4 y, +8 w, +0xc h, +0x14 mode, +0x18/+0x1c the tile's
  scroll, +0x2c the 16-bit pixels, +0x30 the packed file.
- **Drawing** (0x43ede0, `this, ?, mode, pct_h, pct_w`): mode 5 = the picture's own.
  Only the bottom `pct_h`% of its rows are drawn, at y + the rows skipped; `pct_w`% of its
  columns are cut off on the right; clipped to the screen (0x655d10 x 0x624b18, pitch
  0x62ebec, surface 0x624f5c). Modes: 0 copied, 1 colour 0 left out, 2 every other pixel
  (the next row starting one pixel later, then one earlier), 3 the same with colour 0 left
  out, 4 (decided by the picture's own mode, whatever is asked) the 128x128 tile filling
  640x480 from (sx & 0x7f, sy), 0x43f0f0. Its second path (0x4dbcf4 set, aligned 8-byte
  copies) is not read yet; it looks like the same picture copied faster.
- **The title page** (14, 0x446b80) draws `title.lez` (0xbc; `titleGP.lez` 0xc0 for the GP
  edition, 0x4dc1c4) while its three 3D logo pieces fly in (`titleMr\moto`, `racer`,
  `ellipse`, each a textured quad, drawn by 0x48c030), then `title4.lez` (0xbf), which has
  the logo in it, once all three have arrived (0x5409bc = 3). "FULL D3D VERSION" blinks
  above (0x4e90e0 counts frames: drawn from 11 to 20).
- **Checked**: the port's `title4.lez` against the original's settled title page (build/ref/
  orig_p14_title.png): every pixel the same in 5-6-5 terms outside the "Start" button
  (261..378 x 431..469), not drawn yet. All 209 pictures load through the port.

### 4.8 Text: fonts, the text objects, the words (mapped 2026-10-26)


Scope: bitmap fonts (FNT/RAW), FONTS.BPT, the string drawing into the 16-bit screen, the
moving "text objects" list (0x417d20 and friends), where the words come from, the 3D
letters of the name page. Addresses are virtual addresses in game.EXE. Confidence:
**high** = read in the asm or checked by running the tool on the data; **medium** = read
in Ghidra's text, consistent, not checked against the running game; **low** = inferred.

Tool: `tools/fnt.py` (list / text / sheets / rawcheck). Pictures: `build/png/fonts/`
(`sheets/NN_Name.png` every font's characters in its script colours; `sample_*.png`).
Strings: `build/notes/menu_strings.txt` (the whole language table).


#### 4.8.1 The FNT font files

##### 4.8.1.1 Where they are
`FONTES.BKF`: `LOWRES\*.FNT` (11), `HIRES\*.FNT` (21), `DELPHINE\DEBUG.FNT` + `DEBUG.RAW`,
`SYMBOL.FNT` + `SYMBOL.RAW`. The script names `.Raw` files; only DEBUG.RAW and SYMBOL.RAW
are in the bank (GOG), every other font exists only as .FNT. (high)

##### 4.8.1.2 Loading chain (high, asm + decomp)
- `0x472c40` (game start, after Config/List scripts): `0x426350` opens `Data\Fonts.Spt`
  (Script_Open swaps to .BPT), `0x426460(0, &0x4eaaec, 1)` reads only FontNumber (throws
  to stop), `0x426460(0, &0x4f180c, 0)` reads the whole script, creates the font objects,
  and (FontData section, 0x427c04) loads each font with `0x492170`.
- Then for every font: slot 1 `(1, 0xff)` (build the colour table) and `0x490960`
  (Font_Unload: frees glyph blocks, glyph pointers, palette; +0x5248 = 0). Fonts 10 and 11
  (Sprites) get +0x5480 = 1.
- Later `0x4961d0` (Fonts_Refresh; called from 0x409f70 menu start, 0x4592a0, 0x47d660
  Game_Frame, 0x43cf90 help, 0x48ae40/0x48b070 generic, ...) loads the fonts whose
  condition predicate (+0x5478) says yes and unloads the others. (medium: predicates not
  fully read)
- `0x492170` Font_Load: for each of up to 4 files (+0x524c count): extension of the name
  "raw" -> vtable slot 3 (+0xc) = `0x491380`; "iff" -> slot 5 (+0x14) = `0x492130` (empty
  stub). Then computes glyph pointers: for c in each file's range, glyph[c] (+0x18+c*4) =
  block[file] + height * (sum of widths of the chars before c in that file). Also a
  texture atlas layout for Direct3D (+0xa40 x, +0xe40 y, +0x1248 quads) - not needed.
  Pal/Moto fonts: +0x54a2 = largest pixel value. Ends with slot 1 (1, 0xff) and +0x5248 = 1.
- `0x491380` (slot 3): `0x490c90` makes "<name without extension>.FNT" (into 0x508430),
  `0x490d80` reads it; if that fails, reads the .RAW and scans it (1.4).

##### 4.8.1.3 FNT layout (high: 0x490d80 read in the asm; all 34 files parse to their exact
size, see `tools/fnt.py`)
| offset | size | meaning |
|---|---|---|
| 0 | u32 | first character code (ignored: the script's value is used) |
| 4 | u32 | character count (ignored: the script's last-first+1 is used; must agree) |
| 8 | u32 | height in pixels (stored at font+0xa18; the last file loaded wins) |
| 0xc | u32 | palette entries, 0 or 256 (checked 0..256) |
| 0x10 | 3*n | palette, bytes R,G,B (kept at +0x549a/+0x549e only by Pal/Moto fonts (+0x547c), only from the first file that has one) |
| .. | 4*count | widths, u32, one per character (font+0x618+c*4) |
| .. | sum(w)*h | glyphs one after the other, each w*h bytes, rows top first |
Checks in the loader: count > 0, first+count <= 256, palette entries 0..256.
A width of 0 means "no glyph": not drawn and no advance.

Pixel bytes mean different things per font type (2.2): 0 is always see-through.
Observed: Direct fonts use 1..31 (and 128..159 / 128 = colour index 0 = back colour:
CHRONO, DEBUG's opaque box); AntiAlias8Transp fonts use low 5 bits = colour, high 3 bits
= transparency (MENULINK 31,63,..,223; MSG up to 255; MENULAB 1 and 32..160, i.e. the
back colour, translucent edges); Pal fonts use 1..15 into their own palette.

##### 4.8.1.4 The RAW fallback (high: tools/fnt.py rawcheck reproduces DEBUG.FNT and SYMBOL.FNT
byte for byte)
RAW = mhwanh picture (32-byte BE header: magic, u16 version, u16 width @8, u16 height
@10, u16 palette entries @12; palette; pixels). The loader checks
`w*h + 3*pal + 32 == file size`. Each glyph sits in a box drawn in the **frame colour**
(FontData's 4th number). Scan (`0x491210`): from (x, y) search the row rightwards for a
frame pixel; none -> next row, except that the first miss after a glyph row jumps
`height + 2` rows (y of the top line + height + 2); found at (x0, y0): the run of frame
pixels to the right is w+2 long, the run downward from (x0, y0) is h+2 long; the glyph
is the w x h block at (x0+1, y0+1); the scan continues on the same row after the run.
The first glyph fixes the height ("The frame color is incorrect or the first found char
is too small" if < 2); every other glyph must have the same height ("Reading char at
%dx%d"). Glyphs are taken in character order first..last.

##### 4.8.1.5 The font object (C++ classes, vtables) (high for offsets used above)
Size 0x54a6 (AntiAlias8/8Transp: 0x70a6, the extra 0x1c00 = seven 256-entry int tables
for AntiAlias8's blend). Base init `0x490840`.
| off | meaning |
|---|---|
| +0 | vtable |
| +4 | type: 1 Direct (also DirectTrame), 3 CustomColor (also ..Trame), 5 AntiAlias8, 6 AntiAlias8Transp, 7 Pal, 8 Moto |
| +8 | name (script's first string, strncpy 15) |
| +0x18 | glyph pointer [256] |
| +0x418 | colour table u16 [256] (RGB565; index 0 unused) |
| +0x618 | width [256] |
| +0xa18 | height |
| +0xa1c | line spacing (extra pixels between lines), default 1 |
| +0xa20 | letter spacing (extra pixels after each glyph, may be negative), default 1 |
| +0xa38/+0xa3c/+0x1244/+0x1248.. | Direct3D texture and per-glyph quads (ignore) |
| +0x5248 | loaded flag |
| +0x524c | number of files (max 4); +0x5250 names [4][0x81]; +0x5454 first char [4] (bytes); +0x5458 counts [4]; +0x5468 frame colours [4] |
| +0x5478 | condition predicate (function pointer, 0 = none) |
| +0x547c | uses own palette (Pal, Moto) |
| +0x5484 | glyph blocks [4] |
| +0x5494 | fore colour R,G,B (default 255,50,20) |
| +0x5497 | back colour R,G,B (default 100,100,10) |
| +0x549a/+0x549e | palette pointer / entries |
| +0x54a2 | largest pixel value (Pal/Moto) |

Vtables (slot 0 SetColors(fore*, back*), 1 BuildColours(from, to), 2 ?, 3 LoadRaw, 4 ?,
5 LoadIff stub, 6 DrawChar):
| type (script name) | ctor | vtable | slot 0 | slot 1 | slot 6 DrawChar |
|---|---|---|---|---|---|
| Direct | 0x490700 | 0x4d1e68 | 0x490b00 | 0x490b90 | 0x493100 |
| DirectTrame | 0x490700 | 0x4d0710 | 0x490b00 | 0x490b90 | 0x492d20 |
| CustomColor | 0x494020 | 0x4d1e88 | 0x494160 (nothing) | 0x494150 (nothing) | 0x494170 |
| CustomColorTrame | 0x494020 | 0x4d06f0 | nothing | nothing | 0x494590 |
| AntiAlias8 | 0x4949f0 | 0x4d1ea8 | 0x490b00 | 0x494b90 (= 0x490b90) | 0x494bb0 |
| AntiAlias8Transp | 0x495110 | 0x4d1ec8 | 0x490b00 | 0x494b90 | 0x495280 |
| Pal | 0x495700 | 0x4d1ee8 | 0x490b00 | 0x495bd0 | 0x495840 |
| Moto | 0x495c30 | 0x4d1f08 | 0x490b00 | 0x4960e0 | 0x495d50 |
FONTS.BPT only uses Direct, AntiAlias8Transp, Pal and CustomColor (font 18 Symbol,
"Obsolete", no files). So a port needs Direct, AntiAlias8Transp and Pal.
Font array: `*(Font***)0x4f1810`, count `0x4f180c` (35).


#### 4.8.2 FONTS.BPT

##### 4.8.2.1 What it holds (high: tools/fnt.py reads it token by token to its exact end)
Reader `0x426460` (+ its pieces 0x42664e..0x427d90). Order of the tokens (the BPT is the
replay of the reads, so FontNumber appears twice because the game reads it twice):
1. FontNumber = 35 (twice). Must be 0..48. Allocates the pointer array.
2. FontTypes: `index, name, type, condition` until index -1. Type -> class (1.5);
   names "GameMsgLowRes"/"GameMsgHiRes" are forced to Direct whatever the script says
   (0x426911). Condition -> +0x5478: OutGame 0x45a8e0, InGameLowRes 0x45a930,
   InGameHiRes 0x45a970, InGameMenu 0x45a9c0, InGameMenuLowRes 0x45aa00,
   InGameMenuHiRes 0x45aa70, InGameGeneric 0x45aaf0, InGameHelpMenu 0x45ab20,
   Obsolete 0x45ab50 (returns 0, never loaded), NoCondition 0 (always). The predicates test
   the menu object (0x4dc0c4), SmallFonts (0x4f0e08), ScreenMode (0x4dc0c8 == 2) and
   in-game state flags 0x4e8904..0x4e8910. (medium)
3. FontSpacings: `index, letter spacing (+0xa20), line spacing (+0xa1c)` until -1
   (the two numbers are read only if the font object exists).
4. FontForeColors: `index, R, G, B` until -1 (0x490a30 -> +0x5494).
5. FontBackColors: `index, R, G, B` until -1 (0x490a60 -> +0x5497).
6. FontData: `index, first char, last char, frame colour, file` until -1
   (0x4929f0 adds a file: max 4, frame colour must be 1..255).

##### 4.8.2.2 The 35 fonts (from `tools/fnt.py list`; h = height of the .FNT, sp = letter,line)
| # | name | type | condition | h | sp | fore | back | files [chars] |
|---|---|---|---|---|---|---|---|---|
| 0 | ChronoLowRes | Direct | InGameLowRes | 6 | 0,0 | 255,255,255 | 0,0,0 | LowRes\Chrono [48..60] |
| 1 | MotoLowRes | Pal | InGameLowRes | 9 | 0,0 | (255,50,20) | 255,255,255 | LowRes\Moto [41..94] |
| 2 | PlayerLowRes | Pal | InGameLowRes | 8 | 0,0 | 255,0,0 | 255,255,255 | LowRes\Name [32..159] |
| 3 | TimeLowRes | Direct | InGameLowRes | 17 | -3,0 | 0,200,0 | 0,0,0 | LowRes\PosTime [32..57] |
| 4 | ChronoHiRes | Direct | InGameHiRes | 12 | -1,0 | 255,255,255 | 0,0,0 | HiRes\Chrono [48..60] |
| 5 | MotoHiRes | Pal | InGameHiRes | 16 | 0,0 | | 255,255,255 | HiRes\Moto [41..94] |
| 6 | PlayerHiRes | Pal | InGameHiRes | 16 | 0,0 | 255,0,0 | 255,255,255 | HiRes\Name [32..159] |
| 7 | TimeHiRes | Direct | InGameHiRes | 34 | -4,0 | 0,200,0 | 0,0,0 | HiRes\PosTime [32..57] |
| 8 | InfosDebug | Direct | NoCondition | 12 | 0,1 | 0,0,250 | 255,255,200 | Delphine\Debug [32..125] |
| 9 | MenuLink | AntiAlias8Transp | NoCondition | 29 | 2,4 | 200,200,0 | 150,150,0 | HiRes\MenuLink [32..95], HiRes\MenuLin2 [192..211] |
| 10 | SpritesLowRes | Pal | InGameLowRes | 19 | 0,0 | 255,255,255 | 0,0,0 | LowRes\Sprites [65..99] |
| 11 | SpritesHiRes | Pal | InGameHiRes | 33 | 0,0 | 255,255,255 | 0,0,0 | HiRes\Sprites [65..99] |
| 12 | Messages | AntiAlias8Transp | NoCondition | 21 | 1,1 | 255,127,10 | 200,100,0 | HiRes\Msg [32..255] |
| 13 | GameMsgLowRes | Direct (forced) | InGameLowRes | 17 | -3,0 | 255,10,10 | 127,5,5 | LowRes\PosTime [32..57], LowRes\PosTimeL [65..90] |
| 14 | GameMsgHiRes | Direct (forced) | InGameHiRes | 34 | -4,0 | 255,10,10 | 127,5,5 | HiRes\PosTime, HiRes\PosTimeL |
| 15 | SpeedLcdHiRes | Pal | InGameHiRes | 18 | -5,0 | | | HiRes\SpeedLcd [32..57] |
| 16 | SpeedLcdLowRes | Pal | InGameLowRes | 10 | 0,0 | | | LowRes\SpeedLcd [32..57] |
| 17 | MenuHiRes | Direct | OutGame | 22 | 1,1 | 255,0,0 | 0,0,0 | HiRes\Generic [32..255] |
| 18 | Symbol | CustomColor | Obsolete | - | 1,1 | | | (none) |
| 19 | PosLowRes | Direct | InGameLowRes | 17 | -3,0 | 0,200,0 | 0,0,0 | LowRes\PosTime [32..57] |
| 20 | PosHiRes | Direct | InGameHiRes | 34 | -4,0 | 0,200,0 | 0,0,0 | HiRes\PosTime [32..57] |
| 21 | MenuLabel | AntiAlias8Transp | OutGame | 16 | 0,6 | (255,50,20) | 255,255,0 | HiRes\MenuLab [32..90] |
| 22 | MotorLcdHiRes | Pal | InGameHiRes | 21 | 1,1 | | | HiRes\MotorLcd [48..61] |
| 23 | MotorLcdLowRes | Pal | InGameLowRes | 11 | 1,1 | | | LowRes\MotorLcd [48..61] |
| 24 | Keyboard | Pal | OutGame | 31 | 1,1 | | | HiRes\Keyb [27..174], HiRes\Keyb2 [205..245] |
| 25 | FinalPosHiRes | Pal | InGameHiRes | 39 | 1,1 | | | HiRes\FinalPos [47..61] |
| 26 | FinalPosLowRes | Pal | InGameLowRes | 19 | 1,1 | | | LowRes\FinalPos [47..61] |
| 27 | Generic | Direct | InGameGeneric | 22 | 1,1 | (255,50,20) | 0,0,0 | HiRes\Generic [32..255] |
| 28 | CounterHiRes | Pal | InGameHiRes | 28 | -5,0 | | | HiRes\Speed [48..59] |
| 29 | CounterLowRes | Pal | InGameLowRes | 14 | -3,0 | | | LowRes\Speed [48..59] |
| 30 | KeyboardHelp | Pal | InGameHelpMenu | 17 | 1,1 | | | HiRes\KeybHelp [26..173] |
| 31 | DataDiskMenu | Direct | OutGame | 21 | 0,0 | (255,50,20) | (100,100,10) | HiRes\DataDisk [32..90] |
| 32 | GameMenuHiRes | Pal | InGameHiRes | 18 | 0,2 | | | HiRes\GameMenu [32..220] |
| 33 | GameMenuLowRes | Pal | InGameLowRes | 9 | 0,1 | | | LowRes\GameMenu [32..220] |
| 34 | Help | Pal | InGameHelpMenu | 9 | 0,1 | 0,0,0 | | HiRes\Help [32..220] |
(blank / bracketed = the defaults 255,50,20 and 100,100,10; Pal fonts ignore both.)
The 4th FontData number (frame colour) only matters for the RAW scan.

Per language? **No.** FONTS.BPT has no language field; one set of fonts for all six
languages. Accented capitals are in Msg/Generic/GameMenu (chars 192..255; MenuLink adds
192..211 from MenuLin2). Per **resolution**, yes: LowRes/HiRes pairs (2.3). (high)

##### 4.8.2.3 Which font a number gives: `0x4903d0` Font_Get(index, int *substituted) (high)
- If `0x4f0e08` (SmallFonts, from Config) == 0 and `0x4dc0c8` (ScreenMode) == 2
  (640x480): low-res numbers become their hi-res partner: 0->4, 1->5, 2->6, 3->7, 10->11,
  13->14, 16->15, 19->20, 23->22, 26->25, 29->28, 33->32. Otherwise the reverse
  (4->0 ... 32->33). Other numbers stay.
- Returns the mapped font if loaded (+0x5248); else the asked one if loaded; else font 8
  (InfosDebug) and *substituted = 1. Index >= count -> font 0. No fonts at all -> one
  message box "No font loaded", returns 0.
- Direct3D mode (0x4f1814 set, 0x4f1828 clear) replaces AntiAlias8/8Transp fonts by
  font 27 (Generic). Not relevant to the software port.
ScreenMode 0x4dc0c8: 0 = 320x240, 1 = 512x384, 2 = 640x480 (Menu_Frame forces 2 at
0x409fc3 and restores it after). (medium for 0/1, high for 2)
The menu pages use (calls to 0x4903d0 with a constant): font 12 Messages 87 times,
21 MenuLabel 10, 31 DataDiskMenu 9, 9 MenuLink 6, 32 GameMenuHiRes 4, and a few others.


#### 4.8.3 Drawing text

##### 4.8.3.1 Colour tables (slot 1, called as (1, 0xff) after loading and by SetColors) (high)
`0x473a90(r, g, b)` makes a screen colour: `(r&0xff)>>3 << 11 | (g&0xff)>>2 << 5 |
(b&0xff)>>3` for RGB565 (shift globals 0x4ee9b8/bc/c0 = losses 3,2,3, 0x4ee9c4/c8/cc =
positions 11,5,0, masks 0x4ee9d0/d4/d8 = R,G,B masks, set at 0x473c43 from
DDPIXELFORMAT dwRBitMask/dwGBitMask/dwBBitMask - so the 3-byte colours are R,G,B).
- **Direct / AntiAlias8 / AntiAlias8Transp (`0x490b90`)**: a 32-entry ramp at 0x60d1a0:
  per channel `acc = back << 16`, `step = ((fore - back) & 0xffff) * 0x800`; entry i =
  `0x473a90(acc_r>>16, acc_g>>16, acc_b>>16)`, then `acc += step` (32-bit). Entry 0 =
  back colour, entry 31 = back + 31/32 of the way to fore (the &0xffff makes negative
  steps work modulo 256). Then table[v] = ramp[v & 31] for v = from..to (1..255).
- **Pal / Moto (`0x495bd0`)**: table[v] = 0x473a90(palette[v % entries]) (R,G,B bytes).
- **CustomColor**: no table; per pixel (3.4).
- `0x490b00` SetColors(fore*, back*): copies the 3-byte colours into +0x5494/+0x5497 if
  different and then calls slot 1 (1, 0xff). CustomColor's slot 0 does nothing.
- NB: `0x493490` (3.3) also copies its fore/back arguments into +0x5494/+0x5497 but does
  **not** rebuild the table, so for Direct/AntiAlias/Pal they change nothing in software
  (they matter for CustomColor and Direct3D). To recolour a Direct or AntiAlias font,
  callers call slot 0 first (e.g. 0x446f0e, 0x4471f3) and the text objects do it around
  each draw (3.6).

##### 4.8.3.2 `0x493820` DrawString(font, int noscale, char *text, x, y, flags, level, int
*rect_out, u8 *fore, u8 *back) (high)
Wrappers: `0x493e20`, `0x493ee0` (noscale 0, char*), `0x493e60` (noscale 0, CString by
value), `0x493f20`, `0x493fe0` (noscale 1), `0x493f60` (noscale 1, CString).
- Nothing if no screen object (0x4dc0c0), empty text, or Direct3D lost surfaces.
- Target: the back surface (address of (x,y) by `0x49d0f0`, pitch 0x62ebec pixels, clip
  width 0x624f58, height 0x68b008), or an alternative buffer (0x4e7058, pitch 0x4e7054,
  0x4e704c x 0x4e7050) when 0x4ee940 and 0x5d8940 are set. Locks the surface with
  0x49a160 if needed and unlocks with 0x49b0e0.
- Halves x and y when `noscale == 0`, the menu object is null or its first field is 0
  (in-game), and (SmallFonts or ScreenMode != 2). In the 640x480 menu: no change.
- flags & 0x100: word wrap - a copy of the text with '\n' inserted where a line would
  pass the clip width.
- `0x492c20` places the block (w, h from 0x492b10): flags & 0x1f: 2 or 4 -> x = (W-w)/2;
  8 or 0x10 -> x = W - x - w; others (0, 1) leave x. flags & 0xe0: 0x40 -> y = (H-h)/2,
  0x80 -> y = H - y - h; 0x20 or 0 leave y. (Flags 0x21 skip even the measuring.)
- Level: type 5 -> `(level << 11)/100 >> 8` (0..8); type 6 -> `level*0xe000/100 >> 8`
  (0..224); others unchanged (Direct/Pal ignore it in software). Direct3D: 0..63.
- Then `0x493490` (if 0x4f1824 is set, i.e. the 2D drawing is on).

`0x492b10` Measure(font, text, *w, *h, one_line): w = longest line of sum(width[c] +
letter spacing) over chars with width > 0 (so it includes the spacing after the last
char); h = height + (line spacing + height) per '\n' (rounded up to even when 0x624b14).
`0x417d20` and 0x416300 add the line spacing once more to h for layout.

##### 4.8.3.3 `0x493490` the string loop (high)
- Optional fore/back (3-byte) copied into the font (see 3.1 NB).
- 0x624b14 (in-game "every other line" mode): y forced even (moved up one row if it
  would go off the bottom).
- flags & 0x1f == 4 or 0x10: each line is centred / right-aligned inside the block
  width (measured per line, one_line=1).
- For each byte: '\n' -> x back to the block's left (+ that line's alignment), y +=
  height + line spacing; else slot 6 DrawChar(&screen_ptr, pitch, ?, clip_w, clip_h,
  char, &x, &y, level). Characters are unsigned bytes; 0 and width-0 chars are skipped
  without advancing.
- rect_out (if given): x, y of the start, then right = max line end - letter spacing,
  bottom = y + line spacing.

##### 4.8.3.4 DrawChar: one glyph into RGB565 (high: inner loops read in the asm)
Common: glyph w = width[c], h = height; clipped to 0..clip_w-1, 0..clip_h-1; pixel 0
never drawn; afterwards x += w + letter spacing (screen pointer likewise). If 0x624b14
and in-game, rows with odd screen y are skipped. No scaling, no separate shadow pass:
outlines and shadows are pixels of the glyph (e.g. DataDisk's dark edges are ramp
entries near 0 = back colour; MenuLab's 32..160 are the back colour made translucent;
Debug's 128 is an opaque back-colour box).
- **Direct `0x493100` / Pal `0x495840`**: `screen = table[b]` for b != 0.
- **AntiAlias8Transp `0x495280`**: `t = level + (b >= 32 ? b & 0xe0 : 0)`; if t > 224
  skip; `a = (t << 8) / 224` (unsigned, 0..256); S = table[b] (= ramp[b & 31]), D = the
  screen pixel; for each mask m in (0xf800, 0x07e0, 0x001f):
  `out |= ((S&m) - ((((S&m) - (D&m)) * a) >> 8)) & m` - the machine does imul and a
  logical shr on 32 bits; because of the final mask this equals an arithmetic shift
  (floor), which is what tools/fnt.py does. a = 0 -> glyph colour, a = 256 -> screen.
- **AntiAlias8 `0x494bb0`** (unused): k = level + (b >> 5); k >= 8 skip; k == 0 ->
  table[b]; else per channel sum of two lookups in the 1 KB tables at +0x54a6 + k*0x400
  (glyph) and +0x74a6 - k*0x400 (screen) - an 8-level mix. Table fill not read. (low)
- **CustomColor `0x494170`**: v = b & 31; colour = 0x473a90((fore*v + back*(31-v))/31
  per channel). Only font 18 (never loaded).
- **DirectTrame `0x492d20`** (unused): like Direct but a pixel is drawn only if level == 0
  or `level*2 < ((rows_left & 15)^2 + (cols_left & 15)^2) & 0x7f` (a dissolve pattern).
- Every DrawChar also has a Direct3D branch (textured quad via 0x485e50 with colour
  0x4f1804) taken when the font has a texture (+0xa38/+0xa3c) and the menu object's first
  field is 0. Not needed.

##### 4.8.3.5 The named colours at 0x4f0dd4.. (high: bytes read from the image)
3-byte R,G,B colours on a 4-byte stride (passed as `u8*`, not RGB565):
0x4f0dd4 white (255,255,255), **0x4f0dd8 black (0,0,0)**, 0x4f0ddc grey (127,127,127),
0x4f0de0 (0,255,255) cyan, 0x4f0de4 (255,0,255) magenta, **0x4f0de8 (255,255,0) yellow**,
0x4f0dec (255,0,0) red, 0x4f0df0 (0,0,255) blue, 0x4f0df4 (0,255,0) green,
0x4f0df8 (19,188,5) green, 0x4f0dfc (255,160,0) orange.
(0x4903d0 does not make colours: it is Font_Get. The colours are turned into RGB565 by
the font's slot 1 when SetColors is called with them.)

##### 4.8.3.6 Text objects: the list at 0x53b8e8 (high for the list functions, medium for the
motion maths)
Three lists of 48 pointers (0xc0 bytes each): **0x53b8e8** (menu and game messages),
0x53b9a8, 0x53ba68. Object = 0xc2 bytes, vtable 0x4d0560 (base class ctor 0x415cd0,
vtable 0x4d0540). Slots of 0x4d0560: 0 0x489c50 destructor, 1 0x4160a0, **2 0x416da0
Draw**, **3 0x4169b0 IsAlive(force)**, **4 0x4173d0 debug frame**, 5 0x4168b0,
**6 0x4165c0 Relayout**, 7 0x417210, 8 0x415f50, 9 0x416100, 10 0x417080, 11 0x416a60.

Object fields: +4 drawn this frame; +8 align flags; +0xc = 1; +0x10 effect; +0x14 motion
mode; +0x18/+0x20 velocity (doubles); +0x28/+0x30 position (doubles, x and screenH - y);
+0x38 double k; +0x40 double s; +0x48/+0x50 doubles = 1.0; +0x58/+0x5c vx, vy (int);
+0x60/+0x64 target x, y; +0x68/+0x6c authored target; +0x70/+0x74 current x, y;
+0x78/+0x7c authored x, y; +0x80/+0x84 text size; +0x88 start time (ms); +0x8c lifetime
(ms, 0 = forever); +0x90 blink period (ms); +0x94 frame counter; +0x98 blink frame
mask; +0x9c id (from the global counter 0x4e4ecc); +0xa0 int* that receives the id;
+0xa4 no off-screen check; +0xac text (own copy); +0xb0 font; +0xb4 font number;
+0xb8 use own colours; +0xbc fore R,G,B (default 255,255,255); +0xbf back R,G,B
(default 127,127,127).

**`0x417d20` TextList_Add(list, ...) - 26 stack arguments** (this = list in ecx;
checked: args 14 and 15 are never read):
| # | name | FULL D3D VERSION (0x446f8a) |
|---|---|---|
| 1 | text (char*) | "FULL D3D VERSION" (0x4e94e0) |
| 2 | lifetime ms (+0x8c), 0 = until removed | 0 |
| 3 | font number (through 0x4903d0) | 0x1f DataDiskMenu |
| 4 | x | 0 |
| 5 | y | 0x3c = 60 |
| 6 | align flags (+8), applied by 0x417900 | 0x22 = centred horizontally |
| 7 | effect (+0x10): 0 none, 1 FadeIn, 2 FadeOut, 3 Blink, 4 BlinkTransp, 5 Transp75, 6 Transp50, 7 Transp25 | 3 Blink |
| 8 | motion mode (+0x14): 0 static, 1 fly to target, 2 drift, 3 wait lifetime then 1, 4 wait lifetime then 2 | 0 |
| 9 | blink: < 0 -> period -n ms (+0x90); > 0 -> frame mask (+0x98); 0 -> 2 for effects 3 and 4 | -500 |
| 10 | int* receiving the object's id | &0x4e98ac |
| 11 | x step to dodge other texts | 0 |
| 12 | y step to dodge other texts | 0 |
| 13 | if no free place: re-align centred both ways (0x42) | 0 |
| 14, 15 | unused | 0, 0 |
| 16, 17 | vx, vy (+0x58/+0x5c; initial velocity for mode 1, direction for mode 2) | -1, 1 |
| 18, 19 | target x, y (+0x60/+0x64), aligned like x, y | 0, 40 |
| 20+21 | double s (+0x40), mode 2's starting speed | 1.5 (0, 0x3ff80000) |
| 22+23 | double k (+0x38), mode 1 gain / mode 2 acceleration | 1.0 (0, 0x3ff00000) |
| 24 | use own colours (+0xb8) | 1 |
| 25 | fore colour, u8[3] R,G,B, copied to +0xbc | 0x4f0de8 yellow |
| 26 | back colour, u8[3], copied to +0xbf | 0x4f0dd8 black |
Returns the object or 0. Steps: Font_Get; find a free slot (empty or not alive; none ->
0); measure the text (h += line spacing); unless mode 1: align with 0x417900 and, if the
box overlaps another live text (0x417ac0), try y += step12 / y -= step12, then x +=/-=
step11 within the screen, then (arg 13) centre; new object, 0x416170 stores the motion
fields (start time = clock 0x4e4ed0, or 0x4e9814 in demo playback), 0x416300 copies text
+ font and measures; mode 1 objects get slot 6 Relayout at once.
FULL D3D VERSION: the title page adds it on each frame whose counter 0x4e90e0 is 11..20
(the counter wraps after 20), centred at y = 60, yellow ramp over black edges, blinking
500 ms on / 500 ms off. The other arguments are inert for mode 0. Sample:
`build/png/fonts/sample_fulld3d_title.png` (drawn by tools/fnt.py, not a capture).

`0x417900` Align(w, h, *x, *y, flags) on the screen size (W = 0x5d93c8, H = 0x5d8948):
flags & 6: with flags & 0x18: x = W/2 - x - w; else flags & 1 ? x += W/2 : x += (W-w)/2.
Without 6: flags & 0x18 -> x = W - x - w. Vertical: 0x40 with 0x80 -> y = H/2 - y - h;
0x40 with 0x20 -> y += H/2; 0x40 alone -> y += (H-h)/2; 0x80 alone -> y = H - y - h.

`0x416da0` Draw: refresh the font (Font_Get(+0xb4)); motion step `0x416b40`; then by
effect, visible and a level (percent):
- 0: level 0. 5/6/7: level 75/50/25.
- 1 FadeIn: lifetime > 0 -> level = 100 - elapsed*100/lifetime; else if period > 0:
  level = (1 - elapsed % period)*100 (a quirk; period is in ms).
- 2 FadeOut: level = elapsed*100/lifetime (or (elapsed % period)*100).
- 3 Blink: period 0 -> visible when ((++frames) & mask) != mask; else visible when
  elapsed % (2*period) < period. Level 0.
- 4 BlinkTransp: p = period or 500; e = elapsed % 2p, folded (e > p -> 2p - e);
  level = ((e << 4)/p * 100) >> 4.
(elapsed = clock 0x4e4ed0 - start.) If visible: when +0xb8, save the font's colours,
SetColors(+0xbc, +0xbf), draw with 0x493e20(font, text, x, y, 0x21, level) (x, y
doubled when in-game and not 640x480 - undone by 0x493820's halving), restore colours.
Only AntiAlias8Transp (and Direct3D) fonts show the level; Direct and Pal fonts ignore it,
so FadeIn/Out are instant for them.

`0x416b40` motion (each Draw):
- mode 1: if not at the target: d = target - pos (y measured upward), dist = |d|; dist
  <= 1.5 -> snap to target. Else heading = atan2(vy, vx), aim = atan2(dy, dx); the
  difference wrapped by +-2*pi (constants 0x4d0510 pi/2, 0x4d0518 pi, 0x4d0520 2pi,
  0x4d0528 -pi/2); new heading = heading + k*diff*0.05 (0x4d0530); new speed =
  (dist - |v|)*k*0.05; v = speed*(cos, sin); pos += v; x, y = truncated pos.
- mode 2: s += k; x += trunc(vx*s); y += trunc(vy*s) (ints, each frame).
Note: one step per drawn frame, not per ms (frame-rate dependent in the original).

`0x4169b0` IsAlive(force): id != 0, (force or lifetime 0 or elapsed <= lifetime), the
box intersects the screen rectangle (0x624374, 0x6d1040, size 0x624f58 x 0x5d8948)
unless +0xa4, text and font set.
`0x418790` TextList_Update (all 48): lifetime over and mode 3 -> mode 1, start =
timeGetTime(); mode 4 -> mode 2, lifetime 0, vy = 1 if vx = vy = 0, s = 1.0 if 0; then
delete the object if not alive.
`0x418860` TextList_Draw: for each live object Draw (slot 2) then slot 4 (0x4173d0 ->
0x4174f0: only with the debug flag 0x4e4ec8 - frame rectangles and the id in font 8).
`0x417840` TextList_IsEmpty: 1 when no object is alive.
`0x417750` TextList_Clear: deletes all 48.
`0x4178c0` SetTextClock(ms): 0x4e4ed0 = ms if > 0, else 0x4e9814 in demo playback, else
0x50bbc8. Menu_Frame (0x409c63) passes timeGetTime() (also stored in 0x50bbc8); 0x4120e0
passes -1.
`0x4165c0` Relayout (slot 6): positions are authored for 640x480; at 512x384 (ScreenMode
1, when the font was substituted or hi-res) x = ((x<<8)/640 << 9) >> 8, y = ((y<<8)/480
* 384) >> 8; in other low modes x/2, y/2; then re-align start and target with 0x417900.

##### 4.8.3.7 Port recipe (summary)
Load FONTS.BPT and the .FNT files once; keep per font: height, spacings, widths, glyphs,
fore/back, palette. Colour table per font (3.1). Draw strings with 3.2-3.4 into the
RGB565 framebuffer at 640x480; implement Direct, AntiAlias8Transp, Pal. Implement the
48-slot text list with effects 0..7 and modes 0..4 driven by a millisecond clock.
`tools/fnt.py` is the reference for the pixel maths.

**In the port** (`port/src/script.c`, `font.c`, `text.c`): FONTS.BPT and every .FNT
loaded at start; the clock set at the top of Menu_Frame (0x409c70, timeGetTime), the list
updated and drawn before the pointer, cleared on a page change. **Checked**: "FULL D3D
VERSION" (`OMR_TEXT="31,0,60,0x22,3,-500,5,1,FULL D3D VERSION"`) covers the same 2171
pixels as in the original's title capture (orig_p14_title_fulld3d.png against
orig_p14_title.png), every one equal in 5-6-5; shown 0-499 ms, hidden 500-999 ms.
Motion 1 (0x416b40's fly-to-target) came with the race's dialogs (`dialog.c`, §4.19); the dodging
with the race's messages (below).

##### 4.8.3.8 Keeping out of each other's way: 0x417d20's arguments 11..13 (written 2026-10-30) [asm 0x417e00..0x417f9b, 0x418160 the same]
The list 0x53ba68's objects, and GAME OVER's in 0x53b8e8, are placed with **steps 5, 5 and
re-centre 1**: 0x4188b0 (every say: GET READY, the countdown, the switches' messages), 0x481970
(the final place), 0x482220 ("3 LAPS"), 0x482120, and the CD form's pictures. The other callers of
either routine with non-zero steps are 0x483e90 and 0x484380 (steps 5, 5, no re-centre: font 6 in
0x53b8e8, not in the port) and 0x4121e0 (the stub's list 0x53b9a8, never). The font form's four
messages have none, so they stay in the middle.

After 0x417900's aligning, the place is **tested against every live object of that list only**
(0x417ac0: the target for a motion 1 object, else its present position). Its test is not a full
box test: the new box's left or right edge must be in the other's span and its top or bottom in the
other's, so a smaller object wholly inside it is missed; right and bottom edges counted one past.
If taken:
1. y step > 0: y += step until free or y >= screen height − h; failing that, from the start, y −=
   step until free or y < 1.
2. Failing that, x step > 0: the same left (x < 1) then right (x >= screen width − w).
3. Failing that, re-centre: the original arguments x, y through 0x417900 with 0x42. Without
   re-centre the object goes where it first stood.

**The loops move once more after the free test**: the object lands one step beyond the free place,
so a dodge always leaves at least a five-pixel gap more than needed.

0x418160's sprite form tests at the place plus the first sprite's offset (+0xc − +4, +0x10 − +8)
and takes it off after, with the parts' summed width and height. Every first part the port has
is offset 0, so `dodge` leaves that out.

Motion 1 sets all three to 0 (no aligning, no dodge).

0x417b90 → 0x417bd0: the **base object** (0x415cd0, vtable 0x4d0540, 0xac bytes): no text, no draw,
an id, a lifetime, a box. Alive by 0x416920 (the screen test as usual). Two callers: **0x434ae0**,
a dialog's first draw with +0x1c: the list cleared, the box (+0x68, as wide as x2 − x1 + 1) put
there with lifetime 0, its id in +0x2c, removed by 0x4354e0; and **0x482f00**, the in-race table,
200 ms (not in the port). Port: `text.c` `place_taken`, `walk`, `dodge`, `text_reserve`.

What it changes on GOG's data as far as seen: nothing in the race's ordinary pictures (the test
pictures keep their sums, the countdown and GET READY and "3 LAPS" are clear of each other). It
shows when two messages come together: Ctrl+S then Ctrl+K in a race, "SOUND OFF" stays at y 24..36
and "KEYBOARD" lands at y 54..66, **the same rows and columns in the port and in the original**
(a run under wine, 2026-10-30, measured on the red pixels). The tracker's "flying texts" was this
line of the notes: motion 1 had already come with the dialogs.


#### 4.8.4 Where the words come from

##### 4.8.4.1 The language string table (high)
`0x4e2920`: records of 28 bytes `u32 id, char *en, *fr, *de, *it, *sp, *sw` (null = not
translated), 191 records, ended by id -1 at 0x4e3e04 (just before the picture table
0x4e3e20). `0x415a70` SetLanguage(n) (n = 0x4dbce8: 0 US, 1 FR, 2 GR, 3 IT, 4 SP, 5 SW,
the order of the MenuXX folders 0x4e8b60): fills `char *text[192]` at **0x53b5e8** with
"INVALID STRING" (0x4e4eb8), then text[id] = record[n] or the English one. Code reads
it directly as `*(char**)(0x53b5e8 + 4*id)`. Ids 0..191 (not in order): control names
(0-13), "Lap(s)", "ONE RACE"/"CHAMPIONSHIP"/"GRAND PRIX" (19-21), the options texts
(25-48, 93, 171-174, 188-191), network messages, in-game menu items in brackets
("[YES]", "[QUIT RACE]", 101-114), sound/replay messages (115-135), error messages
(175-182). Full dump: `build/notes/menu_strings.txt`. The brackets are characters of the
GameMenu fonts (button ends, presumably; not checked).

##### 4.8.4.2 Pictures, not text (high)
Most menu words (page titles such as "MAIN MENU", buttons "Options", "Exit", "Play Solo")
are LEZ pictures from `data\MenuXX\` (NOTES §4.7, language-dependent folder). Checked:
MenuLabel drawn as "MAIN MENU" does not match the original screen
(`build/png/fonts/sample_mainmenu_label.png` vs `ref_mainmenu_label.png`: the original
is a bolder italic face, i.e. part of the picture).

##### 4.8.4.3 .GEN scripts (medium)
Plain-text "generic" sequences: `GENERIC.BKF` (START, END, DSI1-3, EA1-3, BMG1-3,
SHORT1-8 = credits/intro texts) and `MenuXX\HELP.GEN` (the in-game key help, per
language, opened by 0x43cf90 "openHelpMenu", "Help.gen"). Parser `0x488a30` (CloseGeneric
0x48b070, checkGeneric 0x48ae40); it makes text objects (calls 0x416170/0x416300).
Syntax: `{ ... }` one text item; `[X...]` tags (letter upper-cased): `B n`, `C r,g,b`
fore colour (0x4f1604..), `G r,g,b` back colour (0x4f1608..), `D n` (0x4f162c), `E name`
effect None/FadeIn/FadeOut/Blink/BlinkTransp/Transp75/Transp50/Transp25 -> 0..7
(0x4f1600, high), `F name` font by name (0x490690 Font_FindByName: compares the 15-char name at font+8, default 8; e.g. FMessages, FKeyboardHelp, FHelp; -> 0x4f15fc), `H C|L|R`
horizontal align -> flags 2/1/8, `V C|D|U` vertical -> 0x40/0x80/0x20 (0x4f1610), `I n`,
`L n` lifetime ms (0x4f1614), `MH n` `MV n` `MX n` `MY n` (0x4f1618..0x4f1624, motion),
`S name` (up to 16 chars, 0x5f6760: the key/section, e.g. SF1, SESC), `W n` (a column
position, W-1 = back to default), `Y n`; `\NNN` = character code NNN (key-cap glyphs of
KeyboardHelp, e.g. \131 = F1), `\(` `\)` literal brackets; newline = new line.


#### 4.8.5 The 3D font (`data\menu\font3d`, page 15 name entry) (medium)
Not a bitmap font: 26 3D models `LA.3DF/.MAP` .. `LZ.3DF/.MAP` (3DF + MAP as in NOTES
§4.5, textured, purple), plus `BACKSPAC`, `VALID`, `CADRE` (the selection frame; also
CADRE1-3.3DF), `CURSEUR` (+CURSEUR2). Loaded by page 15's start routine `0x487070`
(loop of 0x1a letters with "data\menu\font3d\l%c.3df/.map"; it first calls 0x49a0e0 and
0x49a100 with 1024.0, meaning not read) and drawn by `0x487a70` from the frame routine `0x487bb0` through
the 3D model renderer (0x47a3a0, 0x47a820, 0x47a8b0). The page calls no bitmap-font routine (no 0x4903d0/0x493xxx/0x417d20 in 0x487bb0); judging
by build/ref/orig_p15_name.png the typed name ("DSI") is made of the same 3D letters.


#### 4.8.6 Open points
- 0x4471d8 (page 5/26 "PRESENTS") draws with MenuLabel at (550, 800) with SetColors
  (white, white): only fits if 0x493820's halving is active at that moment (menu object's
  first field 0). Check under wine.
- The exact meaning of the menu object's first field (0x4dc0c4 -> +0): "drawing in 2D in
  the menu" judging by its uses; not read where it is set.
- AntiAlias8's tables (unused), the `B`, `D`, `I`, `Y`, `W` GEN tags in detail.
- Not compared against a capture of the original yet: the AntiAlias8Transp blend and the
  ramp are from the asm; a capture of a Messages text over a picture would confirm.

### 4.9 The 3D intro pages (25 logo, 14 title) and the software 3D path (mapped 2026-10-26)

Inside 4.9, "section 3.1", "(2.2)" and the like mean 4.9.3.1, 4.9.2.2.
Everything here is from `game.EXE` 1998 (SHA-1 164b4175...), addresses in the program at 0x400000.
Confidence marks: **[asm]** read in the disassembly (build/mr.asm), **[px]** confirmed by
pixel comparison with the running original (tools/orig.sh), **[decomp]** read only in the
Ghidra output, **[guess]** inferred.

The reference renderer is `tools/model.py` (modes `dsi`, `title`, `model`). It follows every
rule below and reproduces the original's frames to within a few dozen pixels (4.9.6).

---------------------------------------------------------------------------------------------

#### 4.9.1 Page 25: the Delphine Software International logo

Page table entry (0x4dbb38 + 25*16): start 0x4537d0, frame 0x453c50, end 0x453bf0.

##### 4.9.1.1 What the start routine 0x4537d0 loads

- **Texture pages** through 0x401160 (thiscall on a texture page object; the objects are an
  array at 0x4f92f8 with stride 0x338):
  - page 0 (object 0x4f92f8) = `data\pages\titleDsi\bnk00_00.raw`
  - page 1 (object 0x4f9630) = `data\pages\titleDsi\bnk00_01.raw`
  - The success flag goes to 0x54098c. Afterwards the u16 at page +0x254 is set to 0x180
    (bit 3 clear, so no half-texel UV tweak in software, see 3.6). +0x150 keeps a copy of the
    path (used only by the Direct3D reload path 0x41b0f0/0x41aee0 when 0x4dc06c is set). [asm]
- **Models** through 0x4216a0(model, name.3df, name.map, -1, 1) into 0x70-byte model
  structures (section 3.1):
  - 0x542150 `camera` (the camera body), 0x5421c0 `D`, 0x542230 `S`, 0x5422a0 `I`,
    0x542310 `DsInt` (the "Delphine Software International" banner). All from
    `data\pages\titleDsi\`. [asm]
  - Each is then recentred on its bounding-box centre by 0x420250 (centre = (min+max)*0.5,
    constant 0x4d0680 = 0.5; normals recomputed by 0x41f920) and scaled by 0x4204b0 with
    per-axis factors:
    camera (0.055, 0.055, 0.06); D, S, I and DsInt each (0.04, 0.04, 0.04). [asm]
- **A picture** 0xb2 `logodsi.lez` is loaded into a fresh 0x74-byte picture object
  (pointer 0x5409c0) but is **never drawn** by page 25; the end routine frees it. [asm]
- **Objects**: an array of 5 object structures of 0x184 bytes at 0x53f850 (built by static
  initialiser 0x4535b0 through ctor 0x48bbc0, which sets +0x14c = -1). Each is initialised by
  0x48bce0(obj, position, angles, model): +0x00..+0x08 angles a,b,c (floats, radians),
  +0x0c..+0x14 position x,y,z, +0x34 model pointer, +0x94 = 1.0. [asm]
  - object 0 = camera model, position (-30, 0, -900), angles (0, -1.701696 [bits 0xbfd9d12d], 0)
  - objects 1..3 = D, S, I; object 4 = DsInt; all position 0, angles (0, -pi/2, 0);
    then object 3 (I, +4 at 0x53fce0) and object 4 (DsInt, +4 at 0x53fe64) get b = +pi/2.
- **Camera** (the view, not the model): camera block = 0x5161c0 + index*0x238 where index
  = 0x4dc0f8. 0x46da40(cam, (0,0,0), (0,0,1500)) sets +0x38 and +0x50; the camera position
  (+0x98..+0xa0) ends at (0, 0, 4500); 0x46e140(cam, target (0,0,0)) builds the look-at
  matrix, which for this position is C = diag(-1, 1, -1) (see 3.3). 0x49a120(1.0) sets the
  screen scale factors to 1. [asm]
- **Tables** filled by 0x453610 (section 1.2). Music: 0x404990(13, 0) = CD/WAV track 13.
- **Button**: 0x40bd40 resets the menu buttons; picture 0x5049b4 (`continue.lez`) placed at
  (-300,-300) i.e. off screen; 0x40bef0 binds key 0x1b (Escape) to it. So Escape skips. [asm]
- 0x5409a8 = timeGetTime() at start (the frame's time base). [asm]

##### 4.9.1.2 Tables (0x453610) [asm]

Six animation slots i = 0..5. Floats.

| table | meaning | values |
|---|---|---|
| 0x540188[6] | progress (sum of dt/duration) | all 0 |
| 0x540ae0[6] | duration in seconds | 15, 14.5, 14, 13.5, 16, 20 |
| 0x5401d0 (stride 0xc) | start position | i0 (50,80,-1000) unused; i1-3 (0,0,20000) unused; i4 (20,0,60000); i5 (50,80,-1000) |
| 0x542100 (stride 0xc) | end position | i0 (50,0,7000) unused; D (200,0,200); S (-50,0,200); I (-250,0,200); DsInt (20,-400,-600); i5 (50,0,7000) |

0x53f788 (count of finished slots, kept as a maximum) = 0.

##### 4.9.1.3 Frame 0x453c50 [asm, px]

1. dt = timeGetTime() - 0x5409a8, as unsigned to float (ms); 0x5409a8 = now. If dt > 500
   (0x4d0bf4) then dt = 100. (So a stall advances only 100 ms.)
2. 0x49b1c0 (begin frame); 0x45a2e0(0,0,0) sets the clear colour black; 0x499ff0 ->
   0x476680 clears the back buffer with a DirectDraw COLORFILL; 0x49a160(0,0) locks and
   resets the 3D lists (section 3).
3. For i = 0..5: progress[i] += dt / (duration[i] * 1000) (constant 0x4d0bfc = 1000);
   t = progress clamped to [0,1] (the clamp is done on the float's integer bits);
   n = number of slots with progress > 1.0.
   - **i = 0 (camera model, object 0)**: angles
     a = 0, b = (1 - t)*pi/2 - 1.701696021 (0x4d0d28), c = (1 - t*t)*pi/2. Position fixed.
   - **i = 1..3 (D, S, I)**: e = end position, L = |e|,
     R = L + 2500 - 2500*t*t (0x4d0d40/0x4d0d44 = 2500), p' = e * R / L;
     theta = -2*pi*t (0x4d0d30), evaluated through the cosine table (section 3.2);
     position = ( p'x*cos(theta) + p'z*sin(theta),  p'y,  -p'x*sin(theta) + p'z*cos(theta) - 800 )
     (0x4d0d48 = 800). Yaw b = (1 - |t|)*8*pi (0x4d0d38) - pi/2 for D and S, + pi/2 for I;
     a = c = 0. So each letter spirals in from 2500 further out, turning four times, and lands
     on its end position shifted 800 towards the camera.
   - **i = 4 (DsInt)**: position = start + (end - start)*t (from z 60000 to -600).
   - **i = 5**: position lerp written into the camera block +0x98..+0xa0 (0x516258..60).
     The camera is not looked again (its matrix stays diag(-1,1,-1)) but 0x48c030 uses the
     position, so the whole logo recedes from z 4500-ish: (50,80,-1000) -> (50,0,7000) over
     20 s. **Visible** (correction: the port without it differed from the original by a
     third of the screen). Slot 5 comes last in the loop, so each frame's objects use the
     position of the frame before. [asm, px: port vs original 1108 px at the end]
4. Objects 0..4 drawn by 0x48c030 (section 3). 0x53f788 = max(0x53f788, n).
5. 0x49ad90 renders the collected faces (software path when 0x504e74 == 0 and
   (0x4dc06c == 0 or 0x4dc070 != 0)). Then 0x40c1f0 (menu buttons), 0x418790 / 0x417840 /
   0x418860 (effect list 0x53b8e8), 0x40c500 (page-transition timer), mouse cursor if
   0x4dbff0 && 0x53c524, 0x49b0e0 (unlock, flip).
6. **Leaving**: r = 0x40c1b0() (returns -1 while a transition runs (menu+0xd48) or
   0x504e84, else the activated button index). r == 0 (Escape) -> 0x40c1e0, then page 14.
   Else if 0x53f788 == 6 (every slot past 1.0, i.e. after the longest, 20 s) -> page 14 via
   0x409eb0(menu, 14), then 0x40c4a0 which sets 0x4dbd00 = 1. [asm, px: the original moves on
   at about 20 s]
7. 0x4dbd00 = 1 makes the next page start with the **1.2 s screen-tile transition**: the
   menu frame sets menu+0xd48, menu+0x20 = now + 1200 (0x4b0), camera 0x504c70 at z 1911;
   0x40c500 advances progress = (now - start)*0.000833333 (0x4d02b4 = 1/1200); the old
   screen (captured by 0x479260) is cut into tiles that fly apart, drawn by 0x479c10 through
   the same 3D path (0x49b2c0 / 0x49b7f0). The new page animates underneath at the same
   time. [asm, px: the shards are visible in captures]

End routine 0x453bf0: 0x4049e0 stops the music, 0x401ab0 frees the texture pages,
0x41f7d0 frees the models, the logodsi picture is freed. [asm]

---------------------------------------------------------------------------------------------

#### 4.9.2 Page 14: the Moto Racer title

Page entry: start 0x4465f0, frame 0x446b80, end 0x446b40.

##### 4.9.2.1 Start 0x4465f0 [asm]

- 0x4dc1c4 = GP edition flag (selects `titleGP` and the GP tables/pictures).
- Texture page 3 (object 0x4f9ca0) = `data\pages\titleMr\bnk00_00.raw` (GP: `titleGP`);
  result flag 0x53f66c.
- Models (0x4216a0 with last argument 1): 0x540338 `ellipse`, 0x5403a8 `moto`,
  0x540418 `racer`. 0x422250(model, 3) adds 3 to every face's page (+0x36), so their page-0
  textures come from page 3. **No recentre** (unlike page 25).
- Scales (0x4204b0):
  - MR: ellipse (0.04, 0.03875, 0.04), moto (0.04, 0.036, 0.0375), racer (0.04, 0.033, 0.0395)
  - GP: ellipse (0.04, 0.0325, 0.0455), moto (0.04, 0.053, 0.04825), racer (0.04, 0.0315, 0.0315)
- Objects at 0x540500 / 0x540684 / 0x540808 (static init 0x446400), angles (0, pi/2, 0),
  position 0.
- Camera: same as page 25 (position (0,0,4500), looking at the origin, C = diag(-1,1,-1));
  0x49a120(1.0).
- Tables by 0x446460 (2.2). Start button: picture 0x5049b0 (`start.lez`) at (0, 430), key
  0x0d (Return).
- 0x53f4e0 = timeGetTime(). The title pictures 0xbf, 0xbe, 0xbd, 0xbc (GP 0xc3..0xc0) are
  drawn once (pre-load), 0x49b0e0, then 0x404990(3, 2) = music track 3, 0x4049f0(0x2d, 0)
  a sound effect.

##### 4.9.2.2 Tables (0x446460) [asm]

| table | meaning | MR | GP |
|---|---|---|---|
| 0x5420e8[3] | progress | 0 | 0 |
| 0x540220[3] | duration (s) | 1.0, 1.2, 1.4 | same |
| 0x53fff8 (stride 0xc) | start | ellipse (60,42,-40000), moto (-12000,44,-798), racer (12000,25,-798) | (69,26,-40000), (-12000,-90,-798), (12000,40,-798) |
| 0x5401a0 (stride 0xc) | end | (60,42,-800), (42,44,-798), (40,25,-798) | (69,26,-800), (252,-90,-798), (-265,40,-798) |

0x5409bc (number of objects that have arrived, kept as maximum) = 0; 0x542148 = 0.

Note the camera matrix flips x: world x = -12000 appears at the **right** of the screen, so
MOTO comes in from the right and RACER from the left. [px]

##### 4.9.2.3 Frame 0x446b80 [asm, px]

1. dt from 0x53f4e0 exactly as page 25 (>500 -> 100). 0x46e140(cam, (0,0,0)) re-looks at
   the origin every frame (no change, the camera is fixed). 0x49b1c0; 0x49a160 (**no clear**:
   the background picture covers the screen).
2. If 0x5409bc > 2 (everything arrived): 0x409bf0 (menu +0x30 = +0x34 = 1), draw picture
   `title4` (0xbf; GP 0xc3) as the whole screen. Frame counter 0x4e90e0: ++, above 10 font
   31 gets yellow/black and "FULL D3D VERSION" is **added** through 0x417d20 (lifetime 0 =
   for ever, Blink with period 500 ms, centred at y 60); above 20 it resets to 0. 0x417d20
   creates the object even when it overlaps a live one (with dodge steps 0 and no
   re-centre every path reaches the creation, asm 0x417e00..0x417f9b), so copies pile up,
   ten every 21 frames, until the 48 slots are full; each blinks 500/500 ms from its own
   start. What is seen depends on the frame rate: under wine (very fast frames) the
   original shows about 600 ms on, 360 ms off (40 captures, build/ref/blink/); at a steady
   60 frames a second the copies' phases cover each other and the text stays on. [asm, px]
3. Otherwise draw the background by 0x5409bc: 0 -> 0xbc `title` (GP 0xc0), 1 -> 0xbd
   `title2`, 2 -> 0xbe `title3`. Each later picture has the arrived objects painted into it,
   so an object stops being drawn in 3D once it has arrived.
   Then for each object k = 0..2: progress += dt/(duration*1000); t clamped to [0,1];
   - k = 0 (ellipse) angles: t < 0.5 -> (a, b, c) = (2*pi*t (0x4d0c08/0x4d0c10), -pi/2, pi);
     t >= 0.5 -> (0, 2*pi*t + pi/2, 0). (Two half-turns about different axes; it ends
     facing the camera.)
   - moto and racer keep angles (0, pi/2, 0).
   - position: progress <= 1 -> lerp start -> end by t; else end, and the arrived count
     increments.
   - the object is drawn (0x48c030) only while 0x5409bc < k + 1.
   0x5409bc = max(0x5409bc, count). 0x4e90e4 accumulates dt.
4. 0x40c1f0, 0x40c500 (the tile transition from page 25 runs over this), 0x49ad90 render,
   effects, cursor, 0x49b0e0.
5. **Leaving**: 0x40c1b0() == 0 (Return / the Start button) -> page 15 (0xf, the name entry).
   No timeout in this routine; the attract mode lives in the menu frame 0x409c00: on page 14,
   when 0x53e5f2 is set and more than 30000 ms passed since menu+0x24 (last input), or when
   0x4dbf80 is set, 0x409a30 starts a demo. [asm; the demo start was not exercised here]

---------------------------------------------------------------------------------------------

#### 4.9.3 The software 3D path

##### 4.9.3.1 Model loader 0x4216a0 and the model structure [asm]

Model structure (0x70 bytes): +0x00 faces (0x40 each), +0x04 vertices (3 floats each),
+0x08 face normals, +0x0c plane d per face, +0x2c face count, +0x30 vertex count,
+0x48.. half extents, +0x58 radius.

**3DF**: u16 version (1), u16 vertex count, vertices as 3 floats each, multiplied by 96.0
(0x4d0690); then faces: u16 n (0 ends the list), u16 colour/attribute (equals n in these
files), n u16 vertex indices.

**MAP**: a 0x200-byte header (bit 7 of byte 1 is a flag), then 0x80 bytes per face:
kind at +0 (< 3 = untextured; 3 or 4 must equal the face's corner count), u values at
+0x22.. and v values at +0x32.. (integers, texels), flags (u32) at +0x48, colour at +0x4e.
Another set of coordinates at +0x0a..+0x1e is not used. An `ANI!` block of 0x80 bytes may
follow (not used by these pages).

Face structure (0x40 bytes): +0x00 draw kind (u16), +0x02 colour RGB565, +0x04..+0x2b five
(u, v) float pairs, +0x2c..+0x34 five u16 vertex indices (+0x34 != 0xffff means a quad),
+0x36 texture page, +0x38 flag bits, +0x3a copy of the draw kind, +0x3c colour 8:8:8
(0x423110).

Corner order (this is the easy thing to get wrong): the builder 0x422c80/0x422be0 stores
the indices reversed, 0x423170 reverses them back and closes the polygon (slot n = slot 0);
the MAP builder 0x422d20/0x422ed0 pairs MAP corner k with **file index k**; then 0x422070
mirrors every vertex x -> -x and 0x423170 reverses every face again (so the winding stays
front-facing after the mirror). Net effect: x negated, corner order reversed, each corner
keeps its own MAP (u,v). [asm, px]

0x4232e0 multiplies u, v by 1/256 (0.00390625). Half-texel adjustments only when page +0x254
bit 3 or Direct3D - never in these pages. [asm]

##### 4.9.3.2 Angles and the cosine table [asm]

0x5fcb80: 16384 floats, COS[k] = cos(k*2*pi/16384), filled by 0x4901e0. Angle to index:
trunc(angle * 2607.594588) & 0x3fff (double constants 0x4d1bf8 / 0x4d0ca8 / 0x4d0ed0;
2607.59 = 16384/(2*pi)); sin(x) = COS[(n - 0x1000) & 0x3fff]. Truncation, not rounding
(_ftol 0x4b3048). The port must use the table to be exact (COS[0x3000] is ~0, not 0).

##### 4.9.3.3 Camera and object transform: 0x46e140 and 0x48c030 [asm, px]

- Camera block (0x238 bytes, 0x5161c0 + 0x4dc0f8*0x238): look-at matrix C at +0x68..+0x88
  (3x3), translation +0x8c..+0x94, position +0x98..+0xa0. 0x46e140(cam, target) builds C
  from the direction position->target; for (0,0,4500) -> (0,0,0) it gives diag(-1, 1, -1).
- 0x48c030(obj, renderer *0x4dc0c0, camera): row-vector rotation
  M = Rz(c)*Ry(b)*Rx(a), entries (cX = cos, sX = sin of the angle, from the table):
  M0 = cc*cb, M1 = sc*ca + cc*sb*sa, M2 = sc*sa - cc*sb*ca,
  M3 = -sc*cb, M4 = cc*ca - sc*sb*sa, M5 = cc*sa + sc*sb*ca,
  M6 = sb, M7 = -cb*sa, M8 = cb*ca.
  View matrix N = M * C; translation T = (pos - campos) * C + cam(+0x8c..0x94).
  0x4039c0 copies; then 0x49b2c0(matrix, vertices, count, 0x1f) transforms.
- The object scale field +0x94 = 1.0 is not used in these pages (scale was baked in by
  0x4204b0).

##### 4.9.3.4 Vertex transform and projection 0x49b2c0 [asm, px]

Sets the x87 precision to 24 bits (PC=single) with round-to-nearest; the port must compute
in float32 for exact pixels. Output records of 0x1c bytes at 0x6d20d8:
+0x00 X*fx, +0x04 Y*fy, +0x08 Z, +0x0c light (0x1f << 13), +0x10 sx, +0x14 sy, +0x18 1/Z.
A vertex is projected only if Z >= near:
sx = round(cx + X*fx/Z), sy = round(cy - Y*fy/Z) (round to nearest through fistp).

Constants for the menu (set by the menu at 0x40a003 via 0x49a0e0/0x49a100, and 0x40a01d,
0x40a037):
- fx = fy = 1911.0 (bits 0x44eee000). (The generic setup 0x4995a0 would use 307200/W and
  230400/H; the menu overrides it.)
- centre cx = 320, cy = 240 (0x60d3c4, 0x624f88, from 0x499ba0(0,0,640,480)).
- near = 256 (0x625774; races use 64). far = 40000 (0x6d1f70).
- 0x62e444 = 1: s-buffer mode.

##### 4.9.3.5 Face setup and sorting 0x49b7f0 / 0x49c590 [asm, px]

0x48c030 calls 0x49b7f0(vertices, faces, count, 1) because object +0x14c == -1 (otherwise
0x49c110). Per face:
- kind 0xd is skipped.
- key = Z of the farthest of the four corners. Skip if that farthest Z <= near.
- if any corner has Z <= near: 3D back-face test (triple product of the corners in view
  space); else 2D: trivial reject if all corners are left of 0, right of 639, above 0 or
  below 479, and the screen area (y2-y1)(x1-x0) + (y0-y1)(x2-x1) must be > 1.0 (so only
  clockwise-on-screen faces survive).
- 0x49c590(face, key (+1024 if flag bit 1, constant 0x4d1f94), vertices, flags, 0):
  bucket = trunc(key * 0.25) (0x4d1f9c); < 0 dropped; >= 7999 clamped to 7998. Buckets are
  LIFO lists (heads 0x655d38, entries 0x18 bytes at 0x65f0e0, at most 7499). The menu skips
  the detail-level kind table.

0x49a160 at frame start resets: vertex counter 0x68b014, face count 0x655d14, every screen
row's free list to one interval [0, 640), 0x6d20c0 = 1, deferred counters 0x6d1048 /
0x6d1f78 = 0.

##### 4.9.3.6 Rendering 0x49ad90 (software, s-buffer) [asm, px]

Front to back: buckets ascending (nearest first); within a bucket from the list head (last
inserted first); at most 2000 faces (0x62e438). Each face goes through 0x4aef20, which looks
up the draw kind in the kind table 0x4f1a70 (16-byte entries {id, sub, flag, span routine},
re-indexed by id at startup by 0x4a2ea0 / 0x4a2e00) and the sub table 0x4f1ee0 ({id,
polygon routine, edge routine, edge routine when flag}).

For the kinds used here (16, 21, 38, 39) sub = 4: polygon routine 0x49e510, edge routine
0x49d9e0 (opaque, flag 0) or 0x49dbf0 (see-through, flag 1).

- **0x49e510 polygon**: fan (0,1,2), (0,2,3). Texture coordinates become integers
  ftol(u * 2^21) (0x4d1fa8; u is texels/256, so this is texels * 8192, 13 fraction bits).
  Near clipping per triangle at Z < near: one corner in front -> one triangle (F, P1, P2);
  two in front -> (F1, F2, P1) and (F2, P2, P1). A clip point P is projected by 0x49d8f0 as
  sx = cx + trunc(X/near), sy = cy - trunc(Y/near) (X, Y already carry fx, fy), and its uv =
  uB + trunc(near - zB) * ((uF - uB) / trunc(zF - zB)).
- **0x49dde0 triangle** (x0,y0,x1,y1,x2,y2, page, u0,v0,u1,v1,u2,v2): rotate the corners so
  corner 0 is topmost; D = (x0-x2)(y1-y2) - (x1-x2)(y0-y2) truncated to int, must be >= 1
  (back faces and slivers dropped). Constant gradients (affine, **no perspective
  correction**): du/dx = trunc((u0-u2)/D*(y1-y2) - (u1-u2)/D*(y0-y2)) (0x655d20), dv/dx the
  same (0x655d24). Edges in 13-bit fixed point (x << 13, slope = (dx << 13)/dy as C integer
  division, no sub-pixel prestep); u and v interpolated down the edge 0 -> 2 (0x624b0c,
  0x68b018 / 0x68b01c, per-row steps (u2-u0)/h as integers); the other side at 0x624b44.
  Rows above 0 are skipped (advancing the edges), rows beyond 479 cut. Per row, if the row's
  free list is not empty, the span [xl >> 13, xr >> 13) is passed to the edge routine.
- **0x49d9e0 (opaque, s-buffer)**: draws the parts of the span that are still free (u,v
  advanced by (a - start)*du for each piece) and removes them from the row's free list (new
  list nodes from 0x60d3d0). Nearer faces were drawn first, so this is a z-buffer-free hidden
  surface removal.
- **0x49dbf0 (see-through)**: records the visible pieces (0x68b040, 0x1c-byte records, at
  most 0x2800) without covering them; 0x49dde0 also records the triangle batch (0x625790,
  0x24 bytes, at most 1000). 0x4af130 replays the batches **last first** after all buckets.
- **Span routines** (kind table): 16 -> 0x4a5ab0 opaque (self-modifying code, texel at
  [v & 255][u & 255]); 21 -> 0x4a5e10 opaque, colour (c >> 1) & 0x7bef (mask at 0x50270c,
  set by 0x401fc0; i.e. half brightness); 38 -> 0x4a5330 skips texel value 0 (see-through);
  39 -> 0x4a5580 skips 0 and halves. For 21/38/39 the texel index is
  ((v >> 13)*256 + (u >> 13)) & 0xffff. No lighting: the light value in the vertex is not
  used by these span routines.
- 0x49b0e0 fills the spans still free with the colour 0x65f0d8 only when 0x625748 is set
  (not in these pages; page 25 clears with COLORFILL, page 14 draws its background first).

An equivalent painter's algorithm (what tools/model.py does): buckets from far to near,
within a bucket in insertion order; opaque spans overwrite, see-through spans skip 0; see-
through batches are drawn in the same order as recorded but only over pixels no opaque
face covered. Pixel-exact results need the float32 transform, the cosine table, the
truncations above and the s-buffer span boundaries.

##### 4.9.3.7 Texture pages 0x401160 [asm, px]

RAW (Image Alchemy) header big-endian: width at +8, height +0xa, palette count +0xc; width
must be >= 256. When the palette count is 0 the image is 24-bit: each row's first 256
texels are read as R, G, B and the rest of the row (w*3 - 0x300 bytes) skipped. A texel is
stored as RGB565 built from the DirectDraw pixel format: (R >> 3) << 11 | (G >> 3) << 6 |
(B >> 3). **Green uses only 5 bits** (its lowest bit is always 0) because the shifts come
from per-channel bit counts 5/6/5 and positions 11/5/0 (globals 0x4ee9ac.., 0x4ee9c4..,
masks 0x4ee9d0..0x4ee9d8 set by 0x473c3c) applied as "top 5 bits then shift". (LEZ menu
pictures use G >> 2, a full 6 bits.) The paletted RAW path converts the palette the same
way then maps indices. 0x43e280 is **not** involved (that is the LEZ/TGA picture path).
Stored as 256 u16 per row at page object +0x330. Texel value 0 is the see-through colour
for kinds 38/39 (pure black RGB < 8,8,8 becomes 0). [asm, px]

---------------------------------------------------------------------------------------------

#### 4.9.4 MAP face flags (u32 at MAP face +0x48) [asm]

For textured faces (MAP kind 3/4) and the loader's last argument = 1 (all these pages):

| flag | effect |
|---|---|
| (none) | draw kind 0x10 (16): opaque textured. (Argument 0 -> kind 1.) |
| 0x10000 | kind 0x26 (38): see-through texture (texel 0 skipped), **two-sided** in practice because the models carry both windings (argument 0 -> kind 2) |
| 0x200000 | kind 0x15 (21): half-brightness texture; with 0x10000 -> 0x27 (39) see-through and half brightness |
| 0x20000 | face flag bit 0 (+0x38) |
| 0x400000 | flag bit 1: sort key + 1024 (drawn as if farther: decals/backgrounds) |
| always | flag bit 2 |
| 0x100000 | flag bit 3 |
| 0x40000 | kind 0x0d: never drawn (skipped in 0x49b7f0) |
| 0x80000 | kind 8: span routine 0x4a5a90 does nothing |
| low 16 bits | texture page number (plus the 0x422250 offset) |

Untextured faces (MAP kind < 3): kind 0 (solid colour, 0x423150), 0x10000 -> 2,
0x40000 -> 0xd, 0x80000 -> 0xc. If the page is loaded and all four (u, v) are equal, the
face is turned into a flat face of that texel's colour.

Bits 0 and 3 of +0x38 had no visible effect in these pages ([guess]: lighting / collision
uses in races).

Flags used by these models: camera 0x200001 / 0x1 (page 1; kinds 21 / 16); D, S, I
0x200000 / 0 (kinds 21 / 16); DsInt, moto, racer, ellipse 0x110000 (kind 38) and 0x310000
(kind 39): flat two-sided billboards in the model's YZ plane (they face the camera after the
b = +-pi/2 yaw).

---------------------------------------------------------------------------------------------

#### 4.9.4b In the port (`r3d.c`, `model.c`, `page_intro.c`, `rand.c`)

Written from 4.9 and the asm, every step a float as the game's (x87 at 24 bits; built with
`-ffp-contract=off`). Checked: the logo against the original 1108 px (end) and 3798 px
(14.6 s) at the best 60 Hz frame; against tools/model.py (given the same camera lag) 306 px
at 14.6 s; the glass as above; the settled title page equal to the original apart from
the pointer (with the pointer on Start, which the original's capture had selected).

#### 4.9.5 What the port needs, in order

1. RAW texture page loader (24-bit and paletted, green 5 bits, 256 texels per row).
2. 3DF + MAP loader with the corner order and mirror of 3.1, the scales and (page 25) the
   recentre.
3. The cosine table, the float32 transform, projection and the sort/clip rules (3.2-3.5).
4. The affine triangle with 13-bit fixed point and the four span kinds; the s-buffer (or the
   painter's equivalent).
5. The two page scripts (1.3, 2.3) with the dt rule (>500 ms -> 100 ms) and progress sums,
   the 1.2 s tile transition between them, Escape on page 25, Return on page 14, and the
   blinking FULL D3D VERSION text (frame counted: 10 on, 10 off at the game's frame rate).

---------------------------------------------------------------------------------------------

#### 4.9.6 Verification against the running original

- Page 25: tools/model.py `dsi` at 19923.5 ms vs build/ref/orig_p25_logo_end.png: 11
  differing pixels of 307200; at 14592 ms vs build/ref/orig_p25_logo_a.png: 28 pixels.
  16 further captures 1.25 s apart fitted with consistent time offsets; residues 14-6776
  pixels, all texel noise on slivers / big camera faces from sub-millisecond timing; the
  orbit, spin, camera-model swing and DsInt approach all match. [px]
- Page 14: captures after the automatic end of page 25 (about 20 s). The fitted model times
  advance by the same 140 ms as the captures (3.02 s -> 1124 ms, 3.16 s -> 1266 ms);
  differences are the Start button (not drawn by model.py) and a few edge pixels; the MOTO
  texture is itself motion-blurred (it is in the page, not an effect). Final state matches
  apart from the blinking text. See build/png/models/title_vs_orig.png (left original,
  right model.py). [px]
- Not verified: the GP edition tables (no GP copy run), the Escape skip on page 25 (xdotool
  key events did not reach the game in the headless session), the 30 s attract-demo
  timeout.

### 4.10 Sound: effects, voices, DirectSound, CD music (mapped 2026-10-26)


game.EXE 1998 GOG build (SHA-1 164b4175..). Addresses are the program's. Confidence:
[asm] read in build/mr.asm; [decomp] Ghidra only; [test] checked on the data; [guess].

#### 4.10.0 Summary for the port

- All sound effects and voices are **125 RIFF WAVE files in SFX.BKF, twice** (`16_BITS\...`
  and `8_BITS\...`), **enciphered with a trivial byte-add** (below). Mono PCM, 5000–22050 Hz,
  16-bit signed or 8-bit unsigned. No other sound files exist in any bank (language banks
  MENUxx hold only pictures and a .GEN; no speech per language: voices are English only).
  `tools/sfx.py` decodes them; `build/sfx/` holds all 250 as plain .wav [test].
- A sound **id (0..255)** maps to a file through **7 tables in the program** of
  `{char *name, int id}` (8 bytes): see §2. The path is
  `data\sfx\` + (`16_Bits\` if Wave16Bits else `8_Bits\`) + folder + name + `.wav`.
- **0x4049f0 play(id, flags)**: ids 0x2f..0x5c are *voices* (switch 0x4db668, volume 0xff),
  everything else *effects* (switch 0x4db664, volume 0xee). Flags: bit0 loop, bit1 use a free
  copy (overlap) instead of restarting, bit2 don't play if this id was the last one played
  in its class. Menu calls all use flags 0 (restart the sound from its start).
- **Volume** v 0..255 → DirectSound attenuation (v−255)×15.625 hundredths of dB (v=0 →
  −39.8 dB). **Pan** p −128..128 → ±p×31.25 hundredths of dB on the other side.
  **Frequency** in Hz, accepted only 1000 < f < 44100.
- **Queued/delayed sounds**: 8 records of 100 bytes at 0x504240 (menu voice prompts after
  600 ms) + 1 sequential voice queue at 0x5028c8 (race announcements); updated once a frame
  by 0x404eb0 (Menu_Frame) [asm].
- **Music** = CD audio tracks 2..13 through MCI. Real CD: plays the track once, mode 2/3
  repeats it **once** (a 1 s timer replays it when MCI says "stopped"). GOG's winmm.dll
  replaces MCI by `PlaySoundA("NN.wav", SND_LOOP|SND_ASYNC)`: **every track loops forever**
  [asm, both]. Track per page/track in §5.

#### 4.10.1 The files

##### 4.10.1.1 Where they are
- `data\SFX.BKF` (28 banks total; the others hold no sound): 250 .WAV =
  2 × (MENU 9, VOICES 31, VOICES\MENU 12, DIVERS 46, KTM 16, NINJA 11). The bank names are
  `16_BITS\MENU\SELECT.WAV` etc.; the game asks for `data\sfx\16_Bits\Menu\select.wav` and
  the bank lookup is case-insensitive (dsi_fopen through the banks, NOTES §3.1).
- `02.wav`..`13.wav` beside game.EXE: the CD's audio tracks (§5). 44100 Hz, 16-bit,
  stereo, plain PCM RIFF [test: python wave]. `build/cd/NN.wav` (cut from the CD image) have
  identical format and the same frame counts (e.g. 03: 6325704 frames both).
- No `data\*.wav` loose files; no per-bike sound bank (the bike sounds are SFX\KTM and
  SFX\NINJA); no per-language speech.

##### 4.10.1.2 Format and cipher (loader 0x440d30, decipher 0x440d00) [asm-checked decomp + test]
`0x440d30(path, WAVEFORMATEX *out, void **data, int *size)`:
1. open (0x42ee60/dsi_fopen mode "rb"), read **0x24 bytes**. If the first byte is `'r'`
   (0x72), the file is enciphered and **each block read is deciphered from i = 0**:
   `plain[i] = stored[i] - 4*(i + 8)  (mod 256)`  (0x440d00: `b += (-8 - i) * 4`).
2. seek `fmtSize - 16` forward (fmtSize = dword at 0x10 of the header), read **8 bytes**
   (decipher again from i = 0): must be `'data'`, size.
3. Require `RIFF`/`WAVE`/`fmt `/`data` at their places (else error 2). GlobalAlloc(size),
   read size bytes, decipher (from i = 0).
4. WAVEFORMATEX built by hand: tag 1 (PCM), channels and rate and bits from the file,
   blockAlign = bits/8 × channels, avgBytes = blockAlign × rate, cbSize 0.
All 250 files are enciphered ('r'), tag 1, mono. 16_BITS = signed 16-bit LE; 8_BITS =
unsigned 8-bit. Many 8-bit files have the RIFF pad byte (+1 after data); BIRD3 and FETE have
32/92 junk bytes after the data (ignored).

Test [test]: every 16-bit file correlates with its 8-bit twin (8-bit minus 128) at median
0.99988 (worst CONTINUE.WAV 0.80: 5 kHz, very coarse 8-bit version); lengths agree to the
sample; zero-crossing rates are those of sounds, not noise (e.g. SELECT 22050 Hz: 3364/s;
the voices 11025 Hz: 1300..4000/s). Rates found: 5000, 8000, 11025, 14000, 16000, 19000,
22050 Hz.

`tools/sfx.py SFX.BKF` lists every file (format, length, peak, zc/s); `-x DIR` writes
plain .wav. It also takes loose files.

##### 4.10.1.3 8 or 16 bits
`0x4db670` = Wave16Bits (CONFIG.BPT; 1 on both the GOG and the CD config). Forced to 0 at
start (0x408720) if the primary buffer did not come out 16-bit. 0x408f30 toggles it and
reloads everything. The port should simply use 16_BITS.

#### 4.10.2 Ids → files (the tables in game.EXE)

Handles: `int 0x519ff8[256]` = sample handle by id (−1 none). Every loader first fills it
with −1, then for each record: path as above, `h = 0x423f80(path)` (load), store,
`0x424030(h, 0xff)` (volume full), `0x424070(h, 0)` (pan centre) [asm].

Two sound sets, never both loaded:
- **Menu set** — `0x403f90` called from 0x40a9a0 (the menu's resource loader, when the menu
  becomes active): menu table (7) and, if voices are on (0x4db668) and not loaded yet
  (0x503554 == 0), the voices-menu table (11); 0x503554 = 1. Freed by 0x4042c0 (from 0x40b1f0,
  the menu's resource release). Page 24 (podium) adds the podium table in its start routine
  (0x404310 → ids 4, 5), freed in its end routine (0x404490).
- **Race set** — `0x4044c0(bikeType)` from 0x410d70 ("Init WAV samples...") at race
  load: divers (18), then ktm (16) if bikeType != 0 else ninja (11), then the track's
  ambient samples (0x4059a0, from `Sound.Spt`, ids from 0x5d up: 0x519ff0 = 0x5d), then
  voices (30) if voices are on. bikeType = `[0x509398 + 0x124*bike]` (bike 0x584499):
  0 = the "Ninja" road bikes, else KTM (the Go sound on page 10 agrees: type 0 → goninja).

0x504874/0x5028a8/0x5028a4 remember which voice table/count/folder is loaded (for the
voices toggle 0x408f30 to reload).

**menu (0x403f90, entering the menu)** table 0x4db7d8, 7 records, folder `MENU\`

| id | rec addr | name | 16-bit file | rate | ms |
|---|---|---|---|---|---|
| 0x28 | 0x4db7d8 | deplett | MENU/DEPLETT.WAV | 16000 | 671 |
| 0x29 | 0x4db7e0 | select | MENU/SELECT.WAV | 22050 | 436 |
| 0x2a | 0x4db7e8 | valide | MENU/VALIDE.WAV | 16000 | 1379 |
| 0x2b | 0x4db7f0 | goktm | MENU/GOKTM.WAV | 16000 | 839 |
| 0x2c | 0x4db7f8 | goninja | MENU/GONINJA.WAV | 16000 | 697 |
| 0x2d | 0x4db800 | title | MENU/TITLE.WAV | 22050 | 1696 |
| 0x03 | 0x4db808 | continue | MENU/CONTINUE.WAV | 5000 | 592 |

**voices-menu (0x403f90 if voices on)** table 0x4db780, 11 records, folder `Voices\Menu\`

| id | rec addr | name | 16-bit file | rate | ms |
|---|---|---|---|---|---|
| 0x36 | 0x4db780 | entrname | VOICES/MENU/ENTRNAME.WAV | 11025 | 1009 |
| 0x52 | 0x4db788 | slctrans | VOICES/MENU/SLCTRANS.WAV | 11025 | 1273 |
| 0x53 | 0x4db790 | selerace | VOICES/MENU/SELERACE.WAV | 11025 | 1363 |
| 0x54 | 0x4db798 | selectmd | VOICES/MENU/SELECTMD.WAV | 11025 | 1229 |
| 0x55 | 0x4db7a0 | playsolo | VOICES/MENU/PLAYSOLO.WAV | 11025 | 859 |
| 0x56 | 0x4db7a8 | playlink | VOICES/MENU/PLAYLINK.WAV | 11025 | 855 |
| 0x57 | 0x4db7b0 | makurchc | VOICES/MENU/MAKURCHC.WAV | 11025 | 1061 |
| 0x58 | 0x4db7b8 | letsgo | VOICES/MENU/LETSGO.WAV | 11025 | 1322 |
| 0x59 | 0x4db7c0 | cngrecor | VOICES/MENU/CNGRECOR.WAV | 11025 | 3209 |
| 0x5a | 0x4db7c8 | cngratim | VOICES/MENU/CNGRATIM.WAV | 11025 | 3064 |
| 0x5b | 0x4db7d0 | yutrygan | VOICES/MENU/YUTRYGAN.WAV | 11025 | 2818 |

**podium (0x404310, page 24 start)** table 0x4db770, 2 records, folder `Menu\`

| id | rec addr | name | 16-bit file | rate | ms |
|---|---|---|---|---|---|
| 0x04 | 0x4db770 | podium1 | MENU/PODIUM1.WAV | 22050 | 3552 |
| 0x05 | 0x4db778 | podium2 | MENU/PODIUM2.WAV | 22050 | 4502 |

**divers (0x4044c0, race)** table 0x4db810, 18 records, folder `Divers\`

| id | rec addr | name | 16-bit file | rate | ms |
|---|---|---|---|---|---|
| 0x01 | 0x4db810 | bip1 | DIVERS/BIP1.WAV | 8000 | 582 |
| 0x02 | 0x4db818 | bip2 | DIVERS/BIP2.WAV | 8000 | 1360 |
| 0x06 | 0x4db820 | chckpvox | DIVERS/CHCKPVOX.WAV | 8000 | 628 |
| 0x03 | 0x4db828 | continue | DIVERS/CONTINUE.WAV | 5000 | 592 |
| 0x28 | 0x4db830 | deplett | DIVERS/DEPLETT.WAV | 16000 | 671 |
| 0x29 | 0x4db838 | select | DIVERS/SELECT.WAV | 22050 | 436 |
| 0x2e | 0x4db840 | pause | DIVERS/PAUSE.WAV | 11025 | 1417 |
| 0x0a | 0x4db848 | derap | DIVERS/DERAP.WAV | 16000 | 496 |
| 0x1a | 0x4db850 | brake1 | DIVERS/BRAKE1.WAV | 16000 | 408 |
| 0x1b | 0x4db858 | brake2 | DIVERS/BRAKE2.WAV | 16000 | 620 |
| 0x12 | 0x4db860 | crash | DIVERS/CRASH.WAV | 16000 | 870 |
| 0x11 | 0x4db868 | rouleman | DIVERS/ROULEMAN.WAV | 11025 | 1987 |
| 0x14 | 0x4db870 | wterdeb | DIVERS/WTERDEB.WAV | 8000 | 1064 |
| 0x15 | 0x4db878 | wterpas | DIVERS/WTERPAS.WAV | 8000 | 944 |
| 0x16 | 0x4db880 | wterfin | DIVERS/WTERFIN.WAV | 8000 | 1130 |
| 0x17 | 0x4db888 | gravier | DIVERS/GRAVIER.WAV | 16000 | 1762 |
| 0x18 | 0x4db890 | glace | DIVERS/GLACE.WAV | 16000 | 1306 |
| 0x19 | 0x4db898 | herbe | DIVERS/HERBE.WAV | 22050 | 896 |

**ktm (0x4044c0 arg != 0)** table 0x4db8a0, 16 records, folder `Ktm\`

| id | rec addr | name | 16-bit file | rate | ms |
|---|---|---|---|---|---|
| 0x21 | 0x4db8a0 | motostp | KTM/MOTOSTP.WAV | 16000 | 2532 |
| 0x26 | 0x4db8a8 | Gear5 | KTM/GEAR5.WAV | 16000 | 1517 |
| 0x25 | 0x4db8b0 | Gear4 | KTM/GEAR4.WAV | 16000 | 1513 |
| 0x24 | 0x4db8b8 | Gear3 | KTM/GEAR3.WAV | 16000 | 1412 |
| 0x23 | 0x4db8c0 | Gear2 | KTM/GEAR2.WAV | 16000 | 1607 |
| 0x22 | 0x4db8c8 | Gear1 | KTM/GEAR1.WAV | 16000 | 1122 |
| 0x1d | 0x4db8d0 | depart1 | KTM/DEPART1.WAV | 16000 | 710 |
| 0x1e | 0x4db8d8 | depart2 | KTM/DEPART2.WAV | 16000 | 945 |
| 0x1f | 0x4db8e0 | depart3 | KTM/DEPART3.WAV | 16000 | 692 |
| 0x1c | 0x4db8e8 | grince | KTM/GRINCE.WAV | 16000 | 771 |
| 0x0e | 0x4db8f0 | choc1 | KTM/CHOC1.WAV | 16000 | 480 |
| 0x0f | 0x4db8f8 | choc2 | KTM/CHOC2.WAV | 16000 | 577 |
| 0x0b | 0x4db900 | land1 | KTM/LAND1.WAV | 16000 | 349 |
| 0x0c | 0x4db908 | land2 | KTM/LAND2.WAV | 19000 | 343 |
| 0x0d | 0x4db910 | land3 | KTM/LAND3.WAV | 16000 | 456 |
| 0x13 | 0x4db918 | wheel | KTM/WHEEL.WAV | 16000 | 368 |

**ninja (0x4044c0 arg == 0)** table 0x4db920, 11 records, folder `Ninja\`

| id | rec addr | name | 16-bit file | rate | ms |
|---|---|---|---|---|---|
| 0x21 | 0x4db920 | motostp | NINJA/MOTOSTP.WAV | 16000 | 2460 |
| 0x26 | 0x4db928 | Gear5 | NINJA/GEAR5.WAV | 14000 | 9751 |
| 0x22 | 0x4db930 | Gear1 | NINJA/GEAR1.WAV | 22050 | 666 |
| 0x1d | 0x4db938 | depart1 | NINJA/DEPART1.WAV | 16000 | 312 |
| 0x1e | 0x4db940 | depart2 | NINJA/DEPART2.WAV | 16000 | 614 |
| 0x1f | 0x4db948 | depart3 | NINJA/DEPART3.WAV | 16000 | 687 |
| 0x1c | 0x4db950 | suspens1 | NINJA/SUSPENS1.WAV | 16000 | 608 |
| 0x0e | 0x4db958 | choc1 | NINJA/CHOC1.WAV | 16000 | 430 |
| 0x0f | 0x4db960 | choc2 | NINJA/CHOC2.WAV | 16000 | 661 |
| 0x0b | 0x4db968 | land | NINJA/LAND.WAV | 16000 | 390 |
| 0x13 | 0x4db970 | wheel | NINJA/WHEEL.WAV | 14000 | 290 |

**voices (0x4044c0 if voices on)** table 0x4db680, 30 records, folder `Voices\`

| id | rec addr | name | 16-bit file | rate | ms |
|---|---|---|---|---|---|
| 0x30 | 0x4db680 | 3 | VOICES/3.WAV | 11025 | 917 |
| 0x31 | 0x4db688 | 2 | VOICES/2.WAV | 11025 | 896 |
| 0x32 | 0x4db690 | 1 | VOICES/1.WAV | 11025 | 914 |
| 0x33 | 0x4db698 | go | VOICES/GO.WAV | 11025 | 895 |
| 0x34 | 0x4db6a0 | ckckpnt | VOICES/CKCKPNT.WAV | 11025 | 973 |
| 0x37 | 0x4db6a8 | finlap | VOICES/FINLAP.WAV | 11025 | 1147 |
| 0x38 | 0x4db6b0 | fuel | VOICES/FUEL.WAV | 11025 | 1067 |
| 0x39 | 0x4db6b8 | gameover | VOICES/GAMEOVER.WAV | 11025 | 1087 |
| 0x3a | 0x4db6c0 | lechef | VOICES/LECHEF.WAV | 11025 | 4250 |
| 0x3b | 0x4db6c8 | newrecor | VOICES/NEWRECOR.WAV | 11025 | 1750 |
| 0x3c | 0x4db6d0 | onegear | VOICES/ONEGEAR.WAV | 11025 | 2354 |
| 0x3d | 0x4db6d8 | oouups | VOICES/OOUUPS.WAV | 11025 | 654 |
| 0x3e | 0x4db6e0 | racecmpl | VOICES/RACECMPL.WAV | 11025 | 1316 |
| 0x3f | 0x4db6e8 | runlast | VOICES/RUNLAST.WAV | 11025 | 2798 |
| 0x40 | 0x4db6f0 | running4 | VOICES/RUNNING4.WAV | 11025 | 1853 |
| 0x41 | 0x4db6f8 | stillrun | VOICES/STILLRUN.WAV | 11025 | 1914 |
| 0x42 | 0x4db700 | timeout | VOICES/TIMEOUT.WAV | 11025 | 865 |
| 0x44 | 0x4db708 | tryagain | VOICES/TRYAGAIN.WAV | 11025 | 1105 |
| 0x45 | 0x4db710 | watarokt | VOICES/WATAROKT.WAV | 11025 | 1754 |
| 0x46 | 0x4db718 | wrongway | VOICES/WRONGWAY.WAV | 11025 | 1058 |
| 0x47 | 0x4db720 | youlost | VOICES/YOULOST.WAV | 11025 | 1976 |
| 0x48 | 0x4db728 | yuralosr | VOICES/YURALOSR.WAV | 11025 | 1701 |
| 0x49 | 0x4db730 | yurdalst | VOICES/YURDALST.WAV | 11025 | 1752 |
| 0x4a | 0x4db738 | opopadow | VOICES/OPOPADOW.WAV | 11025 | 2378 |
| 0x4b | 0x4db740 | ouch | VOICES/OUCH.WAV | 11025 | 796 |
| 0x4c | 0x4db748 | yeehaaa | VOICES/YEEHAAA.WAV | 11025 | 2689 |
| 0x4e | 0x4db750 | yeees | VOICES/YEEES.WAV | 11025 | 1465 |
| 0x4f | 0x4db758 | yeaah | VOICES/YEAAH.WAV | 11025 | 1161 |
| 0x50 | 0x4db760 | WellDone | VOICES/WELLDONE.WAV | 11025 | 1147 |
| 0x51 | 0x4db768 | wouaou | VOICES/WOUAOU.WAV | 11025 | 858 |

Files in the bank that no table names: `VOICES\BESTLAP`, `VOICES\MENU\CHSEMSTR`, and most of
DIVERS (AIGLE1-3, AIRPLANE, BERCY*, BIRD1-3, CORSAIRE, FETE, FOULE, GODZILLA, HELICO, HURRY,
METRO, MOUET-1..3, RESSAC, SABLE, SAUCER, TERRE, VENT): the latter are the tracks' ambient
sounds, loaded by name from each track's Sound.Spt (§6.2). No id 0x35, 0x43, 0x4d. No caller
of 0x52 (slctrans) was found (unused?) [asm: not among the 21 calls of 0x4049f0, 4 of 0x404e10,
10 of 0x404da0].

For the port: the tables are in game.EXE (.data, initialised): read them at the addresses
above like the other tables (name pointer → string in .data at 0x4db9xx..). The folder
strings: 0x4db9b8 `data\sfx\`, 0x4db9ac `16_Bits\`, 0x4db9a4 `8_Bits\`, 0x4db9d4 `Menu\`,
0x4db9c4 `Voices\Menu\`, 0x4db9e4 `Divers\`, 0x4db9ec `Ktm\`, 0x4db9f4 `Ninja\`,
0x4db9dc `Voices\`, 0x4db99c `.wav`.

#### 4.10.3 The DirectSound layer ("CfxServer", fx\*.cpp; object 0x103c bytes)

Wrapper object at `[0x516aa0]`: +0 → CfxServer, +4 = "server alive". Thin methods
(all skip a handle of −1 or a dead server) [decomp, simple]:

| wrapper | server | what |
|---|---|---|
| 0x423eb0(hwnd) | 0x441050 ctor | create (new 0x103c) |
| 0x423f50 | 0x440f30 | destroy |
| 0x423f80(path) | 0x4415a0 | load → handle |
| 0x423fb0(h) | 0x441960 | free (h and its copies) |
| 0x423fd0(h, flags) | 0x441a40 | play → handle of the copy used |
| 0x424000(h) | 0x441ca0 | stop |
| 0x424020 | 0x441cf0 | stop everything |
| 0x424030(h, vol) | 0x441d50 | volume 0..255 |
| 0x424050(h, hz) | 0x441dc0 | frequency |
| 0x424070(h, pan) | 0x441e10 | pan −128..128 |
| 0x424090(on) | 0x441e80 | pause all (1) / resume (0) |
| 0x4240b0 | 0x441f90 | "paused?" |
| 0x4240d0(h) | 0x441fa0 | length in ms |

##### 4.10.3.1 Creation 0x441050 [decomp; numbers asm-checked]
DirectSoundCreate; SetCooperativeLevel PRIORITY (2), falling back to NORMAL (1) on
DSERR_... 0x88780000a (log "Warning normal cooperative level used"). GetCaps: +0xc = caps
PRIMARY16BIT, +0x14 = PRIMARYSTEREO, +0x18 = !CERTIFIED?(0x20 bit), +0x10 = has hw mixing.
Primary buffer (DSBCAPS_PRIMARYBUFFER), and with PRIORITY: SetFormat **22050 Hz (0x5622),
stereo if the card can, 16-bit if the card can**. Then 0x4413b0 reads the actual primary
format back: +0x24 rate, +0x28 bits, +0x2c stereo. `-NoSound` (0x4dc1a8) disables it all: the effects and voices. **Not the music**, which is
MCI's, nor the mixer: run under wine, the race still gave rms 2200 to 3650 (the music alone) and the volume keys still
worked; Ctrl+S and Ctrl+A say texts 134, 135 (0x408b30, 0x408f30). In the port `sound_running()` is 0 and the mixing runs on for
the music (rms 3249 against 5363 with the effects, the same picture).
At start 0x408720 then calls **0x4414c0(stereo=Stereo setting 0x503530, bits16=Wave16Bits,
rate=ReplayRate 0x504568)** = 22050 in both known configs. 0x409570 (options) toggles
quality: 22050 ↔ 11025 (0x2b11), 16/8-bit, stereo/mono.
For the port: a 44100/48000 Hz stereo float mix is fine; the original's software mixer ran at
22050 Hz (so its output had no content above 11 kHz) [guess on whether this is audible].

##### 4.10.3.2 Samples 0x4415a0 / 0x440af0 [decomp]
Slot table: CfxServer+0x30 = 1024 pointers to 0x30-byte sample objects; +0x1030 = high
water mark. A handle is a slot index. Load: new sample object, 0x440af0 creates a secondary
buffer: DSBUFFERDESC size 0x14, **flags = 0xe0 (CTRLFREQUENCY|CTRLPAN|CTRLVOLUME) | 2
(STATIC) if the card has more than 4 free hardware static buffers**, size = data size,
format from the file; Lock/copy/Unlock. Then **4 duplicates** (DuplicateSoundBuffer) are
made at once, chained through +4 ("next copy"), so every sample has 5 voices ready.
Sample object: +0x14 frequency (starts at the file's rate), +0x18 last play flags,
+0x1c IDirectSoundBuffer, +0x20 "has buffer", +0x24 "paused by pause-all", +0x28 "is a
duplicate" (shares data), +0xc data size, +0x10 WAVEFORMATEX copy.

##### 4.10.3.3 Play 0x441a40(h, flags) [decomp; flags path asm-checked at 0x440ec0]
- **flags & 2**: walk the chain h → next... for the first copy whose GetStatus has no
  PLAYING bit; if all play, make one more duplicate (no limit but the 1024 slots) and append
  it. That copy is played and its handle returned.
- otherwise: the head buffer itself is used (a sound already playing is cut and restarted).
- 0x440ec0(copy, flags): SetCurrentPosition(0); +0x18 = flags; **Play(0, 0, flags & 1 ?
  DSBPLAY_LOOPING : 0)** [asm].
Stop 0x441ca0: IDirectSoundBuffer::Stop, +0x24 = 0 (position not reset; the next play resets
it). Free 0x441960 releases the whole chain.

##### 4.10.3.4 Volume, pan, frequency, length [asm-checked decomp]
- 0x441d50(h, v): clamp v to 0..255; `SetVolume(((v*5 - 0x4fb) * 800) / 256)` (C division,
  truncation towards zero) = (v−255)×15.625 hundredths of dB: 255 → 0 dB, 238 (0xee) →
  −265 (−2.65 dB, ×0.737), 128 → −1984, 0 → −3984 (−39.8 dB). Gain = 10^(vol/2000).
- 0x441e10(h, p): clamp −128..128; `SetPan((p*4000)/128)` (trunc.) → ±4000 at the ends
  (±40 dB on the opposite channel). DirectSound pan: negative attenuates the right channel
  by |pan|, positive the left.
- 0x441dc0(h, hz): stores +0x14 = hz always, but calls SetFrequency only if
  1000 < hz < 44100 (0xac44).
- 0x441fa0(h): `ms = (size / (bits/8 * channels)) * 1000 / (+0x14)` (unsigned integer
  arithmetic, using the current frequency).
- 0x441e80(1) pause-all: every copy that is playing *and was started looping* is stopped
  and marked +0x24; 0x441e80(0) replays the marked ones (from position 0, as 0x440ec0 resets).
  Used by the race pause (0x4121e0 / 0x412299) and app (de)activation (0x409700 from
  0x431250).

##### 4.10.3.5 0x4049f0 play(id, flags) — the game's play call [asm]
```
if 0x2f <= id <= 0x5c:                         # voices
    if !voicesOn(0x4db668) or id == -1: return -1
    if (flags & 4) and lastVoice(0x502898) == id: return -1
    lastVoice = id; h = Play(handle[id], flags); Volume(h, 0x4db654 = 0xff); return h
else:                                           # effects
    if !effectsOn(0x4db664): return -1
    if (flags & 4) and lastEffect(0x502710) == id: return -1
    if id < 0 or id >= 256: log "Invalid sound handle", return -1
    lastEffect = id; h = Play(handle[id], flags); Volume(h, 0x4db658 = 0xee); return h
```
The flags go straight to the server (bit 0 loop, bit 1 overlap; bit 2 only matters here).
0x4db654/0x4db658 are initialised data (0xff, 0xee) and never written [asm grep]. Stop:
`0x404af0(h)` = 0x424000. Other race code calls 0x423fd0 directly with the same pattern.

##### 4.10.3.6 Queued and delayed sounds [asm]
Record (0x64 bytes): +0 mode, +4 idle, +8 state, +0xc current index, +0x10 count (write
index for the ring), +0x14 handle playing, +0x18 start time, +0x1c play time, +0x20 length of
the playing copy (ms), +0x24 ids[8], +0x44 delays[8] (ms). Clock = 0x50bbc8 (ms, the menu's
`now`).
- **8 records at 0x504240..0x504560** (ctor 0x404b10, reset 0x404d50 at every menu entry:
  idle, state 0, index 0, count 0; mode stays 0). `0x404e10(n, delay0, id0, delay1, id1, ...)`
  (cdecl, varargs) takes the first idle record, clears it and appends up to 8 (id, delay)
  pairs.
- **Voice queue record 0x5028c8** (mode 1 = sequential; idle; reset by 0x404d50/0x4084e0).
  `0x404da0(id, delay)` appends one pair to its ring of 8 (dropped if full) and makes it
  busy.
- **0x404b30 (update, one record)**, called for the 8 records + the queue by **0x404eb0**
  from Menu_Frame each frame (after the page's frame), and for the queue only by the race
  (0x405f30):
  - idle → nothing.
  - state 0: start = now − length(handle[ids[i]]); state 1.
  - state 1: if now − start ≥ delays[i], or mode ≠ 0: play ids[i] (voice/effect rules of
    0x4049f0, flags 0, no flag-4 test, volumes 0xff/0xee), +0x14 = handle, +0x1c = now,
    +0x20 = its length (0 if not played); state 2.
  - state 2: mode 1 waits until now − playTime > delays[i] + length; mode 0 goes on at
    once. Then i = (i+1) & 7; if i == count → idle, else state 0.
  So in mode 0 (the menu prompts) the delay counts from (state-0 time − the sound's own
  length): a 600 ms prompt longer than 600 ms plays on the second update after it was queued
  (≈ 2 frames). In mode 1 (race announcements) each voice starts `delay` ms after the
  previous one ended.

#### 4.10.4 The menu's sounds, call by call

All with flags 0 unless noted. "voice" = needs VoicesState; "effect" = needs SoundState.

| where | when | id → file | how |
|---|---|---|---|
| 0x4099a0 hover (every menu frame) | the selected button changes to a new one (mouse or arrow keys; not when it becomes −1) | 0x29 select (effect) | play [asm] |
| 0x40a460 WM_LBUTTONDOWN | a click on a button (mouse mode, selected != −1) | 0x03 continue (effect) | play [asm] |
| Menu_Frame 0x409d65 | a page starts with the glass transition (0x4dbd00 set by 0x40c4a0) | 0x2a valide (effect) | play [asm] |
| page 14 title start 0x4465f0 | after its first frame | 0x2d title (effect) | play; music (3, 2) [asm] |
| page 3 main start 0x4422c0 | — | 0x57 makurchc "make your choice" (voice) | 0x404e10(1, 600, 0x57); music (3, 3) [asm] |
| page 3 frame 0x442790 | Solo pressed | 0x55 playsolo (voice) | play once (0x53f554), wait 1000 ms [asm] |
| page 3 frame | Link pressed | 0x56 playlink (voice) | same [asm] |
| page 5/26 start 0x447250 | — | 0x54 selectmd "select mode" (voice) | 0x404e10(1, 600, 0x54); music (3, 3) [asm] |
| page 6 start 0x447a30 | unless 0x4dc094 == 2 | 0x53 selerace "select race" (voice) | 0x404e10(1, 600, 0x53); music (3, 3) [asm] |
| page 6 frame 0x448110 | left / right track arrow | 0x28 deplett (effect) | play [asm] |
| page 7 start 0x449100, page 8 start 0x44a930 | — | — | music (3, 3) [asm] |
| page 9 start 0x44cfd0 | if 0x5401c4 != 0 (new record) | bit 0 set: 0x5a cngratim, else 0x59 cngrecor (voice) | play [asm] |
| page 10 frame 0x44ed30 | bike left / right | 0x28 deplett (effect) | play [asm] |
| page 10 frame | Go (once, 0x53f554) | 0x2c goninja if bike type 0, else 0x2b goktm (effect), then 0x58 letsgo (voice) | presses held until now > 0x53f78c = now + length(letsgo) (or of goXXX if voices off) [asm] |
| page 15 start 0x487070 (call at 0x487a32) | — | 0x36 entrname (voice) | 0x404e10(1, 600, 0x36) [asm] |
| page 15 frame 0x487bb0 | each accepted key | 0x03 continue (effect) | play [asm] |
| page 24 podium start 0x452240 | player not in the first 3 | 0x5b yutrygan (voice) | play; music (11, 0); else music (12, 0) [asm] |
| page 24 frame 0x452f80 | — | podium1/podium2 crowd loops | 4 "sound objects" (§6.2) [decomp] |
| page 25 logo start 0x4537d0 | — | — | music (13, 0) [asm] |

Silent: Return/Space/hotkey presses (only the mouse click sounds), Escape, page changes
without the glass transition, the quit page. Race-set ids used by the in-race menus
(0x4354e0 0x28, 0x4358b0 0x44 tryagain after stopping everything, 0x436150 0x29, 0x436420
and 0x437b70 0x03) and the race pause 0x4121e0 (0x424090(1) pause-all, CD pause, then
**0x2e pause.wav with flags 1 = looping**, handle in 0x4dbe40; 0x412299 stops it and resumes).

#### 4.10.5 Music: CD audio through MCI

##### 4.10.5.1 The calls
- **0x404990(track, mode)** [asm]: if the CD object `[0x503528]` exists: 0x4b18e0(track)
  (choose), obj+0x1c = mode, and if CD audio is on (0x4db66c): 0x4b19f0(−1, mode) (play).
- **0x4049e0()** = 0x4b1720 stop (only caller: the logo's end routine 0x453bf0).
- 0x4096e0(p) / 0x409670(p): pause (1) / resume (0) through 0x4b1660 (MCI_PAUSE; resume =
  MCI_PLAY from the current position to the saved end, no notify). 0x409670 is called on app
  (de)activation (0x4329a5); 0x4096e0 by the race pause and 0x457bf0.
- 0x409410: toggle CD audio (0x4db66c; on: replay obj+0x14, or the level's AudioTrack if
  none). 0x4db66c starts as AudioState (0x504870) if the CD device opened.

##### 4.10.5.2 The CD object (0x503528; ctor 0x4b11f0, open 0x4b1460/0x4b13b0) [decomp, asm where noted]
+0 MCI device id, +4 timer id, +8 open, +0xc repeat, +0x10 MCI "to" (TMSF end of track),
+0x14 current track (−1 none), +0x18 cached mode (0x20c not ready, 0x20d stopped, 0x20e
playing, 0x211 paused, 0x212 door open), +0x1c play mode, +0x20 CD labelled `moto_rac`
found, +0x24 hwnd. Open: MCI_OPEN "cdaudio" on each CD-ROM drive (GetDriveType 5... in the
loop the test is type 3? [decomp says 3; not checked]), volume label compared with
`moto_rac` (8 chars). A **1000 ms Windows timer** (SetTimer, callback 0x4b1330 → 0x4b12e0)
is the only looping mechanism: MM_MCINOTIFY reaches 0x4b1340, which does nothing [asm].

- **0x4b18e0(track)** choose [asm]: −1 → nothing. Not ready / door open → nothing. Query
  MCI_STATUS_NUMBER_OF_TRACKS = n. **track 0 → Rand_Next(1) % n + 1** (0x458cd0 =
  Rand_Next stream 1); else track must be 1..n. Then MCI_CDA_STATUS_TYPE_TRACK: audio
  (0x440) → +0x14 = track; else +0x14 = −1 and error 0x128 (so a random pick of the data
  track 1 gives silence: 1 chance in 13).
- **0x4b19f0(track, mode)** play [asm]: +0x1c = mode; read status. Skip if not ready / door
  open. **Skip if mode is 1 or 3, the drive is playing, and (track == −1 or track ==
  current)** — "keep the music going across pages". Else choose(track) (−1 keeps +0x14),
  get the track's length (MSF), MCI_PLAY with NOTIFY|FROM|TO from (track,0:0:0) to
  (track, length) (TMSF), +0x18 = playing, **+0xc = 1 if mode is 2 or 3** (repeat).
- Timer 0x4b12e0 [asm]: if repeat && cached mode == playing && track != −1: status; if not
  ready / door open → cached mode = stopped; **if stopped → 0x4b19f0(−1, 0)**: the same track
  again, but with mode 0, which clears +0xc. So on a real CD a "repeating" track is played
  **twice** and then stops; a later page start with mode 3 finds it stopped and starts it
  again.
- Mode meanings: 0 once; 1 once, don't restart if playing; 2 repeat (once), always restart;
  3 repeat (once), don't restart if playing.

##### 4.10.5.3 GOG's winmm.dll (proxy, 73728 bytes, 2010) [asm of the DLL]
Its mciSendCommandA (0x100011f0) handles: MCI_PLAY → builds `"0%d.wav"` / `"%d.wav"` from
dwFrom (TMSF from = the track number) and calls **PlaySoundA(name, 0, 0x2000b =
SND_FILENAME|SND_LOOP|SND_NODEFAULT|SND_ASYNC)**; MCI_STOP (and similar) → PlaySoundA(NULL,
0, 1). MCI_STATUS: number of tracks → 13; track type → audio (0x440) for 2..13, other
(0x441) for the rest; mode → playing/stopped/paused/not ready from its own flags; length
items go to the real winmm. So under GOG **every music loops forever** and the timer never
sees "stopped"; the music volume is the wave volume.

**MCI_PAUSE does not reach the music under GOG** [asm of the DLL's jump table 0x10001511/0x10001531:
0x809 goes to 0x100014f2, the real winmm; PlaySound is not told; measured]. The game asks for it in
four places: the Pause key and Ctrl+P (0x431250, Solo only), F1 open and shut (0x43cf90, 0x43e040),
the file dialog 0x457bf0, and on losing the window's focus 0x409670. The pause menu 0x436420 does
not ask (0x424090 only).

**Losing the window's focus: WM_ACTIVATEAPP in 0x4329a5** [asm 0x43336b..0x433474; tested under wine 2026-10-31, another
wine program (notepad) taking the focus, memory reads, captures]. **The race does not stand still**: the picture counter went
on at about 30 a second with no focus, and the race clock with it (0x4e981c 442, 621, 711 at 0, 6, 9 s). The older remark that
the game only moves while it has the focus does not hold for the game's code (nothing in App_OnIdle or 0x47d660 tests it);
what makes it look so on a menu is the below.
- Lost, wParam 0 [0x509b98 is written and read by nobody else]: 0x409670(0): the music that plays is paused (0x4db678 = 1; under GOG
  not heard, above). **0x4e8940 = 0, 0x4f1824 = 0**. If 0x4e893c == 0: 0x470fc0, the player's keys let go (`race_release_keys`);
  0x4e8954 = 1 or 0, the FullScreen to come back to; and if on the full screen: FullScreen 0, 0x4e9ce4 = 1, **0x4e893c = 1**.
- **0x4f1824 = 0: no text (0x493d49, 0x493820) and no picture (0x43edf3, 0x43f0f0) is drawn**, the 3D is. Captured: the race goes
  on with its dials but no LAP TIME, TIME, messages; the port's picture is the same but for the palm's movement. A menu has
  nothing else, so it stands frozen: 0 pixels different between two captures 2 s apart. That was the "standing still".
- 0x4e893c = 1 or 0x4e8940 = 1: 0x49ad90 draws no 3D (only full-screen, not tested in the original: under the hidden display
  the game was run in a window).
- Back, wParam != 0: 0x409670(1); 0x4e893c = 0, **0x4e8940 = 1** (one picture without 3D); 0x49b1c0 at the next: 0x4e8940 = 0,
  0x4f1824 = 1; FullScreen from 0x4e8954, 0x4e8954 = -1. Memory read: 0x509b98, 0x4f1824, 0x4db678, 0x4e8954 all back after a click.
- WM_DESTROY puts FullScreen back from 0x4e8954 before saving (0x432fda): what is saved is what the player chose.
- The sounds: the game's code asks nothing of them. The buffers (flags 0xe2) have no GLOBALFOCUS, which on Windows means they are
  not heard while another program has the focus [DirectSound's documented rule, not measured]; **under wine they stay audible** (rms
  4554 with no focus and the music switched off, 2906 before).

The port (`main.c` app_focus, `video.c` video_focus, `sound.c` music_focus): all the above, with the music held
as for the Pause key (the game's request, not GOG's deafness) and the effects' mixing silent while another program has the focus
(the Windows rule, the one choice here not measured: one line to take out). `OMR_INPUT` `F:blur`, `F:focus` for tests. Picture compared with
the capture: all 2498 different pixels are the palm's and the gull's, none on the dashboard.

Measured (a run under wine, 2026-10-30, track 04 of Speed Bay, effects and voices off with Ctrl+S
and Ctrl+V, the sound recorded from the null sink and each 1.5 s found in 04.wav by correlation,
score 0.999..1.000): **the music runs straight through Pause, through F1 and through the pause
menu**, the file's position minus the time the same to a millisecond over 45 s, in two sessions.
The game did stand: two pictures 3 s apart under Pause are equal in the upper 200 rows, and again
under F1. So on GOG's copy the music never stops. The same asm in the 1997 CD's MOTO.EXE (`push 0x809`, 0x4a81f0) would pause a real
disc: read, not run.

**Port's choice, to be judged by Danik:** `music_pause` (0x4096e0) holds the music and goes on from
the same place, on the Pause key, Ctrl+P and F1: what the game's code asks and the CD did, **not
what GOG's copy does**. To sound as GOG's: an empty `music_pause`. Not written: the focus (0x409670):
the port has no focus handling. Checked in the port by a recording: Pause and F1 silence the music
(RMS 0 once the last voices end), and it comes back where it was.

##### 4.10.5.3a The CD's music in the port (2026-10-31)
`cdaudio.c`: on the CD's program (`exe_is_cd`) the tracks are read from the CD's image: `OMR_CD` (a .cue or its folder), else a .cue in the
game's folder, the one above it, or `../CD`. The .bin is the name the .cue gives, else the .cue's own name, else the only .bin there (the
image here is renamed: the .cue says MOTO_RACER.BIN). A track runs from its INDEX 01 to the next track's INDEX 01: track 2 comes to 4:12:62,
the length 0x442740's copy check wants. Without an image: `NN.wav` beside the files if any, with the CD's rules.
The rules are the disc's (asm above), not GOG's loop: a track at its end has stopped; the 1 s timer plays one with the repeat flag again in
mode 0 (twice in all), then it stays stopped until a page asks for it. In both builds 0x404990 chooses first and plays with -1, so modes 1 and 3 leave going whatever
plays, and the podium's track runs on into the menus [asm; GOG's mode status answers playing from its own flags]. Checked (`OMR_WAV`, main menu, 5 minutes): track 3 twice with a gap of
under a second and then silence from 281 s; 2 s from the middle found in GOG's 03.wav at score 0.99999.

##### 4.10.5.3b The CD's music as files of the player's own (Danik, 2026-11-04)
As the other hybrid ports take a disc's music (OpenScreamer, jfSW, YQuake2): track n as a file the player made from the disc,
`music/trackNN`, `track_NN`, `track NN` or `NN` (any case), `.ogg`, `.mp3` or `.wav`, in the game's folder's `music/`, else in
the game's folder itself. `music_open()` (`sound.c`) takes, in this order: GOG's `NN.wav` beside the files (44.1 kHz 16-bit stereo,
read raw as before), such a file, the CD's image. The decoders are OpenScreamer's (`port/src/third_party`: stb_vorbis 1.22,
dr_mp3 0.7.4, dr_wav 0.14.6, public domain or MIT-0); a file's frames are made two sides (mono doubled) and the mixer's
44.1 kHz by a line between two frames where its rate is another. The end of a track and the loop are the same as the raw
tracks' (`music_read()` 0, then the disc's stop or GOG's `music_rewind()`). On the MiSTer the music thread, on CPU1 with the second
CPU's rows (§4.25), goes from the game's nice -20 to -10: the rows come first, Main_MiSTer's polling at nice 0 after it
(`SCHED_IDLE` would have left it next to nothing beside that polling, which takes all of CPU1's idle time).
Checked (`OMR_WAV`, intro and title, 25 s): GOG's `NN.wav` and the image give the same bytes as before; Ogg q4 of the image's
tracks against the image, every second correlated 0.996..0.999 at the same loudness; MP3 at 48 kHz stereo and WAV at 22.05 kHz
1.000; a 2 s `13.ogg` loops under GOG RELEASE and plays once under 1997 CD.
On the MiSTer (640x240, list 0 track 4, 21600 pictures, the CD's copy with Ogg q4 of its tracks, `OMR_COPY`): first a 41 ms
picture at the race's end, its one frame never shown and one twice: `music_play()` opened track 12 on the game thread,
stb_vorbis building its decoder and the first half second decoded there (GOG's raw `12.wav` 23 ms at the same place). With a
music thread the open and every fill are its own now, 4096 frames per hold of `g_mus_lock`, the thread woken at once by
`music_play()` (a track it cannot open: `g_mus_failed`, which `music_timer()` takes as `music_play()` did); without one
(`OMR_WAV`) as before. Then, never shown / shown twice after the load: Ogg 1/1 and 2/2 (slowest 41, 31 and 28 ms, at no music
call: on the PC the race calls `music_play()` at its start and its end only), GOG's `NN.wav` 0/0 and 0/0 (slowest 25 ms);
the average the same to 0.1 ms, the audio ring never empty. Within the noise of 2026-11-02's 1/1, or the decoding on CPU1
now and then in the second drawer's way: not taken further.

##### 4.10.5.4 Track use (both the CD and the GOG files: 02..13.wav, 44.1 kHz stereo 16-bit)

| track | length | used by | mode |
|---|---|---|---|
| 1 | data | — (random pick → silence) | |
| 2 | 252.8 s | race ROCK_FOREST (ile03); also the copy check: 0x442740 wants track 2's length = MSF 0x3e0c04 = 4:12:62 = 252.83 s [asm + test] | 0 |
| 3 | 143.4 s | **menu**: title (mode 2), main 3, 5/26, 6, 7, 8 (mode 3) | 2 / 3 |
| 4 | 187.4 s | race SPEED_BAY ile01, ile10 | 0 |
| 5 | 185.9 s | race SNOW_RIDE ile05 | 0 |
| 6 | 263.2 s | race RED_CITY ile08 | 0 |
| 7 | 187.9 s | race WEST_WAY ile02 | 0 |
| 8 | 194.6 s | race LOST_RUINS ile04 | 0 |
| 9 | 207.9 s | race DIRT_ARENA ile09, ile11 | 0 |
| 10 | 190.7 s | race GREAT_WALL ile06 | 0 |
| 11 | 57.7 s | podium, player not in the top 3 (with voice 0x5b) | 0 |
| 12 | 159.5 s | podium, player in the top 3; also at the race's end (0x412a50 → 0x48aca0) | 0 |
| 13 | 20.3 s | Delphine logo (page 25); stopped by its end routine | 0 |

Race music: 0x47bf70 at race start (stops all sounds 0x424020, race sound init 0x4084e0,
then **0x404990(AudioTrack 0x4db67c, 0)**). AudioTrack is read from the level's LEVEL.BPT
(field "AudioTrack", 0x428b4c; missing → 0 = random) [asm]; values from the banks: ile01 4,
ile02 7, ile03 2, ile04 8, ile05 5, ile06 10, ile08 6, ile09 9, ile10 4, ile11 9 [test].

##### 4.10.5.5 Volume settings
MasterVolume / CDVolume / WaveVolume (CONFIG.BPT) go to the **Windows mixer** (object
0x5d2958, 0x470cf0(line 0/1/2, value)): value 0..32 (0x5d2dbc[line] = 32 steps) scaled
onto the mixer control's range with MulDiv; restored at exit (0x470970). In-game hotkeys
(0x409720 from 0x431250) and the options page 11 step them (0x470d90 up / 0x470e90 down) and
draw a gauge. CD config: 19/19/19; GOG config: Master 0, CD 32, Wave 32. For the port: a
music gain = CDVolume/32 and an effects gain = WaveVolume/32 (ignore Master, or treat it
as overall) [guess on what feels the same; the original changed system volumes].

#### 4.10.5b The race's sound measured against the original (2026-10-31)
Full report and how it was done: `build/notes/race_sound.md` (git-ignored; recordings under `build/agent_sound/`). The original under wine, recorded from a null sink, its commands logged inside the process, the port by `OMR_WAV`. A known tone through the same path came back at 0.00 dB, so wine's path is calibrated; what it hides is a real machine's primary format and hardware mixer.
**Equal within what can be heard:** everything together within 0.06 dB in 5 windows; music -21.50 against -21.58 dB, and against the files -0.002 dB; effects -8.54 against -8.61 dB, balance 0.00 dB; engines idle and accelerating within 0.3 dB; the followed rider's pitch commands equal (MOTOSTP 35 of 36 on KTM, the rest all); the others' up to where the riders part (V 2278/2287, F 2197/2275); 3, 2, 1, GO exact.
**Found and fixed:** (1) `queue_step` left the voice queue in state 2 when it went idle (0x404ce0..0x404ce7 also sets state 0), so the next voice queued was skipped: CHCKPVOX 2 of 11, no FINLAP, YOULOST, TIMEOUT or GAMEOVER. The wait is `<` too (`cmp eax,ecx; ja`), it was 1 ms late. (2) The position commentary 0x407420 was never written: `race_sound_commentary`. First look 96 s after the start, then every 60 s; riding and the laps checked in a full race, leading (running4, stillrun) by `OMR_AUTOPILOT=win`. (3) 0x4084e0 empties the queues at the race's start (0x4086b0..0x4086e7): no old voice after a restart.
**Not changed, Danik's call:** effects 1 to 2.5 dB lower at 1.6 to 5 kHz, voices -4.6 dB at 4-5 kHz: the port's mixer is linear (`sound.c`), wine's is sharper. Original minus port at 3.2-5 kHz: +1.47 dB linear, +0.60 Hermite, +0.17 8-tap. A 1997 Windows mixer was probably cruder than wine's [not measured], so matching wine may not be matching 1997.
**Open:** with the two programs held in step by the helper's lock, the computer riders part just after GO, a few tenths of a unit at first, median 3100 units by step 1000. Its lock didn't carry the frame-time smoothing 0x4dc150, which may be all it is; `tools/cpucheck.sh` passes. So voices tied to laps were compared by order and gaps, not on the same step. Also not measured: real Windows, latency, the port in real time.

#### 4.10.6 Settings and race sounds (for later)

##### 4.10.6.1 Switches [asm]
CONFIG.BPT order (0x42ae30..): ... AmbiantState 0x50354c, MotoState 0x5028a0,
SoundState 0x504208, VoicesState 0x50355c, AudioState 0x504870, Wave16Bits 0x4db670,
Stereo 0x503530, ReplayRate 0x504568, MasterVolume, CDVolume, WaveVolume (mixer).
GOG CONFIG.BPT: 0 0 1 1 1 1 1 22050 0 32 32; CD: 1 1 1 1 1 1 1 22050 19 19 19.
Live switches: **effects 0x4db664** (= SoundState at start), **voices 0x4db668** (=
VoicesState), **engine 0x4db65c**, **ambience 0x4db660**, **CD audio 0x4db66c** (=
AudioState if the CD opened). 0x408720 (start) clears AmbiantState/MotoState and both live
flags; 0x4084e0 (race sound init) sets engine and ambience to 1 when effects are on and both
are off — so AmbiantState/MotoState are effectively ignored and follow SoundState.
Toggles: 0x408b30 effects (also creates the server if there was none; sets engine = ambience
= effects), 0x408f30 voices (loads/unloads voice sets), 0x408900 engine, 0x408800
ambience, 0x409410 CD audio, 0x409570 quality. Messages via 0x4188b0 with strings
0x53b7c8.. (language).

##### 4.10.6.2 Sound objects: ambience and the podium (0x405030, 0x404ee0) [decomp, partial asm]
A "soundDesc" (0x50 bytes; from `data\ileNN\Sound.Spt` = SOUND.BPT in the track bank,
parsed by 0x42e850: `soundDesc id count name... "FLAGS" 15 numbers`, plus `ambiance n ids`
and `signal`; static ones for the podium at 0x4e9128/0x4e9178/0x4e91c8/0x4e9218):
+0 id, +4 first sample id, +8 number of samples (random pick), +0xc flags (LOOPED 1, PAN 2,
RANDPAN 4, VOL 8 = distance attenuation, FADEIN 0x10, FADEOUT 0x20, FADEFREQ 0x40,
INFPLAY 0x80, SINVOL 0x100, SINFREQ 0x200), +0x10/+0x14 start delay base/random ms,
+0x18/+0x1c play time base/random (also SINVOL period and depth), +0x20 fade-in ms, +0x24
fade-out ms, +0x28/+0x2c repeat count base/random, +0x30/+0x34 frequency base/random
(base < 1 → added to the sample's own rate), +0x38 FADEFREQ / SINFREQ depth, +0x3c volume,
+0x40 fixed pan, +0x44/+0x48 floats near/far distance.
Object (0x40 bytes) state machine (+0x2c): 0 wait delay → 1 play (LOOPED → flags 3, else
2), set frequency, volume 0 if FADEIN (state 2) else full (state 5), pan fixed or RANDPAN
(rand&0x7f)−64 → 2 fade-in: `vol = ftol(sin(min(v, t·v/fadeIn)/v · π/2) · v · att) >> 8`
→ 5 hold (until play time unless INFPLAY; SINVOL/SINFREQ: sin(t/period · 2π)) → 4 fade-out
(mirror of 2) → 3 stop; repeat−−; at 0 re-randomise (0x404ee0); back to 0. All gated by
effects 0x4db664.
Podium (page 24): objects 0x540080/0x5400c0 = podium1 (id 4) looped forever with fade-in
1000 ms, pans +20 and −20, the second at the file's rate − 1000 Hz (a stereo chorus);
0x540100/0x540140 = podium2 (id 5) the same after 7000 ms [decomp; field meanings above].

##### 4.10.6.2a The race's ambience, mapped from the asm (2026-10-26; in the port: race_sound.c)
All [asm] unless marked. GOG game.EXE 3.22. Clock = 0x50bbc8 (`g_now`, ms, unsigned).
Random = Rand_Next(4) 0x4b1160 (`rand_next(4)`, 0..0x7ffe, never negative so the
cdq/idiv remainders are plain C `%`). Sound server calls are the raw ones (0x423fd0 play
on `handle[id]` = 0x519ff8[id], 0x424000 stop, 0x424030 volume, 0x424050 freq,
0x424070 pan): no switch tests, no "last id" bookkeeping, no default volume.
x87 at 53-bit: every intermediate a double, every store to memory a float.

###### 0. Data

**soundDesc (0x4c bytes; array 0x502930, count 0x5028b0, max 32 — the 32nd add raises an error)**
| off | what (Sound.Spt order) |
|---|---|
| +0x00 | id (int) |
| +0x04 | first sample id (the first name's id from 0x4056c0) |
| +0x08 | number of names (random pick: first + rand%count) |
| +0x0c | flags: LOOPED 1, PAN 2, RANDPAN 4, VOL 8, FADEIN 0x10, FADEOUT 0x20, FADEFREQ 0x40, INFPLAY 0x80, SINVOL 0x100, SINFREQ 0x200 |
| +0x10/+0x14 | start delay base / random (ms) |
| +0x18/+0x1c | play time base / random (ms); SINVOL/SINFREQ: +0x18 = the period (ms), +0x1c = SINVOL depth |
| +0x20 | fade-in ms |
| +0x24 | fade-out ms |
| +0x28/+0x2c | repeat count base / random |
| +0x30/+0x34 | frequency base / random; base <= 0 → base + the sample file's own rate |
| +0x38 | FADEFREQ / SINFREQ depth (Hz) |
| +0x3c | volume 0..255 |
| +0x40 | pan (fixed, −128..128) |
| +0x44/+0x48 | float near / far, in metres (×128 → world units) |

Sound.Spt values seen (tools/bpt.py; ILE01..11): e.g. D30 bercy13/14/24
`LOOPED|FADEIN|FADEOUT` 10000 10000 2000 3000 3000 4000 1 0 0 3000 0 250 0 | 0.0 0.0;
D0 mouet-1..3 `PAN|VOL` ... −1500 3000 0 200 0 | 30.0 100.0; D5 airplane
`LOOPED|INFPLAY|VOL|PAN` ... 255 0 | 100.0 300.0; D35 vent `...|SINVOL|SINFREQ`
0 0 22000 60 3000 0 0 0 8400 0 4000 220 50. Commands present: `soundDesc`, `ambiance n id..`
(ILE02: 35 36; ILE09: 30..34), `signal SIG n id..` (e.g. `signal 8 2 1 1`, `signal 7 2 10 11`,
ILE10 `signal 10 1 15`). Note repeat count 0 (vent descs) → `--repeat == 0` never hits
before wrapping (only matters with FADEOUT/no INFPLAY; vent is INFPLAY).

**The set 0x503568 (`this` of 0x405bc0, 0x4059a0, 0x405790)**
| off | what |
|---|---|
| +0x000 | count (0x503568), at most 0x28 = 40 (only 0x405790 checks it) |
| +0x004 + i·0x10 | entry i: +0 kind (0 decor, 1 signal, 2 ambiance), +4 section (kind 0), +8 signal number 6..12 (kind 1), +0xc decor object pointer (kind 0) |
| +0x284 + i·0x40 | sound object i (= 0x5037ec + i·0x40) |
| +0xc6c + s·4 | section numbers: index s = signal number 6..12 → +0xc84 + (s−6)·4 (i.e. +0xc84 is the 7 "nearest section with zone bit k" filled by 0x40d3b0; +0xc6c is just the base for signal numbers 6..12) |
| +0xc84 | 7 ints, filled by 0x40d3b0 each frame (−1 if none) |

**Sound object (0x40 bytes) — written by 0x404ee0**
| off | what |
|---|---|
| +0x00 | desc pointer |
| +0x04 | handle playing (−1) |
| +0x08 | sample id chosen |
| +0x0c | volume = desc+0x3c × [0x5028c0] >> 8 (0x5028c0 = 0xff, set by 0x4084e0; flat image has 0) |
| +0x10 | state start time (ms) |
| +0x14 | delay (ms) |
| +0x18 | play time (ms) |
| +0x1c | fade-in ms (desc+0x20) |
| +0x20 | fade-out ms (desc+0x24) |
| +0x24 | repeats left |
| +0x28 | frequency (Hz) |
| +0x2c | state 0..5 |
| +0x30 | float far·128 |
| +0x34 | float 255 / ((far − near)·128) |
| +0x38 | float last distance |
| +0x3c | "stopped" (1 = no handle held) |

###### 1. Parsing: 0x405ea0 (called at the end of the track load 0x40d650) [asm]

```
0x503550 = detail (0x4dc098); 0x5028b0 = 0 (descs); 0x503538 = 0 (names); set.count = 0
Script s = open "%sSound.Spt" with the level's folder (0x4dc088 + level·0x130 + 0x24,
           e.g. "Data\ile01\") — the port: the same folder string land.c opens Level.Spt with
0x42e850(s)    — the parser; ends at the end of the file (the reader throws, caught)
0x405790(set)  — the decor entries (kind 0)
```
Before it, the loader built each section's **zone mask +0xf8** (0x40e0da..0x40e139):
```
mask = 0; for sec 0..nsec-1: e = sec.event(+0xec); if 6 <= e <= 12: mask ^= 1 << (e-6);
          sec.zone(+0xf8) = mask
```
(the port's Section has `event`; it has no zone field: compute it.)

**0x42e850 parser loop**
```
loop:
  str = script_str()                       (0x424e60)
  if str == "soundDesc":
     d.id = int; n = int; d.count = n
     for j < n: name = str; id = name_id(name) (0x4056c0); if j == 0: d.first = id
     flagstr = str; flags = 0
     for (i = 0; i < strlen(flagstr); i++)
        if (flagstr[i] == '|' or i == 0):
           p = flagstr + i (+1 if it is '|')
           strncmp(p, "LOOPED",6)==0 → |=1; "PAN",3 → 2; "RANDPAN",7 → 4; "VOL",3 → 8;
           "FADEIN",6 → 0x10; "FADEOUT",7 → 0x20; "FADEFREQ",8 → 0x40; "INFPLAY",7 → 0x80;
           "SINVOL",6 → 0x100; "SINFREQ",7 → 0x200; else log "Unknow flag ..." (tested in
           that order, first match wins)
     13 ints → +0x10 .. +0x40 (in order); 2 floats (0x424c00 = script_float) → +0x44, +0x48
     descs[0x5028b0++] = d; if 0x5028b0 == 32: error (fatal: 0x4b4db0 throw)
  elif str == "ambiance": n = int; for n: add_ambiance(int)       (0x406030)
  elif str == "signal":   sig = int; n = int; for n: if !add_signal(int id, sig): log  (0x4060a0)
  else log 'Unknow command "%s"' and go on
```
With script.h: `script_str`, `script_int`, `script_float`, and stop when `script_at_end(&s)`
(or `s.bad`) after a token — the original stops by the reader's exception at EOF.

**0x4056c0 name_id(name) — the ambient sample name table**
Table at 0x502718: {char *name, int id} ×48 max (names copied into 0x504570 + k·16),
count 0x503538. Look up by exact strcmp → its id; else append: k = count++, name copied,
id = **0x5c + count** (after the ++), i.e. 0x5d, 0x5e, … in order of first appearance.

**0x406030 add_ambiance(id)**
find the first desc with desc.id == id; if found: entry[count].kind = 2; obj[count].desc = d;
count++. (Not found: nothing, no message.) No count limit test.

**0x4060a0 add_signal(id, sig) → 0/1**
if sig < 6 or sig > 12: return 0. entry[count].+8 = sig (written even if no desc);
find desc id; if found: entry[count].kind = 1; obj[count].desc = d; count++. return 1.

**0x405790 the decor entries (kind 0) [asm]**
```
if nsec <= 0 return
for i in 0..nsec-1:
  if count == 40: error "…" (0x4dbacc / 0x4dbaa8) and stop
  for j in 0..sec[i].ndecor(+0xd4)-1:
     o = decor[(int16)sec[i].decor[j](+0xb4)]       (0x4dc0e4[], signed word index)
     sid = o+0x44                                   (the .trk decor record's last i32,
                                                     the port's Decor.anim; −1 none)
     if sid == -1: continue
     if count == 40: error, stop
     d = first desc with desc.id == sid
     if d: entry[count] = {kind 0, +4 section i, +0xc o}; obj[count].desc = d; count++
     else log "Warning:no sound description found for sound ID(%d) for object(%s)" (name o+0x5c)
```
A decor listed by several sections gets one entry per listing. Counts per track (decor
references with a sound id): ILE01 11, ILE02 4, ILE08 7, ILE09 2, ILE10 11 (id 12 has no
desc there: warnings), ILE11 12; others 0.

###### 2. Loading the samples: 0x4059a0(set) (from 0x4044c0 after Ktm/Ninja + Divers, before the voices; and 0x408f30) [asm]
```
for each object i < count: d = obj[i].desc
  for j < d.count:
     id = d.first + j
     k = index in the name table (0x50271c ids) with ids[k] == id, searched over all 48 slots
     if not found: k = 0x30 → log 0x4dbaf8; continue
     if handle[id] == -1:
        path = "data\sfx\" + (Wave16 ? "16_Bits\" : "8_Bits\") + "Divers\" + name[k] + ".wav"
        handle[id] = load(path); volume(handle[id], 0xff); pan(handle[id], 0)
```
(The port: same as sound.c's `load_table` but by name; names like "bercy13" → DIVERS/BERCY13.WAV.)

###### 3. Re-arm: 0x404ee0(obj) [asm]
```
d = obj.desc
obj.sample = d.first; if d.count: obj.sample += rand%d.count            (rand #1)
obj.h = -1; obj.stopped(+0x3c) = 1
f = d.freq(+0x30); if f <= 0: f += sample_rate(handle[obj.sample])   (0x442010: the
                                                 file's nSamplesPerSec; 0 if no handle)
obj.freq = f; if d.freq_rand(+0x34): obj.freq += rand % d.freq_rand              (rand #2)
obj.delay = d.delay(+0x10); if d.delay_rand: obj.delay += rand % d.delay_rand    (rand #3)
obj.play  = d.play(+0x18);  if d.play_rand:  obj.play  += rand % d.play_rand     (rand #4)
obj.repeat= d.rep(+0x28);   if d.rep_rand:   obj.repeat+= rand % d.rep_rand      (rand #5)
obj.fadein = d+0x20; obj.fadeout = d+0x24
obj.vol = (d.vol(+0x3c) * [0x5028c0]) >> 8          (int, arithmetic shift)
obj.start = now; obj.state = 0; obj.lastd(+0x38) = 0 (int 0 = 0.0f)
obj.far128 = (float)(d.far * 128.0)                 (double 0x4d0140 = 128.0)
obj.scale  = (float)(255.0 / ((d.far - d.near) * 128.0))   (double 0x4d0148 = 255.0;
             far − near in double from floats; far == near → inf, as the game)
```
Note: the old handle is NOT stopped here (callers stop first when needed).
Callers: 0x4084e0 race sound init (all i < count, after 0x5028c0 = 0xff), 0x405f30
(detail back from 0), 0x405030 state 3 (repeats used up), 0x408800/0x408b30 toggles,
podium 0x452f80.

###### 4. The state machine: 0x405030(obj, audible, x, y, z) [asm]
Listener = camera block +0x98 (0x516258 + cam·0x238; position) and +0xa4 (look-at), cam =
0x4dc0f8 (0 in the race). The port: camera_eye() and the target (+0xa4).
```
if !effects(0x4db664): return
d = obj.desc; att = 255; pan = d.pan(+0x40)
if obj.start > now: obj.start = now                     (unsigned compare)
t = now - obj.start                                     (unsigned)
if !audible:
   if !obj.stopped: stop(obj.h); obj.h = -1; obj.stopped = 1
   return
if d.flags & VOL(8):
   dx = (float)(x - cx); dy = (float)(y - cy); dz = (float)(z - cz)   (stored floats)
   D = sqrt((dy*dy + dz*dz) + dx*dx)                     (double)
   if D > obj.far128:                                    (fcom: strictly greater)
      if obj.lastd > obj.far128 && !obj.stopped: stop(obj.h); obj.h = -1; obj.stopped = 1
      obj.lastd = (float)D; return                       (so a sound stops on the 2nd
                                                          frame beyond far; nothing else
                                                          runs while beyond, timers too)
   att = ftol((obj.far128 - (float)D) * obj.scale)       (fld far128; fsub Dfloat; fmul
                                                          scale) clamp 0..255
   obj.lastd = (float)D
if (d.flags & LOOPED) && obj.state == 5 && obj.stopped:  (came back in range while holding)
   obj.h = -1; obj.stopped = 1
   if effects: obj.h = play(handle[obj.sample], (d.flags & 1) | 2); obj.stopped = 0
   (no freq/volume set here; state 5 below sets the volume)
d = obj.desc; flags = d.flags
if flags & PAN(2):
   Lx,Lz = look-at; Cx,Cz = camera
   inv1 = 1.0 / sqrt((Lz-Cz)^2 + (Lx-Cx)^2)               (double; dz² first then + dx²)
   fz = inv1*(Lz-Cz); fx = inv1*(Lx-Cx)                     (doubles, not stored)
   X = x - Cx (double);  Zf = (float)(z - Cz)
   inv2 = 1.0 / sqrt(X*X + (double)(z-Cz)*Zf)               (the 2nd factor is the float)
   Xn = X * inv2 (double);  Zn = (float)((float)inv2 * Zf)
   p = (float)(fz*Xn - fx*Zn)
   dead zone exactly as 0x4079e0: p >= 0.15f → p - 0.15f; bits(p) >= 0xbe19999a → p + 0.15f;
   else 0 (float 0x4d012c = 0.15, bits 3e19999a)
   pan = ftol(p * 75.29412f)                                (float 0x4d0138, bits 42969697)
switch obj.state:             (jump table 0x40569c; >5: return)
case 0:  if obj.delay < t: obj.state = 1                    return
case 1:  obj.h = -1; obj.stopped = 1                        (old handle not stopped)
         if effects: obj.h = play(handle[obj.sample], (flags & 1) | 2); obj.stopped = 0
         freq(obj.h, obj.freq); obj.start = now
         if d.flags & FADEIN: obj.state = 2; v = 0  else: v = obj.vol; obj.state = 5
         volume(obj.h, (v * att) >> 8)
         if d.flags & RANDPAN(4): pan = rand % 128 - 64     (rand: one call)
         pan(obj.h, pan)                                    return
case 2 (fade-in):
         vol = obj.vol
         x = (unsigned)(t * vol) / (unsigned)obj.fadein     (imul low 32 bits, unsigned div)
         if (int)x > vol: x = vol
         v = ftol((double)att * ((double)vol * sin((double)(int)x / (double)vol * (pi/2))))
             >> 8                                           (double 0x4d0158 = pi/2)
         volume(obj.h, v); pan(obj.h, pan)
         if d.flags & FADEFREQ(0x40): freq(obj.h, (unsigned)(d.ffreq(+0x38) * t) /
                                       (unsigned)obj.fadein + obj.freq)
         if obj.fadein < t: obj.start = now; obj.state = 5   return
case 3 (stop):
         stop(obj.h); obj.h = -1; obj.stopped = 1
         if --obj.repeat == 0: rearm(obj)  (0x404ee0)
         obj.state = 0                                      return
case 4 (fade-out):
         fo = obj.fadeout; vol = obj.vol
         x = (unsigned)((fo - t) * vol) / (unsigned)fo      (unsigned division of the
                                                             32-bit product, as the asm)
         if (int)x < 0: x = 0
         v = ftol(att * (vol * sin((int)x / (double)vol * pi/2))) >> 8
         volume(obj.h, v); pan(obj.h, pan)
         if d.flags & FADEFREQ: freq(obj.h, (unsigned)((fo - t) * d.ffreq) / (unsigned)fo
                                     + obj.freq)
         if fo < t: obj.start = now; obj.state = 3          return
case 5 (hold):
         v = obj.vol; flags = d.flags
         if !(flags & INFPLAY(0x80)) && obj.play < t:
            obj.state = (flags & FADEOUT(0x20)) ? 4 : 3; obj.start = now
         if flags & SINVOL(0x100):
            v = (ftol(sin((double)t / (double)d.play(+0x18) * 2pi)
                      * (double)(d.play_rand(+0x1c) * [0x5028c0])) >> 8) + obj.vol
                                                            (double 0x4d0160 = 2pi;
                                                             t as unsigned 64-bit fild)
         if flags & SINFREQ(0x200):
            freq(obj.h, ftol(sin((double)t / d.play * 2pi) * (double)d.ffreq(+0x38)
                             + (double)obj.freq))
         volume(obj.h, (v * att) >> 8); pan(obj.h, pan)     return
```
Notes: t, delay, play, fadein are compared unsigned (`jae`). In state 5, t keeps counting
from the fade-in's end (start = now at state 2's end), and the SIN* phase uses that t.
With fade-in 0 and FADEIN set, state 2 divides by 0 in the game (no desc does that).
In state 4 once t > fo the product wraps: one frame of a strange volume before state 3
(faithful copy: compute in uint32).

###### 5. Placing the set each frame: 0x405bc0(set) [asm]
```
set.near[0..6] (+0xc84) = -1
0x40d3b0(track, set.near, 0x28)                   (below)
for i < count: e = entry[i]
  kind 0: if detail(0x4dc098) == 0: skip (no call at all)
          aud = 0x40d350(track, e.section, 0x3c)  (below)
          o = e.decor; v = o.model_near(+0x4c)->vertices(+4)[0]   (the near library
              model's FIRST VERTEX; the port: g_land.lib[o->lib].m[0].v[0])
          a = ftol(o.ang[0](+0x14) * 2607.594587618272); ca = cos[a & 0x3fff];
              sa = cos[(a - 0x1000) & 0x3fff]                    (double 0x4d0168;
              table 0x5fcb80 = the port's g_cos)
          b from o.ang[1] (+0x18) → cb, sb;  c from o.ang[2] (+0x1c) → cc, sc
          k = (float)(sc*sb)
          x = (float)((( -(sc*cb)*v1 + (cc*cb)*v0) + v2*sb) + px)
          y = (float)(((((cc*ca) - k*sa)*v1 + ((sc*ca) + sa*(sb*cc))*v0) + (-(cb*sa))*v2) + py)
          z = (float)((((k*ca + cc*sa)*v1 + ((sc*sa) - ca*(sb*cc))*v0) + (cb*ca)*v2) + pz)
              (p = o.pos +0x20/+0x24/+0x28; products of floats in double, summed in the
               order shown; traced with a symbolic x87 run)
          0x405030(obj[i], aud, x, y, z)
  kind 1: s = set.near-base[e.signal]  (= +0xc84 + (signal-6)·4)
          if s == -1: 0x405030(obj[i], 0, 0, 0, 0)
          else 0x405030(obj[i], 1, sec[s].pos x, y, z)
  kind 2: 0x405030(obj[i], 1, 0, 0, 0)
  other: nothing
```

**0x40d350(track, s, range) → visible [asm]**
track +4 = nsec n, **track +0x20 = the camera's section**, written by the drawing's
0x40ce60 (its argument: the section 0x40c7e0 found for the camera, 0 when the track has no
blocks). Sounds run in Game_Frame before the drawing (step 19 vs 22), so this is **the
previous picture's camera section** (the port: race_frame's `sec = land_section(eye, …)`
passed to land_draw).
```
lo = (n - range + c) % n;  hi = (c + range) % n          (C %, signed)
if hi < lo:  return (lo < s && s < n) || (s >= 0 && hi > s)   (wrapped)
else:        return lo < s && hi > s
```

**0x40d3b0(track, out[7], range) [asm]**
```
c = track+0x20; start = (n - range + c) % n; R = sec[c].pos (floats)
best[0..6] = FLT_MAX (0x7f7fffff)
for i in 0 .. 2*range-1:
   s = start + i; if s >= n: s -= n            (cmp n > s keeps it)
   dx,dy,dz = (float)(sec[s].pos - R)          (stored floats)
   d2 = (float)((dy*dy + dz*dz) + dx*dx)        (no sqrt)
   for k in 0..6: if (sec[s].zone & (1<<k)) && best[k] > d2: out[k] = s; best[k] = d2
```
So signal k+6's sound sits at the nearest section (within ±40 of the camera's section)
whose zone bit k is set; none → inaudible (stopped).

###### 6. The frame: 0x405f30 (Game_Frame step 19, after 0x4079e0) [asm]
```
if effects(0x4db664) && ambience(0x4db660) && !pause(0x4dc038):
   0x405bc0(set)
   if 0x503550 != detail(0x4dc098):
      if detail == 0: for i < count: if entry[i].kind == 0: stop(obj[i].h); obj[i].h = -1;
                                                           obj[i].stopped = 1
      elif 0x503550 == 0: for i < count: if entry[i].kind == 0: rearm(obj[i])
      0x503550 = detail
always: 0x404b30 on the 8 queue records 0x504240.. and the voice queue 0x5028c8
        (the port's sound_update)
```
0x503550 is set to the detail at the track load (0x405ea0).

###### 7. Race init 0x4084e0 (the ambience part) [asm]
`0x5028c0 = 0xff` then for i < count: rearm(obj at 0x5037ec + i·0x40). (Handles not
stopped: the race start stopped everything with 0x424020 first.) Then the switch rule:
if effects && !engine && !ambience: engine = ambience = 1.

###### 8. Podium (0x452f80, page 24) — for reference only
Four static objects 0x540080/0x5400c0/0x540100/0x540140 (descs 0x4e9128.. NOTES §4.10.6.2):
rearmed once (when 0x540cc8 is set), then each frame 0x405030(obj, 1, x, y, z) with the
same x,y,z locals (no VOL flag there, so the position does not matter).

###### 9. The port's mapping
- Section (land.h): pos (+0), axes (+0xc), decor[] (+0xb4, u16; read as int16), ndecor
  (+0xd4), event (+0xec). Missing: the zone mask +0xf8 (compute at load, §1), the T row
  +0x30..+0x38 (0 on every track).
- Decor (land.h): pos = +0x20, ang = +0x14, lib (index into g_land.lib), **anim = +0x44 =
  the sound id** (−1 none; track.py also calls it "anim"; it is what 0x405790 reads).
  The near model +0x4c = g_land.lib[lib].m[0]; its +4 vertex array = Model.v ([guess]
  the game does not move those vertices after the load: land.c keeps them raw too).
  Name +0x5c (lib name) only for the warning.
- Cosine table: r3d.h `g_cos[16384]` (0x5fcb80), index = ftol(angle·2607.594587618272).
- Camera: camera_eye() (+0x98) and the look-at +0xa4 (the camera's `target`); the track's
  current section: the section race_frame passes to land_draw (last picture's).
- Detail 0x4dc098 = config.h `g_detail`.
- Script: `script_open(&s, "<level folder>Sound.Spt")` (same folder string as Level.Spt in
  land.c), then the loop of §1 with script_str / script_int / script_float, stopping at
  `script_at_end`.
- Samples: names → DIVERS/<NAME>.WAV in SFX.BKF, ids from 0x5d in first-appearance order;
  sound.c needs a by-name loader (path as §2) and the sample's file rate for 0x442010
  (sound.c's Sample.rate).

In the port: Sound.Spt is read again at every race start (the game reads it at the track's load; only the detail-change memory 0x503550 differs), and the camera's section is the one found from the camera's eye after the previous step (the game's is the previous picture's).

##### 4.10.6.3 Engine (0x406290, per bike, from 0x4079e0 in the race) [asm for the formula, decomp
for the rest]
Gear sound id by gear (bike+0x7c): KTM table 0x4db638 {0, 0x22, 0x23, 0x24, 0x25, 0x26, 0x26},
Ninja 0x4db618 {0, 0x22, 0x22, 0x26, 0x26, 0x26, 0x26}; played looping (flags 3) on a gear
change, the old one stopped. Every frame
`hz = lo[g] + (hi[g] − lo[g]) · r` with (lo, hi) per gear from 0x4db5e0 (KTM: (9000,29000)
(1000,26000) (1000,26000) (1500,16000) (1000,16000) (1000,16000)) or 0x4db5a8 (Ninja:
(8000,48000) (10000,48000) (2000,40000) ×4), r = bike+0x11c / maxRpm(type) clamped
[0.05, 1]; then: sometimes + 2000 + rand%3000 (a misfire, 1 in 4 when flag bit 0), − a
0..3000 ramp (+0x4c, ±1000 a frame in state 3), + bike[+0x307d]·4000, − ftol((bike+0x1b8 −
bike+0x33c)/(X+0x38 · 48) · 1800), + obj+0x70, all × the pitch factor (arg 5, clamped
0.5..1.5) → SetFrequency (ignored above 44099 Hz). Volume = arg 2 · k >> 8, pan = arg 3.
Also skid/surface loops (ids 0x17 gravier, 0x18 glace, 0x19 herbe, 0x0a derap with set
frequencies 18000/20000/8000/5000), landings 0x0b..0x0d, brakes 0x1a/0x1b, start 0x1d..0x1f,
random "oouups"/"ouch" voices on crashes.

**Finished (2026-10-26, from the asm 0x406290..0x40734d; in the port: race_sound.c)** [asm]
Call: `0x406290(this = engine object, obj = rider record, vol, pan, own, f)` (thiscall,
ret 0x14). The rider fields (bikephys.h): speed +0x124, rpm +0x11c, gear +0x7c, keys of the
last step +0x2dc8, events +0x2dac, ground now/last +0x2db8/+0x2dbc, sliding +0x2da0,
wheelie +0x307d, crashed +0x30c1, driven +0x8, front/rear wheel points +0x1b4/+0x338,
section +0x31c1. Carac row 0x509278 + bike·0x124: +0x120 PersoAnim (≠ 0: dirt bike, KTM
tables; 0: Ninja), +0x74 the rpm's maximum (r = rpm / it), +0x48 RealSpeedMax, +0x38 (the
wheel-height divisor). Every sound goes through the server directly (0x423fd0 play on
0x519ff8[id], 0x424030/0x424070/0x424050): no switch test, no 0xee volume.

Engine object (0x78 bytes; own 0x5034b0, others 0x5032c0 + i·0x78): +0 state (1 gear loop
plays, 2 too slow), +4 "was above 1.389 m/s", +8 water started, +0xc skid started, handles
+0x10 skid, +0x14 surface, +0x18 wterpas, +0x1c wterdeb, +0x20 idle (motostp), +0x24/+0x28
gear loop, +0x2c start, +0x30 brake; +0x34 vol, +0x38 brake fading, +0x3c/+0x40 brake Hz /
volume, +0x44 skid volume, +0x48 last surface, +0x4c ramp, +0x50 skid Hz, +0x54 the
throttle-off volume, +0x58 last keys, +0x5c last r, +0x60 its time, +0x64 last gear, +0x68
last wheelie, +0x6c max wheelie, **+0x70 detune: 0, 100, 200, 300 Hz for the 4 others
(0x4084e0), 0 for the own**, +0x74 last crashed. 0x406190 stops the 9 handles.

**Surface** s = 0x465d20(rear point, 1): 0 when there is no track, else 0x40ec60 (lateral
= X·(p − pos), road +0xe4 if +0xf0 < it < +0xf4 else +0xe8) **+ 9** (the second argument);
s = 0 in the air; then 1→2, 9→6, 11→5, 12→4. The tracks' grounds 0..4 thus give 6 (tarmac:
derap), 10 (gravier), 5 (gravier), 4 (herbe), 13 (glace); 2 (water) and 3, 7, 8 are never
reached with the shipped tracks (ILE04's off-road value 26407168 gives no sound).

Order (all volumes `vol·k >> 8`, all pans = pan):
1. Back up after a crash (crashed 0, +0x74 set): stop all, reset. f clamped 0.5..1.5 by
   its bits. r = (float)(rpm / carac+0x74) clamped: > 1.0 (bits) → 1, < 0.05 → 0.05.
   own = 0 if not driven.
2. Events: 0x40 → rouleman 0x11 (k 0xcd); 0x2a → crash 0x12 (k 0xff if bit 2 else 200);
   0x14 and the local player → 1 in 4 voice oouups 0x3d (0x502898 = 0x3d, volume 0xff, if
   voices); 2 and the player → 1 in 4 ouch 0x4b; 0x15 → choc 0xe + rand%2, k =
   ftol(speed·7.2 + 170) ≤ 0xe1, Hz ftol(f·19050).
3. Landing (ground and not ground last): k = ftol(−vy·3 + 180) ≤ 255, vy = 0x48cf20 (the
   velocity's up part in the bike's frame); land 0x0b, dirt 0x0b + rand%3.
4. Sliding: once, by s: 3/5/8/10/12 gravier 18000, 4 herbe 8000, 6/7 derap 5000, 13 glace
   20000; volume starts 0x50, +30 a call to 200; Hz ftol((skidHz + speed·200)·f). Not
   sliding: −30 a call, stopped below 0x50.
5. base = ftol(speed·400 + 1050). s = 2 (water): wterdeb once + wterpas loop at
   ftol(base·f), k 255 (below 2.78 m/s ftol(speed·255·0.36)); leaving water: wterfin at
   base. **Road bikes only**: s 3/5/10 gravier, 4 herbe, a loop at ftol(speed·20 + 12000)
   (no f), k 200 (below 8.33 m/s ftol(speed·200·0.12)); stopped when s changes.
6. Wheelie ends on the ground: wheel 0x13, k = ftol(max(+0x6c, π/2)·121.594) + 0x40.
7. Idle below 8.0556 m/s (29 km/h): motostp loop, k = ftol((8.0556 − speed)·48.316) in
   0..255, Hz ftol(((detune + r·8000) + 16050)·f); below 1.3889 m/s the gear loop stops
   (state 2). Above 8.0556 the idle stops.
8. Coming to a stop (+0x38 and +4 and below 1.3889): 1 in 2 grince/suspens1 0x1c, k 230,
   Hz ftol((rand%4000 + 14000)·f).
9. Brake held on the ground above 8.33 m/s: by s 3/5/8/10 gravier, 4 herbe, 6/7 brake1/2
   (12050 Hz base, else 22050), 13 glace; Hz ftol(ftol(base + speed·4000/RealSpeedMax)·f),
   volume 0xe6; released (or r < 0.2): −50 a call, stopped below 0.
10. Own bike, driven, throttle just opened: Ninja depart 0x1f − min(2, ftol(speed/RSM·5)),
    KTM depart 0x1d + rand%3; k 200.
11. Gear loop (state ≠ 2, gear > 0): another sample only when the table's id changes.
    Hz = ftol((hi−lo)·r + lo); throttle just opened: 1 in 4 + rand%3000 + 2000; ramp
    (s == 3 only: +1000 to 3000, else −1000 to 0); x = ftol(wheelie·4000 + (Hz − ramp)),
    y = ftol((front.y − rear.y)/(carac+0x38·48)·1800); **SetFrequency(ftol((x − y +
    detune)·f))**. Volume k: 250 for computer riders; driven: 255 with the throttle or a
    wheelie, else a random 240 − rand%50 renewed every 50 ms (KTM 100 ms); s == 3: k +=
    rand%50 − 30 (≥ 0); below 5.556 m/s k = ftol((speed − 0.2778)·0.18947·k).
12. Kept: r, keys, gear, crashed, s, wheelie, max wheelie.
Random: Rand_Next(4) throughout. Constants (tools/peek.py): 0x4d0190 0.05, 0x4d01a0 7.2,
0x4d01a8 170, 0x4d01b0 19050f, 0x4d01b4 3f, 0x4d01b8 180f, 0x4d01bc 200f, 0x4d01c0 400f,
0x4d01c4 1050f, 0x4d01c8 2.7778, 0x4d01d0 255f, 0x4d01d8 0.36, 0x4d01e0 20f, 0x4d01e4
12000f, 0x4d01e8 8.3333, 0x4d01f0 0.12, 0x4d0158 π/2, 0x4d0200 121.594, 0x4d0208 8.0556,
0x4d0210 48.316, 0x4d0218 8000f, 0x4d021c 16050f, 0x4d0220 1.3889, 0x4d0228 4000f,
0x4d0230 0.2, 0x4d0238 5f, 0x4d023c 48f, 0x4d0240 1800f, 0x4d0248 5.5556, 0x4d0250 0.27778,
0x4d0258 0.18947.

The race start 0x4084e0 [asm]: 0x54299c = now + 36000, 0x542854 = −1, followed-last 0 (so
the first call resets the own object), the shout flags 0x5034a8/a0/a4, 0x50352c = 0,
0x5028ac = now; the own object and the 4 slots reset (detune i·100); previous camera =
camera; 0x5028c0 = 0xff (the ambience's volume scale); the ambience objects re-armed; last
voice/effect = −1; the queues reset; engine/ambience switches set if effects are on and
both are off.

Pause: the pause menu 0x436420 calls 0x424090(1) (looping copies stopped and marked) before
continue.wav; CONTINUE 0x437ae0, RESTART 0x437610, QUIT 0x437890 call 0x424090(0) (then
the last two stop everything).

**Checked against the original (2026-10-26)**: Speed Bay practice, the same bike, Up held
20 s, recorded under wine through a PulseAudio null sink (the music stream muted) and the
port's OMR_WAV with AudioState 0: the dominant pitch (90–1200 Hz, 0.2 s windows) follows the
original's through every shift to within ~1–2 % (median difference 5 Hz at ~400 Hz) and
the loudness to a few %, the same clipping share (0.45 %).

##### 4.10.6.4 Start countdown (0x4082f0) [decomp]
state 0: bip2 (2) + "go" (0x33); 1/2/3: bip1 (1) + "1"/"2"/"3" (0x32/0x31/0x30); once per
state change.

#### 4.10.7 Open questions
- 0x4b1460: the drive-type test (decomp shows 3 = fixed disk; the cdaudio device is opened
  per letter anyway) — irrelevant for the port.
- What GOG's proxy does on MCI_PAUSE/resume (stop + restart from 0, or nothing).
- Page 6's 0x4dc094 == 2 (which game mode skips "select race").
- slctrans (0x52) has no caller; chsemstr / bestlap have no table entry.
- 1997 CD MOTO.EXE: same scheme expected, tables at other addresses (not looked up).
- Whether the original sounded duller (22050 Hz primary buffer): could be checked by
  recording the original under wine (tools/orig.sh) — not done.

#### 4.10.8 What the port needs, in order
1. **sfx.c**: read SFX.BKF entries through files.c, decipher (`b − 4(i+8)` per block, three
   blocks), keep mono s16 + rate. Read the 7 id tables from game.EXE (addresses in §2; the
   CD build needs its own). Load the menu set on menu entry, podium set on page 24, race set
   at race load (§2).
2. **mixer (SDL audio callback)**: N voices, each {sample, pos (fixed point), step =
   rate/outRate, loop, gain = 10^((vol−255)·15.625/2000) as truncated hundredths, pan as
   DirectSound}. Per sample: a head voice + copies (5 at load, grow on demand). Lock around
   the game-thread calls.
3. **play(id, flags)** exactly as 0x4049f0 (voice/effect classes, last ids, volumes 0xff /
   0xee, flags 1/2/4), stop, stop-all, pause-all/resume, length(ms) as 0x441fa0.
4. **Queues**: the 8 records + voice queue, `queue_n(...)` = 0x404e10, `queue_voice` =
   0x404da0, update all from the menu frame after the page (0x404eb0) with the menu clock.
5. **Menu hooks** (§4): hover 0x29, click 0x03, transition 0x2a, title 0x2d, page start
   prompts via the queue, Solo/Link voices + the 1000 ms wait, page 6/10 arrows 0x28, Go.
6. **music.c**: stream `NN.wav` (or the CD image's tracks for the CD version) with
   0x404990 semantics (modes 0..3, skip-if-playing). Decide looping: GOG loops forever
   (what Danik hears today); the real CD plays repeat tracks twice. Pause on focus loss as
   0x409670.
7. Settings: SoundState / VoicesState / AudioState / CDVolume / WaveVolume from CONFIG.BPT.
8. Race: engine (§6.3), ambience objects (§6.2), countdown, voice queue (later).


#### 4.10.9 In the port (2026-10-26)
`sound.c`: the effects decoded as above (16-bit set), slots of 1024 buffers with 4 copies
each, play/stop/volume/pan/length as §4.10.3, 0x4049f0's classes, the 8 queues and the
voice queue stepped once a menu frame after the page; the menu set loaded with the menu's
resources, the queues reset at the menu's first entry. Mixed at 44100 Hz stereo with linear
interpolation (the original's primary buffer ran at 22050). Music as GOG's winmm.dll plays
it: NN.wav looping, read ahead half a second, 0x404990's modes 1/3 keep a track going.
Hooked: logo music 13 and its stop, title music (3, 2) + 0x2d, main menu (3, 3) + the
queued 0x57 when `Data\champA.spt` opens, hover 0x29, click 0x03, glass 0x2a, Solo/Link
voices. Not yet: CONFIG.BPT's switches and volumes (all on, full), pause on losing focus,
the CD version's tables and audio tracks, the race's sounds.
Checks (`OMR_WAV=FILE` with `--shot`: what the run sounds like, 735 or 736 frames of
sound a picture): a 30 s run from the logo equals 13.wav sample for sample for 19 s, and
03.wav starts at frame 1201 (the title's start); on page 3 "make your choice" (1061 ms)
starts on the second frame, as the queue's quirk says; the down arrow plays 0x29.

### 4.11 Page 15, the name entry: the 3D letter grid (mapped 2026-10-26)


Start 0x487070, frame 0x487bb0, type-a-letter 0x487a70, end 0x488790.
Confidence marks: [asm] read from the instructions, [px] confirmed against the running
original, [decomp] read from Ghidra's C only, [guess] not proven.
Reference renderer: `tools/model.py name ...` (see the end of this file).

Result: **model.py reproduces the settled page with 0 differing pixels in 4 captures**
(16-bit comparison, 640x480), and to within 30 and 8 edge pixels in the 2 s ripple-start
after the intro (sub-millisecond timing). See "Verification".


#### 4.11.1 Start 0x487070 [asm]

##### View
- `0x49a0e0(1024.0)` and `0x49a100(1024.0)`: argument bits **0x44800000**. The focal
  in x is 1024 × [0x6d1f74] (screen scale, 1.0 at 640x480), in y 1024. Elsewhere in
  the menu it is 1911 (0x44eee000); the confirm path puts 1911 back.
- Near plane 256, centre (320,240) unchanged (the menu's), no x flip.
- `0x5f675c = 0` (the "confirm now" flag, see §4).

##### Models (only when [0x4f1284] == 0; afterwards it is set to 1)
Each through `0x4794b0(obj, "…3df", "…map", 0, 0, -1)` → `0x420a50(model, 3df, map, -1, 0)`
then `0x420250` (recentre on the bounding box centre).

| Object | File | Stored at |
|---|---|---|
| letters A..Z | `data\menu\font3d\l%c.3df` / `.map` (%c = 'a'..'z') | 0x5f1880[0..25] |
| delete | `data\menu\font3d\backspac.3df/.map` | 0x5f18e8 (= index 26) |
| confirm | `data\menu\font3d\valid.3df/.map` | 0x5f18ec (= index 27) |
| frame | `data\menu\font3d\cadre.3df/.map` | 0x4f1288 |
| cursor | `data\menu\font3d\curseur.3df/.map` | 0x4f128c |

**Not loaded**: CADRE1..3, CURSEUR2, no texture pages (0x4794b0 with page argument 0).

- `0x420a50` is the same reader as `0x4216a0` (NOTES §4.5) with a last argument. With
  last argument 0: textured faces get draw kind 1/2 (0x15/0x27 with 0x200000), faces
  flagged 0x20000 are moved to the end, and the model is mirrored by `0x422070`
  (x sign + corner order reversed). **Every font3d face is MAP kind 0, flags 0**, so it
  becomes draw kind 0 = flat colour, colour = the u16 at **MAP + 2** of the face record
  (not +0x4e as for textured faces). The shading is baked into those colours
  (e.g. LI: 0x829f, 0x935f, 0x48d8, 0xe55f). [asm, px]
- vertices are file floats × 96, as elsewhere. [asm]

##### Scales [asm]
- Cadre: `0x47a960(cadre, 78.0)` (0x429c0000) — fit to 78.
- `s = 0x47a960(letter A, 62.0)` (0x42780000) — letter A fitted to 62, s returned.
- Letters 1..27 (B..Z, BACKSPAC, VALID): `0x47abd0(model, s)`.
- `0x47ad40` (mirror again, same as 0x422070) on all 28 letter models → they end the
  right way round. The cadre and curseur are **not** mirrored back.
- Curseur: `0x47abd0(curseur, s)`.

`0x47a960 fit(model, size)`: per axis min/max over the vertices; half = f32((max-min)·0.5),
0 becomes 1; s = min(size/half) over the three axes; c = f32((max+min)·-0.5)
(0x4d18c8 = -0.5); every vertex v = f32((v + c)·s). Returns s. `0x47abd0(model, s)` does
the same with s given. [asm]

##### Objects and their start state [asm]
Menu object (0x94 bytes, §3). Camera `0x47af50(0x504bc8, angles (0,0,0), pos (0,0,-2400))`
(-2400 = 0xc5160000) — the identity view apart from 1e-16 terms.

- Wave phase `0x5f6480`: `0x47a340(obj, 0, 0)`, then `0x47a820(obj, (2π,0,0), 1.0 s, mode 2)`
  (2π = 0x40c90fdb) — the phase turns once a second, unbounded.
- Wave amplitude `0x5f63e8`: `0x47a340` to zero (x of its angles is the amplitude).
- Selection spinner `0x5f1690` (static, **not** reset here):
  `0x47a820((0,2π,0), 1.0, 2)` — its yaw (0x5f1694) is the selected letter's yaw.
- Cadre: `0x47a820(cadre, (0,2π,0), 1.0, 2)` — spins once a second.
- Each letter i (0..27): random start
  angles (A·4π, B·4π, C·4π), position (A·2048, B·512+2250, C·1024+4096),
  A,B,C = r/32767·2-1 in that call order, r = `0x458cd0()` (= `0x4b1160(1)`, menu random
  stream 1, 0..0x7ffe). Then rotate to (0,0,0) in 2.0 s mode 1, move to grid[i] in
  2.0 s mode 1.
- Grid table 0x5f18f0[28] (x,y,z floats), cell i: c = i % 7, r = i / 7:
  x = f32(f32(c·(1/6))·1100 - 550), y = f32(f32((3-r)·(1/3))·500 - 250), z = 0.
  Constants: 0x4d1b18 = 1/6 (double), 0x4d1b20 = 1100, 0x4d1b28 = 550, 0x4d1b00 = 1/3,
  0x4d1b08 = 500, 0x4d1b10 = 250. So 7 columns × 4 rows, A..G top row, VALID (27)
  bottom-right, BACKSPAC (26) left of it.
- Name slots 0x5f6530[8] (new objects, `0x479470`): model = letter k's, position
  (0,800,500), random angles r·2π/32767·3 (0x4d1b50 = 3.0).
- Slot positions 0x5f6378[9]: x = f32(k·140 - 490) (0x4d1b30 = 140, 0x4d1b38 = 490),
  y = -325 (0xc3a28000), z = -600 (0xc4160000). k = 0..8; [8] is where the curseur
  would sit after the 8th letter.
- Cursor (selected cell) `0x5f647c = 0`; menu+0xc = 0; `0x5f6528 = 1` (intro running);
  `0x4dbfc8 = 0` (Alt state).
- **Prefill** from the stored name 0x5161b0: each char `& 0xdf`, non-letters become 'A',
  at most 8. Slot k gets that letter's model, position (0,800,500), angles 0. If the
  length > 0 the cursor = 27 (VALID).
- Curseur: position (0,800,500), `0x47a820((0,2π,0), 1.0, 2)`.
- `0x43fa40(picture 0x69 enternam, (0,0))`; `0x5f6524 = 0`, `0x5f652c = 1` (roles
  not traced [guess]: page-local flags); mouse mode menu+4 = 0.
- No buttons (`0x40bd40` with an empty list). No music change.
- Sound: `0x404e10(1, 0x258, 0x36)` — voice sample 0x36 ("enter your name") queued with a
  600 ms delay. [asm; delay semantics guess]


#### 4.11.2 Frame 0x487bb0 [asm, px]

1. `0x49b1c0(1)`, `0x49a160(0,0)`, menu+0x10 = -1.
2. Background: picture id 2 = `paralax3.lez`, mode 4 (tile 128x128): `0x43fab0(0,1,0,0)`
   scrolls it **one row a frame**, pixel (x,y) = tile[(scroll+y) & 0x7f][x & 0x7f].
   Then picture id 0x69 = `enternam.lez` (per language, MENUUS etc.), mode 1 (colour-key)
   at (0,0). [px]
3. Update `0x5f1690` (selection spinner).
4. **Intro** (`0x5f6528` == 1): for i in 0..27: if i == cursor: place the cadre at the
   letter's *previous-frame* position, update + draw the cadre; update letter i; if
   i == cursor its yaw = [0x5f1694]; draw letter i. After the loop the cadre is
   updated and drawn again(**). When letter A's rotation is finished (+0x24 == 0):
   - amplitude object → (120.0, 0, 0) (0x42f00000) in 2.0 s mode 0 (linear);
   - every slot → its slot position (1 s mode 1) and → angles 0 (1 s mode 1);
   - if len < 8: curseur → slot[len] (1 s mode 1);
   - `0x5f6528 = 0`.
   (**) draws the cadre a second time at the same place — same pixels, harmless.
5. **Steady** (`0x5f6528` == 0): update 0x5f6480 (phase) and 0x5f63e8 (amplitude).
   phase p = [0x5f6480 +0] at the start of each row, for r in 0..3, c in 0..6:
   - z = f32(cos(p)·amp), amp = [0x5f63f4]; written to grid[i].z;
   - letter i updated, then its z forced to grid z;
   - i == cursor: update cadre, cadre position = letter's, draw cadre,
     letter yaw = [0x5f1694]; else letter angles = (0,0,0);
   - draw letter i;
   - p += 0.5235987755982988 per column (π/6), and per row the row's start phase
     += 0.4487989505128276 (π/7), each step rounded to float. [asm, px]
   The cosine is the x87 fcos on the float phase (not the COS table). [asm]
6. The 8 slots: update and draw (only those with a model). Then the curseur.
7. `0x40c500`, `0x49ad90` (flush the s-buffer; when [0x504e74]==0 and not D3D),
   pointer `0x442260(0x5049d4)` only if [0x4dbff0] && [0x53c524] (never drawn in the
   windowed captures), `0x49b0e0` (present).
8. No buttons, no text list.

So the draw order is: background, title, (per cell: cadre if selected, letter), typed
letters in slot order, curseur. Each draw is s-buffered and flushed at 0x49ad90, so the
order only matters for equal depth.

##### How the typed name is shown
As 3D letters in the slots 0x5f6378[k] along the bottom (y -325, z -600, x -490..+490
step 140), each object's model being the typed letter's. Typing: the slot object takes the
letter's model *and* its current angles and grid position, then flies to slot[len] and
rotates to 0 in 1 s (mode 1 = sine ease). Deleting: the last slot flies to (0,800,500)
(above the top edge) in 1 s mode 1 while given random angles. The curseur (the small
spinning marker) moves to slot[len] in 0.2 s linear.


#### 4.11.3 The menu 3D object and its routines [asm]

##### Layout (0x94 bytes)
| Off | What |
|---|---|
| +0x00 | angles a,b,c (float) |
| +0x0c | position x,y,z |
| +0x18 | model pointer |
| +0x1c, +0x8c, +0x90 | buffers used only by the glass-shard transition (0x479630/0x479c10) |
| +0x20 | move active |
| +0x24 | rotation active |
| +0x28 | "restart spinner" flag (mode 2: start time reset) |
| +0x2c | rotation start angles |
| +0x38, +0x3c | 0 → draw through 0x49b7f0; else 0x49c110 |
| +0x40 | angle delta |
| +0x4c | rotation end time; +0x54 duration (ms); +0x58 start time; +0x5c mode |
| +0x60 | move start position; +0x6c delta; +0x78 end; +0x80 duration; +0x84 start; +0x88 mode |

##### Setters
- `0x47a340(obj, angles, pos)`: both, clears +0x20 and +0x24.
- `0x47a380(obj, angles)`: angles, clears +0x24.
- `0x47a3a0(obj, pos)`: position, clears +0x20.
- `0x47a820(obj, angles*, float seconds, mode)` / `0x47a8b0(obj, pos*, seconds, mode)`:
  start = current, duration = ftol(seconds × 1000.0) (0x4d18a8 = 1000.0 double),
  start time = timeGetTime, end = start + duration. Mode 2: delta = the given vector
  (a rate per duration); otherwise delta = f32(target - current). Sets the active flag.

##### Update 0x47a3d0 (now = timeGetTime)
- ratio = f32((min(now,end) - start) / f32(duration)).
- mode 0: linear, value = start + delta·ratio.
- mode 1: e = (sin(ratio·π - π/2) + 1)·0.5; constants 0x4d1880 (π), 0x4d1888 (π/2),
  0x4d1890 (1.0), 0x4d1898 (0.5), all doubles; value = start + delta·e.
- mode 2: value = start + delta·(now - start)/duration, **not** clamped; each angle then
  wrapped by subtracting 2π_f (0x4d18a0) while > 2π (negatives through the sign bit test).
  Never ends. With +0x28 set the start time is reset.
- Otherwise the animation ends when min(now,end) == end (flag cleared, value = target).
- Bug: the mode-2 **position** branch uses now - duration instead of now - start (not
  used on this page).
Each result is stored as float. Times are integer ms.

##### Draw 0x479eb0
M = Rz(c)·Ry(b)·Rx(a), the same formula and product order as 0x48c030 (page 25/14).
With the camera C (0x47af50: V = Rᵀ, Vt = -(R·p)): N = M·C, T = pos·C + Ct, rounded to
float at every stored partial (T = f32(cc·sb), T2 = f32(sc·sb) in the matrix).
Then `0x49b2c0(model, N, T, 0x1f)` → `0x49b7f0(…, 1)` (when +0x38 == 0) → faces into the
s-buffer → `0x49ad90` at the end of the frame. **The same path as 0x48c030 → 0x49b2c0 →
0x49b7f0 → 0x49ad90**; only the object/matrix building differs. 0x49b2c0 sets the x87 to
24-bit precision and restores it; outside it the game runs at 53-bit.

##### Flat faces (draw kind 0)
Kind table 0x4f1a70: kind 0 = {0, sub 0, flag 0, 0x4a3950}; with no sub-kind 0x4aef20
calls `0x4a3950(vertices, face)` directly.
- `0x4a3950`: colour through 0x402020 = the 5:5:5 form, i.e. the 565 colour with bit 5
  cleared (`colour & 0xFFDF`); fan of triangles, near clip as in 0x49e510; only
  triangles entirely left of / above the screen are rejected.
- `0x4a34f0`: flat triangle, no area/back-face test (back faces are culled earlier),
  13-bit fixed-point edges.
- `0x4a2f10`: span insert into the s-buffer free list (splits a node on reversed spans);
  `0x4a31d0` is the MMX copy used when [0x4dbcf4].
No textures, no lighting on this page.


#### 4.11.4 Input

##### Mouse (menu+4 set when the mouse moved) [asm, px]
- The 28 letters' (x, y, 0) are copied to 0x5f1728 and projected through the camera
  0x504bc8 (focal 1024).
- The cursor = the cell nearest to the pointer by squared pixel distance, strict `<`,
  so the lowest index wins a tie. **Clicking anywhere acts on the nearest cell** — there
  is no rectangle. If the cursor changed, 0x5f16b8 = 1 (spinner restart).
- [0x5083b8] == 2 (a level: set on button down, cleared on up — a press shorter than a
  frame is lost): letter → type it (0x487a70); 26 → delete; 27 → confirm if len > 0.
  Then 0x5083b8 = 0 and sound 3.

##### Keys (menu+0xc) [asm, px]
- Cleared first (ignored): 0x21-0x24, 0x60-0x6b, 0x6f-0x7b. 0x6c-0x6e are not cleared and
  reach the letter test as 'l','m','n' (numpad +, separator, - type L, M, N).
- 0x08 Backspace: delete (no sound, no spinner restart): cursor = 26; the last slot flies
  to (0,800,500) in 1 s mode 1, angles random r·2π/32767·3; len--; the curseur moves to
  slot[len] in 0.2 s (0x3e4ccccd) mode 0.
- 0x0d Return: if not Alt ([0x4dbfc8] & 1) and len > 0: confirm, with sound.
- 0x20 Space: on a letter type it (sound only if len < 8); on 26 delete (always sound);
  on 27 confirm if len > 0 (sound).
- Arrows (set 0x5f16b8 = 1): Left -1 (below 0 → 27); Up -7 (negative → +28);
  Right +1 (≥28 → 0); Down +7 (≥28 → -28).
- 0x41-0x5a, 0x61-0x7a: 0x487a70((k|0x20) - 0x61), no sound.
- Escape: nothing (no way back except confirm).
- If [0x5f675c] is set the frame goes straight to confirm.

##### 0x487a70 type(i) [asm, px]
If len < 8: slot[len].model = letter i's model; 0x47a340(slot, letter i's angles, grid[i]);
slot → slot pos[len] (1 s mode 1) and → angles 0 (1 s mode 1); name[len] = 'A'+i.
The grid letter is put at (0,800,500) and moves back to grid[i] in 1.5 s (0x3fc00000)
mode 1. len++; cursor = i (no spinner restart); curseur → slot[len] in 0.2 s mode 0.
Always: 0x5f675c = 0.
(i is 0..25 here; 26/27 are handled by the callers.)


#### 4.11.5 Exits [asm]

Confirm:
1. strcpy name → 0x5161b0 (the player name).
2. `_strcmpi` (0x4bf8f0) against the cheat table 0x4f1250, count [0x4f1248] = 3
   (entries: string pointer, variable pointer, value):

   | Name | Variable | Value | Effect |
   |---|---|---|---|
   | CESREVER | 0x4dc050 | 1 | "reverse" tracks; read by the main menu with 0x4dc040 |
   | CTEKCOP | 0x4dc054 | 1 | "pocket" bikes; with 0x4dc03c |
   | CDNALSI | 0x4dc058 | 1 | read by 0x447920 / 0x448110 / 0x44aee0 (track pages) |

3. Focal back to 1911 (0x44eee000) in x and y; `0x40c1e0`.
4. Cheat matched: `0x409eb0(15)` — page 15 again, no transition; the end routine frees
   everything and the start reloads, so the intro plays again, prefilled from 0x5161b0
   (now the cheat word).
   No match: `0x40c4a0` (capture the screen for the glass transition), `0x409eb0(3)`
   → main menu.

End 0x488790: frees the slot objects (model pointer cleared first so the shared letter
models are not freed twice), the 28 letters, the cadre, the curseur; [0x4f1284] = 0.


#### 4.11.6 Verification [px]

Captures in `build/ref/name15/` with `tools/orig.sh` (headless). Comparison by
`model.py name` against each capture, 640x480 16-bit; side-by-side pictures (original |
model | difference) in `build/png/models/name_vs_orig_*.png`.

| Capture | State | Fitted parameters | Differing px |
|---|---|---|---|
| c_idle | DSI typed, cursor 27 | t 904, scroll 76, cadre 905, curseur 899 | **0** |
| orig_p15_name | DSI, cursor 6 | t 957, t_sel 614, scroll 100 | **0** |
| d2_bs_settled | DS after Backspace, cursor 26 | t 496, cadre 497, curseur 491 | **0** |
| d5_q_settled | DSQ after Q, cursor 16 | t 652, t_sel 890, cadre 653, curseur 647, scroll 66 | **0** |
| b_intro_6 | ripple starting | t 990, t_end 986, scroll 65 | 30 (one column x=472) |
| b_intro_7 | ripple growing | t 1442, t_end 1438, scroll 123 | 8 |

The fitted times are consistent: the cadre spinner always started 1 ms before the wave
spinner and the curseur 5 ms after (they are started at those points of the start
routine); the end of the intro was seen 4 ms after the 2000 ms mark. The residues are
single edge pixels from sub-millisecond timing. The intro fly-in itself is random and was
not fitted (needs the random stream's state, see open questions).
Mouse: nearest-cell selection and click-anywhere confirmed (e1/e2/e3/e4). Keys, Backspace,
arrows, typing, the Q fly-in confirmed (d0..d5).


#### 4.11.7 Open questions
- Where 0x5161b0's first "DSI" comes from: bestscor.dat holds "DSI"; the load was not traced.
- Exact semantics of the delay argument of 0x404e10 (voice 0x36, 600).
- The state of random stream 1 at page entry (needed for an exactly equal intro; the port
  can use any seed — the fly-in is random by design).
- Why the pointer is not drawn in the windowed captures (0x4dbff0 / 0x53c524 = fullscreen?).


#### 4.11.8 What the port needs, in order
1. Focal 1024 on entry, 1911 back on exit (menu view setters already needed elsewhere).
2. Model loading with the "last argument 0" path: MAP kind 0 → flat face with colour at
   MAP+2; mirror; recentre; then `fit` / `scale` / mirror-back as in §1.
3. Flat face drawing (kind 0): 0x4a3950 → 0x4a34f0 → 0x4a2f10 into the existing s-buffer.
4. The menu object (0x94) with set/rotate-to/move-to/update (modes 0,1,2 with the float
   roundings of §3) and its draw (M·C, pos·C + Ct) into 0x49b2c0's path; camera 0x47af50.
5. Page 15 start, frame and end as §1/§2/§5, with the grid, wave and slot tables.
6. Input §4 (nearest-cell mouse, keys, type/delete) and the confirm with the cheat table
   (three flags) → page 3 through the glass transition, or page 15 again on a cheat.
7. Voice 0x36 with its delay; sound 3 on clicks/space.
8. The stored player name 0x5161b0 (load from bestscor.dat — to be traced).

#### 4.11.x model.py
`python3 tools/model.py name BKF MS out.png [sel=N] [tsel=MS] [name=ABC] [scroll=N]
[tcadre=MS] [tcur=MS] [tend=MS]` — MS = ms since the wave spinner started; tsel = since
the selection spinner (re)started; tend = since the intro ended (ripple amplitude and
name fly-in). Functions: `load_menu_model`, `fit`, `scale`, `mirror`,
`menu_object_matrix`, `menu_camera`, `menu_draw`, `spin`, `ease`, `grid_xy`, `slot_xyz`,
`wave`, `name_models`, `name_background`, `name`; the Renderer gained `focal=` and flat
faces (`face_flat`, `flat_triangle`, `flat_row`).


#### 4.11.9 In the port (2026-10-26)
`page_name.c` (the page), `obj3d.c` (the menu object 0x47a3d0 and its setters), in `r3d.c`
a settable focal length, flat faces (0x4a3950/0x4a34f0/0x4a2f10) and the menu object's
matrix, camera and projection, in `model.c` the loader with the last argument 0, the mirror
0x422070 with its cosine-table constants, fit and rescale. The first name is CONFIG.BPT's
DefName (0x42af0d reads it into 0x5161b0; `config.c`). The cheat table is read from
game.EXE; its three variables map to the port's flags by address.
Checks: against `tools/model.py name` (0 px from the original's captures), the port's
settled page with DSI (frame 301: t 5000 ms, scroll 45) and after Backspace + Q (DSQ, frame
501) differ only in the mouse pointer (179 px; the original's windowed captures have none).
The logo and title pages are unchanged by the renderer's additions (0 px). The chain
25 → 14 → 15 → glass → 3 runs; CTEKCOP restarts the page and the main menu then shows the
Pocket switch; a confirmed cheat word restarts the page again each time (as the code does);
the mouse picks and types the nearest cell. The fly-in is random (stream 1) and not compared.

### 4.12 The Solo pages: mode, track, bike, top 5, loading (mapped 2026-10-26)

Mapped 2026-10-26 from build/mr.asm (numbers) with build/decomp as a guide, and checked
against captures of the running original (build/ref/orig_p*_*.png). Confidence marks:
**[asm]** read in the machine code, **[decomp]** Ghidra's text only, **[guess]**, **[test]**
seen in the running original. Picture ids are those of the table 0x4e3e20 (NOTES §4.7);
`L` after a name = language folder (data\MenuUs\...), else data\menu\bitmap\.
Call shorthands (NOTES §4.4): 0x40bef0(menu; pic, hl, neighbours, hotkey, group, value,
varptr, blink, x, y, flags) = picture button; 0x43fa40(pic, pt, align) = place;
0x43f9a0(pic, target, seconds, kind, align) = slide; 0x43fab0(pic, dx, dy, 0, 0) = step
(tile scroll); 0x43ede0(pic, surface, mode, pct_h, pct_w) = draw; 0x40c4a0 = glass
capture; 0x409eb0 = ChangePage. Neighbour bytes: Left, Right, Down, Up (0xff none).

#### 4.12.1 The graph

Every 0x409eb0 call in the program was listed with its page argument (79 sites, plus the
three `push N; jmp` sites of page 11 at 0x4445a5/0x44497f/0x44498a) and the condition
that leads to it. Pages reached from page 3 by Solo (0x4dbffc = 1):

```
 3 main menu --Solo: voice 0x55, 1000 ms, 0x4dbffc=1, glass, CD check 0x442740--> 5
                                                   (CD check fails --> 26 = page 5's routines)
 5/26 SELECT MODE   Practice / Single / Championship --(0x4dc094 = 0/1/2, glass)--> 6
                    Back (Esc) --(glass)--> 3
 6 SELECT RACE /    Continue (Return; only if the track is available) --> 10
   CHAMPIONSHIP     Top 5 (modes 0/1) --> 9          Load (mode 2) --> 20
                    Back (Esc) --> 5, or 9 in a championship once page 9 was shown (0x4e90c8)
                    (all with the glass)
10 CHOOSE BIKE      Start --> 1 (Solo) [8 in Link]        Back (Esc) --> 6
 9 TOP 5            (from 6, and after a race) see §5          --> 6, 5, 3, 21
20 championship load --> 6          21 championship save --> 9
 1 loading          one frame: draws load.lez, tears the menu down, loads the race
                    (load fails --> 3)
   race end         event flags 0x4e8918 via 0x4587e0: 0x200 --> 9 (TOP 5), 0x100 --> 5,
                    0x400 --> 6, 0x800 --> 1 (restart), 0x8000000 --> 24 (podium);
                    championship: 0x4580f0 --> 9 (Solo) / 24, then unlock pages
                    17 (pocket), 16 (reverse), 18 (championship over); 16/17/18 --> 24 --> 9
```

Seen in the original [test]: 3 → 5 → 6 → 10 → 1 → race (Single Race, Snow Ride); race →
Escape → QUIT RACE → 9 (TOP 5 SNOW RIDE) → Continue → 6; 6 → Top 5 → 9; Practice on 6
(laps −/+ and the time-attack switch); Championship 5 → 6 (CHAMPIONSHIP, Continue/Load/
Back) → 10 → 1 → race → QUIT RACE → 9 (TOP 5 with Save / Abort / Next race) → Next race → 6.

Not on the Solo path (Link only, [asm/decomp]): 7 session list, 8 Link lobby
(smallmap.lez), 22 network wait; 2 network. 5 → 7 only when 0x4dbffc == 0; 9 → 7/8 and
10 → 8 only in Link; 0x449560 (→ 7/8) is called only by page 7 and by 0x444cd0, which is
dead code (no pointer to it anywhere in the image; it looks like an old CD-retry page:
0x442740 → 5/26, else 3/7/8).

**Options** (main menu button 2 → 11 with the glass): page 11 (start 0x443000, frame
0x443200) is a list of text buttons (bitmap font, NOTES §4.8; capture
build/ref/orig_p11_options.png): DIFFICULTY (button 0: 0x4dc060 cycles 0 EASY, 1 MEDIUM,
2 HARD), CONTROLLER (1: 0x4e90d0 cycles KEYBOARD/JOYPAD/JOYSTICK), "config" (2: → page
**12**, the controls, after setting 0x4dc11c = 1/4/2 from 0x4e90d0 and saving the config
0x4311c0), DETAIL LEVELS (3: 0x4dc098 0..4), OPPONENTS (4: 0x4dbfdc ^= 1, "24"/"12"), D3D
(5), VOICES/SOUNDS/MUSIC (6/7/8: 0x408f30, 0x408b30, 0x409410), SCREEN MODE (9:
0x504bc4 0..2), 16/9 MODE (10: 0x504e80 ^= 1), WAVE OUTPUT (11: 0x409570), MASTER/WAVE/CD
volume −/+ (12..17: 0x470e90/0x470d90 on 0x5d2958 channel 0/2/1), SESSIONS REFRESH RATE
−/+ (18/19: 0x4dc044 0..30), Accept (20: config saved, → **3**), Info (21: → **13**, 3D
card/system info), Data disk (22: → **23**, the data-disk track list; reloads list%02d.spt
via 0x428d60). 12 → 11 (button 0; buttons 1..7 set 0x53f790 = i−1, the key being
redefined), 13 → 11, 23 → 11 [asm for the push values, decomp for the cases]. The
difficulty and the opponents count are race settings (§7).

#### 4.12.2 Page 5 / 26: game mode (Practice / Single race / Championship)

Page table rows 5 (0x4dbb88) and 26 (0x4dbcd8): frame 0x447710, start 0x447250, no end
routine. 26 is the same page reached when the main menu's CD check 0x442740 fails; nothing
in the program tests the page number 26 [asm grep], so it looks and acts exactly like 5.
No texts (no 0x417d20), no 3D, and the frame does not even run the text list.

Calling convention reminders: 0x40bef0(menu; pic, hl, neighbours, hotkey, group, value,
varptr, blink, x, y, flags); 0x43fa40(pic, int pt[2], align) places; 0x43f9a0(pic, int
target[2], float seconds, kind, align) slides; 0x43fab0(pic, a1, a2, 0, 0) steps (for the
tile, a1/a2 = the scroll per frame); 0x43ede0(pic, surface, mode, pct_h, pct_w) draws.
Neighbour bytes: byte0 Left, byte1 Right, byte2 Down, byte3 Up (0xff none).

#### Start 0x447250 [asm]
Pictures (all loaded at menu start, ids from the table 0x4e3e20; `L` = language folder):

| id | name | var | use |
|----|------|-----|-----|
| 0x00 | paralax1.lez (128x128 tile, mode 4) | 0x504880 | background |
| 0x08 | selmode.lez L (640x81) | 0x5048a0 | title bar "SELECT MODE" |
| 0x09 / 0x0a | practic2.lez / practice.lez L (190x293) | 0x5048a4 / a8 | Practice normal / highlighted |
| 0x0b / 0x0c | single2.lez / single.lez L | 0x5048ac / b0 | Single race |
| 0x0d / 0x0e | champio2.lez / champion.lez L | 0x5048b4 / b8 | Championship |
| 0x0f / 0x10 | Grdprix2.lez / Grdprix.lez L | 0x5048bc / c0 | instead of 0x0d/0x0e in the GP edition (0x4dc1c4 != 0) |
| 0x51 | Btback.lez L (121x40) | 0x5049c4 | Back |

Order:
1. Place practic2 (-242,115), single2 (229,595), champio2|Grdprix2 (700,115),
   Btback (270,430). All align 0.
2. 0x40bd40 (clear buttons).
3. **Solo (0x4dbffc != 0)**:
   - button 0: Btback, no hl, nb 0x02ffffff (Up -> 2), hotkey **Escape (0x1b)**, group -1,
     value 0, var 0, blink 0, x,y -1, flags 0.
   - slides, 1.0 s (0x3f800000), kind 1 (eased), align 0: practic2 -> (18,115), single2 ->
     (229,115), champio2|Grdprix2 -> (440,115). (So Practice comes from the left, Single
     rises from below, Championship from the right.)
   - button 1: practic2 / hl practice, nb 0xff0002ff (Right 2, Down 0), hotkey -1.
   - button 2: single2 / hl single, nb 0xff000301 (Left 1, Right 3, Down 0).
   - button 3: champio2 / hl champion (GP: Grdprix2 / Grdprix), nb 0xff00ff02 (Left 2,
     Down 0).
   - all other fields as button 0 (group -1, no var, no blink, flags 0), no hotkeys.
4. Link (0x4dbffc == 0), one sentence: Back (nb 0x01ffffff, Escape), Single placed
   (-263,115) sliding to (121,115), Championship placed (715,115) sliding to (331,115),
   buttons 1 = Single, 2 = Championship; Practice is placed but never drawn.
5. selmode placed (0,-80), slides to (0,0) in 1.0 s, kind 1.
6. **0x404990(3, 3)**: CD track 3 (the menu music; the second argument's meaning is the
   sound helper's business).
7. **0x404e10(1, 600, 0x54)**: queue one sound, id **0x54**, 600 ms later (0x404e10(n,
   {delay, id}...) takes the first channel record of 0x504240.. whose +4 is set, clears
   its +4..+0x10 and appends up to 8 (id -> +0x24[i], delay -> +0x44[i]) [asm]; the id
   being a voice ("select mode"?) is a [guess]). The main menu, page 6 and the name entry
   use the same call.
8. **0x4e90c8 = 0** (see page 6: "the championship table has been shown").

#### Frame 0x447710 [asm]
1. 0x49b1c0, 0x49a160(0,0) (present + begin, hover).
2. Tile: 0x43fab0(paralax1, **-1, +1**, 0, 0) (scrolls the other way from the main menu),
   draw mode 5 (= its own mode 4, tiled), 100, 0.
3. Title: 0x43fab0(selmode, 0,0,0,0) (slide step), draw mode 5.
4. 0x40c1f0 buttons (the slides of the three big pictures are stepped here, as button
   pictures), 0x40c500 glass.
5. 3D list 0x49ad90 (only the glass shards; skipped while 0x504e74), pointer (id 0x55 at
   the mouse), 0x49b0e0, 0x49ad70 if 0x504e74.
6. Press (0x40c1b0) — the whole decision is after the frame is finished:
   - **1..3**: 0x40c1e0; **0x4dc094 (game mode) = pressed − 1**; 0x40c4a0 (glass
     capture); if the mode is neither 0 nor 1 (Championship in Solo): **0x4dc068
     (reverse) = 0, 0x4dc078 (pocket) = 0, 0x4dc08c (track index) = 0, 0x509ba8
     (championship race number) = 0**; then ChangePage(**6**) either way.
   - **0** (Back / Escape): 0x40c1e0, 0x40c4a0, ChangePage(**3**) in Solo, (7) in Link.
   No sound of the page's own (the core plays 0x29 hover / 3 click).

**0x4dc094, the game mode** [asm]: Solo: 0 = Practice, 1 = Single race, 2 =
Championship (verified: button i -> i − 1). In Link the two buttons give 0 = Single, 1 =
Championship (so the numbers mean something else there). Initial value 0 (data).

#### 4.12.3 Page 6: track choice (map, photo, laps, time attack)

Row 6 (0x4dbb98): frame 0x448110, start 0x447a30, end 0x4480d0. No texts created (the
frame runs the text list, which is empty), no 3D, no TextList_Add.

#### The track list behind it [asm + data]
- Loaded by **0x428d60** (fastcall, script object) from `Data\list.spt` (`Data\list%02d.spt`
  when 0x4dc048, the data-disk list number, is non-zero; GOG has LIST.BPT and LIST01.BPT).
  Called at start-up (0x472c40 at 0x472d6e), by 0x4329a5 and by page 23 (0x4514e0).
- Keys: `ListName` (31 chars -> 0x504eb8; "PLUGIN"), `PocketAvailable` -> **0x4dc03c**,
  `ReverseAvailable` -> **0x4dc040**, `NumberIslande` -> **0x51d298** (track count, 8),
  `IslandeList` then per track: name, folder, ON/OFF, laps.
- **0x4dc088** = new(count × 0x130); record:

| off | field |
|-----|-------|
| +0x000 | menu: "pictures not loaded" (page 6 start sets all to 1; 0 after map+photo are loaded; Bitmap_Load 0x43e7f0 always returns 1) |
| +0x004 | name, 31 chars + 0 ("SPEED_BAY"; championship scripts refer to tracks by it) |
| +0x024 | folder, 0x103 chars ("data\ile01\") |
| +0x128 | **available**: 1 if the script says "ON" (strcmpi with 0x4e1dbc "ON"), else 0 |
| +0x12c | **default laps** |

- LIST.BPT: SPEED_BAY ile01 ON 4, SNOW_RIDE ile05 ON 2, ROCK_FOREST ile03 OFF 4,
  LOST_RUINS ile04 OFF 2, WEST_WAY ile02 ON 4, GREAT_WALL ile06 OFF 3, RED_CITY ile08 OFF 3,
  DIRT_ARENA ile09 ON 2. (LIST01.BPT "TWO_NEW_ISLANDS": FUN_FAIR ile10, SEA_OF_SAND ile11
  first, both ON.)
- **0x4dc058**: the name-entry cheat "CDNALSI" (cheat table 0x4f1250: CESREVER -> 0x4dc050,
  CTEKCOP -> 0x4dc054, CDNALSI -> 0x4dc058, each = 1) makes every track selectable.

Championship list (for mode 2) [asm 0x429a00 + data]: `NumberEntry` -> **0x5083b4** (race
count), `LevelList` -> **0x508934** = new(n × 0x50), per race: +0 track name (31), +0x20
**reverse**, +0x24 **pocket**, +0x28 ?, +0x2c ? (ints), +0x30 a track name to unlock or
"NONE". From `Data\champ%c.spt` (CHAMPA/B/C.BPT; the letter is chosen elsewhere). CHAMPA:
12 races, the 6 tracks then the same 6 reversed. **0x509ba8** = current race number.

Per-track pictures: **0x4e90ec** = new(count × 2 picture objects of 0x74 bytes; ctor
0x43e7e0, dtor 0x43e170); track i: map at 0x4e90ec + i×0xe8, photo at + i×0xe8 + 0x74.
Loaded with Bitmap_Load(obj, sprintf("%smap.lez" / "%sphoto.lez", folder), mode **1**)
(strings 0x4e950c / 0x4e9500) -> e.g. `data\ile01\map.lez` (208x315) and `photo.lez`
(237x258), from ILE01.BKF. Only the current track's pair is loaded at start; others on
demand in the frame.

#### Helper 0x447920(int *track) — choose a legal track [asm]
- If mode == 2: if 0x509ba8 >= 0x5083b4 (championship finished): 0x509ba8 = 0, **mode =
  1**, *track = 0. Else *track = the first i whose name (+4) equals race[0x509ba8].name
  (strcmp) (unchanged if none).
- Then, starting at *track, while the track is not usable, *track++ (wrapping), at most
  one full turn. Usable = +0x128 == 1, or 0x4dc058, or (+0x128 == 0 and 0x4dbffc == 0 and
  0x4dbff8 == 0) (the last case is Link only).

#### Start 0x447a30 [asm]
1. Allocate the 0x4e90ec array (fatal "Aborting game" if new fails); every track +0 = 1.
2. 0x542380 = 0 (the main menu's "rise" flag; not used here).
3. 0x447920(&0x4dc08c).
4. Mode 2: **0x4dc068 (reverse) = race.+0x20, 0x4dc078 (pocket) = race.+0x24**.
5. Load the current track's map, place (-200,133); photo, place (740,91); track +0 = 0;
   **0x5409ac = 0x4dc08c** (the track whose pictures are on screen).
6. Pictures placed (align 0), then slides (kind 1, align 0):

| id | name | placed | slides to | time |
|----|------|--------|-----------|------|
| 0x19 | selrace.lez L (640x81) | (0,-100) | (0,0) | 1.0 |
| 0x73 / 0x72 | CHTITRE.lez / CHTITREG.lez (GP) L | (0,-100) | (0,0) | 1.0 |
| — | track map | (-200,133) | (78,133) | 1.0 |
| — | track photo | (740,91) | (320,91) | 1.0 |
| 0x28 | moin.lez (62x53; hl 0x29 moin_l, flash 0x2a moiflach) | (266,580) | (266,404) | 1.0 |
| 0x2b | plus.lez (67x54; hl 0x2c plus_l, flash 0x2d pluflach) | (328,580) | (328,404) | 1.0 |
| 0x2e | laps.lez L (81x36) | (740,351) | (412,351) | 1.0 |
| 0x1a | left.lez (63x114; flash 0x1b left_l) | (-100,148) | (6,148) | 0.7 |
| 0x1c | right.lez (59x112; flash 0x1d right_l) | (740,146) | (578,146) | 0.7 |
| 0x51 | Btback.lez L | (740,438) | (516,438) | 0.7 |
| 0x4d | continue.lez L (120x40) | (740,356) | (514,356) | 0.7 |
| 0x56 | BtHiscor.lez L | (740,397) | (514,397) | 0.7 |
| 0x4f | BtLoad.lez L | (740,397) | (514,397) | 0.7 |
| 0xab | timeatak.lez (183x128) | (342,128) | — (drawn relative to the photo) | |
| 0xac / 0xad | attackon.lez / attackof.lez L (115x74) | (395,393) both | — | |

   (0.7 = 0x3f333333 -> 699 ms after the ×1000.0 truncation, as on the main menu.)
   Both titles are slid; only one is drawn (mode 2 -> CHTITRE[G], else selrace).
7. 0x404990(3,3) CD track 3; unless mode 2: **0x404e10(1, 600, 0x53)** (sound 0x53 in
   600 ms).
No buttons are made here: the frame rebuilds them every frame.

#### Frame 0x448110 [asm]
Locals: the button indices, all −2 (= none) at entry; `leaving` = 0.
1. 0x49b1c0, 0x49a160(0,0).
2. If Solo and mode != 0: **0x4dc074 (laps) = track.+0x12c** (every frame: only Practice
   chooses its laps).
3. 0x40bd40, then the buttons (all group −1, value 0, var 0, blink 0, x,y −1, flags 0):
   - **Mode 2**: 0 Btback (nb 0x02ffffff, Escape); 1 continue (nb 0xff02ffff, **no
     hotkey**); 2 BtLoad (nb 0x0100ffff).
   - **Modes 0/1**: 0 Btback (nb 0x04ffffff, Escape); 1 continue (nb 0xff04ffff, hotkey
     **Return 0xd**); 2 left (nb 0xff01ffff, hotkey **VK_LEFT 0x25**); 3 right (nb
     0xff01ffff, hotkey **VK_RIGHT 0x27**); 4 BtHiscor (nb 0x0100ffff).
     **Mode 0 only**: 5 moin / hl moin_l (nb 0x01010101, hotkey **0x6d numpad −**);
     6 plus / hl plus_l (nb 0x01010101, hotkey **0x6b numpad +**); then if
     track.+0x12c < laps: **0x4dc148 = 0** and no button; else 7 = time attack: pic = hl =
     attackon if 0x4dc148 else attackof, nb −1, no hotkey.
   (Left/Right being hotkeys, the arrow keys turn the tracks instead of moving the
   selection; Up/Down still move it.)
4. Tile 0x43fab0(paralax1, **+1, −1**), draw mode 5.
5. Title: mode 2 -> CHTITRE (GP: CHTITREG), else selrace; step + draw mode 5.
6. **0x5420d4 (word) = 1**, then 0 if any loaded track (+0 == 0) has its map or photo
   sliding (+0x10) ("no slide running").
7. If the current track's +0 == 1 (not loaded): load map + photo (mode 1), map placed
   (-200, **90**), photo (740,91), +0 = 0.
8. If 0x5420d4 == 1 and 0x5409ac != 0x4dc08c (the track changed and nothing is moving):
   new map placed (-200,133) and slid to (78,133); new photo placed (740,91) slid to
   (320,91); old map slid to (78,−400) (up and away); old photo slid to (320,580) (down);
   all **0.5 s** (0x3f000000), kind 1; 0x5409ac = 0x4dc08c. (If the player turns again
   while pictures move, the change waits until they stop; meanwhile the new map sits at
   (-200,90), 8 pixels of it on screen — the original's own quirk.)
9. For every track i: if (its map is sliding or i == current) and loaded: step + draw map
   (mode 5 -> its mode 1, colour 0 transparent), then **maptxt.lez (0xbb, 208x43, L)** at
   (map.x, map.y − 43). Same for the photo; and if **0x4dc148 and mode 0**: timeatak.lez
   at (photo.x + 22, photo.y + 37).
10. Laps digits: **num<laps % 10> at (330,334)**, **num<laps / 10> at (275,334)** (ids
    0x1e+d, num0..num9 53x62; the tens digit is drawn even when 0: "03"); then laps.lez
    (step + draw).
11. 0x40c1f0 buttons.
12. Press (0x40c1b0, != −1), in this order of tests:
    - **left**: 0x40c1e0; track−− (wrap to count−1) until usable (+0x128 == 1 or
      0x4dc058) or back to the start; **laps = track.+0x12c**; draw left_l (0x1b) at
      (6,148) (one-frame flash over the button); **sound 0x28**.
    - **right**: same forwards (wrap to 0); flash right_l (0x1d) at (578,146); sound 0x28.
    - **minus**: 0x40c1e0; if laps > 1: laps−−, 0x409bf0 (redraw), flash moiflach (0x2a)
      at moin's place. No sound.
    - **plus**: if laps < **8**: laps++, 0x409bf0, flash pluflach (0x2d) at plus's place.
    - **back**: 0x40c1e0, leaving = 1, ChangePage(**9** if mode 2 and 0x4e90c8, else
      **5**).
    - **continue**: 0x40c1e0; only if track.+0x128 == 1 or 0x4dc058: leaving = 1,
      ChangePage(**10**).
    - **hiscores**: leaving = 1, ChangePage(**9**).
    - **load** (mode 2): leaving = 1, ChangePage(**20**, championship saves).
    - **time attack**: flash attakfla (0xae, 0x504b38) at attackof's place, 0x40c1e0,
      **0x4dc148 ^= 1**. No sound.
    (ChangePage runs in the middle of the frame: the rest of this frame is still drawn.)
13. Text list: 0x418790, 0x417840, 0x418860.
14. 0x40c500 glass; 0x49ad90; pointer; 0x49b0e0; **if leaving: 0x40c4a0** (glass
    capture of this finished frame); 0x49ad70 if 0x504e74.

#### End 0x4480d0 [decomp]
Destroys the 0x4e90ec picture array (each object's destructor 0x43e170, then delete) and
sets 0x4e90ec = 0. The track list and the choices stay.

#### Globals these pages set (the race reads them; readers listed for the next helper)
| addr | meaning | set by |
|------|---------|--------|
| 0x4dc094 | game mode: Solo 0 Practice, 1 Single, 2 Championship (0x447920 turns 2 into 1 when the championship is over) | page 5 [asm] |
| 0x4dc08c | track index into 0x4dc088 | page 5 (0 for championship), 0x447920, page 6 arrows [asm] |
| 0x4dc074 | laps, 1..8 (Practice), else the track's default (+0x12c) | page 6 [asm] |
| 0x4dc148 | **time attack** on/off (initial 1 in the data); offered in Practice only, and forced 0 when laps > the track's default; read by the race at 0x411205, 0x412d4f, 0x413248.., 0x47b439, 0x47bfe0, 0x47de93.., 0x4805d0, 0x48197e | page 6 [asm; "time attack" from the picture names] |
| 0x4dc068 / 0x4dc078 | reverse / pocket (championship: from the race record; else the main menu's toggles; cleared by page 5 for a new championship) | pages 3, 5, 6 [asm] |
| 0x509ba8 | championship race number (0 for a new championship) | page 5, 0x447920 [asm] |
| 0x4e90c8 | 1 after page 9's start (0x44d4fc), 0 at page 5's start: Back on page 6 in a championship returns to page 9 | [asm] |
| 0x5409ac | the track whose pictures are shown (page 6 only) | [asm] |
| 0x5420d4 | word: no track picture is sliding (page 6 only) | [asm] |
| 0x4dc058 | all tracks selectable (cheat CDNALSI) | name entry [asm] |

Exits: 5 -> 6 (mode chosen), 5 -> 3 (Back). 6 -> 10 (continue; the bike page), 6 -> 9
(hiscores; or Back during a championship), 6 -> 5 (Back), 6 -> 20 (Load, championship).
All with the glass transition.

#### What is drawn differently
- Championship: CHTITRE title, only Back / Continue / Load, no arrows, no laps buttons, no
  hiscores, no time attack; the track is imposed by the race list (with its reverse /
  pocket); laps = the track's default (digits still shown).
- Single race: arrows + hiscores, laps shown but fixed to the default.
- Practice: + the − / + laps buttons and the time-attack switch (and timeatak.lez over the
  photo when on).
- GP edition (0x4dc1c4): Grdprix pictures on page 5, CHTITREG on page 6.
- Link: page 5 has Single/Championship only (modes 0/1), Back goes to page 7; laps not
  forced in page 6.

#### Checked against the captures [test]
build/ref/orig_p5_mode.png: Practice / Single Race / Championship cards at x 18 / 229 / 440,
top y 115, Back at (270,430), SELECT MODE bar at y 0 — as read. orig_p6_track.png (Single
Race): MAP label (maptxt) at y 90 = map.y 133 − 43, map at x 78, photo card at (320,91),
"04" digits at x 275 / 330 y 334, Laps at (412,351), Continue (514,356), Top 5 = BtHiscor
(514,397), Back (516,438), arrows at (6,148) / (578,146) — as read. No −/+ and no time-attack
switch in Single Race, as the code says: they exist in **Practice (mode 0) only**.

#### 4.12.4 Page 10: CHOOSE BIKE (start 0x44e480, frame 0x44ed30, end 0x44ecf0)

Reached from page 6's OK (and back from 8 in Link). Shows the race bike + rider of the
chosen track's family, turning on a "turntable" (the camera orbits it), Left/Right arrows to
change bike (8 per family), four FEATURES bars (Acceleration, Max speed, Grip, Brakes) that
animate to the bike's values, TRANS A-T / M-T (gearbox), Start and Back.
Captures: build/ref/orig_p10_bike_a.png, orig_p10_bike_b.png, orig_p10_manual.png (the
settled positions below match them).

Calling conventions used below: `place(pic, x, y)` = 0x43fa40 align 0; `slide(pic, x, y,
s, kind)` = 0x43f9a0 (all slides here: 1.0 s, kind 1 = eased); `draw(pic, pct_w)` =
0x43ede0(pic, surf, 5, 100, pct_w) (mode 5 = the picture's own; pct_w = % of columns cut
off on the right). Button args in 0x40bef0 order: pic, hl, neighbours (bytes L,R,D,U),
hotkey, group, value, var, blink, x, y, flags. [asm for all numbers]

#### Start 0x44e480 [asm]
1. Bike number **0x584499** (= field +0x78 of player 1's bike object 0x584421): -1 → 0
   (first visit); otherwise kept from the last visit.
2. Zeroed: mouse deltas 0x519ff4/0x519ff6; look-at target **0x53f540/0x53f544/0x53f548**
   (x, y, z); voice-played flag 0x53f554; 0x53f78c = -1 (wait-until time); bike object
   position 0x584451/55/59 (= obj+0x30..0x38: the turntable is at the origin);
   0x5420d0 (switch direction) = 0; 0x5420dc (displayed-bike offset) = 0; 0x4dc130 = 0;
   0x4ea4d0 (lighting from the camera) = 0.
3. **0x40a4d0(track 0x4dc08c)** loads the bike to show (bike number saved around the call):
   reads `<track dir>Level.Spt` (track record 0x4dc088 + n*0x130, +0x24 = dir), takes its
   bike-family script (7th string `data\motoSpd.spt` etc., or the 8th, the Pocket one
   `data\PktSpeed.spt`, when Pocket 0x4dc078 is on), reads the family's 8
   `carac1..8.spt` (0x42a580 → bike table, below), loads the family's two texture banks
   (0x401d90; the second chosen by PersoAnim == 1), then the bike+rider object 0x584421
   (0x45bf70(obj,0,1,1,0), 0x45bdf0, 0x45b480(name 0x51d678, obj, 0), 0x45c7c0(obj,0,0,0),
   0x45c700(obj, texture page)). Families on disc: MOTOSPD (road: data\speed),
   PKTSPEED, MOTOKTM (dirt: data\ktmcross), PKTKTM. Which one a track uses is in its
   Level.Spt (Snow Ride → dirt bikes). [asm for the calls, decomp for the reading]
4. Light 0x52c228/0x52c22c/0x52c230 = (-0.5, -1.0, 0.0); 0x52c230 = 0.
5. Camera (block 0x5161c0 + 0x4dc0f8*0x238): 0x46da40(cam, angles (0,0,0) → +0x38..+0x40,
   offset (0,0,1500) → +0x50..+0x58); position +0x98..+0xa0 = (0, 0, 1500.0);
   0x46e140(cam, (0,0,0)); 0x49a120(1.0).
6. Pictures (id, name: start → slide target):

| ptr | id | file | placed at | slides to |
|-----|----|------|-----------|-----------|
| 0x504940 | 0x30 | choosemo.lez (title bar "Moto Racer / CHOOSE BIKE") | (0,-80) | (0,0) |
| 0x5048e8 | 0x1a | left.lez | (-100,148) | (6,148) |
| 0x5048f0 | 0x1c | right.lez | (740,146) | (578,146) |
| 0x504944 | 0x31 | features.lez (panel "FEATURES: ... TRANS", the four words) | (27,530) | (27,322) |
| 0x5049b0 | 0x4c | start.lez | (740,373) | (512,373) |
| 0x5049c4 | 0x51 | Btback.lez | (740,417) | (512,417) |
| 0x504948 | 0x32 | featacc.lez (green bar) | (145,568) | (145,360) |
| 0x50494c | 0x33 | featspe.lez (yellow bar) | (145,595) | (145,387) |
| 0x504950 | 0x34 | featgri.lez (orange bar) | (145,621) | (145,413) |
| 0x504954 | 0x35 | featbra.lez (red bar) | (145,647) | (145,439) |
| 0x504958 | 0x36 | auto.lez (A-T lit) | (401,564) | (401,356) |
| 0x504960 | 0x38 | autosha.lez (A-T dark) | (401,356) | — |
| 0x50495c | 0x37 | autofla.lez (A-T highlight) | (401,564) | — |
| 0x504964 | 0x39 | manu.lez (M-T lit) | (401,407) | — |
| 0x50496c | 0x3b | manusha.lez (M-T dark) | (401,615) | (401,407) |
| 0x504968 | 0x3a | manufla.lez (M-T highlight) | (401,615) | — |

   Background 0x50493c id 0x2f `choosefo.lez` is not placed (drawn where it is, 0,0
   [guess]). Not created here but drawn on a press: left_l (0x5048ec id 0x1b), right_l
   (0x5048f4 id 0x1d).
   Note: when the gearbox is Manual at entry, A-T dark and M-T lit are *not* slid (they
   appear at once at their final places); in Automatic, A-T lit and M-T dark slide up
   with the rest. [asm]
7. Feature bars: targets 0x540cc4 / 0x53f670 / 0x53f4c8 / 0x5420f8 = `(v*-100)/100 + 100`
   = 100 − v (ints) of the bike's Gfx_acc / Gfx_spe / Gfx_gri / Gfx_bra; previous values
   0x53f4c4 / 0x540af8 / 0x540990 / 0x5420d8 = 0 (so at entry the bars start **full** and
   shrink to their value); shown values 0x53f4e4 / 0x540078 / 0x53f780 / 0x53f54c = 0.
   Animation clock: 0x540cc0 = now, 0x540cbc = now + 1000, 0x53f84c = 1 (running).
8. Buttons (0x40bd40 first):

| # | pic | hl | neighbours | hotkey | blink | does |
|---|-----|----|-----------|--------|-------|------|
| 0 | left.lez | – | 0xff0201ff (R→1, D→2) | **Left (0x25)** | 0 | previous bike |
| 1 | right.lez | – | 0xff02ff00 (L→0, D→2) | **Right (0x27)** | 0 | next bike |
| 2 | start.lez | – | 0x0103ff00 (L→0, D→3, U→1) | **Return** | 0 | go |
| 3 | Btback.lez | – | 0x02ffffff (U→2) | **Escape** | 0 | back |
| 4 | auto.lez if 0x4dc0a0 == 1 else autosha.lez | autofla.lez | 0xffffffff | – | **1** | Automatic |
| 5 | manusha.lez if 0x4dc0a0 == 1 else manu.lez | manufla.lez | 0xffffffff | – | **1** | Manual |

   All group -1, var 0, x,y = -1 (pictures stay where placed), flags 0. Since Left/Right are
   hotkeys, the arrow keys change the bike directly (the arrow navigation never runs for
   them). Buttons 4/5 blink (drawn plain every other frame while highlighted).
   No texts, no TextList_Add, no sounds or CD call in the start routine (the music of
   page 5 keeps playing).

#### Frame 0x44ed30 [asm]
Order:
1. 0x49b1c0(1), 0x49a160(0,0) (hover).
2. draw choosefo.lez (the whole background).
3. **Camera orbit**: 0x46dab0(cam, bike+0x24): rotation from the camera angles (+0x38 a,
   +0x3c **θ**, +0x40 c) combined with the bike's angles (bike+0x24..); camera position =
   bike position (+0x30..) + cam+0x8c.. + 1500 × the matrix's third row (distance
   +0x58). Then cam y (+0x9c) += 200.0 (0x4d0c44); 0x46e140(cam, target 0x53f540);
   cam y += 40.0 (0x4d0bac) (so the view is aimed from y+200 but rendered from y+240).
4. c = fcos θ, s = fsin θ (real FPU sin/cos), a = 250.0·c (0x4d0c48), b = 100.0·c
   (0x4d0c50).
5. **Turntable / bike switch** (dir = 0x5420d0, T = target x 0x53f540, Z = target z 0x53f548;
   tests on c are on its sign bits; nothing happens if c == 0):
   - dir == 0 (idle): dt = now − 0x4e9118, at most 1000; 0x4e9118 = now; θ += dt ×
     0.0006135923 (0x4d0c58 = 2π/10240: one turn in 10.24 s). (0x4e9118 is not updated
     while switching, so after a switch, and on the first frame of a visit, θ jumps by up
     to 1000 ms worth.)
   - (dir == −1 and c > 0) or (dir == 1 and c < 0): if |a| > T: T += |b|, Z += (|b| / −c)·s;
     else T = −|a|, Z = (−|a| / −c)·s, **0x5420dc = 0** (the new bike appears).
   - (dir == 1 and c > 0) or (dir == −1 and c < 0): if −|a| < T: T −= |b|, Z −= (|b| / −c)·s;
     else T = |a|, Z = (|a| / −c)·s, 0x5420dc = 0.
   The look-at point slides sideways by 100|c| a frame (frame-rate dependent), the bike
   whips off one side (3 frames), the index changes, it comes back from the other side.
6. Displayed bike = (0x584499 + 0x5420dc) & 7 → written to bike+0x2f4, +0x478, +0x5fc
   (0x584715, 0x584899, 0x584a1d: the skin index of the three detail levels [guess]). One
   model + rider for the 8 bikes; they differ by texture.
7. If 0x4ea4d0 (never in the menu): light vector from the camera — skipped.
8. **0x45f290(bike 0x584421, renderer, cam, 0)** queues the bike+rider polygons (the race
   bike drawer).
9. Switch end (only when 0x5420dc == 0): if (dir == 1 and c > 0 or dir == −1 and c < 0) and
   T ≤ 0, or (dir == −1 and c > 0 or dir == 1 and c < 0) and T ≥ 0 (float sign bits): T =
   0x53f544 = Z = 0, dir = 0.
10. Step+draw choosemo.lez, step+draw features.lez, step the 4 bars.
11. Bar targets recomputed from the current bike (100 − Gfx). If 0x53f84c: t = min(now,
    0x540cbc); f = (t − start)/(end − start); e = (sin(f·π − π/2) + 1.0)·0.5; shown =
    ftol(prev + (target − prev)·e) for each bar; when t == end, 0x53f84c = 0.
12. Draw the 4 bars at full height with pct_w = shown value (bar length = Gfx %).
13. Press (0x40c1b0 → p):
    - p == 2 (Start) and voice not played (0x53f554 == 0): play sound **0x2c** if the bike's
      PersoAnim (0x509398 + bike*0x124) == 0 (road family) else **0x2b** (dirt) — an effect
      id (< 0x2f: engine roar [guess]); if played, 0x53f78c = its length in ms (0x4240d0 →
      0x441fa0: bytes / (bits/8 × channels) × 1000 / rate); then voice **0x58** (if
      played, 0x53f78c = its length instead); 0x53f78c += now; 0x53f554 = 1.
    - If 0x53f78c != −1 and now ≤ 0x53f78c: nothing else this frame (the press stays
      pending; Left/Right/Back are blocked too).
    - 0 Left: clear press; if bike != 0: sound 0x28, 0x5420dc = 1, bike −= 1, dir = −1;
      left_l.lez placed at (6,148) and drawn this frame; prev bars = old targets, new
      targets, bar clock restarted (1000 ms, 0x53f84c = 1). At bike 0 nothing (no wrap).
    - 1 Right: same with bike != 7, sound 0x28, dir = 1, 0x5420dc = −1, right_l.lez at
      (578,146).
    - 2 Start: clear press; Solo (0x4dbffc != 0): **ChangePage(1)** (loading) **without**
      the glass transition; Link: glass + ChangePage(8) (and, if 0x4dbff8, 0x456db0 sends the
      bike to the others).
    - 3 Back: clear press, ChangePage(6), glass (0x40c4a0 after the frame is finished).
    - 4 A-T: clear; the 6 buttons rebuilt with auto.lez / manusha.lez for 4/5;
      **selected = 2** (Start); **0x4dc0a0 = 1**.
    - 5 M-T: same with autosha.lez / manu.lez; selected = 2; **0x4dc0a0 = 0**.
14. 0x40c1f0 buttons, text list (0x418790/0x417840/0x418860; empty here), 0x40c500 glass,
    **0x49ad90** (the bike is rasterised now, over all the 2D), pointer, 0x49b0e0, glass
    capture if leaving to 6/8, 0x471440(0x5d2dd8) (a timed screenshot/video capture
    tool — nothing for the port).

#### End 0x44ecf0 [asm]
0x401ab0 on the texture-bank objects 0x4f92f8 and 0x4f9630 (frees the bike textures),
0x4db1c0 = 0, 0x4ea4d0 = 0, 0x4dc130 = 0; if bike+0x68 (0x584489, "loaded"): clear it and
0x45be60(0x584421) (free the model). Page 1 loads the race bikes itself.

#### The bike table (0x509278 + i*0x124, i = 0..7, filled by 0x42a580 from carac<i+1>.spt)
Field +0 MotoType (2 road, 1 dirt), **+4 Gfx_acc, +8 Gfx_spe, +0xc Gfx_gri, +0x10
Gfx_bra** (the four bars, ints 0..100), then the physics floats (MoteurVitesseMax,
MotoMasse, ..., RapportDeBoite1-6, materials...), **+0x120 PersoAnim** (0 road rider, 1
dirt rider). The page reads +4..+0x10 and +0x120. Values in the GOG data:

| family | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
|---|---|---|---|---|---|---|---|---|
| SPEED | 75 75 75 75 | 50 75 100 100 | 100 50 75 100 | 50 100 75 75 | 100 75 75 50 | 100 85 45 70 | 75 100 50 75 | 100 75 50 100 |
| SPEEDPKT | 75 75 75 75 | 50 75 100 100 | 100 50 100 75 | 50 100 75 75 | 100 75 75 50 | 100 75 50 75 | 75 100 50 75 | 100 75 50 100 |
| KTMCROSS | 75 75 75 75 | 50 75 100 75 | 100 50 75 75 | 50 100 100 50 | 100 75 75 50 | 100 75 50 75 | 75 100 50 75 | 50 100 75 75 |
| KTMPKT | 75 75 75 75 | 50 75 75 100 | 100 50 75 75 | 75 100 50 75 | 50 100 100 75 | 100 75 50 75 | 100 75 75 50 | 50 75 100 50 |
(acc spe gri bra; PersoAnim 0 for SPEED*, 1 for KTM*.) [test: tools/bpt.py on the banks]

#### What the race reads from page 10
- **0x584499** bike number 0..7 = player 1's bike object +0x78; the race uses it as the
  carac index (0x45c56c: `[obj+0x78]` → table) [asm].
- **0x4dc0a0** gearbox: 1 automatic, 0 manual. 0x4110f1: in Solo player 1's bike field
  +0x88 (0x5844a9) = 0x4dc0a0, computer bikes get 1; 0x45c58b: manual adds 2.0 to the
  bike's carac float at record +0x48 (a top-speed value [guess]). The loading page saves
  it with the other choices (0x40b8f0 → 0x542880) [asm].

#### 4.12.5 Page 9: TOP 5 (start 0x44cfd0, frame 0x44d540, end 0x44d510)

Two pages in one routine: **Solo (0x4dbffc != 0)** = the TOP 5 hi-score page of the
current track/laps/options (title bar "TOP 5   <TRACK NAME>", "2 LAPS [REVERSE] [POCKET]",
"Best RACE" with up to 5 rows, "Best LAP"), shown from page 6's Top-5 button and after a
race (single race / practice / championship); **Link** = the race results with points
(results.lez, Totalpts, Coupe, rows from 0x5d0d61) — out of scope here. Capture:
build/ref/orig_after_race_top5.png (Snow Ride, 2 laps; four rows, best lap 01'15''50).

#### Start 0x44cfd0 [asm]
1. 0x519ff4/6 = 0; 0x5401c4 (new-record row bits) = 0; 0x540cb8 (new best lap) = 0; the
   best-lap record 0x540230 (0xf9 bytes) filled with 0x7f; 0x542380 (second slides
   started) = 0; 0x51a414 = 0; typed char 0x51be90 = 0.
2. Font 12 (Messages): SetColors(white 0x4f0dd4, white 0x4f0dd4).
3. **0x464e80(0)**: loads the track's 5..8 race records into 0x5cf500 (8 × 0xf9 bytes)
   from `Data\bestscor.dat` (blocks: 0x20-byte key, one 0xf9 best-lap record, 8 race
   records; key = track name (track record +4) + "POC" if Pocket + "REV" if Reverse +
   "TRA<laps>" in practice mode 0x4dc094 == 0; not found → all 0x7f) [decomp].
   Record layout (base 0x5cf500 + i*0xf9): +9 name (string, 3 letters), +0xf1 race time ms,
   +0xf5 (row shown when != 0x7f7f7f7f; best lap of that race [guess]).
4. Solo pictures:

| ptr | id | file | placed | slides to |
|-----|----|------|--------|-----------|
| 0x5048c8 | 0x12 | toptitle.lez (bar "Moto Racer / TOP 5") | (0,-80) | (0,0), 1.0 s, kind 1, at start |
| 0x5048d0 | 0x14 | bestRace.lez ("Best RACE") | (-150,135) | (430,135), 0.7 s, kind 1 (later) |
| 0x5048cc | 0x13 | bestlap.lez ("Best LAP") | (30,400) | — |
| 0x5049b4 | 0x4d | continue.lez (single/practice) | (840,429) | (496,429), 0.7 s |
| 0x504ba4 | 0xc9 | resetTA.lez (practice only) | (640,429) | (296,429), 0.7 s |
| 0x5049c8 | 0x52 | BtNextRa.lez (championship) | (840,437) | (496,437), 0.7 s |
| 0x5049c0 | 0x50 | BtAbort.lez (championship) | (640,437) | (296,437), 0.7 s |
| 0x5049b8 | 0x4e | Btsave.lez (championship) | (640,437) | (150,437), 0.7 s |

   Background: 0x504884 id 1 `paralax2.lez` (the green chequered tile, mode 4), scrolled
   (−1, +1) a frame.
5. Buttons:
   - mode != 2 (practice 0 / single race 1): **0 Continue** (continue.lez, nb 0xffffff01:
     Left → 1, no hotkey — Escape does nothing); **1 Reset** (resetTA.lez, nb 0xffff00ff:
     Right → 0) only if mode == 0 and laps 0x4dc074 ≤ the track's default laps
     (track record +0x12c).
   - mode == 2 (championship): 0 Next race (BtNextRa, nb 0xffffff01) only if race 0x509ba8 <
     race count 0x5083b4; then Abort (BtAbort, nb 0xffff0002, **Escape**); then Save
     (Btsave, nb 0xffff01ff) only if 0x509ba8 < 0x5083b4. (So when the championship is over
     Abort is button 0 — and button 0 then means "continue" in the frame: see exits.)
6. New records: for i = 0..4, record i +9 == 'TINI' (0x494e4954, written by the race end
   when the player enters the table) → bit i in 0x5401c4, the marker cleared (name empty).
   If any: sound 0x5a if row 0 (first place) else 0x59 (voices). 0x4656c0(0x540230) loads
   the best-lap record (bestscor.dat, magic "00.1"); if it returns 1 and its name (+9,
   0x540239) is 'TINI': 0x540cb8 = 1, name cleared.
7. 0x4e90c8 = 1 (page 6's Back then returns to 9 in a championship).

#### Frame 0x44d540 [asm]
1. Once toptitle's slide has ended (pic+0x10 == 0) and 0x542380 == 0: start the 0.7 s
   slides of bestRace and the buttons (table above); 0x542380 = 1.
2. 0x49b1c0(1), 0x49a160(0,0); step paralax2 (−1, 1) and draw.
3. Draw toptitle, bestRace, bestlap (each stepped).
4. Title: track name (track record +4, '_' → ' ') in font **31 DataDiskMenu**, SetColors(black
   0x4f0dd8, yellow 0x4f0de8), at (toptitle.x + 450, toptitle.y + 56) (follows the bar's
   slide), 0x493ee0 style 0.
5. Options line: sprintf "%d %s " (laps, text[17] "Lap" if 1 else text[18] "Laps"), +
   "Reverse" if 0x4dc068, + " " if Reverse and Pocket, + "Pocket" if 0x4dc078; uppercased;
   font 31 with 0x490a30(255,255,0) and 0x490a60(0,139,89), slot 1 (1,0xff), at (33,103).
   (Captured: "2 LAPS" yellow.)
6. Blink counter 0x4e9114 = (0x4e9114 + 1) mod 5.
7. Rows i = 0..4, y = 182 + 44 i, only if record +0xf5 != 0x7f7f7f7f:
   ligne.lez (0x504a20 id 0x68, mode 3 in the table) at (30, y − 12); number<i+1>.lez
   (0x504a00 + 4i, ids 0x60..) at (28, y − 14); name in font **12 Messages** at (110, y) —
   for a new-record row: if the name is empty the cursor 0x540070 = 0, and the name is
   shown as "%s_" while 0x4e9114 < 3 (3 frames of 5), else "%s"; time at (480, y) as
   `%02d'%02d''%02d` = (s/60)%60, s%60, (ms%1000)/10 with s = ms/1000 (record +0xf1).
8. Best lap (if 0x540325 != 0x7f7f7f7f): name (0x540239; "%s_" blinking when 0x540cb8 == 1)
   in font **9 MenuLink** at (330, 400); time (0x540325) same format at (180, 400).
9. 0x40c1f0 buttons, text list, 0x40c500, 0x49ad90, pointer, 0x49b0e0.
10. **Name typing** (only in keyboard mode menu+4 == 0, Solo, a WM_CHAR in 0x51be90):
    Backspace (8): cursor −1 (not below 0), the new rows' (and new best lap's) name[cursor] =
    0. Return (0xd): if 0x5401c4: clear it and save (0x464b70); if 0x540cb8: clear it and
    save the best lap (0x4658b0(0x540230)). Any other char while cursor < 3: upper-cased,
    stored at name[cursor] of every new row (and the new best lap), name[cursor+1] = 0,
    cursor += 1. Then 0x51be90 = 0. (The mouse moving puts the menu in mouse mode and
    typing stops until a key is pressed.)
11. Press (0x40c1b0):
    - 0: clear, glass; mode != 2 → Solo: **ChangePage(6)** (Link: 8). Championship: if
      0x4dc07c == 0 → 5 (Link 7), else 6 (Link 8) [0x4dc07c meaning not read: probably
      "championship finished"; guess].
    - 1: clear, glass; Solo: championship → **ChangePage(3)** (Abort); otherwise (the Reset
      button) build `<prefix 0x4e8380><track dir>\timatak` + ("R" if Reverse else "_") +
      ("_" if not Pocket; with Pocket the "P" goes to the wrong buffer 0x53f3c0, so nothing is
      added — a bug of the original [asm]) + "<laps>.bin", delete it (0x4b5190), ChangePage(6).
      (Link: 0x456380 message, 0x4e90b0 = 1, page 7.)
    - 2: clear, glass, ChangePage(21) (championship Save).

#### End 0x44d510 [decomp]
If 0x5401c4 still set: save the table (0x464b70); if 0x540cb8: save the best lap
(0x4658b0(0x540230)). So leaving with the name half typed saves it as typed.

Port (2026-10-26): `page_top5.c` takes the 'TINI' rows and best lap, the typing (WM_CHAR as
`g_menu_char`: SDL's text input, Backspace 8, Return 13) and the saves of Return and the end.

#### Globals
Reads 0x4dbffc (Solo), 0x4dc094 (mode), 0x4dc08c/0x4dc088 (track), 0x4dc074 (laps),
0x4dc068 (Reverse), 0x4dc078 (Pocket), 0x509ba8/0x5083b4 (championship race / count),
0x4dc07c, 0x4dc1c4 (GP edition). Writes 0x4e90c8 = 1, the hi-score tables and
bestscor.dat, deletes the timatak file. Nothing the next race reads.

#### 4.12.6 Pages 20/21: championship Load / Save [decomp + asm, checked 2026-10-26]
**20 Load** (start 0x450600, frame 0x450800), reached from page 6's Load in a
championship; **21 Save** (start 0x450d20, frame 0x450f40), from page 9's Save. (An
earlier reading here had the load parse the save as a script and the save go back to 9:
both wrong.)
- Pictures: tile CHFOND (id 0x71, scrolled (+1, +1) a frame), CHTITRE 0x73 (CHTITREG 0x72
  in the GP edition) at (0,0), CHBARRE2 0x75 at (129,158); the header Load 0x76 / lit 0x77
  (page 21: Save 0x78 / lit 0x79) at (168,164), the lit one while any slot is hovered
  (hit ≥ 1); the slot lines 0x7a, lit 0x7b (page 21: 0x7c) for the hovered slot.
- Buttons, rebuilt every frame: 0 = Cancel 0x47 (page 20: nb 0x08ffffff, Escape, at
  (270,411)) or Continue 0x4d (page 21: nb 0x01ffff09, no hotkey, at (447,411); Cancel is
  placed at (176,411) but never added); then 8 slots at (129, 197 + 22 i), hotkey 0x61 + i
  (**keypad 1..8**), nb Down = next / Up = previous (0x0002ffff, + 0x01010000 each, the last
  0x0700ffff: up from the first and down from the last go to button 0).
- The start reads each slot's first 0x1e bytes (its title) into 0x5409c8 (0x20 each, byte
  0x1f cleared; "" without a file); font 12 SetColors(white, white). Titles drawn in font 12
  at (137, 198 + 22 i).
- File `<0x4e8380 = "">Data\Champ%1d.sav`: the title (0x1e bytes), then u32 reverse,
  pocket, difficulty, opponents (0x4dbfdc), race 0x509ba8, laps, **riders 0x509bac** (the
  race's rider count), then the standings **0x5d0d58, 8 × 0xf9**: 2050 bytes.
- Load press: the file read ("rb"; none → nothing happens), 0x4e90b0 = (race == 0), the
  championship script of that difficulty read again (0x429a00); failed → the frame ends
  there; read → ChangePage(6) **without** the glass. Cancel: glass, page 6.
- Save press: "wb" (opened before anything is checked), title = sprintf("%s ON %s (",
  player 0x5161b0, the race's track) with '_' → ' ' in the whole string, + "P" if the race
  is Pocket (+0x24), + "R" if Reverse (+0x20), + L/M/H by difficulty, + ")", cut at 31;
  the 0x1e bytes written are the buffer's (past the title's end, what it held before). The
  page stays, with the new title listed. Continue: page 9, no glass.
- Checked: the original's page 20 empty, with a slot hovered, and listing a Champ1.sav
  **written by the port**, which it loads (to page 6, Speed Bay 04 laps): the port's
  pictures equal but for the pointer (build/ref/orig_p20_*).

#### 4.12.7 Page 1: loading (frame 0x40b3f0; no start, no end routine)

Page 1 has no start routine, so Menu_Frame calls its frame routine once with `0x504e74`
still 1. That one call does all the work: it draws the loading screen once, tears the menu
down, and loads the race. The program then blocks until the race is loaded. Nothing
animates while it waits. [asm 0x40b3f0..0x40bbf9]

**Pictures** (each is a new 0x74-byte picture object 0x43e7e0, kept in the shared scratch
slots 0x4e90e8..0x4e90f8, freed at the end) [asm]:
| slot | picture | placed at | drawn |
|------|---------|-----------|-------|
| 0x4e90e8 | `sprintf("%sload.lez", track.dir)` (0x4dbd88; track.dir = record +0x24 of the track table, e.g. `data\ile05\` → `data\ile05\load.lez`, in ILEnn.BKF), loaded by 0x43e7f0(path, 0). **In replay playback** (0x4e9830 != 0 and 0x542891[0] != 0) the path is the string at 0x542891 instead | as loaded (0,0), 640x480 | yes, mode 5 |
| 0x4e90ec | id 0xaf `lodanim.lez` (lang 0, the coloured Moto Racer logo over load.lez's grey one) | (498, 381) align 0 | not here: by each step of the race's load, every other step (see below) |
| 0x4e90f0 | id 0xb0 `loadtxt.lez` (lang 1, "Loading...", 235x34) | (0, 408) | yes, mode 5. Not created during replay playback with a name |
| 0x4e90f8 | id 0xba `smallpok.lez`, only if Pocket 0x4dc078 | (4, 460) | only if `menu+0 == 0` |
| 0x4e90f4 | id 0xb9 `smallrev.lez`, only if Reverse 0x4dc068 | (4, 470) | only if `menu+0 == 0` |

**Correction (2026-10-26):** an earlier reading said lodanim and the badges are never drawn
(a `grep | head` cut the list of users short). **0x4470d0** draws the loading screen again
at each step of the race's load (called by the progress routine 0x4421d0 from 0x4011c6,
0x410dc5 "Init WAV samples...", 0x462cbc, and by the menu's first load 0x40ab35/0x40ac16):
0x49a160, load.lez (0x4e90e8), loadtxt (0x4e90f0), the Reverse badge 0x4e90f4 and the
Pocket badge 0x4e90f8 if created **and** `menu+0 == 0` (true by then: the menu is torn
down), then lodanim if 0x4e90fc != 0 and 0x4dc080 == 0 and not a replay (0x4e9830 == 0),
then text 0x4e94f4 in font 21 at (550, 800) if 0x4dc084 (only the first menu load sets it,
after 10 steps), 0x49b0e0, 0x49b1c0(1), and **0x4e90fc ^= 1** (initial 1, never reset: the
logo blinks from step to step). 0x4dc080 is 1 at start (the first menu load shows its own
picture 0xcb/0xca, not the logo) and 0x40b1f0 clears it. orig_p1_loading_snowride.png
shows the logo on (a first step). Page 1's own frame, with the menu still active, draws
neither the badges nor the logo [asm 0x40b6d2,
0x40b6fc: `cmp DWORD PTR [menu],0`]. The capture build/ref/orig_p1_loading_snowride.png
shows the same thing: load.lez, a full-screen picture with the black strip, the vertical
track name and the Moto Racer logo all part of the picture, and "Loading..." at the bottom
left. No text objects and no fonts are used. No sound or CD call is made here: the music
keeps playing until the race changes it.

**Order** [asm]:
1. The pictures are built as in the table above.
2. Draw: 0x49a160(0,0) (frame begin; note there is no 0x49b1c0 before it). Then load.lez
   and loadtxt (mode 5), then the two badges (skipped, see above). Then 0x49b0e0(0,0) and
   **0x49b1c0 with argument 1** (`push 1`; present now). The screen stays like this while
   the rest runs.
3. Menu teardown, an inline copy of 0x409eb0: cursor = arrow (0x45a190(0x4dbf84)), clear
   the text list 0x53b8e8, free the unpacked pixels of every picture, selected = -1,
   0x504e78 = 0, mouse mode = 1, 0x504e74 = 1, the current page's end routine (page 1 has
   none), and the D3D flags. Then **SetPage 0x409f70(menu, 0)** and, as in the quit page,
   the end routine of every page whose started flag is set (started = 0). Then
   **0x40b1f0(menu)**: menu+0 = 0, free the 3D meshes 0x504c70..0x504e70 and every menu
   picture 0x504880..0x504bc4 (plus the GP-edition copies). Then `menu+0 = 0` once more.
   From here App_OnIdle runs the race, not Menu_Frame.
4. **Race parameter block** 0x542860 (0x139 bytes; it is also the header of `.Sav` replay
   files, see 0x4579b0 / 0x457bf0):
   - Not a replay (0x4e9830 == 0): the menu choices are saved into it: +0x08 track
     0x4dc08c, +0x0c laps 0x4dc074, +0x10 mode 0x4dc094, +0x18 reverse 0x4dc068, +0x1c
     pocket 0x4dc078, +0x20 transmission 0x4dc0a0, +0x28 player name (strcpy from
     0x5161b0); +0x14 rider count = 0x4dbf78 if 0..24 (a start-up override set in
     0x414080), else 23 (0x17), or 1 for practice (mode 0) [asm 0x40b924]. Then the whole
     0x139 bytes are copied to 0x580830 (`rep movs` 0x4e dwords + 1 byte).
   - Replay: the reverse. The menu globals and the name are restored from the block.
5. Level script: script object 0x427d90; `sprintf("%sLevel.Spt", track.dir)` (0x4dbd60);
   open with 0x4246a0(path, 1); parse with 0x427e80(0,0,0); close with 0x4246e0. **If the
   parse fails, it returns at once**: the menu is gone and no race is loaded (an edge case;
   the port can report the error instead).
6. Race object: 0x52c0d4, or if 0 the static object 0x52c478 (vtable 0x4d04c8: slot 0
   0x47bf30, 1 0x47b420 load, 2 0x47bf70, 3 0x47d620 unload, 5 0x47d660 Game_Frame).
   0x401f90 frees the 0x2e texture pages at 0x4f92f8. 0x41e560 runs 0x41e4c0(i) for
   i = 0..19 on 0x624afc (texture surfaces) [decomp, guess]. Cursor 0x4dbf88 (the second
   cursor loaded at 0x45a0f8: the busy one [guess]). The display globals saved on the
   menu's first entry are put back: 0x4dc0d4 = 0x504e80, **0x4dc0c8 (screen mode) =
   0x504bc4**, 0x4dc028 = 0x50487c.
7. **`race->vtable[1]` = 0x47b420(track 0x4dc08c, kind 0x4e9834)**, the whole race load:
   - Two 0x4191a0 resets. If time attack (0x4dc148): 0x4136a0 loads the ghost
     `<track.dir>timatak` + (`_` or `R` for reverse) + (`_`, or nothing for pocket: §4.21c) + laps
     digit + `.bin` (`%s%1d.bin`) into 0x581018.
   - 0x4dc144 = reverse; 0x4dc140 = 1 if not reversed, else -1. Some counters are cleared.
   - 0x4100f0. Then 0x410120: race state; 0x4dc08c = track; 4 cameras; 0x4dc0ac = solo;
     0x4e9834 = kind.
   - 0x410270 loads the track (`%sLevel.Spt` again). 0x410450 loads the bikes: the folder
     table depends on pocket (0x51d068 normal, 0x51d0a8 pocket; the KTMPKT/SPEEDPKT
     banks [guess]). 0x410590, 0x40d650 (the track's data), checkpoints counted into
     0x4e9844. 0x410350 loads the horizon `%sBank\Horiz{Hi,Med,Low}.raw`. 0x410a10.
   - **Rider count 0x542874**: Link = the number of network players (type 3 in
     0x508048..); Solo practice = 1; Solo otherwise = 23, or **11 when the option
     "OPPONENTS" 0x4dbfdc = 1 ("12")** [asm 0x47b542..0x47b5af]. This overrides step 4.
   - 0x410a50 sets up the riders (the grid; 0x47bc60 applies the difficulty, see below).
     0x410d70 "Init WAV samples...". Then 0x47b600, 0x410e10, 0x40faf0, 0x4020b0, 0x410ee0,
     and 0x4100a0 "initRace" (whose result is the result of the load).
8. **Load failed** (0): vtable[3] 0x47d620 unloads, the same inline teardown, then
   **SetPage(3)**. The menu becomes active again with a full first-entry setup (§4.4.2.2),
   and the player is back on the main menu.
9. Either way: cursor = arrow; the five pictures are freed and their slots cleared.

So the hand-over is: menu+0 = 0, page 0, all started flags 0, the menu's resources freed,
and the race object loaded. App_OnIdle's next call goes to the race. **Timing:** a single
frame; the picture shows for as long as the load takes (no minimum time) [asm: no timer].

#### 4.12.8 The menu's choices the race reads

| Address | Meaning | Set by (menu) | Read by (race) |
|---------|---------|---------------|----------------|
| 0x4dbffc | **1 = Solo, 0 = Link** | page 3 (Solo/Link), demo 0x409a30 | everywhere: 0x47b420 (rider count), 0x47bc60 (grid speeds), 0x410120 (0x4dc0ac), Game_Frame 0x47d660, race end 0x412a50, 0x4580f0 [asm] |
| 0x4dc094 | **mode: 0 practice, 1 single race, 2 championship** (page 5 buttons 1..3 → 0..2 in Solo; in Link the two buttons give 0/1 [decomp]) | page 5 0x447710, 0x447920 (page 6), 8, 22 (Link), 0x455280 (load championship); default 1 (0x47bf30) | 0x47b420 (1 rider in practice), 0x412a50 (end of race per mode), 0x4580f0, 0x4587e0, HUD 0x47dae0/0x47e3d0 [asm] |
| 0x4dc088 | pointer to the **track table**, records of 0x130 bytes: +0x04 name (e.g. `SNOW_RIDE`, 0x20 chars), +0x24 folder (`data\ile05\`), +0x128 available (1 = ON), +0x12c default laps. Built from `List*.bpt` (`IslandeList`, `NumberIslande` = count in **0x51d298**, `ReverseAvailable`, `PocketAvailable`; 0x428e69/0x4291f0). LIST.BPT holds 8 tracks: SPEED_BAY ile01 ON 4, SNOW_RIDE ile05 ON 2, ROCK_FOREST ile03 OFF 4, LOST_RUINS ile04 OFF 2, WEST_WAY ile02 ON 4, GREAT_WALL ile06 OFF 3, RED_CITY ile08 OFF 3, DIRT_ARENA ile09 ON 2 | start-up | page 1 (paths), 0x410270, 0x4136a0, 0x4580f0 (unlocks +0x128) |
| 0x4dc08c | **track index** into that table | page 6 0x448110 (Left/Right, skipping unavailable tracks unless 0x4dc058) | page 1, 0x47b420 → 0x410120, 0x4136a0 [asm] |
| 0x4dc074 | **laps**, 1..8 (page 6 -/+); reset to the track's +0x12c when the track changes | page 6; default 3 | race HUD/end 0x47e3d0, 0x483860, ghost file name; in the 0x542860 block |
| 0x4dc068 | **Reverse** on (track run backwards) | main-menu switch (only when unlocked); page 5 resets it to 0 for championship | 0x47b420 via 0x542878 → 0x4dc144, 0x4dc140 = -1; ghost name `R` |
| 0x4dc078 | **Pocket** on (pocket bikes) | main-menu switch; page 5 resets it for championship | 0x410450 (bike folders), 0x47bc60 (opponents' speed × 1/2.0 = 0.5 [asm 0x4dbfd8 = 2.0, 0x4d1908 = 1.0]), ghost name `P` |
| 0x4dc0a0 | **transmission: 1 automatic, 0 manual** (page 10 buttons 4 auto / 5 manual) | page 10 0x44ed30; default 1 (0x47bf30, demo) | 0x411010, 0x45bf70 (gearbox), 0x484810 [decomp] |
| 0x584499 | **bike** 0..7 of the local player (= player record 1, 0x581290 + 1×0x3209, field +0; the current player index is 0x4ea4c0). Bike table 0x50927c, 0x124 bytes a bike (4 ints = the features bars, +0x11c chooses voice 0x2b/0x2c) | page 10 (left/right; -1 → 0 on start) | 0x410a50, 0x410d70, 0x408b30/0x408f30, podium 0x452240 |
| 0x584715 / 0x584899 / 0x584a1d | `(bike + 0x5420dc) & 7`, written by page 10's frame to three records (the colours of the other riders' bikes [guess]) | page 10 | the riders' set-up [guess] |
| 0x4dc148 | **time attack** on (attackon/attackof, toggled on page 6) | page 6 only | 0x47b420 (loads the ghost), 0x412a50 (practice: saves the ghost when the run counts, 0x4138c0), HUD |
| 0x4dc060 | **difficulty: 0 easy, 1 medium, 2 hard** (texts 25-28 "DIFFICULTY : EASY/MEDIUM/HARD", options page 11 0x443200 at y 100) | page 11; config 0x42b1c9; championship load 0x455280 | 0x47bc60: opponents' top speed × 0.9 / 1.0 / **1.05** in Solo (× 1.1 in Link) [asm 0x4d1928, 0x4d1930, 0x4d1940]; 0x47dae0: time bonus table 0x515fe0[diff] |
| 0x4dbfdc | **opponents: 0 = "24" riders, 1 = "12"** (text 34 "OPPONENTS : ", strings 0x4e92e0/0x4e92e4) | page 11 (toggle at 0x444602); config | 0x47b420 (23 or 11 opponents) [asm] |
| 0x4dc058 | **all tracks unlocked** (cheat name CDNALSI; the cheat table 0x4f1250 also has CESREVER → 0x4dc050, CTEKCOP → 0x4dc054) | name entry cheat | pages 6/8 (track choice), 0x447920 |
| 0x4dc03c / 0x4dc040 (with 0x4dc054 / 0x4dc050) | Pocket / Reverse **unlocked** (show the main-menu switches) | 0x4580f0 after a championship race (from the championship table 0x508934, +0x2c / +0x28), the cheats | main menu |
| 0x509ba8 | championship: index of the current race; 0x5083b4 = races in the championship; 0x508934 = championship table (0x50 a race; +0x30 the track to unlock) | page 5 resets it to 0 (championship); 0x4580f0 increments it | 0x4580f0, pages 6/9 |
| 0x4e90c8 | "a championship is in progress / standings exist" | page 5 start (0), page 9 start | page 6 (Back → 9 instead of 5) |
| 0x4dc07c | result of 0x4580f0: 1 = the championship goes on, 0 = finished | 0x4587e0 | page 9 (where its Back leads) |
| 0x4dc1c4 | GP edition (titleGP, Grdprix pictures instead of champion) | start-up 0x414080 | menu pages only |
| 0x4e9830 / 0x4e9834 | replay playback / race kind (0 normal, 1 a `.Sav` replay, 2 demo by track name) | demo 0x409a30, event 0x4587e0 | the race throughout; page 1 |
| 0x542860.. | the race parameter / replay header (layout above), copy at 0x580830 | page 1 | 0x457bf0 (replay check), 0x47bc60, 0x410a50 (0x542874) |

**Cross-check against the demo path** (0x457bf0, when no .Sav file but a track name
matches): it sets exactly mode = 1, reverse = 0, pocket = 0, transmission (0x4dc0a0) = 1,
bike 0x584499 = 0, laps = 3, riders 0x542874 = 23, track index, kind 0x4e9834 = 2,
0x4ea4c0 = 1, and the name. The defaults in race-object slot 0 (0x47bf30) are the same
(track 0, laps 3, reverse 0, pocket 0, auto, single race, bike 0). So, before going to
page 1, a race needs: **0x4dbffc, 0x4dc094, 0x4dc08c, 0x4dc074, 0x4dc068, 0x4dc078,
0x4dc0a0, 0x584499, the name 0x5161b0, plus the options 0x4dc060 and 0x4dbfdc, and 0x4dc148
for time attack** [decomp, consistent with the asm of 0x40b8b9 and 0x47b420].

**After the race** (the race asks for a page through the event flags 0x4e8918, which
Menu_Frame step 3 hands to 0x4587e0; bit 4 = unload the race, then one page bit) [decomp,
the flag values in the asm of 0x437602]: bit 0x100 → page 5, **0x200 → page 9** (the
"TOP 5" results; build/ref/orig_after_race_top5.png, reached after QUIT RACE in a single
race; its Continue goes to page 6), 0x400 → 6, 0x1000 → 7, 0x800 → 1 (restart: reload),
0x2000 → 14 (the end of a demo), 0x4000 → 8, 0x8000000 → 24 (podium). In a **championship**,
bits 0x200 and 0x8000000 call 0x4580f0 instead. It goes to page 9 (Solo) or 24 (Link, or
0x587206 == 2). It then unlocks: Pocket → 0x4dc03c and page 17, Reverse → 0x4dc040 and page
16, a track → its +0x128 = 1. It advances 0x509ba8, and after the last race goes to page 18
(championship end). Each later 0x409eb0 call overrides the page, so the last unlock page
wins. It returns "goes on" into 0x4dc07c. The race-end routine 0x412a50 itself only shows
the in-race results (0x437b70) in Solo, or goes straight to page 24 in Link. In practice
with time attack it saves the ghost.

#### 4.12.9 Pages 7, 8, 22 (Link)

None of them can be reached on the Solo path. Every 0x409eb0 that leads to them depends on
Link: 5 → 7 only when 0x4dbffc == 0 [asm 0x4478f2]; 9 → 7/8 and 10 → 8 only when
!solo [decomp]; 0x449560 → 7/8 is called only by page 7's frame and by the unreferenced
0x444cd0 (dead code, no pointer to it in the image); 22 is reached only from 7 (0x44a35b);
2 → 7 (network page).
- **Page 7 (0x449100 / 0x449680 / 0x44a370): the network session list.** SS* pictures
  (SSFOND1, SSSCROL1, SSBARRE*, SSFLECH* arrows, SSCURSE*, SSTITRE*), `session.lez`, the
  buttons `newgame.lez` (create), `join.lez`, `Btupdate.lez` (refresh), `Btback.lez`. It
  uses the DirectPlay object 0x50dbdc and CD track 3 (0x404990(3,3)). Exits: → 2 (back to
  the connection page), → 22 (waiting), → 8 (lobby), → 26 (CD check).
- **Page 8 (0x44a930 / 0x44aee0 / 0x44aeb0): the Link race lobby.** LK* pictures
  (LKBARRE, LKFEATUR, LKAUTO/LKMANUEL, LKPOCON/OFF, LKREVON/OFF, LKLED, LKCOMPU, LKLOCK),
  `smallmap.lez` of the chosen track, accept/cancel. The host sends track name
  (0x456df0), mode/reverse/pocket/difficulty (0x456e50), laps (0x456eb0) to the others. →
  1 (loading) when everybody is ready (or 0x51a414), → 7 on leaving.
- **Page 22 (0x44a390 / 0x44a4f0 / 0x44a4e0): the network "please wait" page.** The
  paralax2 tile, one text in font 9, a Back button (Escape), and 0x4569b0 polling the
  session. → 7 (failed / back) or → 8 (joined).

#### 4.12.10 What the port needs, in order

Checked against port/src as it stands (menu.c, bitmap.c, font.c, text.c, script.c, r3d.c,
main.c). Already there and enough: picture buttons with neighbours/hotkeys/blink, slides,
tile scroll with any (dx, dy), pct_w drawing (the feature bars), the glass, the text list,
fonts with SetColors, BPT scripts, 3DF+MAP models, the camera look-at.

Pages in the order to write them, each with what it needs first:

1. **Page 5/26** (SELECT MODE): nothing new but (a) the mode/settings globals and (b) the
   delayed-sound call 0x404e10 (voice 0x54 after 600 ms) and CD track 3 — left to the sound
   work, a no-op until then. Tile scrolled (−1, +1). Page 26 = the same functions (the port
   never fails the CD check, so 26 is never reached; keep the row anyway).
2. **The race settings** (one struct for §8's globals: solo, mode, track, laps, reverse,
   pocket, time attack (initially 1), gearbox (1), bike (−1 = first visit → 0), difficulty,
   opponents, name, championship race number, 0x4e90c8, 0x4dc058 cheat) with the defaults
   of 0x47bf30 (track 0, laps 3, single race, automatic, bike 0).
3. **The track list**: LIST.BPT / LIST01.BPT through script.c (ListName, PocketAvailable,
   ReverseAvailable, NumberIslande, IslandeList: name, folder, ON/OFF, laps); the
   championship lists CHAMPA/B/C(.01).BPT (NumberEntry, LevelList: name, reverse, pocket,
   2 ints, unlock name), and helper 0x447920 (legal track; mode 2 → 1 when a championship
   is over).
4. **Menu core additions** (small): numpad −/+ in vk_of (SDLK_KP_MINUS → 0x6d,
   SDLK_KP_PLUS → 0x6b); buttons rebuilt every frame without losing the selection or a
   pending press (page 6 does 0x40bd40 + 0x40bef0 each frame; only the count is reset);
   setting the selection from a page (page 10: selected = 2 after A-T/M-T); a page change
   in the middle of a frame (page 6 goes on drawing after 0x409eb0); a typed-character
   input (WM_CHAR → 0x51be90, SDL_TEXTINPUT) for page 9; text buttons (kind 1, 0x40bd60)
   only for the Options page 11.
5. **Page 6** (SELECT RACE / CHAMPIONSHIP): pictures outside the 209-entry table loaded by
   path from the track's folder/bank (`data\ileNN\map.lez`, `photo.lez`; bitmap_load_lez
   exists) and freed by the end routine; the two laps digits (num0..num9, ids 0x1e..0x27);
   the one-frame "flash" pictures; the track-change slides (0.5 s; the change waits while
   pictures move); effect 0x28 on a track change.
6. **Page 9** (TOP 5): strings drawn directly (0x493ee0) in fonts 31, 12, 9 with
   SetColors / 0x490a30 / 0x490a60; `Data\bestscor.dat` read and written (0x20-byte key
   = track name + POC + REV + TRA<laps> in practice; one best-lap record and 8 race records
   of 0xf9 bytes; name at +9, time ms at +0xf1); name typing with the blinking "_"; deleting
   the time-attack ghost (Reset). **Decision for Danik**: the original writes
   bestscor.dat, Champ<n>.sav and the timatak ghosts into the game's own data folder; the
   port may prefer a user folder (as the config). The race-end insertion with the 'TINI'
   marker is race code, not read yet: until then page 9 only shows the table.
7. **Page 10** (CHOOSE BIKE), in two steps:
   a. Everything 2D now: choosefo background, panel and title slides, the four bars eased
      over 1000 ms (pct_w = 100 − Gfx value, from the carac*.spt bike tables: a small
      reader for 4 ints + PersoAnim of the 8 bikes of the family), arrows (hotkeys Left/
      Right, no wrap at 0 and 7), A-T/M-T blinking radio pair setting the gearbox, Start
      (effect 0x2c road / 0x2b dirt then voice 0x58, waits their length in ms → page 1
      **without** the glass), Back (Escape → 6 with the glass). The family comes from the
      track's Level.Spt (and Pocket): MOTOSPD / PKTSPEED / MOTOKTM / PKTKTM.
   b. **The 3D bike on the turntable** — the biggest missing piece, and the race needs it
      too: the bike+rider object (0x45bf70 family: 3DF/MAP parts, .SKL/.PIV skeleton, .ROT
      rider animation, the family's texture banks, skin index (bike + offset) & 7), its
      drawer 0x45f290, the camera orbit 0x46dab0 (distance 1500, y +200 aim / +240 eye,
      one turn per 10.24 s, dt capped 1000 ms), light (−0.5, −1, 0), the sideways whip
      on a bike change. Until it exists page 10 can run without the bike.
8. **Page 1** (loading): one frame — `<track dir>load.lez` full screen and loadtxt.lez at
   (0,408) (lodanim and the badges come with the load's steps, 0x4470d0), present,
   then the menu teardown (all started pages' end routines, menu inactive) and the race
   load. Until the race exists: show the picture and end with a message, as unwritten
   pages do.
9. **Coming back from the race** (later, with the race): the race-end event flags (§8):
   page 9 after a single race / practice (and in a championship through 0x4580f0), the
   unlock pages 16/17/18 and the podium 24; re-entering the menu does the first-entry
   set-up again.
10. **Pages 20/21** (championship load/save, `Data\Champ<n>.sav`, 8 slots, font 12 list)
    and **Options 11** (+12 controls, 13 info, 23 data disk) are off the shortest path to
    a race and can come after; 11 is where difficulty and the opponents count are set.

Nothing new is needed in the text-effect code for these pages (no TextList_Add on 5, 6,
9, 10 or 1; page 9 draws its strings directly). The list widgets are only on 20/21 (a
fixed list of 8 save names) and the network pages.

#### 4.12.11 Captures of the original (build/ref/, 640x480, taken 2026-10-26)

How: `tools/orig.sh start`, then keys and **mouse clicks both work** (xdotool
`mousemove --window <win> X Y` then `click 1`; game coordinates = window coordinates).
Keys sent in the first seconds of the logo are lost: wait ~5 s before Escape. The default
name is confirmed by Return on page 15 (sometimes it needs a second Return). Careful: two
helpers using `tools/orig.sh` at the same time share the Weston socket, the wine prefix and
`wineserver -k`; one restarted the other's game here. A private instance (own copy of
build/orig and the prefix, own Weston socket) avoided it.

| file | what |
|------|------|
| orig_p3_main_link_hl.png | main menu, Link highlighted (pointer over it) |
| orig_p3_to_p5_glass.png | glass transition 3 → 5, ~0.6 s in |
| orig_p5_mode.png, orig_p5_mode_back.png | SELECT MODE settled (Championship / Back selected) |
| orig_p5_mode_single_sel.png, orig_p5_mode_practice_sel.png | Single Race / Practice selected |
| orig_p5_to_p6_glass.png, orig_p5_to_p6_champ_glass.png | glass 5 → 6 (Single; Championship, map and photo still sliding) |
| orig_p6_track.png | SELECT RACE, Single Race, Speed Bay, 04 laps |
| orig_p6_track_hover_right.png, orig_p6_hover_continue.png | right arrow / Continue hovered |
| orig_p6_track_snowride.png | after the right arrow: Snow Ride, 02 laps |
| orig_p6_practice_speedbay.png | Practice: laps −/+ and TIME ATTACK ON (stamp on the photo), 03 laps |
| orig_p6_practice_ta_on.png, orig_p6_practice_ta_off.png | time attack on / off |
| orig_p6_practice_laps5.png | laps 5 (> default 4): the time-attack switch gone |
| orig_p6_championship.png, orig_p6_championship_next.png | CHAMPIONSHIP: Continue / Load / Back, no arrows |
| orig_p6_after_race.png | page 6 again after TOP 5 → Continue |
| orig_p6_to_p10_glass.png | glass 6 → 10 |
| orig_p10_bike_a.png, orig_p10_bike_back.png, orig_p10_bike_b.png | CHOOSE BIKE (dirt bikes on Snow Ride), turntable, bars |
| orig_p10_manual.png | M-T chosen |
| orig_p10_champ_roadbike.png | road bike (Speed Bay) |
| orig_p1_loading_snowride.png, orig_p1_loading_speedbay.png | page 1 |
| orig_race_snowride_start.png, orig_race_speedbay_champ.png, orig_race_escmenu.png | race start, Escape menu |
| orig_after_race_top5.png, orig_p9_top5.png | TOP 5 (Single Race) |
| orig_p9_top5_champ.png | TOP 5 in a championship: Save / Abort / Next race |
| orig_p11_options.png | OPTIONS |

#### 4.12.12 In the port (2026-10-26)
- `choices.c`: the globals of §4.12.8 with the program's initial values (Solo, Practice,
  track 0, 3 laps, time attack on, automatic, difficulty and opponents from Config.Spt),
  the track list (`tracks_load`, 0x428d60, at start-up) and the championship
  (`champ_load`, 0x429a00: at start-up and on every main-menu start, champ<A+difficulty>),
  `track_legal` (0x447920, which stops where it was called from, as the machine code does).
  The program stops with a message when the lists cannot be read, as 0x472c40.
- `page_mode.c` (5 and 26) and `page_track.c` (6). Page 6's presses are handled in the
  middle of the frame (the flashes drawn over the buttons, ChangePage before the glass);
  the frame's own redraw request 0x409bf0 (menu +0x30/+0x34) is not needed.
- Numpad − and + are Windows' VK_SUBTRACT 0x6d / VK_ADD 0x6b.
- Checked against the captures (tools/cmpshot.py, `OMR_TILE` for the tile's scroll, which
  depends on how long the earlier pages ran): SELECT MODE with Championship highlighted;
  SELECT RACE Single, Practice (03 laps, TIME ATTACK ON), Practice at 5 laps (the switch
  gone), Snow Ride after the right arrow, CHAMPIONSHIP: every pixel the same but the
  port's pointer. In tools/shots.sh.
- `bikes.c` + `page_bike.c` (10): Level.Spt → the family script (the Pocket one with
  Pocket) → the 8 carac scripts (MotoType, Gfx×4, 67 floats, PersoAnim); every 2D part
  of CHOOSE BIKE, the bars' 1000 ms sine ease, the turntable's angle and the bike-change
  whip kept as the game keeps them, but **the bike itself is not drawn yet** (§4.12.10
  7b). Matches orig_p10_bike_a / orig_p10_manual outside the bike.
- `scores.c` + `page_top5.c` (9): bestscor.dat read as 0x464e80 / 0x4656c0 (the key
  compared whole, strcmp; a missing block fills the 8 races with 0x7f); the Solo page
  drawn as the frame draws it (fonts 31, 12, 9; `font_set_rgb` = 0x490a30 + 0x490a60 +
  slot 1). The 'TINI' rows, the name typing, the saving and Reset's ghost deletion wait
  for the race (and the decision where the port keeps saves). `g_champ_on` = 0x4dc07c
  (initial 1). Every pixel the same as orig_p9_top5 and orig_p9_top5_champ but the
  pointer.
- `page_load.c` (1): the first frame as the game draws it, then one load step
  (`loading_progress`, 0x4470d0: badges, the logo on every other step); since the race
  is not written yet it then waits 2 s and goes back to the main menu (the original's
  path when a race fails to load). Same as orig_p1_loading_snowride.
- `saves.c` + `page_champ.c` (20, 21): the saves go to the player's folder (`-home`,
  `OMR_HOME`, else `$XDG_CONFIG_HOME` or `~/.config/openmotoracer`) in the game's own
  format and names; reading takes that folder's file first, else the game's `data/` one
  (bestscor.dat too). A default until Danik decides (HANDOVER). The title's unused bytes
  are written as zeros. `tools/shots.sh` sets OMR_HOME to `build/shots/home`, emptied.
- Test switch `OMR_CHOICES=MODE,TRACK,LAPS[,REVERSE,POCKET]`; `OMR_TILE` sets every
  tile picture (paralax1/2, CHFOND...).


#### 4.12.14 The championship's end: pages 16, 17, 18 and the podium, 24 (2026-10-30)
Pages 16 (Modrever.lez, id 199), 17 (Modpock.lez, 198), 18 (podium.lez, 200): the picture, Continue
(id 0x4d) at (496, 442), Return; then page 24. No glass (no 0x40c4a0 in their frames).
Page 24 (0x452240, 0x452f80, 0x452eb0; `page_podium.c`); its data from 0x452090 when the race ends:
- Up to 3 riders by place: name ("Computer", text 183, unless driven), time, livery; what nobody wore is
  rand() & 7. 0x4e911c = the local place + 1, 4 at most; if it is above the count, the count is it.
- 0x40a4d0 again (the family), then each rider's record with `+0x30c1` crashed = 1 so the drawer draws the
  rider at `+0x30b5` by `+0x3085` and the bike (placed at z = 10000) behind the camera: (0, B+50, 0),
  (100, B+11, 0), (-100, B-2, 0), and the 4th (-50, G, 300), turned 135 degrees (B 0 / 30, G -258 / -178 for
  road / dirt). Detail 1 forced.
- The stand: `data\podium\podium` x 0.04, its pages + 2 (banks Bnk00_01/02 in pages 3 and 4), angle (0, pi/2,
  0) at (0, 72, -80). 0x401160 is thiscall: ECX is the page object, which Ghidra's text hides.
- The camera, 7 s: (+-1800, 500, 200) to (0, -50, 1500) (place 4: (0, -50, 2120)), looking at (0, 120, 0)
  (place 4: y 100); rand() & 1 picks the side.
- Each rider's arms: t = i x 0.2, +-6 per second, bouncing 0..1, between two poses by 0x45e390 (place 1 or 4:
  17/18, 19/20, 21/22, 23/24; place 2: 27/28, 17/18, 21/22; place 3: 25/26, 19/20, 17/18).
- The 2D (bestRace.lez, lines, numbers, names and times in font 12 from t >= 0.6, one row more at 0.7 and 0.8;
  Solo: the player's row alone; Continue at (496, 429), Escape; then page 9) is drawn **before** the 3D is
  rendered, so in the software renderer the wall covers all of it. The original's capture shows none of it either.
- Sound: ids 4, 5 (0x404310), the four objects 0x4e9128 (§4.10.6.2) a frame; the player among the three:
  music 12, else music 11 and voice 0x5b.
- Compared: `build/ref/orig_p24_podium*.png`, taken by forcing a finish (poke place 0x5873a6, state 0x587206,
  0x509ba0 = 1 in a championship race; `n` at the replay question). Settled, the stand differs from the port's in no
  pixel; only the riders (the moment of their arm movement, their liveries) and the pointer differ.
  **The player out of the three (the 4th rider)**, captured 2026-10-31 (a championship race, the cursor on Continue with
  Up three times since Return is not its key; place 5 poked): the stand and camera in no pixel different from `OMR_PODIUM=4`, the
  kneeling rider's shape and its arm's movement the same; only the liveries differ, the test's own.
- 0x4dc078 (saved, 0, restored) is the Reverse switch: nothing the port's family load reads.

### 4.13 The race's track: files, the .trk, how the race draws it (mapped 2026-10-26)

Marks: **[asm]** read instruction by instruction, **[decomp]** from Ghidra's C, **[test]**
checked by running/rendering (the original under wine, frozen, its memory dumped),
**[guess]**. Tools: `tools/track.py` (read, summarise, top views, a race view),
`tools/track_raster.py` (the race's span kinds as `tools/model.py`'s renderer extended),
`tools/track_horizon.py` (the far background); captures in the private
`build/trackcap/` (`orig_track.sh`, `dump.sh`, `loop.sh`).

#### 4.13.1 Files, the .trk format, which pieces are drawn

Confidence marks: **[asm]** read instruction by instruction in `build/mr.asm`, **[decomp]**
from Ghidra's C (fine for control flow, not for x87 numbers), **[test]** checked by
running/rendering (the original under wine, frozen and its memory dumped), **[guess]**.

Tools (mine): `tools/track.py` (read, summarise, top views, a race view through the
renderer of `tools/model.py`), `tools/track_horizon.py` (the far background,
written by a helper), `tools/track_raster.py` (the race's span kinds, written by
a helper), `build/trackcap/` (captures of the original's race with memory dumps, private
runner `orig_track.sh`, `dump.sh`, `loop.sh`).

```
python3 -I tools/track.py info  "Moto Racer/data/ile01/ile01.trk"
python3 -I tools/track.py tops  "Moto Racer/data" build/png/tracks
python3 -I tools/track.py view  TRK BKF out.png  x y z  tx ty tz  [focal]
```

---

##### 4.13.1.1 What a race loads, in order

Menu page 1 (the loading page) `0x40b3f0` [asm/decomp]:
1. `"%sload.lez"` (the loading picture, Bitmap_Load), sets `*0x4dc0c4 = 0` (race kind
   remap, §5.4).
2. `"%sLevel.Spt"` read by the script reader `0x427e80` (fields in §3).
3. The race object's vtable +4 = `0x47b420(track index 0x4dc08c, mode)`:

| # | routine | what it reads / does |
|---|---|---|
| 1 | `0x4191a0(0x52c0d8)` | player list / bikes chosen (menu state) [decomp] |
| 1b | `0x4136a0` | time-attack ghost, only if `0x4dc148` [decomp] |
| 2 | `0x4191a0(0x52c0f0)` | second list [decomp] |
| 3 | `0x4100f0` | resets [decomp] |
| 4 | `0x410120` | cameras 0..3: `0x46da40` sets offset (0, 191, −484) (+0x50..+0x58), +0x114/+0x118 = MediumZone/HightZone of the detail row (§3) [asm] |
| 5 | `0x410270` | Level.Spt again through `0x427e80` [asm] |
| 6 | `0x410450` | the bike script: `FileMoto` 0x51d068 or `FilePkt` 0x51d0a8 through `0x429e20` (FileBnk0/1, FileMotoMed/Low/VeryLow, wheels, Skl…), then the 8 bike entries at 0x519de4 through `0x42a580` (Materials…) [decomp] |
| 7 | `0x410590` | **texture pages**: counter reset `0x401f90`; page 0 = bike bank 0x518b20, page 1 = 0x518b60, **pages 2..19 = the level's `bank\bnk00_02.raw` … `bnk00_19.raw`** (entries at 0x51bfe8 + k·0x40) through `0x401d90` → `0x401160` [asm] |
| 8 | `0x40d650(0x51cee8)` | **the track `.trk`** (`FileLand`), into the track object 0x52c108 (§2) [asm] |
| 9 | — | counts the checkpoints → 0x4e9844 [decomp] |
| 10 | `0x410350` | the horizon: HorizHi/HorizMed/HorizLow.raw, only the one for ScreenMode 0x4dc0c8 is kept (§4.13.3 §1) [asm] |
| 11 | `0x410a10` | `0x45b340` per entry of 0x5191e0 (reads files; not followed) |
| 12 | `0x410a50` | bikes: `0x45bf70`, start positions `0x45c7c0` from StartPosition 0x51bfd8.. [decomp] |
| 13 | `0x410d70` | the WAV samples (sound helper's topic) |
| 14 | `0x47b600` | podium pictures `FINAL\1rst`, `2nd`, `3rd`, `Laureat` (`0x420a50`, `0x420250` recentres) [decomp] |
| 15 | `0x410e10` | renderer set-up: near 0x625774 = **64.0**, far 0x625788 = 10000, 0x62e438 = 2000 max faces, 0x62e444 = 1 (s-buffer on), 0x6d1f70 = FlatZ[detail]; `0x4592a0` screen: **fx = W·394/640, fy = H·394/480** (0x49a0e0/0x49a100), centre W/2, H/2 (0x499ba0) [asm] |
| 16 | `0x40faf0`, `0x4020b0` | checksums of data [decomp] |
| 17 | `0x410ee0`, `0x4100a0` | final resets [decomp] |

**Dead file names** [asm, grep of every reference]: `FileFrame3d` 0x51cf28, `FileDecor`
0x51cf68, `DecorInv` 0x51cfa8, `FileNet` 0x51cfe8, `FileMap` 0x51d028, `FileBalise`
0x51d0e8, `FileSmallMap` 0x51d128 are only written by the script reader, never read; the
files are not shipped either. The minimap is a model inside the track object (+0x28,
`0x40ed10`). Paths: every file is tried with the HD prefix 0x4e8380 then the CD prefix
0x4e8480.

So, for one track, from `data/ileNN/`: `load.lez`, `Level.Spt` (BPT), the bike scripts
and banks (shared), `bank\bnk00_02..19.raw` (from the level's BKF), the `.trk`,
`bank\HorizHi.raw` (or Med/Low), samples, `FINAL\*` pictures.

##### 4.13.1.2 The .trk format (loader 0x40d650) [asm, test: all 10 tracks parse to the last byte]

All little-endian. Every shipped track is version 5. In-memory offsets in brackets.

```
"TRK\0"  i32 version  i32 nsec
nsec sections (0xfc bytes in memory, array at track+0):
  3f   pos                       [+0x00]
  9f   axes rows X (across), Y (up), Z (along the road)   [+0x0c]
  3f   offset (always 0)         [+0x30]
  u32  block (road model index)  [+0x3c]   = i/4 almost everywhere (4 sections per block)
  4x4f planes (n, d)             [+0x40 +0x50 +0x60 +0x70]  rebuilt at load by 0x40e5c0
  f    racing line, lateral offset from pos along X   [+0xb0] (AI, 0x46b120 [decomp])
  v>0: i32 n, n x u16 decor indices  [+0xb4, count +0xd4, at most 16]
  v>3: f left edge, f right edge     [+0xf0, +0xf4]
i32 nblock, nblock road models (0x70 bytes each in memory, array track+8, count +0x10):
  model (0x422a20, faces on): i32 nv, nv x (3f pos, u32 dropped),
        i32 nf, nf faces (0x423240, version < 6 form):
        u16 kind [+0], u16 flags [+0x38], u16 colour RGB565 [+2], u16 texture page [+0x36],
        5 u16 corners [+0x2c] (5th = 0xffff: triangle; else quad, 5th unused),
        5 x (f u, f v) in texels [+4]
v>0: i32 nlib, nlib library entries (0x79 bytes; three arrays 0x4dc0d8/dc/e0, count 0x509b9c):
        i32 len, name, 3 models: near / medium / far   (name at +0x70)
     i32 ndecor, ndecor decor objects (array 0x4dc0e4, count 0x4dc0ec):
        i32 len, name, 3f pos, 3f angles (a, b, c), i32 lib, v>4: i32 animation
v>1: i32 n2, n2 records for sections 0..n2-1 (n2 = nsec on every track):
        i16 checkpoint number (-1 none) [+0xd8], u16 time [+0xda] (replaced by
        TimeCheckPoint from Level.Spt), u16 ? [+0xde],
        v>3: i32 event [+0xec], i32 ground off road [+0xe8], i32 ground on road [+0xe4],
             i32 flag 0/1 [+0xe0]
v>2: i32 horizon offset -> 0x4db2b0, f32 horizon scale -> 0x4db2b4 (§4.13.3 §8)
     "AnimDecor\0" block (0x48b7e0): i32 version, i32 n, n records of 0x20 (or 0x40)
        bytes: word0 = decor | type<<16 | sub<<24; type 7 with version > 2 adds i32 nframes,
        i32 npoints, frames of 3f (paths) [asm for the reader; what they mean: §4.13.5 §3]
```

Section fields as the game uses them:
- **Section walk** `0x40c7e0(hint)`: from the hint, step forward while the point is on the
  positive side of the next section's start plane (+0x40), backward while negative of its
  own [asm]. `0x40c740` = nearest section by distance (no hint) [asm].
- **Planes** (+0x40..+0xa0, 7 of them): rebuilt from pos and axes with ±2048 offsets
  (0xc5000000/0x45000000) by `0x40e5c0` [asm].
- **Road or not** `0x40ec60`: lateral = X·(p − pos); within [+0xf0, +0xf4] → ground
  material +0xe4, else +0xe8 [asm].
- **Events** `0x486240`: code +0xec 0..5 (16..21 when racing in reverse, 0x4dc140 = −1)
  set flags 0x5d2edc.. with 75.0 [asm] — the turn signs on the screen (§4.13.5 §2);
  6..12 toggle bits of the zone mask +0xf8 [asm].
- +0xde: no reader found; looks like leftovers [guess].

Numbers per track (`track.py info`): 435 to 1048 sections, 110 to 262 blocks, 8 000 to
16 800 road faces, 41 to 257 decor objects. Road face kinds used: 0 (flat colour), 16 and
21 (textured), 38 and 39 (textured, see-through/other; §5.4). Texture pages 2..19 only.

**Top views** `build/png/tracks/ile01..ile11.png` and `sheet.png` [test]: every track is a
**closed circuit**: the last section ends 271–922 units from the first (sections are
about 500 units long). Road faces filled with their colour field, white road edges from
X × (+0xf0, +0xf4), red centre line, blue racing line, yellow decor, green checkpoints.
No track is point-to-point.

##### 4.13.1.3 Level.Spt fields the race uses (0x427e80) [asm]

NewMotorEngine 0x52c234, the file names (above), the bank list (entries: name, D3D twin or
NULL, int flags → 0x51d168 + 4k), StepGlobal 0x4dbfe0, YScaleLevel 0x516aa8,
StartPositionX/Y/Z 0x51bfd8.. (ints made floats), ComputeLandOptimise 0x5d2eac, AI tuning
0x5427b8.., NbComputer 0x4dbfcc, ComputerSpeed, TimeSkill, NbCheckPoint 0x4dbfd0,
TimeCheckPoint 0x509228.. (overwrites section +0xda), FactorPocket 0x4dbfd8, AudioTrack
0x4db67c, and **the detail table**: 5 rows × 9 floats at 0x51d1b8 (row stride 0x24), the
row = CurrentDetail 0x4dc098 from CONFIG (4 in the GOG install):

| col | name | ILE01 rows 0..4 |
|---|---|---|
| 0 | LenRLPlane (side planes) | 3500 … |
| 1 | LenBPlane (far plane) | 18000 … |
| 2 | FlatZ → 0x6d1f70 | 4000 3000 4000 4000 8000 |
| 3 | HorizFlatZ | |
| 4 | MediumZone (int) → cam +0x114 | 5500 … |
| 5 | HightZone (int) → cam +0x118 | 3500 … |
| 6 | gouraudTexturedZ | 0 0 0 4000 32000 |
| 7 | texturedZ | 0 0 8000 12000 48000 |
| 8 | gouraudZ | 0 4000 12000 22000 52000 |

Columns 6..8 are copied every frame by `0x47ff90` into 0x6d20c8 / 0x62435c / 0x6d20d0,
and FlatZ into 0x6d1f70, but **no reader of those four globals was found** by address
[asm grep]; they may be read through a pointer, or be dead (the software path's choice of
span is the kind remap, §5.4). Open.

##### 4.13.1.4 Camera [asm/decomp, test]

Camera blocks at 0x5161c0 + i·0x238 (i = 0..3):
`+0` bike pointer, `+4` bike index, `+8` mode (7 chase `0x46f840`, 3 TV/look-at, 6, 1),
`+0x50..+0x58` offset behind the bike (0, **191**, **−484**), `+0x68` **C** (3×3, camera
coordinates = (world − pos) · C, forward = column 2) [test], `+0x8c` T (0 in every dump),
`+0x98` position, `+0xa4` target, `+0xc0` angle (billboards use it), `+0xd4..+0x110`
frustum planes (`0x46e320`: far = LenBPlane, sides from 0.6·0x6d1f74), `+0x114/+0x118`
MediumZone/HightZone, `+0x1dc` 64.0 ×8, `+0x224..` chase extra distances (0, 128, 256,
512). Chase lerp constants 0x4d1404 = 0.2, 0x4d1408 = 0.5, 0x4d140c = 1/3 [decomp].
Replay/TV cameras `0x46e9f0` zoom through `0x49a120` (a dump had ×3: fx 1182); `0x4583e0`
resets zoom to 1.

Projection (the same as NOTES §4.9): **fx = fy = 394 at 640×480** (0x6d20c4), centre
(320, 240), **near 64**, far 10000 (clip), 2000 faces at most.

##### 4.13.1.5 How the race draws the track

###### 5.1 The frame `0x47ff90` [asm/decomp]
`0x459130`; the zone globals (§3); `0x4816c0`; **`0x402960` the horizon** (software: before
the lock `0x49a160` — it is blitted straight onto the back surface, the 3D is drawn over
it); **`0x40c7e0` current section** with the player's section as hint (`0x40c740` nearest
when there is none); **`0x46e320` frustum planes**; **`0x40ce60` road blocks + decor**;
`0x458b80` (if DrawLand 0x4e9900) per visible section — the bikes/shadows [guess];
`0x4575c0`/`0x4573e0`; with param & 8: `0x484810`, `0x40f2d0`, `0x486310`, `0x47b7a0`;
**`0x49ad90` render** (buckets → s-buffer → spans, NOTES §4.9); HUD; `0x49b0e0` flip.

There is **no sky model and no fog**: the sky is the horizon picture plus two colour fills
(§4.13.3); everything else is road blocks and decor models.

###### 5.2 Which blocks are drawn: `0x40ce60` [asm, test]
From the camera's section s and its block b0 = s.block, with forward f = C column 2 and
the section's start-plane normal n:
- f·n ≥ 0.25 (looking along the road): **10 blocks ahead, 1 behind**;
- −0.25 < f·n < 0.25 (sideways): 5 and 5;
- else (looking back): 1 ahead, 10 behind.
(10 = track +0x24, set by the constructor `0x40c560`.)
Blocks drawn: b0, b0+1 … b0+ahead, b0−1 … b0−behind (wrapping). Each block model is
drawn with the identity rotation at its own world coordinates. The decor list: the decor
indices (+0xb4) of sections s … s+4·ahead+4, then s−1 … s−(4·behind+4); each decor object
drawn once. **Decor only when the detail level ≠ 0.** Library entries named `CAMERA…`
are skipped (`0x473240`) [asm].

###### 5.3 Decor objects (class vtable 0x4d0110) [asm]
- init `0x4022a0`: pos +0x20, angles +0x14, model A +0x4c, visible +0x2c.
- vtable 0x4d0110: +0 0x402230 (destructor), +4 0x4022a0 init, +8 0x403190, **+0xc
  0x4022f0 = what `0x40ce60` calls**, +0x10 0x403180 [asm, pefile].
- `0x4022f0` (the race's draw) **always draws the near model +0x4c**: matrix `0x403880`,
  `0x49b2c0(.., model+4, model+0x30, 0x1f)` then `0x49b7f0(.., faces model+0 + model+0x68·64,
  count model+0x2c − model+0x68, 1)` [asm]. My render does the same and matches [test].
- `0x403190` (vtable +8, the same draw with LOD; caller not found): distance d (ftol of
  the length) < cam+0x118 (HightZone) → +0x4c; else < cam+0x114 (MediumZone) → +0x54;
  else d < 0x1400 → +0x58; else nothing (a missing model falls through to the next)
  [asm]. Probably the D3D or an older path [guess].
- matrix `0x403880` from angles (a, b, c), with s/c of each:
  R0 = (cc·cb + sc·sb·sa, sc·ca, sc·sa·cb − cc·sb),
  R1 = (cc·sb·sa − sc·cb, cc·ca, cc·sa·cb + sc·sb),
  R2 = (sb·ca, −sa, ca·cb).
- billboards: if model +0x5c == 1, angle b = cam +0xc0 (faces the camera). Nothing in the
  race ever sets it: dead code, §4.13.5 §1.
- animated objects (+0x30) go through `0x49b490`. Also never set in the race: §4.13.5 §1.
  What AnimDecor moves goes through the ordinary path, §4.13.5 §3.

###### 5.4 Kinds in the race: `0x49c590` remap [asm]
`*0x4dc0c4 == 0` in the race (1 in the menus). With the property table 0x6d1f88 and the
reverse map 0x624b58 (built in `0x4995a0`):
- detail 4: a face whose sort key < 6144 gets props | 0x20 → **16→31, 21→32, 38→53,
  39→54**: the near road is drawn with **perspective-correct spans** (span sub-type 6,
  divide every 16 pixels: `0x4a5c40`, `0x4a5e90`, `0x4a53b0`, `0x4a5610`; polygon
  `0x4a0f90`, edges `0x4a0260`/`0x4a04d0`);
- detail 3: the same below key 3072;
- detail 0..2: textures dropped (16/21 → 0 flat colour, 38/39 → 37) unless detail 2 and
  face flags & 8.
Sort key = the face's farthest Z, +1024 when flags & 2; bucket = key / 4 (as the menu's).
The sub-6 spans, clip and edge rules in full: `§4.13.2`  [asm].

###### 5.5 Gouraud? No [asm, §4.13.2 §3]
The light values (0x68b00c, 0x6d1f80, 0x655d30) are read only by polygon routines to fill a
batch field and by span kinds the tracks do not use; **no span of kinds 0, 16, 21, 28, 31,
32, 37, 38, 39, 50, 51, 53, 54, 62 reads them**. No gouraud, no fog on the tracks. The
gouraud*Z globals written per frame are therefore without effect in the software race.
Flat colour (kind 0 and the stencil kinds 37/51) = face colour & 0xFFDF.

##### 4.13.1.6 Rendering a track in the tool, and the check against the original [test]

`track.race_view()` takes a camera (pos, C, focal), finds the section like the game,
picks the blocks and decor like `0x40ce60`, transforms with the menu renderer of
`tools/model.py` set to the race constants (focal 394, near 64), remaps kinds like
`0x49c590`, draws the horizon (`track_horizon.draw_horizon`, offset/scale from the .trk),
textures = the level's BKF pages 2..19.

Captures: the original run privately (`build/trackcap/orig_track.sh`, its own Weston
socket and wine prefix), frozen with SIGSTOP every 3 s and its memory dumped
(`dump.sh`: camera blocks 0x5161c0, track 0x52c108, visible list 0x5dabc8, globals,
screen 0x6d1040/0x624f80/0x625770/0x6d1f70/0x6d20c0). The tool renders with the dumped
camera; `*_cmp.png` = original | tool:

| capture | track, camera | section game / tool | pixels off by > 24 |
|---|---|---|---|
| p2 | West Way, TV (×1) | 594 / 594 | 17 944 |
| p3h | Snow Ride, chase, horizon | 51 / 51 | 28 249 |
| p7h | Snow Ride | 129 / 129 | 39 825 |
| q14h | West Way, chase | 5 / 5 | 33 121 |
| q15h | West Way, TV ×3 (fx 1182) | 12 / 12 | 36 839 |
| q17h, q19h, q20h, q22h | West Way | 131/130, 170/169, 254/253 | 29 440 … 53 864 |

(of 307 200), with the menu's affine spans for every kind. **With the race rasterizer**
(`track_raster.RaceRenderer`: sub-6 perspective spans for
31/32/53/54, kind 0 flat, 37 stencil) the same captures, `*r_cmp.png`:

| capture | pixels off by > 24 |
|---|---|
| q14r (West Way start, chase) | 18 201 |
| q15r (TV ×3) | 15 537 |
| q17r / q19r / q20r / q22r | 15 079 / 18 985 / 21 333 / 12 862 |
| p3r / p7r (Snow Ride) | 14 059 / 17 280 |

What is left is almost all the bikes, riders, shadows and HUD (not drawn by the tool):
the road's lane marks, the canyon walls, the buildings and the horizon line up with the
original by eye. The text below describes the first, affine, attempt. The land, decor, horizon picture, its height and the fill colours line up
with the original; what differs is the bikes/riders/shadows/HUD (not drawn by the tool)
and the near road: the tool's fallback draws kinds 31/32/53/54 with affine spans, so the
road's lane markings zig-zag where the original's are straight, and a canyon wall face
crossing the near plane (q20, ILE02 block 42 faces 34..37, kind 16 → 31) comes out as a
dark wedge at the top right where the original shows the lit wall. The game's decor list
in the q20 dump (0x5dabd8, 53 entries: sections 170..214 then 169..162) is exactly the
§5.2 rule for section 170 [test]. The section is the game's
or one less (the game walks from the previous frame's section with the bike's position;
the tool starts from the camera — expected, not a bug).

##### 4.13.1.7 Open questions

- `0x458b80` (per visible section; bikes and shadows?).
- The chase camera's exact motion (`0x46f840`): lerps 0.2/0.5/(1/3), extra distances.
- FlatZ 0x6d1f70 and the gouraud zone globals: written, no reader found (no gouraud
  span is used on tracks, §5.5) — probably D3D-only.
- Kinds 28 and 50 (spans that draw nothing): invisible occluders? Not seen in the
  shipped road faces (kinds 0/16/21/38/39 only).
- +0xde in the second section table.
- Who calls the decor LOD method (vtable +8, 0x403190); in the race frame only +0xc is
  called, so the medium/far library models look unused in the software race.

##### 4.13.1.8 What the port needs, in order

Done (§4.13.4): the .trk, pages 2..19, the section walk, block/decor visibility, the race's
renderer set-up and kinds, the horizon, the decor matrix, a still test camera.
Left: the minimap model (+0x28). (The chase camera: §4.20 §2.5. AnimDecor and the event
signs: §4.13.5. Billboards do not exist in the race.)

#### 4.13.2 The race's rasterizer (sub 6 perspective, kind 0 flat, kinds 37/51/62)

Read from `build/mr.asm` (decomp only as a guide). Marks: [asm] read instruction by
instruction, [skim] read the shape only, [guess] not proven. Python reference:
`tools/track_raster.py` (`RaceRenderer`, a `tools/model.py` Renderer subclass; used by
`tools/track.py`). Tested on ile01 at detail 4 and 2: no
errors, the road now has perspective-correct texture, kind 37 palms are leaf shapes.

##### 4.13.2.1 Dispatch (0x4aef20) [asm]

Bucket entry +0x10 == 0 and 0x6d20a0 == 0 → entry = 0x4f1a70 + face(+0x3a kind)·16
{id, sub, flag, span}; id < 0 → skip. `sub == 0` → the span field is the *polygon* routine,
called (vertices, face) directly (kind 0 → 0x4a3950). Else 0x68b010 = flag, 0x624358 = id,
0x655d08 = span, 0x6d20bc = sub.edge (sub.edge_seethrough if flag), call sub.polygon.

| kind | sub | flag | span | what it draws |
|---|---|---|---|---|
| 0 | 0 | 0 | 0x4a3950 (polygon) | flat colour |
| 28 | 6 | 0 | 0x4a5090 `ret` | nothing, but closes the s-buffer |
| 31 | 6 | 0 | 0x4a5c40 | texel |
| 32 | 6 | 0 | 0x4a5e90 | texel halved |
| 37 | 4 | 1 | 0x4a50c0 | face colour where texel != 0 (affine) |
| 50 | 6 | 1 | 0x4a50b0 `ret` | nothing (segments still recorded) |
| 51 | 6 | 1 | 0x4a5150 | face colour where texel != 0 |
| 53 | 6 | 1 | 0x4a53b0 | texel, 0 = hole |
| 54 | 6 | 1 | 0x4a5610 | texel halved, 0 = hole |
| 62 | 6 | 1 | 0x4a5890 | ((screen>>1)&m) + ((texel>>1)&m), 0 = hole |

Sub 6: polygon 0x4a0f90, edge 0x4a0260 (opaque) / 0x4a04d0 (see-through) [asm].
Kind 5 → 0x4a4bc0 (`ret`).

##### 4.13.2.2 Colour (all of 0, 37, 51) [asm]

0x49e510, 0x4a0f90 and 0x4a3950 all do: c = face+0x02; c555 = ((c & 0xffc1) >> 1) | (c & 0x1f);
0x402020 turns 5:5:5 back into the screen format → **net colour = face.colour & 0xFFDF**
(green's lowest bit cleared) → 0x624364 (low word).

##### 4.13.2.3 Light / gouraud (question 3) [asm]

Readers of 0x68b00c (avg light of 3 corners, int /3) and 0x6d1f80 / 0x655d30: only the
polygon routines (to fill batch +0x20), sub 5 (0x49ed50–0x49fa10), 0x4a17d3–0x4a25c4,
0x4a6100+, 0x4ac0cb–0x4ae6d4 and 0x4adc1f. **No span used by kinds 0, 16, 21, 28, 31, 32, 37,
38, 39, 50, 51, 53, 54, 62 reads them**, nor does kind 0's edge 0x4a2f10. No gouraud on
tracks. (The vertex +0x0c from 0x49b2c0's last argument is only averaged and stored.)

##### 4.13.2.4 Kind 0 (0x4a3950 → 0x4a34f0 → 0x4a2f10) [asm]

Already in `tools/model.py` (`face_flat`, `flat_triangle`, `flat_row`) and right for the race
too (it reads `model.NEAR`, set to 64 by track.py): fan (0,1,2) then (0,2,3) if face +0x34
!= 0xffff; near cut per triangle like 0x49e510; no area test; 13-bit edges with C division;
s-buffer row fill [xl, xr). The MMX twin 0x4a31d0 (if 0x4dbcf4) is the same fill.
Screen outcode pretests only drop "all x < 0" / "all y < 0" triangles: no pixel effect.

##### 4.13.2.5 Sub 6 polygon 0x4a0f90 [asm]

Per fan triangle (0,1,2), then (0,2,3) if face +0x34 != 0xffff (the 5th index):
- corner uv: face (+4 + 8k) floats × 256.0 (0x4d1fb0) = texels (= track.py's `uv`), then
  `uz = f32(u · rz)`, `vz = f32(v · rz)` with rz = vertex +0x18 = f32(1/Z), which 0x49b2c0
  stores for **every** vertex (fld1 / fdiv Z), in front or not.
- Z < near (0x625774; race 64) per corner. Rotations exactly as 0x49e510 (`model.face`):
  none behind → tri(c0,c1,c2); one behind B with F1 = (r+1), F2 = (r+2) → tri(F1,F2,P1),
  tri(F2,P2,P1), P1 = clip(B,F1), P2 = clip(B,F2); two behind, F = r, B1 = r+1, B2 = r+2 →
  tri(F, clip(B1,F), clip(B2,F)).
- clip point: geometry = 0x49d8f0 (model.clip: X, Y on the line, Z = near, sx = 320 +
  ftol(X/near), sy = 240 − ftol(Y/near), +0x18 = f32(1.0/near)). **Texture in float**, unlike
  sub 4's integer slope: uB = uzB / rzB, uF = uzF / rzF,
  `u = (uF − uB) / (zF − zB) · (near − zB) + uB` (divide first, then × t), stored
  `f32(u / near)`; v likewise. In the one-in-front block (0x4a1535) the call is
  tri(F, P(esi), P(ebp)) = tri(F, clip(B1,F), clip(B2,F)) in model.py's naming, and for
  clip(B1,F) the values uB, vB, zF − zB, near − zB are spilled to float temporaries
  (0x4a15b4, 0x4a15e5, 0x4a1600, 0x4a160a) before use [asm].
- outcode pretest on clipped triangles: rejects all-left / all-above only (no pixel effect).
- counter 0x625730++ a face.

##### 4.13.2.6 Perspective triangle 0x4a0700(rz0..2, x0,y0, x1,y1, x2,y2, tex, uz0,vz0 .. uz2,vz2) [asm]

```
rotate (x,y,rz,uz,vz) until y1 >= y0 and y0 <= y2
D = ftol((x0-x2)(y1-y2) - (x1-x2)(y0-y2)); if D < 1.0: return      (ints, float compare)
A = y1-y2, C = y2-y0
dudx = f32(((uz0-uz2)A + (uz1-uz2)C) / D)   -> 0x655d1c   (same for vz -> 0x655d18,
drdx = f32(((rz0-rz2)A + (rz1-rz2)C) / D)   -> 0x65e1c0    rz)
16x copies: 0x68b020 = 16 dudx, 0x68b02c = 16 dvdx, 0x625734 = 16 drdx (0x4d1fac = 16.0)
x edges: 13-bit fixed, slope = ((xb-xa) << 13) / h  (C idiv), no subpixel prestep
the left edge (0x624b0c) carries uz/vz/rz (0x624b00/08/10), steps f32((Eb - Ea)/h)
case A (y1 <= y2): skip if y0 >= y2 or y2 < 0
   part 1 rows y0..y1 (if y1-y0 > 0 and y1 >= 0): left 0->2, right 0->1
   part 2 rows y1..y2 (if y2-y1 > 0): left continues 0->2, right 1->2;
          if y1 < 0: xl = (x0<<13) - y0*sl, E = E - dE*y0, xr -= y1*sr
case B (y1 > y2): skip if y1 <= y0 or y1 < 0; right 0->1 for both parts
   part 1 rows y0..y2 (if C > 0 and y2 >= 0): left 0->2 from E0
   part 2 rows y2..y1 (if y1-y2 > 0): left 2->1 from E2, step (E1-E2)/h;
          if y2 < 0: xl -= y2*sl, E = E2 - dE*y2, xr = (x0<<13) - y0*sr
first part row < 0: x -= y*slope, E = f32(E - dE*y) (y as float), count += y, row = 0
last row: count += (H - ylast) if H < ylast        (H = 0x625784 + 1)
per row: if the row's free list is not empty: edge routine (0x6d20bc);
         E = f32(E + dE); xl += sl; xr += sr
```
Rows: free list 0x623bd8[row] (0x62ebe8 walks it), screen row 0x6d1064, texture 0x625758.
See-through kinds: if 0x6d20c0 == 0 the part is skipped; after each **part** with segments
a batch record (0x24 bytes at 0x625790 + 0x24·0x6d1f78): +0 id, +4 count, +8 tex,
+0xc colour, +0x14 16du, +0x18 16dv, +0x1c 16drz, +0x20 light; at 1000 batches 0x6d20c0 = 0.

##### 4.13.2.7 Edge routines [asm]

Inputs: xl = 0x624b0c sar 13, xr = 0x624b44 sar 13; outputs to the span: 0x6d20b4 start,
0x6d1f7c end, 0x6d1054 uz, 0x6d1f84 vz, 0x6d20ac rz.
At start x s: `E_s = f32(E_row + (s - xl) · dEdx)` (fild n × gradient + value).

**0x4a0260 opaque**, per node [a, b) of the row (i16 start, i16 end, next):
```
a < xl:  b < xl → next.  span [xl, min) with E_row unadvanced;
         b < xr → span to b, node.end = xl, next
         else   → span to xr, node.end = xl, if b > xr insert node [xr, b) (pool 0x60d3d0,
                  counter 0x65e1d4) after it; stop
a >= xl: a > xr → stop.  E advanced by a - xl
         b > xr → span [a, xr), node.start = xr, stop
         else   → unlink node, span [a, b); stop if b == xr else next
```
(xl > xr: a reversed span draws nothing but rewrites the node, as 0x4a2f10.)
No s-buffer (0x62e444 == 0): clip xl to 0 (E advanced by −xl) and xr to 0x62e448 + 1.

**0x4a04d0 see-through**: same walk, no list change, records a segment per overlap
(0x1c bytes at 0x68b040 + 0x1c·0x6d1048: +4 row pointer, +8 i16 start, +0xa i16 end,
+0xc uz, +0x10 vz, +0x14 rz). On entry, count == 0x2800 → 0x6d20c0 = 0, return; after a
record that reaches 0x2800 it returns.

**Replay 0x4af130** (after all buckets): batches popped last first; per batch the switch
0x4af338/0x4af2f4 loads globals by id (53/54/50/62 → 16du/16dv/16drz/tex/colour/light;
51 → also 0x65e1cc; 37/38/39 → colour, tex, du, dv), then its segments popped last first
(0x4af3ac/0x4af37c: sub-6 ids reload uz/vz/rz from the segment; 37/38/39 load u, v) and
the span = kind table +0xc is called. Segments of all see-through kinds share one stack, so
`model.Renderer.deferred` (reversed batches, reversed segments) already has the order.

##### 4.13.2.8 Perspective span 0x4a5c40 (31) and twins [asm]

```
n = end - start; if n <= 0: return
z = 8192.0 (0x4f1a24) / rz;  u = ftol(uz·z);  v = ftol(vz·z)       (13-bit fixed texels)
while n:
  if n >= 16:  uz = f32(uz + 16du); vz = f32(vz + 16dv); rz' = rz + 16drz; rz = f32(rz')
               z = 8192/rz'   (rz' unrounded: fst keeps the stack value)
               du = (ftol(uz·z) - u) sar 4;  dv = (ftol(vz·z) - v) sar 4;  k = 16
  else:        k = n; m = 0.0625 (double 0x4d1fb8) · k
               uz = f32(uz + 16du·m); vz = f32(vz + 16dv·m); rz' = rz + 16drz·m
               z = 8192/rz';  du = ftol((uz·z - u) · (1.0/k)) (double 0x4d1fc0);  dv same
  n -= k; k pixels:  texel = page[((v>>13)·256 + (u>>13)) & 0xFFFF]   (u wraps into v)
                     ... write ...;  u += du; v += dv
```
u, v run on from the previous block (only the step is recomputed), so errors self-correct.
Texel offset as the game forms it: `(((u & ~0x1000) sar 12) + ((v & 0xffffe01f) sar 4)) &
0x1fffe` bytes = the index above × 2.
Pixel rules (only the write differs between the twins, by `diff` of the routines):
31 c; 32 `(c >> 1) & 0x7BEF` (0x50270c = (channel masks without low bit) >> 1, set at
0x402010); 53 skip if c == 0; 54 skip if c == 0 (the raw texel) else halved; 51 the face
colour if c != 0; 62 `((screen>>1)&m) + ((c>>1)&m)` if c != 0.
Kind 37 (0x4a50c0, affine, sub 4): the face colour where page[((v>>13)·256 + (u>>13)) &
0xFFFF] != 0, u/v from 0x6d20b0/0x6d20b8 + 0x655d20/0x655d24 a pixel.

##### 4.13.2.9 Precision [asm + guess]

0x49b2c0 alone sets the x87 to 24-bit (and #4 masks) and restores the old control word; the
renderer runs at the program default, which I take as MSVC's 53-bit [guess: no global fldcw
found besides the CRT's]. So: doubles for stack values, f32 wherever a DWORD is stored
(gradients, row values, span uz/vz/rz). ftol 0x4b3048 = truncation toward zero.
Overflow of `x << 13` and `ftol` (0x80000000) are 32-bit in the game; the Python wraps the
edge integers with i32, not the texel coordinates (only their low 16 bits after sar 13 are
used, which unbounded ints give the same).

##### 4.13.2.10 Not done / open
- 0x49dde0's handling of 0x6d20c0 for sub-4 see-through (38/39/37) not read; the Python
  counts their segments/batches toward the shared caps but does not skip them when full.
- Kind 62's exact use, 28/50's purpose (invisible occluders?) not looked at in the data.


#### 4.13.3 The far background (the horizon picture)

Draw: `0x402960(camera)`, called from the race draw 0x47ff90 with camera block
0x5161c0 + i*0x238 (software path: before 0x49a160). Load: `0x410350(track)`.
Python model: `tools/track_horizon.py` (`draw_horizon(...)`; run it as a script
to render a test PNG). Confidence: [asm] read instruction by instruction, [decomp] from
Ghidra's C, [guess], [test] checked by rendering.

##### 4.13.3.1 Loading [asm]

`0x410350`: sprintf `"%sBank\HorizHi.raw"`, `...HorizMed.raw`, `...HorizLow.raw` into three
sprite banks with 0x4776b0 (strncpy 0x100 to bank +0x48), frees all three with 0x477b00,
then loads only one with 0x477b30, chosen by ScreenMode 0x4dc0c8:

| mode | bank | file | sprites (handle global → rect x1,y1,x2,y2 inclusive) |
|---|---|---|---|
| 2 (640x480) | 0x5d41d6 (bank 6) | HorizHi.raw 256x256 | 0x4eaa3c (0,0)-(255,127) "Horizon 1 HiRes"; 0x4eaa40 (0,128)-(255,255) "Horizon 2 HiRes" |
| 1 (512x384) | 0x5d408d (bank 5) | HorizMed.raw | 0x4eaa44 (0,0)-(203,101); 0x4eaa48 (0,102)-(203,203) |
| 0 (320x240) | 0x5d3f44 (bank 4) | HorizLow.raw | 0x4eaa4c (0,0)-(255,63), used for both tiles |

Banks: array 0x5d3a20, stride 0x149. +0x48 file name, +0x30 surface, +0x34 surface id
(-1 = software), +0x38 sprite count, +0x3c sprite array. Sprites defined by 0x480930.

**The second picture is not a mirror**: the RAW holds a 512-pixel-wide panorama cut in two
256x128 halves stacked (A on top, B below) [test: the halves join seamlessly when drawn
B,A,B,A...].

##### 4.13.3.2 Sprite record (0x4e bytes, 0x477790) [asm]

+0x00 frame count (1); +0x04..+0x18 offsets (0); **+0x1c x1, +0x20 y1, +0x24 x2, +0x28 y2
(inclusive)**; +0x2c handle (counter 0x4eea18; 0x477270 finds a sprite by it); +0x30 bank;
+0x34/+0x37 colour-key low/high (3 bytes each, from 0x4f0dd8 = black); +0x3a colour-key
enable = 0 (tiles are opaque); +0x42 CString name; **+0x46 top fill colour, +0x48 bottom
fill colour** (u16 RGB565); +0x3e the last frame drawn; +0x4a default blit flags (1:
clip to the screen; 0x4772f0). 0x477790's 16 arguments (`ret 0x40`): name, x1, y1, x2, y2,
+4, +8, +0xc, +0x10, +0x14, +0x18, frame count (+0), key on (+0x3a), key low*, key high*,
one unused [asm 0x47780a..0x4778e8]. Drawn by 0x477400 (§4.20 §3.5).

##### 4.13.3.3 RAW → 16-bit (0x477b30) [asm]

Header 32 bytes, big-endian: width +8, height +0xa, palette count +0xc.
- palette count 0: 24-bit rows of bytes R,G,B.
- else palette + 8-bit pixels; index 0 → pixel 0; a palette entry with R<8, G<4, B<8 gets
  R = 8 (keeps it off the black key).
Conversion uses the pixel-format globals filled at 0x473c3c..0x473d7a from the DirectDraw
masks: masks 0x4ee9d0/d4/d8 = 0xf800/0x7e0/0x1f, positions 0x4ee9c4/c8/cc = 11/5/0, bit
counts 0x4ee9ac/b0/b4 = 5/6/5, shifts 0x4ee9b8/bc/c0 = 3/2/3. So
`p = (R>>3)<<11 | (G>>2)<<5 | B>>3` — **6-bit green** (unlike the texture pages).

##### 4.13.3.4 Fill colours +0x46 / +0x48 (0x4782e0..0x4785af) [asm]

Per sprite, after loading. Channel bytes of a pixel: `((p & mask) >> pos) << shift`.
- n1 = clamp(x2 - x1, 1, 10); sum n1 pixels of **row y1 from x1 rightwards**; divide
  (unsigned) by n1 → top colour, repacked `((c & 0xff) >> shift) << pos` → **+0x46**.
- The sums are **not reset**: n2 = clamp(y2 - y1, 1, 10); add n2 pixels of **row y2 from
  x2 leftwards** onto the top average; divide by n2; `& 0xff` (can wrap: e.g. a white top
  plus a bright bottom gives a dark colour) → **+0x48**.
Only sprite A's colours are used for the fills (2. picture's are ignored).
So: above the picture = mean of the top-left 10 pixels; below = (top mean + sum of the
bottom-right 10 pixels) / 10, i.e. the bottom mean plus a tenth of the top mean — a game
quirk, then & 0xff per channel.

##### 4.13.3.5 The heading (yaw) [asm 0x402990..]

Camera C[r][c] at cam+0x68+4*(3r+c), T at +0x8c..+0x94.
1. v = C row 2 + T (each stored f32). L = sqrt(vy²+vz²+vx²). If L≠0:
   `yaw = atan2(vz/L/L, vx/L/L)` (fpatan; the double division by L is in the code and does
   not change the angle); else 0.
2. If yaw < 0 (bits > 0x80000000): add 100π (double 0x4d00d8 = 314.1592653589793) until
   ≥ 0.0 (float 0x4d00c4). `ym = fmod(yaw, 2π)` (0x4b30ca = CRT `_CIfmod`, table name
   "fmod" at 0x4f2ce0; 2π = double 0x4d00e0), kept as a double.

##### 4.13.3.6 The pitch [asm]

1. t = ym - π/2 (double 0x4d00e8); n = trunc(t·2607.594587618272) (double 0x4d00d0 =
   16384/2π); c = COS[n&0x3fff], s = COS[(n-0x1000)&0x3fff] (table 0x5fcb80, COS[k] =
   cos(k·2π/16384) f32); Q = COS[0x3000] (0x608b80, ≈ -4.4e-8).
   w = (s, -(Q·c), c·1.0) — the level world direction of that heading.
2. p = w·C + T (px = C00·s + C10·a + C20·b + T0, etc., stored f32).
3. sinφ = py / |p|; φ = asin(sinφ) (0x4b3092 = CRT `_CIasin`, name "asin" at 0x4f32f0);
   φ = 0 when |p| = 0.
**Roll is ignored**: the picture is never tilted, only shifted [asm].

##### 4.13.3.7 x position [asm]

k = (0x62e448 · 6) / 640, integer (0x62e448 = screen width − 1: 639 → 5, 511 → 4, 319 → 2).
P = 2π/k; Wt = x2−x1+1 (256), Ht = y2−y1+1 (128) of sprite A.
q = trunc(ym/P); r = ym − q·P; m = trunc(2Wt · r/P); **x = −(2Wt + m)**.
So one full turn scrolls k·2Wt pixels (2560 at 640x480): the 512-pixel panorama repeats
5 times around the circle.

##### 4.13.3.8 y position [asm]

i = trunc(φ·2607.59…); cφ = COS[i], sφ = COS[i−0x1000]; camY = cam+0x9c;
A1 = cφ − (sφ·Q)·Q; B1 = (cφ·Q)·Q + sφ (Q terms ≈ 0);
Y' = camY·A1 − sφ·50000; Z' = camY·B1 + 50000·cφ (float 0x4d00f0 = 50000.0);
v = trunc(fx / Z' · Y' · scale); **ytop = v − Ht − offset + vtop + cy**.
- fx = 0x6d20c4 (394.0 at 640x480, 0x4592a0: W·394/640).
- vtop = 0x6d1040 (0), cy = 0x624f88 (240), vbottom = 0x625784 (479): 0x49a010, no
  letterbox.
- offset = int 0x4db2b0, scale = float 0x4db2b4, both from the track's .trk (read by the
  TRK loader): ile01 (5, 0.25), ile02 (5, 1.0), ile03 (−10, 1.0), ile04 (0, 0.0),
  ile05 (−15, 0.0), ile06 (0, 0.0), ile08 (−25, 1.0), ile09 (3, 1.0), ile10 (0, 1.0),
  ile11 (0, 1.0). With scale 0 the picture sits at a fixed height (bottom edge at
  cy − offset) whatever the pitch.
In words: the picture's bottom edge is where a point 50000 units ahead on the ground
(camY below the eye) projects, turned by the pitch only.

##### 4.13.3.9 Tiling [asm]

cnt = 0; while x + Wt < 0: x += Wt, cnt++.
first = A if cnt is odd else B; second = the other.
while x ≤ 639: blit(first, x, ytop); x += Wt; blit(second, x, ytop); x += Wt.
0x5f1310 = 1 during the loop, 0 after [asm; purpose not looked at].
Blit 0x477400 → 0x4788b0 → 0x478990: opaque copy, clipped to x 0..0x62e448, y 0..479;
skipped if the clipped rect has left ≥ right or top ≥ bottom (inclusive coords) [asm].

##### 4.13.3.10 Fills [asm]

Software path: 0x45ab60 = IDirectDrawSurface::Blt (vtable +0x14) with IsLost/Restore;
flags 0x4ee960 | 0x400 (DDBLT_COLORFILL), DDBLTFX (size 100) fill colour; RECT right and
bottom exclusive. (D3D path: 0x485e50, out of scope.)
- Below: colour A.+0x48, rect (0, max(ytop+Ht, vtop), 640, 479) → rows to 478.
- Above: colour A.+0x46, rect (0, vtop, 640, min(ytop, 479)) → rows up to ytop−1.
- An empty/inverted rect draws nothing. **Row 479 is never filled** by this function
  [asm; the 3D drawn after covers it in practice — guess].

##### 4.13.3.11 Check [test]

`python3 tools/track_horizon.py build/bkf/ILE02/BANK/HORIZHI.RAW out.png YAW`
(level camera at height 300, offset 5, scale 1) gives the desert buttes on a seamless
strip, dark-blue fill above, orange fill below, and the strip moves with the yaw.

##### 4.13.3.12 Open points

- x87 precision control in 0x402960 not checked: the model computes in doubles and rounds
  only the values the code stores as DWORDs [guess: matches to the pixel].
- Not compared with a screenshot of the original race (the original was not run).


#### 4.13.4 In the port (2026-10-26)

- `r3d.c` is the race's renderer too: `r3d_race(detail)` sets the near plane (64) and the
  kind remap of 0x49c590 for the detail row (`CurrentDetail`, `config.c`; -1 = the menus:
  near 256, kinds as they are). Faces carry their texels as floats too (`ut`, `vt`) for
  the sub-6 kinds: `face6`/`clip6`/`tri6`/`row6`/`row6s`/`span6` are §4.13.2 §5–§8. Kind
  37 is the affine stencil. The see-through pieces of every kind share one table and one
  list of batches (kind, page, colour, the 16-pixel steps), replayed last first; a full
  table stops the see-through kinds (0x6d20c0). At most 2001 faces are drawn a frame
  (0x62e438: the count compared `<=` 2000.0).
- `land.c`: Level.Spt (FileLand, the 20 banks: pages 2..19 from the level's folder), the
  .trk (§4.13.1 §2), the section walk, `land_draw` (0x40ce60: blocks
  ahead/behind by the camera's forward against the section's start plane, the decor of the
  sections around, each object once, cameras hidden, the decor matrix 0x403880 with its
  two float temporaries).
- `horizon.c`: §4.13.3 for 640x480 (HorizHi.raw only so far).
- Test switch `OMR_RACEVIEW=TRACK,FOCAL,X,Y,Z,C00..C22`: no menu, one track from a still
  camera. Three runs in `tools/shots.sh` (race_westway, race_snow, race_tv). **Checked:**
  the port's picture is the same to the pixel as `tools/track.py`'s (RaceRenderer +
  track_horizon) for the eight captures q14, q15, q17, q19, q20, q22, p3, p7 (camera from
  each dump's block). Against the original's screens, the rest is the bikes, riders,
  shadows, tyre marks and the HUD.
- Later: all three horizons (`horizon.c`, one picture and its two rectangles for each screen size).
- **The kind drawn lives in the model's face, not in the bucket's entry [asm + memory +
  pixels, 2026-10-31].** 0x49c590 and 0x49ca70 write the kind they choose (0x49c6dd,
  0x49cad4) into face +0x3a, and 0x4aef20 reads it from there when drawing (0x4af07f). Copies
  of a model share their faces, so **every copy of a model is drawn in the kind of the last
  copy queued**: at Speed Bay's start the nearest palm (sort keys 3554..4003) is drawn in the
  affine 38, not 53, because a palm beyond 6144 was queued after it. The write comes after
  the test of a bucket below 0, before the test of the full list of entries. Found by reading
  the original's see-through tables out of its memory (0x625790, 0x68b040, stopped by SIGSTOP):
  equal counts batch for batch, other ids. The port: `Face.drawn`. The palm then 0 of 16100
  pixels different, and of the whole picture only what moves (boat, gulls, the clocks).
  (`tools/track.py` has the old rule, and its "same to the pixel" above was only against the
  port.)

#### 4.13.5 What stands by the road and over it: "billboards", the turn signs, the place's figure, AnimDecor (mapped 2026-10-30)

The question was the things seen in the original's race and missing from the port: big
diamond signs with an arrow, a chequered-flag object with a wreath, sailing boats, birds.
They turned out to be three different things, and none of them is what `land.c` called
"billboards".

##### 4.13.5.1 The "billboards" (.trk +0x5c) are dead code [asm]

- The decor's draw method 0x4022f0 (vtable 0x4d0110, +0xc) has a branch for `model +0x5c == 1`
  (angle b from the camera's +0xc0) and one for `decor +0x30` (through 0x49b490).
- The loader 0x40d650 writes 0 to +0x5c of all three library models (0x40dcbf, 0x40dcfc,
  0x40dd05, with ebx = 0). The other writers of that offset are the model's constructor
  0x41f7ae and a model copy at 0x48bfbe. No writer of 1 was found.
- `decor +0x30` is set to 0 by 0x4022a0 and by the loader, and nothing else writes it.
- So neither branch runs in the race, and the port leaves them out. Read from the running
  game at Speed Bay [test]: +0x5c is 0 in all 49 entries of each of the three library arrays
  (0x4dc0d8, dc, e0; stride 0x79), and +0x30 is 0 in all 112 decor objects (found by their
  vtable: they are not evenly spaced from 0x4dc0e4, gaps 0x70 to 0x540).
- The draw method always takes the **near** model (+0x4c), as the port already did.

##### 4.13.5.2 The turn signs are on the screen, not in the world [asm, test]

The big diamonds (green with a left arrow, red/orange with an S-bend) look as if they stand
over the road, but they are the library's near models drawn flat on the screen, blinking.

- **Models.** 0x473370 (from the race's start 0x47bf70) looks through the track's library by
  name (strncmp) for slot 0 `counter` (the dial's face, already `hud_dials_3d`) and slots
  1..6: `g_45` 0x4eabb8, `d_45` 0x4eabb0, `g_90` 0x4eaba8, `d_90` 0x4eaba0, `g_chica`
  0x4eab98, `d_chica` 0x4eab90 (g/d: gauche/droite, left/right; chica: chicane). They have 37
  or 43 faces, all of kind 0 (flat colour).
- **Records** 0x5d2eb8 + i x 0x18: +0 library index, +4 x, +8 y, +0xc armed, +0x14 time.
- **Arming.** 0x47e3d0 at 0x47e65d, for every section passed with new furthest progress, when
  the rider is 0x508834 (the followed one, the port's `race_player()`): 0x486240(section
  +0xec). There code −= 0x10 if 0x4dc140 == −1 (racing in reverse); 0..5 → armed = 1, time =
  75.0. So the code runs in the rules' section walk, the numbers 6..12 being the ambience's.
- **Clock** (the loop of 0x486310, each picture): if 0x4e9834 is 1 or 2, or DisplaySignal
  0x4f0e14 is 0, or 0x4f15f4 (the victory credits) is set, then time = 0 and no draw. Else
  time −= dt; below 0 it is disarmed. It is drawn while ftol(time x 0.1) is even: a blink each
  100 ms, for 750 ms (dt is in units of 10 ms).
- **Drawing.** Every vertex: x = W − 1 − 320·W/640 + vx·W/640, y = H − 1 − 400·H/480 −
  vy·H/480, so around (319, 79) at 640x480, with z 2048 (vertex +8 = 2048, +0x18 = 1/2048,
  +0xc = 0x3e000). Each face through 0x49c590 at key 15.0; 0x4f1a18 stays 0, so the kinds pass
  through the detail row.
- **F8** (0x432176, needs 0x4e8944, the port's `g_hotkeys`) toggles 0x4f0e14 and says word 180
  "Signals OFF" or 181 "Signals ON" through 0x4188b0 (id 0x4e98f0, life 2000, align 0x22, x 0,
  y 0x14, effect 2 fade-out, font 12, blink 4). DisplaySignal is saved in CONFIG.BPT.

##### 4.13.5.3 AnimDecor: what the boats and gulls do [asm, test]

The boats and gulls were always in the decor list and the port drew them, standing still. The
"AnimDecor\0" block of the .trk moves them.

- **Loader** 0x48b7e0, object at 0x5f6778: +0 count, +4 a pause flag (always 0), records at +8,
  stride 0x190. File version 1 has records of 0x20 bytes, others 0x40. More than 0x3f records
  or 0x4f frames is a fatal error. All ten tracks parse to the last byte.
- **Record:** +0 type, +4 sub (which angle: 0, 1, 2), +8 decor index, +0xc decor pointer, +0x1c
  t (file value x 0.1), +0x20 a, +0x24 b, +0x28 c (x 0.1 when type 8), +0x2c period (x 0.1),
  +0x30 from[3], +0x3c to[3], +0x48 nframes, +0x4c the near model's +0x1c (no reader), +0x50..
  frame pointers. Type 7 with version > 2 is followed by i32 nframes and, if > 0, i32 npoints
  and the frames of 3 floats a point.
- **Step** 0x48b7a0 → 0x48b370, called in Game_Frame at 0x47d871 with dt, after the riders'
  steps. Each record's t += dt, then by type:

  | Type | What moves | Formula |
  |------|------------|---------|
  | 1 | an angle | triangle wave from a to b over the period |
  | 2 | an angle | (a + b + sin(t/period x 2π + c)(b − a)) x 0.5, with the float 2π 6.2831854820251465 |
  | 3, 5 | an angle | ramp t/period x a + c |
  | 4 | the position | triangle wave from `from` to `to` |
  | 6 | | a fatal error |
  | 7 | the near model's vertices (+4) | i = int(t/period): 0 → the **medium** model's vertices (+0x1c), else frame[i−1]; the clock wraps at (n+1) x period |
  | 8 | the position | t mod (c + period); x = t > period ? 1 : t/period, from `from` to `to` |

- Type 7 repoints a library model's vertices, so all decor of that library entry flap together.
  In every track near nv == medium nv == npoints; medium is not always the same as near
  (Gull05 in ile01). One record in cerv10 has no frames.
- Tracks use: ile01 (2,2) (3,1) (2,0) (7,0) ..., ile02 and ile11 (8,0), ile05, ile08, bercy,
  ile10 (4,0).
- This touches no rider, physics or random stream. The port steps it in the same place of the
  fixed step, so replays stay the same (`tools/replaycheck.sh`).

##### 4.13.5.4 The place's figure: the flags and the wreath [asm, test]

At the end of a race the player's place flies in: "1", "2" or "3" in chequered flags, turning,
before a wreath.

- **Load.** 0x47b600 (from 0x47b420 at 0x47b5cc) reads from `<track>\final\` 1rst, 2nd, 3rd and
  Laureat, .3df and .map, through 0x420a50(.., −1, 1), into 0x515ff0 + i x 0x70, each centred by
  0x420250. 0x420a50's end moves the faces with the MAP's bit 0x20000 (flags & 1) to the front
  by exchanges: for each i whose face lacks the bit, for each later j whose face has it, swap.
  All of a figure's faces share one key, so that order decides what hides what. Only the
  Laureats of ILE03 and ILE06 have such faces.
- **Reset** at the end of 0x47bf70: 0x4f0c40 (turn) = 0, 0x4f0c3c (z) = 16000, 0x4f0c44 = 128.
- **When.** 0x47b7a0(0, dt), in the frame's drawing with flag 8: when 0x51601c != 0 (the first
  figure loaded), the mode 0x4dc094 != 0 (not practice), the place +0x2f85 of rider 0x508834 is
  0..2 and its state +0x2de5 is 2 (finished).
- **Moves.** turn += dt x 0.015707963267948967. If 0x504eac == 0: z > 2500 → z += dt x −80,
  stopping at 2500; the wreath's z is z, or when z == 2500 sin(turn x 4.0) x 128 + z. Otherwise
  (the winner's lap has its credits): z += dt x −30, stopping at 0. Not drawn when z <= 768.
- **Drawing.** Matrices from the tables' g_cos[0] and g_cos[0x3000]; the figure also turns about
  Y by the index int(turn x 2607.594587618272). The wreath's faces at key 16.0, the number's at
  13.0, both with the detail row forced to 3. The near plane is set to 1.0 meanwhile (0x49a120),
  with no effect in practice since the faces are rendered later.

##### 4.13.5.5 In the port

- `signs.c`: `signs_start`, `signs_event`, `signs_step`, `signs_draw`, `signs_key`,
  `figure_load`, `figure_start`, `figure_step`, `figure_draw`. `land.c`: `read_anims`,
  `land_anim_step`. `r3d.c`: `r3d_key_faces`, 0x49c590's loop at a given key and detail.
  `config.c`: DisplaySignal.
- The signs' time and the figure are stepped with the fixed steps, not each picture as in the
  original (the same speed; the blink can differ by one step).
- Under a dialog (its drawing flags −10, bit 3 off) neither is drawn or moved, in both: seen in
  the original, the figure's z stays at 16000 under the Continue dialog with state 2 poked.
- The 16:9 screen: the signs use 0x625784 and 0x62e448, the live view, so they need nothing of their own.

##### 4.13.5.6 Compared with the running original (Speed Bay, 640x480, 2026-10-30) [test]

Method: the game frozen with SIGSTOP for its pictures and for reading its memory (reading a
stopped process works; **`memtool poke` on a stopped process hangs**, so poke it running). A
sign armed by poking +0xc and a large time; the finish forced by poking state 0x587206 = 2 and
place 0x5873a6 (rider 1). The port was forced to the same sign, or to the z and turn read from
the game's memory, with test code that is not in the source. Pixels are compared inside what
the object changes in the port's own picture, counted as different above 8 of 255 per colour.
The backgrounds differ, as the two scenes were not at the same moment.

- **Signs:** all six, 0 pixels different inside the diamond out of about 13000.
- **Which sign:** the first section with an event in a real ride armed slot 5 (`g_chica`, the
  red S-bend), at lap time 2.34 s, and the port's first event on this track is the same
  sign. The later order was not checked against the original.
- **Figure:** "1" at z 9120, 4880 and 2500 (turn 1.35 to 6.35): 3 or 4 pixels of up to 34869.
  "2" and "3" with the wobble at turn 76.0 and 78.3: 4 of 35147 and 3 of 42091.
- **Not exact:** four captures of "2" and "3", each taken 0.2 s after the game was let go
  from a stop, differ in 6224 to 8735 pixels at the turn read from memory. Trying earlier
  turns on one of them brings it down to 761 of 39611, not to 0, so these look like pictures
  older than the memory but that is not proven. The captures above at the same large turns
  were taken longer after a stop and agree, so it is not the size of the turn.
- **The moves:** `z` and `turn` read from memory move at the mapped rates (53 units between
  captures: z −4240, turn +0.8325).
- **AnimDecor:** 42 records (25 of type 2, 11 of type 3, 6 of type 7). The angle of each type 2
  and 3 object in memory equals the formula from its record's t at two moments, within
  2.2e-7. Types 1, 4 and 8 do not occur on this track, and type 7 was not checked in memory;
  those rest on the reading of the code.
- **F8:** the original shows "Signals OFF" and "Signals ON" at the top, in red. The port's text
  was not compared to the pixel.

### 4.14 The Options pages: 11 OPTIONS, 12 SETTINGS (controls), 13 DIAGNOSTIC, 23 DATA DISK (mapped 2026-10-26)

Mapped 2026-10-26 from build/mr.asm (every number below was read in the machine code unless
marked otherwise) with build/decomp as a guide, and checked against the captures
build/ref/orig_p11_options.png, orig_p12_settings_keyboard.png, orig_p13_diagnostic.png,
orig_p23_datadisk.png. Confidence marks: **[asm]** read in the machine code, **[decomp]**
Ghidra's text only, **[guess]** an interpretation, **[px]** seen in a capture.

Conventions as in §4.12 / §4.4:
- Picture ids are those of the table 0x4e3e20 (id = (pointer variable − 0x504880)/4);
  `L` after a name = language folder (data\MenuUs\...), else data\menu\bitmap\. Every
  picture used here has table mode 1 (colour 0 transparent), except the tiles (mode 4).
- 0x40bef0(pic, hl, nb, hotkey, group, value, var, blink, x, y, flags) = picture button;
  0x40bd60(pos[2], text, font, nb, style, hotkey, flags) = text button (kind 1, NOTES
  §4.4.3.2). Neighbour bytes nb: byte0 Left, byte1 Right, byte2 Down, byte3 Up, 0xff none.
  "x,y −1" = the picture stays where it was placed.
- 0x43fa40(pic, pt, align) place; 0x43f9a0(pic, target, seconds, kind, align) slide (kind 1 =
  sine ease); 0x43fab0(pic, dx, dy, 0, 0) step (tile scroll / slide step);
  0x43ede0(pic, surface, mode, pct_h, pct_w) draw (mode 5 = the picture's own mode).
- Text: 0x4903d0(n, 0) = Font_Get(n); "SetColors(f, a, b)" = font vtable slot 0 (0x490b00:
  fore = a, back = b, a NULL keeps the old one, then rebuilds the colour table);
  0x493ee0(font, text, x, y, flags, level, 0, 0, 0) = DrawString (flags 0 = top-left at
  x,y). Colours: white = 0x4f0dd4 (255,255,255), yellow = 0x4f0de8 (255,255,0).
- T<n> = language string n, `*(char**)(0x53b5e8 + 4n)` (table 0x4e2920; English given).
- Frame skeleton common to all four pages (NOTES §4.4.6): 0x49b1c0 present, 0x49a160(0,0)
  begin (+ hover 0x4099a0), ... page ..., 0x40c500 glass shards, 0x49ad90 3D list (only if
  0x504e74 == 0 and software), mouse pointer (id 0x55 at the mouse, if fullscreen and
  DirectDraw), 0x49b0e0 end, 0x49ad70 if 0x504e74. Below only the page's own part is listed.
- No page here plays a sound of its own: only the core's 0x29 (hover) and 3 (click).

Font 12 = "Messages" (AntiAlias8Transp, height 21, spacing 1,1, chars 32..255); font 24 =
"Keyboard" (Pal, height 31, HiRes\Keyb [27..174] + Keyb2 [205..245]); font 31 =
"DataDiskMenu" (Direct, height 21, chars 32..90) (NOTES §4.8.2.2).

---------------------------------------------------------------------------------------------

#### 4.14.1 Page 11 OPTIONS (row 0x4dbbe8: start 0x443000, frame 0x443200, no end routine)

Reached from the main menu's Options button (glass). Exits: config → 12, Accept → 3,
Info → 13, Data disk → 23, **all four with the glass** (the capture 0x40c4a0 is taken after
0x49b0e0 when a local flag [ebp−0x30] was set by those four cases) [asm 0x444937..0x444a62].
**Escape does nothing on this page**: no button has a hotkey [asm 0x4443f5..0x4443fc].

##### 4.14.1.1 Start 0x443000 [asm]
1. Place (all align 0): optitre.lez (id **0x3c** L, 640x81, "OPTIONS" title bar) at (0,0);
   accept.lez (**0x46** L, 120x40) at (500,420); btinfo.lez (**0xaa**, 121x40, "Info") at
   (25,420); DATADISK.lez (**0xb7** L, 121x40, "Data disk") at (275,420). No slides.
2. Controller index **0x4e90d0** from InputDevice **0x4dc11c**: 1 → 0 (KEYBOARD), 4 → 1
   (JOYPAD), 2 → 2 (JOYSTICK); any other value leaves 0x4e90d0 as it was.
3. SetColors(font 9, white, white); SetColors(font 12, white, white) (font 12 only if it
   exists).
4. **The data-disk scan**: path = the data prefix string 0x4e8380 (filled at start-up; the
   game folder prefix, normally empty [guess]) + `Data\List*.bpt` (0x4e9290);
   0x4dc04c = 0; `_findfirst`/`_findnext` (0x4b5830/0x4b5980/0x4b5ac0); for every entry
   whose attributes lack 0x10 (not a directory): `0x51a420[0x4dc04c++] = (short)
   atoi(name + 4)` (so LIST.BPT → 0, LIST01.BPT → 1). **0x51a420 is never read anywhere**
   [asm: the only reference is this write]; only the count **0x4dc04c** matters: page 23
   steps the list number 0..count−1 directly. GOG: LIST.BPT + LIST01.BPT → 2; the CD has only
   LIST.BPT → 1 (then the "Data disk" button is not made).
5. Direct3D only: if 0x4dc06c && !0x4dc070 && the D3D driver record (0x53c754 +
   0x53be30*0x194) is 0: detail 0x4dc098 = 0. (Software: never.)

##### 4.14.1.2 Frame 0x443200 [asm]
Everything is rebuilt every frame: the buttons are cleared and re-added after the hover test,
so the hover result `sel` of this frame refers to last frame's buttons (identical layout).

1. 0x49b1c0, 0x49a160(0,0).
2. Tile paralax2.lez (id **0x01**, 128x128, mode 4): step (**−1, −1**), draw mode 5, 100, 0.
3. `sel` = 0x40c0c0 (the selected button index, −1 none).
4. optitre (id 0x3c) draw mode 5 (no step).
5. 0x40bd40 (clear buttons).
6. The 15 rows (table 1.3), each: SetColors(font 12, c, c) with c = yellow if `sel` is the
   row's button (or one of its two), else white; DrawString(font 12, label, **x 50**, y,
   flags 0); then the value button(s) (text buttons in font 12, style 0, hotkey −1, flags as
   listed), and for the volume rows the gauge.
7. Accept / Info / Data disk picture buttons (1.4).
8. Controller check: if 0x4e90d0 is 1 or 2 and 0x418b60(&0x51ae88, 0) == 0 (no joystick
   known: it re-runs the joystick detection 0x4189c0 and tests again) → 0x4e90d0 = 0. This
   happens **after** the value button was built, so without a joystick a press on
   CONTROLLER shows JOYPAD for exactly one frame, then KEYBOARD again [asm 0x44448f].
9. SetColors(font 12, white, white); 0x40c1f0 draws the buttons (text buttons: yellow when
   selected, else white; picture buttons without highlight: dimmed mode 3 unless selected).
10. p = 0x40c1b0; if p != −1: 0x40c1e0, 0x409bf0. Switch p (jump table 0x444aac, 23
    cases; 1.5).
11. Text list 0x53b8e8: 0x418790, 0x417840, 0x418860 (normally empty here).
12. 0x40c500, 0x49ad90, pointer, 0x49b0e0, glass capture if a page change was made, 0x49ad70.

##### 4.14.1.3 The rows (labels at x 50; value buttons at x 300 unless noted) [asm]

| # | y | label (T id) | button(s) (index: text, nb, flags) | value shown |
|---|---|---|---|---|
| 1 | 100 | T25 "DIFFICULTY : " (yellow if sel==0) | 0: value, nb 0xff01ffff (Down 1), flags 0 | **0x4dc060**: 0 T26 "EASY", 1 T27 "MEDIUM", 2 T28 "HARD" (other: no button made) |
| 2 | 120 | T29 "CONTROLLER : " (sel==1) | 1: value, nb 0x000302ff (R 2, D 3, U 0); when 0x4e90d0 == 2 nb 0x0003ffff (no Right) | **0x4e90d0**: 0 T30 "KEYBOARD", 1 T31 "JOYPAD", 2 T32 "JOYSTICK" |
| – | 120 | – | 2: **picture** BtConfig.lez (**0x57** L, 52x30, "config"), hl 0, nb 0xffffff01 (Left 1), hotkey −1, group −1, value 0, var 0, blink 0, **x 475, y 120**, flags 0 | – |
| 3 | 140 | T33 "DETAIL LEVELS : " (sel==3) | 3: text "%d" (0x4e4ed8) of 0x4dc098+1 into buffer 0x53f678, nb 0x0104ffff (D 4, U 1) | **0x4dc098** 0..4 shown 1..5 |
| 4 | 160 | T34 "OPPONENTS : " (sel==4) | 4: nb 0x0305ffff | **0x4dbfdc** (NbComputerCut): 0 → "24" (0x4e92e0), 1 → "12" (0x4e92e4) |
| 5 | 180 | T35 "D3D : " (sel==5) | 5: nb 0x0406ffff | 0x4dc06c == 0 → T41 "NOT DETECTED"; else 0x4dc070 != 0 → T39 "OFF"; else 0x53be0c ? T42 "FILTERED" : T40 "ON" |
| 6 | 200 | T36 "VOICES : " (sel==6) | 6: nb 0x0507ffff | sound server 0x516aa0+4 == 0 → T132 "CANNOT USE SOUND RESOURCE"; else **0x4db668** 0 T39 "OFF" / 1 T40 "ON" |
| 7 | 220 | T37 "SOUNDS : " (sel==7) | 7: nb 0x0608ffff | same test → T132; else **0x4db664** OFF/ON |
| 8 | 240 | T38 "MUSIC : " (sel==8) | 8: nb 0x0709ffff | CD object 0x503528 null or its +8 (opened) 0 → T133 "CANNOT USE CD PLAYER"; else **0x4db66c** OFF/ON |
| 9 | 260 | T43 "SCREEN MODE : " (sel==9) | 9: text buffer 0x53f3c0, nb 0x080affff | see 1.6 |
| 10 | 280 | T44 "16/9 MODE : " (sel==10) | 10: nb 0x090bffff | **0x504e80** 0 OFF / 1 ON |
| 11 | 300 | T93 "WAVE OUTPUT : " (sel==11) | 11: text buffer 0x53f2b8, nb 0x0a0cffff | see 1.7 |
| 12 | 320 | T82 "MASTER VOLUME :" (sel 12 or 13) | mixer line 0 present (0x5d2960): 12 "-" (0x4dd198) at (300,320) nb 0x0b0e0dff flags **1**; 13 "+" (0x4e92a8) at (400,320) nb 0x0b0eff0c flags **1**. Absent: 12 T71 "Not available." at (300,320) nb 0x0b0effff flags 0; 13 "" (0x4dbea4) at (300,320) nb 0xffffffff flags 0 | gauge 1.8, channel 0 |
| 13 | 340 | T83 "WAVE VOLUME :" (sel 14/15) | line 2 present (0x5d2c48): 14 "-" nb 0x0c100fff, 15 "+" (400,340) nb 0x0c10ff0e, flags 1. Absent: 14 "Not available." nb 0x0c10ffff, 15 "" nb −1 | gauge, channel 2 |
| 14 | 360 | T84 "CD AUDIO VOLUME :" (sel 16/17) | line 1 present (0x5d2ad4): 16 "-" nb 0x0e1211ff, 17 "+" (400,360) nb 0x0e12ff10, flags 1. Absent: 16 "Not available." nb 0x0e12ffff, 17 "" nb −1 | gauge, channel 1 |
| 15 | 380 | T189 "SESSIONS REFRESH RATE : " (sel 18/19) | 18 "-" at (300,380) nb 0x101413ff flags 1; 19 "+" at (400,380) nb 0x1014ff12 flags 1 | value drawn directly (not a button) at **(312, 380)**, font 12 in the label's colour: 0x4dc044 == 0 → T190 "manual", else "%d %s" (0x4e92a0) of 0x4dc044 and T191 "sec." |

Flags 1 = the button repeats every frame while the left mouse button is held (Menu_Frame
step 8), so the −/+ buttons auto-repeat at the frame rate. The empty "" button only keeps
the index count at two per row; its measured rectangle has no width [guess], so it can
hardly be hovered. The labels of the volume rows have no trailing space (strings as in
the table). In the capture "D3D : NOT DETECTED", "WAVE OUTPUT : 16 bits / 22 Khz /
stereo", "CD AUDIO VOLUME : Not available.", the gauges for master and wave [px].

##### 4.14.1.4 Bottom buttons [asm 0x4443df..0x44448a]
- **20** Accept: accept.lez (0x46 L), hl 0, nb **0x12ffff15** (Left 21, Up 18), hotkey −1,
  group −1, value 0, var 0, blink 0, x,y −1 (stays at (500,420)), flags 0.
- If **0x4dc04c > 1** (two or more List*.bpt): **21** Info: btinfo (0xaa), nb **0x12ff1416**
  (Left 22, Right 20, Up 18); **22** Data disk: DATADISK (0xb7 L), nb **0x12ff14ff** (Right
  20, Up 18). Else **21** Info: btinfo, nb 0x12ff14ff. All other fields as Accept.
  (So Info is always button 21; the arrows between Info and Data disk run the "wrong" way:
  Left from Info goes to Data disk, which is to its right — as in the machine code.)

##### 4.14.1.5 Presses (switch at 0x444504) [asm]
| p | action |
|---|---|
| 0 | 0x4dc060 = (0x4dc060 + 1), 3 → 0 (difficulty cycles 0,1,2) |
| 1 | 0x4e90d0 = +1, 3 → 0 (then 1.2 step 8 next frame may force it back to 0) |
| 2 | config: 0x4dc11c = {0→1, 1→4, 2→2}[0x4e90d0]; **0x4311c0** (see 1.9); glass; **ChangePage(12)** |
| 3 | detail: unless (0x4dc06c && !0x4dc070 && D3D record +0x16c == 0): 0x4dc098 + 1, 5 → 0 |
| 4 | 0x4dbfdc ^= 1 |
| 5 | D3D (only if 0x4dc06c): if 0x4dc070 == 0: if the driver can filter (record 0x53c778+idx*0x194): 0x53be0c 0 → 1 (FILTERED), 1 → 0 with 0x4dc070 = 1 (OFF); else 0x53be0c = 0, 0x4dc070 = 1; if 0x4dc070 != 0: 0x4dc070 = 0. Then 0x4e9cf8 = −1 and the glass mesh 0x504c70 is deleted and rebuilt from vitre.3df (0x479470, 0x41f770, 0x420590, 0x479630). **D3D only; in software 0x4dc06c = 0 and nothing happens** |
| 6 | 0x408f30 voices toggle (1.10) |
| 7 | 0x408b30 effects toggle |
| 8 | 0x409410 CD music toggle |
| 9 | screen mode (1.6) |
| 10 | 0x504e80 ^= 1 |
| 11 | 0x409570 wave output (1.7) |
| 12 / 13 | 0x470e90(&0x5d2958, **0**) down / 0x470d90(.., 0) up — master |
| 14 / 15 | down / up channel **2** — wave |
| 16 / 17 | down / up channel **1** — CD |
| 18 | 0x4dc044 − 1, below 0 → 0 |
| 19 | 0x4dc044 + 1, above 30 → 30 |
| 20 | Accept: 0x4dc11c from 0x4e90d0 as case 2; 0x4311c0; glass; **ChangePage(3)**. **No config file is written here** |
| 21 | glass; **ChangePage(13)** |
| 22 | glass; **ChangePage(23)** |

##### 4.14.1.6 SCREEN MODE [asm 0x443afa..0x443c13, 0x444787..0x444874]
**0x504bc4** is the menu's copy of ScreenMode **0x4dc0c8** (0 = 320x240, 1 = 512x384, 2 =
640x480): on entering the menu 0x409f70 saves 0x4dc0c8 → 0x504bc4 and forces 0x4dc0c8 = 2;
the loading page (0x40ba3e) copies 0x504bc4 back into 0x4dc0c8 when the race starts. Same
pair for 16/9: **0x504e80** ↔ Mode_169 **0x4dc0d4** (0x4dc0d4 forced 0 in the menu).
- **Window** (fullscreen flag 0x4dbff0 == 0): each frame the text is chosen from 0x504bc4:
  0 "320*240" (0x4e92d0), 1 "512*384" (0x4e92c8), 2 "640*480" (0x4e92c0) into 0x53f3c0.
  Press: 0x504bc4 + 1, 3 → 0, text refreshed.
- **Fullscreen**: each frame `i = 0x474cf0(0x4dc0cc, 0)` = the display-mode table entry
  (0x5d8960, 36 bytes: +0 w, +4 h, +0xc pixel count, +0x10 mode index; count 0x4ee984)
  whose pixel count equals 0x4dc0cc (the current w*h: 0x12c00, 0x30000 or 0x4b000); if
  none, it loops calling 0x474cf0(.., 1) until one is found; then 0x4dc0cc = entry.pixels,
  0x504bc4 = entry.index, 0x4dc0d0 = i, text "%d*%d" (0x4e92d8) of w, h. Press:
  0x474cf0(0x4dc0cc, 1) = the entry with the smallest pixel count above the current one
  [decomp for 0x474cf0's wrap-around]; if found: the same four assignments; if not found
  (and still fullscreen) nothing. Windows/DirectDraw only: the table holds the enumerated
  modes [guess: only the three the game supports]. **Port**: use the window branch (the
  three fixed strings); the chosen mode is the in-race resolution.

##### 4.14.1.7 WAVE OUTPUT [asm for the text, decomp for 0x409570]
Text (each frame into 0x53f2b8): if 0x516aa0 == 0 or *0x516aa0 == 0 or (*0x516aa0)+0x1038
== 0 (no DirectSound buffer): T71 "Not available."; else sprintf "%d %s / %d %s /%s"
(0x4e92ac): (Wave16Bits 0x4db670 ? 16 : 8), T47 "bits", (server+0x24 = sample rate)/1000,
T48 "Khz", (server+0x2c stereo ? T61 " stereo" : T62 " mono") → "16 bits / 22 Khz /
stereo" [px].
Press 0x409570 [decomp]: if the buffer exists: if server+0x1c != 1 and the card does 16-bit
(+0xc): 0x4db670 ^= 1, recreate the primary buffer 0x4414c0(stereo, 16bit, rate), then
0x4db670 = (actual bits == 16); if now 16-bit, stop. Otherwise change the rate: 22050 if the
current rate ≤ 0x4099 (16537), else 11025, recreate; and if the old rate was below 16537 and
the card does stereo (+0x14): toggle stereo, 0x4db674 = the new stereo flag. Starting at
16/22/stereo the presses cycle 8/11/st → 16/11/st → 8/22/mono → 16/22/mono → 8/11/mono →
16/11/mono → 8/22/st → 16/22/st [decomp, derived]. **Port**: keep the cycle and the text;
the mixer can resample to its own output.

##### 4.14.1.8 Volume gauges and −/+ [asm for the drawing, decomp for the mixer]
Mixer object **0x5d2958** (Windows mixer; lines: 0 master, 1 CD, 2 wave; per line 0x174
bytes; line present = +8+line*0x174 = 0x5d2960 / 0x5d2ad4 / 0x5d2c48; steps =
0x5d2dbc + 4*line = 0x5d2dbc / 0x5d2dc0 / 0x5d2dc4, **32** per NOTES §4.10.5.5).
- 0x470c70(line) = current value 0..steps (MulDiv of the control onto 0..steps).
- 0x470d90(line) up / 0x470e90(line) down: v ± 1, applied only if 0 ≤ v ± 1 ≤ steps
  (no wrap), written to the control (mixerSetControlDetails). Nothing if the line is absent.
- Gauge (only when the line is present), drawn right after the label, before the buttons:
  featacc.lez (id **0x32**, 81x9) placed at **(312, y+8)** (328 / 348 / 368), drawn mode 5,
  pct_h 100, **pct_w = (−100 × v) / steps + 100** (C integer division, truncation toward
  zero), i.e. that percentage of its columns cut off on the right: full bar at v = steps.
- The values are read back from the mixer each time, so they are system volumes.
  **Port**: keep three integers 0..32 (initial from CONFIG.BPT MasterVolume / CDVolume /
  WaveVolume; GOG 0/32/32, CD 19/19/19) and use them as gains (NOTES §4.10.5.5); treat
  every line as present (or keep "Not available." for CD if the port has no separate music
  volume — the GOG capture under wine shows CD "Not available.").

##### 4.14.1.9 0x4311c0 (controller set-up, not the config writer) [decomp]
If 0x4dc11c == 2 (joystick) or 4 (joypad): **JoyType 0x4dc120** = the same value, re-detect
the joystick (0x4189c0 on 0x51ae88); if none connected (0x51bcd4 == 0): 0x4dc11c = 1
(keyboard), return 0. Else return 1. Also called at start-up (0x472c40).

##### 4.14.1.10 The three sound switches [decomp; NOTES §4.10.6.1]
- **0x408f30 VOICES**: if the sound server is not running yet (0x516aa0+4 == 0) and the menu
  is active it first tries to create it (0x423f50/0x423eb0) and reloads the effect and voice
  sets; if it still cannot: a message "[%s]" (0x4dbb20) of T134 "CANNOT USE SOUND RESOURCE"
  is added to the text list **0x53ba68** (0x4188b0: 2000 ms, centred, y 30, blinking) —
  a list only the race draws, so **nothing is visible on page 11** [asm: no menu page draws
  0x53ba68]. Otherwise **0x4db668 ^= 1**; switching on loads the voice samples if they were
  not loaded (0x503554). In the race it also posts "VOICES ON/OFF" (T122/T123).
- **0x408b30 SOUNDS**: same server start; then **0x4db664 ^= 1**, engine 0x4db65c = ambience
  0x4db660 = the new value; off → stop all sounds (0x424020); on → reset the sound
  channels. In the race "SOUND ON/OFF" (T120/T121).
- **0x409410 MUSIC**: if the CD object exists but is not open and the menu is active, it
  reopens the CD (0x4b15a0, 0x4b1460). If open: **0x4db66c ^= 1**; 0 → stop (0x4b1720),
  1 → play the current track again (0x4b19f0, track +0x14, or 0x4db67c if none). In the
  race "AUDIO-TRACK ON/OFF" (T124/T125). If no CD: the "[CANNOT USE CD PLAYER]" message
  (T135) to 0x53ba68 (invisible in the menu).
**Port**: VOICES/SOUNDS/MUSIC = plain toggles of the three flags (stop the music on OFF,
restart the menu track on ON); the "CANNOT USE" texts only if the port has no audio.

---------------------------------------------------------------------------------------------

#### 4.14.2 Page 12 SETTINGS / controls (row 0x4dbbf8: start 0x444b10, frame 0x444f30)

Reached only from page 11's "config" button (glass). Exit: Accept (Escape) → glass → 11.

##### 4.14.2.1 Start 0x444b10 [asm]
Place (align 0 unless said): setting.lez (**0x6e** L, 640x81, "SETTINGS" title bar) at
(0,0); accept.lez (0x46 L, 120x40) at (0, 420) **align 1** (x centred: 260); padcard.lez
(**0x6a** L, 186x306, "Joypad" card), clavcard.lez (**0x6b** L, "Keyboard" card),
joycard.lez (**0x6c** L, "Joystick" card) all at (36, 101); choset.lez (**0x6d** L, 351x47,
"Choose your settings") at (251, 101). **0x53f790 = 0** (the row being redefined:
Accelerate). (0x53f790 is also cleared by pages 20 and 21 — shared scratch [asm].)

##### 4.14.2.2 Frame 0x444f30 [asm]
1. 0x49b1c0, 0x49a160(0,0); `sel` = 0x40c0c0.
2. Tile paralax3.lez (id **0x02**): step (**−1, −1**), draw mode 5.
3. setting.lez step + draw mode 5.
4. **0x5409b4 = 0**. By controller **0x4e90d0**:
   - **0 keyboard**: clavcard step + draw. If 1 ≤ `sel` ≤ 7 (a row is selected):
     k = menu last key (+0xc). If k is not a digit 0x30..0x39, not a letter 0x41..0x5a and
     not one of **0x0d Return, 0x10 Shift, 0x11 Ctrl, 0x14 CapsLock, 0x20 Space, 0x21..0x28
     (PgUp, PgDn, End, Home, Left, Up, Right, Down), 0x2d Insert, 0x2e Delete, 0x60..0x69
     (numpad 0..9), 0x6c (Separator), 0x6e (numpad Decimal)** → +0xc = 0 (byte table
     0x44580c / jump table 0x4457e8). Then if keyboard mode (menu+4 == 0) and +0xc != 0:
     **0x4310f0(0x53f790, k)** (bind).
   - **1 joypad**: padcard step + draw. b = 0x418c60(&0x51ae88, 0) − 1 → 0x5409b4. If b !=
     −1: **0x431140(0x53f790, b)** (no restrictions).
   - **2 joystick**: joycard step + draw. b as above; if b != −1, by row 0x53f790 (jump
     table 0x445870):
     - 0 Accelerate / 1 Brake: b = −2 or −3 (left/right) refused; else bind (row, b); if
       b == −4 (up) also bind (other of 0/1, −5 down); if b == −5 also bind (other, −4).
     - 2 Turn Left / 3 Turn Right: b = −4 or −5 refused; bind (row, b); b == −2 → (other of
       2/3, −3); b == −3 → (other, −2).
     - 4 Higher Gear / 5 Lower Gear: only buttons (b ≥ 0).
     - 6 Turbo/Stunts (and any other): anything.
   Joypad/joystick polling happens every frame whatever the selection or input mode:
   holding a button binds it to the last selected row.
5. choset step + draw mode 5.
6. 0x40bd40; **button 0** Accept: accept.lez, hl 0, nb **0x03ff0506** (Left 6, Right 5, Up 3),
   hotkey **Escape 0x1b**, group −1, value 0, var 0, blink 0, x,y −1, flags 0.
   **Buttons 1..7** (rows i = 0..6): pic = set.lez (**0x6f**, 351x37) when i == 0x53f790,
   else set2.lez (**0x70**, 351x37); hl 0, nb **0xffffffff** (arrows do nothing, so they
   can be bound), hotkey −1, group −1, value 0, var 0, blink 0, **x 251, y 148 + 37 i**
   (148, 185, 222, 259, 296, 333, 370), flags 0.
7. 0x40c1f0 (rows without a highlight picture: dimmed mode 3 unless hovered/selected).
8. The 7 rows' texts, y = **156 + 37 i**, bindings at 0x53eae0 + 8 i (+0 key, +4 button):
   name = T0 "Accelerate", T1 "Brake", T2 "Turn Left", T3 "Turn Right", T4 "Higher Gear",
   T5 "Lower Gear", T6 "Turbo/Stunts" (row 6 goes through sprintf with T6 as the format).
   SetColors(font 12, white, white) each row. Then:
   - keyboard (0x4e90d0 == 0), key k = +0: k == −1 → "%s None" (T8) at (255, y) font 12;
     else name ("%s", T7) at (255, y) font 12, and the key cap: the one character
     **k + 0x13** ("%c", 0x4e92e8) in **font 24** at **x = 590 − width24[k + 0x13]**
     (right-aligned on 590; the width table is font+0x618), **y − 5**.
   - joypad/joystick, b = +4: b ≥ 0 → "%s Button %d" (T9; b as stored, so the first button
     shows "Button 0"); b == −2 / −3 / −4 / −5 → the name at (255, y), then jleft.lez
     (**0xcd**) / jright.lez (**0xce**) / jtop.lez (**0xcc**) / jbas.lez (**0xcf**) (48x35)
     at **(550, y − 7)**, mode 5; anything else (−1) → "%s None". (T10..T13 "%s Stick
     Left/Right/Up/Down" exist but are not used here.)
9. 0x40c500, 0x49ad90, pointer, 0x49b0e0, 0x49ad70. (No text list on this page.)
10. p = 0x40c1b0; if p != −1: 0x40c1e0, 0x409bf0. **p == 0**: glass 0x40c4a0, then
    **ChangePage(11)**. **1 ≤ p ≤ 7**: 0x53f790 = p − 1. Finally **if 1 ≤ sel ≤ 7: 0x53f790 =
    sel − 1** (the hovered row becomes the row to redefine).

##### 4.14.2.3 How redefining works (consequences of 2.2, for the port)
- The row to redefine is the last hovered/selected row (highlighted with set.lez from the
  next frame). Moving the mouse onto a row and pressing a key: the key switches the menu to
  keyboard mode (except Shift/Ctrl), the selection stays, the key is bound that frame.
- Keys reach the page through menu+0xc after the core's handling (0x40a0f0): Escape is
  Accept's hotkey (leaves, never bound); Return/Space press the selected row (pressed = row)
  and are also bound (the bind runs before the press is cleared); Tab moves the selection and
  is filtered out; arrows do not move (rows have no neighbours) and are bound. From Accept,
  Left → row 6, Right → row 5, Up → row 3 (its nb), so the keyboard can reach the rows.
- Since +0xc is only cleared by a press or a new key, a held allowed key keeps re-binding the
  same row every frame (harmless).
- **0x4310f0(row, key)** [decomp, 74 bytes]: row outside 0..6 → nothing; every action whose
  key == key gets −1; then key[row] = key. **0x431140(row, btn)**: the same on the +4
  (button) column. So a key/button is never bound twice.
- **0x418c60(joy 0)** [decomp]: joyGetPosEx-style poll (flags 0x483); returns 1 + the index
  of the lowest pressed button; else by the axes (0..0xffff): x < 0x2000 → −1, x > 0xdfff →
  −2, y < 0x2000 → −3, y ≥ 0xe000 → −4, else 0. After the page's "− 1": button n → n, none →
  −1, **left −2, right −3, up −4, down −5** (the values stored in the +4 column).
- Bindings **0x53eae0[7][2]** (key, button): defaults by 0x430f20 at start-up [decomp]:
  keys Up 0x26, Down 0x28, Left 0x25, Right 0x27, Higher gear 'Z' 0x5a, Lower gear 'A'
  0x41, Turbo Space 0x20; buttons 0, 1, −2, −3, 2, ... — then CONFIG.BPT overrides them
  (Action%dKey / Action%dBtn). GOG CONFIG.BPT: keys 38, 40, 37, 39, 65 'A', 90 'Z', 32;
  buttons 0, 1, −2, −3, 2, 3, −1 — matching the capture (Higher Gear A, Lower Gear Z).
  Read by the race input (0x431250 WM_KEYDOWN, 0x432700 WM_KEYUP, 0x411b30/0x431190 for
  the buttons, 0x43e0f0) [asm refs].

---------------------------------------------------------------------------------------------

#### 4.14.3 Page 13 DIAGNOSTIC (row 0x4dbc08: start 0x4458a0, frame 0x4459d0)

Reached from page 11's Info button (glass). Exit: Back (Escape) → glass → 11.

##### 4.14.3.1 Start 0x4458a0 [asm]
1. diagnos.lez (**0xc5** L, 640x81, "DIAGNOSTIC" title bar) at (0,0); Btback.lez
   (**0x51** L, 121x40) at (500, 440).
2. Button 0: Btback, hl 0, nb **0x0bffffff** (a stale Up 11; irrelevant), hotkey **Escape**,
   group −1, value 0, var 0, blink 0, x,y −1, flags 0.
3. SetColors(font 12, white, white).
4. **0x540ccc = 0x440540()**: the system information record (static at 0x53f278):
   +0 CPU name, +4 speed string or NULL, +8 OS name, +0xc memory string (see 3.3).
5. Icons (75x75) at x 32: proc.lez (**0xb3**) y 85, video.lez (**0xb4**) y 185, son.lez
   (**0xb5**) y 285, joy.lez (**0xb6**) y 385.

##### 4.14.3.2 Frame 0x4459d0 [asm]
1. 0x49b1c0, 0x49a160(0,0), 0x409bf0.
2. Tile paralax1.lez (id **0x00**): step (**+1, −1**), draw mode 5.
3. Draw mode 5: diagnos, proc, video, son, joy. Then 0x49ad90 (unconditionally here).
4. The lines, all font 12 (white from the start routine), **x 128**, flags 0, each built in
   buffer 0x53f678 (label + value concatenated):

| y | text | source |
|---|---|---|
| 80 | T172 "Operating System : " + OS | sysinfo+8; NULL → T71 "Not available." |
| 100 | "%s%s (%s)" (0x4e9324): T49 "Processor : ", CPU name, speed | CPU name NULL → "%s%s" (0x4dbb28) T49 + T71; speed NULL → "%s%s" T49 + CPU name |
| 120 | T50 "MMX(R) Technology : " + value | 0x4dbcf0 (MMX present) ? (0x4dbcf4 (MMX used) ? T51 "Detected and Enabled." : T52 "Detected but User Disabled.") : T53 "Not detected." |
| 140 | T171 "Physical Memory : " + memory | sysinfo+0xc; NULL → T71 |
| 200 | T89 "Direct Draw driver : " + T73 "Available." | constant |
| 220 | T92 "Direct 3D driver : " + driver | 0x53be30 == −1 → T71; else the name at 0x53c600 + 0x53be30*0x194, plus " (PowerVr mode)" (0x4e9314) if 0x4dc1cc. The capture shows an empty name [px] |
| 240 | T86 "%sMB video board" with the memory text below | see below |
| 280 or 290 | sound block | see below |
| 400 | 0x51bcd0 ? T58 "Joystick port detected." : T57 "No joystick port detected." | after **0x4189c0(&0x51ae88)** — the joystick is re-detected every frame |
| 420 | 0x51bcd4 ? T60 "Joystick or Joypad connected." : T59 "No Joystick or Joypad connected." | same |

   Video memory text (DirectDraw caps, Windows only): if 0x4ee99c: m = 0x4ee998 (+ 0x5d93c4 ×
   0x5d93b0 × 2, the screen surface, when 0x4ee950 == 1); m a whole number of MB → "%d"
   (m >> 20), else "%5.2f" (m × 2^−20, float 0x4d0b70). Else: m = 0x4ee990 (+ the same
   surface), t = 0x4ee98c; both whole MB → "%d(Tex)+%d" (t >> 20, m >> 20), else
   "%5.2f(Tex)+%5.2f" [asm 0x445e3f..0x445f77]. Capture: "257.50MB video board".
   Sound block: if the DirectSound buffer exists (0x516aa0 → +0x1038): y0 = **290**, or
   **280** if hardware mixing (+0x10); "T68 'Sound board : '" + (+0xc ? T45 "16 bits" : T46
   "8 bits") + (+0x14 ? T61 " stereo" : T62 " mono") at (128, y0); y += 20; if hardware
   mixing: T66 " (With hardware mixing)" at (**128 + w**, y) where w = the measured width
   (0x492b10) of "Sound board : " alone, y += 20; T67 "Direct Sound Driver : " + (+0x18 ?
   T69 "Installed" : T70 "Not installed") at (128, y), y += 20. Without the buffer: 0x4dc1a8
   ? T64 "Sound disabled" : T63 "Sound resource is not available" at (128, 300), y = 320.
   Then "Volume control : %s" (T74) with mixer object present (0x5d2958 != 0) ? T73
   "Available." : T71, at (128, y).
5. 0x40c1f0, 0x40c500, 0x49ad90, pointer, 0x49b0e0, 0x49ad70 (no text list).
6. If p = 0x40c1b0 == 0: 0x40c1e0, glass 0x40c4a0, **ChangePage(11)**.

##### 4.14.3.3 0x440540, the system information (Windows; port substitutes) [decomp]
- CPU: CPUID vendor "AuthenticAMD" → "AMD", "GenuineIntel" → "Intel", "CyrixInstead" →
  "Cyrix", else "Unknow"; family 3 / 4 / 5 / 6 → "%s 386", "%s 486", "%s Pentium(R)",
  "%s Pentium(R) Pro" / "Pentium(R) II", Cyrix MediaGX names...; any other family →
  "%s CPU Family:$%x" (0x4e8d40) — the capture's "AMD CPU Family:$f". Speed: a timing loop
  0x4404b0 → "%d Mhz" (0x4e8e04), NULL if 0.
- OS: GetVersion() < 0x80000000 → "Windows(R) NT"; else major > 3 → "Windows(R) 95", else
  "Win32s".
- Memory: GlobalMemoryStatus dwTotalPhys, MB rounded up ((n + 0xfffff) >> 20), "%d Mb."
  (T188).
**Port**: fill these with its own facts (e.g. "Linux", the CPU vendor/MHz or "Not
available.", RAM from sysconf), "Direct Draw driver : Available." as is, "Direct 3D driver
: " + empty or "Not available.", the video line from a fixed or SDL value, the sound block
from the SDL audio format (16 bits stereo, no hardware mixing → y0 290), and the joystick
lines from SDL_NumJoysticks.

---------------------------------------------------------------------------------------------

#### 4.14.4 Page 23 DATA DISK / plug-in track lists (row 0x4dbca8: start 0x451d10, frame 0x4514e0,
end 0x451fc0)

Reached from page 11's Data disk button (glass; only shown when there are ≥ 2 List*.bpt).
Exits (**no glass on either**: no 0x40c4a0 in the page [asm]): Accept (Return) → 11 with the
list now shown; Cancel (Escape) → the old list reloaded → 11.

##### 4.14.4.1 Globals
- **0x4dc048** = the list number in use (0 → `Data\list.spt`, n → `Data\list%02d.spt`,
  0x4e9614 / 0x4e8a80; opened as .BPT). Initial 0 (data), **not saved in the config**, so the
  game always starts with LIST.BPT. Readers: start-up 0x472c40 / main menu 0x4422c0
  (championship file `Data\champ%c%02d.spt`), page 20 0x450800, race set-up 0x455280, the
  exit save 0x4329a5 [asm refs].
- **0x540180** (short) = 0x4dc048 saved at page start (for Cancel).
- **0x542380** = "the list is loaded and its pictures are wanted" (shared with other pages as
  a page-state word). **0x540994** = "pictures are sliding out, a new list is due".
  **0x53f554** = "no picture loaded this frame" (also the main menu's voice-once flag).
- **0x4e90ec** = array of picture objects (0x74 bytes each, ctor 0x43e7e0, dtor 0x43e170),
  one per track of the list (count 0x51d298), created with `new` (count stored before it).
  The end routine 0x451fc0 deletes it and sets 0x4e90ec = 0.
- Track records 0x4dc088 + i×0x130 (NOTES §4.12.8): **+0 is used here as "picture to
  load"** (1 = load), +0x24 the folder (`data\ile05\`).
- After a list change the track table 0x4dc088/0x51d298, ListName 0x504eb8 and the
  championship tables are those of the new list (0x428d60 and 0x429a00 reload them).

##### 4.14.4.2 Start 0x451d10 [asm]
1. 0x540180 = (short) 0x4dc048.
2. Place: DATTITLE.lez (**0xb8** L, 640x81, the "DATA DISK" bar under a black pill) at
   (0,0); accept.lez (0x46 L) at **(442, 425)**; cancel.lez (**0x47** L, 120x40) at **(85,
   425)**; left.lez (**0x1a**, 63x114) at (−100, 181); right.lez (**0x1c**, 59x112) at
   (740, 181).
3. Slides, 0.7 s (0x3f333333), kind 1, align 0: left → **(7, 181)**, right → **(581, 181)**.
4. 0x40bd40; buttons (all hl 0, group −1, value 0, var 0, blink 0, x,y −1, flags 0):
   **0** accept, nb −1, hotkey **Return 0x0d**; **1** left, nb **0xff02ffff** (Down 2),
   hotkey **Left 0x25**; **2** right, nb **0xff01ffff** (Down 1), hotkey **Right 0x27**;
   **3** cancel, nb −1, hotkey **Escape 0x1b**. (Built once, not every frame.)
5. 0x4e90ec = new picture array of 0x51d298 objects (fatal "Not Enough Memory"
   (pageMainMenu.cpp line 0x1dcd) if it fails); every track's +0 = 1.
6. **0x542380 = 1, 0x540994 = 0**.

##### 4.14.4.3 Frame 0x4514e0 [asm]
1. Two script objects on the stack (0x428c70 list reader, 0x429910 championship reader).
2. 0x49b1c0, 0x49a160(0,0).
3. Tile paralax2.lez (id 0x01): step (**+1, +1**), draw mode 5. DATTITLE step + draw.
4. SetColors(font 31, yellow, NULL (back unchanged)); DrawString(font 31, **ListName
   0x504eb8**, **(350, 27)**, flags 0) — "PLUGIN" for LIST.BPT [px], "TWO_NEW_ISLANDS" for
   LIST01.BPT (font 31 has no '_' (95 > 90): such characters are skipped [guess from the
   font range]).
5. **List change**: if pictures[0].+0x10 (its slide running) == 0 and 0x542380 == 0:
   0x540994 = 0. Then if 0x540994 == 0 and 0x542380 == 0: open the list file for 0x4dc048
   and read it (0x428d60); on failure close it and **return at once** (rest of the frame
   skipped, including the unlock). Open `Data\champ%c.spt` (0x4e927c) or
   `Data\champ%c%02d.spt` (0x4e9264) with %c = 'A' + difficulty 0x4dc060 (and the list
   number) and read it (0x429a00); failure → the same early return. Then 0x542380 = 1,
   delete and re-create the picture array for the new count, every track's +0 = 1.
6. **Pictures**: 0x53f554 = 1; for i = 0 .. min(0x51d298, 8) − 1:
   - if track[i].+0 == 1 and 0x540994 == 0: load `"%ssmalphot.lez"` (0x4e9604) with the
     track folder into picture i (Bitmap_Load 0x43e7f0, mode 1; it always returns 1, so
     +0 becomes 0); place it at (x, y0) and slide it (0.7 s, kind 1) to (x, y1) with
     **x = 85 + 126 × (i mod 4)**, row r = i / 4, **y0 = 840 r − 200** (−200 above the screen
     for the top row, 640 below for the bottom row), **y1 = 95 + 166 r**; 0x53f554 = 0;
     **stop the loop** (one picture loaded per frame; the later pictures are not drawn this
     frame).
   - else (if the array exists): step picture i (0x43fab0) and draw it mode 5.
   (So the eight pictures arrive one per frame, row 0 falling from above, row 1 rising from
   below.) smalphot.lez is in each track's bank; the track name is part of the picture [px].
7. 0x40c1f0 (accept/cancel/arrows without highlight → dimmed unless selected).
8. p = 0x40c1b0:
   - **0** (Accept / Return): 0x40c1e0, **ChangePage(11)**.
   - **1 / 2** (arrows, Left / Right keys): 0x40c1e0; only if 0x542380 == 1, 0x540994 == 0
     and 0x53f554 == 1 (every picture loaded, nothing loaded this frame): 0x542380 = 0,
     0x540994 = 1; every shown picture i slides (0.4 s, 0x3ecccccd, kind 1) to
     **x = −100 if ((i/2) & 1) == 0 else 640**, same y (columns 0-1 leave to the left,
     2-3 to the right); then **1**: 0x4dc048 = (0x4dc048 ≤ 0 ? 0x4dc04c : 0x4dc048) − 1;
     **2**: 0x4dc048 = (0x4dc048 < 0x4dc04c − 1 ? +1 : 0). Presses while not ready are
     swallowed. The new list is loaded by step 5 once picture 0's slide has ended.
   - **3** (Cancel / Escape): 0x40c1e0; 0x4dc048 = 0x540180; reload the list and the
     championship as in step 5 (failure → early return, page stays); **ChangePage(11)**.
9. Text list 0x53b8e8, 0x40c500, 0x49ad90, pointer, 0x49b0e0, 0x49ad70.
Quirk: Accept pressed while the pictures slide out (list number already changed, new list
not yet read) leaves with the old track table under the new number [asm-derived].

##### 4.14.4.4 End 0x451fc0 [asm]
Deletes the picture array (vector destructor, then operator delete), 0x4e90ec = 0.

---------------------------------------------------------------------------------------------

#### 4.14.5 The config file: when and how it is written

**Not on any Options page.** Accept on page 11 only stores the controller (0x4311c0); the
settings live in globals until the program exits.

##### 4.14.5.1 When [asm 0x433000..0x4330d5, decomp of 0x4329a5]
In the window procedure on **WM_DESTROY (2)**, if the config was read successfully at start
(0x4e716c, set at the end of the reader 0x42ae30):
1. If the menu is active: 0x4dc0c8 = 0x504bc4, 0x4dc0d4 = 0x504e80, 0x4dc028 = 0x50487c (the
   menu copies of screen mode, 16/9, 0x4dc028) — the old values kept and restored after.
2. FullScreen 0x4dbff0 = 0/1 if the command line forced it (0x4e8954 0/1; initial −1).
3. If not aborting (0x4ea818 == 0) and 0x4e8590 == 0 (set when 0x45a520 on the data path
   fails — a read-only folder [guess]): open `Data\Config.Spt` for writing (Script_Open mode
   2 → fopen "wrt", text, the .Spt name itself) and **0x42c4c0** writes it ("Cannot save
   config file." on failure).
4. If 0x4e714c (initial 1: "scripts are binary"): 0x4e714c = 0, 0x4e7148 = 1, reopen
   `Data\Config.Spt` for reading — in this mode Script_Open reads the text and also creates
   `Data\CONFIG.BPT` ("wb", cipher seeded 0x1a2b3c4d) into which the token reader writes
   every value it reads — and run the reader 0x42ae30 over it; then 0x4e714c = 1,
   0x4e7148 = 0, and **0x42ee70 deletes the .Spt** (opens it "rb"; if it exists, close and
   remove 0x4b5190) [decomp for 0x42ee70].
5. Then (separately) if the track list has tracks: the current list is written back
   (`Data\list.spt` / `list%02d.spt` by 0x4dc048, writer 0x4291f0) — the track unlock states.
So the lasting file is CONFIG.BPT (NOTES §3.4 format: values only, strings as u32 length +
enciphered characters, numbers as 4 enciphered bytes).

**The list in the port** (written 2026-10-31; until then it was never saved, so a championship's
unlocks and Reverse/Pocket were lost when the program ended): `tracks_save` 0x4291f0 writes the
list in use to the player's folder when the program ends (`LIST.BPT`, `LIST<nn>.BPT`: ListName,
Pocket 0x4dc03c, Reverse 0x4dc040, count, then name, folder, ON/OFF, laps); `tracks_load` reads
that copy first, else the game's, and a damaged copy falls back to the game's. Only the list in use
is written, as the game: unlocks won and not yet saved go when page 23 reads another list. Written
with nothing unlocked, both files equal the game's byte for byte. The cheats' flags (CDNALSI...) are
apart and are not saved. `tools/unlock_all.sh` (`OMR_UNLOCK_ALL`) makes both lists all ON; `--undo`
goes back to the game's. The CD's LIST.BPT holds the same tracks as GOG's, so one copy serves both.
Not run from end to end: a real championship win reaching the file.

##### 4.14.5.2 The text written by 0x42c4c0 [asm]
One line per field: the key (written "%s"), the value "%32d" (right-aligned in 32
characters) — the name "%8s" — then "\n". Keys and sources, in order (= the CONFIG.BPT
token order = the reader 0x42ae30's order):

| key (exact) | source | GOG value |
|---|---|---|
| `DefName:\t\t\t\t` | player name 0x5161b0 (%8s) | 'DSI' |
| `DrawLand:` | 0x4e9904 | 1 |
| `ScreenMode:` | 0x4dc0c8 | 2 |
| `FullScreen:` | 0x4dbff0 | 1 |
| `Mode_169:` | 0x4dc0d4 | 0 |
| `MapMode:` | 0x4dbdac | 4 |
| `DisplaySignal:` | 0x4f0e14 | 1 |
| `CurrentDetail:` | 0x4dc098 | 4 |
| `CurrentDifficulty` (no colon: the reader's string) | 0x4dc060 | 1 |
| `NbComputerCut` (no colon) | 0x4dbfdc | 0 |
| `AmbiantState:` | 0x50354c, or the live 0x4db65c when the sound server runs | 0 |
| `MotoState:` | 0x5028a0, or live 0x4db660 | 0 |
| `SoundState:` | 0x504208, or live **0x4db664** | 1 |
| `VoicesState:` | 0x50355c, or live **0x4db668** | 1 |
| `AudioState:` | 0x504870, or live **0x4db66c** when the CD is open | 1 |
| `Wave16Bits:` | 0x4db670 | 1 |
| `Stereo:` | 0x503530, or live 0x4db674 | 1 |
| `ReplayRate:` | 0x504568 (= the server's rate when it runs) | 22050 |
| `MasterVolume:` | mixer 0x470c70(0) | 0 |
| `CDVolume:` | mixer line 1 | 32 |
| `WaveVolume:` | mixer line 2 | 32 |
| `InputDevice:` | 0x4dc11c | 1 |
| `JoyType:` | 0x4dc120 | 2 |
| `CounterType:` | 0x4f0dcc | 1 |
| `ShowCounter:` | 0x4f0dc8 | 1 |
| `D3DOnlyEmulation:` | 0x4dc070 ? 1 : (0x53be0c ? 2 : 0) | 2 |
| `DetectD3DCard:` | 0x4dc030 | 1 |
| `LastLevel:` .. `LastPokt:`, `LastMoto:` | 0x580838, 0x58083c, 0x580840, 0x580844, 0x580848, 0x58084c, 0x580854 (LastLevel, LastMaxTour, LastCurMode, LastNbComp, LastRev, LastPokt, LastMoto) | 0, 4, 1, 23, 0, 0, 0 |
| `Action%dKey:`, `Action%dBtn:` for d = 0..6 | 0x53eae0 + 8d, +4 | 38/0, 40/1, 37/−2, 39/−3, 65/2, 90/3, 32/−1 |
| `SmallFonts:` | 0x4f0e08 | 1 |
| `EnumSession:` | **0x4dc044** (sessions refresh rate) | 10 |
| `SpeedUnit:` | 0x4f0dd0 | 0 |

The reader 0x42ae30 also derives: 0x4dc0cc from ScreenMode (0x12c00 / 0x30000 / 0x4b000,
anything else → mode 2), **0x504bc4 = 0x4dc0c8**, **0x504e80 = 0x4dc0d4**, the mixer volumes
(0x470cf0), D3DOnlyEmulation 2 → 0x53be0c 1, 0x4dc070 0; 0 → both 0; else 0x4dc070 1
(command-line D3D switches 0x4dc194/198/19c override) [decomp].
**Port**: save the same fields when the program quits (the port already reads
Config.Spt/CONFIG.BPT in this order: config.c); where it is saved is Danik's decision
(saves.c's player folder is the obvious place).

---------------------------------------------------------------------------------------------

#### 4.14.6 The globals: initial values (mr_flat.bin) and readers

Initial values are the image's; CONFIG.BPT overwrites most at start-up (GOG values in 5.2).

| addr | meaning | initial | range | written by | read by (besides these pages) |
|---|---|---|---|---|---|
| 0x4dc060 | difficulty | 1 | 0..2 | config 0x42b1c9, p11 | champ file letter (0x4422c0, p23, 0x472c40), race set-up 0x454aa0, 0x455280, 0x47bc60, 0x47dae0; champ saves p20/21; Link p8 |
| 0x4e90d0 | page-11 controller index | 0 | 0..2 | p11 start / press | only p11, p12 |
| 0x4dc11c | InputDevice | 1 | 1 kbd, 4 joypad, 2 joystick | config, 0x4311c0, p11 | race input 0x411010, 0x411b30, 0x431250, Game_Frame 0x47d660 |
| 0x4dc120 | JoyType | 2 | 2 / 4 | config, 0x4311c0 | 0x431250 |
| 0x4dc098 | detail level | 0 | 0..4 (shown 1..5) | config, p11 | the race renderer (0x405bc0, 0x40ce60, 0x410120, 0x461f70, 0x46e620, 0x47b7a0, 0x49c590, 0x4b0be0, ...); F-key in race 0x431250 |
| 0x4dbfdc | NbComputerCut | 0 | 0 "24", 1 "12" | config, p11 | race set-up 0x47b420; champ saves 0x450800/0x450f40 |
| 0x4dc06c | Direct3D present | 0 | 0/1 | 0x41e8f0, 0x41f2b0 | everywhere (render path); **0 in the port** |
| 0x4dc070 | Direct3D switched off | 0 | 0/1 | config, p11, page changes | everywhere; port: 1 (software) or ignore |
| 0x53be0c | D3D filtering | 0 | 0/1 | config, p11 | D3D only |
| 0x4db664 | effects on | 1 | 0/1 | 0x408720, 0x408b30 | sound play 0x4049f0, 0x404b30, race sounds |
| 0x4db668 | voices on | 1 | 0/1 | 0x408720, 0x408f30, 0x408b30 | 0x4049f0, voice loading |
| 0x4db66c | CD music on | 1 | 0/1 | 0x408720, 0x409410 | 0x404990 (play track), 0x409670, 0x4096e0 |
| 0x4db670 | Wave16Bits | 1 | 0/1 | config, 0x409570 | sample loading 0x403c90.. |
| 0x504bc4 | menu's ScreenMode | 0 (CONFIG: 2) | 0..2 | 0x409f70, config, p11 | Menu_Frame, 0x40ba44 (→ 0x4dc0c8 for the race), exit save |
| 0x504e80 | menu's Mode_169 | 0 | 0/1 | 0x409f70, config, p11 | 0x40ba3e (→ 0x4dc0d4), exit save |
| 0x4dc044 | EnumSession (sessions refresh, s) | 10 | 0..30 (0 = manual) | config, p11 | Link session list p7 (0x449100, 0x449680) — network only |
| 0x4dc04c | number of List*.bpt | 0 | ≥ 0 | p11 start | p11 (Data disk button), p23 |
| 0x51a420 | list numbers (short[]) | 0 | | p11 start | nobody |
| 0x4dc048 | list in use | 0 | 0..count−1 | p23 | see 4.1 |
| 0x53f790 | row being redefined | 0 | 0..6 | p12, p20, p21 | p12 |
| 0x53eae0 | bindings [7]{key, btn} | 0 | key VK / −1; btn ≥0, −1..−5 | 0x430f20, config, p12 | race input |
| 0x5409b4 | last joystick reading − 1 | 0 | | p12 | p12 |
| 0x5d2958.. | mixer (master/CD/wave) | runtime | 0..32 | config, p11, in-race keys 0x409720 | exit save |

---------------------------------------------------------------------------------------------

#### 4.14.7 What the port can drop or must fake (D3D, network, Windows)

- **Direct3D**: the D3D row (always "NOT DETECTED", its button a no-op), the D3D-only detail
  restriction, the vitre rebuild on case 5, "Direct 3D driver", the video-memory line
  (fake a value or "Not available."), D3DOnlyEmulation/DetectD3DCard in the config (write
  them back unchanged).
- **Network**: SESSIONS REFRESH RATE (0x4dc044, EnumSession) only matters to the Link
  session list; keep the row, the −/+ and the saved value so the page looks the same.
- **Windows**: the mixer (volumes become the port's own gains), the display-mode table
  (window branch), the DirectSound format cycling (WAVE OUTPUT: cycle the text), the system
  information on page 13, joystick detection (SDL), Alt+Enter. The "CANNOT USE ..." texts
  only when the port runs without audio.
- Page 11's empty "" text buttons, the auto-repeat flags and the rebuild-every-frame scheme
  are part of the keyboard navigation (indices 12..19 depend on them): keep them.

#### 4.14.8 Open points
- 0x474cf0's wrap-around and what the fullscreen mode table holds [decomp/guess] — the port
  uses the windowed branch anyway.
- 0x440540's family names for families 5/6 (Pentium strings) were read in the decomp only.
- The "" button's hit rectangle (0x492a90 on an empty string) not read.
- 0x4e8590 (blocks the config save) meaning is a guess.
- ListName characters outside font 31's range (e.g. '_') assumed skipped.

#### 4.14.9 In the port
- `page_options.c`: pages 11, 12, 13, 23. Page 13 shows the port's own machine (the
  processor from /proc/cpuinfo, the memory from sysconf) in place of 0x440540's Windows
  questions; the joystick lines and page 12's bindings read SDL's joysticks (§4.22).
- `config.c`: CONFIG.BPT read in 0x42ae30's order and written again at the end
  (§4.14.5) to the player's home folder, never the game's; the home copy is read first.
  The game's master volume (0 in the GOG copy, a Windows mixer line) starts full unless
  the port saved one. `script.c`'s ScriptOut writes the cipher (lengths plain).
- `sound.c`: the three switches and the volumes (effects gain = master × wave / 1024,
  music gain = master × CD / 1024).
- **Checked** (`tools/shots.sh` options, settings, datadisk) against orig_p11_options,
  orig_p12_settings_keyboard, orig_p23_datadisk: only the pointer differs, and on page 11
  the CD volume row (wine's MCI offers no CD volume, so the original says "Not
  available."). Page 23's list name takes font 31's back colour from the title (page_intro:
  yellow on black there), so it matches only when the title ran first, as it always does.

### 4.15 The bike + rider object (page 10 CHOOSE BIKE, and the race) (mapped 2026-10-26)

Mapped from game.EXE (GOG 3.22, base 0x400000) for the port. Marks: [asm] read in the
disassembly (build/mr.asm), [decomp] from Ghidra's text only, [guess] inferred,
[px] confirmed by a picture. Local variables of 0x45f290 are named `L0xNNN` = the
frame offset from esp after its `sub esp,0x424` (the drawer's arguments are L0x428 =
renderer, L0x42c = camera, L0x430 = LOD argument).

**Verified [px]:** `python3 -I tools/bikeparts.py menu build/bkf FAMILY SKIN THETA out.png
[capture.png]` draws the page-10 bike from this description (doubles rounded to float at
the main steps, tools/model.py's s-buffer renderer). Against the two captures in
build/ref, 99.97 % (KTMCROSS, skin 0, θ = 1.302: 24806 of 24814) and 100.0 % (SPEED,
skin 0, θ = 2.4985: 26329 of 26329) of the drawn pixels are *exactly* the capture's
RGB565 value (θ found by search; the 8 misses are edge pixels). Everything below about the menu path (loading, the pose, the
fork stretch, the swing arm, the wheels, the skin palettes, the camera) is therefore
confirmed as a whole, including the odd parts noted as such.

---------------------------------------------------------------------------------------

#### 4.15.1 Loading (page 10 start, 0x40a4d0) [asm for the calls]

Order:

1. Level.Spt → family script (0x51d068, or the Pocket one 0x51d0a8) read by 0x429e20
   (field list in §4.15.1.1); the 8 carac files by 0x42a580 into the bike table
   0x509278 + i·0x124 (carac float at +0x14+4k = `Bike.phys[k]` in port/src/bikes.c).
2. base page = 0x401f80() (the page counter 0x4db1c0: 0 in the menu).
3. 0x401d90(FileBnk0 0x518b20, 0) → page 0; 0x401d90(FileBnk1 0x518b60, flag) → page 1,
   flag = (bike 0's PersoAnim == 1), i.e. 1 for the dirt families.
4. 0x410a10: every PilotRot file through 0x45b340 into records 0x51d2a8 + k·0xf4
   (count 0x5191e0).
5. 0x413450: the 3D models (§4.15.1.3).
6. 0x45bf70(obj 0x584421, 0, 1, skin 1, 0) – the object (§4.15.2); 0x45bdf0 → 0x45bb30 zeroes
   every physics field of §4.15.2.2; 0x45b480(rot 0x51d678 = PilotRot #4 = staticPied.rot,
   obj, slot 0) – the pose; 0x45c7c0(obj, 0, 0, 0) – the position; then obj+0xbc..+0xe8 =
   the bike matrix built inline from COS[0] and COS[0x3000] (identity up to ~1e-16, zero
   translation) [asm 0x40a710–0x40a7c2]; 0x45c700(obj, base page) – page offsets.
   (The Direct3D branch, 0x4dc06c, is out of scope.)

At page end 0x44ecf0 frees the banks 0x4f92f8/0x4f9630, sets 0x4db1c0 = 0 and calls
0x45be60 (frees the object's arrays) [asm].

##### 4.15.1.1 The family script (0x429e20) [decomp; names checked against the .BPT files]

| # | field | stored at | MOTOSPD (road) | MOTOKTM (dirt) |
|---|-------|-----------|----------------|----------------|
| 1 | FileBnk0 | 0x518b20 | speed\bnk00_00.raw | ktmcross\bnk00_00.raw |
| 2 | FileBnk1 | 0x518b60 | speed\bnk00_01.raw | ktmcross\bnk00_01.raw |
| 3 | FileSkl | 0x518ba0 | speed\corps.skl | ktmcross\corps.skl |
| 4 | FileSklHand | 0x518be0 | speed\corps.skl | ktmcross\mHand\corps.skl |
| 5,6 | FileRoueArriere 3df / map | 0x518c20 / 0x518c60 | rouear | rouear |
| 7,8 | FileRoueAvant 3df / map | 0x518ca0 / 0x518ce0 | roueav | roueav |
| 9,10 | FileMotoMed 3df / map | 0x518e60 / 0x518e20 | medium\corps | medium\corps |
| 11,12 | FileMotoLow | 0x519020 / 0x518fe0 | kawalow\kawalow | low\ktmlow |
| 13,14 | FileMotoVeryLow | 0x5190a0 / 0x519060 | kawavlow\kawa_vlw | verylow\ktmveryl |
| 15,16 | FileGuidon | 0x5190e0 / 0x519120 | guidon | guidon |
| 17,18 | FileCockpit | 0x519160 / 0x5191a0 | cockpit | cockpit |
| … | PilotRot names until one without '.' | 0x5191e4 + k·0x40, count 0x5191e0 | static, gauche, droite, staticHaut, **staticPied** (#4), … | same + depart, sautdelire… |
| | "_N_name" entries take two ints (race jumps) | 0x580d10 + k·0x10, list 0x580d0c, count 0x5cf4fc | – | `_1_`, `_3_` … |
| | MotoCarac ×8 | 0x519de4 + k·0x40 (+0x519fa4) | carac1..8 | carac1..8 |

PKTKTM names `rouearF.map` / `roueavF.map` and uses medium\corps for all three LODs;
PKTSPEED uses data\SpeedPkt.

##### 4.15.1.2 Texture pages (0x401d90 → 0x401160) [asm]

0x401d90(name, flag): page object 0x4f92f8 + n·0x338 (n = 0x4db1c0, at most 0x2c), the
old one freed (0x401ab0), loaded by 0x401160, name copied to page+0x4c, returns n, counter+1.

0x401160(page, name, flag) on the RAW ("mhwanh", big-endian header, see tools/raw.py):
- **Paletted (palette count ≤ 256) with flag 0** (bnk00_00 of every family): the 8-bit
  texels are kept as they are at page+0x330 (pointer; 256 bytes a row); palette 0 = the
  RAW's own palette converted to RGB565 as (R>>3)<<11 | (G>>3)<<6 | (B>>3) (green keeps 5
  bits, its low bit 0), pointer at page+0xc; then "<name without extension>.%d" (format
  0x4db1e4) for 1..7 – each a whole RAW file of which only the 0x20-byte header is skipped
  and 256 RGB triples read – into page+0xc+k·4; page+8 = palette count (8 here: the banks
  hold .1 … .11 but only .1–.7 are read). **The 8 palettes are the 8 bikes' liveries.**
- 24-bit with flag 1 (dirt bnk00_01): RGB565 texels as above, page+0x254 = 2.
- **24-bit, 256x256, flag 0** (road bnk00_01 in the menu): the texels are *not read* (the
  page keeps no picture) [asm 0x4012f3 → 0x401432]. Harmless: no menu face uses page 1 on
  road bikes (only one face of the medium LOD model does).

##### 4.15.1.3 The models (0x413450) [asm]

Each through 0x420a50(3df, map, -1, flag) (flag = bike 0's PersoAnim == 1), then
0x4204b0 scale or 0x420250 centre (+0x41fa10, not needed for drawing [guess]):

| model | address | treatment |
|-------|---------|-----------|
| guidon (handlebar, cockpit view) | 0x504ed8 | ×8 |
| cockpit | 0x51bf60 | ×8 (flag 0) |
| medium / low / very low (LOD 2, 3, 4) | 0x53a788 / 0x53a7f8 / 0x53a868 (skeleton model slots 2–4) | ×0.95 |
| rear wheel | 0x52c240 and 0x52c2b0 (one per skeleton slot 0/1) | centred on its box |
| front wheel | 0x51ac58 and 0x51acc8 | centred on its box |

0x420a50 is 0x4216a0 (§4.9.3.1: ×96, corner order, mirror x) with these kinds
[decomp, checked by the picture]:
- MAP kind 3/4 (textured; must equal the corner count): kind **0x10** if flag and the face's
  page ≠ 0, else **1**; with MAP flag 0x10000 (see-through) **0x26** / **2**; 0x200000 →
  0x15 (0x27 if see-through) regardless of flag; 0x40000 → 0xd; 0x80000 → 8; flags +0x38:
  bit0 = 0x20000, bit1 = 0x400000, bit2 always, bit3 = 0x100000.
- A textured face whose corners 0,1,2 have the same (u, v) becomes kind 0 with the texel as
  its colour (the palette index on a paletted page, the RGB565 value otherwise).
- Otherwise on a paletted page the colour is the index of the 555→565 colour in palette 0
  (0 if absent); on others the colour itself (0x423110).
- MAP kind < 3 (untextured): kind 0 (2 if 0x10000, 0xd, 0xc), colour = u16 at MAP **+2**.
- After loading, faces with bit0 are moved to the front of the list (pairwise swaps)
  [decomp]; then 0x4232e0 (u, v × 1/256), 0x422070 (mirror).

The wheels in the menu: SPEED kinds 1 (36) and 2 (12) on page 0 (paletted, so they take the
livery's palette); KTMCROSS kinds 0x10 (30), 0x26 (20), 0 (10) on page 1 (RGB).

#### 4.15.2 The object (0x584421 = player 1; stride 0x3209 for 0x584421 + k·0x3209)

##### 4.15.2.1 0x45bf70(obj, p1, p2, skin, p4) [asm]

- obj+8 = p1, obj+4 = p2; if the bike's PersoAnim == 1, p4 = −1.
- Race shadow quads at +0x2ef1/+0x2f31 (0x422de0, page 0x2c) and +0x311d/+0x315d
  (0x422f80) – drawn only by the race (0x461f70 from 0x458b80) [asm].
- Frees the old per-bone face arrays.
- Skeleton slot 1: LOD +0x638 = 1, 0x462c60(FileSklHand); slot 0: +0x638 = 0,
  0x462c60(FileSkl). After each: every vertex and every bone pivot minus bone slot 0's
  pivot; 0x462580 per-vertex normals into obj+0x2d60+slot·4 (only the race lighting
  0x462a40 reads them); bone slot 3 unlinked from its parent's child list.
- 0x48bce0 sub-objects (§4.15.2.2): body (+0x24, model 0x53a6a8), rear wheel (+0x32c,
  0x52c240), front wheel (+0x1a8, 0x51ac58); +0xb4 = carac+0x48 + 2.0 when manual and
  p1 && p2; +0x68 = 1 (loaded), +0x6c = 1, +0x108 = obj.
- Skins: +0x170 = +0x5fc = skin & 7; +0x2f4 = +0x478 = skin & 7 unless p4 == −1 (then −1);
  skin < 0 → all −1. +0x18/+0x1c = FLT_MAX; +0x2e21, +0x31c1, +0x31bd = 0.

##### 4.15.2.2 Layout (offsets from obj) [asm unless marked]

| offset | size | what |
|--------|------|------|
| +0x24 | 0x184 | body sub-object (0x48bce0): angles +0x24, **position +0x30** (copies +0x3c, +0x48), model ptr +0x58, skin **+0x170** (LOD 2–4 model) |
| +0x78 | 4 | bike number (0x584499): the carac row used for the geometry |
| +0xbc..+0xe8 | 48 | the bike matrix (3x3 + translation), copied to **+0x2e55** by the drawer |
| +0x110 | 4 | steering (body+0xec) |
| +0x124 | 4 | speed (body+0x100) |
| +0x1a8 | 0x184 | front wheel: position **+0x1b4**, model ptr +0x1dc, skin **+0x2f4** |
| +0x32c | 0x184 | rear wheel: position **+0x338**, model ptr +0x360, skin **+0x478** |
| +0x4bc.. | 12 | pos + c38·48·(row 1) + T (0x45c7c0) [decomp] |
| +0x5fc | 4 | skin of the skeleton faces |
| +0x638 | 4 | detail: 0 skeleton, 1 hand skeleton, 2/3/4 medium/low/very low model |
| +0x63c + (s·20 + b)·100 | 100 each | bone b of skeleton slot s (s = 0, 1; 20 bones each) |
| +0x2d4c + s·4 | 4 | bone count n |
| +0x2d60 + s·4 | 4 | vertex normals (race lighting) |
| +0x2d74 + s·4 | 4 | file bone → slot map (int array) |
| +0x2e1d | 4 | lean t (or +0x30dd while crashed, +0x30c1) |
| +0x2e39 | 4 | suspension (frame height) |
| +0x2e3d / +0x2e49 | 12 / 12 | rear / front wheel spin angles |
| +0x2e8d | 4 | pitch p |
| +0x2e91 | 4 | suspension (rear) |
| +0x3085 | 48 | saved frame matrix (the rider's reference) |
| +0x30b5 | 12 | saved body point |
| +0x30c1 | 4 | crashed |
| +0x304d | 4 | current ROT record (0x51d678 = staticPied) |

The skeleton's geometry lives in the model records 0x53a6a8 + s·0x70 (+4 vertices,
+0x1c copy, +0x30 count).

**Bone record (100 bytes)** [asm]: +0 next sibling, +4 parent, +8 first child, +0xc angles
a, b, c (float, from the ROT), +0x18 matrix (3x3 + T), +0x48 pivot, +0x54 first vertex,
+0x58 vertex count, +0x5c face count, +0x60 faces (0x40-byte Face records as NOTES
§4.15.4.9.3.1).

**Carac fields used here** (row 0x509278 + bike·0x124, float at +off):

| off | name | road SPEED | dirt KTMCROSS | used for |
|-----|------|-----------|---------------|----------|
| +0x38 c38 | RoueAxeDistance | 1.75 | 1.45 | wheelbase /96 |
| +0x3c c3c | RoueRayonAv | 0.29 | 0.30 | front radius /96 |
| +0x40 c40 | RoueRayonAr | 0.32 | 0.30 | rear radius /96 |
| +0x48 c48 | RealSpeedMax | 77–84 | 44–48 | steering fade (race) |
| +0x58 c58 | HauteurCadre | 0.58 | 0.55 or 0.60 | frame height /96 |
| +0x6c c6c | FourcheAngleArriere | −0.55 | −1.05 | swing arm angle offset |

#### 4.15.3 File formats

All little-endian; `tools/bikeparts.py check` reads every file of SPEED, SPEEDPKT,
KTMCROSS, KTMPKT and PODIUM: 10 SKL, 106 ROT – all consumed to the exact last byte, all
vertex ranges contiguous, all face indices valid.

##### 4.15.3.1 .SKL (0x462c60; reads with 0x458d40 = i16, −1 at end of file, and 0x458d70 = f32) [asm]

| field | type | notes |
|-------|------|-------|
| n | i16 | if < 0 (all shipped files: −1): "new" format, **n read again**, scale 3.5 (0x40600000); else old format, scale 10.0 |
| — | | n ≥ 19 → error message (0x4ea550) |
| parent[n] | i16 | in file order; −1 root |
| slot[n] | i16 | new format only: file bone i → bone slot (array at +0x2d74) |
| nv | i16 | → model +0x30 |
| vertex[nv] | 3 f32 | × 3.5 (old: 3 i16, each ftol(i/10.0) then float) |
| per file bone i: pivot | 3 f32 | × 3.5 → bone slot[i] +0x48 (old: 3 i16 / 10 truncated) |
| first, count, nfaces | 3 i16 | → +0x54, +0x58, +0x5c |
| faces[nfaces] | 36 bytes each | below |
| extra group | i16 count, faces | kept in bone slot n (+0x58 = 0; no vertices) |

Face (36 bytes): i16 unused; i16 kind (0x462c30: 3 → **1**, 4 → **2** (see-through), anything
else kept: KTMPKT/COCKPIT.SKL and the PKTKTM sub-skeletons have kind 0 faces; with 0x4ea4d0
set, 3 stays 3); i16 colour (RGB555); then 5 × (i16 vertex index, i16 u, i16 v) → indices
at face+0x2c+2k, u, v as floats at face+4+8k / +8+8k (texels, later ×1/256 by 0x4232e0).
The 5th index is 0xffff for a triangle (whose 4th repeats the 1st); for a quad the 5th
repeats the 1st. The file's kind is **not** the corner count (kind 3 occurs on quads and
4 on triangles). Page +0x36 = 0 for every face (the base page added by 0x45c700).
Indices refer to the *whole* skeleton vertex array (the bones' ranges follow each other
in file order, which is the order the drawer transforms them).

Colour of each face at load [asm 0x4634b8–0x46367b]:
- page 0 has no texels (not in these pages): 0x402020(colour) (555 → 565).
- **u0 = u1 = u2 and v0 = v1 = v2** (the "flat-uv" faces: 59–98 per skeleton, whatever
  their file kind): kind 0,
  colour = 0x402020 of the texel byte at page 0 [v·256 + u] taken as an RGB555 value –
  which works out as **texel & 0xDF** (bit 5 lost; the drawer then uses it as a palette
  index). A bug of the original that the port must copy for exact colours.
- else, paletted page 0: index in palette 0 of the file's colour put through
  `(c & 0xffc1) >> 1 | (c & 0x1f)` and then 0x402020 (0 if none) – read at details 0 and 1, where
  0x49ca70 makes these faces flat (the rider's yellow and green; a test at those details showed it).
- then 0x4232e0 (u, v × 1/256).
In the last (extra group) loop the flat-uv case stores the **raw texel** (no 0x402020, so
no & 0xDF) [asm 0x4639cb–0x463a2d, px: 19–25 such faces per skeleton; with the mask the
match with the capture drops from 99.97 % to 99.6 %], and with no texels on page 0 the
colour is kept as it is.

Linking [asm]: for every file bone with slot[i] > 0, child = slot[i], parent =
slot[parent[i]]: child+0 = parent+8 (old first child), child+4 = parent, parent+8 = child
(prepend). 0x45bf70 then unlinks slot 3 (the rider's root) from slot 0's list; slot 3 keeps
its own next pointer (so recursion from slot 3 also revisits slot 2 – harmless, same
result).

The shipped skeletons (17 bones, same tree):

| file | slot | parent (file) | what [guess from the picture] | SPEED verts | KTMCROSS verts |
|------|------|---------------|-----------------------------|-------------|----------------|
| 0 | 0 | – | frame, tank, seat, engine | 71 | 60 |
| 1 | 1 | 0 | fork crown + handlebar (steers) | 0 | 58 |
| 2 | 15 | 1 | lower fork (stretched to the axle) | 24 | 12 |
| 3 | 2 | 0 | swing arm | 12 | 18 |
| 4 | 16 | 3 | rear axle (pivot only) | 0 | 0 |
| 5 | 3 | 0 (unlinked) | rider root (pelvis) | 0 | 0 |
| 6–16 | 4–14 | 5,6,7,6,9,6,11,12,11,11,15 | rider: body, legs, arms, head | … | … |

Then the extra group (91 / 120 faces) – faces spanning several bones (the rider's joints).

##### 4.15.3.2 .ROT (0x45b340) [asm]

"ROT\0" (compared with "ROT"), 3 i16 read and ignored, then up to 20 records of 3 i16
(a, b, c); angle = i16 × 0.0015339807878856412 (double 0x4d0e68 = 2π/4096), stored as
float at record+4+k·12, count at +0; record size 0xf4. File size = 10 + 6·bones
(STATICPIED.ROT: 17 bones, 112 bytes). 0x45b480(rot, obj, s) copies min(count, n) records
to bone **slot** k (+0xc), i.e. the records are in slot order. staticPied: slots 0–2 and
15–16 zero, slot 3 (24,0,0), slots 4–14 the rider's pose.

##### 4.15.3.3 .PIV, .SNI, CORPS.3DF/.MAP/.RAW – not read by the game

No reference in game.EXE to ".piv", ".sni" or their tags ("SNI" occurs only inside a
Swedish menu string). .PIV is a modeller dump (u32 1, "00-corps", source paths such as
`C:\CUBED\KTM\corps.skl`, stale memory); .SNI is "SNI\0", 3 i16, then "ROT\0" blocks (an
animation sequence of the modeller). The top-level CORPS.3DF/.MAP/.RAW of the banks are the
source of the skeleton, also unused. `bikeparts.py check` lists them; their contents are
not decoded further [guess for the layouts].

#### 4.15.4 Drawing: 0x45f290(obj, renderer, camera, lod) [asm, traced]

Page 10 calls it with lod = 0 every frame after setting the three skins (+0x2f4, +0x478,
+0x5fc) to the displayed bike. The race calls it with −1 (keeps +0x638 from the LOD
choice 0x45f160) from 0x458b80, then the shadow 0x461f70.

Matrices are row-vector 3x3 + translation; "A·B" = A then B (0x403a80, out.T = A.T·B +
B.T). R(a, b, c) = 0x40fd40 (§4.9.3.3 formula, angles through the cosine table, index
= ftol(angle × 2607.594587618272)). 0x403740 = the same with a translation. angles(M) =
0x46bd00/0x46d270 (the inverse of R by fpatan; |cos b| < 1e-5 handled).

##### 4.15.4.1 Entry

If lod ≠ −1: +0x638 = lod and +0x2e55 = +0xbc (the bike matrix). B = copy of +0x2e55.
Light L0x128 = 0x465d60(): 0x1f when there is no track (0x52c10c == 0: the menu), else 0x10
or 0x1f by the section's lighting (+0xe0) [asm]. The vertex light word becomes 0x1f << 13 =
0x3e000, which selects the unlit span routines.

##### 4.15.4.2 LOD 2–4 (one model) [asm]

M = R(0, π/2, 0)·R(0, 0, −t)·B, T = (0, c40·96, 0)·M + rear(+0x338) − campos, then ·C;
0x49b2c0 with the light, or if 0x4dc130 && light: 0x49b2c0 then the lighting 0x422540
with the light vector 0x52c228..; faces 0x49c110(skin +0x170).

##### 4.15.4.3 LOD 0/1 (the skeleton): placing the parts

Notation: t = lean (+0x2e1d, or +0x30dd when crashed), p = pitch (+0x2e8d), st = steering
(+0x110), v = speed (+0x124), e39 = +0x2e39, e91 = +0x2e91. **Menu: all 0.**

1. Wheel models of the slot: +0x360 = 0x52c240 + s·0x70, +0x1dc = 0x51ac58 + s·0x70.
2. L298 = R(0,0,−t)·B; Mbody0 (rear wheel) = R(+0x2e3d spin)·L298.
3. f = steering fade: dirt 1 − v/(c48·2), road 1 − 2v/c48, clamped to [0, 1] (menu 1);
   s = st·f·2π/8192 (double 0x4d1048).
4. Mfront = R(+0x2e49 spin)·R(0, s, 0)·R(0,0,−t)·R(−p,0,0)·B;
   Mframe (L0x268, copy L0x208) = R(0,0,−t)·R(−p,0,0)·B.
5. rear = +0x338, front0 = +0x1b4 (set by 0x45c7c0: pos + (0, −c40·96, −c38·48)·Bike and
   pos + (0, −c3c·96, c38·48)·Bike: the wheels' ground points). dir = unit(front0 − rear);
   **front = rear + dir·c38·96 (written back to +0x1b4)**; mid L0x1bc = rear + dir·c38·48.
6. y1 = c58·96 + e39·40; h = c40·48 + c40·0.125·96·e91 (menu: c58·96, c40·48).
   **P_body = mid + (0, y1, 0)·Mframe + (0, h, 0)·L298** (L0x1b0, again L0x70).
7. Mrear = R(angles(L298)) with translation P_body.
8. The fork: head = pivot[1]·Mrear; Msteer = R(0, s, 0)·Mframe; grip = head +
   (pivot[15] − pivot[1])·Msteer; len = |grip − head|; fdir = unit(grip − head);
   reach = |front − head| − c3c·96; **front axle = head + fdir·reach**; the front wheel:
   0x48c400(obj+0x1a8, renderer, cam, Mfront, axle − front) (with 0x4dc130:
   0x4221d0(light ≠ 0x1f) then 0x48c6d0 – the lit variant).
   q = |pivot[15] − pivot[1]|·len / reach; **pivot15' = pivot[1] + unit(pivot[15] −
   pivot[1])·q** (the stretched pivot of the lower fork, L0x1fc); slot 1 angles =
   (0, −s, 0) (+0x6ac: the handlebar turns with the steering).
   (Note: the lower fork's vertices move by pivot[15] − pivot15', i.e. by d − d²/reach
   along the fork, a first-order stretch.)
9. The swing arm (by **bone slot 3's pivot** – the rider's root, 0x7b0 – not the swing
   arm bone, slot 2; looks like code written for the old format where slot = file index;
   kept because the picture matches): P3 = pivot[3]·Mrear; k = (pivot[3] − ẑ)·Mrear − P3;
   n̂ = unit((pivot[3] − x̂)·Mrear − P3); a = rear − P3;
   θ3 = atan2(n̂·(k × a), k·a) (both divided by |k||a|, fpatan) → 0x580a70;
   **φ = c6c + θ3**; swing = R(φ, 0, 0)·Mrear (R built inline with b = c = 0 from COS[0]
   and COS[0x3000]). The swing arm's vertices (file bone 3) are drawn with
   swing·(T − campos)·C; **rear axle = pivot[16]·swing**; the rear wheel:
   0x48c400(obj+0x32c, …, Mbody0, axle − rear).
10. Rider reference: if not crashed, +0x3085 = Mframe, +0x30b5 = P_body, Pref = P_body;
    else Pref = saved + (0, 32 dirt / 64 road, 0).
11. Bone matrices: 0x463d90(slot 0, Mframe) and 0x463d90(slot 3, +0x3085). For a root:
    L = R(angles) with T = pivot − pivot·R (a rotation about the pivot), **M = P⁻¹·L**
    (P⁻¹ = the rigid inverse); 0x464190 for the others: **M = parent.M·L**; both recurse
    into +8 (child) and +0 (next). Then slot 15 again: L = R(angles15) with
    T = −pivot15·R + pivot15', M15 = M1·L.

##### 4.15.4.4 Vertices and faces

For each file bone i = 0..n−1 (slot = map[i]), the bone's vertex range is transformed
by 0x49b2c0 into one output array (L0x174 = the start, from i = 0):
- i = 0: R(angles(Mframe)) with T = P_body − campos, ·C.
- i = 3: the swing matrix above.
- other: V = M_slot⁻¹ (transpose, T' = −T·Mᵀ) with T' += (Pref if 3 ≤ slot ≤ 14 else
  P_body) − campos, ·C (0x403a80 with the camera +0x68); skipped if the range is empty.
  So a vertex goes through the inverse local rotations from its bone up to the root, then
  the frame: the ROT angles act as inverse rotations about the pivots.
Then if 0x4ea4d0 (race only): 0x462a40 lights the output with (0x52c228..) and the camera.
0x4ea4cc = 0xff. Faces: for slot 0..n (n = the extra group): if +0x5fc ≥ 0,
0x49c110(output, faces, count, skin +0x5fc), else 0x49b7f0(…, 1).

Drawing order into the buckets: front wheel, rear wheel, then the skeleton's faces slot
by slot.

##### 4.15.4.5 0x48c400(wheel sub-object, renderer, cam, M by value, dx, dy, dz) [asm]

T = pos(+0xc) + d − campos; N = M(3x3)·C, T·C + cam T; 0x49b2c0(model vertices, 0x1f);
faces 0x49b7f0 if the skin (+0x14c) < 0, else 0x49c110(…, skin).

##### 4.15.4.6 0x49c110 and the paletted path [asm]

Like 0x49b7f0 (§4.9.3.5) except: skips a face only if flag bit0 and kind 0xd;
**key = the farthest Z, no +1024**; area > 1.0 (as an int compare of the float). If the
face's page has palettes (page+8 ≠ 0) → 0x49ca70(face, key, verts, skin, flags), else the
usual 0x49c590. 0x49ca70: bucket ftol(key·0.25) (0x4d1f9c), clamped to 0x1f3e, < 0
dropped, at most 0x1d4b faces (0x655d14); entry +0x10 = palette pointer page+0xc+skin·4,
+0x14 = skin; kind copied to face+0x3a (race: 0 or 0x2a when 0x4dc0c4 && detail < 2).

Dispatch 0x4aef20 (bucket entry +0x10 ≠ 0), by face+0x3a (tables 0x4af100/0x4af0e0):

| kind | routine | pixel |
|------|---------|-------|
| 0 | 0x4a3cf0 | flat: colour index < 256 → palette[index] (index ≥ 256 kept as RGB565); light < 0x3e000 → (c >> 1) & mask 0x50270c |
| 1 | light ≥ 0x3e000 ? 0x4a7b70 : 0x4a8c70 | 0x4a7b70 → triangle 0x4a74e0 → span 0x4a72d0 → pixel 0x4a7240: **palette[texel[((v>>13)·256 + (u>>13)) & 0xffff]]**, opaque, s-buffer |
| 2 | ≥ 0x3e000 ? 0x4ab070 : 0x4ac1f0 | records the visible pieces (0x68b040, 0x1c bytes, ≤ 0x2800) and draws them after all buckets (0x4af130); pixel 0x4aa6c0: palette[texel], **skipped when the palette colour is 0** |
| 3 | 0x4a6a50 | (crash kind, race) |
| 4 | 0x4ad410 | |
| 9 | 0x4ae730 | |
| 42 | 0x4a9df0 | (race low detail) |

0x4a7b70 is 0x49e510 (§4.9.3.6) with the palette pointer stored in 0x65e1cc, no
light average, no colour setup; u, v × 2097152 (0x4d1fa8) as there [asm]. In the menu the
light is 0x3e000, so only the unlit routines run.

##### 4.15.4.7 Menu values (page 10)

LOD slot 0 (main skeleton); pose staticPied; t = p = st = v = e39 = e91 = 0; wheel spins
0; f = 1; light 0x1f; 0x4dc130 = 0, 0x4ea4d0 = 0; not crashed; no shadow; skins = the
displayed bike for the frame and both wheels (the frame write overrides the dirt families'
−1 for the wheels); geometry from the carac of bike +0x78.

Race-only parts: lean, pitch, steering, speed fade, suspension (+0x2e39, +0x2e91), wheel
spin, the crash reference (+0x30c1), the LOD choice 0x45f160 (d = |pos − campos| /
zoom 0x6d1f74: ≥ 3000 → 4, 2500–3000 → 3, < 1000 (and 0x4ea4e0 == 0) → 0, else 2), the
hand skeleton (slot 1, chosen at 0x45c23d, 0x45e30f, 0x467b98…), lighting (0x422540,
0x462a40, 0x48c6d0 with 0x4dc130), the shadow (0x461f70), the paletted kinds 3/4/9/42.

#### 4.15.5 The camera (page 10) [asm]

- Start: 0x46da40(cam, angles (0,0,0), offset (0,0,1500)) – thiscall, the *second* argument
  goes to +0x50..+0x58, the first to the angles +0x38..+0x40; position (0, 0, 1500);
  0x46e140(cam, (0,0,0)); 0x49a120(1.0).
- Each frame, 0x46dab0(cam, obj+0x24): Mcam = R(+0x38, +0x3c = θ, +0x40) (same formula as
  0x40fd40), Mobj = R(obj angles +0x24..) ; +0x68 = **Mobj·Mcam**; position = obj pos
  (+0x30..) + cam+0x8c.. + (+0x58 = 1500)·(third row of that product); +0xc0 = 0x490220
  (a roll angle, not used for drawing). With the object's angles 0 and +0x8c = 0 (the
  previous look-at leaves it 0): **eye = (1500 sin θ, 0, 1500 cos θ)** (sin/cos from the
  table).
- Then y += 200.0 (0x4d0c44); 0x46e140(cam, target 0x53f540 = (0, 0, 0) when idle): d =
  unit(target − eye), h = √(dx² + dz²); A = [[dz/h, 0, dx/h], [0, 1, 0], [−dx/h, 0, dz/h]],
  Bp = [[1, 0, 0], [0, h, dy], [0, −dy, h]], **C = A·Bp**, translation +0x8c = 0 (if h = 0
  nothing changes).
- Then y += 40.0 (0x4d0bac) without a new look-at: the picture is taken from
  **(1500 sin θ, 240, 1500 cos θ)** with the orientation aimed from y = 200.
- Projection as the menu's: fx = fy = 1911, centre (320, 240), near 256 (§4.9.3.4).
- θ: += dt × 2π/10240 per frame (one turn in 10.24 s), see §4.12 page 10 frame.

With this, `bikeparts.py menu` matches the captures at θ = 1.302 (dirt) and 2.4985 (road).

#### 4.15.6 What a port needs (estimate)

| part | C lines |
|------|---------|
| SKL + ROT readers, the 17-bone tree, recentring, unlinking slot 3 | ~180 |
| paletted texture pages (8-bit texels + 8 palettes from .1–.7) | ~70 |
| wheel loading = model_load with the 0x420a50 kind rules (flag, page ≠ 0, flat-uv) | ~60 |
| the drawer (menu subset of 0x45f290: §4.15.4.3–4.4, LOD 0 only) | ~300 |
| r3d: per-face palette pointer, kinds p0/p1/p2 (flat palette, opaque palette, see-through palette), the 0x49c110 key rule | ~120 |
| camera orbit 0x46dab0 + look-at already in r3d | ~40 |

About 750 lines; the race adds lean/pitch/steering/suspension (already in the formulas
above), the LOD models, lighting and the shadow.

#### 4.15.7 Open points

- The x87 order of each step was not reproduced in the reference tool (doubles rounded
  at the main steps), yet 99.97–100 % of the pixels are exact. The port should still
  compute as the code does: float32 per stored value, the table trig, the inline R(φ,0,0)
  of the swing arm built from COS[0]/COS[0x3000].
- 0x41fa10 after the wheel centring was not read (needed? the picture matches without it).
- PKTKTM's wheel maps (rouearF/roueavF) are not handled by the tool's `menu` command.
- The meaning of 0x4dc130 (lit wheel variant 0x48c6d0) and of the kinds 3/4/9/42 was not
  followed (race).

#### 4.15.8 In the port
- `bike3d.c`: the family's bank pages (0 paletted with its liveries, `r3d_load_page_pal`;
  1 in RGB for the dirt families only), the skeleton (`skl_load`, 0x462c60 with the
  colour rules of §4.15.3.1, the bit-5 loss included), the pose staticPied (`rot_load`),
  the wheels (`model_load_bike`: 0x420a50's kinds and one-texel faces), page 10's camera
  (`bike3d_menu_camera`) and the drawer's menu part (`bike3d_draw`: §4.15.4.3 with
  lean, pitch, steering, speed and suspension at 0). `bikes.c` keeps the family script's
  names (`g_fam`).
- `r3d.c`: `r3d_points` + `r3d_faces` (r3d_view's two halves; with a skin, 0x49c110: the
  farthest corner alone as the depth, a paletted page's faces drawn through the palette:
  kind 0 the palette's colour, 1 palette[texel], 2 the same put off till last and
  skipping colour 0, as kind 0x26). The page seen through palette k is made once.
- Page 10 frees the bike after the frame in which the page ended (the game draws that
  frame's 3D list from the memory it has just freed).
- Test switch `OMR_TURN=ANGLE`: the turntable held. **Checked** (`tools/shots.sh`
  bike_dirt, bike_road): against orig_p10_bike_a (KTMCROSS, θ 1.302) 8 pixels at a boot's
  edge differ, against orig_p10_champ_roadbike (SPEED, θ 2.4985) none; the pointer apart.
- The angle-to-index constant is the game's double 2607.594587618272 (0x4d0300,
  0x4d1bf8, 0x4d00d0), not 16384/2π computed (2607.5945876176133); corrected everywhere.

#### 4.15.9 The shadows and the lighting in the race (mapped 2026-10-30; `shadow.c`, `bike3d.c`, `r3d.c`) [asm]

The faces, the drawing and the picture were already in "4. Bikes, shadows, level of detail,
lighting" (§4.7, §4.3, §4.4 there). What was added to write it:

**The shadow's points 0x461aa0(obj)**, called by the steps (0x45d580, the computer's 0x46bf80)
just before the frame and lean, stored at obj+0x2ea9 (six points):

- d = (0, 0, 50) M (0x410010: M = +0xbc, its translation included); a = pos + d;
  angles = 0x458e20(pos, a) = (atan2(ny, h), atan2(−nx/h, nz/h), 0) on the unit direction;
  R = 0x40fd40(angles); its inverse 0x40fe80; L54 = (40, 0, 0) through the inverse, that is the
  first column of R x 40: across the bike, level with the road. L48 = (0, 0, 40) M.
- front = +0x1b4, which the drawer rewrites as rear + unit(front − rear) x c38 x 96 (c38 the carac
  float at 0x38); the port's drawing doesn't write back to the physics, so `shadow.c` works it out.
- Each point is laid on the road from its own section, straight down (0x40c7e0 from +0x31c1,
  0x40c960, the same height as 0x40c8b0): + 4.0, the two in the middle + 8.0.
- A crashed bike (+0x30c1, and NewMotorEngine 0x52c234: the first int of LEVEL.BPT, 1 on all ten
  tracks, so the port doesn't read it) has four more at +0x30ed round P = +0x30b5 (where the body
  came down): h = 0x490220(+0x3085); M' = R(0, −h, 0), rows X, Y, Z, T; Z2 = Z x 2;
  c0 = (Z2 − X) x 32 + T + P, c1 = (Z2 + X) x 32 + T + P, c2 = (X − Z2) x 32 + T + P,
  c3 = (−Z2 − X) x 32 + T + P; all on the road of P's section + 2.0.

0x4dc130 is no shadow switch (it is the lighting's, below), and the light vector 0x52c228.. has no
part in the points: **the shadow doesn't follow a sun**, it is always straight under the wheels.
The on-board player has no shadow: the cockpit branch of 0x458b80 doesn't call 0x461f70.

**Lighting.** The port's `bike3d_lighting(on)` is 0x4dc130, on round the bikes in `race_frame`, off
for page 10 and the podium. `r3d_dark(dark, swap)` is set before a bike's faces, reset after. So:

- 0x465d60: section flag +0xe0 = 0: lit; else dark. Dark sections per track (`tools/track.py`'s
  flag_e0): Speed Bay 32 (34, and the odd ones of 147..205), West Way 154, Rock Forest 20, Lost
  Ruins 155, Snow Ride 40, Great Wall 18, Red City 263, Dirt Arena 3, ile10 8, ile11 15. The whole
  bike and rider go dark at once on such a section, with no fade; that they are the places in a
  shadow across the road [guess, from where they lie].
- The far model (LOD 2 to 4) and the skeleton: paletted faces halved when dark, kinds 1 → 0x15
  (halves the texel), 2 → 0x27, 42's flat colour halved. RGB-page faces aren't touched.
- The wheels: 0x48c400 in the race goes through 0x4221d0 and 0x48c6d0, whose 0x422540 leaves words
  of 8..24 << 13, below 0x3e000: **a wheel's paletted faces are always halved**, in the sun too. On
  a dark section 0x4221d0 swaps kinds 0x10 → 0x15 and 0x26 → 0x27, which halve RGB texels as well.
- Twin of kind 44: 62 near the camera, as `queue()`'s other twins.
- A paletted flat face (0x4a3cf0) keeps its colour as it is: pal[c] or the raw value, halved when
  dark, with no 5:5:5 step. Only the unpaletted path goes through 0xffc1 → 0x402020, which loses
  green's low bit (`flat_face` had it for both: 166 pixels of the dark bike were one green step off).

**Checked** (captures in the scratch folder, none kept; the original at `-FrameRateMax60`, which
`tools/orig.sh` doesn't pass by default):

- Lit: Speed Bay's grid against `build/ref/orig_race_speedbay_champ.png` and a new capture: the box
  round the player's bike and shadow (240,340..400,460) is equal to the pixel (the build before
  this work: 3318 wrong). In the whole picture only the texts, the countdown's number, and what
  moved (the palm's fronds, gulls, boats) differ: the other riders and their shadows are equal.
- Dark: on the original, memtool wrote 1 in the flags of sections 2, 3, 4 at the player's section 3
  (no throttle, so it stays there), a scratch copy of the port did the same when loading: the box
  240,260..400,460 round the bike, rider and shadow is **equal to the pixel, wheels included**.
- A crash's second shadow was only compared by eye with `orig_fall_a.png`, a different moment.


#### 4.15.10 Dust, spray, tyre marks (mapped 2026-10-30; `effects.c`) [asm]

The bike's step calls **0x466d70(obj, keys)**, keys the step's, +0x2dc8 still the last step's:

- Nothing without a track. Only for bikes with section in [s − 10, s + 30) (wrapped, 0x40ec30) of
  the followed rider's s. So the stream 1 draws depend on where the camera's rider is.
- lat = 0x48d0f0(obj), the sideways speed. m = 0x40ec60(section, pos): the ground's kind (lateral
  l = (pos − section pos) . X; edge[0] < l < edge[1] gives ground_on +0xe4, else ground_off +0xe8).
- **Spray**: on the ground and m > 0: q = ftol(speed/top x 64.0f); if q > 4: dot = (M2 + M3) . vdir
  (rows Z and T), n = ftol(q x (1 − |dot|) x 5.0), size = (q + 2n)/3 with C's division; while the
  bits of the clock 0x4ea508 < 30.0: dir = unit(rear − rear1), a piece at pos with speed x 0.55.
  For the followed rider the clock += dt, past 50 less 50, bits above 50: 0. So spray comes in
  bursts, 30 of each 50 units of dt, for every bike by the one clock.
- Then only if the bits of speed > 5.0:
  - **New strips**: (brake just pressed: !(+0x2dc8 & 2) and keys & 2) or just sliding (!+0x2da4,
    +0x2da0): two strips from the ring (0x457350, 16 of them, an old strip is emptied); half width
    +0x188 = 1.0 (a computer's 5.0).
  - **Marks**: ((keys & 2) or sliding with |lat| > 0.5) and on the ground: (±w, 0, 0) M give two
    points each side of the rear and of the front wheel, added to their strips (0x457200: the road
    of each point's own section + 2.0; at 62 faces the strip stops). A driven bike's w: −1 when
    |lat| didn't grow, +2.0 when it did, within [1, 12].
  - **Smoke**: ((wheelie and bits(+0x3081) < 25.0) or (keys & 2) or (sliding, |lat| > 0.5)) and on
    the ground: 24 puffs for a driven bike, 4 for a computer's; six draws of stream 1 each: r1
    (life = ((r1 & 1) x 2 + 2) x 15: 30 or 60), three of a spin nothing reads, c = r & 3, a = r & 3,
    negated unless keys & 4. Each at (rear1 − rear) x 0.03125 x i + rear, 10 up; speed (a, 0, c) M,
    the translation of M included. Type 5, size 14.

**Computer riders leave no braking marks**: their step (0x46bf80, at 0x46c760) gives this call the
step's keys but puts the old +0x2dc8 back first, and doesn't keep the brake in it, so "brake just pressed" holds
on each step of braking: each step starts two new strips with one pair of points each, which have
no face. (Faithful. Their sliding does leave marks, width 5.)

**Crash 0x467c40(n, pos, base, mask, m)**: three draws for a spin nothing reads; each of n pieces
life = ((r & 1) x 5 + 5) x 8 (40 or 80), f1, f2, f3 = r & 3, size = (r & mask) + base; smoke at pos,
10 up, speed f3 M0 + f2 M1 + f1 M2 + T. The crash: (6, pos, 15, 16, M); the body landing: (4, +0x30b5,
7, 4, +0x3085). So 3 + 5n draws, as the old stub kept.

**The pieces**: 128 at 0x52c498, 0x104 bytes; 0x457620 and 0x4576a0 take the first not live, none
free: nothing. Only their own life ends them, even over a race's end; the strips go at a start
(0x457320). One move a step, 0x457590 from Game_Frame step 15 (0x4b0730, read in the asm; the
decompile's ftols aren't to be trusted there):

- Type 5 (smoke, 0x4afe90, kind 0x44 = 68, page 1): old = size; size = ftol(dt x 0.6 + old); a
  square of half side 2 x old centred on the point; v.y += dt x −0.0864; y += v.y dt; v.x *= 0.8
  (z the same); life −= dt, dead below 0. **The size's cut depends on the frame rate**: at dt 1.6
  (`-FrameRateMax60`, the port) 0.96 is lost and smoke stays 14; at dt 3.3 (the hidden display
  without the switch, slow machines) it grows by 1 a move. The memory shows both: 14 everywhere at
  dt 1.6 (15 after a frame of 17 ms), 14 + moves at 3.3, up to 32. So captures to compare smoke with
  must be taken with the switch. Texels (224,160)..(255,191): one frame of the page's strip
  of 32 x 32 (u 224..255), set again at each move, the first before any picture; the other frames
  aren't used.
- Types 1..4 (spray by the ground's kind, 0x4b00e0, kind 0x26 = 38, page 1; other kinds no piece):
  size n/2, top n, height 1. While 2 top >= size and size <= 256: size, height += dt x 3.84, each cut
  to an int; then height −= dt x 1.28, size += dt x 1.28. half = size/2; height < 0: dead; quad
  (−half, h, 0) (half, h, 0) (half, 0, 0) (−half, 0, 0). v = dir speed, v.y += dt x −0.024525, made
  unit again; pos += dir (speed dt 0.01 x 128). Texels and colour by type in `effects.c`.
- **Drawn** 0x4575c0 → 0x4b0be0: detail >= 2; at detail 2 one in 8 pieces. The camera's heading
  +0xc0 (0x490220(C)) x 2607.594587618272 (0x4d2080) gives a rotation built from the game's cos
  table with g_cos[0] and g_cos[0x3000], so about Y: upright squares facing the camera, two faces
  back to back.
- **Kind 68** (in the see-through batches): each channel towards white by a/32, a = texel & 0x1f:
  s + ((31 − R) a >> 5) << 11, the same for G's top five bits and for B. Kind 69, its twin, wasn't
  seen created and isn't written.
- **Strips** (0x4573e0 after the pieces): kind 0xc, colour 0x4208 = pack(8, 8, 8). **Kind 12**, in the
  batches: half the screen + half the colour, no texture (0x402020 packs 0x4208 → 0x2104), so the
  road goes dark.

Stream 1 is used otherwise only by `rules.c`'s choice of a voice (`rand_next(1) > 0x3fff`), so these
draws change which voice speaks, as in the game, and never a rider's place: `tools/replaycheck.sh`'s
logs stay equal byte for byte with the build before. In `physcheck` and `cpucheck` the hooks are NULL
and the crash keeps drawing its 3 + 5n: their reports are the same.

The order in a picture: bikes each with its shadow, the pieces, the strips.

**Checked** against the original at `-FrameRateMax60` (no two rides are in step, so not to the pixel):

- The table 0x52c498 read beside the pictures. Smoke as above. On grass at speed the original's
  type 3 pieces run (size, height) (8,7) (14,13) (16,10) (18,7) (20,4) (22,1) with top 4 at speed
  17.8; a scratch build of the port printed the same steps on sand (type 2): (8,7) (16,10) (20,4)
  with top 4, and (8,7) (16,10) (22,1) with top 5.
- Under the brake the port's 128 pieces are all taken and the smoke comes in pulses as slots free.
  The original's table was only read at speed under dt 3.3 (100 to 119 live) and after the stop
  under dt 1.6 (10 to 30 live), not during a brake at dt 1.6; its pictures show the same pulses.
- By eye, brake from speed on Speed Bay's straight and on the pavement: the same white patch of
  smoke behind the rear wheel, towards pink on sand, the same dark strip in the middle. No smoke at
  12 on the dial in the original, none at 15 in the port; 5.0 in the bits of speed is 18 on the dial.
- Not compared with a capture: a computer rider's marks, a crash's debris, spray at detail 2 (one
  in 8), and whether the spray's pictures on the road look the same (they are small and rare in
  the captures). These rest on the asm.

### 4.16 The race: load, grid, frame (mapped 2026-10-26, in progress)

The race object 0x52c478 (vtable 0x4d04c8): +0 0x47bf30 defaults, +4 **0x47b420 load**,
+8 **0x47bf70 start** (the grid, every rider's state), +0xc 0x47d620 unload, +0x10
0x47ec40 (the frame's time), +0x14 **0x47d660 Game_Frame**, +0x18 **0x47ecd0 the step**
(dt 0x4e9808), +0x1c 0x47ec50, +0x20 **0x47ff90 the drawing** [flat image].

#### 4.16.1 Riders (0x410a50) [asm, decomp]
- Records 0x581218 + k x 0x3209; k = 1 is the local player (0x584421), the opponents
  k = 2 .. riders + 1. 0x509bac = the riders + 1 (25 for 23 opponents).
- Riders 0x542874 (0x47b420): practice 1, else 23, or 11 with OPPONENTS "12" (0x4dbfdc).
- Player: 0x45bf70(rec, 1, 1, skin, 0) with skin = the chosen bike (if page 0 is
  paletted, 0x4f9300), else -1. Opponents: +0x78 (the carac row) = 0, 0x45bf70(rec, 0, 1,
  skin, 0) with skin = k - 1, or 0 when k - 1 is the player's bike; -1 if the player's
  is. +0x88 = 1 for an opponent, +8 = 1 for the player. Every one first placed at the
  Level.Spt start point 0x51bfd8 (0x45c7c0); the grid comes at the start.
- 0x45c7c0(rec, x, y, z): +0x30 = +0x3c = +0x48 = the point; rear wheel's ground point
  +0x338 = p + (0, c40 x -96.0, c38 x -48.0) M + T, front +0x1b4 = p + (0, c3c x -96.0,
  c38 x 48.0) M + T (M = +0xbc, T = +0xe0; floats 0x4d0e90/94/98); +0x4bc..; the section
  by 0x40c7e0 from +0x31c1 into +0x31c1 and +0x31bd; copies of the point at +0x2df5..
  +0x2e19; +0x198 = M row 2 + T [asm 0x45c7c0].

- The bikes' pages in a race (0x410590): page 0 = FileBnk0 with the flag 0 (paletted, the
  liveries), page 1 = FileBnk1 with the flag **(Level.Spt bank 1's number & 4) >> 2**
  (0x51d16c; 5 on every track, so 1: read in RGB), not the dirt flag page 10 uses. A road
  family's page 1 thus holds its picture in a race; one face of the medium model uses it.
  (Level.Spt's own bank 0 and 1 names are not loaded.) [asm 0x4105c4, 0x4106e1]

#### 4.16.2 The grid (0x47bf70, Solo) [asm 0x47c0ca..0x47c4cc]
- dir = 0x4dc140 (1, or -1 reversed). s = wrap(dir x 0x509bac x 3) (0x40ec30 brings a
  section number into 0 .. count - 1); reversed: s = wrap(s - 3 dir).
- For k = 0x509bac - 1 down to 1 (the player last, so at the back): if not practice,
  s = wrap(s - 3 dir) (in practice all on one section). p = (0, 0, 10.0) through the
  section's +0xc matrix (axes + the offset +0x30, 0x410010) + the section's +0;
  not practice: p += off x axes[0] + offset, off = +128 for even k, -128 for odd (an
  int through fild). The two plane distances 0x4b10c0 computed there are thrown away.
- 0x45bdf0 clears the rider's state. Not reversed: +0xbc..+0xe8 = the section's 12
  floats from +0xc (axes, offset). Reversed: z = offset - axes[2] (0x4570d0 is a - b;
  the offset is 0, so -axes[2]) made unit, x = (0, 1, 0) x z and y = z x x (0x46d610 is
  a x b, (out, a, b)), then x and y made unit; rows +0xbc x, +0xc8 y, +0xd4 z; +0xe0 = 0
  [asm 0x47c28e..0x47c3ff].
- 0x45c7c0(rec, p); +0x31c1 = s; +0x2f7d = +0x2f81 = 0.

#### 4.16.3 In the port
- `race.c`: page 1 loads the race (`race_load`: the track, the bikes, the riders, the
  grid) and hands over (`menu_leave`); `race_frame` draws the sky, the track and the
  riders (`bike3d_draw_rider`, the detail by distance as 0x45f160: >= 3000 the very low
  model, >= 2500 low, < 1000 the skeleton, else medium). Escape: the pause menu.
  Practice's second record is not drawn [guess: it is the time-attack ghost].
- 2026-10-26: the player's bike is `bikephys.c` (bit-exact against the original's recorded
  steps, tools/physcheck.sh), stepped in `step()` with bike against bike; the drawing's
  Rider copies the frame's matrix (the tumbling one when crashed), the place, the wheels
  and the section. `camera.c` is the chase 0x46f840 (§4.20 §2.5) with F5's distances, Tab's
  look back and the finish orbit; it starts as 0x47bf70 sets it, 191 above its place, and drops
  in at 0.8 a frame [memory of the original; written 2026-10-31, it had started at rest]. The dials take O+0x118, O+0x11c / bike+0x70 x 16000 and the gear. The
  computer riders are bikephys records with driven = 0: they stand on the grid until
  0x46bf80 is written. Not yet: F6 on-board (the cockpit 0x462110), the TV director,
  joysticks. The drawing has the lean, pitch, steering, suspension, spins (§4.15.4).
- Smooth pictures (the port's, `g_smooth`, F10, `openmotoracer.ini` in the home folder):
  the riders and the camera of the last two steps are kept and each picture is drawn
  between them by the time since the last step (so 16 ms behind); a crash starting or
  ending, a bike moved > 1000, a camera cut (Tab, the start) are drawn as the step left
  them. What the drawer gives back to the game (+0x3085, +0x30b5) is computed from the
  step's own state (bike3d_body) after each step, never from a picture: the same ride
  with it on and off leaves the player's bike identical to the bit at every frame.
- `input.c`: the keys held, for the race; OMR_INPUT gains `hold:KEY` and `release:KEY`.
- `rules.c` (§4.17): the countdown and time limit 0x47dae0, laps/checkpoints/wrong way/
  finish 0x47e3d0, places 0x47e1e0 with the C library's quicksort as the game has it
  (0x4b3100, checked against its machine code: the 1998 MSVC qsort, shortsort at 8),
  fastest lap 0x481800, lap record 0x458310 (against bestscor.dat's best lap). The race
  runs a step of 16 ms (dt 1.6) for every 16 ms of the clock, as GOG's original does with
  `-FrameRateMax60` (§4.2, §4.18.1). The countdown's beeps and voices
  (0x4082f0) once a picture, as the dashboard plays them; recorded: "3" "2" "1" "GO" at 0,
  2, 4, 6 s. The dashboard's texts, splits shown, events (turn signs) not yet.
- Sounds: the race's set 0x4044c0 (`sound_race_load`: Ktm or Ninja by the bike's
  PersoAnim, Divers, Voices; the track's ambience not yet), freed at the race's end; the
  menu reloads its own when it becomes active again (0x40a9a0). Music: Level.Spt's
  AudioTrack (after FactorPocket), played at the start.
- Level.Spt after the banks, in the order 0x427e80 reads it: StepGlobal, YScaleLevel,
  StartPosition x3, ComputeLandOptimise (ints); 9 detail columns x 5 (MediumZone and
  HightZone ints); **13** AI rows x 3 floats (RATTRAPAGE_SPEED/CURVE/COUPLE, ATTENTE1 x3,
  ATTENTE2 x3, CURVE_RANGE, COUPLE_OUT_RANGE, COUPLE_IN_RANGE, SPEED_FACTOR); NbComputer
  + that many floats; TimeSkill x3; NbCheckPoint + that many ints; FactorPocket; AudioTrack.
  The .trk reader (0x40df3c) gives the checkpoint sections, in section order, the
  TimeCheckPoint values one after the other; 0x47b4ff counts them into 0x4e9844. Every
  track: as many checkpoint sections as NbCheckPoint [test].

### 4.17 The race's rules and state machine (mapped 2026-10-26)

Mapped 2026-10-26 from build/mr.asm (objdump), build/decomp, the image (tools/peek.py reads
values at their addresses), the LEVEL.BPT scripts (tools/bpt.py) and the memory captures in
build/trackcap/. Tags: **[asm]** read in the machine code, **[decomp]** Ghidra's text only,
**[guess]**, **[cap]** checked in a capture of the running original.

Out of scope here (other helpers): the bike physics (race vtable +0x18 0x47ecd0, the rider
steps 0x45d540 / 0x45e940, the inputs' meaning), the drawing, cameras and HUD (vtable +0x20
0x47ff90, 0x46f840), the load 0x47b420 and the grid 0x410a50. Link (network) play is only
marked where it branches.

#### Summary (read this first)

**The frame** (Game_Frame 0x47d660, §1): clock (ms, clamped, §2) → in-race dialogs and the
event word 0x4e8918 (§3; a dialog with +0x10 holds the race; the event handler 0x4587e0
unloads / restarts / changes page) → inputs and replay record/playback (part C §4) → start
countdown and time limit (§4) → dt = ms·0.1 and every rider's step in rider order (§5) → the
physics step 0x47ecd0 → laps, checkpoints, wrong way, finish, places (part A) → cameras →
engine sounds, ambience and the voice queue (part C) → the end-of-race machine 0x412a50
(part B) → position commentary → draw → limiter. The Pause key (or Ctrl+P) sets 0x4dc038 and
App_OnIdle then skips the race entirely; Escape opens the pause menu (part B §3).

**The rules in one paragraph.** 6 s countdown (0x4dc0a8 from 6000; numbers "3/2/1" at 6, 4,
2 s and GO, drawn and voiced by the HUD), riders held by the step while 0x4dc0b4. At GO the
time left = the start section's TimeCheckPoint + TimeSkill[difficulty] (both from the
track's Level.Spt; table in §4); each checkpoint passed on new ground adds its
TimeCheckPoint (5 s "extended time" message); the time runs out half a second below zero:
GAMEOVER voice, the player out (state 3); during that half second below zero the HUD
warns and TIMEOUT plays (at most every 2 s, so once)
(single race, championship, time attack; practice has no limit). A lap counts when the line is
crossed with progress = furthest progress; the first crossing from behind the line counts
none. At laps = 0x4dc074 the player is finished (state 2), the race is over (0x509ba0) and a
place voice plays (winner WELLDONE/YEAAH, 4th or worse YOULOST). Then timed end sequences
(7 s, a winner's 14 s lap of honour with the computer riding the bike, 3 s after time over),
then per mode: results dialog with REPLAY YES/NO/SAVE (single race / championship), or the
ghost save + hi-score table + CONTINUE YES/NO (practice); bestscor.dat updated in every Solo
mode; back to the menu by event bits (page 9 TOP 5, page 6, page 24 podium, 18 championship
end). No championship points in Solo: a race in the top 3 advances it. Places: finishers by
race time, racers by progress, then the rest, sorted with MSVC's qsort (same qsort needed).

**Corrections to docs/NOTES.md**: §4.2 "at least 1.0" — the floor in 0x47df40 is a compare
with 0.0 (0x4d1900 = 0.0f), so the step is ms·0.1 for any frame of 1..200 ms. 0x411b30 is
the joystick reader, not the computer riders' driving. 0x412e50 is the split time at a
checkpoint, not the start.

**Confidence**: the frame order, the clock, the countdown/time limit, the rider steps' order,
laps/checkpoints/places and the end machine are read in the machine code; the dialog engine's
internals (keys, layout, drawing), the victory lap 0x48acd0/0x48ae40 and Link are not mapped.
Checked against the captures (0x4dc0a8 during and after the countdown on SNOW_RIDE and
WEST_WAY, 0x4dc150 ≈ 33 ms at the original's 30 pictures a second, section counts) and the
TOP 5 capture after QUIT RACE. Nothing was checked against a whole race of the original.

**Size of a C port**: this part ≈ 250 lines (frame, clock, countdown/time limit, steps
loop, start) + part A ≈ 500 + part B ≈ 900 (+ 600–900 for a dialog engine unless the port
draws these menus with its own widget) + part C ≈ 950 (sounds ≈ 570, replays/demos ≈ 250,
joystick ≈ 120) — about 2,600–3,500 lines in all.

**Open points** (each part has its own list below): what +0x2f89 holds; whether the player's
grid slot is behind the line (then the first crossing also adds section 0's time); race
object +0xc and +0x10 = 1's writer; 0x4ea4c4 (no writer, 1 in the image); the [SAVE] row's
condition 0x4e8924 (`-saverecord`) and the replay writer 0x4579b0 (not ported); obj+0x2da0 and the +0x31f9 button
(physics); confirm the finish voices and the time-out by a recorded race of the original.

#### 0. Conventions

- **Rider records**: `R(k) = 0x581218 + k·0x3209`, k = 0..24 (25 records; loops that run
  "0x581218 .. < 0x5cf4f9" cover 0..24, loops "0x584421 .. < 0x5cf4f9" cover 1..24). R(k) is
  the bike object of NOTES §4.15.2 (0x584421 = R(1)). Riders in a race are k = 1..; the local
  player is `0x4ea4c0` (1 in Solo), the rider whose keys are read is `0x4dc0fc` (1 in Solo).
  [asm: the loop bounds in 0x411010, 0x47d660, 0x47dae0]
- Fields used in this part (offsets from R(k)) [asm unless marked]:

| off | what |
|-----|------|
| +0x4, +0x8 | +8 = 1 the player (a human), computer riders +4 = 1 and +8 = 0; Link: +4 = 0, +8 = 1 a remote player (part A; 0x411122 picks 0x4dc0fc in Link from them) |
| +0x68 | loaded / in the race (loops skip a record with 0) |
| +0x88 | automatic gearbox: 1 for every computer rider in Solo, the menu's 0x4dc0a0 for R(1) (0x4110c4) |
| +0x2ddd | race time, ms (int), counts while the rider's state is 0 or 1 (0x47dcdc) |
| +0x2de1 | best lap ms (0x7f7f7f7f = none) (part A) |
| +0x2de5 | **rider state**: 0 = not yet over the start line, 1 = racing, 2 = finished, 3 = out of the race (time over 0x47deec; championship quit 0x43791f) (part A) |
| +0x2f79 (i16) | the next checkpoint's number, 1 at the start (k = 1..count−1) [asm 0x4112b2] |
| +0x2f7b (i16) | laps completed (indexes the lap-time row 0x5db290; = 0x4dc074 when finished, 0x47eae2) |
| +0x2f85 | finishing place (Link only, Game_Frame) |
| +0x31c1 | current track section (0x40c8b0 on the track object 0x52c108 finds it from the position, 0x45eb11) |
| +0x31e1 / +0x31e5 | control kind (2 or 4 = a joystick, read by 0x411b30) / controller number (part C) |
| +0x31e9 / +0x31ed | key bits from the keyboard / from a joystick (or'ed together when stepping; part C) |
| +0x31f1 / +0x31f5 | analog x / y, 0x8000 = centre |

- `0x5db290 + k·0x20`: 8 ints per rider, the **time of each lap** in ms (row k, column = lap
  index +0x2f7b) [asm 0x47dd1d].
- `0x509bac`: the number of rider records in use (loops run k = 0 or 1 .. < 0x509bac).
- `0x4dbffc`: 1 Solo, 0 Link (NOTES §4.12.8). Every "Link only" below is `0x4dbffc == 0`.

#### 1. Game_Frame 0x47d660 (race vtable slot +0x14), the order [asm]

ECX = the race object (0x52c478). Race object fields: +4 = 1 once started (0x410132,
0x4113a9), +8 = "finish voice played" (Link block below; 0 at start), +0xc = 1 always (set by
0x410120 in both branches, and by 0x411010 on a restart from a finished race): enables the
time-out warning, +0x10 = demo/replay stop request (2 = stop, 0x4121e0), +0x14 = 0.

1. If `0x4e4f00 == 1`: call `[0x53be00]->vtable[+0x50]` and clear it (a display object's
   restore; set at 0x41ae7f) [asm; drawing side].
2. If `0x4e4f08 == 1` (D3D: not enough texture memory, set by 0x41f223/0x41f245): message box
   0x43c4e0("[WARNING]", "[TEXTURES : NOT ENOUGH]\n[VIDEO MEMORY]", button "[OK]" 0x4f, id 0xb)
   and give the dialog 0x4e8910 the close test 0x47dab0; 0x4e4f08 = −1. Else if `0x580a58 == 1`
   the same with "[NOT ENOUGH VIDEO MEMORY]\n[DIRECT3D NOT AVAILABLE]\n[IN THIS MODE]" (id 0xc).
   **0x47dab0** closes the box when `0x4e9ce4 != 0` or 10 s after it opened
   (`0x50bbc8 − box+0x64 > 0x2710`, unsigned) [asm]. D3D only: the port never shows these.
3. `0x4f0c4c = 0x4dc14c; 0x4dc14c = 0` ("textures changed" flag set by the texture loaders
   0x40108c, 0x401d00, 0x41c27b…; drawing side) [asm].
4. **0x4120e0** the clock (§2). `0x51d294 = 0`.
5. **0x4126a0** the dialogs and the event words (§3). Returns 0 = the race is held this frame.
   - 0 (held): if the player's control kind `0x4dc11c` is 2 or 4 (a joystick) and not (dialog
     0x4e8904 exists and its first dword == 9): **0x411a10**, which turns the joystick's
     directions and button 0 into arrow/Return key messages so it drives the dialog (part C). Then the debug log 0x4961d0(0, "handleMenu is FALSE").
     Go to step 17.
   - 1: 0x4961d0(0, "handleMenu is TRUE") — 0x4961d0 is a debug log, nothing for the port
     [asm: writes only when 0x4f1810 is set].
6. `0x5429b0 = 1` (the local player's controls are read; see §6).
7. **0x4121e0** the demo/replay overlay (part C). 0 → step 17 (a caption page is up, the race
   waits).
8. **race->vtable[+0x10] = 0x47ec40 → 0x4116f0**: this frame's inputs (keyboard and joystick bits, the
   replay's record or its recording; part C). The computer riders get no inputs here; the
   bike step drives them. 0 → step 17 (playback ended).
9. `0x4e9810 = _ftol(0x4e9808)` — this frame's milliseconds as an int (0x4e9808 is still in ms
   here; _ftol truncates). `if (0x519fec > 0) 0x519fec −= 0x4e9810` (the "extended time"
   message's 5 s timer, set to 5000 by 0x47e62f in 0x47e3d0; read by the HUD 0x4823c9,
   0x483d2b) [asm].
10. **0x47dae0** the start countdown and the time limit (§4).
11. **0x47df40** dt to step units, then every rider's step (§5).
12. **race->vtable[+0x18] = 0x47ecd0(dt)** (physics helper). Link only: 0x457050/0x456380 send
    this player's state (message 0x66) to the others [asm 0x47d7e4..0x47d86a].
13. `0x48b7a0(0x5f6778, dt)` (animated decor), `0x459000()` + `0x5429cc = 0x459090(1)`
    (an rdtsc profiler, debug only), `0x457590(dt)` (128 dust/smoke sprites),
    **0x47e3d0** (laps, checkpoints, wrong way, finish: part A), **0x47e1e0** (the places
    sort, part A), `0x4dc0f8 = 0` (the camera/view index the cameras loop over),
    `0x4583e0(dt)`, `0x458620(dt)` (camera mode and update; drawing helper). dt here =
    0x4e9808 = step units.
14. If `0x4dc138 != 0` (a camera flag): `0x467b30(R(k), 1)` (the rider skeleton's pose) for
    every loaded rider k = 1..24. If `0x4db65c != 0` (sound on): **0x4079e0** (engine sounds
    and shouts, part C). Always **0x405f30** (ambience, and the voice queue 0x5028c8 where every
    0x404da0 voice of the race actually starts, part C).
15. **Link only — all finished** [asm 0x47d905..0x47d9f5]: if no loaded rider k = 1..24 has
    state +0x2de5 ∈ {0, 1}: `0x509ba0 = 1`, 0x47e1e0 again, then for every rider i = 1..24
    with R(i)+8 != 0: `R(i)+0x2f85 = 0x459f50(i)` (its place); for the local network
    player, once (race+8 = 0 → 1): voice 0x404da0(id, 500) with places counted from 0:
    winner (0) → 0x50 WELLDONE if the random 0x458cd0(−1) < 0x4000 else 0x4f YEAAH;
    place == 0x509bac − 1 → 0x48 YURALOSR; place ≥ 3 → 0x47 YOULOST; 2nd and 3rd get none.
    (Solo plays the same set from 0x47e3d0 at the finish, part A.) (Part B maps the Solo end.)
16. **0x412a50** the end of the race per mode (part B). If it returns non-zero: if kind
    0x4e9834 == 0 and `0x509ba0 == 0` (not over) and `0x52c238 == 0` (not out of time) and mode
    0x4dc094 != 0 (not practice) and the player's state == 1: **0x407420** (part B). Then
    **race->vtable[+0x20](dt, −2)**, the drawing.
17. `0x5809f4 = 0x459fd0()` the frame limiter (§2.3); `0x4e8918 = 0x4e891c = 0` (the event
    words are consumed every frame).

So the order of one frame is: clock → dialogs/events → (demo caption) → inputs → timers →
countdown/time limit → rider steps → physics step → race rules → cameras → sounds → end test
→ draw → wait.

#### 2. Timing

##### 2.1 0x4120e0 the frame's milliseconds [asm]

- `last = 0x50bbc8; 0x459fa0()` (0x4e9cfc = timeGetTime) `; 0x50bbc8 = 0x459fc0()` (= 0x4e9cfc);
  `0x4e980c = 0x50bbc8 − last` (u32 ms since the previous frame's reading).
- If kind 0x4e9834 != 1 (not a .Sav replay): the running mean
  `0x4dc150 = (f32)( ((double)(int)0x4e980c − 0x4dc150) · 0.125 + 0x4dc150 )` —
  `fild dword; fsub dword 0x4dc150; fmul qword 0x4d0380 (0.125, bits 3fc0000000000000);
  fadd dword 0x4dc150; fstp dword`. **Unclamped** ms. Captures: 33.0 ± 0.3 at the original's
  30 pictures a second under wine [cap: all p*/q* dumps]. Readers: 0x46a893.. (physics:
  divides by it) — for the physics helper. In replay playback 0x4116f0 does the same mean
  with the replay's ms.
- `0x4e9808 = (f32)(u64)0x4e980c` (fild qword with a zero high half). If it is > 200.0
  (f32 0x4d0398, bits 43480000): 0x4e980c = 200, 0x4e9808 = 200.0. Else if its bits as an
  int are ≤ 0 (elapsed 0): 0x4e980c = 30, 0x4e9808 = 30.0 (bits 41f00000).
- `0x4dbfb4 = _ftol((f32)0x4dbfb4 − 0x4e9808)`: the race's mouse-pointer timer (set to 3000 by
  the dialogs 0x4334dd…; below 0 the pointer animates in 90 ms frames and is hidden after −630,
  0x45a21b). Then 0x45a190(0x4dbfac) puts that pointer, and 0x4178c0(−1) + 0x418790 on the two
  text lists 0x53ba68 and 0x53b8e8 (their frame update, NOTES §4.8.3.6).

##### 2.2 The step's dt [asm 0x47df40]

`0x4e9808 = (f32)(0x4e9808 · 0.1f)` (f32 0x4d1970, bits 3dcccccd), compared with
**0.0f** (0x4d1900, bits 00000000, .rdata): below 0 → 1.0. **Correction to NOTES §4.2**: the
floor is a test against 0.0, not "at least 1.0"; with ms in 1..200 the step is 0.1..20.0
units of 10 ms. 0x4e9810 (int ms) is taken before the scaling. Everything after 0x47df40 in the
frame (the riders' steps, 0x47ecd0, 0x48b7a0, 0x457590, the cameras, the drawing) gets the
scaled dt. At 60 pictures a second: 16 or 17 ms → 1.6 / 1.7 (the port's fixed 1.6667 is the
mean).

##### 2.3 0x459fd0 the limiter and the fps counter [asm]

- `prev = 0x5809e0; start = timeGetTime(); 0x5809e0 = start`. If `0x4dc200` (the
  `-frameratemax N` value) != 0: `wait = 1000 / N` (idiv) and loop
  `now = timeGetTime(); 0x5809e0 = now` until `now − prev ≥ wait` or `now − start ≥ wait`
  (both unsigned). So the frame is padded to 1000/N ms from the end of the previous wait, but
  never waits more than 1000/N in one call.
- Then the counter: the 24 dwords 0x5809f8..0x580a54 shift up one (oldest out), `0x5809f8 =
  start − prev` (the frame's time without its wait), the count 0x4e9d00 + 1 (at most 24, cut
  back to before any entry > 1000 ms); returns `1000·count / sum` (0 if the sum is 0) — the
  frame rate shown by the debug display (0x5809f4). The port only needs the wait (60 Hz).

#### 3. 0x4126a0 dialogs and events (the "handleMenu" test) [asm]

Four in-race dialog slots (part B §4): **0x4e8910** message boxes and the display-mode list
(its items write **0x4e891c**, handled by race slot +0x1c 0x47ec50: display re-init, not
rules), **0x4e890c** F1 help (event word 0x4e8914, unused in practice), **0x4e8904** the
results / pause / continue dialogs (event word **0x4e8918**), **0x4e8908** the F11 sound and
F12 display menus (no event word).

1. 0x43e0f0() (part B).
2. For each of the four, in that order: if it exists and +0x28 != 0: 0x434960(dialog) (its
   frame).
3. For each, in the same order: if it exists and has a close test at +0x30 and the test
   returns non-zero: call +0x38(0) if set, else +0x20 = 1; then if still there and (+0x20 != 0
   or +0x24 == 0): close it (0x435810, 0x435620, 0x4356d0, 0x435770 respectively).
4. `if (0x4e891c) race->vtable[+0x1c](&0x4e891c)` (0x47ec50, part B). If bit **0x40000** of
   0x4e891c, or dialog 0x4e8910 exists with +0x10 != 0 (modal): unless bit **0x80000**, draw
   the race once `race->vtable[+0x20](dt, 0x4e8910+0x18)`; **return 0** (held).
5. `if (0x4e8914) 0x4587e0(&0x4e8914)`; the same test with 0x4e890c → return 0.
6. `if (0x4e8918) 0x4587e0(&0x4e8918)`; the same test with 0x4e8904 → return 0.
7. Return 1.

So bit 0x40000 in an event word = "hold the race this frame", 0x80000 = "and don't draw it"
(e.g. the race was unloaded by the event), a dialog with +0x10 = a pause. Note the asm reads
`0x4e8910+0x18` even when only the bit (not the dialog) caused the hold.
Since step 17 of Game_Frame clears 0x4e8918 and 0x4e891c (not 0x4e8914) at the end of every
frame, an event lives one frame.

#### 4. 0x47dae0 the start countdown and the time limit [asm]

Globals: **0x4dc0ac** = 1 for every race (0x410120 sets it to Solo, 0x411010 sets it to 1 in
Link; nothing clears it) — the routine does nothing when 0. **0x4dc0b4** = 1 while the start
countdown runs. **0x4dc0a8** = the countdown, then the **time left**, ms (int).

Set-up by 0x411010 (the race's (re)start, called by the load's 0x410ee0, by 0x4587e0 for a
restart, and by the Link handlers 0x454330/0x454680) [asm 0x411279]:
`0x5429b0 = 0, 0x4dc0b4 = 1, 0x4dc0a8 = 6000` (0x1770), lap fields of R(1..count−1)
(+0x2f79 = 1, +0x2f7b = 0), 0x50bbc8 = 0x51d29c = timeGetTime, 0x4e9818 = 0; then replay
record start 0x4577c0 (kind 0) or playback start 0x4577e0 (part C). (If 0x4dc0ac were 0:
0x4dc0b4 = 0, 0x5429b0 = 1, no countdown.)

Each frame:
1. **Link only, while counting**: the synchronised start (0x4e982c, 0x4f0c50, 0x4dbff8 host,
   0x508030.. player table, DirectPlay latency 0x4dc104->+0x4c, qsort, busy-waits, 0x456d40/
   0x456d80, 0x4e892c) when ≤ 3000 ms are left. Not mapped (out of scope).
2. `0x4dc0a8 −= 0x4e9810` — always in Solo (in Link it stops at 3000 until the start is
   synchronised).
3. **Not counting** (`0x4dc0b4 == 0`): for every R(k), k = 0..24, loaded (+0x68), state
   +0x2de5 ∈ {0, 1}, and (k == 0x4ea4c0 or R(k)+8 == 0): `R(k)+0x2ddd += ms` and
   `0x5db290[k·8 + R(k)+0x2f7b] += ms`. So the race time and the lap times count only after
   the start, only while the rider races, and in Link only for the local player and the
   computer riders.
4. If `0x4dc0a8 < 0`:
   - **Counting → the start** ("GO"): 0x4dc0b4 = 0. If the track object's start section has a
     checkpoint (`i16 [0x52c108]+0xd8 ≥ 0`): `0x4dc0a8 = i32 [0x52c108]+0xda` + (Solo:
     `0x515fe0[difficulty 0x4dc060]`; Link: 120000 = 0x1d4c0). The countdown's own overshoot
     is dropped (the value is set, not added). Then for every loaded R(k) with (k == local or
     +8 == 0): race time +0x2ddd = 0 and its 8 lap times = 0. Then for k = 0..0x509bac−1:
     with s = R(k)+0x31c1 (section) and n = `0x52c10c` (the track's section count):
     **not reversed**: state +0x2de5 = 1 if 0 ≤ s < n/2 (n/2 = C division, rounded to zero);
     **reversed** (0x4dc144): state = 1 if s < 0 or s ≥ n/2. Otherwise the state stays 0.
     [asm 0x47de0d..0x47de35; an earlier reading of mine had the two sides swapped.] So a
     rider already past the line (first half of the lap; last half when run backwards) races
     at once (state 1: his next crossing counts a lap), while one still behind it stays 0 and
     his first crossing only turns him to 1, counting no lap (0x47e3d0, part A). Note this
     test runs over k = 0..0x509bac−1, the other loops over loaded records. `0x5429b0 = 1`.
     Return.
   - **Racing → out of time**: only when not a replay (0x4e9830 == 0), the local player's
     state is 0 or 1, and (mode 0x4dc094 != 0, or time attack 0x4dc148): if
     `0x4dc0a8 + 500 < 0` (half a second of grace below zero): `0x52c238 = 1` (time over),
     0x482120(0,0,0,0,0) (the HUD's "time over" text, 5 pushes), voice **0x404da0(0x39 GAMEOVER, 500)**,
     player state = **3**. Else, if race+0xc: 0x481af0(0,0,0,0,0) (the HUD's warning) and every
     2000 ms (`0x4f0c54 + 2000 < 0x50bbc8`, unsigned; 0x4f0c54 = now) voice **0x42 TIMEOUT**.
     **Practice without time attack has no time limit.**

**The time table** — `0x515fe0[3]` is **TimeSkill** from the track's Level.Spt (0x427e80 at
0x428a9c: the first int by 0x425bb0, the next two by 0x425080; the image holds zeros), the
section time +0xda is **TimeCheckPoint** (0x509228.., NbCheckPoint 0x4dbfd0; part A for how
they map onto sections and how 0x47e3d0 adds them). Values (tools/bpt.py on each LEVEL.BPT):

| track | folder | TimeSkill easy / medium / hard (ms) | NbCheckPoint | TimeCheckPoint (ms) |
|-------|--------|------|---|------|
| SPEED_BAY | ile01 | 25000 / 20000 / 15000 | 3 | 15000 15000 13000 |
| WEST_WAY | ile02 | 50000 / 23000 / 20000 | 3 | 13000 15000 11000 |
| ROCK_FOREST | ile03 | 33000 / 15000 / 10000 | 3 | 11000 17000 12000 |
| LOST_RUINS | ile04 | 35000 / 25000 / 20000 | 5 | 21000 20000 18000 20000 20000 |
| SNOW_RIDE | ile05 | 50000 / 25000 / 20000 | 5 | 15000 16000 14000 13000 13000 |
| GREAT_WALL | ile06 | 38000 / 24000 / 20000 | 3 | 17000 16000 16000 |
| RED_CITY | ile08 | 25000 / 15000 / 8000 | 4 | 18000 13000 14000 15000 |
| DIRT_ARENA | ile09 | 30000 / 25000 / 20000 | 5 | 21000 21000 19000 20000 15000 |
| (ile10) | ile10 | 25000 / 20000 / 15000 | 3 | 18000 25000 25000 |
| (ile11) | ile11 | 40000 / 25000 / 20000 | 6 | 12000 15000 14000 18000 11000 12000 |

Check [cap]: the captures of single races on medium (0x4dc060 = 1): SNOW_RIDE right after
the start q45 0x4dc0a8 = 37752 ≤ 25000 + 15000; WEST_WAY q16 35109 ≤ 23000 + 13000; during
the countdown (0x4dc0b4 = 1) q14 5434, q15 2264, q43 4079, q44 908, all below 6000. The
captures never show a value above those sums.

**Where the riders are held on the grid**: not by this routine. While 0x4dc0b4 = 1 the rider
step 0x45d540 skips its motion call (0x45d722) and plays the rider's start pose from
`2000 − 0x4dc0a8` (0x45d87d, the last two seconds), 0x45eaf6 skips 0x4660f0, the joystick reader
0x411b30 gives no bits (0x411be3) — for the physics helper [asm, only the tests read].
The countdown's numbers, lights and voices come from the HUD (0x413410 → 0x413030 /
0x413180 reading 0x4dc0a8, part A), so they follow the pictures drawn, not the steps.

#### 5. 0x47df40 dt and the riders' steps [asm]

1. dt scaling (§2.2).
2. **Time attack in practice** (Solo, mode 0, 0x4dc148): only R(1), the player:
   0x45d540(R(1), keys = R(0x4dc0fc)+0x31e9 | +0x31ed, ax +0x31f1, ay +0x31f5, dt). The
   ghost is **R(2)** (0x58762a): if loaded (R(2)+0x68), the ghost player 0x4192a0(0x52c0d8,
   dt, R(2) fields …) sets its pose from the ghost record, 0x467e30(R(2), …) when it says
   so, and 0x45e940(R(2), dt) (the passive step). Then the recorder **0x4191c0(0x52c0f0, dt,
   …R(1)'s position, angles…)** writes the player's run into the new ghost; then
   `0x5875ce = 0` (R(1)+0x31ad, a one-frame event flag of the player that the recorder
   takes with R(1)+0x2e85 and +0x31a1..+0x31a9 [asm for the addresses; meaning guess]). (Ghost files: part B / 0x4136a0 / 0x4138c0.)
3. Otherwise, for k = 1..24 in order: Solo → every loaded R(k): **0x45d540(R(k), keys, ax, ay,
   dt)** with its own input fields (zero for the computer riders, joystick bits from 0x411b30
   for a joystick player, filled in 0x4116f0). Link → the local player and (host) the remote humans (type 3 in 0x508048 + k·0x24)
   get 0x45d540, the others 0x45e940(R(k), dt) (moved by the network).

#### 6. 0x5429b0, "the player has control"

Writers [asm]: 0 at the (re)start (0x41128d), 1 at "GO" (0x47de44), 1 at every running frame
(Game_Frame step 6), 0 when the local player finishes his last lap (0x47eaf1, in 0x47e3d0,
together with 0x509ba0 = 1 in Solo). Readers: 0x4116f0 (a joystick player's bits masked to
the throttle bit and analog y centred when != 1), 0x411b30, and the keyboard/joystick reader 0x431250
(0x4312f3.., called from the window code 0x4329a5, i.e. between frames). Because Game_Frame
sets it to 1 before 0x4116f0 runs, the masking in 0x4116f0 never happens during a running
frame [asm, order]; what it really gates is the window-side key reading between the end of
one frame and the start of the next [guess: effect ≈ none; the port can drop it].

#### 7. Globals of this part

| address | meaning | tag |
|---------|---------|-----|
| 0x4e980c | frame ms (u32), clamped 1..200, 0 → 30 | asm |
| 0x4e9808 | frame time f32: ms until 0x47df40, then step units (ms·0.1) | asm |
| 0x4e9810 | frame ms as int (from 0x4e9808 before scaling) | asm |
| 0x4dc150 | running mean of the raw frame ms (f32, factor 0.125) | asm, cap |
| 0x50bbc8 | timeGetTime at this frame's start (also "now" for the rules) | asm |
| 0x4e9cfc | the last timeGetTime read by 0x459fa0 | asm |
| 0x5809e0 | limiter's last time; 0x5809f8[24] frame times; 0x4e9d00 their count; 0x5809f4 fps | asm |
| 0x4dc200 | -frameratemax N (0 = no wait) | asm |
| 0x4dbfb4 | race mouse-pointer timer (ms) | asm |
| 0x4dc0ac | race has a start countdown (always 1) | asm |
| 0x4dc0b4 | countdown running | asm, cap |
| 0x4dc0a8 | countdown ms (6000 →0), then time left ms | asm, cap |
| 0x515fe0[3] | TimeSkill (Level.Spt) per difficulty | asm + data |
| 0x52c108 | the track object (sections; +0xd8 i16 checkpoint of the start section, +0xda its time) | asm |
| 0x52c10c | the track's section count | asm |
| 0x519fec | "extended time" message timer, ms (5000 at a checkpoint) | asm |
| 0x52c238 | time over (player out of time) | asm |
| 0x509ba0 | race over (Solo: player finished; Link: everybody) | asm |
| 0x4f0c54 | time of the last "hurry" voice 0x42 | asm |
| 0x5429b0 | player has control (§6) | asm |
| 0x4e982c, 0x4f0c50, 0x4e892c | Link start synchronisation | asm (not mapped) |
| 0x5db290 | lap times, 8 ints per rider record | asm |
| 0x4e4f00, 0x4e4f08, 0x580a58, 0x4dc14c→0x4f0c4c | display/D3D flags (not for the port) | asm |
| 0x4dc0f8 | the view/camera index (0 after the rules; drawing helper) | asm |

#### 8. 0x411010 the race's (re)start, the rest [asm]

Called by 0x410ee0 (end of the load), 0x4587e0 (restart event), and 0x454330/0x454680 (Link).

1. If `0x504ea0` (set to 1 by 0x412960 at the end of a race, part B): race+0xc = 1, R(1)+8 = 1,
   0x504ea0 = 0x504eac = 0, R(1)+0xb4 = 0x504ea4, 0x4f0dc8 = 0x4dbe38, 0x4e8944 = 1,
   R(1)+0x84 = +0x8c = 0, R(1)+0x124 (speed) = R(1)+0xec, 0x48edd0(R(1)+0x24), and
   R(1)+0xec = 0x48df90(R(1)+0x12c, R(1)+0xec) — putting the player's bike back as it was
   saved by 0x412960 [asm; meaning: part B].
2. 0x4b35f0(0) (srand(0) [guess: MSVC runtime]), 0x431230, 0x4dbe30 = 0x4dbe34 = 0,
   `0x4ea4c0 = 0x457760()` (local player).
3. For k = 0..24: 0x470fc0(R(k)+0x31e1); R(k)+0x88 = 1 (Solo; the player's = 0x4dc0a0) or the
   Link player table's value.
4. 0x4dc0fc = 1 (Solo) / the last local human (Link).
5. `0x4e982c = 1`, R(local)+0x31e1 = 0x4dc11c (the control device: 2 or 4 make 0x4126a0's
   held branch call 0x411a10), R(local)+0x31e5 = 0, 0x4e892c = 0, 0x5d26a1/0x5d26a5 =
   0x7f7f7f7f, bytes 0x5d25b0 = 0x5d25b8 = 0. In playback 0x42e6e0(0x53e4d0, 0, 1) (part C).
6. `0x52c238 = 0x509ba0 = 0x504ea0 = 0x504eac = 0`; Link: 0x4dc0ac = 1.
7. Best records: 0x4656c0(0x581118) reads this track's best from Data\bestscor.dat (part B);
   none → 0x581118 = 0, 0x581209/0x58120d = 0x7f7f7f7f. **The ghost buffer 0x581018**
   (0xf9 bytes): kept when time attack in practice and a ghost was loaded (0x52c0d8); else
   filled with 0x7f, and in Solo (not time attack) 0x465290(0x581018, name 0x5161b0) or,
   failing, the last record 0x465070(0x581018, 0x4654e0() − 1) from bestscor.dat [part B].
8. The countdown set-up (§4), 0x4e981c = 0, Link host 0x456fb0, 0x457320(), and
   0x463be0(0x580a78 + k·0x1a, R(k)) for k = 0..24 (part A). race+4 = 1, race+8 = 0.

#### Race rules, part A: places, laps, checkpoints, splits, wrong way, finishing

Scratch part (helper A) for build/notes/race_rules.md. Everything read in the machine code
unless marked. Addresses of riders: **obj(k) = 0x581218 + k·0x3209**, k = 0..24 (k = 0 is
the menu's bike, never loaded in a Solo race: +0x68 = 0 in every capture). In a Solo race
the player is k = 1 and the opponents k = 2.. (loop bound `k < 0x509bac`).

##### 0. The rider's race fields (offsets from obj) [asm]

| offset | size | meaning | written by |
|---|---|---|---|
| +0x04 | 4 | "p2" of 0x45bf70: 1 for the player and the computer riders (Solo); in Link 1 only for the host's own type-3 slots | 0x45bf70, 0x410c8d |
| +0x08 | 4 | "p1": 1 = the player (Solo k = 1), 0 = computer rider; Link: 1 = a player slot | 0x45bf70 |
| +0x68 | 4 | loaded / in the race | 0x45bf70, grid |
| +0x2dd4 | 8+1 | rider name (8 chars) | grid (0x410cff copies the player's name 0x5161b0) |
| +0x2ddd | 4 | **race time, ms** (counted by 0x47dae0 while state is 0 or 1) | 0x47dae0, reset 0x47bf70/0x45bdf0, ghost rider below |
| +0x2de1 | 4 | **best lap, ms**; 0x7f7f7f7f = none | 0x47e3d0, reset to 0x7f7f7f7f (0x47bf70 at 0x47cf82, 0x45bdf0) |
| +0x2de5 | 4 | **state**: 0 not yet over the start line, 1 racing, 2 finished, 3 out of the race (time over: 0x47dae0 at 0x47deec; quit from the in-race menu in a championship: 0x43791f) | 0x47e3d0 (0→1, 1→2), 0x47dae0, 0x437602, reset 0 |
| +0x2f79 | 2 (i16) | **next checkpoint number** (0..ncp−1); 1 at the start | 0x47e3d0, 0x411010 (=1) |
| +0x2f7b | 2 (i16) | **laps completed** | 0x47e3d0, 0x411010 (=0) |
| +0x2f85 | 4 | **finishing place**, 0-based (0 = winner); 7 after a reset | 0x47e3d0 at the finish, Game_Frame (Link) |
| +0x2f89 | 0x30·4 | **split times**: race time at each checkpoint passed, index 0x46d4b0 (≤ 0x30) | 0x412e50; cleared by 0x47bf70 (`rep stos` 0x30 dwords) |
| +0x30c1 | 4 | crashed (NOTES §4.15) — read by the camera choice 0x4583e0 | physics |
| +0x31b1 | 4 (float) | signed distance from the section walk (see 0x40c7e0) | 0x47e3d0 via 0x40c7e0 |
| +0x31b5 | 4 | **progress** = sections advanced since the start (negative while behind the line) | 0x47e3d0, grid/reset |
| +0x31b9 | 4 | **furthest progress** reached | 0x47e3d0, grid/reset |
| +0x31bd | 4 | section of the previous frame | 0x47e3d0 |
| +0x31c1 | 4 | **current section** | 0x47e3d0 (also written by the physics and AI walks 0x45e940, 0x46b120, ...) |
| +0x31e1.. | | control type and inputs (other helper) | |

Per-rider lap times: **0x5db290 + k·0x20**, 8 ints = the time of lap 0..7 in ms (0x47dae0
adds the step's whole ms 0x4e9810 to entry [lap] while state is 0/1). 0x5db2b0 is rider 1's.

Initial progress (0x47bf70 at 0x47cff4, and 0x45bd40 / 0x45bb30 the same) [asm]: prev
section = current; N = section count 0x52c10c, half = N/2 (C division, toward 0).
- forward: progress = sec − N if sec ≥ half, else sec;
- reverse (0x4dc144): progress = N − sec if sec ≥ half, else −sec;
- furthest = progress. So a grid placed just behind the line starts with a small negative
  progress.

##### 1. 0x47e3d0 — the per-frame lap / checkpoint / finish rules (Game_Frame, after the step)

Called once a frame from Game_Frame after the step and the effects (order there:
0x48b7a0, 0x459000, 0x457590, 0x459090, **0x47e3d0, 0x47e1e0**, then the cameras).

Locals: `shown` = 0x4119a0() (below); `anyline` = 0.

"**counts(k)**" below = `k == 0x4ea4c0` (the local player) **or** (obj+4 ≠ 0 and (obj+8 ≠ 0
or 0x4ea4c4 ≠ 0)). 0x4ea4c4 is 1 in the image and no writer by address was found, so in
Solo every rider counts (player and computer riders); the riders that do not count are
Link's remote players (obj+4 = 0, obj+8 = 1), whose race state comes from the network.

For k = 1 .. 0x509bac−1, skipping riders with obj+0x68 = 0:

1. `cp = 0, line = 0`. **Section walk**: obj+0x31c1 = 0x40c7e0(track 0x52c108, hint =
   obj+0x31c1, point = obj+0x30 (body position), out = &obj+0x31b1).
2. If the section changed (cur ≠ prev = obj+0x31bd):
   - `d = cur − prev`.
   - **Forward** (0x4dc144 = 0): if d > half, d −= N. If d < −half: d += N (wrapped
     past section 0 forwards), and **line = 1** if counts(k) and progress == furthest
     (checked before adding d).
   - **Reverse**: d = prev − cur. **line = 1** if 0 ≤ prev < half and cur > half and
     counts(k) and progress == furthest. Then d −= N if d > half; d += N if d < −half.
   - progress += d.
   - If d > 0 and k == shown: 0x4f0c58 = 0 (wrong-way voice may play again) and the
     wrong-way message (text id 0x4e986c) is removed (0x4177e0 on list 0x53ba68).
   - If d > 0 and progress > furthest: furthest = progress; for each of the d sections
     entered, s = 0x40ec30(prev + dir) stepping by dir = 0x4dc140 (±1), s wrapped into
     0..N−1:
     - if section s is a checkpoint (i16 at sec+0xd8 ≥ 0): **cp = 1** if counts(k); and
       if k == shown: **0x4dc0a8 += the int at sec+0xda** (the time bonus, ms; read as a
       32-bit int at 0x47e614), **0x519fec = 5000**, and the "time out" warning text
       (id 0x4e9858, made by 0x481af0) is removed.
     - if k == 0x508834 (the rider the camera follows): 0x486240(int at sec+0xec), the
       section's event (turn signs, NOTES §4.13).
   - **Wrong way** (d ≤ 0 and k == shown) [asm 0x47e692]: with f = (float)0x4dc140 and
     the current section's start-plane normal n = sec+0x40/0x44/0x48:
     `(f·obj[+0xd4] + obj[+0xe0])·n0 + (f·obj[+0xd8] + obj[+0xe4])·n1 + (f·obj[+0xdc] +
     obj[+0xe8])·n2 < 0.0` (float 0x4d1900 = 0.0, bits 00000000) → 0x481db0 (wrong-way
     message, 4000 ms, text id 0x4e986c) and, once until the next forward section
     (0x4f0c58), **voice 0x46 WRONGWAY** (0x404da0(0x46, 500)). x87 order: the three
     products summed as (n2-term + n0-term) + n1-term, all in 80-bit registers, one compare
     [asm]. obj+0xd4.. = third row of the bike matrix (forward axis) [guess],
     obj+0xe0.. = its translation slot [asm offsets; NOTES §4.15 calls +0xbc..+0xe8 the
     matrix + translation — so the test is really "forward axis + position·n" without the
     plane's d; the meaning of +0xe0 here is open, see open points]. Only tested on the
     frame the rider enters a section backwards (or the same-numbered one after a wrap).
   - prev = cur.
3. **Link remote riders** (k ≠ local, obj+4 = 0, obj+8 ≠ 0): line = 1 if the old network
   record's checkpoint (i16 at 0x580a9e + k·0x1a) is 0 and obj+0x2f79 is 1; cp = 1 if they
   differ; then 0x463be0 copies obj's race time, best lap, state, cp, laps, place into the
   record 0x580a92 + k·0x1a (+0x16 = 1). Out of scope (note only).
4. **Checkpoint passed** (cp and state == 1):
   - not a replay (0x4e9830 = 0): for a Link remote rider, cp field −1 around the call;
     **0x412e50(obj, k == shown)** (split time, §3); remote rider: +1 back.
   - then, if counts(k) (or k == local): obj+0x2f79 += 1, and = 0 when ≥ the checkpoint
     count **0x4e9844**. (In a replay the split is not taken; the counter still moves.)
   So +0x2f79 is the number of the next checkpoint: the line is checkpoint 0 (section 0
   on every track, `tools/track.py info`: ile01 has checkpoints at sections 0, 319, 539;
   ile04 at 0, 161, 309, 453, 616), the race starts with 1.
5. **Line crossed** (line):
   - if counts(k): state 0 → **1** (the first crossing from the grid starts the rider's
     race; no lap counted); state 1 → **laps += 1**; then
     - lap == 1 and not a replay: a dead check (0x4e90b4 vs 0x4e90b8, then `nop; jmp`
       over a text "ILLEGAL VERSION" built on the stack: a copy protection disabled in
       this build) [asm 0x47e8ce].
     - if k == 0x508834 and laps == 0x4dc074 − 1 (**final lap**): remove texts 0x4e9870,
       0x4e985c, 0x4e9860, 0x4e9874, 0x4e9878; 0x481c50 (the final-lap message, 1000 ms,
       text id 0x4e9864); **voice 0x37 FINLAP**.
   - if state == 1 (now):
     - **time-attack ghost** (obj == obj(2) = 0x58762a, loaded, laps == 0x4dc074, practice
       0x4dc094 = 0, time attack 0x4dc148): race time obj+0x2ddd = ftol(float 0x52c0e8)
       (the ghost recording's total time); if 0x4dc074 == 1 also lap-0 time 0x5db2d0 and
       best lap = it.
     - if counts(k): for i = 0 .. laps−1: if lap time [i] < best lap: best = it, and in Solo
       **0x458310(obj)** (lap record check, §4).
     - if k == shown and laps == 0x4dc074: **0x5429b0 = 0**; Solo: **0x509ba0 = 1** (the
       player has finished: read by 0x4116f0 and Game_Frame); Link: 0x456c80 sends the race
       time (message 0x6d).
     - anyline = 1.
     - **Finish** (laps == 0x4dc074): 0x519fe4 = 0x4dc100 (the number of loaded riders,
       0x4817a0; the physics picks the finish animation from place vs it, 0x45dad7); if
       counts(k): **state = 2**, **0x47e1e0()** (places now), and in Solo (or obj+8 = 0)
       **obj+0x2f85 = 0x459f50(k)** (0-based place). Solo and k == shown: voice by
       place p: p == 0 → 0x458cd0(−1) (Rand_Next(1), NOTES rand) > 0x3fff ? **0x4f YEAAH**
       : **0x50 WELLDONE**; p == 0x509bac − 1 → **0x48 YURALOSR**; p > 2 → **0x47
       YOULOST**; p = 1, 2 → none. All with 0x404da0(id, 500).
       Note: places are 0-based and 0x509bac = loaded riders + 1, so p == 0x509bac − 1
       cannot happen in Solo (a single race: 0x509bac = 25, 24 riders loaded): the "you're a loser"
       voice is dead there; the last place gets YOULOST [asm + test].
6. After the loop: if anyline → **0x481800()** (who holds the fastest lap, §5).

###### 0x4119a0 — "shown" = the local player's rider [asm]
Link: 0x457050(net id, 0). Solo: the first k ≥ 1 with obj+0x68 and obj+8 set (the player,
1). None: kind 0x4e9834 == 2 (demo) → 0x508834, else −1. So the time bonus, wrong way,
split and the place voices go to the player only.

###### 0x40c7e0(track, hint, point, float *out) — section walk [asm]
(NOTES §4.13.) N ≤ 0 → 0. hint ≥ N → 0. dist(s) = 0x4b10c0 = plane +0x40 of section s
(n·p + d). If dist(hint) < 0.0 (0x4d02d0 = 0.0): step back (wrapping to N−1) until
dist(s) ≥ 0, out = that dist (≥ 0). Else step forward while dist(next) ≥ 0 (next = (s+1)
mod N), out = dist(next) of the first next that is < 0, i.e. minus the distance left to
the next section's start. Returns s.

###### 0x40ec30(s) — wrap a section number into 0..N−1 (loops of ±N) [asm]

##### 2. 0x47e1e0 — the places (sort) [asm]

Builds **0x516ab8[0..0x4dc154−1] = rider numbers in race order** (0x4dc154 = how many).
For k = 0..24 with obj+0x68, a key (low 27 bits) | k<<27:
- state 2 (finished): `(~racetime) & 0x7ffffff` → straight into 0x516ab8 (group A);
- state 0 or 3: `racetime & 0x7ffffff` → group C;
- otherwise (racing): t = obj+0x31b1 (float); forward: ftol(t + 1024.0) (double 0x4d1978,
  bits 0000000000009040), reverse: ftol(−t); clamp 0..0xfff; key = t | progress<<12
  (progress not masked: a negative progress fills the high bits, the compare masks them
  to 27 bits) → group B.
- Each group sorted by MSVC's qsort 0x4b3100 with **0x457730 = descending** on the low 27
  bits (b > a → 1, equal 0, else −1). Then A, B, C concatenated and every entry >> 27.
So: finishers by race time (fastest first), then racers by (progress, distance into the
section), then riders not yet over the line / out of time (more race time first).
**Ties** (equal keys, e.g. riders still on the grid, all race time 0 after the reset at
GO) keep whatever order MSVC's qsort leaves: a port must use the same qsort algorithm to
get the same places [asm; consequence].

###### 0x459f50(k) — place of rider k: its index in 0x516ab8, −1 if absent [asm]

##### 3. 0x412e50(obj, isShown) — split time at a checkpoint [asm]

Does nothing while 0x504ea0 ≠ 0 (the race's end phase, set by 0x412960/0x412a50, cleared
by 0x411010).
- i = **0x46d4b0(obj)**: cp = obj+0x2f79, L = obj+0x2f7b; cp == 0 → (L+1)·ncp − 1, else
  L·ncp − 1 + cp (= consecutive numbering of checkpoint passes; cp is the one being passed).
  If i ≤ 0x30: **obj+0x2f89[i] = race time** obj+0x2ddd.
- Only if **0x4e984c** (1 in Solo, 0 in Link: set by 0x4100a0 "initRace") and isShown:
  - 0x5dbc70[0] = 0 (empty text).
  - Link: the reference is the leader (0x516ab8[0]) when the player's place ≥ 1: its name
    → 0x581018 (8 chars, 0x581020 = 0) and its 0x30 splits → 0x581025; else 0x581018 =
    0x7f (none).
  - Solo: the reference is what 0x581018 already holds: the time-attack ghost / the
    track's record splits loaded at the start (0x411010 fills 0x581018.. with 0x7f7f7f7f,
    0x3e dwords + 1 byte, unless time-attack practice with a ghost 0x52c0d8, then loads
    it with 0x465290/0x465070 in Solo) [asm for the fill; the loaders are the other
    helpers'].
  - If 0x581018 ≠ 0x7f and ref = 0x581025[i] is neither 0 nor 0x7f7f7f7f: diff = split −
    ref (0x4f0e00), shown as |diff| through 0x481730 (ms → min 0x4f0d88 = (ms/1000/60)%60,
    sec 0x4f0d8c = (ms/1000)%60, hundredths 0x4f0d90 = (ms%1000)/10) with format
    `" %02d:%02d;%02d"` (0x4dbed0, diff 0), `"-%02d:%02d;%02d"` (0x4dbec0, ahead) or
    `"+%02d:%02d;%02d"` (0x4dbeb0, behind) into 0x5dbc70, cut at 10 chars (0x5dbc7a = 0).
    The HUD draws it (other helper).

##### 4. 0x458310(obj) — lap record during the race [asm]

Solo, obj+8 (the player), best lap b valid (≠ 0x7f7f7f7f, ≠ 0) and
(track record 0x58120d valid and b < it and (b < race best 0x5d26a5 or none)) or
(no track record and (b < race best or none)):
0x482dc0(kind 0x4e9834, obj, 0, 0, 0) (the record message), **0x5d26a5 = b**, name →
0x5d25b0 (8 chars, 0x5d25b8 = 0), **voice 0x3b NEWRECOR**.
0x581118.. is the track's record read by 0x4656c0 in 0x411010 (best race time 0x581209,
best lap 0x58120d, both 0x7f7f7f7f and name "" when there is none); 0x5d26a1 / 0x5d26a5
are this race's best, reset to 0x7f7f7f7f by 0x411010 [asm for the resets; the record
file is bestscor.dat, other helper].

##### 5. 0x481800 — fastest lap holder [asm]
Over k = 0..24 with (k == local or (obj+0x68 and (obj+8 or 0x4ea4c4))) and best lap ≠
0x7f7f7f7f: key = best & 0x7ffffff | k<<27, MSVC qsort ascending (0x4817d0, unsigned) when
more than one; **0x4f0db8 = first >> 27** (the rider with the fastest lap; the HUD's).
With no entry it reads an uninitialised stack word (called only after a line crossing,
so the crosser normally has one).

##### 6. The start: countdown, lights, voices, when control is given

**0x411010 (race object, start/restart of a race)** [asm 0x411010..0x4113bb]: among much
else (rider reset, Link sync, replay set-up, the reference splits above), if 0x4dc0ac
(countdown wanted; 1 in Link, set in Solo by 0x410120 [other helper]):
**0x5429b0 = 0, 0x4dc0b4 = 1 (countdown on), 0x4dc0a8 = 6000 ms**, and riders 1..
0x509bac−1: next checkpoint (+0x2f79) = 1, laps (+0x2f7b) = 0; the clocks 0x51d29c /
0x50bbc8 = timeGetTime, 0x4e9818 = 0. Without countdown: 0x4dc0b4 = 0, 0x5429b0 = 1.
Also: 0x52c238 = 0 (time over), 0x509ba0 = 0 (player finished), 0x504ea0 = 0 (end
phase), 0x504eac = 0, 0x4e982c = 1, 0x4e892c = 0, race best 0x5d26a1/0x5d26a5 =
0x7f7f7f7f, record name 0x5d25b0/0x5d25b8 = "".

The countdown is counted down by 0x47dae0 (parent's part: 0x4dc0a8 −= ms per frame; at
< 0 the countdown ends, 0x4dc0b4 = 0, the time limit is set and 0x5429b0 = 1, riders
already past the line (section in [0, N/2) forward) get state 1).

**What is shown and heard** — from the HUD, 0x413410 (called by the drawing 0x4804d0 /
0x4806e0, i.e. once per drawn picture) [asm]:
- While 0x4dc0b4: **0x413030(race, &0x4dbe44)**: 0x4dbe44 = 500; s = (0x4dc0a8 / 1000)
  % 60 (0x481730). **Lights** 0x482220(n) with n = 2 if s < 2, 1 if s < 4, else 0 (text
  id 0x4e9868; the light picture's meaning is the drawing helper's). Solo: 0x482830(1,..).
  **Number** c = s/2 + 1 (6000..4000 ms → 3, 3999..2000 → 2, 1999..0 → 1); when c ≠
  0x542854: 0x542854 = c, text "%02d" (0x4dbf0c) → 0x508430 shown 2000 ms (0x4188b0 id
  0x4e98d0, font 0x42), **0x4082f0(c)** = BIP1 + voice "3"/"2"/"1" (0x30/0x31/0x32, NOTES
  §4.10.6.4, once per change), and if c > 1 "GET READY" (0x4dbf00) in the game-message
  font 0x4f0d78 (colours 0xf5,0xff,0xc / 200,200,0), id 0x4e98e8. obj+0x14 of the race =
  s. So: 3 at 6.0 s, 2 at 4.0 s, 1 at 2.0 s, GO at 0.
- After it (0x413180, 0x4dc0b4 = 0) the first picture with 0x4dbe44 > 0: when s/2 ≠
  0x542854, "GO" (0x4dbf14) for 500 ms (the 0x4dbe44 value) id 0x4e98e8 and **0x4082f0(0)
  = BIP2 + voice GO (0x33)**; then 0x4dbe44 = 0.
  **s here is not the time left.** 0x4f0d8c is "the last clock split's seconds": the drawing
  splits the time left (0x4804e6, 0x4806f6), then the lap panel 0x483860, called just before
  this test, splits each lap time it shows into the same variable (0x483af9). So on the first
  picture s is the lap's clock, 0, and 0 ≠ the countdown's last 1: GO always comes. Testing the
  time left instead loses GO when the limit's seconds % 60 are 2 or 3, and GET READY (life 0,
  same text id) then blinks for the whole race. Of all the limits only West Way on easy does
  that (63 s, either way round); the port did until 2026-10-31, Danik found it. The original
  on West Way, easy (0x4dc060 = 0, TIME 63): GO on the screen, GET READY gone, and memtool
  after it 0x542854 = 0, 0x4f0d8c = 2 with 60.7 s left (so the lap's clock, not the time left).
  Tests had never run on easy: `tools/shots.sh` race_go, race_after_go (`OMR_DIFFICULTY`).
- Because these run in the drawing, the countdown's texts and voices follow pictures, not
  steps (a 1-picture lag at most) [consequence].

**Control**: the bike step uses 0x4dc0b4 / 0x4dc0a8 directly (0x45d6c4.., 0x45d88b: the
physics helper's); 0x4116f0 zeroes the computer riders' keys while 0x5429b0 ≠ 1
(`&= 1`, analog 0x8000) [asm 0x411922..0x411933]. Game_Frame sets 0x5429b0 = 1 at the start
of every frame that runs the race (0x47d768) — so the "≠ 1" case only lasts within a frame
after 0x47e3d0 cleared it for the player's finish, or during the countdown frames before
0x47dae0 ends it [asm; the frame order is the parent's].

**Checkpoint feedback** (0x413180, HUD, per picture) [asm]: while 0x519fec > 0 (set to
5000 at a time bonus; Game_Frame subtracts the frame's ms) and the followed rider's
(0x508834) next-checkpoint number differs from race+4, and it is racing (state 1) and not
the very start (forward: cp ≠ 1 or laps ≠ 0; reverse: cp ≠ 1 or laps ≠ 0x4dc074 − 1):
race+4 = cp, **voice 6 CHCKPVOX** (the checkpoint chime), texts 0x4e9874/0x4e9878
removed; practice without time attack: texts 0x4e985c/0x4e9860 removed and 0x481f10 (lap
time message); otherwise text 0x4e9870 removed and **voice 0x34 CKCKPNT** ("checkpoint").

##### 7. The other per-frame calls (what they are; not rules)

- **0x457590(dt)** [asm]: for the 128 effect records at 0x52c498 + i·0x104 (active flag
  +0xfc): 0x4b0730(dt) — dust/smoke sprites growing and fading (quads at +0xc0..+0xec).
  The physics or drawing helper's.
- **0x459000 / 0x459090(1)** [asm]: an rdtsc profiler, only when 0x4dc12c (debug); 0x459090
  returns cycles / (0x4e9cec · 100) → 0x5429cc (time of the frame's middle part, for a
  debug display). Not a rule; the port drops it.
- **0x48b7a0(0x5f6778, dt)** [asm]: if +4 == 0, for i < [+0]: 0x48b370(dt) on record +8 +
  i·0x190 — the animated decor (AnimDecor block of the .trk, NOTES §4.13). Not a rule.
- **0x467b30(obj, 1)** per loaded rider when **0x4dc138** (set by the camera code
  0x46d750/0x46ea6c/0x46eb78/0x46ec96, cleared 0x46ea48: a camera close enough to show
  skeletons [guess]) [asm/decomp]: the winner's (place 0, state 2) celebration timer
  0x5dabd0 −= 0x581014 while 0x504ea0; the skeleton pose (0x45f160 unless crashed +0x30c1
  or +0x319d, which force LOD 1) and with arg 1 0x45d810(obj, carac PersoAnim). The
  drawing helper's.
- **0x4583e0(dt)** [decomp]: the camera mode of camera 0x5161c0 + 0x4dc0f8·0x238 following
  0x508834 — rules only as inputs: mode 7 during the countdown (+0x34 = 0) and **mode 7
  with +0x34 = 1 once the followed rider has finished (state 2)**, mode 6 when 0x4dc00c
  and the rider is not crashed, mode 3 when 0x4dc014, else 7 or 1 (0x4dc020); in replays,
  demos, the end phase 0x504ea0 or 0x4e8968/0x4dc134, mode 1 and 0x46e9f0 (when
  0x51a40c > 0). Cameras: other helper.
- **0x458620(dt)** [decomp]: camera update 0x4700e0; during the countdown a camera value
  at +0x3c (0x5161fc) moves back to 0 at dt·double 0x4d0e00 / 0x4d0e08. Other helper.

##### 8. Globals (this part)

| address | meaning |
|---|---|
| 0x4dc0a8 | ms: the countdown (6000 → 0) while 0x4dc0b4, then the **time left**; += a checkpoint section's int +0xda when the player reaches it on new ground |
| 0x4dc0b4 | countdown running |
| 0x4dc0ac | a countdown is wanted at the start (0x411010) |
| 0x4dc140 / 0x4dc144 | direction ±1 / reverse |
| 0x4dc074 | laps of the race |
| 0x4dc100 | number of loaded riders (0x4817a0; 23 in the captures) |
| 0x4dc154 | number of entries in 0x516ab8 (23 in the captures) |
| 0x516ab8 | race order: rider numbers, place 0 first (0x47e1e0) |
| 0x509bac | 1 + riders created (loop bound; places can't reach it − 1 in Solo) |
| 0x509ba0 | the player has finished (Solo) |
| 0x5429b0 | 0 while the countdown runs / after the player's finish within a frame; computer riders' keys masked when ≠ 1 |
| 0x519fec | ms left to show the checkpoint feedback (5000 at a bonus) |
| 0x519fe4 | 0x4dc100 copied at a finish (finish animation by place) |
| 0x4f0c58 | wrong-way voice already played (until a forward section) |
| 0x4f0db8 | rider holding the fastest lap |
| 0x4e9844 | checkpoints on the track |
| 0x4e984c | split times shown (Solo) |
| 0x4ea4c0 | local player's rider; 0x4ea4c4 = 1 (count every rider; no writer found) |
| 0x508834 | the rider the camera/HUD follows (1 in Solo, 2 in a demo by name, the player in Link) |
| 0x52c108 / 0x52c10c | track sections pointer / count (captures: 0x288 and 0x29c) |
| 0x52c0d8 / 0x52c0e8 | time-attack ghost object / its recorded total time (float) |
| 0x581018.. | reference splits: name 8 chars @0x581018 (0x7f = none), 0x30 splits @0x581025 |
| 0x581118.. | the track's record: name, best time 0x581209, best lap 0x58120d |
| 0x5d26a1 / 0x5d26a5, 0x5d25b0 | this race's best time / best lap, the record holder's name |
| 0x5db290 + k·0x20 | rider k's 8 lap times |
| 0x5dbc70 | split text (10 chars) |
| 0x504ea0 | race end phase (0x412a50); no splits taken |
| 0x542854 | last countdown number shown |
| 0x4dbe44 | "GO still to show" (500 during the countdown) |
| text ids | 0x4e9858 time-out warning (0x481af0), 0x4e9864 final lap (0x481c50), 0x4e9868 lights (0x482220), 0x4e986c wrong way (0x481db0), 0x4e9870 lap message (0x481f10), 0x4e985c/0x4e9860/0x4e9874/0x4e9878 other HUD texts, 0x4e98d0 countdown number, 0x4e98e8 GET READY / GO |

##### 9. Checked [test]
- Captures build/trackcap (the attract demo, 3 laps, tracks 1 and 4; see below for a single race): 0x4dc0b4 = 1 with
  0x4dc0a8 5434, 2264, 4079, 908 (all < 6000) during the countdown; afterwards 0x4dc0b4 =
  0 and 0x4dc0a8 = 7 493..37 876 (time left); 0x4dc100 = 0x4dc154 = 23 (the demo: records 1..23, all
  computer); 0x52c10c = 648 / 668 sections; obj(0)+0x68 = 0 (slot 0 not raced).
- A Solo single race measured (build/nbcap1, 2026-10-26): 0x509bac = 25, 0x542874 = 23,
  0x4dc100 = 24, records 1..24 loaded (+0x68 = 1), 1 the human, the HUD "24/24". 0x410a50:
  the human at record 1 except in a demo (0x4e9834 = 2), then 23 opponents from the next
  record; 0x4dc100 is 0x4817a0's count of +0x68 over the 25 records.
- Checkpoint layout from tools/track.py (checkpoint 0 at section 0 on ile01 and ile04).

##### 10. Open points
- The wrong-way vector: obj+0xe0..+0xe8 is called the matrix's translation in NOTES §4.15;
  adding a position to an axis and dotting with the plane normal without its d is odd —
  check what the step writes at +0xe0 (a velocity?) when the physics is mapped.
- 0x4ea4c4 has no writer by address (maybe written through an index from 0x4ea4c0);
  read it in a capture to be sure it is 1 during a race.
- The 0x48 voice (last place) cannot trigger in Solo by the arithmetic; confirm with a
  race finished last (should say YOULOST).
- Whether the player's grid slot is behind the line (then the first crossing also adds
  section 0's time to 0x4dc0a8, on top of the time set at GO by 0x47dae0) — needs the
  grid (0x410a50) and a capture of 0x4dc0a8 just before/after the first crossing.
- Splits in Solo when no ghost/record exists: 0x581018 filled with 0x7f.. → no split text.

##### 11. Size of a C port (this part)
0x47e3d0 ≈ 180 lines, 0x47e1e0 + an MSVC-compatible qsort ≈ 60 + 90, 0x412e50 +
0x46d4b0 + 0x481730 ≈ 60, 0x458310 + 0x481800 ≈ 50, the countdown/GO/checkpoint HUD
triggers (0x413030/0x413180's rule parts) ≈ 70, section walk (land.c may already have
0x40c7e0) ≈ 30. **About 500 lines.**

#### Race rules, part B: the end of the race and the way back to the menu

(Scratch part for build/notes/race_rules.md. Read from build/mr.asm unless marked. Tags:
[asm] read in the machine code, [decomp] Ghidra's text only, [guess].)

Rider objects: obj(k) = 0x581218 + k·0x3209 (obj(1) = 0x584421 = the Solo player, obj(2) =
0x58762a). Fields used here [asm, meaning from the code that uses them]:

| obj+ | what |
|------|------|
| +0x8 | 1 = driven by a human (0 = AI; the winner's lap sets it 0) |
| +0x68 | rider present (loaded) |
| +0xb4 | top speed (float; ×0.8 during the winner's lap) |
| +0x2dd4 | rider name (string; the record's +0 in bestscor) |
| +0x2ddd | race time, ms (int; +dt each frame in 0x47dae0) |
| +0x2de1 | best lap, ms (int; min of the lap times, set in 0x47e3d0 at 0x47eaae) |
| +0x2de5 | state: 0/1 racing (1 = after the start [guess 0 = on the grid]), 2 finished, 3 out (time over, quit, abandon) |
| +0x2f79 / +0x2f7b | i16 / i16, +0x2f7b = laps done (compared with 0x4dc074 at 0x47eae2) |
| +0x2f85 | place, 0-based (0 = first), written when the rider finishes (Link: Game_Frame tail) |
| +0x2f89 | 0xc0 bytes (48 ints) copied into score/ghost records at +0xd [guess: lap/checkpoint times] |
| +0x31e1.. | input block (control type +0x31e1; 0x470fc0 clears +8..+0x20 of it: keys 0, analog 0x8000) |

The running order: 0x459f50(i) [asm] = the index of rider i in the order array 0x516ab8
(0x4dc154 entries), i.e. the current place 0-based, or −1. (Who fills the array: 0x47e3d0 /
0x47e1e0, the rules part A.)

**Port** (2026-10-26): race.c `race_end` (0x412a50 without the winner's lap), `events`
(0x4587e0's race bits), `champ_advance` (0x4580f0; pages 16/17/18/24 fall back to 9 with a
message until written); dialog.c `dialog_results` / `dialog_continue` and the pause menu's
arg 0; scores.c `scores_write_race` (0x464680), `scores_write_rows` (0x464b70),
`best_lap_write` (0x4658b0), written whole into the player's folder. Not yet: the
replay (YES acts as NO), the winner's lap, the podium data, the ghost, page 9's names.

##### 1. Game_Frame's tail (0x47d905..0x47da5d) [asm]

After the drawing-independent rules, in this order:
1. **Link only** (0x4dbffc == 0): the all-finished test. Walk obj(1).. (stride 0x3209, to
   0x5cf4f9): a rider with +0x68 != 0 and state +0x2de5 ∈ {0,1} means "still racing" → stop.
   If none: 0x509ba0 = 1, 0x47e1e0 again, then for every network player i (obj(i)+8 != 0,
   i = 1.. until 0x5d247e): obj(i)+0x2f85 = 0x459f50(i); for the local one (i ==
   0x457050(net+0x400, 0)) and once (race+8 == 0, then race+8 = 1) a voice by 0x404da0(id,
   500): place 0 → 0x458cd0(−1) = random stream 1 (0x4b1160(1)) ≤ 0x3fff ? 0x50 WellDone :
   0x4f yeaah; place − 0x509bac == −1 (last) → 0x48 yuralosr; place ≥ 3 → 0x47 youlost;
   places 1, 2 → nothing. (Ghidra drops the −1 argument of 0x458cd0.) Out of scope; noted.
2. `r = 0x412a50()` (the end-of-race machine, §2). If r != 0:
   - **position commentary** 0x407420 (§6) if kind 0x4e9834 == 0 and 0x509ba0 == 0 (race
     not over) and 0x52c238 == 0 (not time over) and mode 0x4dc094 != 0 (not practice) and
     the local player's state obj(0x4ea4c0)+0x2de5 == 1;
   - the drawing: race vtable +0x20 (0x47ff90)(dt 0x4e9808, −2).
   If r == 0 (only the Link paths that changed page) nothing is drawn.
3. Always: 0x5809f4 = 0x459fd0() (limiter/fps), 0x4e8918 = 0, 0x4e891c = 0.

##### 2. 0x412a50, the end-of-race machine (returns 1 = draw, 0 = race gone) [asm]

Globals: **0x509ba0** race over (Solo: set at 0x47eb04 in 0x47e3d0 when the local player's
laps +0x2f7b == 0x4dc074; Link: Game_Frame tail); **0x542a20** race ended by the network
server (Link, 0x4556bd); **0x52c238** time over (0x47deac); **0x4e9824** finish timer,
**0x4e9828** time-over timer (both −1 after the race reset 0x47bf70 at 0x47c0a9; ms, counted
down by the int frame time 0x4e980c); **0x504ea0** winner's lap on, **0x504eac** winner's
sequence started; **0x4f15f4** the "generic" text sequence running (0x48aca0 sets it,
0x48ae40/0x48b070 clear it). All of these are cleared by 0x410120 (race state, load) and
0x411010 (start / restart) [asm 0x4101fc.., 0x4111b7..].

Order:
1. Replay playback (0x4e9830 != 0) → return 1.
2. **Race over** (0x509ba0 or 0x542a20):
   - 0x4e9824 == −1 (first frame): 0x4e9824 = 7000; voice **0x3e racecmpl** queued
     (0x404da0(0x3e, 500)). If 0x504ea0 == 0 and 0x412960() != 0 (the winner's lap, below):
     0x4e9824 = **14000**. Return 1.
   - 0x4e9824 > 1: 0x4e9824 −= 0x4e980c; if negative → 1. Return 1.
   - 0x4e9824 == 1:
     - winner's lap on and not started (0x504ea0 && !0x504eac): race+0xc = 0;
       0x48aca0(rand() > 0xfff) (C rand 0x4b3600, so 1 with 7/8 chance: stored in 0x4f1634,
       0x4f1638 = −1, 0x4f15f4 = 1, **CD track 12** 0x404990(12, 0) — the victory music;
       the sequence itself is drawn by 0x48acd0 from 0x47ff90, drawing helper); 0x504eac = 1;
       0x4dc00c = 0.
     - winner's lap on and the sequence over (0x4f15f4 == 0): give the bike back — race+0xc
       = 1, obj(1)+8 = 1, 0x504ea0 = 0x504eac = 0, top speed obj(1)+0xb4 = 0x504ea4 (saved),
       0x4f0dc8 = 0x4dbe38 (saved), 0x4e8944 = 1, obj(1)+0x84 = obj(1)+0x8c = 0,
       obj(1)+0x124 = obj(1)+0xec, 0x48edd0 on obj(1)+0x24, obj(1)+0xec = 0x48df90(obj(1)+0x12c,
       old +0xec) (float in ST0, stored with fstp) → **0x4e9824 = 0**. Return 1.
     - winner's lap on, sequence still running → wait (return 1).
     - no winner's lap → 0x4e9824 = 0. Return 1.
   - 0x4e9824 == 0, **the results** (runs once: every dialog below sets +0x10, which stops
     Game_Frame before 0x412a50 from the next frame):
     - (replay → return 1); kind != 1 → **0x452090** (podium data, §5).
     - mode 2 championship: Solo → **0x437b70** (results dialog, §4.2). Link → 0x4644e0
       (Link points), race vtable +0xc (unload), host (0x4dbff8) → 0x4580f0 else page 24;
       return 0.
     - mode 1 single race: Solo → 0x437b70. Link → 0x4644e0, unload, page 24; return 0.
     - mode 0 practice: if time attack 0x4dc148 and obj(1) place 0x5873a6 == 0 → **save the
       ghost** (§7); then **0x464680** (hi-score table, §8) and **0x4358b0** (the
       "[CONTINUE]" dialog, §4.3).
     - Then (all modes, Solo, kind 0): if the race's best-lap record 0x5d25b0 has a lap
       (+0xf5 = 0x5d26a5 not 0x7f7f7f7f and not 0): +9 = 'TINI' (0x494e4954), **0x4658b0(0x5d25b0)**
       saves it as the track's best lap (§8), 0x5d26a5 = 0x7f7f7f7f. Return 1.
3. **Time over** (0x52c238): 0x4e9828 == −1 → 3000, return 1; > 0 → count down, when
   negative → 0, return 1; == 0 → Solo and not replay → **0x4358b0** (§4.3). Return 1.
4. Otherwise return 1.

**0x412960, the winner's lap** [asm]: kind 0x4e9834 != 0 → 0x504ea0 = 0, return 0. Already
on → 1. Only Solo single race (0x4dbffc != 0, 0x4dc094 == 1) and the local player's place
obj(0x4ea4c0)+0x2f85 ≤ 0 (won): 0x504ea4 = obj(1)+0xb4 (top speed), obj(1)+0xb4 ×= double
0x4d03a8 = 0.8 (bits 9a9999999999e93f; fld float × qword, fstp float), 0x4dbe38 = 0x4f0dc8,
obj(1)+8 = 0 (the AI drives the player's bike), obj(1)+0xec = obj(1)+0x1a4 = obj(1)+0x124,
0x504ea0 = 1, 0x470fc0 on obj(0x4dc0fc)+0x31e1 (inputs released), obj(1)+0x2dc8 (0x5871e9)
= 0, 0x4f0dc8 = 0x4e8944 = 0x4dc00c = 0; return 1. So a single-race win gets 14 s of the
AI riding a lap at 80 % top speed with the victory sequence, before the results.
**Escape** during it (0x431250, §3): 0x504eac = 1, 0x48b070(1) if 0x4f15f4 (stop the
sequence), 0x4e9824 = 1 → the bike is handed back and the results come next frame.

##### 3. Keys during the race: 0x431250 (WM_KEYDOWN, from the race's message handler 0x4329a5) [asm]

0x4329a5 first gives a key to the open dialogs, in the order 0x4e8908, 0x4e8910, 0x4e890c,
0x4e8904 (each 0x433ec0 = the dialog's key/mouse handler; a dialog whose +0x20 (done) is set
is closed there: 0x4354e0 + free). Only then 0x431250. If the menu is active (menu+0 != 0)
0x431250 hands the key to the menu (0x40a0f0). The seven control keys (table 0x53eae0, 8-byte
records, compared with the key) are handled first when kind == 0 (inputs helper). Then a
switch on the virtual key (byte table 0x432668 indexed by vk−9, jump table 0x4325c4):

| key | what (rules part) |
|-----|-------------------|
| **Escape 0x1b** | if not paused (0x4dc038 == 0): winner's lap on → skip it (§2); else Solo → **0x436420(1)** the pause menu (§4.1); Link → 0x43b700(0x4326e0) (Link menu, texts 165–169) |
| **Pause 0x13**, **Ctrl+P / Ctrl+Pause** | Solo only: 0x4dc038 ^= 1; 0x4096e0(paused) pauses/resumes the CD (0x4b1660 on 0x503528 if CD music on 0x4db66c); 0x409700(paused) → 0x424090 pauses/resumes every sound. While 0x4dc038 is set **App_OnIdle 0x415960 does not call the race frame at all** (0x41599a), so the picture freezes; the clock is not stopped, so the first frame after has the clamped dt 200 ms (0x4120e0). 'P' without Ctrl does nothing here. |
| Shift 0x10 / Ctrl 0x11 | 0x4dc10c = 1 / 0x4dc108 = 1 (held flags; cleared by CONTINUE and key-up) |
| F1 0x70 | 0x43cf90 the help overlay (HELP.GEN) in slot 0x4e890c, pausing sound/CD |
| F11 0x7a / F12 0x7b | sound menu 0x43a400 / display menu 0x438f70 in slot 0x4e8908 (toggles; F12 also 0x43c2b0 message box) |
| F2 0x71, F3 0x72, F4 0x73 | screen size / display mode changes (0x474cf0, 0x4dc0c8/0x4dc0cc/0x4e9ce4) — not rules |
| F5 0x74, F6 0x75, F7..F10, Tab, Insert, A D F G H J K L S T U V, numpad * + - / | cameras, HUD and debug toggles (0x46d750 camera, 0x4847f0, 0x4188b0 texts...) — drawing helper / debug, not mapped here |

##### 4. The in-race dialogs (slot 0x4e8904; event word 0x4e8918)

###### 4.0 The dialog object (0x78 bytes, ctor 0x4338b0) and the four slots [asm/decomp]
Slots: **0x4e8904** results / pause / continue (this file), events **0x4e8918**;
**0x4e8908** sound menu F11 / display menu F12; **0x4e8910** message boxes 0x43c570 (the
video-memory warnings Game_Frame makes from 0x4e4f08 / 0x580a58; the display-mode list whose
items write their +0x3c into **0x4e891c**, handled by race slot +0x1c **0x47ec50**: a mode
index (low word) with a flag in the high word → 0x4dc0d0/0x4dc0cc/0x4dc0c8, 0x4e9ce4 = 1
display re-init — not rules); **0x4e890c** help F1, events **0x4e8914** (no writer found
besides 0x4587e0's reset: unused in practice).
Fields (dword index ×4): +0 kind (4 results, 6 pause, ...), +0xc 1, **+0x10 = 1 stops the
race** (0x4126a0 returns 0: no step, the drawing gets +0x18 as its 2nd argument), +0x14,
+0x18 = −10 (draw argument), **+0x20 done** (the item handler sets it; 0x4126a0 /
0x4329a5 then close and free it: 0x4356d0 for 0x4e8904, 0x435620 0x4e890c, 0x435770
0x4e8908, 0x435810 0x4e8910 which also puts the arrow cursor back), +0x24 shown, +0x28
active (0x434960 runs each frame: gives each row a hotkey letter not used by the control
keys/F-keys, table 0x53eb20 built by 0x43e0f0 each frame, then 0x434860 layout), +0x30
optional "close?" test, +0x38 optional close callback, +0x3c 1, +0x48/+0x4c item arrays
(+0x50 count), +0x58 rows (0x6c bytes each, +0x5c count), +0x64 creation time 0x50bbc8.
Items 0x415dd0 (0xc4 bytes, text 0x416300(item, text, font), colours 0x4163e0). Each row has
two items (normal / highlighted: colours 0x4f0de8 / 0x4f0dec) and callbacks: +0x80 0x4361c0
(highlight), +0x84 0x436150 (move: plays 0x29 select), +0x88 0x436140 (enabled: 1), +0x8c
the action. The engine (0x433xxx–0x435xxx: keys, mouse, layout, drawing) is not mapped here.
Texts: text[id] = *(0x53b5e8 + 4·id) (NOTES §4.8.4.1).

###### 4.1 Pause menu 0x436420(arg) (Escape: arg 1; after a single race's results: arg 0) [asm]
0x424090(1) pauses the sounds, plays effect 3 (continue) 0x4049f0(3, 0), replaces any dialog
in 0x4e8904; kind 6, +0x10 = 1, +0xc = 1, +0x18 = −10, +0x3c = 1, +0x14 = (arg != 1).
Title "MOTO RACER" (0x4e8aec) font 9. Rows (font 0x20):
1. replay playback (0x4e9830) → **[NEXT DEMO]** (107) → 0x437680; else **[RESTART]** (104) → 0x437610.
2. playback → **[QUIT DEMO]** (106) → 0x4376d0; else **[QUIT RACE]** (105) → 0x437890.
3. only arg 1: kind 0 → **[CONTINUE RACE]** (110); kind != 0 → playback ? [CONTINUE DEMO]
   (108) : [CONTINUE REPLAY] (109) → 0x437ae0.
4. only arg 1 and ((kind 1, not playback, mode 2: watching a championship race's replay) or
   (Link host)): Link host → [ABANDON] (113) → 0x437b40; else **[QUIT REPLAY]** (111) → 0x437760.

Actions (each sets +0x20 done first; 0x424090(0) resumes the sounds where shown) [asm]:
- **RESTART 0x437610**: 0x424090(0), 0x424020; kind 1 → 0x463ca0 (results back); Solo →
  **0x464680** (the finished human riders go into the hi-score table, §8); events |= 8|1|2
  (kind 0, race reset vtable +8, 0x411010 restart).
- **QUIT RACE 0x437890**: events = 0; kind 1 → 0x463ca0; Solo → 0x464680; championship →
  the local player's state (0x457760) = 3; events |= 8|4 and Solo 0x200 (page 9 / champ
  0x4580f0) or Link 0x1000 (page 7); then draws picture id 0xb1 at once (a waiting screen,
  0x43e8e0/0x43ede0, through the D3D/soft calls 0x499ba0 640x480, 0x49a0e0/0x49a100(1912.0)).
- **CONTINUE 0x437ae0**: 0x470fc0 on obj(0x4dc0fc)+0x31e1 (inputs released), 0x4e8964 =
  0x4dc10c = 0x4dc108 = 0, 0x424090(0).
- **NEXT DEMO 0x437680**: events |= 0x100000 | 0x20000.
- **QUIT DEMO 0x4376d0**: if 0x4dbf80 → 0x471e00 only; else 0x49a120(1.0), events |=
  4|0x8000|0x2000 (unload, stop the demo script, page 14), race vtable +0 (defaults 0x47bf30).
- **QUIT REPLAY 0x437760** (championship only, not playback; else does nothing): Link host
  message 0x6c; kind 1 → 0x463ca0; Solo → 0x464680; 0x4ea4c0 = 0x457760(); events |= 8;
  local state 2 and place +0x2f85 ≤ 2 → (Link 0x4644e0) events |= 4|0x200; otherwise
  events |= 0x40 (the continue dialog).
- **ABANDON 0x437b40** (Link): events = 0x1000000 (local state 3 unless finished).

0x457760 = the local player [asm]: Solo → the first k ≥ 1 with obj(k)+8 != 0 (k <
0x509bac), else the value of 0x508834; Link → network.

###### 4.2 Results dialog 0x437b70 (Solo single race and championship, at 0x4e9824 == 0) [asm+decomp]
0x424020 (stop sounds), effect 3; replaces 0x4e8904; kind != 1 → **0x463c30** saves every
rider's results (obj(1..23): +0x2ddd, +0x2de1, +0x2de5, +0x2f79, +0x2f7b, +0x2f85; 0x1a
bytes each at 0x5d26ca, +0x16 = 1) so that a replay can be watched and the results put back
(0x463ca0). Kind 4, +0x10 = 1, +0x14 = 1, +0xc = 1, +0x18 = −10, +0x3c = 1.
Title **[REPLAY]** (100) font 9; rows **[YES]** (101) → 0x438650, **[NO]** (102) →
0x438690, and **[SAVE]** (114) → 0x438680 only with the switch `-saverecord` (0x4e8924), through a file dialog (not ported: the game has no way to play the file back).
- **YES 0x438650**: events = 0x13 (bit 0x10 kind = 1: watch the replay, 1 race reset, 2
  restart) — the race replays from the recorded inputs.
- **SAVE 0x438680**: 0x4579b0(0) writes the `.Sav` replay (dialog stays).
- **NO 0x438690**: events = 0; 0x463ca0 (results back); Solo → **0x464680**; then
  - championship: 0x4ea4c0 = 0x457760(); place = state 2 ? +0x2f85 : 0x459f50(player);
    place > 2 → events |= **0x40** (continue dialog: try again); else (Link 0x4644e0)
    events |= 8|4|**0x200** (unload → 0x4580f0);
  - single race: events |= 8|**0x80** → the pause menu with arg 0 ([RESTART] / [QUIT RACE]).
  (Practice never gets here.)

###### 4.3 The continue dialog 0x4358b0 (practice end, time over, championship not in the top 3) [asm]
0x424020, **voice 0x44 tryagain** (0x4049f0(0x44, 0)), replaces 0x4e8904; +0x10 = 1, +0x14 =
1, +0xc = 1, +0x18 = −10, +0x3c = 1. Title **[CONTINUE]** (99) font 9; rows **[YES]** →
0x436210, **[NO]** → 0x436250.
- **YES 0x436210**: events = 9 (Link client) or **0xb** (Solo, or Link host): kind 0, race
  reset (vtable +8) and 0x411010 — the same race again.
- **NO 0x436250**: events = 8, Solo: single race → 0x40c (unload, **page 6**), practice or
  championship → 0x20c (unload, **page 9**, champ via 0x4580f0). Then the waiting picture
  0xb1 as QUIT RACE.

##### 5. Podium data 0x452090 (results, kind != 1) [asm+decomp]
Name strings 0x53f7e8 (3 × 0x20) all set to text[183] "Computer" (0x53b8c4). Count 0x4e9120
= Solo 0x509bac − 1 (Link: players), at most 3. For place p = 0..count−1: the rider (obj(1..
0x509bac−1), +0x68 != 0) whose +0x2f85 == p: human (+8) → name copied from +0x2dd4; race
time +0x2ddd → **0x540998**[p], skin +0x5fc → **0x540c90**[p]. **0x4e911c** = place of
0x4119a0() (the local player) + 1, at most 4. Unfilled 0x540c90 entries (from the number
found to 3) = rand() & 7. Page 24 (podium, start 0x452240) reads these; it plays voice 0x5b
yutrygan and music 11 when the player is not in the top 3, else music 12 (NOTES §4.10).

##### 6. Position commentary 0x407420 (each drawn frame, conditions in §1) [asm]
Voices on (0x4db668), not paused, at most every **60000 ms** (0x54299c + 0xea60 < now
0x50bbc8; 0x54299c = now), only if 0x4ea4c0 == 0x508834. place/count = finished (state 2)
? (+0x2f85, 0x519fe4) : (0x459f50(i), 0x4dc100). Only if count > 1:
- place 0: id = table 0x4db988[r & 1] = {0x41 stillrun, 0x40 running4}, r = 0x4b1160(4);
  appended to the voice queue 0x5028c8 with delay 500 (ring index 0x5028d8, full when
  (index+1)&7 == 0x5028d4; 0x5028cc = 0 makes it busy).
- count − place == 1 (last): if 0x4b1160(4) is even and the last voice 0x502898 != 0x49:
  0x502898 = 0x49 and **0x49 yurdalst** played at once (0x423fd0(handle 0x51a11c, 4),
  volume 0x4db654).
- (count > 2 and count − place == 2) or (count > 3 and count − place ∈ {2,3}): id = table
  0x4db990[r % 3] = {0x38 fuel, 0x3c onegear, 0x3f runlast}, r = 0x4b1160(4), queued with 500.
- 0x50353c = place, 0x5028b8 = count (also when nothing was said).
(Note the countdown is reset only when the 60 s have passed, even if nothing is said.)

##### 7. The ghost (time attack, practice) [asm]
At the end of a practice race with 0x4dc148 and obj(1)'s place 0x5873a6 == 0 [the ghost is
rider 2 and is in the order, so the run "counts" when the player beat it: guess, the order is
part A's]: float 0x52c100 (recorder +0x10) = (float) obj(1)+0x2ddd (fild, fstp); 0x4190d0
copies the live recorder **0x52c0f0** into **0x52c0d8** (+0 count, +8, +0xc samples of 0x34
bytes copied, +0x10 time, +4 = +0x14 = 0); 0xc0 bytes obj(1)+0x2f89 → 0x581025; strcpy
0x581018 = "SHADOW" (0x4dbea8); 0x581018 → 0x581118 (0xf9 bytes); **0x4138c0(0x52c0d8,
0x581018)**: file `<0x4e8380 ""><track dir>timatak` + (Reverse ? "R" : "_") + (Pocket ?
nothing : "_") + laps `%1d` + `.bin`, mode "wb". **Bug kept by the loader 0x4136a0 too**: with
Pocket the "P" is appended to the wrong buffer 0x53f3c0, so the name just lacks a letter.
Contents: u32 pocket 0x4dc078, u32 reverse 0x4dc068, u32 laps 0x4dc074, the 0xf9-byte
record, u32 rec+0 (count), u32 rec+8, u32 rec+0x10 (time, float bits), count × 0x34-byte
samples. No time comparison: the file is overwritten whenever the condition holds.

##### 8. bestscor.dat writes [asm]
File `Data\bestscor.dat` (0x4ea590), "r+b", else created "w+b"; u32 magic 0x312e3030 "00.1"
(a different first word → the file is recreated). Blocks of 0x20-byte key + one 0xf9 best-lap
record + 8 × 0xf9 race records; the key as the port's scores_key() (track name + "POC" +
"REV" + practice "%s%s%d" "TRA" laps). Missing block → appended (key, best lap 0x7f with
name +9 = 0 and +0xf5 = 0x7f7f7f7f, 8 records of 0x7f).

**0x464680, the race table** (practice end; RESTART, QUIT RACE, QUIT REPLAY, results NO in
Solo): 25 records in a frame array (0x7f), the file's 8 read into 0..7 (fseek −0x7c8 back to
them). For every rider i = 1..**25** (0x19 inclusive: the last is past the 24 objects, reads
0x5cf4f9.. [edge]) with state +0x2de5 == 2 and +8 != 0 (finished humans): record **slot 6**
(always the same slot, so the last one wins; Solo has one): +0 strcpy name +0x2dd4, +9 =
'TINI' (page 9's "new row" marker), +0xd = 0xc0 bytes from +0x2f89, +0xcd = 0, +0xf1 = race
time +0x2ddd, +0xf5 = best lap +0x2de1. Then exact duplicates removed (0xf9-byte compare,
later ones shifted out, slot 24 refilled 0x7f), qsort 25 by +0xf1 (0x4644a0: a − b, so
0x7f7f7f7f sorts last), slots 6..24 refilled 0x7f, **8 written back** (rows 6, 7 always
0x7f: the table keeps 6), the first 6 copied to 0x5cf500 (page 9's rows), fclose.
**The best lap**: during the race 0x458310(obj) (from 0x47e3d0 when a rider's best lap
improves, Solo, humans only): new lap L = +0x2de1 (valid, != 0) counts if it beats the
stored best lap 0x58120d (the track's record, loaded into 0x581118 by 0x4656c0 at each start
0x411010; 0x7f7f7f7f if none) and this race's 0x5d26a5 (or that is empty) → 0x482dc0(kind,
obj, 0, 0, 0) (the "new record" HUD [guess]), 0x5d26a5 = L, strncpy(0x5d25b0, name, 8),
0x5d25b8 = 0, **voice 0x3b newrecor** (0x404da0(0x3b, 500)). At the race end (§2) the record
gets 'TINI' and **0x4658b0** writes it as the block's best lap (same file logic; seek
−0x8c1 = 9 records back, write the record and the 8 read rows unchanged). Page 9 then finds
the 'TINI' markers, asks for the 3-letter name and saves again (NOTES §4.12.5).

##### 9. 0x4587e0, the event handler (from 0x4126a0 for 0x4e8914 / 0x4e8918, from 0x4116f0
and Menu_Frame) [asm]
Bits on *p, in this order:
| bit | action |
|-----|--------|
| 0x800000 | |= 0x1000 \| 4 (Link: unload, page 7) |
| 0x1000000 (else) | race exists: 0x4ea4c0 = 0x4119a0(); its state = 3 unless 2 (abandon) |
| 0x200000 / else 0x400000 | 0x438f70(0) display menu / 0x43a400(0) sound menu |
| 0x100000 / else 0x8000 | demo script 0x53e4d0: next item 0x42e550 / close it 0x4246e0, 0x4e8944 = 1, replay 0x4e9830 = 0, kind 0x4e9834 = 0 |
| 0x10 / else 0x20 / else 0x8 | kind 0x4e9834 = 1 (watch the replay) / 2 / 0 |
| 0x1 | race vtable +8 (0x47bf70, the race reset) |
| 0x2 | 0x411010 (start / restart: riders back on the grid, timers, the race's records) |
| 0x4 | race vtable +0xc (0x47d620 unload); "page changed" |
| 0x40 / else 0x80 | 0x4358b0 continue dialog / 0x436420(0) pause menu after a race |
| 0x100 | page 5 |
| 0x200 | championship (mode 2) → 0x4dc07c = 0x4580f0(); else page 9 |
| 0x400 | page 6 |
| 0x1000 | page 7 |
| 0x800 | page 1 (reload: restart from the loading page) |
| 0x2000 | page 14, menu+0x24 = timeGetTime |
| 0x4000 | page 8 (does not count as a page change) |
| 0x8000000 | championship → 0x4dc07c = 0x4580f0(); else page 24 |
| 0x20000 | replay playback: next demo from the script (0x42e580 name/time/picture; 0x457bf0 sets the race from it; 0x542995 = its time; 0x542891 = its picture name, 0x104 chars): another track → unload + page 1; same track → reset + 0x411010; no more → unload, kind = replay = 0, page 14 |
| 0x10000 | not playback: 0x4580f0(); goes on → busy cursor, race vtable +4 load(track, kind) (fail → page 5), arrow cursor; over → page 5 |
Then *p = **0xc0000** if a page change / unload happened (0x40000 = the race does not step,
0x80000 = and is not drawn this frame), else 0. Pages via 0x409eb0(menu 0x4dc0c4, page); the
last call wins.

##### 10. 0x4580f0, championship advance (no points in Solo) [asm]
Page 9 if Solo and obj(1)'s state 0x587206 != 2, else page **24** (podium) (Link: always 24).
Link client (!Solo, !host) → return 1. **Solo: only if obj(1)'s place 0x5873a6 ≤ 2 (top 3)**:
race r = 0x509ba8 of the table 0x508934 (0x50 bytes a race): +0x2c != 0 → 0x4dc03c (Pocket
unlocked) = it, page **17**; +0x28 != 0 → 0x4dc040 (Reverse unlocked), page **16**; the track
named at +0x30 (strcmp with each track record's +4, 0x51d298 tracks) gets +0x128 = 1
(unlocked); 0x509ba8 += 1. (Not top 3: nothing, the same race comes again.) Then 0x509ba8 ≥
0x5083b4 (races) → Solo championship → page **18** (championship end), return 0; else return
1. Link: same unlocks without the place test. The caller stores the result in 0x4dc07c
(page 9's Continue: 0 → page 5, 1 → page 6). Link points: 0x4644e0 (not mapped).

##### 11. Paths, summed up (Solo)
- **Practice**: last lap done → 0x509ba0 → "race completed" voice, 7 s → [ghost if TA and
  beaten] → table → best lap → "[CONTINUE]" (try again voice): YES = same race again
  (0xb), NO = page 9 TOP 5.
- **Single race**: finish → 7 s (14 s + victory lap and CD 12 if won; Escape skips) →
  "[REPLAY] YES / NO / SAVE": YES watches the replay (kind 1; its Escape menu has CONTINUE
  REPLAY), NO → table → [RESTART] / [QUIT RACE]: RESTART = again (table written again: the
  duplicate test drops the same record), QUIT RACE = page 9 (capture
  build/ref/orig_after_race_top5.png).
- **Championship**: as single race, NO → top 3: page 24 podium (or 17/16 unlocks, 18 at the
  end), else "[CONTINUE]": YES again, NO page 9 (TOP 5) with the same race next.
- **Time over** (0x47dae0 sets 0x52c238, voice 0x39 gameover, state 3): 3 s →
  "[CONTINUE]": YES again; NO: single race → page 6, others → page 9.
- **Escape**: pause menu RESTART / QUIT RACE / CONTINUE RACE (sounds paused, race stopped
  by +0x10).

##### Open points
- The dialog engine (keys/mouse/layout/drawing, 0x433ec0, 0x434860, 0x4354e0, 0x435090...)
  is only outlined; which keys move and press (arrows, Return, the row hotkeys from
  0x434960, mouse 0x200..0x205) needs reading before the port's dialogs.
- obj+0x2f89 (0xc0 bytes) and record +0xd: lap or checkpoint times (part A).
- Whether the ghost rider is counted in the order 0x516ab8 (decides when the ghost is saved).
- race+0xc: **the dashboard on**: 0x47ff90 draws 0x4806e0 (the HUD and the text lists) only while it is set; 0 during the winner's credits. [asm; px: the original's capture]

##### Size of a C port (this part)
End machine 0x412a50 + winner's lap ~150 lines; event handler and page hand-over ~150;
the three dialogs' contents and actions ~200; bestscor writes (table + best lap) ~150; ghost
save ~60; championship advance + podium data ~100; position commentary ~60; race keys
(Escape/Pause/F1/F11/F12 only) ~60. About **900 lines**, plus the dialog engine (not mapped,
roughly 600–900 more) unless the port draws these menus with its own small widget.

#### Race rules, part C: race sounds per frame, replays and demos (mapped 2026-10-26)

Scope: 0x4079e0 (engines, Doppler, rider shouts), 0x405f30 (ambience + voice queue),
0x4116f0 (race vtable +0x10: inputs, replay record/playback, demo clock), 0x411b30
(joystick → rider inputs), the `.Sav` file (0x4579b0 / 0x457bf0), the demo list, 0x4121e0
(demo captions: dead in this build), 0x411a10 (joystick → menu keys). Sound basics are in
NOTES §4.10 and not repeated here.

Tags: [asm] read in the machine code, [decomp] Ghidra's text only, [guess].
Rider object: obj(k) = 0x581218 + k·0x3209 (k = 1 is 0x584421, the player in Solo; the bike
number is obj+0x78 = 0x581290 + k·0x3209). Offsets below are from obj(k).
Constants read with `tools/peek.py` (bits in brackets).

---------------------------------------------------------------------------------------------

##### 1. 0x4079e0 — engine sounds of the bikes near the camera [asm]

Called by Game_Frame only when the engine switch **0x4db65c** is on (after the cameras
0x4583e0/0x458620 and 0x467b30, before 0x405f30). Does nothing unless effects **0x4db664**
are on and the race is not paused (**0x4dc038** = the pause flag, toggled by VK_PAUSE in the
key handler 0x431250 at 0x4314f7, Solo only; while it is set App_OnIdle does not call the
race at all, 0x41599a).

Globals:
| addr | what |
|------|------|
| 0x4dc0f8 | current camera index; camera records at 0x516258 + cam·0x238 (+0 position x,y,z; +0xc the point looked at) |
| 0x5032b0/b4/b8 | the camera position of the previous call (float ×3), stored at the end of every call (and at the start when 0x4dc138 is set — the camera was cut, so no Doppler jump) |
| 0x508834 | the **followed rider** (whose engine is "the own one"): 1 at the grid, **2 in a demo** (0x410d3d), cycled by a key once the local rider is out (0x432029) |
| 0x5028bc | the followed rider of the last call |
| 0x5034b0 | engine-sound object (0x78 bytes) of the followed bike |
| 0x5032c0 + i·0x78, i = 0..3 | engine-sound objects of the 4 nearest other bikes |
| 0x504210 + i·0xc | the 4 slots for them: +0 used, +4 rider index, +8 distance |
| obj+0x3049 | the rider's distance to the camera (float), written here |
| obj+0x68 | rider present (only riders with it set are heard) |
| obj+0x30 / obj+0x3c | position now / position of the previous step (body sub-object +0xc / +0x18) |
| 0x509bac | rider slots + 1 (riders are 1..0x509bac−1) |

Order:
1. If 0x4dc138 (camera cut): previous camera position = current one.
2. For every present rider 1..0x509bac−1: obj+0x3049 = |obj+0x30 − camera position|
   (float sums in the order dx, dy, dz stored as floats, then sqrt of dy²+dz²+dx²... the
   asm sums y², then z², then x² with fsqrt; write it as the asm 0x407a64..0x407acb).
3. If 0x508834 changed since the last call: stop the own engine object (0x406190 stops its 9
   voice handles +0x10..+0x30) and reset its fields (handles −1, states 0); remember it.
4. **Own bike**: pitch factor f = 1.0, **except in a replay or demo (0x4e9834 != 0)** where
   the Doppler factor below is used (in a race the camera rides with the bike).
   Volume = ftol((10240.0 − obj+0x3049) · 0.033203125) clamped to 0..255.
   `0x406290(own object, obj, volume, pan 0, own = 1, f)`.
   own = 1 lets 0x406290 play the start sounds 0x1d..0x1f (§4.10.6.3).
5. If the followed rider is the local player (0x4ea4c0) and not a replay/demo: **0x407600**
   (rider shouts, §2).
6. Other bikes: list (1, index, distance) for every present rider except the followed one;
   `qsort` by distance (0x4079a0, ascending); take the leading ones closer than **10240.0**
   (bits 0x46200000 compared as an int), **at most 4**.
7. Slot matching: clear all 4 `used`; a listed rider already in a slot keeps it (distance
   updated, marked used, its list entry consumed). Unused slots: index = 0 and their sound
   object stopped (0x406190). Each remaining listed rider takes the first free slot from a
   running index; that slot's sound object is reset (handles −1, states 0).
8. Camera forward on the ground: (look-at − position).xz normalised → (fz, fx) =
   ([esp+0x50], [esp+0x54]) = (dz/L, dx/L).
9. For each used slot (in slot order): Doppler f, volume as in 4, pan:
   - p = ndx·fz − ndz·fx with (ndx, ndz) = (bike − camera).xz normalised (the sine of the
     angle off the camera's axis);
   - dead zone: p ≥ 0.15 → p − 0.15; p ≤ −0.15 → p + 0.15; else 0 (the test is on the float
     bits: below 0xbe19999a unsigned, i.e. positive or above −0.15, gives 0);
   - pan = ftol(p · 75.29412) (= 64/0.85, so ±64).
   `0x406290(slot object, obj, volume, pan, own = 0, f)`.
10. Previous camera position = current.

**Doppler** (0x407b99..0x407d1b and 0x408075..0x4081c7) [asm]:
k = 1.0 / (0x4e9808 · 0.01) — 0x4e9808 is by now the step in 10 ms units (0x47df40 scaled
it by 0.1), so k turns a per-step move into units per second.
vc = (camera − previous camera)·k, vb = (obj+0x30 − obj+0x3c)·k, u = (camera − bike)
normalised (left as is if its length is 0.0).
f = (1 + (u·vc)·C1) · (1 + (u·vb)·C2), C1 = −1/46080, C2 = +1/46080.
The sums are formed y, z, then x (u·vc = uy·vcy + uz·vcz + ux·vcx); copy the asm order.

| constant | value | bits |
|----------|-------|------|
| 0x4d0268 double | 0.01 | 3f847ae147ae147b |
| 0x4d0150 double | 1.0 | 3ff0000000000000 |
| 0x4d0134 float | 0.0 (length test) | 00000000 |
| 0x4d0270 double | −2.170138888888889e−05 (−1/46080) | bef6c16c16c16c17 |
| 0x4d0278 double | +2.170138888888889e−05 | 3ef6c16c16c16c17 |
| 0x4d0280 float | 10240.0 (hearing range) | 46200000 |
| 0x4d0284 float | 0.033203125 (= 340/10240) | 3d080000 |
| 0x4d012c float | 0.15 (pan dead zone) | 3e19999a |
| 0x4d0138 float | 75.29412 | 42969697 |

So a bike is at full volume (255) within 2560 units and silent beyond 10240; only the 4
nearest other bikes are heard, plus the followed one.

##### 2. 0x407600(rider) — the rider's shouts (local player only) [asm]

Clock: 0x50bbc8 (ms, the race's `now`). Random: Rand_Next 0x4b1160(4) (stream 4).
Voices go through the sound server directly (0x423fd0 play, 0x424030 volume), with the voice
switch 0x4db668 and "last voice" 0x502898 / "last effect" 0x502710 rules of 0x4049f0
(§4.10.3.5, flag 4: not the same id twice in a row).
Bike kind: carac row +0x120 (0x509398 + bike·0x124) != 0 → dirt bike (the same test picks
the KTM engine table in 0x406290).

a. **Drifting** — obj+0x2da0 (set by the bike step 0x45d540 when on the ground,
   obj+0x2db8, and |0x48d0f0(body)| ≥ 0.04 (double 0x4d0f40) [asm]; a slide angle [guess]):
   held continuously for more than **3500 ms** (0x503534 start, 0x5034a8 flag) and more than
   **4000 ms** since the last shout of this kind (0x5028ac, pulled back to now if it is in
   the future): once per episode (0x503548), **1 in 3** (Rand_Next(4) % 3 == 0) voice
   **0x4e "yeees"** (not through 0x4049f0: no repeat test; 0x502898 = 0x4e).
b. **Dirt bikes, airborne** — obj+0x2db8 == 0 (not touching the ground) for more than
   **1200 ms** (0x504560, flag 0x5034a0) and more than 4000 ms since the last (0x503540):
   once per jump (0x5028b4), 1 in 3: a random one of the four ids at 0x4db978 =
   **0x4c yeehaaa, 0x4a opopadow, 0x51 wouaou, 0x3a lechef** (Rand_Next(4) & 3), played with
   flags 4 (all four are voices; an id outside 0x2f..0x5c would go the effect way).
c. **Road bikes, flat out** — speed obj+0x124 / carac+0x48 (RealSpeedMax) > **0.93** (float
   0x4d0260, bits 3f6e147b) for more than **15000 ms** (0x503558, flag 0x5034a4): voice
   **0x45 "watarokt"** put in the voice queue ring (0x5028c8: ids +0x24, delays +0x44,
   delay 500) unless the ring is full, **once per race** (0x50352c, cleared by the race
   sound set-up 0x4084e0 and the switches 0x408900/0x408b30).

##### 3. 0x405f30 — ambience and the queued voices, every race frame [asm]

1. If effects 0x4db664, ambience 0x4db660 and not paused 0x4dc038:
   - **0x405bc0(0x503568)**: the track's ambience set (from Sound.Spt, §4.10.6.2) placed in
     the world. Set layout [decomp]: +0 count (at most 40), entries of 0x10 at +4: +0 kind,
     +4 object, +8 section-kind index, +0xc decor object; sound objects (0x40 bytes,
     §4.10.6.2) at +0x284 + i·0x40; +0xc6c section numbers by kind; +0xc84 seven section
     numbers found near the camera by 0x40d3b0(track 0x52c108, out, range 0x28).
     Kind 0 = anchored to a decor object (its position transformed by the object's angles
     through the cosine table 0x5fcb80; only if visible within 0x3c sections, 0x40d350,
     and only when the detail level 0x4dc098 > 0); kind 1 = at the position of a track
     section (0x52c108 + n·0xfc, +0 x,y,z); kind 2 = not positional. Each then goes to
     0x405030 (the object's state machine, NOTES §4.10.6.2) with (audible, x, y, z).
   - **Detail level 0x4dc098** (0..4, F-key in the race) went to 0: every kind-0 entry's
     sound is stopped (handle −1, +0x3c = 1). Went from 0 to above 0: kind-0 entries are
     re-armed (0x404ee0). 0x503550 = the level seen last.
2. Always (sound off or paused too): the 8 queued-sound records 0x504240..0x504560 and the
   race's **voice queue 0x5028c8** are updated (0x404b30, §4.10.3.6). This is where every
   0x404da0 voice of the race (countdown, lap, finish, time-out) is actually played.

##### 4. 0x4116f0 — race vtable +0x10 (via 0x47ec40): inputs, replay, demo clock [asm]

Called by Game_Frame after 0x4121e0; if it returns 0 the frame does no step and no drawing
of its own (the race is ending). Game_Frame then sets **0x4e9810 = ftol(0x4e9808)** (ms,
truncated) — see the main notes.

Globals:
| addr | what |
|------|------|
| 0x4e9830 | **playback** (1 while a demo or a `.Sav` replay runs; set by 0x409a30, cleared by events 0x8000 / end of the demo list) |
| 0x4e9834 | race kind: 0 a normal race (recording), 1 a `.Sav` replay, 2 a demo by track name (events 0x8/0x10/0x20 set 0/1/2) |
| 0x4e9814 | replay/demo clock, ms (int): + dt each recorded frame, + dt each played frame; reset by 0x4577c0/0x4577e0 |
| 0x4e9838 | frames recorded (or loaded); 0x4e983c playback index |
| 0x558aa8 | the replay buffer: 0x4fb0 = 20400 records of 8 bytes (to 0x580828) |
| 0x542995 | header +0x135: the demo's length in ms (from the .dem; −1 in a saved replay) |
| race object +0x10 | 0 running; **2 = end the playback now** (0x4121e0 sets it when the demo time is up; 0x410120 clears it); 1 = end it when the caption ends (writer not found) |
| 0x4dc0fc | the local input slot (1 in Solo; 0x411116/0x411138) |
| obj+0x31e9 | key bits held from the keyboard (WM_KEYDOWN/UP in 0x431250, only when 0x4e9834 == 0) |
| obj+0x31ed | key bits from a joystick, rebuilt every frame |
| obj+0x31f1 / +0x31f5 | analog steering / throttle 0..0xffff, 0x8000 = centre |
| obj+0x31e1 / +0x31e5 | control kind (2 or 4 = a joystick) / controller number |
| obj+0x31f9, +0x31fd, +0x3201 | a held button (+0x31f9), its previous state (+0x31fd), and a one-shot press (+0x3201, cleared by the bike routine 0x45d810 at 0x45e372 after copying it to +0x3205); the key handler sets +0x31f9 and +0x3201 together (0x43144f); its meaning is the physics helper's (a put-back/reset request [guess]) |

Order:
1. Local slot: +0x31ed = 0, +0x31f1 = +0x31f5 = 0x8000.
2. Kind 1, or playback with kind 2 — the playback part:
   - It ends (go to 3) when playback and any of: (0x509ba0 "all finished" or 0x542a20 (a Link
     "race over" message, 0x4556bd)) and the end timer 0x4e9824 == 0; or time over
     0x52c238 and its timer 0x4e9828 == 0; or kind 1 and index ≥ count; or race +0x10 == 2.
     (0x4e9824/0x4e9828 are the end-of-race delays of 0x412a50, −1 → 3000/7000/14000 ms:
     a replay plays on through the finish sequence and stops when it is over.)
   - Kind 1: **0x4578a0(&0x4e980c, &keys +0x31e9, &+0x31f1, &+0x31f5)** reads the next
     record (fails at the end → 3). Then the clocks follow the replay: 0x50bbc8 = 0x4e9814,
     0x459fb0 sets the `now` 0x4e9cfc to it, **0x4e9808 = (float)recorded dt**, and the
     smoothed frame time 0x4dc150 += (dt − it)·0.125 (double 0x4d0380). So a replay runs on
     its recorded steps whatever the real frame time.
   - Kind 2 (demo by track name): 0x4e9814 = ftol(0x4e9814 + 0x4e9808) — the real frame
     time in ms; nobody's inputs are read (the bikes drive themselves; the camera follows
     rider 2).
3. Playback over: if 0x4e9830: **0x4e8918 = 0x120000** (0x100000 = advance the demo list
   0x42e550, 0x20000 = load the next entry), **0x4587e0(&0x4e8918)**, draw (vtable +0x20 with
   0x4e9808 and −1) unless the result has 0x80000; return 0. (Kind 1 without playback, not
   reachable: message 0x53b7b8 for 5000 ms and kind = 0.)
4. Kind 0 (a normal race) only:
   - for riders 1..0x509bac−1: kind 2 or 4 → +0x31ed = **0x411b30(i)** (joystick bits),
     +0x31f1 = joystick X 0x51bcf8 + n·0x38, +0x31f5 = joystick Y 0x51bcf4 + n·0x38
     (n = +0x31e5); **before the race is running (0x5429b0 != 1): +0x31f5 = 0x8000 and only
     bit 1 (throttle) kept**. Other kinds: 0, 0x8000, 0x8000.
   - **0x4577f0(0x4e980c, +0x31e9 | +0x31ed, +0x31f1, +0x31f5)** records the local slot.
5. Return 1.

###### 4.1 The replay record (0x4577f0 record, 0x4578a0 play) [asm]
Two dwords a frame at 0x558aa8 + i·8:
- word 0: bits 24..31 = **dt in ms** (0x4e980c, min(dt, 255)); bits 0..23 = the key bits,
  with **bit 7 = obj+0x31f9 and bit 6 = obj+0x3201** of the local slot written in.
- word 1: (analog steering +0x31f1 << 16) | analog +0x31f5 (not masked).
Recording stops silently when 20400 frames are full (5 min 40 s at 60 frames a second; at
the original's variable rate a long race can outrun it, and the replay then ends early).
Playback: dt = w0 >> 24, keys = w0 & 0xffffff, X = w1 >> 16, Y = w1 & 0xffff; 0x4e9814 +=
dt; index + 1; +0x31f9 = (keys & 0x80) != 0; keys & 0x40 → +0x3201 = 1. Past the end: keys
= 0, returns 0.
**Only the local rider's inputs and the frame times are stored**: every other rider, the
random streams and the physics must replay the same way from the same start (0x411010 at
race start/restart: kind 0 → 0x4577c0 clears count, index, clock; else 0x4577e0 rewinds
index and clock). For the port: the race must be exact step for step for `.Sav` files to
play, and a port at fixed 1/60 s steps would record dt = 16/17 ms values like the
original's limiter does.

###### 4.2 The .Sav file [asm 0x4579b0, asm+decomp 0x457bf0]
Header = the race parameter block 0x542860 (0x139 bytes; page 1 copies the menu choices into
it at load, NOTES §4.12.7):
| off | what |
|-----|------|
| +0x00 | 0x139 (header size; checked) |
| +0x04 | 0x400 (version; checked) |
| +0x08 | track index |
| +0x0c | laps |
| +0x10 | mode |
| +0x14 | rider count (set at page 1; not rewritten by the save) |
| +0x18 / +0x1c / +0x20 | reverse / pocket / transmission |
| +0x24 | bike of the local player (obj+0x78 of 0x4ea4c0) |
| +0x28 | name, 8 chars, +0x30 = 0 |
| +0x31 | loading picture path (0x104 chars, +0x134 = 0); empty in a saved file |
| +0x135 | demo length ms (−1 in a saved file) |
Then int32 count, then count × 8 bytes of records. Written with `wb` (0x4dbf40).

- **Save 0x4579b0(name)**: the in-race menu's "save replay" button (0x438680 → name 0)
  opens a Windows file dialog (`*.Sav`, "Record Files (*.Sav)|*.Sav|All Files (*.*)|*.*||",
  the current folder saved and restored around it). Fills the header from the menu globals
  (track 0x4dc08c, laps 0x4dc074, mode 0x4dc094, reverse 0x4dc068, pocket 0x4dc078,
  transmission 0x4dc0a0, bike, name 0x5161b0), writes header, count, records; message
  0x53b7c0 (language table) for 2000 ms.
- **Load 0x457bf0(name)**: open (0x42ee60, `dsi_fopen` with 0x4db238), read 0x139 bytes;
  needs +4 == 0x400 and +0 == 0x139, else message 0x53b7bc "REPLAY COMPATIBILITY ERROR"
  style (only if the menu is not active) and a log line "Version of replay file %s is
  incompatible...". In playback (the only way it is reached): kind 0x4e9834 = 1, the menu
  globals and the name taken from the header, mode forced to 1; count read (< 0x4fb0) and
  the records; message 0x53b7c4 for 2000 ms.
  If the file cannot be opened or is refused, and playback: the name is compared
  (strncmp, 0x20) with the track table's names (record +4, 0x51d298 tracks): a match is a
  **demo by track name**: kind 2, mode 1, reverse 0, pocket 0, automatic, bike 0, 23
  opponents, 3 laps, 0x4ea4c0 = 1, name kept.
  The branch for "not in playback" (same track, mode 1, same reverse/pocket → laps and bike
  taken) and the file dialog (name 0) have no caller in this build (0x457bf0's two callers,
  0x409a30 and 0x4587e0, always pass a name with 0x4e9830 = 1) [asm].

###### 4.3 The demo list (title page → demos) [asm/decomp]
`Data\Demo\EA.dem` (BMG.dem in the GP edition), loaded at start-up into the script object
0x53e4d0 (0x42e370): `NbItems: 2`, `Demo1: WEST_WAY 90000 Data\\menu\\bitmap\\pagdem.lez`,
`Demo2: SNOW_RIDE 90000 Data\\menu\\bitmap\\logomr.lez` (GP: logomrGP.lez). Each entry
(0x42e5a0, key `Demo%d`): a `.Sav` name or a track name, the length in ms, the loading
picture. +0x11a current index, +0x11e count (+0x122 / 0x53e5f2 = has entries).
- **0x409a30** (page 14 after 30 s, NOTES §4.4.2.3 step 6): 0x4e9830 = 1, 0x4e8944 = 0,
  reads the current entry, 0x457bf0(name); on success 0x542995 = length, 0x542891 = picture,
  0x4dbffc = 1 (Solo), tears the menu down and goes to page 1 (the loading screen shows the
  entry's picture). On failure: list closed, kinds 0, 0x4e8944 = 1, "Error in demo script"
  box only with the `-d` switch.
- Both GOG entries are track names → **kind 2**: a 23-opponent single race of 3 laps, nobody
  steered from the keyboard, the camera on rider 2, real frame times, **ending when the demo
  clock 0x4e9814 passes 90000 ms** (0x4121e0 → race +0x10 = 2 → 0x4116f0 → events
  0x120000).
- 0x4587e0 with 0x100000: the index + 1, wrapping to 0 (0x42e550); with 0x20000 (and
  0x4e9830): the next entry via 0x457bf0; the same track → race restart in place (vtable +8
  and 0x411010), another track → unload (vtable +0xc) and page 1; no entry → unload, kinds
  0, playback off, page 14 (title). So the demos loop WEST_WAY, SNOW_RIDE, WEST_WAY, ...
  until a key (the menu's business).
- The in-race menu (0x436420) during playback offers two other buttons: 0x437680 (sound on,
  events 0x100000 | 0x20000: the next demo) and 0x4376d0 (events 4 | 0x8000 | 0x2000: unload,
  close the demo list 0x8000 → 0x4e8944 = 1, playback off, page 14; with `-d` 0x471e00
  instead). In a race: 0x437610 = restart (8 | 1 | 2) [asm].

##### 5. 0x4121e0 — demo captions, dead in this build [asm]

Game_Frame calls it once the dialogs let the race run; it returns 1 except while a caption
holds the race.
- Not in playback (0x4e9830 == 0 or kind 0): free the caption sprite 0x5d3a20 if it holds
  pixels (0x5d3a50), return 1.
- Playback: **if 0x542995 > 0 and 0x4e9814 > 0x542995: race +0x10 = 2** (the demo's time is
  up). The rest is a caption system: an entry 0x4e9840 (picked by 0x42e6f0 from the demo
  clock: +4 duration, +8 kind 0 picture / 1 text, +0xc file or text, +0x10 0 = the race
  waits while it shows, +0x14 clear the screen to colour +0x1c..+0x1e first, +0x18 shown)
  shown through the text list 0x53b9a8 with id 0x4e98e4, the sound paused (0x424090(1),
  0x4096e0(1)) and DIVERS\PAUSE (0x2e) looping meanwhile. **But 0x42e6f0 is a stub
  (`xor eax,eax; ret 8`), 0x4e98e4 stays 0, and 0x4179f0 with id 0 finds nothing** — so no
  caption ever shows: 0x4e9840 is set to 0 every frame and 1 is returned.
  (If a caption were showing and not blocking, the race would be drawn with the dt of the
  current replay record, (float)(record >> 24), and −1.)
- Port: only the time check. 

##### 6. 0x411a10 — the joystick drives the in-race menu [asm]

Game_Frame calls it when 0x4126a0 returned 0 (a dialog holds the race) and the controller
type 0x4dc11c is 2 or 4 (a joystick), unless the dialog 0x4e8904 is of kind 9 (the key
redefinition box [guess]). Polls the controller 0x51ae88 (0x418da0(0)); on each change of
the direction bits 0x51bd04 (bit 0 up, 1 down, 2 left, 3 right) and button bit 0 of
0x51bd00 it posts WM_KEYDOWN (0x100) / WM_KEYUP (0x101) of VK_UP 0x26, VK_DOWN 0x28,
VK_LEFT 0x25, VK_RIGHT 0x27, VK_RETURN 0xd to the window 0x52c068 (PostMessageA 0x70d9cc).
Last states 0x4dbe30 / 0x4dbe34. Port: feed SDL controller presses to the menu as these
keys.

##### 7. 0x411b30(rider) — joystick → key bits (not the AI) [decomp, structure asm-checked at the call]

For a rider whose control kind is a joystick: polls controller n = +0x31e5 (fails → 0);
maps the stick directions and each button (count 0x51b06c + n·0x194) through the player's
assignments 0x431190(code) (−4/−5 = up/down, −2/−3 = left/right, 0.. = buttons) to the
actions: 0 → bit 1 (throttle), 1 → bit 2 (brake), 2/3 → bits 4/8 (with the analog axis
mirrored or centred), 4/5 → bits 0x10/0x20 (gear up / down; only once the start is over,
0x4dc0b4 == 0, and the race runs, 0x5429b0), 6 → +0x31fd = 1 (only while 0x5429b0).
Brake cancels throttle. Controller type 0x4dc11c != 2 → both analog values 0x8000 (digital
pad); == 2: the X axis is centred when a direction is assigned to the stick (0x53eae4,
0x53eaec). +0x31fd changing to 1 sets +0x31f9 and +0x3201, to 0 clears +0x31f9. The AI's
driving is not here: AI riders get zero inputs here and are driven inside the bike step
(physics helper).

---------------------------------------------------------------------------------------------

##### Open points
- What obj+0x2da0's quantity is exactly (0x48d0f0 of the body; a slip angle [guess]) and
  what the +0x31f9/+0x3201 button is: the physics helper's.
- Who sets race +0x10 = 1 (not found; only the dead caption path reads it).
- The language strings 0x53b7b8/0x53b7bc/0x53b7c0/0x53b7c4 (replay messages) not resolved
  to text.
- Whether the AI in a kind-2 demo also drives rider 1 (0x45d540/0x45e940, physics helper).

##### Rough size of a C port
- Engines near the camera, Doppler, pan (0x4079e0) + shouts (0x407600): ~200 lines (on top of
  the engine voice 0x406290 already planned, ~250).
- Ambience placement (0x405bc0, 0x405f30) on top of the §4.10.6.2 objects: ~120 lines.
- Replay record/playback, .Sav load/save, demo list and the kind-2 flow: ~250 lines
  (the file dialog replaced by a fixed name or a switch).
- 0x4121e0: 5 lines. 0x411a10 / 0x411b30 (SDL game controller): ~120 lines.

### 4.18 The player's bike: one race step (physics and controls; mapped 2026-10-26)

Mapped 2026-10-26 from game.EXE (GOG 3.22, base 0x400000) for the port. Marks: [asm] read in
the machine code, [decomp] Ghidra's text only, [guess] inferred, [run] measured in the running
original (tools/win/memwatch.c, §9). Constants are given as the game reads them: address,
width (f = float, d = double) and value (bits in brackets where the digits are not exact).

The reference in Python is tools/bikephys.py (its routines name the game's addresses); the
checks that run it against recordings of the original are build/phys/chk_*.py (§9).

Conventions: `obj` = the bike + rider object (player 1 = 0x584421; rider k at
0x584421 + k·0x3209, k = 0..23; one more record before, 0x581218). `body` = obj+0x24, a
sub-object of the 0x48xxxx class (its fields are written body+Y = obj+0x24+Y; both are given).
`C` = the carac row 0x509278 + obj[+0x78]·0x124 (§3). `dt` = the step (§1).

---------------------------------------------------------------------------------------------

#### 1. The frame and the step [asm]

- `0x4120e0` (each race frame, from Game_Frame 0x47d660 before anything else): `ms` =
  timeGetTime − the previous frame's (0x459fa0 reads timeGetTime into 0x4e9cfc, 0x459fc0 gives
  it; the previous one kept at 0x50bbc8) → **0x4e980c** (int). Unless a replay plays
  (0x4e9834 == 1), a smoothed copy 0x4dc150 += (ms − 0x4dc150)·d 0x4d0380 = 0.125 (fild, fsub
  f, fmul d, fadd f, fstp f). Then **0x4e9808** = f32(ms) (fild of the 64-bit {ms, 0}); if
  ms > f 0x4d0398 = 200.0 → ms = 200, 0x4e9808 = 200.0; else if 0x4e9808's bits ≤ 0 (ms ≤ 0) →
  ms = 30, 0x4e9808 = 30.0. (A replay, 0x4e9834 == 1, overwrites 0x4e980c/0x4e9808 with the
  recorded ms in 0x4116f0 → 0x4578a0.) 0x4dbfb4 −= 0x4e9808 (truncated). 
- `0x47df40` (from Game_Frame after the race rules 0x47dae0): **dt = f32(0x4e9808 · f 0x4d1970
  (0.1f = 3dcccccd))**; **if dt < f 0x4d1900 = 0.0 → dt = 1.0**. (NOTES §4.2 says "at least
  1.0": the constant is 0.0, so dt is 0.1 .. 20.0; ms ≥ 1 makes it ≥ 0.1.) dt is in
  hundredths of a second. **One step a frame, no substeps.**
  - Solo (0x4dbffc) Practice (0x4dc094 == 0) with time attack (0x4dc148): only player 1 is
    stepped, 0x45d540(0x584421, keys, joyY, joyX, dt), then the ghost (0x4192a0 replays the
    best lap into rider 2, 0x58762a, and steps it with 0x45e940), then 0x4191c0 records.
  - Otherwise, riders k = 0..23 with obj+0x68 (loaded): Solo → 0x45d540(obj, obj+0x31e9 |
    obj+0x31ed, obj+0x31f1, obj+0x31f5, dt). Link (0x4dbffc == 0): the local player(s) the same,
    the remote ones 0x45e940(obj, dt).
- `0x45d540`(obj, keys, joyY, joyX, dt): global **0x581014 = dt**; obj+8 (set by 0x45bf70's
  p1: a driven bike) → **0x45d580** (this note), else **0x46bf80** (the computer riders' model:
  0x46b120 drives along the track; not mapped here).
- The frame limiter 0x459fd0 (`-FrameRateMax60`, 0x4dc200 = 60): waits until 1000/60 = **16 ms**
  (integer division) have passed since the last limiter call or since the routine was
  entered. So GOG's original runs its race at 62.5 frames a second with ms = 16, **dt = 1.6**
  every step [run: 813 frames in a row all 16 ms]. Without the switch, under wine on the
  hidden display, every frame was 33 ms (dt 3.3) [run].

Port: a fixed step of dt = 1.6f at 62.5 Hz is exactly what the original does at
`-FrameRateMax60` on a fast machine (every step the same). 1/60 s steps (dt = 1.6667) are not
an original step and change the results slightly (the step is not linear in dt). [guess]

#### 2. Controls

##### 2.1 Keyboard: the bits at obj+0x31e9 [asm 0x431250 (key down), 0x432700 (key up)]
The bindings 0x53eae0[7]{key, button} (CONFIG.BPT, NOTES §4.14; defaults Up, Down, Left,
Right, A, Z, Space). The key handlers work on the local player's object only
(0x584401 + 0x4dc0fc·0x3209 = obj+0x31e9, 0x4dc0fc = 1 in Solo). Nothing when a replay plays
(0x4e9834 != 0).

| action | key down | key up |
|---|---|---|
| 0 accelerate | 0x4e8964 = 1; bit1 cleared, **bit0** set | 0x4e8964 = 0; bit0 cleared |
| 1 brake | (race running 0x5429b0) bit0 cleared, **bit1** set | bit1 cleared; bit0 set again if 0x4e8964 (accelerate still held) |
| 2 left | (0x5429b0) bit3 cleared, **bit2** set | bit2 cleared |
| 3 right | (0x5429b0) bit2 cleared, **bit3** set | bit3 cleared |
| 4 gear up | (0x4dc0b4 == 0 and 0x5429b0) **bit4** set, 0x4dc110 = 1 | bit4 cleared |
| 5 gear down | (same) **bit5** set | bit5 cleared |
| 6 turbo / stunt | (0x5429b0) **obj+0x31f9 = 1**, obj+0x3201 = 1 | obj+0x31f9 = 0 |

Windows repeats WM_KEYDOWN for a held key (the last one pressed), which re-applies its row:
holding Right then tapping Left leaves neither bit once Left is released (Left's down cleared
bit3, its up clears bit2, and Right no longer repeats). The port should feed key-down
repeats the same way, or at least the one-press semantics above.

##### 2.2 Joystick: obj+0x31ed (bits), obj+0x31f1 (Y), obj+0x31f5 (X) [decomp 0x411b30, 0x4116f0]
Each frame (0x4116f0, race-frame input) for every rider whose input kind obj+0x31e1 is 2
(joystick) or 4 (pad): obj+0x31ed = 0x411b30(k) (the same bit meanings, from buttons and the
axes when bound to them), obj+0x31f1 = axis Y 0x51bcf8[obj+0x31e5 · 14], obj+0x31f5 = axis X
0x51bcf4[...] (0..0xffff, centre 0x8000); before the race runs (0x5429b0 != 1) X = 0x8000 and
only bit0 is kept. Otherwise +0x31ed = 0, +0x31f1 = +0x31f5 = 0x8000. The keyboard player:
+0x31ed = 0, axes 0x8000. Then 0x4577f0 records (ms, keys|joybits, Y, X) for the replay.

##### 2.3 The race phases
0x411010 (race start): 0x4dc0b4 = 1 (countdown), 0x4dc0a8 = 6000 ms, 0x5429b0 = 0, unless
0x4dc0ac == 0 (Link: 0x4dc0ac is Solo, so a Practice time attack **has** its countdown: §4.21c)
→ 0x4dc0b4 = 0, 0x5429b0 = 1. Player 1's
obj+0x88 = 0x4dc0a0 (1 automatic, 0 manual) in Solo; every other rider 1.

#### 3. The carac table (0x509278 + bike·0x124, parser 0x42a580) [asm]

Read at load from CARACn.BPT (the BPT tokens, 73 numbers in exactly this order; tools/bikephys.py
`Carac`). All floats except +0x00..+0x10 and +0x120. Values: SPEED1 (road) / KTMCROSS1 (dirt).
"Readers" are the code that reads the field with an absolute address (0x5092xx[bike·0x124]);
code that reads through a row pointer exists too, so "parser only" means [guess] unused.

| C+ | name | SPEED1 / KTM1 | what it does (readers) |
|---|---|---|---|
| 0x00 | MotoType | 2 / 1 | 1 = dirt: grip factor from the lateral speed (§5), slide terms in 0x45d110, the dirt brake-slide, landing threshold 8 instead of 32 on straights (§7), dirt riders' pose |
| 0x04..0x10 | Gfx acc/spe/gri/bra | ints | the CHOOSE BIKE bars only (NOTES §4.12) |
| 0x14 | Frein | 100 / 80 | brake deceleration (0x48ef30 brake; at least 10) |
| 0x18 | MoteurVitesseMax (MVM) | 7 / 8 | the engine-speed scale: engine value 0..MVM; rpm = e·RegimeMax/MVM |
| 0x1c | MotoMasse | 280 / 150 | divides the drive force and the drag (0x48ef30) |
| 0x20 | RetardCadre | 16 / 8 | smoothing of the frame pitch (0x466a80) |
| 0x24 | RoueAccelerationAngulaire (RAA) | 10 / 30→6 | steering rate gain per step (0x466240). Dirt bikes are clamped at load: > 6 → 6, else > 3 → 3 |
| 0x28 | RoueMaxAccelerationAngulaire | 30 / 42 | parser only |
| 0x2c | RoueVitesseRetour (RVR) | 30 / 35 | steering return to centre (0x466240) |
| 0x30 | AngleSuspension | 0.1963 | pitch clamp (0x466a80), effects (0x466d70) |
| 0x34 | RoueLimiteAngulaire (RLA) | 600 / 980 | steering angle limit obj+0x110 (0x466240) |
| 0x38 | RoueAxeDistance | 1.75 / 1.45 | wheelbase: wheel points ±48·it (ground matrix 0x468ca0, 0x469c60) |
| 0x3c / 0x40 | RoueRayonAv / Ar | .29 .32 / .30 .30 | wheel radii: spin angles (0x466940), wheel points (0x469c60) |
| 0x44 | RoueRayon | .27 | parser only |
| 0x48 | RealSpeedMax | 80 / 46 | top speed (m/s): body+0x90 = obj+0xb4, +2.0 for a manual box (0x45bf70) |
| 0x4c | VitesseMaxBoite (VMB) | 61 / 40 | wheel speed ↔ engine (rpm = MVM/(ratio·VMB)·wheel), steering transfer |
| 0x50 | RotationTransfert | .0028 / .0026 | steering angle → yaw rate (0x466240) |
| 0x54 | DivTransfert | 32 | parser only |
| 0x58 | HauteurCadre | .58 / .55 | frame height (0x469c60, ·96) |
| 0x5c/0x60/0x64 | RessorAmplitudeDec / Min / Factor | .885 / −10 / 10 | the spring bounce (0x466a80) |
| 0x68 / 0x6c | FourcheAngleAvant / Arriere | −.65 −.55 / −1.25 −1.05 | +0x6c by the drawer 0x46031d; +0x68 parser only (both are the 3D bike's, NOTES §4.15) |
| 0x70 | NbSpeed | 6 / 5 | number of gears (stored as a float; −1.0 when 0x4dc1f4 == 0, which never happens) |
| 0x74 | RegimeMax | 12000 / 10000 | rpm scale (HUD, sound, torque curve) |
| 0x78..0x8c | RapportDeBoite1..6 | .3 .6 .9 1.2 1.34 1.45 | gear ratios (ratio[g] = C+0x74+4g) |
| 0x90..0x9c | RegimeCoupleMax, CoupleMax, Couple1000, CoupleRegimeMax | 10500 15 6 2 / 6500 7 1 1 | the torque curve 0x48ee30 |
| 0xa0 | ZoneRouge | .9 / .85 | parser only (red line drawn from it elsewhere? [guess]) |
| 0xa4 | MinAdherenceWheel | .2 / .1 | used only as "k = it if > 1.0": with the GOG data never (k = 1) |
| 0xa8..0xb0 | MinAdherenceDerap[3] | | never read |
| 0xb4..0x11c | Material* triples (Neutre, EauProfonde, Eau, Sable, Herbe, Terre, Goudron, Huile, BordRoute) | | never read: **the grip does not depend on the surface** |
| 0x120 | PersoAnim | 0 / 1 | read by 0x45d580 each step: 1 = dirt rider (the `flat` flag below, poses) |

#### 4. The driven bike's step: 0x45d580(obj, keys, joyY, joyX, dt) [asm]

In this order (the detail of each routine is in §5–§8):
1. Read C+0x120 (PersoAnim). Clear obj+0x319d, obj+0x2dac (this step's event bits) and
   obj+0x2e95 (landing kick). If dt's bits as int < 1 (dt ≤ +0) → dt = 1.0.
2. **0x465fa0** (start of step): the matrix obj+0xbc..0xe8 copied to obj+0x140..0x16c;
   obj+0x2da8 = 0x48ca10 (the longitudinal speed); positions shifted (obj+0x3c..0x44 →
   obj+0x48..0x50, obj+0x30..0x38 → obj+0x3c..0x44), likewise the wheel points (+0x1b4 → +0x1c0 →
   +0x1cc, +0x338 → +0x344 → +0x350, +0x4bc → +0x4c8 → +0x4d4); obj+0x54 = 0x581014 (dt);
   obj+0x2da4 = obj+0x2da0; obj+0x2dbc = obj+0x2db8; a negative speed obj+0x124 is set to 0.
3. obj+0x14 = 0.
4. Brake dive: if !(keys & 2) or speed ≤ +0 (bits): if obj+0x2ea5 is negative →
   obj+0x2ea5 = f32(obj+0x2ea5 + 0x581014 · d 0x4d0f38 = 0.0025), else obj+0x2ea5 = 0.
   (0x466660 makes it negative when braking.)
5. obj+0x2e39 (frame dive) moves toward 0 by d 0x4d0ed8 = 0.01 per **step** (not scaled by dt):
   > 0.01 → −0.01, < −0.01 → +0.01, else 0.
6. lat = 0x48d0f0 (lateral speed); obj+0x2da0 (sliding) = obj+0x2db8 (on the ground) and
   |lat| ≥ d 0.04.
7. Not in the countdown (0x4dc0b4 == 0): if obj+8 and obj+4, obj+0x2e85 = obj+0x31f9 (turbo
   held); then **0x4660f0**(obj, dt) (turbo/wheelie, §5 "0x4660f0").
8. Not crashed (obj+0x30c1 == 0): **0x465da0**(keys, joyY, joyX, dt) → **0x48ef30** (throttle,
   engine, brake, slope, drag; §5).
9. Not in the countdown and not crashed: **0x466240**(keys, joyY, joyX, dt) (steering and yaw, §6.5).
10. flat = PersoAnim && 0x46c7a0(obj+0x31c1, 8) == 0 (a dirt bike on a road that does not bend in
    the next 8 sections).
11. **0x48d620**(body, dt, flat) (integration, §6.3). Then if obj+0x180 (impulse |P|) is ±0:
    obj+0xc = obj+0x10 = 0 (forced input off).
12. **0x45d110**(obj, dt) (sideways slide; its result is dropped) (§6.4).
13. r = **0x468ca0**(obj, flat) (ground, walls, air, landing; §7). It always returns 1.
14. **0x466660**(obj, keys) (air counter, brake dive, gear keys, the gearbox; §5 "Gearbox", §6.6),
    **0x4667e0** (acceleration, dive), **0x466940** (wheel spin), **0x466a80** (suspension, pitch),
    **0x461aa0** (shadow points), **0x469c60** (frame matrix, lean, wheel points),
    **0x4683e0** (crash tumble), **0x467b30**(obj, r) (rider pose, stunts), if r **0x466d70**(obj,
    keys) (dust, spray, skid marks), **0x467930** (speedometer, rpm), obj+0x31d9 = **0x46d340**
    (lane) (§8).
15. obj+0x2dc8 = keys (last step's keys).

Every field the step reads or writes is listed, with its writer, in four tables: §5 "Fields"
(engine and gearbox, body+0x0c..0x118), §6.1 (body fields, the matrix layout and the obj fields
of the integration and steering), §7.6 (the contact) and §8 "Fields" (everything after the
contact, obj+0x7c..0x3205). The input fields are in §2.

After every bike has been stepped, Game_Frame calls the race object's slot +0x18 = **0x47ecd0**
(bike against bike, §8.11; not in Practice).

---------------------------------------------------------------------------------------------

#### 5. The engine: throttle, engine, gearbox, brake, drag

(Throttle, engine, gearbox, brake, drag (the "engine" part of the bike step))

Conventions as build/phys/brief.md: `obj` the bike object, `body` = obj+0x24 (the 0x48xxxx class
works on body; every field is given as body+X (= obj+X+0x24)). `C` = carac row
0x509278 + body[+0x54]·0x124. `ratio[g]` = float at C+0x74+4g (g = 1..6 → RapportDeBoite1..6;
address 0x5092ec + (bike·0x49 + g)·4). `MVM` = C+0x18 MoteurVitesseMax, `VMB` = C+0x4c
VitesseMaxBoite, `RM` = C+0x74 RegimeMax, `RCM` = C+0x90 RegimeCoupleMax, `CM` = C+0x94 CoupleMax,
`C1000` = C+0x98 Couple1000, `CRM` = C+0x9c CoupleRegimeMax, `NbSpeed` = C+0x70, `Masse` = C+0x1c,
`Frein` = C+0x14, `MinAdhW` = C+0xa4 MinAdherenceWheel, `RSM` = C+0x48 RealSpeedMax.
f32(x) = rounded to float by an `fst(p) DWORD`; everything else stays in the 80-bit register
(53-bit mantissa if the process default control word holds [guess]). Constants: "double 0xADDR =
value (bits)" are read as doubles (`QWORD PTR`), "float" as floats.

##### Fields (established here)

| body+ | obj+ | type | name / meaning | mark |
|---|---|---|---|---|
| 0x0c..0x14 | 0x30 | f32×3 | position (only passed, unused, to 0x459f90) | [asm] |
| 0x54 | 0x78 | int | bike number (carac row) | [asm] |
| 0x58 | 0x7c | int | **gear** 1..NbSpeed (engine sound reads obj+0x7c) | [asm] |
| 0x5c | 0x80 | int | gear-shift timer, ms | [asm] |
| 0x60 | 0x84 | int | **shift state**: 0 none, 1 shifting up, 2 shifting down. While ≠ 0 the clutch is out: 0x48deb0/0x48dd90 return their pass-through value and 0x48ef30 revs the engine freely | [asm] |
| 0x64 | 0x88 | int | gearbox: 1 automatic, 0 manual (0x4dc0a0 for player 1, 1 for computer bikes) | [asm] |
| 0x68 | 0x8c | int | pending gear change (+1 up, −n down) | [asm] |
| 0x6c | 0x90 | f32 | engine speed at the end of the last shift (written by 0x48e6e0) | [asm] |
| 0x6c+4g | 0x90+4g | f32 | (g = 1..NbSpeed−1) **upshift table**: road (wheel) speed at which the automatic box leaves gear g; built by 0x48e3a0 when body+0x70 is ±0 | [asm] |
| 0x88 | 0xac | int | brakes grip: 1 normally; 0x466240 sets 0 for a *player's dirt* bike braking while steering (then braking does not slow the wheel, it sets +0x118) | [asm+decomp] |
| 0x8c | 0xb0 | int | brake held time, ms (reset to 0 when the throttle is ≥ 0) | [asm] |
| 0x90 | 0xb4 | f32 | top speed: RSM, + 2.0 for a manual box (0x45bf70) | [asm] |
| 0x94 | 0xb8 | f32 | power multiplier: 1.0; 1.2 (0x3f99999a) or 2.0 when 0x4dc1f8, while the wheelie/turbo angle obj+0x307d ≠ 0 (0x4660f0) | [asm] |
| 0x98..0xc4 | 0xbc..0xe8 | f32×12 | bike matrix, rows R0 (+0x98, right), R1 (+0xa4, up), R2 (+0xb0, forward), T (+0xbc) | [asm; names guess] |
| 0xc8 | 0xec | f32 | **engine speed** in MVM units (0..MVM; ×RM/MVM = rpm) | [asm] |
| 0xcc | 0xf0 | f32 | time in the air: 0 on the ground (0x466660), +2.592 (double 0x4d0f20) each step in the air; ±0 = on the ground everywhere below | [asm] |
| 0xe4 | 0x108 | ptr | owner bike object (→ obj+8 = driven by an input, 0 = computer) | [asm] |
| 0x100 | 0x124 | f32 | **speed** = length of the velocity | [asm] |
| 0x104 | 0x128 | f32 | speed at entry of 0x48ef30 (after the top-speed clamp) | [asm] |
| 0x108 | 0x12c | f32 | **wheel speed**: driven-wheel road speed, signed, kept in ±body+0x90 | [asm] |
| 0x10c..0x114 | 0x130..0x138 | f32×3 | velocity direction (unit; 0 when the speed ≤ 1e-5) | [asm] |
| 0x118 | 0x13c | int | dirt brake-slide: set by 0x48ef30 when a player's dirt bike brakes without grip; cleared at each 0x48ef30 entry (after being read) | [asm] |

Globals: 0x4dc0b4 countdown (1 until the start), 0x4dc1f8 (set to 1 at 0x45d1c0 when 0x4dc094 == 2
championship and 0x509ba8 ≥ 3: torque ×1.1 with a manual box, turbo ×2.0) [asm], **0x4dc1fc** (set
in 0x468ca0 at 0x468eec from a GetWindowLong(…, −21) + [0x540488] == 0x7b5403 test: a copy
protection; when 0 the wheel speed is capped 9.0 under top speed in 0x48dd90 — the port takes 1)
[asm; protection = guess], 0x4dc1f4 (set to 1 at start-up 0x414333; when 0, NbSpeed − 1.0 at
load) [asm], 0x4ea4c8 gear-key latch (one global for all bikes), 0x581014 = dt.

##### Shared pieces

**Local velocity** [asm, repeated inline 6× in 0x48ef30, in 0x48cbe0, 0x48d0f0, 0x48eb40, 0x48e6e0]:
d = body+0x10c normalised by 0x40fcd0 (len = f32(√((x²+y²)+z²)); if len ≠ 0: each comp f32(c·(1/len))),
v = f32(d·speed) per component (0x46d540), inv = 0x40fe80(body matrix) = transpose of R with
translation −R·T (each f32: inv.t_i = −f32((R_i.z·T.z + R_i.y·T.y) + R_i.x·T.x) in machine order),
local = 0x410010(v, inv): local_i = f32(((R_i.z·v.z + R_i.y·v.y) + R_i.x·v.x) + inv.t_i).
So local.x = lateral, local.y = up, **local.z = forward** component; the translation is applied as to
a point (zero effect if T = 0 in the race [guess: check obj+0xe0..0xe8 in memory]).

**Grip factor k** (inline; also 0x48eb40 which returns it) [asm]:
- dirt (C+0 MotoType == 1): lat = 0x48d0f0 (80-bit, see below); a = |lat| stored as double;
  if a > double 0x4d1d48 = 0.01 (3f847ae147ae147b) and a < double 0x4d1d50 = 20.0
  (4034000000000000): k = f32(a·(−0.05) + 1.0) (double 0x4d1d58 = −0.05 bfa999999999999a,
  double 0x4d1c00 = 1.0); else k = 1.0. (0x48eb40: lat comes from its own float local vector.)
- road: k = 1.0, or C+0xa4 MinAdherenceWheel if its bits as int > 0x3f800000 (> 1.0; GOG data: 0.2
  and 0.1, so k = 1.0 always).

**Blend wheel ← forward**: wheel = f32((fwd − wheel)·k + wheel).
**Blend forward ← wheel**: 0x48cbe0(f32((wheel − fwd)·k + fwd)).

**Wheel clamp** [asm]: if wheel > max → max; else if −max > wheel → −max (max = body+0x90; −max
computed as fchs, stored f32).

##### 0x465da0(obj, keys, joyY, joyX, dt) — input → throttle [asm]
Called by 0x45d580 only when not crashed (obj+0x30c1 == 0).
1. s = obj+0x2de5 (race state). If s ∉ {0,1} and obj+8 ≠ 0: keys = 2 (brake).
2. If countdown (0x4dc0b4): keys |= 2. Else if obj+0x307d (wheelie/turbo angle) ≠ ±0: keys |= 1.
3. If keys & 2: obj+0x2e85 = 0 (turbo request off) and, if obj+8 && obj+4, obj+0x31f9 = 0.
4. dir = (f32(obj+0xe0 + obj+0xd4), f32(obj+0xe4 + obj+0xd8), f32(obj+0xe8 + obj+0xdc)) = T + R2;
   len = f32(√((y² + z²) + x²)); if len ≠ 0.0 (float 0x4d0e54): dir = f32(dir·(1/len)) each.
5. Throttle t and scale:
   - joyY ≠ 0x8000 and s ∈ {0,1}: t = 0x8000 − joyY; if t < 0: t = t / 2000 (idiv, toward 0;
     −16..0, a dead zone of 2000); scale = 0x8000. (Keys are ignored.)
   - else if keys & 3: t = −1 if (keys & 2) and not countdown; else 1 if keys & 1; else 0. scale 1.
     (In the countdown accelerate gives +1, nothing else.)
   - else t = 0, scale = 1.
6. 0x48ef30(ECX = obj+0x24; dir.x, dir.y, dir.z, t, scale, dt) (stdcall, ret 0x18).

##### 0x48ef30(body; dx, dy, dz, t, scale, dt) — engine, brake, slope, drag [asm]
Locals: dt10 = f32(dt·10.0f) (float 0x4d1da8 = 10.0 41200000) clamped to 500.0f (0x4d1dac,
43fa0000) — the step in ms; `free` (edi) = countdown || body+0x60 ≠ 0 || body+0x118 ≠ 0 (engine
disengaged); `air` (ebx) = body+0xcc ≠ ±0; `hold` = t < 0 || countdown; `cpu` = (owner obj+8 == 0).

1. **Speed over top speed**: if speed > max: speed = max; if !air: wheel ← blend from fwd (local z
   of the clamped velocity). Wheel clamp. If !free: rpm = 0x48deb0(&wheel, gear, rpm).
2. **Wheel over top speed**: if wheel > max: wheel = max; if !air: forward ← blend from wheel
   (0x48cbe0). If !free: rpm = 0x48deb0(&wheel, gear, rpm).
3. saved = speed (→ body+0x104 at the end).
4. **Throttle** (t ≥ 0): body+0x8c = 0. target = f32((float(t)/float(scale))·MVM) (fild scale,
   fild t, fdivrp, fmul MVM; the compare below uses the unrounded value).
   a. target ≥ rpm and t > 0:
      - free or air: rpm = f32(MVM·dt10·0.0006 + rpm) (double 0x4d1dd0 = 0.0006
        3f43a92a30553261); if < 0 → 0; if > MVM → MVM. Then if air: wheel = 0x48dd90(&rpm, gear,
        wheel).
      - driving (on the ground, engaged):
        idle = MVM/RM·1200.0f (float 0x4d1db0 44960000); if idle > rpm: rpm = f32(idle).
        T = 0x48ee30(rpm) (80-bit).
        F = f32(((((RM / MVM) · rpm) · 2.0f (float 0x4d1c54)) · double 0x4d1db8 =
        0.05235987755982988 (3faacee9f37bebd5, = π/60)) · T), in that order (fld RM; fdiv MVM;
        fmul rpm; fmul 2.0f; fmul π/60; fmulp T).
        (= wheel torque × engine angular speed: rpm·2π/60 → power.)
        if wheel > double 0x4d1c90 = 0.0001 (3f1a36e2eb1c432d): a = f32(F / wheel)
        else a = f32(F · double 0x4d1dc0 = 10000.0 (40c3880000000000)).
        acc = f32(dt10 · a / Masse · double 0x4d1c80 = 0.001 (3f50624dd2f1a9fc));
        cap = dt10 · double 0x4d1dc8 = 0.03 (3f9eb851eb851eb8) (kept as double); if cap < acc:
        acc = f32(cap).
        wheel = f32(wheel + acc); if wheel > max: wheel = max.
        r = 0x48deb0(&wheel, gear, rpm); rpm = f32(r); if r > MVM: rpm = MVM; else if rpm > target:
        rpm = target.
        wheel = 0x48dd90(&rpm, gear, wheel).
        if !air: forward ← blend from wheel (0x48cbe0).
   b. target < rpm, or t == 0 (closing the throttle):
      - free or air: rpm = f32((dt10 · double 0x4d1de0 = −0.0006 (bf43a92a30553261) + 1.0) · rpm);
        < 0 → 0; > MVM → MVM. If air: wheel = 0x48dd90(&rpm, gear, wheel).
      - driving: if MVM/RM·1000.0f (float 0x4d1d80 447a0000) < rpm (engine above 1000 rpm):
        rpm = f32(eb[gear]·rpm·dt10·double 0x4d1dd8 = −0.001 (bf50624dd2f1a9fc) + rpm) with
        **eb[g] = float at 0x4f17dc + 4g** (engine-brake per gear, read from game.EXE: g1 0.25,
        g2 0.2, g3 0.16, g4 0.12, g5 0.08, g6 0.05; the table sits right after the string
        "AnimDecor" at 0x4f17d4, so gear 0 would read junk); < 0 → 0;
        wheel = 0x48dd90(&rpm, gear, wheel); if !air: forward ← blend from wheel.
        (At or below 1000 rpm nothing happens.)
5. body+0x118 = 0.
6. **Brake** (hold = t < 0 or countdown):
   - if body+0x88 == 0 and !cpu and !countdown (player's dirt bike sliding): body+0x8c = 0;
     if dirt: body+0x118 = 1. No braking.
   - else: b = Frein; if Frein's bits < 10.0f's (int compare) → b = 10.0; else if (double)b >
     double 0x4d1df0 = 140.0 → b = 140.0f (0x430c0000). If (double)body+0x8c < double 0x4d1e00 =
     200.0 and !cpu: b = f32(b · float 0x4d1e08 = 0.16666667 (3e2aaaab)) (the player's first 200 ms
     of braking are soft). dec = f32(b · double 0x4d1e10 = 0.2777777777777778 (3fd1c71c71c71c72,
     = 1/3.6) · dt10 · 0.001). wheel moves toward 0 by dec: if wheel > 0 (int > 0): wheel =
     f32(wheel − dec), if it went negative → 0; if wheel < 0 (bits > 0x80000000): wheel =
     f32(wheel + dec), if > 0 → 0. If !free: rpm = 0x48deb0(&wheel, gear, rpm). If !air: forward ←
     blend from wheel. body+0x8c = ftol((double)body+0x8c + dt10).
7. **On the ground (!air): slope and rolling resistance**:
   fwd = local z of the current velocity (f32).
   If !hold, or speed > double 0x4d1ca0 = 0.1: g = 0; (always on the ground here) g =
   f32(−dy · dt10 · double 0x4d1d60 = 0.004905 (3f741743e963dc49, = 9.81/2000)); if g > double
   0x4d1d68 = 0.55 → g = 0.55f (0x3f0ccccd); fwd = f32(g + fwd). (Braking at a standstill: no slope.)
   r = 0x459f90(pos) = float 0x4d0e4c = 0.02 (3ca3d70a) (a constant; the position arguments are
   dropped by `ret 0xc`); dirt: r = f32(r · 0.5 (double 0x4d1c48)). rr = f32(r · dt10 · double
   0x4d1e18 = 0.00981 (3f841743e963dc49)); if speed's bits < 20.0f's: rr = f32(rr · (double)speed ·
   double 0x4d1e20 = 0.05). fwd moves toward 0 by rr: if fwd < 0 (bits > 0x80000000): fwd =
   f32(rr + fwd), if the unrounded sum > 0 → 0; else fwd = f32(fwd − rr), if < 0 → 0.
   0x48cbe0(fwd) (forward component of the velocity := fwd).
   wheel ← blend from the new fwd (only when !air, which holds). Wheel clamp. If !free: rpm =
   0x48deb0(&wheel, gear, rpm).
8. **Air drag** (both cases): d = speed·speed·dt10·(1.6f in the air (float 0x4d1e28 3fcccccd),
   0.3f on the ground (float 0x4d1e2c 3e99999a)) (80-bit), d = f32(d / Masse · 0.001);
   speed = f32(speed − d); negative → 0. (Only the length changes; the direction stays.)
9. If !air: v = local velocity (with the new speed); k = 0x48eb40(body) (80-bit);
   wheel = f32((local.z − wheel)·k + wheel). Wheel clamp. If !free: rpm = 0x48deb0(&wheel, gear, rpm).
10. body+0x104 = saved speed (step 3).

##### 0x48deb0(body; float *wheel, int gear, float rpm) → rpm from the wheel [asm]
If body+0x60 ≠ 0 (shifting): return rpm. If gear < 1: gear = body+0x58.
If *wheel < 0 (bits > 0x80000000): r = rpm; else r = f32(MVM / (ratio[gear]·VMB) · *wheel)
(fld ratio; fmul VMB; fdivr MVM; fmul *wheel). If r < 0 (bits) → 0.
If MVM < r: r = MVM; *wheel = f32(ratio·VMB / MVM · r).
Return r (a float). (wheel = rpm·ratio·VMB/MVM: at the red line MVM the wheel speed is
ratio·VMB, e.g. 1.45·61 = 88.45 in 6th for SPEED1.)

##### 0x48dd90(body; float *rpm, int gear, float wheel) → wheel from the engine [asm]
If body+0x60 ≠ 0: return wheel. If gear < 1: gear = body+0x58.
If (float)gear > NbSpeed: return wheel. If *rpm < 0.0001 (double): return wheel.
w = f32(*rpm / MVM · ratio[gear] · VMB); if the unrounded value < 0.0f → 0.
lim = body+0x90; if 0x4dc1fc == 0: lim = f32(lim − 9.0f) (float 0x4d1c98).
If w > lim: w = lim; *rpm = f32(MVM / (ratio·VMB) · lim). Return w.

##### 0x48ee30(body; float e) → torque (ST0) [asm]
R = f32(RM / MVM · e) (rpm); compare with RCM unrounded.
- R < RCM: slope = (CM − C1000) / (RCM − 1000.0f) (80-bit; stored f32 as `slope`),
  icpt = f32(slope80 · double 0x4d1d88 = −1000.0 + C1000).
- else: slope80 = (CRM − CM) / (RM − RCM), stored f32; icpt = f32(CM − RCM·slope80).
T = (f32slope · R + icpt) · double 0x4d1d90 = 9.81 (40239eb851eb851f); stored f32; if the
unrounded value < double 0x4d1d98 = 0.2 → T = 0.2f (0x3e4ccccd).
If 0x4dc1f8 and body+0x64 == 0 (manual): T = f32(T · double 0x4d1da0 = 1.1).
Return body+0x94 · T (80-bit, not rounded).
(SPEED1: torque line through (1000, 6), (10500, 15), (12000, 2), ×9.81.)

##### 0x48cbe0(body; float w) — set the forward velocity component [asm]
w clamped to ±body+0x90 (if max < w → max; else if −max > w → −max).
v = local velocity (shared piece; lx = local.x, ly = local.y as floats).
new.x = f32(((R1.x·ly + R0.x·lx) + R2.x·w) + T.x)
new.y = f32(((R0.y·lx + R1.y·ly) + R2.y·w) + T.y)
new.z = f32((R2.z·w + (R1.z·ly + R0.z·lx)) + T.z)
speed (body+0x100) = f32(√((y² + z²) + x²)).
If speed's bits & 0x7fffffff: inv = 1/speed (80-bit), n = f32(new·inv) each (else n = new).
If speed > double 0x4d1c08 = 1e-5 (3ee4f8b588e368f1): dir (body+0x10c) = n; else dir = 0.

##### 0x48d0f0(body) → lateral velocity (ST0, 80-bit) [asm]
d = normalised dir (as 0x40fcd0, len stored f32), v = f32(d·speed) each, inv as above (each f32),
return ((R0.z·v.z + R0.y·v.y) + R0.x·v.x) + inv.t0 (no rounding).

##### 0x48eb40(body) → grip factor k [asm]
The grip factor above; dirt: computes the full local vector with float stores (as 0x410010) and
uses |local.x| (float, then held as double).

##### Gearbox

###### Keys: inside 0x466660(obj, keys) [asm 0x466735..0x4667d5]
(The first part of 0x466660 — air time, the brake-on-ground pitch, 0x48e550 — see below/other notes.)
On the ground (obj+0x2db8 ≠ 0) it calls **0x48e550(body)** (automatic box) after its other work;
in the air it does not.
Then, only for an input-driven bike (obj+8 ≠ 0):
- bit4 and bit5 both: latch 0x4ea4c8 = 1.
- bit4 (gear up) only: if latch == 0: if manual (obj+0x88 == 0) 0x48e0a0(body); latch = 1.
- bit5 (gear down) only: if latch == 0: if manual 0x48e120(body); latch = 1.
- neither: latch = 0.
Finally 0x48e6e0(body, ftol(dt · 10.0f (float 0x4d10a0))) (ms, truncated).
(Key bits 4/5 are only set when not in the countdown and the race runs, 0x431250/0x411b30.)

###### 0x48e0a0(body) — request one gear up [asm]
If body+0x60 == 0: if (float)gear ≥ NbSpeed: gear = ftol(NbSpeed). Else if (float)(gear +
pending) < NbSpeed: pending++, timer = 200 (auto) / 150 (manual), state = 1.

###### 0x48e120(body) — request one gear down [asm]
If state ∈ {0, 2}: if gear < 2: gear = 1. Else if gear + pending > 1: pending−−, and if state ≠ 2:
timer = 300 (auto) / 200 (manual), state = 2. (Repeated presses during a down-shift add steps.)

###### 0x48e6e0(body, ms) — finish a shift [asm+decomp]
If state ≠ 0: timer −= ms; when timer < 1:
- state 1: pending = 0, state = 0. If (float)gear < NbSpeed: gear++; if not countdown: rpm =
  0x48deb0(&wheel, gear, rpm); wheel = 0x48dd90(&rpm, gear, wheel); if on the ground: forward ←
  blend from wheel (0x48cbe0, grip factor computed inline); body+0x6c = rpm. Else gear =
  ftol(NbSpeed).
- state 2: n = |pending|; state = 0, pending = 0; body+0x6c = rpm. If gear < 2: gear = 1. Else
  n = min(n, gear − 1); if n > 0: automatic box and gear − n > 1: while gear − n > 1 and
  table[gear − n − 1] − 2.0 (double 0x4d1d40) > wheel: n++ (skip further down to a gear the wheel
  speed suits); gear −= n; if not countdown: rpm = 0x48deb0(&wheel, gear, rpm); wheel =
  0x48dd90(&rpm, gear, wheel).
- other states: state = 0, pending = 0.
So a shift takes 150/200/300 ms with the clutch out (engine free, wheel coasting).

###### 0x48e550(body) — automatic box [decomp, compares asm-checked]
- Manual (body+0x64 == 0): build the table (0x48e3a0) if body+0x70 is ±0; return.
- Automatic, only when body+0x118 == 0 and state == 0:
  if rpm ≤ 0 (int bits < 1): gear = 1; return.
  If not countdown: build the table if needed; g = gear.
  If (float)g ≥ NbSpeed or wheel ≤ table[g]: if g > 1 and wheel < table[g−1] − 2.0 (double) and
  state ∈ {0,2}: down-shift request as 0x48e120 but timers 300 auto / 200 manual.
  Else (wheel > table[g]) and state == 0: up-shift request as 0x48e0a0 (200 / 150 ms).

###### 0x48e3a0(body) — the upshift table [asm]
n = ftol(NbSpeed); top = min(RSM, ratio[n]·VMB) (the product unrounded, stored f32).
g = 1, v = 0.0f; if NbSpeed > 1.0 (bits): while v < top and (float)g < NbSpeed:
c = v; r1 = f32(0x48deb0(&c, g, 0.0)); r2 = f32(0x48deb0(&c, g+1, 0.0)) (c may have been clamped by
the first call); p1 = f32(0x48ee30(r1)·r1); if 0x48ee30(r2)·r2 (unrounded) > p1, or (g < 4 and
MVM · double 0x4d1d38 = 0.97 < r1): table[g] = v, g++. v = f32(v + 0.1 (double 0x4d1ca0)).
Then table[g..NbSpeed−1] = top. (Shift up where the next gear gives more power, or at 97 % of the
red line in gears 1–3.) Note 0x48deb0 returns 0.0 while a shift is in progress, so a table built
then is wrong [asm; whether that happens: the manual path builds it regardless of state].

##### 0x4660f0(obj, dt) — wheelie/turbo angle and the power multiplier [asm]
(Called by 0x45d580 when not in the countdown; listed here because it sets body+0x94.)
- Crashed (obj+0x30c1): angle obj+0x307d = 0, held time obj+0x3081 = 0, request obj+0x2e85 = 0,
  power obj+0xb8 = 1.0; if obj+8 && obj+4: obj+0x31f9 = 0. Return.
- Request (obj+0x2e85, a copy of the Space key obj+0x31f9 made by 0x45d580) and held time's bits <
  200.0f's: angle = f32(dt · double 0x4d10a8 = 0.0471238898038469 (3fa8209f5b22baa7) + angle); if
  > double 0x4d0eb0 = π/2 → 1.5707964f (0x3fc90fdb); held = f32(held + dt).
- Else if angle > 0 (int > 0): angle = f32(dt · double 0x4d10b0 = −0.025132741228718346
  (bf99bc65b68b71c3) + angle); if negative: obj+0x2e95 = −32.0f (0xc2000000), angle = 0.
- angle ≠ ±0: obj+0xb8 = 2.0 if 0x4dc1f8 else 1.2f (0x3f99999a). Else obj+0xb8 = 1.0 and, if no
  request, held = 0.
So the action-6 key lifts the front (0..π/2 in ~33 steps of dt 1) for at most 200 dt units (2 s) a
press, gives ×1.2 power and full throttle (0x465da0 step 2) while the angle is up.

##### Related fields seen but not mapped here
- obj+0x11c: int engine rpm for the sound/HUD (ftol, at least 800), obj+0x114/+0x118 ints, obj+0x120:
  written by 0x467930 (post-step; it calls 0x48e180 with obj+0x7c).
- obj+0x2e95 (set −32.0 when the wheelie ends), obj+0x2ea5 (0x45d580/0x466660 pitch).
- Engine sound (NOTES §4.10.6.3) reads obj+0x7c (gear), obj+0x11c, obj+0x307d.

##### Open points
- Units: speeds look like m/s ×? (RSM 80, VMB 61; 1/3.6 in the brake formula suggests Frein in
  km/h per second) [guess]. obj+0x118 → int speed for the HUD in 0x467930 (not read).
- Whether the matrix translation T (obj+0xe0..0xe8) is zero in the race (else every "local
  velocity" carries a −R·T offset, and 0x465da0's direction is T + R2): read it in memory.
- 0x4dc1fc / 0x4dc1f4 values in GOG's running game (expected 1 / 1): read in memory.
- FPU precision (the 80-bit intermediates assume the default 53-bit control word).

---------------------------------------------------------------------------------------------

#### 6. The body: integration, steering, slide, orientation

(Body integration, steering, lean/slide, orientation, gearbox helpers)

`obj` = bike object (player 1 0x584421), `body` =
obj+0x24 (the class at 0x48xxxx works on it: body+Y = obj+0x24+Y). `C` = carac row
0x509278 + bike*0x124 (names in build/phys/brief.md). `dt` = the step (ms*0.1, >= 1.0), also in
global 0x581014. `dts` below = seconds = float(dt*0.01). Marks: [asm] read in the machine code,
[decomp] Ghidra only, [guess].

Float/double constants are given as they are read (width, address, value, bits).
"f32(x)" = a store to a float (rounding); everything between stores is x87 extended (the FPU's
precision control: process default, no fldcw in the program [guess 53-bit]).
x87 compares: "a < b" etc. below are the exact branch conditions read from the flags.
"int-compare" = the code compares the float's bits as a signed or unsigned integer (MSVC trick):
`(int)bits < 1` means "<= +0.0 or negative"; `(uint)bits > 0x80000000` means "negative, not -0".

Units: speed in m/s (C+0x48 RealSpeedMax 80 = 288 km/h; 0.5556 = 2 km/h threshold),
world = 128 units per metre (displacement = speed*128*seconds; radius*1/128 -> metres).

---------------------------------------------------------------------------------------------

##### 6.1 Body fields (body = obj+0x24) [asm unless marked]

Constructor 0x48bce0(body, &pos, &angles, model) (thiscall, ret 0xc): +0x0c..+0x14 = pos,
+0x00..+0x08 = angles, +0x34 = model, +0x44 = 1, +0x58 = 1, +0x88 = 1, +0x94 = 1.0f; zeroes
+0x30, +0x48, +0x60, +0x68, +0x6c, +0x70..+0x84 (6 dwords), +0x8c, +0xc8, +0xd0, +0xd4,
+0xe4, +0xe8, +0xec, +0xf0..+0xfc, +0x100, +0x104, +0x108, +0x118, +0x180.

| body | obj | name | written by / meaning |
|------|-----|------|----------------------|
| +0x00..08 | +0x24 | angles (menu/ctor) | not used by the step parts here |
| +0x0c..14 | +0x30..38 | **position** x,y,z (world units) | 0x48d620 `pos += d` |
| +0x18..20 | +0x3c..44 | position one step ago | 0x465fa0 (start of step) |
| +0x24..2c | +0x48..50 | position two steps ago | 0x465fa0 |
| +0x30 | +0x54 | dt of the step | 0x465fa0 copies 0x581014 |
| +0x34 | +0x58 | model pointer | ctor |
| +0x54 | +0x78 | bike number (carac row) | |
| +0x58 | +0x7c | **gear** 1..NbSpeed | gearbox §7 |
| +0x5c | +0x80 | shift timer (ms, int) | 0x48e0a0/0x48e120/0x48e550, counted down by 0x48e6e0 |
| +0x60 | +0x84 | shift state: 0 none, 1 up, 2 down | while != 0 the rev/wheel conversion 0x48deb0 returns its "previous" argument |
| +0x64 | +0x88 | automatic gearbox (1) / manual (0) | 0x411010 (= 0x4dc0a0 for player 1, 1 for others) |
| +0x68 | +0x8c | pending gear delta (int) | |
| +0x6c | +0x90 | rev at the last shift (table[0]) | 0x48e6e0 |
| +0x70.. | +0x94.. | table[g] = upshift wheel speed of gear g (g = 1..NbSpeed-1) | 0x48e3a0 |
| +0x88 | +0xac | drive/grip flag (1; 0 = dirt bike braking + steering) | 0x466240; read by 0x48ef30 (engine part) |
| +0x90 | +0xb4 | **max speed** = C+0x48 RealSpeedMax (+2.0 manual, 0x45bf70) | |
| +0x94 | +0xb8 | turbo factor 1.0 / 1.2 / 2.0 | 0x4660f0 |
| +0x98..c4 | +0xbc..e8 | **orientation matrix** (12 floats, layout below) | 0x45cb00 |
| +0xc8 | +0xec | **engine rev** e, 0..C+0x18 MoteurVitesseMax | 0x48deb0 |
| +0xcc | +0xf0 | **airborne counter**: 0 on the ground, +2.592 (double 0x4d0f20) per step without contact | 0x466660 |
| +0xd0 | +0xf4 | smoothed half-sum of the longitudinal speed change | 0x4667e0 (other part) |
| +0xd4 | +0xf8 | **longitudinal acceleration** (m/s^2) = (lon - lon_at_step_start)/(dt*0.01) | 0x4667e0 |
| +0xd8..e0 | +0xfc..104 | displacement of this step (world units) | 0x48d620 |
| +0xe4 | +0x108 | pointer to the owning obj | 0x45bf70 |
| +0xe8 | +0x10c | **steering rate** | 0x466240 |
| +0xec | +0x110 | **steering angle** | 0x466240 |
| +0x100 | +0x124 | **speed** (m/s, magnitude) | 0x48d620, 0x48d2c0 |
| +0x108 | +0x12c | **wheel speed** (signed, clamped to +-max) | 0x48d620 tail, gearbox |
| +0x10c..114 | +0x130..138 | **F**: unit velocity direction ("forward") | 0x48d620, 0x48d2c0 |
| +0x118 | +0x13c | flag: doubles the dirt v^2/r term (0x45d110), blocks the automatic gearbox (0x48e550) | set elsewhere |
| +0x11c..148 | +0x140..16c | matrix of the previous step | 0x465fa0 |
| +0x150..158 | +0x174..17c | **P**: unit direction of an extra (impulse) velocity | 0x48d620; set by collisions (other part) |
| +0x15c | +0x180 | **|P|**, impulse speed, decays 33.33 m/s per second | 0x48d620 |
| +0x168..170 | +0x18c..194 | **V**: unit smoothed motion direction (what moves the bike) | 0x48d620 |

**Matrix layout** (body+0x98, 12 floats M0..M11): M0,M1,M2 = image of the x axis ("right"),
M3,M4,M5 = y ("up"), M6,M7,M8 = z ("forward"), M9..M11 = translation T. A point maps as
out = M0..2*x + M3..5*y + M6..8*z + T. **T is always 0 for the bike**: every writer of
obj+0xe0..0xe8 in the step stores 0 (0x45cb00 at 0x45ccdc, and 0x469c60/0x46bb2e/0x46c50b)
[asm]. The "inverse" built inline by 0x48ca10/0x48d0f0/0x48d2c0/0x48e6e0 is the transpose,
with translation -(R^T T) = -0.0: it never changes a value.

obj fields outside body used here: +0x8 driven by inputs (else AI), +0xc forced-input active,
+0x10 forced input bits (OR-ed into the keys by 0x466240), **+0x18 turn radius in metres
(signed; FLT_MAX = none)** (0x45d110), +0x2da0 "sliding" flag (0x45d580: grounded +0x2db8 and
|lateral| >= 0.04), +0x2da8 longitudinal speed at step start (0x465fa0 via 0x48ca10),
+0x2db8 wheels on the ground (contact part), +0x2e29 steering is analogue (1) / keys (0),
+0x2e55..+0x2e84 copy of the matrix (written by 0x45b4e0, 0x469c60 and the drawer 0x45f290;
read by 0x45ca00), +0x2ea5 brake-dive angle (<= 0), +0x307d turbo/wheelie angle (0x4660f0),
+0x31c1 current track section.

---------------------------------------------------------------------------------------------

##### 6.2 Small helpers [asm]

- 0x40fcb0(v) |v|: sqrt((x*x + y*y) + z*z), returned unrounded in ST0.
- 0x40fcd0(v) normalise in place: n = f32(sqrt((x*x + y*y) + z*z)); if n != 0: k = 1/n
  (extended); x = f32(x*k), y = f32(y*k), z = f32(k*z).
  The same inline sequence (n stored as float, then 1/n) is used everywhere a vector is
  normalised below ("norm()").
- 0x410010(out, v, M) (stdcall): out.x = f32(((M6*vz + M3*vy) + M0*vx) + M9),
  out.y = f32(((M1*vx + M4*vy) + M7*vz) + M10), out.z = f32(((M8*vz + M5*vy) + M2*vx) + M11).
- 0x403a80(out, A, B) (stdcall): out = B∘A (apply A, then B), each element one f32 store:
  o0 = a2*b6 + (a1*b3 + a0*b0); o1 = (a0*b1 + a2*b7) + a1*b4; o2 = (a0*b2 + a1*b5) + a2*b8;
  o3 = (b0*a3 + a5*b6) + b3*a4; o4 = (a5*b7 + b1*a3) + b4*a4; o5 = (a5*b8 + b5*a4) + b2*a3;
  o6 = (b0*a6 + a7*b3) + a8*b6; o7 = (b1*a6 + b7*a8) + a7*b4; o8 = (b5*a7 + a6*b2) + b8*a8;
  o9 = ((b0*a9 + a11*b6) + b3*a10) + b9; o10 = ((a9*b1 + a11*b7) + b4*a10) + b10;
  o11 = ((a9*b2 + b5*a10) + a11*b8) + b11. (Products commute; the grouping is what counts.)
- Angle -> table: i = __ftol(angle * double 2607.594587618272 [0x4d0300 / 0x4d0ed0, bits
  0x40a45f306dc9ce2b]) (truncation), cos = COS[i & 0x3fff], sin = COS[(i-0x1000) & 0x3fff],
  COS = 0x5fcb80 (NOTES §4.9.3.2).
- 0x40fd40(M, &angles a,b,c) (thiscall, ret 4) Euler matrix, T = 0 [asm]. With ca,sa / cb,sb /
  cc,sc from the table (sc kept as a float in memory):
  M0 = cb*cc; M1 = (sb*sa)*cc + sc*ca; M2 = sc*sa - f32(sb*ca)*cc; M3 = -(sc*cb);
  M4 = ca*cc - f32(sb*sa)*sc; M5 = cc*sa + f32(sb*ca)*sc; M6 = sb; M7 = -(cb*sa); M8 = cb*ca.
  (In M1 the product sb*sa is the unrounded one; in M4 the float-rounded copy.)
- 0x46d270(M, out) angles from a matrix [asm]: sb = M6; cb = f32(sqrt(double 1.0 - sb*sb));
  if cb > float 1e-5 (0x4d0e50, 0x3727c5ac): b = f32(atan2(sb, cb)); a = f32(atan2(-(M7/cb),
  M8/cb)); cc' = f32(M0/cb); sc' = f32(-(M3/cb)); else b = +-1.5707964f (sign of M6's bits),
  a = 0, cc' = M4, sc' = M1. c = f32(atan2(sc', cc')). out = (a, b, c).
  (fpatan: atan2(st1, st0).)
- 0x45cb00(obj, R by value: 48 bytes on the stack, ret 0x30) **turn the bike by R in its own
  frame and re-square the matrix** [asm]: M = 0x403a80(R, M) (= M∘R); (a,b,c) = 0x46d270(M);
  rebuild M from (a,b,c) through the table (constant 0x4d0ed0), different rounding from 0x40fd40:
  M0 = cc*cb; M1 = sc*ca + f32(sb*cc)*sa; M2 = sc*sa - f32(sb*cc)*ca; M3 = -(sc*cb);
  M4 = cc*ca - f32(sc*sb)*sa; M5 = cc*sa + f32(sc*sb)*ca; M6 = sb; M7 = -(cb*sa); M8 = cb*ca;
  T = 0. So every rotation quantises the orientation to 16384 steps a turn per axis.
- 0x458da0(a) wrap: while a < -pi (double 0x4d0e18) a += 2pi (double 0x4d0e20); while a > pi
  (0x4d0e28) a -= 2pi; the loops keep the extended value, one f32 at the end.
- 0x48ca10(body) **longitudinal speed** [asm]: u = norm(F); w = f32(speed*u) per component;
  returns (M7*wy + M8*wz) + M6*wx + (-0.0) (unrounded, ST0). = w·Z.
- 0x48d0f0(body) **lateral speed** [asm]: same w; returns (M2*wz + M1*wy) + M0*wx + (-0.0) =
  w·X. (Right after 0x48d620 F is parallel to Z, so this is ~0; the lateral part lives in F
  between 0x48d2c0 and the next 0x48d620.)
- 0x40c8b0(track 0x52c108, section s, &p) **road height** [asm]: if count [0x52c10c] <= 0 ->
  0.0f. Section record = [0x52c108] + s*0xfc; plane test = 0x4b10c0(record+0xa0, p) (n·p + d,
  ((ny*py + nz*pz) + nx*px) + d; counts calls in 0x709378); if < 0.0f -> plane (a,b,c,d) at
  record+0x90, else at record+0x80. If b == +-0 -> 0.0f; else returns
  -(((pz*c) + (a*px)) + d) / b (extended; callers store a float).

---------------------------------------------------------------------------------------------

##### 6.3 0x48d620(body, dt, flag) — integrate one step [asm]

Called from 0x45d580 with flag = 1 when the bike is a dirt bike (C+0x120) and
0x46c7a0(obj+0x31c1, 8) == 0 (other part), else 0. thiscall, ret 8. In order:

1. dtk = f32(dt * float 0.01f [0x4d1c10, 0x3c23d70a]) (= seconds); old = speed (body+0x100).
2. F = 0x410010(M, (0,0,1)) = Z + T; F = norm(F) (inline) ; store body+0x10c..114.
3. **Branch on the airborne counter**: if body+0xcc <= double 2.592 [0x4d1c18,
   0x4004bc6a7ef9db23] -> **ground**, else **air**.

**Air** (more than one step without contact):
  a. V = norm(V); V = f32(V*speed) per component.
  b. dtk becomes a double (`fst qword`) for the rest of the routine (same value).
  c. Vy = f32(Vy - dtk*double 24.525 [0x4d1c20, 0x4038866666666667]) (gravity 9.81*2.5).
  d. speed = f32(sqrt((Vy*Vy + Vz*Vz) + Vx*Vx)); V = norm(V).
  e. if |P| (body+0x15c) != +-0: Px,Py,Pz = f32(P*|P|); Py = f32(dtk*double -24.525
     [0x4d1c28] + Py); |P| = f32(sqrt((Py*Py + Pz*Pz) + Px*Px)); k = 1/|P| (extended);
     P = (f32(Px*k), f32(Py*k), f32(k*Pz)).
  f. k = (speed * double 128.0 [0x4d1c30]) * dtk; d = (f32(Vx*k), f32(Vy*k), f32(k*Vz)).
  g. if max (body+0x90) < speed: speed = max.

**Ground**:
  a. c = (Vy*Fy + Vz*Fz) + Vx*Fx; if c < 0: speed = f32(speed * |f32(c)|).
  b. grip g: k = (C+0x120 dirt ? double 11.0 [0x4d1c40] : double 6.0 [0x4d1c38]) /
     |obj+0x18| (turn radius, metres); g = f32(double 1.0 [0x4d1c00] - k);
     int-compare: if bits(g) > 0x3f800000 -> g = 1.0f, else if g < double 0.5 -> g = 0.5f.
     (So g = 1 when no radius, 0.5..1 in tight turns.)
  c. r = f32(speed / max * float 2.0f [0x4d1c54]); if the unrounded value > double 0.9
     [0x4d1c58] -> r = 0.9f (0x3f666666); then if r < double 0.85 [0x4d1c68] -> r = 0.85f
     (0x3f59999a). w = 1.0f - r (extended).
  d. **V blends towards F**: Vx = f32((Fx*w) + Vx*r); Vy (computed, then thrown away);
     Vz = f32((w*Fz) + Vz*r); then **Vy = Fy** exactly. V = norm(V).
  e. dtk -> double; k = ((speed*g) * double 128.0) * dtk;
     d = (f32(Vx*k), f32(Vy*k), f32(k*Vz)).
  f. if flag == 0 or int-compare dy <= 0: dy = f32(dtk * double -1255.68 [0x4d1c78,
     0xc0939eb851eb851f] + dy) (a downward push of 9.81*128 per second, every grounded step).
  g. if |P| != +-0: c2 = (Pz*Fz + Py*Fy) + Px*Fx; Py = (c2 < 0 ? -Fy : Fy); P = norm(P).

**Tail** (both):
  1. kP = (|P| * double 128.0) * dtk; dx = f32(Px*kP + dx); dy = f32(Py*kP + dy);
     dz = f32(kP*Pz + dz) (the unrounded dz is kept and stored after the next two lines).
  2. pos.x = f32(dx + pos.x); pos.y = f32(dy + pos.y); pos.z = f32(dz + pos.z).
  3. if |f32(old - speed)| > double 0.001 [0x4d1c80]:
     - if body+0xcc == +-0 (on the ground this step): a = f32(0x48ca10(body)); b = 0x48eb40(body)
       (engine part: blend factor, ST0); wheel = f32((a - wheel)*b + wheel).
     - clamp wheel to [-max, +max] (if wheel > max: max; else if -max > wheel: f32(-max)).
     - rev = f32(0x48deb0(body, &wheel, gear, rev)) (§7).
  4. dec = f32(dtk * double 33.333333333333336 [0x4d1c88, 0x4040aaaaaaaaaaab]);
     if int-compare |P| > 0: |P| = f32(|P| - dec), negative -> 0; else if negative (bits >
     0x80000000): |P| = f32(|P| + dec), int > 0 -> 0.

Fields: reads/writes body+0x0c..14, 0x90, 0xc8, 0xcc, 0xd8..e0, 0xe4(->obj+0x18), 0x100,
0x108, 0x10c..114, 0x150..15c, 0x168..170, M (read).

---------------------------------------------------------------------------------------------

##### 6.4 0x45d110(obj, dt) — sideways slide (lean of the velocity) [asm]

Runs after 0x48d620. thiscall, ret 4. Returns the new lateral speed (ST0).
1. dts = f32(dt * double 0.01 [0x4d0ed8]) (overwrites the argument slot).
2. If body+0xcc (obj+0xf0) > double 2.592 [0x4d0f20]: return 0x48d0f0(body) (nothing else).
3. lon = f32(0x48ca10(body)); lat = f32(0x48d0f0(body));
   grip = f32(0x459f80(...)) = 1.0f: 0x459f80 is a stub returning float [0x4d0e48] = 1.0 and
   popping 4 ignored arguments (rear wheel x, y, z, bike) — ret 0x10.
4. Championship check: if 0x4dc094 == 2 and [0x509ba8] >= 3: 0x4dc1f8 = 0, then the bytes at
   0x442761/0x442762 (code of the main menu: 0x75 0x22) are compared with 0x75 / 0x22 and both
   `jne` go to the next instruction, then 0x4dc1f8 = 1. **0x4dc1f8 is a "genuine copy" flag**:
   initial value 1 in the image, always 1 in this build; when 0 the game would weaken the turbo
   (0x4660f0: factor 1.2 instead of 2.0) and something in 0x48ef30 (0x48eef2). The code at
   0x45d1cf ("ILLEGAL VERSION" text) is unreachable. Port: treat 0x4dc1f8 as 1.
5. **Turn radius** (only when obj+0xc == 0 and int-compare speed > 1.0f):
   R = 0x45cd10(x0 = obj+0x48, z0 = obj+0x50, x1 = obj+0x3c, z1 = obj+0x44, x2 = obj+0x30,
   z2 = obj+0x38, 0, 0) (positions two steps ago, one step ago, now) * double 0.0078125
   [0x4d0ee0] (= /128 -> metres).
   - Dirt (C+0 MotoType == 1): Rloc = f32(R*k) and obj+0x18 = Rloc (obj+0x18 unchanged and
     Rloc = FLT_MAX when the condition fails).
   - Road: obj+0x18 = FLT_MAX first; when the condition holds obj+0x18 = f32(R*k). Rloc stays
     FLT_MAX (not used for road bikes).
6. Road camber: p = obj+0x30..38; py = (sections ? f32(0x40c8b0(track, obj+0x31c1, p)) : 0);
   q = (f32((M9 + M0) + px), f32((M1 + M10) + py), f32((M2 + M11) + pz)) (the point one unit
   to the bike's right; obj+0xe0 + obj+0xbc etc.);
   h = sections ? f32(qy - 0x40c8b0(track, section, q)) : qy.
   s = f32(h / sqrt(h*h + double 1.0)) (sine of the right axis' tilt over the road).
7. a = 0; if dirt: a = f32(lon*lon / Rloc) (centripetal); if body+0x118: a = f32(a * double 2.0).
   s9 = f32(s * double 9.81 [0x4d0f30, 0x40239eb851eb851f]); g9 = f32(grip * double 9.81).
8. new = f32(((dirt ? s9 + a : s9) * dts) + lat); dec = f32(g9 * dts).
9. Towards 0 without crossing: if int-compare new > 0: new = f32(new - dec), < 0 -> 0; else if
   new < 0 (as double): new = f32(dec + new), > 0 -> 0.
10. 0x48d2c0(body, new); return new.

Effect: road bikes never keep a lateral speed (the camber term is at most 9.81 and the decay
is 9.81, from a lateral of ~0); dirt bikes slide outwards when v^2/r beats g*(1 - s).

###### 0x45cd10(x0,z0,x1,z1,x2,z2, &cx, &cz) — signed turning radius [asm]

stdcall, ret 0x20; returns ST0. Points P0 (oldest), P1, P2 (newest) in the ground plane (x,z).
1. A = f32(z1-z0), B = f32(x2-x1), C = f32(x1-x0), D = f32(z2-z1).
2. If (|D| < double 1e-4 [0x4d0eb8] and |B| < 1e-4) or (|A| < 1e-4 and |C| < 1e-4):
   *cx = *cz = 0 (if given), return float FLT_MAX [0x4d0e74].
3. d1 = f32(sqrt(C*C + A*A)); d2 = f32(sqrt((z1-z2)^2 + (x1-x2)^2)) (each difference
   unrounded); l1 = d1, l2 = d2, int-compares: > 800.0f -> 800.0f, then < 128.0f -> 128.0f.
4. Two points half a clamped step from P1 (stored as floats over the argument slots):
   Q1z = f32(((l2/d2)*D)*0.5 + z1); Q1x = f32(((l2/d2)*B)*0.5 + x1);
   Q0z = f32((((z0-z1)/d1)*l1)*0.5 + z1); Q0x = f32((((x0-x1)/d1)*l1)*0.5 + x1)
   (0.5 = double 0x4d0f08).
5. G = f32(Q1z - z1); H = f32(Q1x - x1); J = Q0x - x0 (unrounded); E = f32(Q1z - Q0z);
   K = G*E + H*Q1x; Num = double(K*(Q0z - z0) - Q0x*(J*G));
   Den = double((Q0z - z0)*H - J*G).
6. If |f32(Den)| <= 1e-4 or |G| <= 1e-4: *cx = *cz = 0, return FLT_MAX.
7. cx = f32(Num/Den); cz = f32(((Q1x - cx_unrounded)/G)*H + Q1z) (cz uses the extended
   quotient, later uses of cx the float); store *cx, *cz if given.
8. R = f32(sqrt((x1-cx)^2 + (z1-cz)^2)) (sum: (x1-cx)^2 first); side =
   (cz - Q0z)*(Q1x - Q0x) - (cx - Q0x)*E; if side < 0: R = -R. Return f32(R * double 2.0).

###### 0x48d2c0(body, lat) — set the lateral speed [asm]

thiscall, ret 4. 1) lat clamped: if max < lat -> max; else if -max > lat -> f32(-max).
2) w = f32(speed*norm(F)). 3) l = 0x410010(w, transpose of M) (ly = ((M3*wx + M4*wy) + M5*wz),
lz = ((M8*wz + M7*wy) + M6*wx), each + -0.0, f32). 4) back to world with lx = lat:
x = f32(((M0*lat + M3*ly) + M6*lz) + M9); y = f32(((M1*lat + M7*lz) + M4*ly) + M10);
z = f32(((M2*lat + M5*ly) + M8*lz) + M11). 5) speed = f32(sqrt((y*y + z*z) + x*x)).
6) u = (x,y,z); if speed != +-0: k = 1/speed, u = (f32(x*k), f32(y*k), f32(k*z)).
7) F = u if |speed| > double 1e-5 [0x4d1c08], else F = (0,0,0).

---------------------------------------------------------------------------------------------

##### 6.5 0x466240(obj, keys, joyY, joyX, dt) — steering and yaw [asm]

Called by 0x45d580 only when not in the countdown (0x4dc0b4 == 0) and not crashed. ret 0x10.
1. lat0 = f32(0x48d0f0(body)) (lateral speed left by the previous step).
2. if obj+0xc: keys |= obj+0x10.
3. **Steering angle** s = obj+0x110, rate r = obj+0x10c; RAA = C+0x24
   RoueAccelerationAngulaire, RLA = C+0x34 RoueLimiteAngulaire, RVR = C+0x2c RoueVitesseRetour;
   K = double 0.5555555555555556 [0x4d10c0] (2 km/h):
   - **Analogue** (joyX != 0x8000): n = joyX - 0x8000, n = 0 if speed < K; obj+0x2e29 = 1;
     s = f32(((float)n * RLA) * float 3.0517578125e-05 [0x4d10c8] (1/32768)). r untouched.
   - **Right** (keys & 8) and speed > K: obj+0x2e29 = 0; if s < 0 (bits > 0x80000000): s = 0;
     r = f32(RAA*dt + r); new = r*dt + s (unrounded); s = f32(min(new, RLA)) (RLA when
     RLA <= new).
   - else **Left** (keys & 4) and speed > K: obj+0x2e29 = 0; if int-compare s > 0: s = 0;
     r = f32(RAA*dt + r); new = s - r*dt; s = f32(max(new, -RLA)) (-RLA when -RLA >= new).
   - else **centre**: ret = obj+0xf8 (longitudinal acceleration) * RVR * double 1/6
     [0x4d10d0]; if ret < RVR: ret = RVR (else f32(ret)); r = 0; if int-compare s > 0:
     s = f32(s - ret*dt), negative -> 0; else if s negative: s = f32(ret*dt + s), int > 0 -> 0.
   (Keys: right is tested before left; 0x431250 already makes them exclusive.)
4. **Turn factor** T: if obj+8 == 0: T = 0.004f (0x3b83126f). Else f = speed / C+0x4c
   VitesseMaxBoite * double 0.5 [0x4d0f08] (kept as double); if f < double 0.0 [0x4d0f28]:
   f = 0, else f = min(f, 0.5). T = RT - (f*RT)*0.5f (RT = C+0x50 RotationTransfert, 0.5f =
   float 0x4d1018); if obj+0x307d (turbo angle) != +-0: T = T*0.5f (from the unrounded value);
   T = f32(T). Then T2 = f32((T*dt) * float 0.01f [0x4d1004]).
5. obj+0xac (body+0x88) = 1; if driven and dirt (MotoType 1): keys: 0 when (keys & 2) and
   (keys & 0xc) (braking while steering) else 1; analogue: 0 when joyX < 0x4000 or > 0xc000,
   else 1; then 1 again if int-compare speed < 2.0f.
6. **Grip multiplier** m = 1.0f; if obj+0x2da0 (sliding): m = |lat0| * double 0.1 [0x4d0ea8]
   + double 1.0 [0x4d0ec8]; if m < 1.0 -> 1.0 else min(m, double 1.6 [0x4d10e0]); m = f32(m);
   if (keys & 2) and (keys & 0xc): m = 1.6f (0x3fcccccd).
7. yaw = f32((m * T2) * s); R = 0x40fd40((0, yaw, 0)); 0x45cb00(obj, R).
   **The bike turns by yaw = T2*m*steering radians per step about its own up axis**, i.e.
   rate = RotationTransfert*(1 - 0.5*min(speed/(2*VMB), 0.5)) * steering per second (x0.5 with
   turbo). Road SPEED1: RLA 600 * RT 0.0028 = 1.68 rad/s at full lock, standing; 1.26 rad/s at
   VMB and above.

---------------------------------------------------------------------------------------------

##### 6.6 0x466660(obj, keys) — airborne counter, brake dive, gear keys, shifts [asm]

thiscall, ret 4. Runs after 0x468ca0 (ground contact sets obj+0x2db8).
1. If obj+0x2db8 == 0 (no contact): obj+0xf0 = f32(obj+0xf0 + double 2.592) (per step, not
   scaled by dt). Else:
   a. obj+0xf0 = 0; lon = f32(0x48ca10(body)).
   b. If (keys & 2) and lon > double 0.1 [0x4d0ea8]: **nose dip**: a = 0x45d0c0(obj,
      0.024543693f [0x3cc90fdb] = 2pi/256); R = 0x40fd40((a, 0, 0)); 0x45cb00(obj, R)
      (a pitch of 1/256 turn per braking step, unless the slope test says 0);
      obj+0x2ea5 = f32([0x581014]*double -0.001963495408493621 [0x4d10f0] + obj+0x2ea5);
      if < double -0.19634954084936207 [0x4d10f8]: obj+0x2ea5 = -0.19634955f (0xbe490fdb).
      (0x45d580 brings +0x2ea5 back: when not braking or speed < 1: if > 0 -> 0, else
      += dt*double 0.0025 [0x4d0f38], int > 0 -> 0 [decomp].)
   c. 0x48e550(body) (automatic gearbox, §7).
2. If driven (obj+8): gear keys with a global latch 0x4ea4c8 (one shift per press; shared by
   all bikes): both 0x10 and 0x20 -> latch = 1; 0x10 alone and latch 0: if manual (obj+0x88
   == 0) 0x48e0a0 (up); latch = 1; 0x20 alone and latch 0: if manual 0x48e120 (down);
   latch = 1; neither -> latch = 0. (In automatic the keys do nothing but set the latch.)
3. 0x48e6e0(body, __ftol([0x581014] * float 10.0f [0x4d10a0])) (= ms, truncated).

0x45d0c0(obj, a) [asm]: a = f32(0x458da0(a)); p = 0x45ca00(obj) (as double); if p > double
1.4726215563702154 [0x4d0f10] or p < double -1.4726215563702154 [0x4d0f18]: return 0; else a.
0x45ca00(obj) **pitch of the bike** from the matrix copy obj+0x2e55 (X = +0x2e55..5d,
Z = +0x2e6d..75, T = +0x2e79..81) [asm]: fx = f32(Zx+Tx), fy = f32(Zy+Ty), fz = f32(Zz+Tz),
Xx' = f32(Xx+Tx), Xz' = f32(Xz+Tz); h = f32(fz*fz + fx*fx); n = f32(sqrt((fz*fz + fy*fy) +
fx*fx)) — sum order (fy*fy + fz*fz) + fx*fx; r = f32(sqrt(h) * n); if |r| < 1e-4: return
(int-compare fy > 0 ? 1.5707964f : -1.5707964f); else return atan2((((fz*fy)*Xx') +
((-(fx*fy))*Xz'))/r, h/r) (unrounded).

---------------------------------------------------------------------------------------------

##### 6.7 Gearbox and rev helpers [asm unless marked]

NbSpeed = C+0x70 (float), ratio[g] = C+0x74+4g (RapportDeBoite g), VMB = C+0x4c, MVM =
C+0x18 MoteurVitesseMax, RSM = C+0x48 RealSpeedMax.

- **0x48deb0(body, &v, g, prev)** rev from wheel speed (thiscall, ret 0xc): if body+0x60
  (shifting) return prev; if g < 1: g = gear; if v negative (bits > 0x80000000): e = prev
  else e = f32(MVM / (ratio[g]*VMB) * v); if e negative: e = 0; if MVM < e: e = MVM,
  v = f32(((ratio[g]*VMB) / MVM) * e) (written back). Returns e.
- **0x48df90(body, &v, prev)** (reset only: 0x411010, 0x412a50) choose a gear for v: if
  shifting return prev; loop: e = MVM/(ratio[g]*VMB)*v; if (unrounded) e < double 0.1
  [0x4d1ca0]: if g <= 1 stop, else g-- (store); else if MVM < f32(e): if (float)g < NbSpeed
  g++ (store) else stop; repeat while MVM < e. Then e negative -> 0; else if MVM < e: e = MVM,
  v = f32(((ratio[g]/MVM)*VMB)*e). Returns e.
- **0x48edd0(body)** (reset): wheel = speed, clamped to [-max, max].
- **0x48e0a0(body)** gear up (manual key): only if not shifting: if NbSpeed <= (float)gear:
  gear = __ftol(NbSpeed); else if (float)(gear + pending) < NbSpeed: pending++, timer =
  (auto ? 200 : 150) ms, state = 1.
- **0x48e120(body)** gear down (manual key): only if state is 0 or 2: if gear <= 1: gear = 1;
  else if gear + pending > 1: pending-- (also when already state 2), and if state was 0:
  timer = (auto ? 300 : 200), state = 2.
- **0x48e550(body)** automatic gearbox (every grounded step): manual -> only build the table if
  table[1] (body+0x70) == +-0 (0x48e3a0). Auto: nothing if body+0x118 or shifting; if
  int-compare rev <= 0: gear = 1; nothing during the countdown; build the table if empty;
  up: if (float)gear < NbSpeed and table[gear] < wheel: (if not shifting) pending++, timer 200,
  state 1 (same tests as 0x48e0a0); else down: if gear > 1 and table[gear-1] - double 2.0
  [0x4d1d40] > wheel and state is 0 or 2: as 0x48e120 with timer 300.
- **0x48e3a0(body)** shift table: n = __ftol(NbSpeed); vmax = f32(min(RSM, ratio[n]*VMB));
  g = 1, v = 0.0f; if int-compare NbSpeed > 1.0: while v < vmax: vc = v; e1 =
  f32(0x48deb0(&vc, g, 0)); e2 = f32(0x48deb0(&vc, g+1, 0)) (vc possibly clamped by the first
  call); p1 = f32(T(e1)*e1); if T(e2)*e2 > p1, or (g < 4 and MVM*double 0.97 [0x4d1d38] < e1):
  table[g] = v, g++; v = f32(v + double 0.1); stop when (float)g >= NbSpeed.
  Then table[g..NbSpeed-1] = vmax. T = 0x48ee30 (torque curve, engine part).
- **0x48e6e0(body, ms)** shift in progress: if state: timer -= ms; when timer <= 0:
  - state 1: pending = 0, state = 0; if NbSpeed <= (float)gear: gear = __ftol(NbSpeed); else
    gear++; unless countdown: rev = f32(0x48deb0(&wheel, gear, rev)); wheel =
    f32(0x48dd90(&rev, gear, wheel)) (engine part); if on the ground (body+0xcc == +-0):
    l = transpose(M)*(speed*norm(F)); k = 1.0f; dirt: x = (0x410010 of norm(F) scaled by
    0x46d540(speed) through 0x40fe80(M)).x [engine-part helpers, not fully read]; a = |x|;
    if a > double 0.01 and a < double 20.0: k = f32(a*double -0.05 + 1.0); road: k = 1.0,
    or C+0xa4 MinAdherenceWheel if its bits > 1.0f (never, values 0.1..0.2);
    0x48cbe0(body, f32((wheel - lz)*k + lz)) (engine part: sets the forward speed);
    body+0x6c = rev.
  - state 2: n = |pending|; state = pending = 0; body+0x6c = rev; if gear <= 1: gear = 1;
    else n = min(n, gear-1); if n >= 1: auto: while gear-n > 1 and table[gear-n-1] - 2.0 >
    wheel: n++ (several gears at once); gear -= n; unless countdown: rev, wheel as above.
  - other: state = pending = 0.

---------------------------------------------------------------------------------------------

##### 6.8 Order inside the step (this part's routines, from 0x45d580)

0x465fa0 (previous position/matrix, lon at start) -> ... -> **0x466240** (steer, yaw rotation,
uses lat from the previous step) -> 0x46c7a0 -> **0x48d620** (F from the matrix, V blend,
gravity/air, move) -> **0x45d110** (turn radius, slide, F gets its lateral part) -> 0x468ca0
(ground) -> **0x466660** (air counter, brake dive, gearbox) -> 0x4667e0 (longitudinal
acceleration into obj+0xf8, used by the next step's centring) -> ...

##### 6.9 Open points
- 0x48eb40 (blend factor of the wheel speed), 0x48dd90, 0x48ee30, 0x48cbe0, 0x46d540, 0x40fe80:
  engine part, not read here.
- Who sets body+0x118 (obj+0x13c), obj+0xc/+0x10 (forced inputs), P (+0x150..15c): collision
  part.
- FPU precision control assumed 53-bit (no fldcw found); if DirectX set 24-bit, every
  "extended" above would be single [guess].
- Ghidra's text for 0x45cd10 names the differences differently (it is right); for 0x48d620
  it shows the blended Vy being stored then overwritten — the asm agrees (Vy = Fy).

##### 6.10 The bike's rotation: 0x40fd40, 0x403a80, 0x46d270, 0x45cb00 [asm]

(Supersedes the [decomp] line on 0x45cb00 in §7.2. Transcribed bit-exact in tools/bikephys.py:
rot_40fd40, matmul_403a80, angles_46d270, rot_45cb00, rotate_45cb00.)

- Angle → table: i = ftol(a · d 0x4d0300 / 0x4d0ed0 = 2607.594587618272 (40a45f306dc9ce2b))
  (truncation toward 0, of the unrounded product), cos = COS[i & 0x3fff], sin =
  COS[(i − 0x1000) & 0x3fff]. sin(0) is therefore COS[0x3000] ≈ −1.8e−16, not 0: the "zero"
  angles of a yaw rotation leave tiny terms in the matrix, which the bit-exact path must keep.
- **0x40fd40**(out, &(a0, a1, a2)): the rotation of NOTES §4.9.3.3, with A = f32(s1·s0) and
  B = f32(s1·c0) stored on the way and every element one register expression, stored once:
  M0 = c1·c2, M1 = (s1·s0)·c2 + s2·c0 (s1·s0 unrounded here), M2 = s2·s0 − B·c2,
  M3 = −(s2·c1), M4 = c0·c2 − A·s2, M5 = c2·s0 + B·s2, M6 = s1, M7 = −(c1·s0), M8 = c1·c0, T = 0.
- **0x403a80**(out, A, B): out = A·B for 3×4 row-vector matrices (out row r = Σk A[r][k]·B row k,
  translation row = A.T·B + B.T); every element is one register sum of three products, rounded
  once; the sums run in a fixed order per element (written out in tools/bikephys.py
  matmul_403a80).
- **0x46d270**(M) → (a0, a1, a2): c = f32(√(1.0 − M6²)); if c ≤ f 0x4d0e50 = 1e−5: a0 = 0, a1 =
  ±π/2 (f 3fc90fdb; minus when M6 is negative), a2 = atan2(M1, M4). Else a1 = f32(atan2(M6, c)),
  a0 = f32(atan2(−M7/c, M8/c)) (quotients unrounded), a2 = f32(atan2(f32(−M3/c), f32(M0/c))).
  atan2 is the x87 fpatan.
- **0x45cb00**(obj, R by value): P = 0x403a80(R, obj matrix) (so R turns the bike about its own
  axes); P is written to obj+0xbc..0xe8; then the angles of P (0x46d270) are taken and the
  matrix is **rebuilt from them through the table**, T = 0: with A = f32(s1·c2), B = f32(s2·s1):
  M0 = c2·c1, M1 = s2·c0 + A·s0, M2 = s2·s0 − A·c0, M3 = −(s2·c1), M4 = c2·c0 − B·s0,
  M5 = c2·s0 + B·c0, M6 = s1, M7 = −(c1·s0), M8 = c1·c0.
  **Consequence: the bike's heading moves in whole steps of the table, 2π/16384 = 0.000383 rad.**
  At full lock (SPEED1, 600) the computed yaw per step is 0.0228 rad but the bike turns 57 or 58
  table steps (0.02186 / 0.02224) [run, §9]: the truncation loses one to two steps of the table
  every step. A port that keeps the heading as a float angle would turn 2–4 % faster than the
  original at full lock, more at small steering angles.
- 0x466240 calls 0x40fd40 with (0, yaw, 0) and passes the result to 0x45cb00; the wall spin in
  0x468ca0 does the same with its own angle (§7.3).

---------------------------------------------------------------------------------------------

#### 7. The contact with the track: ground, walls, air, landing

(Contact with the track (ground, walls, air, landing) — 0x468ca0 and its helpers)

Marks: [asm] read instruction by instruction, [decomp] Ghidra's text only, [guess], [test] checked
with build/phys/ground.py on the 10 shipped tracks. Conventions as build/phys/brief.md: `obj` = bike
object, `body` = obj+0x24 (offsets below are obj-relative unless written body+X), `C` = carac row,
`dt` = the step (float [0x581014]). "K[addr]" = a constant at addr, f = float, d = double.
x87: the process default control word (53-bit) is assumed [guess]; "ext" below means a value kept in
an FPU register (not rounded to float); "→f" = stored as float.

##### 7.1 The track geometry the physics uses (not the road models)

The physics never touches the road block models. Each section (0xfc bytes, array [0x52c108], count
[0x52c10c]) carries planes (n.x, n.y, n.z, d), "distance" = n·p + d:

| plane | built by | what |
|---|---|---|
| +0x40 | 0x40e5c0 [decomp] | start plane: n = normalize((0,0,1)·M) = the section's Z row, through pos (+0). Walk test. |
| +0x60 | the .trk file | left wall (inside of the road = positive side) |
| +0x70 | the .trk file | right wall |
| +0x80 | 0x40e5c0 [decomp] | road triangle 1: corners Li, Lj, Ri; n = normalize((Lj−Li)×(Ri−Li)), through Li (n.y > 0 on every track [test]) |
| +0x90 | 0x40e5c0 [decomp] | road triangle 2: corners Rj, Lj, Ri; n = normalize((Lj−Rj)×(Ri−Rj)), through Rj (n.y < 0 on every track [test]; harmless, see 0x40c8b0) |
| +0xa0 | 0x40e5c0 [decomp] | vertical split plane along the diagonal Lj–Ri: n = normalize(Ri.z−Lj.z, 0, −(Ri.x−Lj.x)), through Lj; ≥ 0 → triangle +0x80, < 0 → +0x90 |

Corners [decomp, 0x40e5c0]: M_i = the section's 3x4 (axes rows X, Y, Z at +0xc, offset +0x30), point(v) =
pos + v·M + offset (0x410010 then 0x40ffc0). Li = line(point_i(0,0,0) → point_i(−2048,0,0)) ∩ wall
+0x60 of section i; Lj = line(point_{i+1}(0) → point_{i+1}(−2048,0,0)) ∩ +0x60 of section i;
Rj = line(point_{i+1}(0) → point_{i+1}(2048,0,0)) ∩ +0x70 of i; Ri = line(pos_i + offset → point_i(2048,0,0))
∩ +0x70 of i (the intersection is the infinite line, 0x4b0f60). So **the road surface is a strip of two
flat triangles per section, spanning wall to wall**; the walls are its sides. [test] the surface passes
through every section's pos (|h(pos) − pos.y| < 3e-12 on ILE01); walls lie 200..6000 units from the
centre, outside the edges +0xf0/+0xf4 (the drawn road); one ILE11 section has its pos outside a wall.
(build/phys/ground.py rebuilds the planes and the height in Python.)

Plane builder precision is not mapped (0x40e5c0 read in decomp only); a port should build these
planes as the game does or, better, read them from the original's memory once to check. [open]

##### 7.2 Helpers [asm unless marked]

###### 0x4b10c0 plane distance (ECX = plane, arg p) → ST0
((n.y·p.y + n.z·p.z) + n.x·p.x) + d, all ext; increments the counter [0x709378] (statistics only).

###### 0x4b0f60 line ∩ plane (ECX = plane; out, A.x, A.y, A.z, B.x, B.y, B.z by value; ret 0x1c)
- If |A.x−B.x| ≤ 1e-5f and |A.y−B.y| ≤ 1e-5f and |A.z−B.z| ≤ 1e-5f (K[0x4d2090] f 0x3727c5ac): out = A.
- dy = B.y−A.y →f, dz = B.z−A.z →f, dx = B.x−A.x →f (but the denominator uses dx ext);
  den = (n.x·dx + n.y·dy) + n.z·dz →f; den == 0 → out = A.
- t = −(((n.x·A.x + n.z·A.z) + n.y·A.y) + d) / den   (ext)
- out = (dx·t + A.x, dy·t + A.y, dz·t + A.z) each →f (dx here the stored float).

###### 0x40c740 nearest section (ECX = track 0x52c108, arg p) → EAX
Best = FLT_MAX (0x7f7fffff), index −1; for every section i: L = |pos_i − p| (sqrt of the summed
squares in ext; sum order not transcribed) compared with the best (float); strictly smaller → best =
L →f, index i (first wins on ties). Used by the drawing
when there is no hint; not by the bike step.

###### 0x40c7e0 section walk (ECX = track; hint, p, float *dist_out) → EAX   (NOTES §4.13.1)
- nsec ≤ 0 → 0. hint ≥ nsec → 0.
- s = plane40(hint)·p. If s < 0: i = hint−1 (−1 → nsec−1) repeatedly until plane40(i)·p ≥ 0; return i.
- Else: repeatedly try i+1 (mod nsec) while plane40(i+1)·p ≥ 0; return the last i with ≥ 0.
- dist_out (if not NULL) = the last distance computed →f (always NULL in the bike code).
- No loop guard (the circuits are closed). Result: the section whose start plane is behind p and
  whose successor's start plane is ahead.

###### 0x40c8b0 road height (ECX = track; sec, p*) → ST0
- nsec ≤ 0 → 0.0. s = planeA0(sec)·p (ext, compared with 0.0f); s < 0 → plane +0x90, else +0x80.
- If n.y's bits & 0x7fffffff == 0 → 0.0. Else **h = −(((n.z·p.z + n.x·p.x) + d) / n.y)** (ext,
  the caller stores it →f). The sign of the plane's normal does not matter.

###### 0x40c960 point on the road below/above p (ECX = track; sec, p*, out*) (only 0x461aa0 uses it)
Same triangle choice; out = 0x4b0f60(plane, A = p, B = (p.x, p.y + 10000.0f →f, p.z)), i.e. the vertical
projection of p onto the road plane. nsec ≤ 0 → out = (0,0,0). K[0x4d02d4] = 10000.0f.

###### 0x40ca80 wall crossing (ECX = track; sec, A*, B*, hit_out*, v*, n_out*) → EAX 0/1
- If |A−B| ≤ 1e-5f (K[0x4d02c8]) per component → return 0, nothing written.
- For the left wall (+0x60), then the right wall (+0x70), in that order: if plane·B < 0 (B outside):
  hit_out = 0x4b0f60(plane, A, B); n_out = the plane's normal (copied); **v is mirrored**:
  k = (v·n) in ext as ((n.x·v.x + n.y·v.y) + n.z·v.z), then v.x = v.x − k·(2·n.x) (ext→f),
  v.y −= (k·2n.y →f), v.z −= (k·2n.z →f) (K[0x4d02d8] = 2.0f); v normalized (0x40fcb0 length, if ≠ 0
  multiply by 1/len); result = 1. With both walls crossed the second one wins hit_out and n_out and
  v is mirrored twice.

###### 0x40cd90 keep a point between the walls (ECX = track; sec, p*)
If plane60·p < 0: p = 0x4b0f60(plane60, A = section pos (+0), B = p). Then the same with +0x70.
(The point is moved back along the line from the section's centre point.)

###### 0x40ec60 ground type under p (ECX = track; sec, p.x, p.y, p.z by value) → EAX
lateral = ((d.z·Z... ) see NOTES: lateral = X·(p − pos) − X·offset with X = the section's X row,
summed as dx·X0 + ((dz·X2 − (oy·X1 + ox·X0 + oz·X2)) + dy·X1) [asm]; if edge_left(+0xf0) < lateral
< edge_right(+0xf4) (strict) → section+0xe4 (on-road ground) else +0xe8 (off-road ground).
Users: 0x465d20(obj, p, flag) = 0x40ec60(obj's section, p) (+9 if flag; 0 when no track) for the
engine sound 0x406290, and 0x466d70 (effects; ground > 0 and on the ground starts dust when the
speed ratio is high) [asm 0x466f54..]. **Not the bike's grip**: see §5.

###### 0x46c7a0(sec, n) → EAX: "the road bends ahead" [asm]
a = pos[wrap(sec + n·dir)] − pos[sec], b = pos[wrap(sec + (n·dir)/2)] − pos[sec] (dir = [0x4dc140],
−1 when racing in reverse; /2 truncates toward 0; wrap = 0x40ec30), both with y = 0 and normalized
in x/z (len = sqrt(z² + x²) ext, →f, × 1/len); d = |(b.x·a.x) + ((0) + b.z·a.z)|;
**d > 0.995 (K[0x4d1300] d) → 0 (straight) else 1**.
In 0x45d580 [asm 0x45d748]: `flag = (C.PersoAnim != 0) && 0x46c7a0(obj+0x31c1, 8) == 0` — a dirt
bike on a straight; the flag goes to 0x48d620 and to 0x468ca0.

###### 0x46d610(out, a, b): out = a × b [asm] (standard cross product, each component one ext
expression a.y·b.z − b.y·a.z etc. →f). 0x4570d0(out, a, b): out = a − b. 0x46d5e0(a, b) → ST0:
a·b summed as (b.x·a.x) + ((a.z·b.z) + (a.y·b.y)) in ext. 0x46bce0
(ECX = v; x, y, z): v = (x, y, z).

###### 0x490220 heading of a 3x4 matrix (ECX = M) → ST0 [asm]
a = row2 + T (Z + translation, →f each), b0 = row1.x + T.x, b1 = row1.y + T.y (→f);
c = sqrt(1.0) · |a| (|a| = sqrt((a1² + a2²) + a0²) →f, then c →f). If |c| < 1e-4 (K[0x4d1e50] d):
return +π/2f if a.x's bits > 0 (int compare) else −π/2f (K[0x4d1e5c]/[0x4d1e60]). Else
**return atan2((b0·a1 − a0·b1)/c, a2/c)** (fpatan; numerator summed as (−a0·b1) + b0·a1).
For a level matrix with Z = (sin h, 0, cos h) this is **−h**: 0x490220 gives minus the yaw
(yaw = atan2(Z.x, Z.z)).

###### 0x46bdc0 look-along matrix (ECX = out 3x4; from*, to*) [decomp]
Z = normalize(to − from); X = normalize(Z.z, 0, −Z.x); Y = normalize(−Z.x·Z.y, Z.z² + Z.x², −Z.z·Z.y);
rows X, Y, Z, T = 0.

###### 0x458e20(out, a*, b*) direction → angles (used by 0x461aa0) [asm]
d = b − a (→f each); L = sqrt((dy² + dz²) + dx²) ext; nx = dx/L, ny = dy/L (ext), nz = dz/L →f;
h = sqrt(nz² + nx²) (nx, ny →f first). h ≠ 0: out = (atan2(ny, h), atan2(−nx/h, nz/h), 0) (pitch, −yaw,
0) →f. h == 0: out = (+π/2 if a.y < b.y, −π/2 if a.y > b.y, else 0; 0; 0).

###### 0x40fd40 R(a, b, c) from the cosine table (NOTES §4.9.3.3); with (0, h, 0) a yaw rotation.
###### 0x45cb00(obj, R by value, 0x30 bytes) [decomp]: bike matrix (+0xbc) = 0x403a80(R, matrix),
then the angles are taken back out (0x46d270) and the matrix rebuilt from them through the table, T = 0.
(Described by the bike-step helper.)

##### 7.3 0x468ca0(obj, flag) — the contact step, called after the integration [asm]

Called from 0x45d580 as `r = 0x468ca0(obj, flag)` right after 0x45d110; **always returns 1**
[asm 0x469c49], so 0x467b30(obj, 1) and 0x466d70 always run after it. `flag` = the dirt-on-a-straight
flag of §2. Stack locals used below: `move`, `dir`, `hit`, `h`.

1. obj+0x2dc0 = obj+0x2dc4 = 0. persoAnim = C+0x120 (kept as raw int bits).
2. Histories (sub-object +0xc pos → +0x18 → +0x24): rear wheel +0x338 → +0x344 → +0x350; front
   wheel +0x1b4 → +0x1c0 → +0x1cc; +0x4bc → +0x4c8 → +0x4d4 (oldest overwritten first);
   +0x2dbc (on ground last step) = +0x2db8.
3. old = +0x31c1; **+0x31c1 = walk(old, body pos +0x30)**.
4. move = pos(+0x30) − prev pos(+0x3c) (each →f; prev pos was saved by 0x465fa0 before the
   integration); L = sqrt(((z²) + y²) + x²) (z kept ext from the subtraction, →f into L);
   dir = move × (1/L) (→f) if L ≠ 0, else dir = move.
5. (0x468e7a–0x468ef6: a copy-protection check in the championship (0x4dc094 == 2) that calls through
   0x70d98c and sets 0x4dc1fc; the "ILLEGAL VERSION" branch after it is jumped over in this build
   (`nop; jmp`) — nothing to port.)
6. **Wall test**:
   - section unchanged (old == +0x31c1): hit = 0x40ca80(+0x31c1, A = prev pos, B = pos, hit_out,
     v = dir, n_out = obj+0x31c5).
   - changed: save dir; try 0x40ca80 with the **old** section; if it hit and walk(old, hit_out) == old,
     keep that; otherwise restore dir and test the new section +0x31c1 as above.
7. **If a wall was hit** (dir is now the mirrored, normalized direction; n = obj+0x31c5):
   - pos = hit_out + n·32.0f (K[0x4d1024]; each component n·32 ext + hit →f).
   - +0x31c1 = walk(+0x31c1, pos).
   - obj+0x174..+0x17c (body+0x150, the bounce direction) = dir.
   - yaw = −0x490220(bike matrix +0xbc); **+0x2e31 (wall spin) = 0x46ad10(n, yaw)** (§4).
   - If **not on the ground last step** (+0x2dbc == 0): r = CRT rand() (0x458ce0, MSVC rand, not the
     game's stream); (r & 3) == 0 → **crash 0x467e30(obj, &move)** and +0x2dac |= 2, else +0x2dac |= 1.
     +0x180 (body+0x15c, bounce speed) = speed(+0x124) × 0.8 (K[0x4d11c0] d) →f; if < 0.5555… (K[0x4d10c0]
     d) → 0x3f0e38e4 (0.5555556f). **speed = 0**.
   - Else (on the ground): a = |((n.z·dir.z + n.x·dir.x) + n.y·dir.y)| →f (cosine of the incidence;
     the mirror keeps it).
     - a > 0.98 (K[0x4d11d0] d) and speed > 27.777… (100 km/h, K[0x4d11d8] d) → crash 0x467e30(obj, &dir), |= 2;
     - else r = rand(): ((r & 3) == 0 and a > 0.5f (bits > 0x3f000000) and speed > 22.22… (80 km/h,
       K[0x4d11e0] d)) or (turbo +0x307d ≠ 0 and speed > 5.555… (20 km/h, K[0x4d0f50] d)) → crash
       0x467e30(obj, &dir), |= 2; else |= 1.
     - speed = speed × 0.75 (K[0x4d0ff0] d) →f.
     - q = (dir.x·move.x) + ((dir.z·move.z) + (dir.y·move.y)) (ext; move = the unnormalized move).
       - q ≥ 0 (a glancing hit): +0x18c..+0x194 (body+0x168, the direction of motion) = dir;
         speed = (1.0f − a) × speed →f; speed = speed × 0.9 (K[0x4d11b8] d) →f; **0x48d2c0(body, 0.0)**;
         if speed < 0.2777… (1 km/h, K[0x4d0f70] d) → speed = 0x3e8e38e4 (0.2777778f).
       - q < 0 (head-on): s = speed (as a double); +0x180 = s × 0.5 (K[0x4d0f08] d) →f, if < 1.3888…
         (K[0x4d11e8] d) → 0x3fb1c71c (1.3888888f); speed = s × 0.3 (K[0x4d11f8] d) →f; if < 1 km/h →
         0.2777778f.
   - Then (both cases): 0x48cbe0(body, speed) (sets the body's velocity from the speed);
     0x48edd0(body) (+0x12c = speed clamped to ±body max +0xb4); +0xec = 0x48deb0(body, &+0x12c,
     gear +0x7c, +0xec) (the engine value from the wheel speed); **+0x31d1 = 200.0f** (0x43480000,
     a wall-contact timer [guess]); +0x2e85 = 0; if obj+8 && obj+4 (a driven bike): +0x31f9 = 0
     (turbo key released).
8. **0x40cd90(+0x31c1, pos)**: pos kept between the walls (always, hit or not).
9. h = 0x40c8b0(+0x31c1, pos) →f. tol = 32 (8 when flag) as an int, fild'ed.
10. **Airborne if h + tol < pos.y** (ext sum vs float):
    - +0x2db8 (on ground) = 0; driven bike: +0x31f9 = 0.
    - If speed(+0x124)'s bits & 0x7fffffff ≠ 0 — **the bike's nose follows the flight path**:
      f = normalize(+0x18c − 0) (the motion direction); X1 = (0,1,0) × f (unused but for its length);
      Z' = normalize(Z.x + T.x, f.y, Z.z + T.z) (Z = +0xd4.., T = +0xe0..; i.e. the bike's horizontal
      heading with the flight's vertical slope); X' = (0,1,0) × Z'; Y' = Z' × X'; X', Y' normalized
      (0x40fcd0); matrix rows +0xbc = X', +0xc8 = Y', +0xd4 = Z'; T (+0xe0..+0xe8) = 0.
      (A cross product d × X1 is computed and then overwritten with zeros: dead code.)
    - speed == 0: the matrix and T are left alone.
11. **Else on the ground**:
    - If it was airborne (+0x2dbc == 0) — **landing**: +0x2e95 = (prev pos.y(+0x40) − h) × −2.0f
      (K[0x4d1028]) →f (a downward kick for the suspension [guess]); speed = (float)(int)+0x118 ×
      0.2777… (K[0x4d0f70] d) →f — the speed is reset to the displayed km/h (+0x118, an int) / 3.6,
      dropping the vertical part; 0x48cbe0(body, speed); 0x48edd0(body); +0xec = 0x48deb0(…);
      +0x2dac |= 0x80.
    - **pos.y = h**; +0x2db8 = 1; +0xf0 (body+0xcc) = 0; +0x2dc0 = 1; +0x2dc4 = 1.
    - **Pitch from the road under the two axles**: kR = C.RoueAxeDistance(+0x38) × −48.0f
      (K[0x4d0e94]), kF = RoueAxeDistance × 48.0f (K[0x4d0e98]);
      R = ((kR·Z.x + T.x) + pos.x, (kR·Z.y + T.y) + h, (kR·Z.z + T.z) + pos.z),
      F likewise with kF (each component →f). **Note** the x component multiplies by k in ext (the
      register after `fst`), y and z by k rounded to float.
      If persoAnim == 0 (road bike) or flag: secF = walk(+0x31c1, F), secR = walk(+0x31c1, R); else
      both = +0x31c1 (dirt bikes on bends use the body's section for both).
      F.y = height(secF, F), R.y = height(secR, R) (→f).
      Z = normalize(F − R); X = (0,1,0) × Z; Y = Z × X; X, Y normalized; rows +0xbc = X, +0xc8 = Y,
      +0xd4 = Z; T = 0. **The ground matrix has no lean (X is horizontal)**; the lean +0x2e1d is
      applied elsewhere (drawing / other routines).
12. **Wheel positions** (always): s_R = C.HauteurCadre(+0x58)·96.0f (K[0x4d102c]) kept ext, then
    s_R = C.RoueRayonAr(+0x40)·−192.0f (K[0x4d1030]) + that →f; s_F = C.RoueRayonAv(+0x3c)·−192.0f +
    (the same ext HauteurCadre·96) →f; r_R = RoueAxeDistance·−48 (x in ext, y/z →f as above),
    r_F = RoueAxeDistance·48.
    rear (+0x338) = ((r_R·Z + s_R·Y) + T) + pos (per component); front (+0x1b4) likewise with r_F, s_F.
    I.e. the wheel centres sit at ±48·wheelbase along Z and 96·(HauteurCadre − 2·radius) along Y
    from the body point (units: 96 per metre).
13. +0x2db0 = height(walk(+0x31c1, rear), rear) →f (always). If on the ground (+0x2db8):
    front.y (+0x1b8) = height(walk(+0x31c1, front), front); rear.y (+0x33c) = +0x2db0.
14. **Wall spin** +0x2e31: if its bits & 0x7fffffff == 0 → return 1. Else
    - > 0: d = dt × 0.0235619449… (K[0x4d1208] d = 3π/400) →f; +0x2e31 = +0x2e31 − d →f; if the result
      is negative (bits > 0x80000000) → 0.
    - < 0: d = dt × −0.0235619449… (K[0x4d1210] d) →f; +0x2e31 −= d; if the result is > 0 → 0.
    - **0x45cb00(obj, R(0, d, 0))**: the bike is turned about the vertical by d (the full d, even when
      +0x2e31 was clamped). return 1.
    So a wall hit spins the bike by +0x2e31 radians at 3π/400 rad per 1/100 s (135°/s).

##### 7.4 0x46ad10(n*, yaw) → ST0: the spin a wall hit gives [asm]
(ret 8; ECX unused.)
1. nl = *n. R = R(0, yaw, 0); fwd = (0,0,1)·R + T (0x410010) = (sin yaw, 0, cos yaw) from the table.
   If fwd·nl > 0 (0x46d5e0 vs 0.0f): **n (= obj+0x31c5) = (0,0,0)** (0x46bce0) and return 0.0
   (already heading away from the wall: no spin).
2. left = (−1,0,0)·R(0, yaw, 0) + T; if left·nl < 0: nl = −nl.
3. A = 0x46bdc0(from 0, to nl); idx = ftol(yaw × 2607.594587618272 (K[0x4d0ed0] d)); c = COS[idx & 0x3fff],
   s = COS[(idx − 0x1000) & 0x3fff], C0 = COS[0] ([0x5fcb80]), C3 = COS[0x3000] ([0x608b80]);
   v = (c·C0·−100, ((1+s)·C3·C0)·−100, (C3·C3 − s·C0·C0)·−100) (K[0x4d12a8] = −100.0f; each →f) ≈
   100·(−cos yaw, 0, sin yaw) = the bike's left; B = 0x46bdc0(from 0, to v).
4. α = 0x490220(A) →f, β = 0x490220(B) →f. Each brought into [0, 2π): while < 0 add 2π
   (K[0x4d0e58] d, added in ext, stored after the loop), while ≥ 2π subtract.
   If α + π (double) < β: β −= 2π (→f). If α − π > β: β += 2π (→f).
   δ = β − α →f, brought into [0, 2π) the same way; if δ > π (K[0x4d0e60] d): δ −= 2π (→f).
   return δ: the signed yaw that turns the bike's left side onto the wall's normal (in −π..π).

##### 7.5 What is not here / findings for the other parts

- **The carac Material* triples (+0xb4..+0x11f) and MinAdherenceDerap (+0xa8..+0xb0) are never read**:
  an address grep of every instruction for 0x50932c..0x509394 (and 0x509320..0x509328) finds only the
  parser 0x42a580 [asm]. MinAdherenceWheel (+0xa4, 0x50931c) is read by the body code 0x48ea1a,
  0x48edac, 0x48f0cf, … (engine/grip helper's topic). The track's ground types (+0xe4/+0xe8) only feed
  the engine sound (0x465d20) and the effects (0x466d70). So **grip does not depend on the surface**.
- No "off-road slows you" here: off the drawn road (outside +0xf0..+0xf4) the surface is the same two
  triangles and the same physics up to the walls; any off-road drag must come from elsewhere [guess:
  none found in this part].
- The suspension values: in 0x468ca0 only +0x2e95 (landing kick) and the wheel heights (+0x1b8,
  +0x33c, +0x2db0) are produced; Ressor* (C+0x5c..+0x64) are read by 0x466a80 [asm grep], AngleSuspension
  (C+0x30) at 0x466d21, FourcheAngleArriere (C+0x6c) only by the drawer 0x46031d; +0x2e39 is decayed in
  0x45d580. (The step helper / post-step helper map those.)
- Jumps: there is no jump code in the contact: a jump is just pos.y leaving h + 32 (a crest), the nose
  following the path (step 10) and the landing (step 11). The `_N_` jump animations (list 0x580d0c,
  count 0x5cf4fc) are read only by 0x45e057.. (in 0x45d810, the rider animation) [asm grep].
- Crash start 0x467e30(obj, v*) [asm for the fields]: nothing if already crashed (+0x30c1); else
  +0x31a1..+0x31a9 = v, +0x31ad = 1, **+0x7c (gear) = 1, +0x30c1 = 1**, +0x30c5 = 400.0f; +0x30c9 = normalize(v.x,
  v.y + 0.6 (K[0x4d1180] d) →f, v.z); +0x30d5 = +0x30d9 = speed × 0.15 (K[0x4d1188] d) →f; +0x30b5 =
  pos + dir·speed (dir = +0x30c9); fall side +0x30dd = −π/2 (0xbfc90fdb) if +0x2e31 < 0, +π/2 if > 0;
  if +0x2e31 == 0 by the sign of the lean +0x2e1d; if the lean is 0 too, CRT rand() & 1 → −π/2 else +π/2;
  then +0x3085 (the rider's reference matrix) = Ra(3π/2 via table index 0x3000) composed (0x403a80) with
  R(0, −0x490220(+0x3085)·…, 0) — built inline from COS[0], COS[0x2000], COS[0x3000] [asm 0x467fb4–
  0x468172, element order not transcribed]; +0x30e1 = 0, +0x30e5 = +0x30e9 = +0x30dd; then
  0x467c40(6, pos, 15, 16, bike matrix by value) (a sound/effect [guess]). Crash progress and being
  put back on the road are not in this part (0x466d70 / 0x461aa0 / 0x4683e0).
- Random numbers: the wall crash chances and the fall side use **the C runtime's rand()** (0x4b3600
  through 0x458ce0), not the game's Rand_Next stream; a port needs MSVC's LCG (seed state shared with
  every other rand() caller) to reproduce them.

##### 7.6 Fields (obj-relative) established here

| field | what |
|---|---|
| +0x30 / +0x3c / +0x48 | body position / previous step / the one before (body+0xc, +0x18, +0x24) |
| +0xbc..+0xe8 | bike matrix rows X (+0xbc), Y (+0xc8), Z (+0xd4, forward, rear→front), T (+0xe0, set 0 here) |
| +0xec | engine value (body+0xc8), recomputed by 0x48deb0 after a hit or landing |
| +0xf0 | body+0xcc, set 0 on the ground (airborne measure [guess]) |
| +0x118 | speed as an int in km/h (the HUD) [guess from the 1/3.6] |
| +0x124 | speed in m/s-like units (km/h ÷ 3.6) (body+0x100) |
| +0x12c | body+0x108 drive speed (clamped speed) |
| +0x174..+0x17c | bounce direction (body+0x150), set to the mirrored direction on a wall hit |
| +0x180 | bounce speed (body+0x15c) |
| +0x18c..+0x194 | direction of motion (body+0x168) |
| +0x1b4 / +0x1c0 / +0x1cc | front wheel centre / previous / before |
| +0x338 / +0x344 / +0x350 | rear wheel centre / previous / before |
| +0x4bc / +0x4c8 / +0x4d4 | a third sub-object's position history (sub-object at +0x4b0) |
| +0x7c | gear (body+0x58); a crash sets 1 |
| +0x2dac | event bits this step: 1 wall scrape, 2 wall crash, 0x80 landing (cleared in 0x45d580) |
| +0x2db0 | road height under the rear wheel |
| +0x2db8 / +0x2dbc | on the ground now / last step |
| +0x2dc0, +0x2dc4 | on the ground (both set together here) |
| +0x2e31 | wall spin still to apply (radians) |
| +0x2e95 | landing kick ((prev y − h)·−2; also −32 from 0x4660f0; cleared each step in 0x45d580) |
| +0x30c1 | crashed |
| +0x31c1 | current section index |
| +0x31c5..+0x31cd | last wall normal (zeroed when the bike already heads away) |
| +0x31d1 | 200.0 after a wall hit (timer [guess]) |
| +0x31f9 | turbo key held (cleared when airborne or after a hit, driven bikes only) |

##### 7.7 Open points
- Precision of the plane builder 0x40e5c0 (decomp only) and of the x87 control word.
- What 0x48cbe0 / 0x48d2c0 do exactly with the speed (other helper).
- Where the bike is put back on the road after a crash, and the off-road behaviour (if any) outside
  this routine.
- +0x4b0 sub-object's identity; +0x31d1's reader.

##### 7.8 The section planes: 0x40e5c0 (track load, called from the .trk loader 0x40d650 at 0x40e147) [asm]

Checked bit for bit: `python3 -I tools/trkplanes.py "Moto Racer/data/ile01/ile01.trk"
build/phys/sections_speedbay.bin` builds all 788 × 4 planes of Speed Bay exactly as the running
original holds them (memory dump of the section array [0x52c108], 788 × 0xfc bytes) [test]. The
inputs in memory (pos +0, matrix +0xc, walls +0x60/+0x70) equal the file's values.

Arithmetic: the FPU's 53-bit default; "f" = stored to a float, everything else stays in a register.
Section i (0xfc bytes): pos P (+0), matrix M (+0xc: rows X, Y, Z, then T at +0x30), walls W60, W70.
j = (i + 1) mod count (idiv). For every i in order:

1. **+0x40** start plane: n = (0,0,1)·M_i + T_i through 0x410010 (each component f of
   ((M6·v.z + M3·v.y) + M0·v.x) + T; so Z row + T, not Z alone); normalised by 0x40fcd0
   (len = f(√((x²+y²)+z²)); if len ≠ 0: c = f(c · (1/len)), 1/len unrounded); plane through P_i
   (0x4b0f20: normal copied, d = f(−((n.y·p.y + n.z·p.z) + n.x·p.x))). The file's plane 0 is
   overwritten.
2. Corner points, each from a line A→B cut with a wall by 0x4b0f60:
   - point(P, M, v) = f(P + f(v·M + T)) per component (0x410010 then 0x40ffc0).
   - Li = W60_i ∩ line(point(P_i, M_i, 0) → point(P_i, M_i, (−2048,0,0)))
   - Lj = W60_i ∩ line(point(P_j, M_j, 0) → point(P_j, M_j, (−2048,0,0)))  (next section's
     line, this section's wall)
   - Rj = W70_i ∩ line(point(P_j, M_j, 0) → point(P_j, M_j, (2048,0,0)))
   - Ri = W70_i ∩ line(f(P_i + T_i) → point(P_i, M_i, (2048,0,0)))
     (A for Ri adds T directly instead of 0·M + T; the same value.)
   - 0x4b0f60(plane; A, B): if |A−B| ≤ 1e-5f (float 0x4d2090) on all three axes → A. Else
     dx = B.x − A.x kept unrounded, dy = f(B.y − A.y), dz = f(B.z − A.z), dx32 = f(dx);
     den = f((n.x·dx + n.y·dy) + n.z·dz) (dx unrounded here!); den == 0 → A;
     t = −((((n.x·A.x + n.z·A.z) + n.y·A.y) + d) / den);
     result = (f(dx32·t + A.x), f(dy·t + A.y), f(dz·t + A.z)).
3. **+0xa0** split plane: n = (f(Ri.z − Lj.z), 0, f(−(Ri.x − Lj.x))), normalised as above (the
   inline copy of 0x40fcb0 + the 1/len multiply), through Lj.
4. **+0x80** triangle Li, Lj, Ri: a = f(Lj − Li), b = f(Ri − Li) per component;
   n = (f(a.y·b.z − a.z·b.y), f(a.z·b.x − a.x·b.z), f(a.x·b.y − a.y·b.x)); normalised; through Li.
5. **+0x90** triangle Rj, Lj, Ri: c = f(Ri − Rj), d = f(Lj − Rj); n = d × c (same form as 4,
   a = d, b = c); normalised; through Rj.

Nothing else of the section is written (the only stores outside the stack frame are the four
0x4b0f20 calls on +0x40, +0xa0, +0x80, +0x90).

Pitfalls (each one changes bits): T is part of the +0x40 normal; in 0x4b0f60 the denominator
uses the unrounded dx but the result uses the rounded dx, while dy/dz are rounded for both; the
lengths are rounded to float before 1/len; the products and differences of the cross product
are unrounded, only the three components are stored. objdump's `fsubp st(2),st` /
`fsubrp st(1),st` read literally as Intel's (ST(i) = ST(i) − ST(0) / ST(0) − ST(i)) give the
signs that match memory (n80.y > 0, n90.y < 0).

---------------------------------------------------------------------------------------------

#### 8. After the contact: suspension, lean, crash, collisions, effects, HUD values

(After the ground contact: what 0x45d580 does from 0x466660 on (and the crash, collision and)
(effect routines it reaches))

Marks: [asm] read in the machine code, [decomp]
Ghidra's text only, [guess]. Field names `obj+X` (obj = bike object, player 1 0x584421, rider
number n (1-based) at 0x581218 + n*0x3209, so `obj+0x2dd0` = n); `body+Y` = obj+0x24+Y.
`C+X` = carac row (brief.md). `dt` = the step (global 0x581014, ms*0.1). Constants are given as
value and bits; **F** = read as a float (DWORD), **D** = read as a double (QWORD). x87 at the
process default (53-bit mantissa) [guess, no fldcw in the program].

Units found on the way [asm]: positions are **1/128 m** (`|dpos|/dt * 0.78125` gives m/s:
1000/1280; the km/h counter is `units/s * 0.028125` = 3.6/128); **speed obj+0x124 = body+0x100
is in m/s** (22.222 = 80 km/h, 18.0556 = 65 km/h, 5.5556 = 20 km/h, 0.27778 = 1 km/h are the
thresholds the code uses); dt = ms*0.1, so a timer of 400 dt-units = 4000 ms.

Helpers used below [asm unless noted]:
- `M*v` = 0x410010(out, v, M) (stdcall, 3x4 row-vector form, M = 12 floats X,Y,Z rows + T):
  out.x = ((M6*vz + M3*vy) + M0*vx) + M9; out.y = ((M1*vx + M4*vy) + M7*vz) + M10;
  out.z = ((M8*vz + M2*vx) + M5*vy) + M11, each rounded to float on the store.
- `LOOKAT(a,b)` = 0x46bdc0(out, a, b): F = b-a (floats), L = |F| (0x40fcb0, 80-bit, stored F);
  if L != 0: r = 1/L (80-bit), F *= r (each stored). nx = -F.x (F). Y = (nx*F.y, F.z*F.z - nx*F.x,
  -(F.z*F.y)); X = (F.z, 0, nx); |X| = sqrt(F.z*F.z + nx*nx), if != 0 X.x = F.z*(1/|X|),
  X.z = nx*(1/|X|); |Y| = sqrt((Y.y^2 + Y.z^2) + Y.x^2), if != 0 Y *= 1/|Y|. out = rows X, Y, F,
  T = 0.
- `YAW(M)` = 0x490220(M), returns ST0: z = Zrow+T (3 floats), y0 = Y0+T0, y1 = Y1+T1 (floats);
  m = sqrt(D 1.0 [0x4d1e48]) * f32(sqrt((z1^2 + z2^2) + z0^2)), stored F; if |m| < D 1e-4
  [0x4d1e50] return z0 > 0 (int bits) ? F +pi/2 [0x4d1e5c, 3fc90fdb] : F -pi/2 [0x4d1e60];
  else atan2((y0*z1 + (-z0)*y1)/m, z2/m) (fpatan, 80-bit) — the heading of the Z row.
- `PITCHF(obj)` = 0x45ca00(obj): the same on the frame matrix obj+0x2e55 (X row +0x2e55,
  Z row +0x2e6d, T +0x2e79): z = Z+T, x0 = X0+T0, x2 = X2+T2; h = f32(z2^2 + z0^2);
  m = f32(sqrt(h) * f32(sqrt(z0^2 + (z1^2 + z2^2)))); |m| < D 1e-4 [0x4d0eb8] -> z1 > 0 ?
  F pi/2 [0x4d0ec0] : F -pi/2 [0x4d0ec4]; else atan2((z2*z1*x0 + (-(z0*z1))*x2)/m, h/m).
  It is the up/down angle of the frame's forward axis.
- `WRAPPI(a)` = 0x458da0(float a) -> ST0: while a < D -pi [0x4d0e18] add D 2pi [0x4d0e20] (the
  loop runs in 80-bit, one store at the end), then while a > D pi [0x4d0e28] subtract 2pi.
- `WRAP2PI_GE(a)`: if the bits of a are > 0x80000000 (negative, -0.0 excluded): add D 2pi
  [0x4d0e58] in 80-bit until >= F 0 [0x4d0e54], store F; then if a >= 2pi: subtract 2pi in 80-bit
  until < 2pi, store. `WRAP2PI_GT`: the same but the second loop runs while a > 2pi. Inline
  everywhere below; which form is used is said each time (test ah,1 vs test ah,0x41).
- `SECWALK(hint, p)` = 0x40c7e0 (track 0x52c108), `ROADY(sec, p)` = 0x40c8b0 -> ST0,
  `0x40ca80` = the edge/wall test (ground fork's), `0x40ec30` = wrap a section index (EAX).
- `RAND1()` = 0x458cd0 = Rand_Next(1) (the game's stream 1); `CRAND()` = 0x458ce0 = the C
  library's rand() (msvcrt), NOT the game's streams.
- COS[] = 0x5fcb80 (16384 floats); angle -> index = ftol(a * D 2607.594587618272 [0x4d0ed0])
  (truncation), cos = COS[n & 0x3fff], sin = COS[(n - 0x1000) & 0x3fff]. COS[0] = 1,
  COS[0x2000] (0x604b80) = -1, COS[0x3000] (0x608b80) ~ -1.8e-16. Several routines below build
  an Euler rotation inline from these (two of the three angles 0); the exact x87 order is at the
  listed addresses and is the same builder as the menu's (NOTES §4.15.1) [asm located].

---------------------------------------------------------------------------------------------

##### 8.0 Order inside 0x45d580 (for reference) [asm call order, decomp]
... 0x48d620 (integrate) ; 0x45d110 ; r = 0x468ca0(obj, flat) (ground; r != 0 = wheels on the
road, see the ground section) ; **0x466660(obj, keys)** (body fork) ; **0x4667e0** ; **0x466940** ;
**0x466a80** ; **0x461aa0** ; **0x469c60** ; **0x4683e0** ; **0x467b30(obj, r)** ; if r:
**0x466d70(obj, keys)** ; **0x467930** ; obj+0x31d9 = **0x46d340(obj)** ; obj+0x2dc8 = keys.
Before the integration: **0x4660f0(obj, dt)** (turbo wheelie, when not in the countdown) and
`flat = dirt && 0x46c7a0(obj+0x31c1, 8) == 0`.
After all bikes are stepped, Game_Frame calls the race object's slot +0x18 =
**0x47ecd0(dt)**: bike-against-bike collisions (§11). 0x467e30 (crash start) is called from
0x468ca0 (3 places), 0x47ecd0 (bikes) and the ghost (0x47df40).

##### 8.1 0x46c7a0(int sec, int n) -> EAX: "is the road bending ahead" [asm]
A = 0x40ec30(n*[0x4dc140] + sec), B = 0x40ec30((n*[0x4dc140])/2 + sec) (C division, toward 0;
0x4dc140 = +1, or -1 when the track runs reversed). Section pos at track[s]+0 (0xfc-byte records,
array [0x52c108]).
a = pos[A] - pos[sec], b = pos[B] - pos[sec] (x and z only, each stored F; y set to 0).
a /= |a|xz, b /= |b|xz (each only if the length != F 0; r = fld1/len, products stored).
c = f32((by*ay + bz*az) + bx*ax) with the y terms 0; return |c| > D 0.995 [0x4d1300] ? 0 : 1.
0x45d580 passes (obj+0x31c1 = the bike's section, 8): `flat` = 1 for a **dirt** bike (C+0x120)
when the road 8 sections ahead is straight within 0.995; it goes to 0x48d620 and 0x468ca0.

##### 8.2 0x4667e0(obj): forward acceleration -> frame dive [asm]
fwd = 0x48ca10(body) (the velocity's component along the body's Z row, body fork), prev =
obj+0x2da8 (saved by 0x465fa0 at the start of the step).
- obj+0xf8 = f32((fwd - prev) / (dt * D 0.01 [0x4d0ed8]))   (forward acceleration)
- s = ((fwd2 - prev) + obj+0xf4) * F 0.5 [0x4d1018] (fwd2 = a second call, same value), kept in
  80-bit for the test, stored F as s.
- if s (80-bit) != obj+0xf4: if s != 0 (bits): v = f32(s * D -0.09817477042468103 [0x4d1108]
  = -pi/32); if |obj+0x2e39| < |v| -> obj+0x2e39 = v. Then if obj+0x2e39 > D pi/16 [0x4d1110]
  -> 0x3e490fdb; if < D -pi/16 [0x4d10f8] -> 0xbe490fdb.
- obj+0xf4 = s.
- rear wheel sub-object +0xcc (obj+0x3f8, "time off the ground"): 0 if obj+0x2dc0 (rear wheel
  touching) else += f32(dt * D 6.272 [0x4d1120]); front (obj+0x274) the same with obj+0x2dc4.
obj+0x2e39 = **frame dive** (decays by 0.01 a step toward 0 at the top of 0x45d580; braking
gives s < 0 -> a positive kick).

##### 8.3 0x466940(obj): wheel spin angles [asm]
Skipped when (obj+8 && keys(obj+0x31e9) bit1) — a driven bike braking locks both wheels — or
(!obj+8 && the previous keys obj+0x2dc8 bit1).
f = (obj+0xe0 + obj+0xd4, obj+0xe4 + obj+0xd8, obj+0xe8 + obj+0xdc) (Z row + T, each F).
dr = rear wheel pos obj+0x338 - previous obj+0x344 (x, y, z, 80-bit, not stored);
obj+0x2e3d += ((dr.z*f.z + dr.y*f.y) + dr.x*f.x) / ((C+0x40 * dt) * F 96 [0x4d102c]).
df = front obj+0x1b4 - previous obj+0x1c0; obj+0x2e49 += ((df.x*f.x + df.y*f.y) + df.z*f.z) /
((C+0x3c * dt) * F 96). (RoueRayonAr / RoueRayonAv.) One store each.

##### 8.4 0x466a80(obj): rear suspension and the frame pitch [asm]
d = dt clamped to [3, 5] by integer compares of the float bits (> 5.0 -> 5.0; < 3.0 -> 3.0).
1. obj+0x2e9d (spring amplitude) = f32(C+0x5c RessorAmplitudeDec * obj+0x2e9d).
2. obj+0x2e95 (spring push) = f32(d * D 5.184 [0x4d1128] + obj+0x2e95); if > 0 (int) -> 0.
3. k = f32(obj+0x2e95 / d).
4. obj+0x2e99 (phase) = f32(obj+0x2e9d / C+0x64 RessorAmplitudeFactor + obj+0x2e99).
5. if obj+0x2e9d > k: obj+0x2e99 = 0x3fc90fdb (pi/2); obj+0x2e9d = f32((k - obj+0x2e9d) *
   F 0.5 + obj+0x2e9d).
6. if C+0x60 RessorAmplitudeMin > obj+0x2e9d -> obj+0x2e9d = C+0x60.
7. obj+0x2e8d = 0; obj+0x2e91 (**rear suspension**) = f32(fsin(obj+0x2e99) * obj+0x2e9d).
8. a0 = f32((PITCHF(obj) - obj+0x2ea5) + obj+0x2e39) — kept as a0; a = WRAP2PI_GT(a0) (the
   second loop runs while a > 2pi).
9. p = obj+0x2ea1 (smoothed pitch): while bits > 0x80000000: p += 2pi (stored each time);
   while p > 2pi: p -= 2pi (stored each time).
10. diff = f32(a - p); if |diff| > D pi: A = (double)p; if A + pi < (double)a -> a = f32(a - 2pi);
    then if A - pi > (double)a -> a = f32(a + 2pi); diff = f32(a - p).
11. obj+0x2ea1 = f32(diff / C+0x20 RetardCadre + p).
12. obj+0x2e8d (**pitch**) = f32(obj+0x2ea1 - a0); = WRAPPI(...); if > C+0x30 AngleSuspension
    -> C+0x30; if < -C+0x30 -> -C+0x30.
Writers of the inputs: obj+0x2e95 is zeroed at the top of 0x45d580, set to -32.0 (0xc2000000) by
0x4660f0 when a wheelie ends, and by 0x468ca0 at 0x469470 (landing); obj+0x2ea5 by 0x45d580
and 0x466660 (body fork); obj+0x2e39 above.

##### 8.5 0x4660f0(obj, dt): the turbo wheelie (action 6) [asm]
Inputs: obj+0x31f9 = action-6 key held (keyboard handler); 0x45d580 copies it into obj+0x2e85
for a human bike (obj+8 && obj+4) every step before calling this (when not in the countdown).
- crashed (obj+0x30c1): obj+0x307d = obj+0x3081 = obj+0x2e85 = 0; obj+0xb8 = 1.0; human ->
  obj+0x31f9 = 0. Return.
- else if obj+0x2e85 && obj+0x3081 < 200.0 (int compare with 0x43480000): obj+0x307d (wheelie
  angle) = f32(dt * D 0.0471238898038469 [0x4d10a8] + obj+0x307d); if > D pi/2 [0x4d0eb0] ->
  0x3fc90fdb; obj+0x3081 (turbo time) = f32(obj+0x3081 + dt).
- else if obj+0x307d > 0 (int): obj+0x307d = f32(dt * D -0.025132741228718346 [0x4d10b0] +
  obj+0x307d); if it went negative: obj+0x2e95 = -32.0, obj+0x307d = 0.
- then obj+0xb8 (power factor, read by the engine) = obj+0x307d != 0 ? ([0x4dc1f8] ? 2.0 : 1.2
  (0x3f99999a)) : 1.0; and if obj+0x307d == 0 and !obj+0x2e85: obj+0x3081 = 0.
So: holding the key lifts the front wheel (pi/2 in 33 dt-units = 0.33 s) and multiplies the
power by 1.2 for at most 200 dt-units (2 s); it can be used again only after the key is released
and the wheel is down. [0x4dc1f8] is set by 0x45d110 (body fork). obj+0x2e85 is also cleared by
0x465da0 when braking (bit1) and by 0x466220 in a bike collision.

##### 8.6 0x469c60(obj): the frame matrix, the wheel points and the lean [asm]
Called from 0x45d580, 0x46bf80 (computer bikes) and twice by 0x47ecd0 after a collision.
dirt = C+0x120.
1. s = 0; if obj+0x31d1 (collision timer) != 0: obj+0x31d1 = f32(obj+0x31d1 - dt), < 0 -> 0;
   s = f32(obj+0x31d1 * F 0.005 [0x4d100c, 3ba3d70a]).
2. w = f32(F 1.0 - (obj+0x124 / obj+0xb4) * F 8.0 [0x4d1020]); w < 0 -> 0; if s > w: w = s.
   (obj+0xb4 = C+0x48 RealSpeedMax, +2.0 for a manual box.) w = 1 below 1/8 of the top speed.
3. U = M*(0,0,1) with M = the bike matrix obj+0xbc (= Z row + T).
   D = obj+0x18c..+0x194 (body+0x168: the travel direction, body fork); if D.U < 0 (0x46d5e0,
   80-bit) D = -D.
4. mix = U*w + D*(1-w) (0x46d540 scales, stored; 0x40ffc0 adds, stored);
   Mt = LOOKAT((0,0,0), mix) (local; rows X, Y, Z, T = 0).
5. **Road bike** (dirt == 0): with a = C+0x58*F 96 (80-bit, not stored):
   br = f32(C+0x40 * F -192 [0x4d1030] + a), cr = f32(C+0x38 * F -48 [0x4d0e94]);
   rear obj+0x338 = ((cr*Z + br*Y) + T) + obj+0x30 per component (Z, Y, T rows of Mt);
   bf = f32(C+0x3c * F -192 + a), cf = f32(C+0x38 * F 48 [0x4d0e98]);
   front obj+0x1b4 = ((cf*Z + bf*Y) + T) + obj+0x30.
   secF = SECWALK(obj+0x31c1, front), secR = SECWALK(obj+0x31c1, rear);
   front.y = ROADY(secF, front); rear.y = ROADY(secR, rear) (stored F). (The section hint is not
   updated here.)
   If the wheelie angle obj+0x307d > 0: d = front - rear (stored); p = Mt^-1 * d (Mt transposed,
   translation -(T.X), -(T.Y), -(T.Z) = 0); th = f32(fsin(obj+0x307d) * D -pi/4 [0x4d1218]);
   n = ftol(th * 2607.594587618272); p' = R(n) p, the inline Euler rotation about the local X
   axis by th (with COS[0], COS[0x3000] for the other two angles; asm 0x46a343..0x46a4fe);
   front = Mt * p' + rear.
   Frame: F = normalize(front - rear) (0x40fcd0); X = (0,1,0) x F; Y = F x X (0x46d610 cross:
   out = (a.y*b.z - b.y*a.z, b.x*a.z - b.z*a.x, b.y*a.x - b.x*a.y)); normalize X, Y;
   obj+0x2e55 = X, +0x2e61 = Y, +0x2e6d = F, +0x2e79..+0x2e81 = 0.
6. **Dirt bike**: the wheel points are not recomputed (they come from 0x468ca0); if the wheelie
   angle > 0 the front point is rotated about the rear one: q = M^-1 rotated (0x40fe80) * (front -
   rear) (0x410010), angles (f32(fsin(obj+0x307d) * D -pi/4), 0, 0) -> matrix 0x40fd40 -> q' -> back
   through M (0x410010 with the copy), front = rear + q'. Then obj+0x2e55 = LOOKAT(rear, front)
   (0x403a20 copy).
7. **Lean obj+0x2e1d** (both kinds). A = YAW(Mt), B = YAW(obj+0xbc) (each stored F).
   - **Human dirt bike** (obj+8 && obj+4 && dirt):
     A = WRAP2PI_GE(A); B = WRAP2PI_GE(B); if (double)A + pi < (double)B: B = f32(B - 2pi);
     if (double)A - pi > (double)B: B = f32(B + 2pi);
     e = B - A (80-bit, compared with 0 then stored F); WRAP2PI_GE(e) (the add loop is entered on
     the 80-bit sign); then again: bits negative -> add loop; >= 2pi -> subtract loop; if e > D pi:
     e = f32(e - 2pi).
     v = obj+0x124 limited to 50.0 (int compare 0x42480000).
     L = f32(atan((-e / [0x4dc150]) * v * D 1.445912045170292 [0x4d1220])) (fchs, fdiv the float
     global, fmul v, fmul D, fpatan with 1.0).
     if L > D 1.04 [0x4d1228] -> 0x3f851eb8 (1.04f); else if L < D -1.04 [0x4d1238] ->
     0xbf851eb8.
     k = [0x4dc150] * D 0.02 [0x4d1248], if k > D 1.0 -> 1.0, stored F.
     obj+0x2e1d = f32((L - obj+0x2e21) * k + obj+0x2e21); obj+0x2e21 = obj+0x2e1d.
   - **Otherwise**: A, B wrapped and paired as above; e = f32((B - A) * (1 - w)) (stored),
     WRAP2PI_GE; e2 = f32(e * F 2.0 [0x4d0f60]), WRAP2PI_GE; if e2 > D pi: e2 = f32(e2 - 2pi);
     clamp: > D pi/3 [0x4d1250] -> 0x3f860a92, < D -pi/3 [0x4d1260] -> 0xbf860a92.
     - dirt (a computer dirt bike): obj+0x2e1d = f32((e2 / (D 3.0 [0x4d1278] - (obj+0x124 *
       D 0.045 [0x4d1270]) * D 0.5 [0x4d0f08])) * D -1.3 [0x4d1280]) (one 80-bit chain).
     - road, not human (obj+8 == 0 or obj+4 == 0) or a replay/demo (0x4e9834 == 1 or 2):
       obj+0x2e1d = -e2.
     - road, human: f = [0x4dc150], but 20.0 if obj+0x2e29 == 0 and [0x4dc150] < 20.0 (int
       compare 0x41a00000); L = f32((e2 / f) * F -30 [0x4d1288]); reloaded: if L > D 0.3
       [0x4d11f8]: L = f32(L * D 1.4 [0x4d1290] - D 0.11999999999999997 [0x4d1298, 3fbeb851eb851eb6]);
       else if L < D -0.3 [0x4d12a0]: L = f32(L * 1.4 + 0.11999999999999997); then clamp
       +-1.04 to +-1.04f as above.
   **[0x4dc150] is the frame time in ms smoothed by 1/8** (0x4120e0: `x += (ms - x) * D 0.125`
   every frame before the 200/0 clamps, the recorded ms in a replay at 0x41180f): the lean of a
   human bike depends on it, so the port must keep it (a steady 60 Hz port: 16.667 limit, the
   original under timeGetTime alternates 16/17). obj+0x2e29 is written by 0x466240 (steering,
   body fork).
   Meaning: the lean is the angle between where the bike points (body matrix) and where it
   travels (Mt, the velocity blended in above 1/8 of top speed), scaled by speed.

##### 8.7 0x4683e0(obj): the crash tumble [asm]
Only while obj+0x30c1. Every step:
1. obj+0x30c5 (timer) = f32(obj+0x30c5 - dt); if negative -> **0x4681d0(obj)** and return.
2. k = dt * F 0.01 [0x4d1004, 3c23d70a] (80-bit; kept stored as F in a local and reloaded).
   step = f32(k * obj+0x30d5 * D 128.0 [0x4d0ef8]); P = obj+0x30b5 (the tumbling body's point),
   old = P; P += D * step (D = obj+0x30c9 unit direction; 0x46d540 then 0x46d510, floats).
3. sec = SECWALK(obj+0x31c1, P); if 0x40ca80(track, sec, &old, &P, &hit, &D, &out) != 0 (an edge
   or wall was crossed; D is passed by pointer [guess: reflected]): P = hit; sec =
   SECWALK(obj+0x31c1, P); obj+0x30d5 = f32(obj+0x30d5 * D 0.5).
4. g = ftol(ROADY(sec, P)) (truncated to an int).
   - **P.y < g + 8** (hit the ground): P.y = (float)g; D.y = f32(-D.y * D 0.01); normalize D;
     obj+0x3085 (tumble matrix) = R(-pi/2, 0, 0) x Ryaw(-YAW(obj+0x3085)) (inline Euler builders,
     0x40fd40, product 0x403a80; asm 0x468570..0x46869a) — the bike laid flat at its heading;
     obj+0x30e9 = 0; obj+0x30d5 = f32(k * D -4.166666666666667 [0x4d1190] + obj+0x30d5), < 0 ->
     0; if D.y > D 0.05 [0x4d1198]: debris 0x467c40(4, P, 7, 4, obj+0x3085) and obj+0x2dac |=
     0x40 (sound event).
   - **else** (in the air): D' = (D.x*s, D.y*s + dt * D -0.0981 [0x4d11a0], D.z*s) with
     s = obj+0x30d5 (each stored F); s' = f32(sqrt((D'z^2 + D'x^2) + D'y^2)); r = D 1.0 / s';
     D = D'*r (stored); obj+0x30d5 = f32(s' + k * D -0.5555555555555556 [0x4d11a8]), < 0 -> 0.
5. f = (obj+0x30d5 / obj+0x30d9) * k; angles a = f32(obj+0x30e1*f), b = f32(obj+0x30e5*f),
   c = f32(obj+0x30e9*f); R = the Euler rotation of (a, b, c) from COS[] (asm 0x468821..);
   obj+0x3085 (3x4) = obj+0x3085 x R (each row times R, translation row too).
6. If obj+0x30d5 < D 0.2777777777777778 [0x4d0f70] **and** obj+0x30c5 < 100.0 (int compare
   0x42c80000): **0x4681d0(obj)**. Else: computer bike (obj+8 == 0): obj+0xec *= D 0.9,
   obj+0x1a4 *= D 0.9; driven bike: obj+0x124 = f32(obj+0x124 * D 0.9 [0x4d11b8]);
   0x48cbe0(body, obj+0x124); 0x48edd0(body); obj+0xec = 0x48deb0(body, &obj+0x12c,
   obj+0x7c, obj+0xec); obj+0x180 *= D 0.9.
7. obj+0x319d = 1.
So a crash lasts 300..400 dt-units (3..4 s): the timer starts at 400; it ends early only after
3 s when the tumble has nearly stopped. The bike body itself (obj+0x30, the integrator's) is not
moved by the tumble: 0x45d580 skips the throttle (0x465da0) and steering (0x466240) while
crashed and the speed decays by 0.9 a step here; the tumbling picture is obj+0x30b5 with the
matrix obj+0x3085.

##### 8.8 0x467e30(obj, float *v): crash start [asm]
Nothing if already crashed. obj+0x31a1..+0x31a9 = v; obj+0x31ad = 1; obj+0x7c (gear) = 1;
obj+0x30c1 = 1; obj+0x30c5 = 400.0 (0x43c80000).
D (obj+0x30c9) = (v.x, f32(v.y + D 0.6 [0x4d1180]), v.z), normalized (0x40fcd0).
obj+0x30d5 = obj+0x30d9 = f32(obj+0x124 * D 0.15 [0x4d1188]) (tumble speed and its start).
obj+0x30b5 = (D.x*obj+0x124 + obj+0x30, D.y*obj+0x124 + obj+0x34, obj+0x124*D.z + obj+0x38)
(the full speed, not 0.15 of it).
Roll side obj+0x30dd: if obj+0x2e31 (written by 0x468ca0) != 0: its sign (negative -> -pi/2
0xbfc90fdb, positive -> +pi/2 0x3fc90fdb); else if the lean obj+0x2e1d != 0: its sign; else
**CRAND() & 1** (odd -> -pi/2, even -> +pi/2).
obj+0x3085 = R(pi-based constant matrix with COS[0x2000]) x R(YAW(obj+0x3085)) (inline Euler
builders, product 0x403a80, asm 0x467fb4..0x46816e); obj+0x30e1 = 0, obj+0x30e5 = obj+0x30e9 =
obj+0x30dd (roll rates). Debris: 0x467c40(6, obj+0x30, 0xf, 0x10, obj+0xbc).

##### 8.9 0x4681d0(obj): end of a crash ("put back") [decomp, asm-checked flow]
With V = [0x508834] (the rider the camera follows, 1 in Solo) and obj+0x2dd0 = this rider's
number:
- driven bike (obj+8), obj+0x2dd0 == V: obj+0x180 = 0, obj+0x124 = 0, 0x48cbe0(body, 0.0),
  0x48edd0(body), obj+0xec = 0x48deb0(body, &obj+0x12c, obj+0x7c, obj+0xec); obj+0x30c5 = 0,
  obj+0x30c1 = 0; [0x4ea510] = 1 (the chase camera snaps, 0x46edb9); then every other loaded
  bike still crashed (obj+0x30c1) gets 0x4681d0 too (recursively).
- driven bike, not V: the same reset, but only if rider V is not crashed itself.
- computer bike (obj+8 == 0): obj+0xec = obj+0x1a4 = 0, obj+0x30c5 = obj+0x30c1 = 0,
  obj+0x31d1 = 200.0, [0x4ea510] = 1; if it is V, the other crashed bikes too; if not V, only when
  V is not crashed.
So **other bikes stay down while the followed rider is down** (their timer goes negative and
0x4681d0 is retried every step), and all get up together. Nothing moves the bike: it restarts at
standstill where its body slid to (the body was never displaced by the tumble). Keeping it on the
road is the integrator's and 0x468ca0's job.

##### 8.10 0x467c40(n, x, y, z, kind, mask, matrix M by value): debris pieces [decomp, asm-located]
Draws RAND1() three times (r1, r2, r3) first, then for each of n pieces: r4 (bit0 -> size), r5,
r6, r7 (& 3 each -> offsets a, b, c in M's frame), r8 (& mask) -> 0x457620((r8 & mask) + kind,
x, y + F 32 [0x4d10a0], z, velocity = M rows times (b, a, c) + T, spin = (|r| & 0x1f) * F
[0x4d1150] of r3, r2, r1, size ((r4&1)*5+5)*8, 0, colour 0x45a2e0(4,4,4) = 0x2104, 5).
**Stream 1 is drawn 3 + 5n times** (crash start 33, ground bounce 23). 0x457620 takes the first
free particle of 0x52c498 + i*0x104 (count to 0x534794) and initialises it (0x4afe90, no random).

##### 8.11 0x47ecd0(this, dt): bike against bike [decomp; constants and widths asm]
Only when the game mode 0x4dc094 != 0 (not in Practice). For each pair (i, j), riders 1..N-1
with i < j (N = [0x509bac]), both loaded (+0x68), not handled yet this call (a 25x25 table), and
**0x46c9f0(sec_i, sec_j) < 4** (sec_j - sec_i, plus nsec when negative: only when j is 0..3
sections ahead of i — a pair with j behind i is never tested [decomp]):
1. t from the swept test 0x411f00(prev_i obj+0x3c, prev_j, pos_i obj+0x30, pos_j, radii F 48,
   F 48) [asm]: relative start d0 = prev_i - prev_j, motion dv = (pos_i - prev_i) - (pos_j -
   prev_j); returns the first root of |d0 + t dv| = 96 (quadratic, F 2.0 / F 4.0 constants,
   rejected when |dv|^2 < D 1e-5 [0x4d0388] or the discriminant < 0); accepted when
   D 0.05 [0x4d1988] <= t <= 1.0.
2. For each: d = pos - prev, s = |d| / dt * D 0.78125 [0x4d1990] (m/s), n = d/|d|.
   A = the faster (s_i > s_j keeps i as A), B = the other; sA, nA, sB, nB.
3. hard (crash allowed for A): |sA - sB| > 25.0 (int compare of the bits), or (> D 12.5
   [0x4d19a0] and |lean| of A or B > D 0.6981317 (40 deg) [0x4d19a8]), or a wheelie on A or B
   (+0x307d != 0). fall (crash allowed in a head-on): sA or sB > D 22.222 [0x4d19b0] or a wheelie.
   If (A or B is in the air, +0x2db8 == 0) and (A or B has +8 != 0): hard = fall = 1.
   **CRAND() & 3 != 0 -> hard = 0** (3 times in 4 a side hit does not crash).
   headon = nA.nB < 0.
4. Both moved back to the contact: pos = (pos - prev)*t + prev - n (own direction, one unit);
   their wheel points recomputed from the matrix (rear = Y*(C+0x40 * F -96) + Z*(C+0x38 * F -48)
   + T + pos; front = Y*(C+0x3c * F -96) + Z*(C+0x38 * F 48) + T + pos).
5. A: obj+0x31d1 = 200.0; 0x466220 (turbo off). Driven A (+8): side hit: hard ? (clamp
   obj+0x124 and sB to 22.222, 0x467e30(A, nA), flags +0x2dac |= 8) : flags |= 4; obj+0x124 =
   sB * D 0.95 [0x4d19c8]. Head-on: obj+0x174 = nB, obj+0x180 = 5.5555553 (0x40b1c71c); fall ?
   (crash with nA, |= 0x20) : |= 0x10; obj+0x124 = 0. Then 0x48cbe0(body, obj+0x124), 0x48edd0,
   obj+0xec = 0x48deb0(...). Computer A: the same choices with obj+0x124 = sB * D 0.9 (side) or
   5.5555553 and obj+0x18c = nB (head-on); obj+0xec = obj+0x1a4 = obj+0x124.
6. B: 0x466220. Driven B: side: obj+0x174 = nA (normalized), obj+0x180 = f32((double)sA *
   D 1.1 [0x4d1940] - obj+0x124), negative -> 0, flags |= 4 (no crash for B on a side hit);
   head-on: obj+0x174 = normalize(sA*nA + sB*nB), fall ? crash(B, nB) |= 0x20 : |= 0x10;
   obj+0x180 = sA - sB, < D 5.5556 [0x4d19d0] -> 5.5555553; obj+0x124 = 0; 0x48cbe0(body, 0),
   0x48edd0, 0x48deb0. Computer B: obj+0x18c = normalize(sA*nA + sB*nB); side: hard ?
   crash(B, that) |= 8 : |= 4, obj+0x124 = sA; head-on: crash/0x20 or 0x10, obj+0x124 = sA - sB;
   then obj+0x124 *= D 1.1, < 5.5556 -> 5.5555553; obj+0xec = obj+0x1a4 = obj+0x124.
   B: obj+0x31d1 = 200.0.
7. 0x469c60(A), 0x469c60(B). (Link play then sends the hit, 0x456bf0.)
obj+0x174..+0x180 = body+0x150..+0x15c: the push direction and size the integrator adds and
decays (body fork, 0x48d620). obj+0x2dac bits 4/8/0x10/0x20/0x40: sound events [guess].

##### 8.12 0x466d70(obj, keys): dust, spray, skid marks [decomp; asm for the window and the draws]
(At its start a check that would print "ILLEGAL VERSION" is jumped over: `jmp 0x466ea8` at
0x466d86, GOG's patch [asm].) Only for a bike within sections [s-10, s+30) of rider V's section
s (0x40ec30 wraps both ends; wrapped window handled) [asm]:
- ground material m = 0x40ec60(obj+0x31c1, obj+0x30) (on the road: section +0xe4, else +0xe8).
  If on the ground (obj+0x2db8) and m > 0: q = ftol((obj+0x124 / obj+0xb4) * F 64 [0x4d1060]);
  if q > 4: spray 0x4576a0((q + 2*ftol(...))/3, ...) while [0x4ea508] < 30.0; [0x4ea508] (a shared
  timer) advances by dt for rider V, wraps at F 50, reset past 50; [0x4ea50c] toggles. No random.
- above 5 m/s (bits > 0x40a00000): a new skid trail pair (0x457350 twice -> obj+0x2f71,
  obj+0x2f75; obj+0x188 = 1.0, 5.0 for a computer bike) when braking starts or a slide starts
  (obj+0x2da0 set, obj+0x2da4 clear); while braking or sliding with |0x48d0f0(body)| > 0.5 and on
  the ground, 0x457390 adds a segment under each wheel; obj+0x188 grows by D 2.0 (or shrinks by 1)
  within [1, 12].
- **dust burst**: on the ground and (wheelie with obj+0x3081 < 25.0, or braking, or sliding with
  |...| > 0.5): 24 particles (4 for a computer bike), each drawing **RAND1() six times** (one for
  the size, three for the spin, two & 3 for the offset; the second offset is negated unless key
  bit2 = left) -> 144 draws a step for a braking player. Particle kind 0xe, colour 0x2104.
These are effects only, but they **consume stream 1** every step a bike brakes or slides.

##### 8.13 0x467930(obj): the speedometer, rpm and the computer bikes' gear [asm]
dist = f32-free 80-bit sqrt((dz^2 + dy^2) + dx^2) of the rear wheel's move (obj+0x338 - +0x344);
kmh = ftol(dist / (dt * D 0.01) * D 0.028125 [0x4d1160]); obj+0x114 = obj+0x118 = kmh (int).
Computer bike (obj+8 == 0): obj+0x120 = f32((((float)kmh - (float)old118) / dt * D 100.0 +
obj+0x120) * D 0.5), limited to [-50, 40] (int compares); obj+0x118 = ftol(obj+0x120 * dt *
D 0.01) + old118; < 0 -> 0 (a smoothed display speed).
Human bike (obj+8 && obj+4): obj+0x11c (rpm) = ftol(C+0x74 RegimeMax / C+0x18 MoteurVitesseMax
* obj+0xec). Others: obj+0xec temporarily = f32(obj+0x118 * D 0.27777777777777778), >= 0;
r = 0x48e180(body, &obj+0x7c) (gear and fraction from speed, below); obj+0xec restored;
obj+0x11c = ftol(r * C+0x74), < D 800 -> 800.
**0x48e180(body, int *gear)** [asm]: v = body+0xc8, R = C+0x48. Road: the first of v < R*D 0.18,
0.33, 0.48, 0.62, 0.74 (80-bit products) gives gear 1..5 with f = F 0.23, 0.38, 0.53, 0.67, 0.79,
else gear 6, f = 1.0. Dirt: R*D 0.15, 0.3, 0.45, 0.75 -> gears 1..4, f = 0.2, 0.35, 0.5, 0.8,
else 5, 1.0. Returns f32(v / (f*R)) clamped to [0, 1]; *gear = the gear.

##### 8.14 0x46d340(obj) -> obj+0x31d9: which lane [asm]
L = X.(obj+0x30 - pos) - X.(section +0x30 offset) (section X axis at +0xc; the offset is 0);
line = section +0xb0 (racing line). For k = 0, 1, 2: c = (float)(240k) + line; hi =
ftol(c + F 120 [0x4d1320]), lo = ftol(c - 120); if lo < L < hi return k. Else: L <= ftol(line -
120) -> 0; L > ftol(line + F 600 [0x4d1324]) -> 2; else -1. (Lanes 240 units = 1.9 m wide
starting on the racing line; read by the computer bikes [guess].)

##### 8.15 0x461aa0(obj): shadow points [decomp]
Six points obj+0x2ea9 + i*12 (i = 0..5): around the wheels at +-40 units across (front-left,
front-right, mid-right, rear-right, rear-left, mid-left), each put on the road
(SECWALK(obj+0x31c1) + 0x40c960) and raised F 4.0 [0x4d0f80] (F 8.0 [0x4d1020] for points 2 and
5). While crashed with NewMotorEngine [0x52c234]: four corners obj+0x30ed..+0x3119 around the
tumble point from the tumble matrix (F 32, F -2, F 2.0), on the road + 2. Drawing only.

##### 8.16 0x467b30(obj, r) and 0x45d810: the rider's pose; stunts [decomp, asm for the stunt]
- obj+0x638 (skeleton LOD) from 0x45f160 (camera distance, drawing) unless crashed or obj+0x319d
  (then 1, the hand skeleton). If r == 0 (wheels not on the road): the bones take PilotRot #0
  (static) as they are; else 0x45d810(obj, dirt) blends the ROT records (0x51d2a8 + k*0xf4) by
  speed, frame pitch, lean, the crash timer, etc. **Cosmetic**: 0x45d810 writes only the bones,
  obj+0x304d/+0x3051/+0x3055..+0x3079 (its own state), obj+0x319d and obj+0x3205.
- **Stunts** (only the local player's bike, 0x581218 + [0x4ea4c0]*0x3209, and only when r != 0)
  [asm 0x45dff2..0x45e1b7]: start when in the air (obj+0x2db8 == 0), obj+0x124 > D 18.0556
  [0x4d0fd0] (65 km/h), rear.y - obj+0x2db0 >= D 64.0 [0x4d0fd8] (half a metre above the ground
  point 0x468ca0 keeps), action 6 newly pressed (obj+0x3201 && !obj+0x3205) and no stunt running:
  k = [0x580d0c + obj+0x3061*16] (the j-th "_N_" entry = its PilotRot index); obj+0x3065 = k,
  obj+0x305d = &rot[k] (0x51d2a8 + k*0xf4), obj+0x3055 = race clock [0x50bbc8] (ms);
  **Rand_Next(5) is drawn and thrown away**, then obj+0x3061 = clock % [0x5cf4fc] (unsigned).
  Running: t = clock - obj+0x3055; frames N = [0x580d10 + k*16], length ms = [0x580d14 + k*16];
  u = f32(fsin(t / len * D pi) * D 1.2), clamped to [0, 1]; i = min(ftol(N*u), N-1); blend rot
  k+i-1 -> k+i by N*u - i; ends when t > len; obj+0x319d = [0x580d08 + k*16].
  (The family script parser 0x429e20 stores, for PilotRot k named "_N_x": N at 0x580d10+16k,
  its first int at +0x14 (length ms), its second at +0x08 (the flag); the list of such k at
  0x580d0c + 16j, count [0x5cf4fc].) **Stunts change no physics field.**
- obj+0x3205 = obj+0x3201, obj+0x3201 = 0 at the end of 0x45d810 (so the "pressed" latch lasts
  until the next pose update with r != 0... [asm: only when 0x45d810 runs, i.e. r != 0 and LOD < 2]).

##### 8.16a The body in full, as `port/src/pose.c` has it (2026-10-30) [decomp f_45d810, f_45b4e0]
Poses by their place in the family's list: 0 static (full crouch), 1/2 left/right, 3 staticHaut
(sitting up), 4 staticPied (the foot down), 5/6 lean low, 7 standing on a jump, 8 the win, 9 sitting
back on a drop, 10..14 the gestures past the line (2nd, 3rd, next to last, last, others), 15, 16 the
fall, 29/30 the look over the shoulder, 31/32 the dirt start. A name "_N_x" begins a stunt of N
poses (then two numbers: its ms, whether it needs the hand skeleton); any name beginning "_" loses
3 letters (0x429e20: the port used to skip these, so every pose after them was out of place).
- 0x45b4e0(o, a, b, t): n = the smaller count. t bits <= 0: a; t >= 1: b; each angle the short way
  (b -= 2pi if a + pi < b; b += 2pi if b < a - pi), then brought into 0..2pi.
- speed >= 5.5556 m/s (20 km/h): `+0x304d` = pose 4. Normal: **blend(rot3, rot0, (speed - 5.5556) /
  (top * 0.27778 - 5.5556))**, top 120 km/h (road) or 60 (dirt): up at 20 km/h, flat at the top
  (read the decomp with care: the routine's `a` is the local piVar7, `b` piVar10).
- The winner's lap, place 0, finished: 0x5dabd0 from 200, less dt a step (2 s): T > 175 rot8 -> rot3,
  25 < T <= 175 rot8, 0 < T <= 25 rot0 -> rot8, each with the hand skeleton; then normal.
- A driven bike finished: blend(rot3, gesture of the place, (speed - 5) * 0.1), hand skeleton.
- speed < 5.5556: blend(`+0x304d`, rot3, speed * 0.18). From pose 31 in two halves (31 -> 32 -> 3).
- Dirt, no wheelie: x = (front.y - rear.y) / (wheelbase * 48) * 4, -1..1; s = +0x3051 += (x - s) / 4;
  blend to rot7 by s (or rot9 by -s).
- 40 km/h and up, the look: the nearest loaded rider, 2 steps running, within 512 units, 20 km/h
  faster, on the ground: dot of the two directions on the ground. If it rises and |dot| < 0.7: rot29
  or rot30 by the side's sign for 400 ms, weight sin(t / 400 * pi).
- The lean: positive rot6 -> rot2, negative rot5 -> rot1. Road: inner blend (speed - 8.3333) * 0.12,
  weight lean * 4.3478 under 16.67 m/s, * 2.1739 above. Dirt: f = lean * 2, blend(high, low, (f - 0.46)
  * 1.08696), weight f * 2.1739.
- The end: not crashed, 900 < d < 1000: towards rot0 by (d - 900) * 0.01. Crashed and timer < 400:
  rot15 by (400 - timer) * 0.02; once the tumble's speed is under 3/4 of its start, rot16 by sin of the
  rest; hand skeleton.
- Detail 1 (`FileSklHand`) takes over from the distance while crashed or +0x319d, which each step
  clears at its start. Road: the same file; dirt: `mHand\\corps.skl`. Both details' wheels are the same
  files. 0x4ea4e0 (the far model between 1000 and 2500 regardless) has no writer: always 0.
- Game_Frame runs 0x467b30 again for all riders when the camera has cut (0x4dc138).
- Port's differences: the clock is the step's (16 ms each); Rand_Next(5) is not drawn (nothing else
  uses that stream); the detail is worked out again from the drawn place in a picture.
- Compared with the original: `build/ref/orig_salute*.png`, `orig_fall_a.png` against the port's
  frames: the foot down, sitting up, crouching, the lean, the fall, the salute look the same. Stunts seen
  in the port (Great Wall's jump, Space in the air); not captured from the original.

##### Fields (this part) — read (R) / written (W)
| field | meaning | by |
|---|---|---|
| +0x7c | gear (int) | W 0x467e30 (1), 0x48e180 for computer bikes; R crash/collision |
| +0xb4 | top speed for ratios (C+0x48, +2 manual) | R 0x469c60, 0x466d70 |
| +0xb8 | power factor 1.0 / 1.2 / 2.0 (wheelie) | W 0x4660f0 |
| +0xbc..+0xe8 | bike matrix (X, Y, Z rows, T) | R |
| +0xec | body+0xc8 (engine/drive speed, body fork) | R/W 0x467930, crash, collisions |
| +0xf4 / +0xf8 | smoothed forward speed change / forward acceleration | W 0x4667e0 |
| +0x114 / +0x118 | speed shown (km/h, int); +0x118 smoothed for computer bikes | W 0x467930 |
| +0x11c | rpm (int) | W 0x467930 |
| +0x120 | computer bikes' display acceleration | W 0x467930 |
| +0x124 | speed, m/s (body+0x100) | R; W crash 0.9 decay, collisions, reset |
| +0x174..+0x180 | push direction / size (body+0x150..+0x15c) | W collisions, reset (+0x180 = 0) |
| +0x184 / +0x188 | skid width state | W 0x466d70 |
| +0x18c..+0x194 | travel direction (body+0x168); computer bikes' push dir | R 0x469c60; W collisions |
| +0x1a4 | computer bikes' speed copy | W crash, collisions |
| +0x1b4 / +0x1c0 | front wheel point / its previous | W 0x469c60 (road); R spin, effects |
| +0x274 / +0x3f8 | front / rear wheel time off the ground | W 0x4667e0 |
| +0x338 / +0x344 | rear wheel point / previous | W 0x469c60 (road) |
| +0x2da0 / +0x2da4 | sliding now / last step | R 0x466d70 |
| +0x2da8 | forward speed at the step start | R 0x4667e0 |
| +0x2dac | sound event bits (0x40 crash bounce; 4/8/0x10/0x20 hits) | W |
| +0x2db0 | ground height under the rear wheel (0x468ca0) | R stunts |
| +0x2db8 | on the ground | R |
| +0x2dc0 / +0x2dc4 | rear / front wheel touching | R 0x4667e0 |
| +0x2dc8 | last step's keys | R 0x466940, 0x466d70 |
| +0x2dd0 | rider number (1-based) | R 0x4681d0 |
| +0x2e1d / +0x2e21 | lean / previous lean | W 0x469c60 |
| +0x2e29 | steering flag (0x466240) | R 0x469c60 |
| +0x2e31 | sideways slip sign (0x468ca0) | R 0x467e30 |
| +0x2e39 | frame dive | W 0x4667e0 (and 0x45d580 decay) |
| +0x2e3d / +0x2e49 | rear / front wheel spin angle | W 0x466940 |
| +0x2e55..+0x2e81 | frame matrix (X, Y, Z, T=0) | W 0x469c60 |
| +0x2e85 | turbo held (copy of +0x31f9) | R 0x4660f0 |
| +0x2e8d | pitch | W 0x466a80 |
| +0x2e91 | rear suspension | W 0x466a80 |
| +0x2e95 / +0x2e99 / +0x2e9d | spring push / phase / amplitude | W 0x466a80 (+0x2e95 0x4660f0, 0x468ca0) |
| +0x2ea1 | smoothed frame pitch | W 0x466a80 |
| +0x2ea5 | pitch offset (0x45d580, 0x466660) | R 0x466a80 |
| +0x2ea9..+0x2eed | 6 shadow points | W 0x461aa0 |
| +0x2f71 / +0x2f75 | skid trail ids | W 0x466d70 |
| +0x3055..+0x3079 | pose: stunt start, list position, rot, look-at-rival state | 0x45d810 |
| +0x307d / +0x3081 | wheelie angle / turbo time | W 0x4660f0 |
| +0x3085..+0x30b1 | tumble matrix | W 0x467e30, 0x4683e0 |
| +0x30b5 | tumble point | W 0x467e30, 0x4683e0 |
| +0x30c1 / +0x30c5 | crashed / crash timer (400 at start) | W 0x467e30, 0x4681d0, 0x4683e0 |
| +0x30c9 / +0x30d5 / +0x30d9 | tumble direction / speed / start speed | W crash |
| +0x30dd / +0x30e1..+0x30e9 | roll side / tumble rotation rates | W crash |
| +0x30ed..+0x3119 | crash shadow corners | W 0x461aa0 |
| +0x319d | detailed skeleton this step (stunt flag, crash) | W |
| +0x31a1..+0x31ad | crash impulse, flag | W 0x467e30 |
| +0x31c1 | section index (hint) | R |
| +0x31d1 | collision timer (200 after a hit) | W collisions, 0x4681d0; decays 0x469c60 |
| +0x31d9 | lane 0/1/2/-1 | W 0x46d340 |
| +0x31f9 / +0x3201 / +0x3205 | turbo held / pressed latch / previous latch | R/W |

##### Random numbers in this part
- Stream 1 (Rand_Next(1) via 0x458cd0): debris 3 + 5n per burst (crash start n = 6, crash
  bounce n = 4); dust 6 per particle, 24 (computer 4) particles a step while braking/sliding/
  wheelie-starting on the ground near rider V.
- Stream 5: one draw per stunt start (value discarded).
- The C library's rand() (0x458ce0): crash roll side when lean and slip are both 0; in every
  tested bike pair `rand() & 3`.
If any game logic (the computer bikes) also draws stream 1, these effects change the race: the
port must draw the same numbers in the same order. [open]

##### Open points
- 0x40ca80's use of the tumble direction (reflected or not) — ground fork.
- The Euler builders inlined in 0x467e30, 0x4683e0, 0x469c60 (wheelie): transcribe from the
  listed asm ranges with COS[] (exactness needs COS[0x3000] ~ -1.8e-16, not 0).
- 0x45d810's blend weights for every case (cosmetic) are only in decomp form here.
- Is obj+0xbc's translation (T) ever non-zero in the race? Several sums add it (Z row + T).
- Who else draws Rand_Next(1) (computer bikes 0x46bf80/0x46b120?) — decides whether effects must
  be exact.
- 0x47ecd0 checked in decomp only (constants by width from the asm); its x87 order not written.

---------------------------------------------------------------------------------------------

#### 9. Checks against the running original [run]

How: tools/orig.sh start with `-FrameRateMax60` (the GOG launcher's setting), the hidden
display (Weston :2), Solo → Practice → Speed Bay, SPEED bike 1, automatic or manual box.
tools/win/memwatch.c (built to build/memwatch.exe) samples every 8 ms: 0x4e9808 (ms as a
float), 0x4e980c (ms), 0x50bbc8 (the frame's time stamp), 0x4dc0b4 (countdown), 0x5429b0
(running) and the whole player object 0x584421 (0x3209 bytes); xdotool holds the keys
(build/phys/drive.sh). The last sample of each frame (same 0x50bbc8) is the state after that
frame's step. Recordings (build/phys/): w1.bin (no frame limit: 33 ms frames; accelerate into a
wall), w2.bin (automatic: accelerate 3.5 s, coast, brake), w3.bin (manual: A/Z shifts),
w4.bin (accelerate, left 0.8 s, right 0.8 s, turbo 1.5 s, into the wall).

Each check runs one step of the reference from the recorded state k and compares with the
recorded state k+1, bit for bit:

| what | result |
|---|---|
| Frame time | with -FrameRateMax60 every race frame is 16 ms (813 in a row), so dt = 1.6 every step; without it 33 ms (dt 3.3) |
| Engine step 0x465da0 → 0x48ef30 + the gearbox in 0x466660 (speed +0x124, wheel +0x12c, engine +0xec, gear +0x7c) (build/phys/chk_engine.py) | **bit-exact on every step on the ground outside wall contact and turning**: w2 555, w3 427, w4 245, w1 506 steps (dt 1.6 and dt 3.3; automatic and manual shifts, coasting, braking, the turbo). The only differences: steps with a wall hit (+0x2dac ≠ 0) and while turning (the slide 0x45d110 and the steering change the speed after the engine, which that check does not model) |
| Upshift table 0x48e3a0 | 17.8, 34.5, 50.8998, 65.6996, 72.8995 = the bits in memory |
| Integration 0x48d620 (position, displacement) (chk_integ.py) | x and z bit-exact on the straight; y differs (0x468ca0 puts the bike on the road after it, not modelled in that check) |
| Steering 0x466240: angle +0x110 and rate +0x10c (chk_steer.py) | **bit-exact on all 107 steering steps of w4** (left, right, centring) |
| Heading after 0x45cb00 | the turn predicted with the table rebuild equals the recorded heading change to the 1e−6 printed (median 2e−6), against 6e−4 without the table; the full matrix is not compared bit for bit because 0x468ca0 rebuilds pitch and roll from the road afterwards |
| Wall spin (0x468ca0, +0x2e31) | after the hit at step 261 the extra heading change is 0.0377 a step and +0x2e31 falls by 0.0377 a step = 3π/400 · dt |
| Section planes 0x40e5c0 (tools/trkplanes.py) | all 4 planes of all 788 Speed Bay sections bit-exact against the memory dump |
| Turbo / wheelie 0x4660f0 | the angle +0x307d rises 0.0754 a step (dt · 0.04712), the held time +0x3081 1.6 a step |
| Shift timers, rpm | 150 / 200 / 300 ms timers seen as the code says; rpm = MVM/(ratio·VMB) · wheel |
| Genuine-copy flags | 0x4dc1f4 = 0x4dc1f8 = 0x4dc1fc = 1 in the running game (memtool): so always torque ×1.1 with the manual box, turbo power 2.0, no wheel cap, NbSpeed as in the file |
| The bike's matrix T (+0xe0..0xe8) | 0.0 in every recorded step |

The checks run with the x87's 53-bit default: their bit-exact agreement is the evidence that no
routine on this path changes the precision control. So **a port in C with `double` for the
register values and `float` for every store reproduces these steps exactly** (with SSE2 double
arithmetic, no x87 80-bit, no fused multiply-add: build with -ffp-contract=off; sqrt and the
divisions are correctly rounded in both). atan2 (fpatan) and its use through f32 and the table
truncation has not shown a difference yet; a libm atan2 may differ by an ulp of the double now
and then [guess: harmless after the f32 and the truncation, almost always].

What was **not** checked against the original: the ground contact 0x468ca0 (heights, the matrix
from the road, landing, walls) beyond the wall-spin decay, the slide 0x45d110, the suspension,
lean and frame (0x4667e0 … 0x469c60), crashes and getting up, bike-against-bike, the effects.
Those are mapped from the machine code only ([asm] marks in §7, §8).

#### 10. Open points

Resolved on the way: the FPU precision (53-bit, bit-exact steps), the matrix translation T (0),
the copy-protection flags (all 1), the plane builder (bit-exact), dt (0x4d1900 is 0.0: dt
never forced to 1.0 in 0x47df40; 0x45d580 forces 1.0 only for dt ≤ 0).

Open:
1. 0x468ca0 checked only by reading (and the wall spin): a check of a whole step on the road
   (y, the matrix's pitch/roll, +0x2db8) needs 0x40c8b0 and 0x40c7e0 in the reference; the
   planes are now exact, so this is the next check to make.
2. The slide 0x45d110 and the turn radius 0x45cd10: [asm], not run (needed for the speed while
   turning to stay exact).
3. The Euler builders inlined in 0x467e30, 0x4683e0 and 0x469c60 (crash tumble, wheelie frame)
   are located but not written out instruction by instruction.
4. 0x47ecd0 (bike against bike) from the decompiler only; not in Practice.
5. Random numbers: wall-crash chances and the fall side use the C library's rand() (0x458ce0,
   msvcrt's LCG: seed·214013 + 2531011, bits 16..30), the effects Rand_Next(1), a stunt
   Rand_Next(5). If the computer riders draw stream 1 too, the effects must be drawn in the same
   order for a race to stay the same.
6. The computer riders' step 0x46bf80 / 0x46b120 is not mapped (the task covered the player).
7. 0x4dc150 (the smoothed frame time) feeds the lean of a human rider (0x469c60): with the
   port's fixed step it is a constant 16 (it converges to the frame time).
8. Joystick bits 0x411b30 from the decompiler only.
9. C+0x28, 0x44, 0x54, 0xa0 have no reader with an absolute address; one through a row pointer
   is not excluded.
10. Rounding: the port's step should be dt = 1.6 (16 ms) at 62.5 steps a second to be the
    original's step; 1/60 s (dt 1.6667) gives slightly different results (not linear in dt).

#### 11. Size of a C port

The routines on the player's path (§4–§8) are about 40–45 KB of x87 code. Written as clean C
with double registers and float stores:
- step order, controls, throttle and engine, gearbox, torque: ~900 lines;
- integration, slide, turn radius, steering, the rotation helpers: ~700 lines;
- the ground contact with the track helpers (walk, height, walls, planes): ~900 lines;
- suspension, pitch, lean, frame and wheel points, wheel spins, speedometer, lane: ~700 lines;
- crash start, tumble, get-up, bike-against-bike: ~600 lines;
- effects (dust, spray, skid marks) and the rider's pose/stunts: ~500 lines (visual).
**About 3,500–4,500 lines of C**, of which ~2,500 decide where the bike goes. The cosine table
and the carac rows come from the player's game.EXE / the BPT files; tools/bikephys.py and the
recordings in build/phys/ can check each piece step for step.

### 4.19 The in-race dialogs (pause, results, continue, F1, F11/F12; mapped 2026-10-26)

**Port** (`dialog.c`, 2026-10-26): the dialog object, hotkeys, arrows, Return, the growing
shaded box and the closing slide; the pause menu (arg 1) with RESTART, QUIT RACE, CONTINUE.
Its picture after Escape matches build/ref/orig_race_escmenu.png on every button pixel
(SmallFonts 1: font 33, halved places). The race's restart and quit go through events done
at the next frame, as 0x4dc0c4's. The mouse: §2.6a. Not yet: the championship's 0x4580f0 after
QUIT RACE.

Scratch notes for NOTES §4.17 part B §3–§4 (helper, 2026-10-26). Read in build/mr.asm unless
marked. Tags: [asm] read in the machine code, [decomp] Ghidra's text only, [guess],
[cap] checked against a capture of the original (build/ref/orig_race_escmenu.png).
Names as in NOTES §4.4 (menu) and §4.8 (fonts, text objects). "Item" = a text object of
the class of §4.8.3.6 (0xc2 bytes, vtable 0x4d0560): the dialogs are built from the very
same objects as the text list, so `text.c` is most of the drawing.

#### Summary

- **Four slots**, each a 0x78-byte dialog object or 0: **0x4e8908** F11 sound / F12 display
  menus, **0x4e8904** pause / results / continue, **0x4e890c** F1 help, **0x4e8910** message
  boxes. Frame order (0x4126a0): 0x4e8910, 0x4e890c, 0x4e8904, 0x4e8908. Key/mouse order
  (0x4329a5): 0x4e8908, 0x4e8910, 0x4e890c, 0x4e8904, then the race keys 0x431250. Draw
  order (0x47ff90 step 19, after the HUD and the debug lines, before present): 0x4e8908,
  0x4e8904, 0x4e890c, 0x4e8910 [asm].
- A dialog = **rows** (0x6c bytes) pointing at **items** (text objects). Row 0 is the title
  (not selectable); rows 1.. are buttons. Each button row has two items: *normal* (flies
  in, then still) and *highlighted* (still, blinking 2 frames on / 2 off). The row shows
  one of them (row+0) [asm/decomp].
- **Opening**: sound, the race held (+0x10), the HUD's message list cleared, the first
  button selected (select sound 0x29); title and buttons **fly in** (text motion mode 1,
  §2.4), the **see-through box grows** from its centre over 15 drawn frames (§3.2).
- **Box**: the items' final bounding box + 5 px; inside it the picture is darkened to 50 %,
  with a 5-px soft bevel (230/256 → 128/256) [asm][cap: exact 213,201–426,275].
- **Keys**: a letter **hotkey** per row (first letter of its text not used by the control
  keys), shown as the GameMenu font's lowercase (highlighted) glyph; arrows move to the
  nearest row in that direction (no wrap); Return presses the selected row; Escape is an
  extra hotkey of CONTINUE (pause) / NO (results). Every other key-down is swallowed while
  the dialog is up (+0xc), except F2–F4, numpad * and /, Shift/Ctrl combinations [asm].
- **Mouse**: hovering selects (and leaving all rows deselects); press = left button up on
  the row selected at button down. Pointer = the Windows cursors from game.EXE (§2.6).
- **Closing**: a chosen row sets +0x20; the dialog is freed at once (in the message
  handler, or in 0x4126a0); sound 0x28; every row's shown text is copied into the message
  list 0x53b8e8 and **slides off to the left, accelerating** (mode 2), fading (§2.5).
- The **Pause key** (0x4dc038) is not a dialog: the race frame is simply not called, the
  picture freezes, CD and looping sounds pause; no text, no PAUSE.WAV (0x2e is only played
  by the dead replay-caption code 0x4121e0) [asm].

#### 1. The objects

##### 1.1 The dialog (0x78 bytes, ctor 0x4338b0) [decomp, fields checked against the users in asm]

| off | ctor | what |
|-----|------|------|
| +0x00 | 0 | kind: 4 results, 5 continue, 6 pause (others: forks' sections, §5); 9 = "joystick does not drive it" (Game_Frame step 5) |
| +0x04 | 0 | unused here |
| +0x08 | 1 | draw the box (0x434ae0) |
| +0x0c | 0 | **takes the keyboard**: arrows/Return work, every key-down is swallowed, pointer = "no" sign when not over a row. 1 in all three race dialogs |
| +0x10 | 0 | **holds the race** (0x4126a0 returns 0: no steps, the race is drawn with flags +0x18) |
| +0x14 | 0 | HUD variant: 1 → 0x4804d0 instead of 0x4806e0 (race_view.md §3.1) |
| +0x18 | −1 | the race's drawing flags while held; the race dialogs set **−10** (0xfffffff6): bit 3 off = no dials (0x484810 parts, 0x40f2d0, 0x486310, 0x47b7a0) |
| +0x1c | 1 | reserve the box in the race message list 0x53ba68 (§3.1) |
| +0x20 | 0 | **done**: set by an action; the dialog is then closed and freed |
| +0x24 | 0 | rows allocated (0x433990 sets 1); 0 → closed at once |
| +0x28 | 1 | active: 0x434960 runs each frame |
| +0x2c | 0 | id of the box-reservation object in 0x53ba68 |
| +0x30 | 0 | optional close test (fn → non-zero = close; e.g. 0x47dab0) |
| +0x34 | 0 | optional callback called at the start of every draw |
| +0x38 | 0 | optional close callback (called by 0x4126a0 instead of setting +0x20) |
| +0x3c | 0 | hotkey letter case: 1 = text upper case, hotkey letter lower case (all race dialogs); 2 = the reverse |
| +0x40/+0x44 | −1 / 0 | vx, vy of the closing slide (0/0 = no slide) |
| +0x48 | 0 | item* array, +0x4c int array (receives each item's id), +0x50 item count |
| +0x54 | 0 | **selected row** (row*, 0 = none) |
| +0x58/+0x5c | 0 | row array (0x433990(n): n × 0x6c), row count |
| +0x60 | 0 | drawn-frame counter (box growth) |
| +0x64 | now 0x50bbc8 | creation time (ms; the message-box timeout 0x47dab0 reads it) |
| +0x68..+0x74 | 0 | box x0, y0, x1, y1 (recomputed by 0x434860 every frame) |

Building a dialog (the same in every constructor) [asm]: free the slot's old dialog
(rows, items through their vtable slot 0 (1), arrays) — **no close effects**; new dialog;
set the fields; make items on the stack (0x415dd0 + 0x416170 motion + 0x416300 text/font +
0x4163e0 colours); 0x433990(rows); item arrays (n ≤ 0x7f); 0x433a30(i, &item) copies each
(type +0xc: 0 base 0xac bytes, 1 text 0xc2, 2 0x178 bytes 0x415e80 — help) and calls its
slot 6 **Relayout** (§2.3); 0x433c90 per row; row callbacks; select row 1 and call its
enter callback (§2.2); optional row cursors 0x433c20(row, 0xe4).

##### 1.2 The row (0x6c bytes; 0x433990 init, 0x433c90/0x433dd0 fill) [decomp+asm]

| off | init | what |
|-----|------|------|
| +0x00 | 0 | **shown item** (= +4 normal, or +0xc when selected) |
| +0x04, +0x08, +0x0c, +0x10 | 0 | items 1–4. 0x433c90(dlg, row, i1, i2, i3, i4, n, c1..cn): +4 = item[i1] … (0 → i1), +0 = +4; the race dialogs pass (n, h, h, n): +8/+0x10 are never shown [decomp: no reader found] |
| +0x14 | 0 | leave (fn(row)): 0x4361c0 = row+0 = +4; ends a number edit (+0x54 2 → 1, clamp +0x3c to [+0x48, +0x44], write *+0x4c) |
| +0x18 | 0 | enter: **0x436150** = snap all 4 items to their targets (0x416b00), row+0 = +0xc, +0x54 1 → 2 (+0x50 = 0), **sound 0x29 select** |
| +0x1c | 0 | left button down: 0x436140 = return 1 |
| +0x20 | 0 | left button up: **the action** |
| +0x24 | 0 | arg (unused here) |
| +0x28 | 0 | right button up (0 here) |
| +0x2c | 0 | Return / hotkey down: **the action** |
| +0x30 | 0 | hotkey up (0 here) |
| +0x34/+0x38 | 0 | Windows cursor handle / resource id over this row (0xe4) |
| +0x3c..+0x54 | | number field: +0x3c value, +0x40/+0x44 = −1/... (+0x44 max), +0x48 min, +0x4c int* target, +0x50 caret, +0x54 0 none / 1 shown / 2 editing (F11/F12 only, §5) |
| +0x58 | **1** | static (title, labels): not selectable, no hotkey, no lowercase. Buttons set 0 |
| +0x5c | 1 | visible (drawn, selectable) |
| +0x60/+0x64 | 0 | check box: flag, int*; the first char of the text becomes ' ' or '^' (fonts 2, 6, 32, 33) (§5) |
| +0x68 | 0 | 4 hotkey chars. 0x433c90's n (clamped 0..4) chars; with +0x3c == 1 'A'–'Z' are made lower case there; [0] is overwritten each frame by the automatic letter (§2.1) |

##### 1.3 Items: 0x416170 arguments (this order; NOTES §4.8.3.6 fields)

`0x416170(x, y, lifetime, align, effect, mode, blink, int *id, vx, vy, tx, ty, s.lo, s.hi,
k.lo, k.hi)` → +0x70/+0x78 = x, +0x74/+0x7c = y, +0x8c lifetime, +8 align, +0x10 effect,
+0x14 mode, +0x58/+0x5c vx/vy, +0x60/+0x64 (and authored +0x68/+0x6c) target, +0x40 double
s, +0x38 double k; +0x48 = +0x50 = 1.0; start time = the text clock 0x4e4ed0 (0x4e9814 in
replay playback); blink < 0 → period −blink ms, > 0 → frame mask, 0 with effect 3/4 → 2.
For mode 1/3: +0x28 = (double)x, +0x30 = (double)(H − y), +0x18/+0x20 = vx, vy [decomp, asm
of the motion confirms the fields].

#### 2. The engine

##### 2.1 Per frame: 0x434960 (from 0x4126a0, when +0x28) [asm+decomp]

0x4126a0 first calls **0x43e0f0**: the 64-byte "used letters" table 0x53eb20 = toupper of
the 7 control keys (Action0..6Key, 0x53eae0 + 8i: defaults Up, Down, Left, Right, 'A',
'Z', Space → '&', '(', '%', ''', 'A', 'Z', ' '), then 0x70..0x77 (F1–F8, which are 'p'..'w'
as chars) and 9 (Tab). Then per dialog (order 0x4e8910, 0x4e890c, 0x4e8904, 0x4e8908):
- for each row with +0x58 == 0, +0x5c != 0, row+0 != 0, item(+4)+0xc == 1 and a text: walk
  the **normal item's** text; the first alphabetic char whose upper case is not in the
  table becomes **hotkey[0]** (upper case); none → hotkey[0] = 0. A non-zero letter is
  appended to the table (≤ 64). So "[RESTART]" → R, "[QUIT RACE]" → Q, "[CONTINUE RACE]" →
  C, "[YES]" → Y, "[NO]" → N, "[SAVE]" → S; with the default keys A and Z are never
  hotkeys.
- 0x434860: the box = min/max over the rows' **shown** items of 0x4172d0(item): mode 1 →
  the *target* rectangle (tx, ty, tx+w−1, ty+h−1), else the current one (slot 7 0x417210:
  x, y, x+w−1, y+h−1); items that are not alive are skipped (start 0xffff,0xffff,0,0).

##### 2.2 Keys and mouse: 0x433ec0(hwnd, msg, wparam, lparam) → 1 = consumed [asm+decomp]

Called by the race window procedure 0x4329a5 for each existing dialog in slot order
0x4e8908, 0x4e8910, 0x4e890c, 0x4e8904 until one returns non-zero; then that dialog is
closed and freed if +0x20 != 0 or +0x24 == 0 (0x4354e0 + free; slot 0x4e890c also logs),
the cursor is set (below), DefWindowProc, return. If none consumed: the race's own
handling (WM_KEYDOWN → 0x431250). Let `cur` = +0x54 at entry.

**WM_KEYDOWN (0x100)**, key = vk:
1. Ctrl (0x11): 0x4dc108 = 1, consumed. Shift (0x10): 0x4dc10c = 1, consumed.
2. Ctrl held: consumed only for 'F' (0x46) — so Ctrl+P (pause) still reaches the race.
   Shift held: not consumed.
3. F2, F3, F4 (0x71–0x73), numpad * (0x6a), numpad / (0x6f): not consumed (screen size /
   modes, the race handles them).
4. `cur` editing a number (+0x54 == 2): digits (and numpad 0–9 = vk 0x60–0x69), Backspace,
   Delete, Left/Right move the caret, Return stores if min ≤ value ≤ max else Beep (F11/F12).
5. Else: a row whose 4 hotkeys contain (char)vk → **sel = that row, call its +0x2c**
   (the action); no enter callback, no select sound. Else if +0xc: arrows → 0x435090 (2.3);
   a row found → sel = row, call its +0x18 (enter: highlight + sound 0x29); Return → call
   `cur`+0x2c if any. Then consumed iff +0xc (or a hotkey action returned non-zero).
Key repeat = Windows' own auto-repeat of WM_KEYDOWN (no timing in the game) [asm: none].

**WM_KEYUP (0x101)**: Ctrl/Shift up clear 0x4dc108/0x4dc10c (consumed). Arrows: consumed if
+0xc. Else a hotkey row: its +0x30 (0 in the race dialogs) and sel = row; consumed iff +0xc.

**Mouse** (lparam x,y; all set the pointer timer 0x4dbfb4 = 3000 ms):
- position → 0x53eb60 (x, y shorts); windowed (0x4dbff0): ClientToScreen (sic); y −=
  (0x624b18 − 0x68b008)/2 when non-zero [guess: the letterbox band].
- 0x200 move: (0x4dbfb0 = 0.) If no button held (0x4e897c, 0x4e8980): row = hit test
  0x435430 (first selectable row whose shown item's *current* box contains the point).
  No row → sel = 0 (**deselects**). A selectable row other than `cur` with an enter
  callback → sel = row, enter (highlight + 0x29). Consumed if the row is `cur`.
- 0x201 left down: SetCapture, 0x4e897c = 1; hit row (selectable) → sel = row, +0x1c (1);
  else sel = 0.
- 0x202 left up: ReleaseCapture (unless the right button is held), 0x4e897c = 0; **sel's
  +0x20 = the action**, wherever the pointer is now (selection does not follow the pointer
  while a button is held).
- 0x204/0x205 right down/up: capture flags 0x4e8988/0x4e8980; up → sel's +0x28 (none here).
At the end (all messages): if sel changed and the old row has +0x14 → leave (old shows
its normal item). Pointer: 0x4e8960 = sel's cursor (+0x34), or if no sel and +0xc the
"no" cursor 0x4dbf90; 0x4329a5 then SetCursor through 0x45a190 when it changed.

##### 2.3 Arrow navigation 0x435090(dlg, vk) [asm for the comparators, decomp for the rest]

From = sel's shown item's current box (x0,y0,x1,y1); no sel (or not alive) → (W−1, H−1, 0, 0)
so that every row qualifies. Candidates: rows with +0x58 == 0, +0x5c != 0, an alive shown
item; with box (a0,b0,a1,b1):
- Left: a1 < x0, key = a1<<16 | |(y0+y1)/2 − (b0+b1)/2|<<8 | index, sorted **descending**
  (0x4353d0) → nearest on the left first.
- Right: a0 > x1, key = a0<<16 | |dy|<<8 | index, **ascending** (0x435400).
- Up: b1 < y0, key = b1<<16 | |(x0+x1)/2 − (a0+a1)/2|<<8 | index, descending.
- Down: b0 > y1, key = b0<<16 | |dx|<<8 | index, ascending.
(The fields overlap when the offset ≥ 256; C division truncates.) First of the sorted list
(index = key & 0xff) → new sel; none → nothing (no wrap-around).

##### 2.4 Items in motion (text class 0x416b40, mode 1 "fly to target"; one step per Draw) [asm]

If (x, y) == (tx, ty) nothing. Else with doubles px = +0x28, py = +0x30 (y up), vx = +0x18,
vy = +0x20, H = 0x5d8948:
```
dX = tx - px;  dY = (H - ty) - py;  dist = sqrt(dX*dX + dY*dY)
if dist <= 1.5: x = tx; y = ty; px = tx; py = H - ty; done
aim = atan2(dY, dX); head = atan2(vy, vx); speed = sqrt(vx*vx + vy*vy)
if aim > pi/2 && aim - pi > head:        head += 2pi; diff = aim - head
else if aim <= -pi/2 && aim + pi < head: diff = (aim + 2pi) - head
else:                                     diff = aim - head
head += k*diff*0.05;  speed = (dist - speed)*k*0.05     (k = +0x38)
vx = cos(head)*speed; vy = sin(head)*speed; px += vx; py += vy
x = trunc(px); y = trunc(H - py)                         (_ftol = toward zero)
```
Mode 2 (closing slide): s += k (+0x40 += +0x38); x += trunc(vx*s); y += trunc(vy*s) (ints).
0x416b00 (enter callback): mode 1/3 → x, y = target, px = x, py = H − y.
One step per drawn frame (frame-rate dependent: 30 or 60 a second in the original).

##### 2.5 Closing 0x4354e0 (dialog) [asm+decomp]

Only if +0x24 and in-game (menu+0 == 0): cursor back (0x45a190(0)); remove the box object
from 0x53ba68 (0x4177e0 by id +0x2c); **sound 0x28 deplett**; if +0x40 or +0x44 is non-zero
(the ctor's −1/0: always here): for every row whose shown item is a text item, a new text
in the message list **0x53b8e8**: `0x417d20(text, lifetime 10000, font +0xb4, x = item+0x70,
y = +0x74, align 0x21 (none), effect 2 FadeOut, mode 2, blink 0, no id, steps 0, vx = −1,
vy = 0, target 0, s = 1.0, k = 2.0, own colours 0)`, then 0x4163e0(own colours +0xb8, fore
+0xbc, back +0xbf) copied from the item. So each line moves −3, −5, −7 … px a frame (n²+2n
after n frames) and is deleted when off screen; only font 9 (AntiAlias8Transp) shows the
fade. The HUD draws 0x53b8e8 (0x4806e0/0x4804d0) [asm]. Then the slot routine frees it
(0x435620/0x4356d0/0x435770/0x435810) and sets the cursor variable 0x4dbfac = arrow.
[guess] whether 0x417d20's overlap test can refuse a line (steps 0) was not read.

##### 2.6 The pointer in the race [asm]

Windows cursors (SetCursor; resources in game.EXE, 32×32 1-bit; loaded at 0x45a0a0):
0x4dbf84 = 116 arrow, **0x4dbf8c / rows' 0xe4 (228) = checkered arrow**, 0x4dbf88 = 229
arrow + hourglass, **0x4dbf90 = 251 "no" circle** (dialog up, not over a row), 0x4dbf94/98/9c
= 248/249/250 the arrow dissolving, 0x4dbfa0/a4/a8 = 253/254/255 the circle dissolving.
Hot spots: 116/229/234 (0,0), arrows 228/245–250 (2,4), circles 251–255 (11,11).
0x45a190(c) (in-game, 0x4dbfb0 == 0): timer t = 0x4dbfb4 (3000 at every mouse message,
minus the frame's ms in 0x4120e0 each race frame, even when held). t ≥ 0: c. t < 0: the
dissolve set of the current shape (circle set if the current is 0x4dbf90): t ≥ −90 A,
−180 B, −270 C, −360 B, −450 A, −540 B, −630 C, below −630 **hidden**. So the pointer
vanishes 3.63 s after the mouse stops. (The menu's own drawn pointer, NOTES §4.4.6.7, is
not used in the race.) Port: draw these cursor bitmaps at the mouse position.

##### 2.6a The mouse and the pointer in the port (written 2026-10-30)

`dialog.c` `dialog_mouse_move/button` are 0x433ec0's 0x200, 0x201, 0x202, 0x204/5 as in §2.2, the
hit test `row_at` is 0x435430 (the shown item's box **where it is now**, so a row still flying
in is hit where it flies). The held flags 0x4e897c/0x4e8980 are cleared only by the handler, as
in the game: a dialog closed by a key while the button is down leaves the flag set until the
next button-up reaches a dialog. A button-up with a row chosen presses that row even if the
button went down before the dialog existed.

`pointer.c` is 0x45a190 with the timer 0x4dbfb4. The cursors are read from game.EXE's own
resources (`exe_resource`: RT_GROUP_CURSOR 12 gives the RT_CURSOR number; 4 bytes hot spot,
BITMAPINFOHEADER, 2 palette entries, XOR rows, AND rows, bottom up; all 32x32, 1 bit) and put
**into the picture** as Windows would: `(screen & AND) ^ XOR`. The game uses SetCursor; drawing it
makes it scale with the window like the menus' pointer and works on a screen without a hardware
cursor (the MiSTer), and the pictures show it. While the Pause key holds the race the picture
is not redrawn: `pointer_undraw` puts back what it covered so the pointer still moves.
Deviation: the timer starts at 3000 as in the image (0x4dbfb4), but no pointer is shown until
a first mouse message arrived (the port's `g_seen`): the tests run without a mouse, and a
player who never touched it has nothing to see for 3.6 s. The timer runs in race frames only
(0x4120e0) and every mouse message anywhere resets it, so a mouse moved in the last menu shows
the arrow for the race's first 3.6 s, as in the game.
Not read: 0x4dbfb0 (WM_NCMOUSEMOVE over the window frame: the arrow, no dissolving), the band
0x624b18 − 0x68b008 (the port's picture is always the full 640x480).
Compared: the original's pause menu answers `xdotool` the same way (hover, click on CONTINUE RACE
goes on); its Windows cursor does not show in an X capture of the window, so the pointer's shape
is from the code and the resources alone.

##### 2.7 Joystick

0x411a10 (NOTES part C §6): while a dialog holds the race and the control type is a
joystick (0x4dc11c 2 or 4), unless 0x4e8904's kind is 9: direction changes → WM_KEYDOWN/UP
of the arrows, button 0 → Return, posted to the window — so it goes through 2.2.

#### 3. Drawing: 0x434ae0(dialog), from 0x47ff90 [asm]

Only if +0x24. Per call:
1. +0x60 += 1; call +0x34 if set.
2. +0x1c: if the reservation object (id +0x2c) is not alive in 0x53ba68 (first call):
   0x434860, **TextList_Clear(0x53ba68)** (the race's own messages vanish), 0x417b90(box,
   0, &+0x2c): a base text object (no text, never drawn) of the box (unpadded, lifetime 0)
   so new race messages dodge it.
3. +0x8 (box): 0x434860; **if +0x60 < 15**: f = +0x60 × (1/15) (double 0x4d0750);
   hw = (x1−x0)/2, hh = (y1−y0)/2, cx = (x0+x1)/2, cy = (y0+y1)/2 (C division);
   x0 = trunc(cx − hw·f), x1 = trunc(cx + hw·f), y0 = trunc(cy − hh·f), y1 = trunc(cy + hh·f)
   (x87, f in extended precision). Then x0 −= 5, y0 −= 5, x1 += 5, y1 += 5; software:
   **0x49a5a0(box)** now (Direct3D 0x4dc06c && !0x4dc070 [guess D3D]: after the rows).
4. For each row with a shown item:
   - if a text item, +0x58 == 0, +0x5c and a non-empty text: with +0x3c == 1 the text (in
     place) gets every 'a'–'z' made upper case and the **first** 'A'–'Z' equal to one of
     the row's hotkeys made lower case (+0x3c == 2: the opposite);
   - check box / number fields (§5);
   - if +0x5c: item slot 2 **Draw** (0x416da0: motion step, effect, colours, DrawString at
     (+0x70, +0x74) flags 0x21), slot 4 (debug frame, only with 0x4e4ec8).
   - number edit caret: string 0x4e8adc ("\x82\x83") in font 32 at the caret.
5. Coordinates: the item positions are screen pixels (Draw doubles and DrawString halves
   under the same condition, in-game and (SmallFonts or ScreenMode != 2)).

##### 3.1 The box shading 0x49a5a0(rect) (software) [asm]

Clip rect to the view (left 0x5d9a78, top 0x5d893c, width 0x5d93c8, height 0x5d8948;
inclusive x1, y1). Empty (x1 ≤ x0 or y1 ≤ y0) → nothing. E = 0x4ee9ac = **5** (the red bit
count, used as the bevel width). For each row y0..y1 (pixels of the locked back surface):
scale `s(t) = (t*256/E + 256) >> 1` for t = 4..0 gives **230, 204, 179, 153, 128**.
- Top rows y < y0 + E − 1 (y0..y0+3): t = y0 − y − 1 + E (4..1): left part x0 … x0+E−t−1
  with the left-edge scale, middle x … x1 − E + t with s(t), right part with the right-edge
  scale → a mitred bevel. Bottom rows y > y1 − E + 1: t = y − y1 − 1 + E, the same.
- Other rows: x0..x0+4 left-edge scale `s = ((x0 − x + E − 1)*256/E + 256) >> 1`
  (x0: 230 … x0+4: 128), interior x0+5..x1−5: **p = (p >> 1) & ~0x0410** (50 %, the bits
  shifted across channels removed: 1 << (11−1) | 1 << (5−1)), right x1−4..x1: `s = ((x −
  x1 + E − 1)*256/E + 256) >> 1` (x1: 230).
- A scaled pixel: per channel m in (0xf800, 0x07e0, 0x001f): `((p & m) * s >> 8) & m`.
Checked on the capture: box 213,201–426,275, ring values match 230…128 of the background.

##### 3.2 Where things land at 640×480 (SmallFonts 1, the default) [asm][cap]

Font_Get's second result is 1 for fonts without a low/high partner (font 9) and 0 for
32/33: Relayout halves the authored x, y and target **only for partnered fonts** when
SmallFonts or ScreenMode != 2 (at ScreenMode 1 with an unpartnered font: x·512/640,
y·384/480 by ((x<<8)/640<<9)>>8 and ((y<<8)/480·3<<7)>>8), then Align (0x417900) with the
item's flags on W × H. In the race with SmallFonts = 1 the buttons use **font 33
GameMenuLowRes (h 9)**, the title font 9 MenuLink (h 29). Pause menu: title target y =
240 − 5 − 29 = 206, centred (w 204 → x 218); rows y = 240, 251, 262; box 213,201–426,275 =
the capture. With SmallFonts 0 at 640×480: font 32 (h 18), rows 240/262/284, offsets ×2.
At 320×240 everything halves except font 9 (still 29 px high). [cap only for the default]

#### 4. The three race dialogs (slot 0x4e8904; events 0x4e8918)

Common: +0xc = 1, +0x10 = 1, +0x18 = −10, +0x3c = 1, +0x40/+0x44 = −1/0 (ctor), +0x8 = 1,
+0x1c = 1. Fonts: title **9** (MenuLink, AntiAlias8Transp, effect 7 = 25 % see-through)
fore **green (19,188,5)** back yellow (255,255,0); buttons font **32** (→ 33), Pal: the
colours given (normal yellow/yellow, highlighted red/red) do nothing in software — the
highlighted item differs only by blinking (effect 3, frame mask 2) and the hotkey glyph.
Every button row: leave 0x4361c0, enter 0x436150, down 0x436140, up = Return = the
action, cursor 0xe4. Row 1 is selected at the end (enter → its items snap, sound 0x29).
Actions: NOTES §4.17 part B §4.1–4.3 (each sets +0x20 first).

##### 4.1 Pause menu 0x436420(arg) (kind 6; Escape in Solo: arg 1; after a single race's results: arg 0) [asm+decomp]
Opening: 0x424090(1) pause the looping sounds, sound **0x03 continue**. +0x14 = (arg != 1).
| row | text (id) | item normal: x, y → target; align; effect; mode; blink; vx,vy; s, k | highlighted | hotkeys |
|---|---|---|---|---|
| 0 title | "MOTO RACER" (0x4e8aec, font 9) | (0, 200) → (0, 5); 0xc2; 7; 1; 0; 0,−1; 4.0, 5.0 | — | — |
| 1 | [RESTART] 104 / playback [NEXT DEMO] 107 | (300, 0) → (0, 0); 0x62; 0; 1; −400; −1,0; 4.0, 6.0 | (0,0) 0x62, effect 3, mode 0, blink 2 | R (auto) |
| 2 | [QUIT RACE] 105 / [QUIT DEMO] 106 | (−300, 22) → (0, 22); 0x62; 0; 1; −500; 1,0; 4.0, 6.0 | (0,22) | Q |
| 3 (arg 1) | [CONTINUE RACE] 110 / [CONTINUE DEMO] 108 / [CONTINUE REPLAY] 109 | (0, 200) → (0, 44); 0x62; 0; 1; −600; 0,1; 4.0, 6.0 | (0,44) | C + **Escape** |
| 4 (arg 1, champ. replay or Link host) | [QUIT REPLAY] 111 / [ABANDON] 113 | (0, −200) → (0, 66); 0x62; 0; 1; −700; 0,1; 4.0, 6.0 | (0,66) | auto |
Align 0xc2 = centred, y = H/2 − y − h (above the centre); 0x62 = centred, y += H/2.
(All x, y of font-32 items are halved first in the default setting: 300 → 150, 22 → 11.)
Solo race: RESTART, QUIT RACE, CONTINUE RACE. After a single race (arg 0): RESTART, QUIT
RACE only — Escape then does nothing (swallowed).

##### 4.2 Results 0x437b70 (kind 4; Solo single race / championship) [asm+decomp]
Opening: 0x424020 stop all sounds, sound 0x03; 0x463c30 (kind != 1); +0x14 = 1 (HUD variant).
| row | text | normal item | highlighted | hotkeys |
|---|---|---|---|---|
| 0 | [REPLAY] 100, font 9, green/yellow | (−300, 5) → (0, 5); 0xc2; 7; mode 1; vx 1, vy 0; s 4.0, k **2.0** | — | — |
| 1 | [YES] 101 | (10, 0) static; **0x6a** (right edge at W/2 − 10, y += H/2); effect 0; blink −500 | same place, effect 3, blink 2 | Y |
| 2 | [NO] 102 | (10, 0) static; **0x63** (left edge at W/2 + 10) | same | N + **Escape** |
| 3 (0x4e8924) | [SAVE] 114 | (0, 22) static; 0x62 | same | S |
Only the title moves (from the left). YES/NO side by side (Left/Right), SAVE below.

##### 4.3 Continue 0x4358b0 (kind 5) [asm+decomp]
Opening: 0x424020 stop all sounds, **voice 0x44 tryagain**; +0x14 = 1.
Title [CONTINUE] 99 (font 9, green/yellow, from (−300,5) to (0,5), k 4.0); [YES] / [NO] as in
4.2 (rows 1, 2; hotkeys Y, N; **no Escape hotkey**: Escape is swallowed).

#### 5. F11 / F12 menus, F1 help, message boxes

In §5a (F1 help, message boxes incl. the Shift+F12 mode list, Link-only boxes) and §5b
(F11 sound / F12 display menus) at the end of this file, from two helper reads. In short:
F1 help is modal (race held, no HUD, CD/sounds paused) and pages Help.gen; F11/F12 are
non-modal left-edge columns (no box, race keeps running, hotkeys and mouse only); message
boxes are D3D warnings (never in software) and the Shift+F12 mode list.

#### 6. Open points

- Effect 3 blinking and the fly-in are per drawn frame: 30 frames a second (CD, wine
  capture) or 60 (GOG -FrameRateMax60). The port's 60ths are the GOG speed.
- 0x417d20's handling of an overlapping closing line with steps 0 (could it refuse?).
- 0x4dc06c/0x4dc070 named D3D by context only (software path taken in the port).
- 0x624b18 − 0x68b008 mouse y offset: assumed the letterbox band.
- Row items +8/+0x10: no reader seen in the engine (all dialogs here pass n,h,h,n).
- Capture of the results/continue dialogs not available; positions from the code.

#### 7. Size of a C port (rough)

Dialog object, rows, per-frame hotkeys, layout box: ~150 lines. Keys/mouse/arrow
navigation: ~200. Drawing + box shading: ~120. Text-object mode 1/2 motion and the closing
slide (in text.c): ~80. Pointer (cursor bitmaps from game.EXE, dissolve): ~80. The three
race dialogs (contents, actions already counted in part B): ~150. F11/F12 menus ~80, the
mode list ~40, message box generic ~60, F1 help ~80 plus the Help.gen page engine (the .GEN
engine, not mapped here; shared with the victory sequence). Total **~1000 lines** without
the .GEN engine.

---------------------------------------------------------------------------------------------
#### 5a. F1 help, message boxes, Link-only dialogs (helper read; tags as given)

Only F1 help, the F12 mode list and the two D3D warnings reach a Solo player; a software
port needs F1 and (maybe) the F12 list. 0x438780 and 0x43b700 are Link only. No in-race
dialog uses item type 2 (0x415e80, a sprite object of 0x418160).

##### F1 help 0x43cf90 (slot 0x4e890c)
**Opens** [asm 0x431cf2..0x431e0b]: F1 (vk 0x70), not with Ctrl (other handling) nor
Shift (0x484380 debug), only Solo (0x4dbffc) and not replay playback (0x4e9830 == 0).
On success 0x4096e0(1) pauses the CD and 0x409700(1) pauses the sounds (as the Pause key).
F1 is a hotkey of its close row, so a second F1 closes it (0x431250 never sees it).
**Built** [decomp]: help object 0x4f15f8 = 0x48b120 (0x24 bytes, +0x1c = 1, +0x20 page 0),
file 0x415a10 (language folder `data\MenuXX\`) + "Help.gen" (0x4e8b40) through 0x48b180 →
0x48a4a0 (the .GEN parser, NOTES §4.8.4.3). Load failure → returns 0, nothing paused.
Fields: +0 = 7, +0x10 = 1, +0x14 = 1, +0xc = 1, **+0x18 = −0x4a** (bits 0x01, 0x08, 0x40
clear: no dials, **no HUD**), +0x34 = 0x43deb0 (draw callback: 0x48a150(1) for every help
element (+0x10 array, +0x18 count): darkened box 0x49a5a0 around an element with +0x49,
its texts via 0x489de0), +0x28 = 0 (no auto hotkeys/layout), +8 = 0 (no engine box),
+0x40/+0x44 = 0 (no slide-out), +0x3c = 0, no close test. 11 items, 7 rows.
Items: one-char strings in font 30 KeyboardHelp (Pal, never halved), lifetime 0, mode 0,
normal effect 0 blink −500, highlighted effect 3 blink 2:
| items | char | x, y | align | lands |
|---|---|---|---|---|
| 0, 1 | ';' | 5, 2 | 0x68 | x = W−5−w, y = H/2+2 |
| 2, 3 | '9' | 5, 2 | 0xc8 | x = W−5−w, y = H/2−2−h |
| 7, 8 | '5' | 5, 28 | 0x68 | y = H/2+28 |
| 9, 10 | '4' | 5, 28 | 0xc8 | y = H/2−28−h |
| 4, 5 | 0x83 (F1 key cap) | 5, 5 | 0x88 | bottom right, y = H−5−h |
| 6 | — | — | — | bare base object (never drawn), hotkey holder |
(';' '9' '5' '4' = key caps Down, Up, PgDn, PgUp [guess from the actions].)
Rows (0x433c90; hotkeys are VK codes; rows 0–2, 5, 6 selectable with the shared callbacks
and cursor 0xe4; rows 3, 4 static, hotkey only):
| row | items | hotkeys | action |
|---|---|---|---|
| 0 | 0, 1 | Down, Right | 0x43df70 next page (if page < count−1: 0x48b190(page+1)) |
| 1 | 2, 3 | Up, Left | 0x43df40 previous page |
| 2 | 4, 5 | F1, Escape, Return | 0x43e040 close |
| 3 | 6 | Home | 0x43def0 first page |
| 4 | 6 | End | 0x43df10 last page |
| 5 | 7, 8 | PgDn | 0x43dff0 page += 5 (≤ count−1) |
| 6 | 9, 10 | PgUp | 0x43dfa0 page −= 5 (≥ 0) |
No own sounds (0x29 from the enter callback only). **Close 0x43e040** [asm]: +0x20 = 1,
delete 0x4f15f8 (0x48a440 + free, = 0), 0x4096e0(0) + 0x409700(0) resume CD and sounds,
0x470fc0 releases the player's inputs; then 0x435620: 0x4354e0 (sound 0x28, no slide),
free, cursor arrow. **Not mapped**: the Help.gen page layout and the elements' slide
(0x48b190 / 0x48a000 / 0x48a150 / 0x489de0) — the .GEN engine.

##### F1 in the port (written 2026-10-30; `dialog.c` `dialog_help`, `generic.c` `help_*`)

The page layout the first read left open: Help.gen is a script of 29 one-line blocks (`[L0]` no
life, `[W50]` the key's width, `[FKeyboardHelp]`, `[FHelp]`, the text in `\(...\)`). 0x48b120 sets
+0x1c = 1, and with it 0x48a4a0 and 0x48b190 lay the blocks **one under the other**: 0x48a000(block,
y, &next) puts the lines from y, next = y + the lines' heights (+ the [V] offset, none here).
0x48b190(page) stacks them again from 0 and then takes the first line's y of the block asked for
off every piece, so that block is at the top of the screen and the others follow it: it is one
long list that scrolls by whole blocks, not pages. A block without lines: nothing moves (returns
0, +0x20 kept). 0x43deb0 draws every block each frame through 0x48a150(1): with 1 no piece is ever
marked seen (0x417350), so +0xa4 stays 1 and pieces off the screen stay alive. No block has [D],
[MX], [MY].

The rows are as in §5a; the rows 3 and 4 (Home, End) have item 6, a bare object that is never
drawn or hit, and +0x58 = 0: the hotkey search of 0x433ec0 has no test of +0x58 at all, so
any row's hotkeys answer. All the help's actions return 1 (asm), so a key they take never reaches
the race. No row is chosen at first, so the pointer is the "no" circle until it is on a key.

`race_frame` now takes 0x47ff90's flags from the dialog that holds the race (`dialog_flags`): bit 3
the dials (and their digits), bit 6 the dashboard and all texts, else bit 5 the list 0x53ba68 only.
Normal −2, the race's dialogs −10, the help −0x4a. So the pause menu has no dials, which the
original's capture always showed (the port drew them until now).

Two things found while comparing, both also true of the pause menu:
- +0x1c: a dialog's first draw empties the message list 0x53ba68, so "GET READY" goes with it
  (`text_clear_list(1)`; the box object 0x417b90 put in the list for new messages to dodge is not
  written, as the dodging itself is not).
- A dialog's items are 0x416170 + 0x416300, then 0x433a30 and slot 6: **no line's gap in their
  height**, unlike 0x417d20's text objects (`TextArgs.no_gap`). Seen as the key caps aligned to
  their bottom edge one pixel too high, and the pause menu's box one row too tall.

Compared with the original (F1, PgDn, Down, End, a run under wine, 2026-10-30): the list to the
pixel on all four (x < 210; 4..183 pixels of sky and sea differ with the race's moment), the five
key caps to the pixel, the pause menu's box and rows to the pixel. Closing puts the dashboard back
and the race goes on. Not compared: the blinking cap under the pointer (the capture shows both
states of a blink, and the pointer not at all).

##### Message box 0x43c570 (slot 0x4e8910)
`0x43c570(title, header, horiz, modal, box, kind, deflt, count, btn[])`, btn = 9-byte
records (char *text, u8 char, i32 value); 0x43c4e0 = the same with the buttons on the
stack. Fields: +0x10 = +0x14 = +0xc = modal, +8 = modal | box, +0 = kind, +0x3c = 1,
+0x1c = 1, slide −1/0, +0x28 = 1. Rows count+2, items 2·count+2. **No initial selection**
(Return does nothing until an arrow or the mouse picks a row) [decomp].
- Row 1 header: font 32 (→ 33), (0, 0), align 0xc2 (bottom edge at H/2), effect 0, mode 0.
- Row 0 title: font 12 Messages (AntiAlias8Transp, never halved, its own colours
  255,127,10 / 200,100,0) at (0, h_hdr + 5) align 0xc2, **effect 3 blinking 333 ms**.
- Button i (row 2+i, items 2+2i / 3+2i): font 32, align 0x62, effect 0 both (normal fore
  green 0x4f0df4 back yellow, highlight fore red back yellow: Pal font → **the selected
  button looks like the others**). First at (0, 0); then y += h + 10 (horiz 0) or x += w +
  10, measured with font 32 and doubled under SmallFonts (so 640×480 default: step h+5).
- Hotkeys: default row (i == deflt) n = 3 {0, Escape, char}, others n = 1 {char}; the
  supplied letters are lower-cased by 0x433dd0 so never match: in practice the auto letter
  (+ Escape on the default row). Callbacks shared, row +0x3c = value, cursor 0xe4.
- Action **0x43cf60**: +0x20 = 1, **0x4e891c = value**; next frame race vtable +0x1c
  0x47ec50 (display mode change). Close 0x435810 → 0x4354e0 (0x28, slide), arrow cursor.
Callers: **0x43c4e0 from Game_Frame**, D3D only ("[WARNING]", the two video-memory texts,
not modal, close test 0x47dab0, button "[OK]") — never in a software port.
**0x43c2b0 = the F12 display-mode list**: title text 43 "SCREEN MODE : ", header
"[ ]******[   X    Y  ]" (0x4e8b10), horiz 0, modal = Solo, box 1, kind 0xd; one button per
16-bit DirectDraw mode (records 0x24 bytes at 0x5d8960, count 0x4ee984): "[%c]******[%4d X
%-4d]" (0x4e8b28), letter from 'A' up skipping used letters, stopping before 'X' (past 'Z'
→ ' '); value = index | 0xffff0000; last "[CANCEL]" (103), value −1, the default (Escape).
With default keys 'A' is taken, so the first mode is [B] [decomp; not run].

##### Link only
- **0x438780** (from 0x410ee0 at race start, skipped in Solo): "WAITING FOR\n
  SYNCHRONIZATION..." (130, font 9, (4,4) align 0x81, effect 4) + [CANCEL] (bottom right),
  slot 0x4e8904 **kind 9** (the joystick exception), +8 = 0, own draw 0x438e20.
- **0x43b700** Link Escape menu (texts 165–170): only when not Solo. Not mapped.

---------------------------------------------------------------------------------------------
#### 5b. F11 sound / F12 display menus (slot 0x4e8908) (helper read; tags as given)

##### Keys (0x431250, asm 0x432200..0x4325a3)
- **F11** (not with Ctrl or Shift): no 0x4e8908 → **0x43a400(0)**; 0x4e8908 of kind 3 (the
  sound menu) → close it (0x4354e0, free, arrow cursor): a toggle; another kind → close,
  then 0x43a400(0).
- **F12**: Ctrl → nothing. **Shift+F12** (unless 0x4dc1cc): 0x4e8910 of kind 0xd → close,
  else **0x43c2b0** (the mode list, §5a). **F12 alone**: 0x4e8908 of kind 1 → close; other
  kind → close and 0x438f70(0); none → 0x438f70(0).
- 0x4587e0: bit 0x200000 → 0x438f70(0), else 0x400000 → 0x43a400(0) (the menus' own
  SND/DISP rows). Argument always 0 → no close test.

##### Common [decomp, field writes checked]
- **Not modal**: +0x10 = 0, +0x14 = 0, **+0xc = 0** (arrows/Return not handled; keys that
  are not a row hotkey go to the race), **+8 = 0 no box**, +0x1c = 0, +0x3c = 1, +0x38 =
  0x43a290 (sets +0x20), slide −1/0 on close (sound 0x28). No opening sound, **no first
  selection** (no 0x29).
- Font 32 (→ 33 h 9), Pal. Two items a row: **short tag** (row+4): 0x416170(x −60, y Y, 0,
  align **0x40**, effect 0, **mode 1**, blink 0, vx 1, vy 0, target (0, Y), s 4.0, **k 7.0**),
  flying in from the left to the left screen edge; **long text** (row+8 = +0xc, shown while
  hovered): (0, Y, 0, 0x40, effect 0, mode 0, blink −500), no blinking.
- Rows 0x433c90(row, short, long, long, short, n = 0): hotkey = first free letter of the
  short tag. Callbacks: leave 0x4361c0, enter 0x436150 (snap, long item, 0x29), **+0x1c =
  the action (runs on button down)**, +0x20 = 0, +0x2c = the action (hotkey / Return — but
  Return needs +0xc, so hotkeys and the mouse only). F12 rows also +0x24 = 0x43a350
  (toggle SmallFonts: 0x4866d0) — never called by 0x433ec0 [guess: dead]. Cursor 0xe4.
- Align 0x40 = vertically centred, x as is: with SmallFonts at 640×480 y = 235 + Y/2,
  tags fly from x = −30 to 0. (SmallFonts 0: 231 + Y, from −60.) [calc]
- Check boxes (+0x60 = 1, +0x64 = int*): the first char of the drawn text becomes ' ' (0)
  or '^' (set) — for "[  MOTO]"/"[  AMB]" it replaces the '[' [decomp].

##### F12 display menu 0x438f70 (kind 1; row 5 empty)
| row | Y | short (id) | long (id) | hotkey* | action | check |
|---|---|---|---|---|---|---|
| 0 | −20 | "  L]" 138 | "  LOW RESOLUTION 320X240]" 141 | L | 0x43a2d0: 0x4dc0c8 = 0, 0x4dc0cc = 0x12c00, 0x4e9ce4 = 1 | – |
| 1 | −40 | "  M]" 137 | "  MEDIUM RESOLUTION 512X384]" 140 | M | 0x43a300: mode 1, 0x30000, re-init | – |
| 2 | −60 | "  H]" 136 | "  HIGH RESOLUTION 640X480]" 139 | H | 0x43a320: mode 2, 0x4b000, re-init | – |
| 3 | 10 | "  B]" 142 | "  LETTER BOXED FORMAT]" 143 | B | 0x43a370: re-init, 0x4dc0d4 ^= 1 | 0x4dc0d4 |
| 4 | 50 | "  FS]" 147 | "  FULL SCREEN]" 148 | F | 0x43a3a0: re-init, 0x4dbff0 ^= 1 | 0x4dbff0 |
| 6 | 80 | "  SND]" 149 | "  SOUND MENU]" 150 | S | 0x43a3d0: done, 0x4e8918 = 0x400000 | – |
| 7 | 100 | "  HIDE]" 151 | "  HIDE MENU]" 152 | I (H taken) | 0x43a2b0: done | – |
Long items red/yellow, short none (Pal: invisible anyway). Texts 144–146 (D3D) loaded but
unused. Screen y (SmallFonts, 640×480): H 205, M 215, L 225, B 240, FS 260, SND 275,
HIDE 285 [calc]. *Hotkeys with the default control keys.

##### F11 sound menu 0x43a400 (kind 3; no colour calls)
| row | Y | short (id) | long (id) | hotkey* | action | check |
|---|---|---|---|---|---|---|
| 0 | −50 | "  CD]" 153 | "  CD AUDIO TRACKS]" 154 | C | 0x43b680 → 0x409410 CD music on/off | 0x4db66c |
| 1 | 0 | "[  MOTO]" 157 | "[  MOTO SFX]" 158 | M | 0x43b690 → 0x408900 toggles 0x4db65c (message text 134, 2 s) | 0x4db65c |
| 2 | −20 | "  SND]" 155 | "  SOUNDS]" 156 | S | 0x43b6a0 → 0x408b30 sound server on/off | 0x4db664 |
| 3 | 50 | "  VOICE]" 161 | "  SAMPLEDS VOICES]" 162 | V | 0x43b6b0 → 0x408f30 (voices [guess]) | 0x4db668 |
| 4 | 80 | "  DISP]" 163 | "  DISPLAY MENU]" 164 | D | 0x43b6d0: done, 0x4e8918 = 0x200000 | – |
| 5 | 100 | "  HIDE]" 151 | "  HIDE MENU]" 152 | H | 0x43a2b0: done | – |
| 6 | 20 | "[  AMB]" 159 | "[  AMBIENT SOUNDS]" 160 | B | 0x43b6c0 → 0x408800 toggles 0x4db660, stops the ambience | 0x4db660 |
Screen y (SmallFonts, 640×480): CD 210, SND 225, MOTO 235, AMB 245, VOICE 260, DISP 275,
HIDE 285 [calc]. Note: the menu ties MOTO to 0x4db65c and AMB to 0x4db660; NOTES §4.14.5's
config table has them the other way round — Game_Frame runs the engine sounds only when
0x4db65c is set, so **the config table's labels look swapped** [guess, to check].

##### Shift+F12 mode list → 0x4e891c → 0x47ec50
Value with high word set, index < count, 16 bpp and **both** w ≠ W **and** h ≠ H →
0x4dc0cc = pixel count, 0x4dc0c8 = mode, 0x4dc0d0 = index, 0x4e9ce4 = 1 (quirk: a mode
sharing the width or the height with the current one is refused).

##### In the port (written 2026-10-30; `dialog.c` `dialog_side`, `race_sound.c` `race_sound_toggle_*`)
The rows are as in the two tables. What the table's actions do in the race:
- **0x408b30** (SND, Ctrl+S): no server → text 134 ("no sound") with 0x4dbb20, 2 s, y 30, effect
  3, font 0x20. Else 0x4db664 ^= 1, and **0x4db65c and 0x4db660 take the same value** (so the MOTO
  and AMB ticks follow it, seen); switching on starts the engines and the ambience afresh and
  forgets the last voice and effect. Message text 120 on / 121 off in slot 0x4e98ec (2 s, align
  0x22, y 20, FadeOut, font 12), which replaces the one before.
- **0x408f30** (VOICE, Ctrl+V): 0x4db668 ^= 1, texts 122 / 123. **0x409410** (CD, Ctrl+A): the
  music, no CD → text 135 as for 134; texts 124 / 125. **0x408900** (MOTO): 0x4db65c ^= 1, no
  message, engines stopped or started afresh. **0x408800** (AMB): 0x4db660 ^= 1, the objects
  stopped or armed again, no message.
- These switches are written to CONFIG.BPT at the end as the options' are (so a test that presses
  one must start from a scratch home).
- The two columns in the three orders: keys and mouse side, help, race (0x4329a5); each frame's
  hotkeys help, race, side (0x4126a0); drawn side, race, help (0x47ff90). The column change
  (0x4e8918's bits, 0x4587e0) waits for the next frame in `dialog_frame`, as the close's slide
  does not: the old column is freed at once when the key F11 or F12 does it, and on `done` when a
  row did.

What the port does (2026-10-31): **L, M, H** change the size (`race_ask_screen`, done at the
next frame's start as 0x4e9ce4 does), **B** the 16:9 band with its tick; every dialog item is
placed again from its authored place by the 0x4165c0 rule (`dialog_screen`, `item_place`); the
menus stay 640x480. **F2, F3, F4** the same keys. **Shift+F12**: the game's list of Windows'
modes shows the three sizes the port has in place of whatever Windows offers, a mode sharing
width or height with the present one refused as the game does (`act_mode`). **FS** is SDL's
desktop full screen, its tick from the port's state: off at the start, as a window under wine
shows it. Compared at 512x384 and 320x240, and 16:9 at 640 and 320, against captures: picture,
column, pause box, help, messages in the same places, the differences left being what moves.
Getting the keys there: the hidden Weston stops giving the original's window the keyboard at
320x240, so `tools/win/keypost.c` posts them from inside wine, 2 s apart.

A column open across races [tested 2026-10-31, memory reads of 0x4e8908 and pictures]: **a restart keeps
it, leaving the race closes it, in the original and the port alike.** RESTART (0x411010, no unload): the slot
stays, the column shown (0 pixels different in x 0..40, y 200..300 sixteen seconds on). QUIT RACE: 0x47d620 ->
0x4113c0 -> 0x435770 frees it, the slot 0 at the Top 5. This note used to say the port differs: it never did.

Compared with the original (F11, F12, the pointer on MOTO, on MEDIUM; a run under wine,
2026-10-30, again after merging the roadside objects): **the tags and long texts to the pixel**:
what differs in the left column is the boat and birds that move with the race (54 and 12 pixels)
and, in the hover pictures, the port's pointer alone (about 175 pixels in a 15x25 box, the capture
never shows it). Under the Continue dialog neither the original nor the port opens a column. Clicked in the port only: SND takes MOTO's and AMB's ticks with it and
says SOUND OFF; DISP changes column; HIDE closes; the same key closes; the other key changes
column. Not compared: the messages' moment and the pointer itself (a capture never shows it).

### 4.20 The race view: cameras, dashboard, drawing order (mapped 2026-10-26)

Moto Racer GOG 3.22 `game.EXE`. Addresses are the program's (base 0x400000).
Marks: [asm] read from the disassembly, [decomp] from Ghidra's C only, [guess],
[px] checked against a capture, [test] checked numerically against memory dumps
in `build/trackcap/` (the headless original, mostly attract mode).

Bike object O(k) = 0x581218 + k*0x3209, k 1-based (player index 0x508834; the
captures have k = 2). Bikes 1 .. [0x509bac]-1. Camera block CAM = 0x5161c0 +
i*0x238 (i = [0x4dc0f8], always set to 0 before the camera and drawing steps;
there is only one camera in use per frame).

---------------------------------------------------------------------------
#### 1. The frame

##### 1.1 Game_Frame 0x47d660 (the race's App_OnIdle step) [asm]

Order:
1. One-shot message boxes (0x4e4f00; 0x4e4f08/0x580a58 -> 0x43c4e0).
2. [0x4f0c4c] = [0x4dc14c]; [0x4dc14c] = 0 (the frame's "new text" flag passed to 0x4841c0 at the end).
3. 0x4120e0: the clock. ms = now - last, >200 -> 200, <=0 -> 30. Then the text
   lists' clocks (0x53ba68, 0x53b8e8).
4. 0x4126a0: the in-race windows (pause/quit, objects 0x4e8904..0x4e8910). If it
   returns 0: 0x411a10 and the rest of the step is skipped (the window code
   draws and presents on its own) [asm; the window side not followed].
5. [0x5429b0] = 1 (keys allowed).
6. 0x4121e0: demo-story texts; returns 0 -> skip.
7. race vtable +0x10 (0x47ec40).
8. [0x4e9810] = ftol(ms); [0x519fec] -= ms.
9. 0x47dae0: HUD messages and timers.
10. 0x47df40: dt = ms*0.1 (at least 1.0) into [0x4e9808]; the bike steps
    (0x45d540 / 0x45e940) - the physics helper's part.
11. vtable +0x18 = 0x47ecd0(dt).
12. Link only: 0x457050, 0x456380 (network, out of scope).
13. 0x48b7a0(0x5f6778, dt): animated decor (AnimDecor).
14. 0x459000 (rdtsc profiling - ignore; 0x459000/0x459090/0x459130/0x4591b0 all are).
15. 0x457590(dt).
16. 0x47e3d0 (laps, checkpoints), 0x47e1e0.
17. [0x4dc0f8] = 0; **0x4583e0(dt) chooses the camera mode; 0x458620(dt) moves the camera** (§2).
18. If [0x4dc138]: 0x467b30(O(k), 1) for every loaded bike (LOD + pose, §4.6).
19. Sounds: 0x4079e0 (if [0x4db65c]), 0x405f30.
20. Solo: when all have finished: [0x509ba0] = 1 and the voices 0x47/0x48/0x4f/0x50 via 0x404da0.
21. 0x412a50 race end (the race-rules helper's part); maybe 0x407420.
22. **vtable +0x20 = 0x47ff90(dt, flags = 0xfffffffe)**: everything drawn (§1.2).
23. 0x459fd0: frame limiter [asm]: waits (timeGetTime) until 1000/[0x4dc200] ms
    have passed since the last frame; [0x4dc200] = 30 by default, set by the
    command-line switch `-FrameRateMax<n>` (21..98 accepted, 0 = no limit; parsed in
    0x414080 at 0x414c4e). GOG's launcher writes `-FrameRateMax60` into
    motoracer.ini, so the GOG game runs at most 60 frames a second; the 1997 CD
    without the switch at most 30. It then keeps the last 24 frame times
    (0x5809f8..) for an average (the info line).
24. [0x4e8918] = [0x4e891c] = 0.

Race vtable 0x4d04c8: +0 0x47bf30, +4 0x47b420, +8 0x47bf70, +0xc 0x47d620,
+0x10 0x47ec40, +0x14 0x47d660, +0x18 0x47ecd0, +0x1c 0x47ec50, +0x20 0x47ff90.

##### 1.2 The drawing 0x47ff90(dt, flags) [asm]

flags is always 0xfffffffe from Game_Frame (bit 0 clear, everything else set).
[0x5dafd8] = the Direct3D flag (0 = software, the port's case).

1. [0x5dbc88] = [0x5ef058] = [0x4dc1e0] = 0 (per-frame counters).
2. Detail level [0x4dc098] (0..4) picks columns 6..8 of the detail table into
   [0x6d20c8], [0x62435c], [0x6d20d0] (NOTES §4.13 detail remap).
3. race+0x18 = 0. (flags & 1 would call 0x499fb0/0x499fd0: never.)
4. 0x4816c0: the HUD fonts, numbers 20, 7, 4, 5, 11, 14, 6 -> 0x4f0d64 .. 0x4f0d7c.
5. [0x4f0dc4]++ (frame counter).
6. D3D only: 0x423870 / 0x4236b0.
7. Software (flags & 2): **0x402960 the horizon, drawn before the lock** (it writes
   the whole background).
8. **0x49a160(0, d3d)**: lock the surface, origin = surface + (top [0x62ebec] *
   pitch [0x6d1040] + left [0x624374]) * 2; reset the depth buckets and the
   s-buffer row lists for rows 0 .. [0x62e448].
9. Which section the camera is in:
   - if a TV kind ([0x4e9834] || [0x4e8968] || [0x504ea0]) and trackside cameras exist
     ([0x51a40c] > 0): 0x40c740(campos) (search the whole track);
   - else 0x40c7e0(hint = O(k)+0x31c1, campos) (search near the bike's section).
10. 0x46e320(CAM, far = LenBPlane [0x51d1bc], 0.6f [0x4dbfe4]): the frustum planes
    CAM+0xd4..+0x110 for the visibility tests.
11. 0x40ce60(track 0x52c108, section, CAM, list 0x5dabd8): the track sections seen,
    count -> [0x5dabcc] (NOTES §4.13, already in the port as land.c).
12. flags & 6 == 6 and DrawLand [0x4e9900]: **0x458b80(list, count)**: bikes, their
    shadows (§4.1).
13. 0x4575c0 the particles (smoke/dust billboards), 0x4573e0 the skid-mark strips
    (§4.4).
14. flags & 8: 0x484810(1), 0x484810(2) (the dials' 3D parts, see §3b), 0x40f2d0,
    0x486310, 0x47b7a0(race, 0, dt).
15. Software: **0x49ad90 renders the 3D list** (buckets far to near through the
    s-buffer, NOTES §4.9).
16. flags & 0x40: if race+0xc (always 1 in a race): the HUD **0x4806e0** (or
    0x4804d0 when the window [0x4e8904] has its +0x14 set); else if flags & 0x20:
    0x418860(0x53ba68) only the text list.
17. flags & 8: 0x484810(4) (the dials' 2D parts).
18. [0x4dc124]: 0x483e90 the debug lines ('VERS 3.21 ...').
19. 0x434ae0: the in-race windows 0x4e8908, 0x4e8904, 0x4e890c, 0x4e8910.
20. flags & 0x10 && [0x4f15f4] && ![0x4f15f8]: 0x48acd0.
21. 0x4841c0([0x4f0c4c], ...).
22. D3D: 0x423590 / 0x49ad90.
23. **0x49b0e0(0, d3d)**: fill the spans left free with colour [0x65f0d8] - only
    when [0x625748] is set, and only 0x49a560 (the demo story) sets it - then unlock.
24. D3D: 0x423940.
25. 0x471440(0x5d2dd8).
26. **0x49b1c0: present** (0x419900, the flip/blit).
27. If [0x4e9ce4] (F2/F3/F4 asked for a new screen): 0x4592a0([0x4dc0c8],
    [0x4dbff0], [0x4dc0d4], [0x4dc028]) sets the screen up again; [0x4e9ce4] = 0.
28. [0x4e981c]++.

So the order on screen is: horizon (full background), track + bikes + shadows +
other 3D through the bucket/s-buffer renderer, then the HUD and texts on top, then
the windows, then present.

(Screen sizes, viewports and the other draw calls: §1b.)

---------------------------------------------------------------------------
#### 1b. Screen sizes, viewports, other draw calls

**0x4592a0(mode 0x4dc0c8, FullScreen 0x4dbff0, Mode_169 0x4dc0d4, 0x4dc028)** [decomp + asm]. It is called by 0x410e10 at race load, and by the race draw when 0x4e9ce4 is set (F2/F3/F4 or Alt+Enter).
- **Sizes:** 320x240, 512x384, 640x480, with 0x4dc0cc = W·H. In fullscreen the size goes through the display-mode table; if the switch fails it falls back to 640x480.
- **Focal:** 0x49a0e0((float)(W·394/640)) and 0x49a100((float)(H·394/480)), with integer division. That gives 394, 315 or 197.
  - The raw values go to 0x6d20cc and 0x709360. Multiplied by the zoom 0x6d1f74 they go to 0x6d20c4 and 0x6d104c.
  - The full height is used even in 16:9.
- **0x410e10 also sets:**
  - near plane 0x625774 = 64.0
  - 0x625788 = 10000.0 and 0x62e438 = 2000.0
  - far plane 0x6d1f70 from the detail table 0x51d1c0[detail·9]
- **Viewport, 0x499ba0(0, 0, W, H, wide, dbl)** [asm]:
  - x0 at 0x624374 is 0; the top is at 0x6d1040.
  - Width goes to 0x624f58, 0x655d10 and 0x5d93bc; height goes to 0x68b008, 0x624b18 and 0x5d9c20.
  - Centre: 0x60d3c4 = W>>1 and 0x624f88 = top + (h>>1).
  - Clip edges: 0x62e448 = W−1 and 0x625784 = top + h − 1.
  - Row table: 0x62e468[y] = (0x4f1a3c + y)·pitch, with the pitch in 0x62ebec.
  - 0x473a00 copies x0, top, W and h into 0x5d9a78, 0x5d893c, 0x5d93c8 and 0x5d8948.
- **F4, 16:9 (Mode_169):**
  - The picture height becomes ftol((W/16·9) × ((4/3)/(W/H))) = W·9/16, i.e. 360, 288 or 180. The 4/3 is the double at 0x4d1f78.
  - In the software path the top stays at 0 and 0x4f1a3c stays 0. The centring offset is only set in the DirectDraw path when 0x53c524 is set.
- **Clearing the screen:** when the draw flags have bit 1, 0x499fb0(1) → 0x476540 restores a lost surface and fills it with black. It fills the whole surface, or only the bands above and below when 0x5d893c is not 0. It then does a GetDC/ReleaseDC pair (0x499fd0). For the port this is just "clear to black".
- **"Every other line" mode (0x4dc028 → 0x624b14)** would render at half height with double pitch, keeping needles and text on even rows. It is **dead in this build**: 0x4dc028 is 0 in the EXE, and its only writers copy it to or from 0x50487c or set it to 0 [asm].
- **What depends on the screen size:**
  - HUD positions measured in pixels from the corners (dials at W−102, bottom−74)
  - the turn signs, scaled by W/640 and h/480
  - Font_Get's low-res/hi-res choice
  - 0x493f20 halving text coordinates unless at 640x480 with SmallFonts 0
  - the focal

**There is no split screen in this build [asm].**
- 0x499ba0 is only ever called with (0, 0, full size): from the menu (0x409f70), from 0x4592a0, and from video init (0x4995a0).
- Four camera blocks (0x5161c0 + i·0x238) are set up in a loop over 0x4dc0f8 = 0..3 at race start (0x47bf70). 0x4dc0f8 is then set back to 0, and the race draw uses block 0.
- 0x4dc0fc is the one viewed player: 1 in a demo, otherwise the last slot with +4 and +8 set.
- 0x4ea4c4 is never written; it is a constant 1.
- Bike field +0x31dd is a camera-block index used for distance to the camera (0x45f160), not a viewport.
- The string "PLAYERS" (0x4e99b0) has no references.
- So two players in this build means Link (DirectPlay) only, which conflicts with the CLAUDE.md scope "one or two players on one PC". That needs Danik's decision.

**The other draw calls** [asm unless marked]:
- **0x4575c0, dust and smoke:** 128 sprite records at 0x52c498 + i·0x104 (active flag at +0xfc), aged by 0x457590, drawn through 0x4b0bec → 0x49b2c0/0x49b7f0 with camera 0. Visible in play; the port needs it.
- **0x4573e0, tyre marks:** 16 strips at 0x542a28 + i·0x1608.
  - Each holds up to 62 faces and 128 points; the counts are at +0x1600 and +0x1604.
  - 0x457200 adds two points per call, on the ground plus 2.0. It is called from the rider routine 0x466d70 through 0x457390.
  - The port needs it.
- **0x40f2d0, track map:** controlled by MapMode 0x4dbdac (F9; 4 means off, the GOG default).
  - It draws the track object's map model (+0x28 faces, +0x2c vertices): first face white, the rest grey (8,8,8) or (16,16,16), plus rider markers of size 50 or 30.
  - It is placed in camera space, easing toward (−2000, −1200, 4000) for modes 0 and 2 (−500 instead of −1200 in 16:9), or (0, 0, 2000 or 2500) for modes 1 and 3.
  - Modes 0 and 1 turn with the player's heading.
  - It is inserted with 0x49c590 at key 15.0 (0x40f710: pushes 0, 2, verts, 15.0, face). The 2 is the fourth argument, which only Direct3D reads; the fifth is 0, so in software it goes in the bucket of its key, not bucket 0x1f3f as this note said before.
  - **Made by 0x40ed10** at the race's load (0x40e147) and at each set-up of the screen (0x4592a0, which also puts the place back): one piece for every 4th section, a ribbon 1024 either side along the section's axis 0, flat (map y = the world's z), the last pair of points the first again, centred on its box and scaled by 1280 / the box's half diagonal. Kind 12: half of what is behind + the colour (`rgb` 0x45a2e0), which is why it looks see-through and teal.
  - The riders are triangles (0x40f7cd) from their heading (0x40fd40), red and orange by twos at key 10, the followed one yellow at key 0. Not drawn under the winner's credits (0x504eac).
  - In the port: `trackmap.c`, drawn after the dials as here. F9 is read before the pause test, as the game has no such test. The mode is saved. **Compared with the original** (Practice, Speed Bay, 3 laps; the bike standing): y >= 225 has 0 pixels different in all five modes (mode 3's map crosses the middle of the screen, what differs above is the palm, the gull and the digits). A Single race during the countdown (F9 at a picture not the same as the capture's), modes 0, 1, 2: 65, 262 and 1375 pixels different for y >= 225, of which the count's digit is one second off in modes 1 and 2 and "4 LAPS" has dropped in mode 2; the riders' marks and their colours are the same by eye, not counted one by one. Under F2/F3/F4 `trackmap_build()` must be called again (`racekeys_new_screen()`).
- **0x486310, dial face and turn signs:**
  - The face is described in 3b.
  - Signs: slots 1..6 hold the six sign models, flat-coloured, centred near (319,79) at 640x480.
  - 0x486240 arms a sign with timer 75.0 from a track section's event code 0..5 (16..21 when racing in reverse).
  - Each picture the timer drops by dt. The sign shows when ftol(timer·0.1) is even, i.e. it blinks every 100 ms.
  - Signs are hidden when 0x4e9834 is 1 or 2, when DisplaySignal 0x4f0e14 (F8) is off, or during the victory lap (0x4f15f4).
- **0x47b7a0, finish-place figure:** the models 1RST, 2ND and 3RD (.3DF/.MAP) from `<track>\FINAL\`, loaded by 0x47b600.
  - Shown when the mode 0x4dc094 is not practice, the player's place (+0x2f85) is 0..2, and the state (+0x2de5) is 2.
  - It flies in to a distance of 2500 and then wobbles. The port needs it.
- **0x48acd0:** steps the victory "generic" sequence 0x48ae40 while 0x4f15f4 is set.
- **0x434ae0:** draws the four in-race dialog slots 0x4e8904..0x4e8910 (already covered in NOTES part B).
- **0x484380:** turns on or off a 6-second text in the bottom-left corner, `[ WxH] / [ VIDEO x.xxM TXT x.xxM]`. It appears on every screen set-up and on F1; it is the red text in q14. Optional for the port.
- **0x483e90 and 0x4841c0:** debug only (0x4dc124).
- **0x471440:** a timed screenshot/video capture. Drop it.
- **0x49b1c0:** clears the per-frame flags and calls 0x419900 (the flip/blit to the screen: present), then a pending palette/surface update (0x499ed0) [asm].

**Rough C size:** dials ~250 lines; HUD slots, signs and face ~120; screen set-up and viewport ~100; tyre marks ~80; dust ~60. The drawing itself is r3d's.

##### 1b.1 Open points (screens, dials, extras)
1. Settled (captures of F4 at all three sizes, 2026-10-31): the 16:9 picture sits at the **top**, the rest is black; under wine the window shrinks to W x the view's height.
2. [guess] font+0x774 is the width of 'O'.
3. What 0x4e9834 is (it hides the signs).
4. How the track map, put in the farthest bucket, ends up visible.
5. The finish figure's float constants (0x4d18e0..0x4d18fc) are not decoded.
6. Two players on one PC does not exist in the original, only over Link. Danik needs to decide the scope.
7. The tool has not matched q19 yet: its true speed and gear are unknown, and with a guess of speed 0 and gear 1, 167 pixels still differ.

---------------------------------------------------------------------------
#### 2. The cameras

##### 2.1 The camera block (0x238 bytes) [asm]

| off | what |
|-----|------|
| +0 | B = the followed bike's body sub-object (O(k)+0x24) |
| +4 | k, the followed bike (−1: none) |
| +8 | mode: 7 chase, 6 on-board/bike-mounted, 3 look-at from a fixed point, 1 TV director |
| +0xc | state counter (5, then 2 inside the chase step) |
| +0x28 | TV orbit angle |
| +0x34 | finish orbit flag (bike finished: the camera swings round to the front) |
| +0x38,+0x3c,+0x40 | angles a,b,c (b = yaw +0x3c, set to π by Tab) |
| +0x50,+0x54,+0x58 | offset (0, 191, −484) |
| +0x68 | C, 3x3 rotation (world->view rows), +0x8c T (view translation) |
| +0x98 | position |
| +0xa4 | target |
| +0xc0 | roll angle, from 0x490220(C) |
| +0xc8 | the chase's lerped position (before the distance correction) |
| +0xd4..+0x110 | frustum planes (0x46e320) |
| +0x114,+0x118 | MediumZone, HightZone |
| +0x11c + 12i | 8 preset offsets: (0,−160,1680) (0,160,−20) (−250,150,0) (250,150,0) (−70,120,200) (70,120,200) (0,400,400) (0,500,500) |
| +0x17c + 12i | 8 preset angles: (0,0,0) (0,π,0) (0,π/2,0) (0,−π/2,0) (0,π,0) (0,π,0) (−π/4,π,0) (−π/4,π,0) |
| +0x1dc + 4i | 8 floats, all 64.0 (the chase height) |
| +0x21c | preset index; ≠ 0 = bike-mounted preset camera |
| +0x220 | chase distance index (F5), 0..3 |
| +0x224 + 4j | extra distance: 0, 128, 256, 512 |

Reset 0x46d760. Per race 0x410120: 0x46da40 sets angles (0,0,0), offset
(0,191,−484); the light vector [0x52c228] = (−0.5, −1.0, 0); [0x4dc130] = 1;
[0x4dc0b4] = Solo. [test] presets, extra distances and 64.0 read identically in all
dumps.

Helpers [asm]: 0x410010(out, v, M) = v·M + T (row vector times the 3x3, plus T);
0x40ffc0 add; 0x40ff80 copy; 0x46d590(out, v, d) = v·(1/d); 0x403a80(out, A, B)
= A then B (A·B); 0x40fd40 builds R(a,b,c); 0x40fe80 rigid inverse; 0x403710
identity; 0x4039c0 / 0x403a20 copies.

**Look-at 0x46e140(CAM)** [asm][test]: u = unit(target − pos), h = sqrt(ux²+uz²);
A = [[uz/h,0,ux/h],[0,1,0],[−ux/h,0,uz/h]], Bp = [[1,0,0],[0,h,uy],[0,−uy,h]],
C = A·Bp (yaw then pitch, no roll); T = 0; +0xc0 = 0x490220(C).
Checked: C equals this look-at from +0x98 to +0xa4 to 5e-8 in every mode-3 dump,
and from +0xc8 to +0xa4 in 18 of 19 chase dumps (see §2.5).

**0x490220(M)** [asm]: F = row2 + T, U = row1 + T; returns atan2((Ux·Fy − Fx·Uy)/|F|,
Fz/|F|) (±π/2 when |F| < 1e-4). For a pure yaw R(0,b,0) it gives −b. Used both
for the roll at +0xc0 and for the bike's heading in the chase.

Trig: the table COS at 0x5fcb80, index = ftol(angle · 2607.594587618272) (16384
steps per 2π); R(0,θ,0) is built inline from the table (the compiler factored
S0·COS0·(1+sin) - same result).

Projection: 0x49a120(z) sets zoom [0x6d1f74] = z and fx [0x6d20c4] = base·z.
[test] fx = 394·zoom in every 640x480 dump (zoom 1.0 or 3.0).

##### 2.2 Choosing the mode 0x4583e0(dt) [asm]

Every frame:
- on-board [0x4dc008] = [0x4dc00c] (F6) && the player's bike not crashed (+0x30c1);
- 0x49a120(1.0) (zoom back to 1);
- ([0x4dc014] is tested but never written: dead.) Then:
  - on-board: CAM+4 = player k, mode 6;
  - else, if the player's O(k)+0x2de5 == 2 (finished): +0x34 = 1, mode 7;
  - else +0x34 = 0, mode 7.
  (The Solo [0x4dc0b4] and [0x4dc020] branches are the same; [0x4dc020] is never
  written, so mode 1 "free debug camera" is dead.)
- Then, if ([0x4e9830] || [0x504ea0] || [0x4e8968] || [0x4dc134] || [0x4e9834] == 1)
  and trackside cameras exist ([0x51a40c] > 0): mode 1 and 0x46e9f0(CAM, dt, k), the
  TV director (§2.6). These flags are the replays/demos/after-finish view
  ([0x504ea0] is set by 0x412960 when the player has finished a single race [guess]).

##### 2.3 Moving it 0x458620(dt) [asm]

- mode 1: only the dead debug path (0x46dde0) - the TV director already placed it.
- else, Solo: the yaw +0x3c returns to 0 by dt·0.02454369260617026 (π/128; double
  at 0x4d0e00 / 0x4d0e08) a frame, from either side, clamped at 0. Then
  **0x4700e0(CAM, dt)**.

##### 2.4 0x4700e0: the mode dispatch [asm]

**Mode 3** (look from where the camera is at a bike): if k ≥ 0: +0 = B,
target = P (the bike's position O(k)+0x30); 0x46e140 look-at from the unchanged
position.

**Mode 6, on-board** (when +0x21c ≠ 0 it goes to the preset path of 0x46f840
instead). X = the wheelie angle O(k)+0x307d. Let M, T be the bike matrix
(O(k)+0xbc / +0xe0) and P its position.
- dirt bikes (PersoAnim [0x509398 + bike·0x124] ≠ 0): new target = (0, 170+10X, 40)·M + T + P,
  new eye = (0, 170−10X, 0)·M + T + P;
- road bikes: target (0, 170+5X, 40), eye (0, 170−5X, 0).
  Floats: 10.0 [0x4d1414], 170.0 [0x4d1418], −10.0 [0x4d1420], 5.0 [0x4d1424],
  −5.0 [0x4d1428], 40.0 [0x4d141c].
- x and z are taken straight (written as (new−old)+old); y is lerped:
  y = old + (new−old)·k with k = 0.5f [0x4d1408] for dirt, 0.7692307692307692
  (double 10/13 [0x4d1430]) for road; for both eye and target.
- look-at 0x46e140; then roll r = lean O(k)+0x2e1d · 0.25f [0x4d13e0]:
  C = C·R(0,0,r) (table sin/cos, inline), T likewise; +0xc0 = 0x490220(C).
- In this mode 0x458b80 draws the cockpit instead of the player's bike (§4.1).
- [2026-10-26, mapped and ported: `bike3d_cockpit*`, `camera.c` on-board] The cockpit's
  two models come from the bike's carac script keys **FileGuidon** (0x4e79f0, the
  handlebar: 3DF/MAP names into 0x5190e0 / 0x519120) and **FileCockpit** (0x4e79e4:
  0x519160 / 0x5191a0), read by 0x429e20 (0x42a0f2 on); loaded by 0x413450 into 0x504ed8
  (handlebar) and 0x51bf60 (cockpit) through 0x420a50(…, −1, 0) (no page offset: their
  faces name **page 7**, the level's `bank\bnk00_07.raw`, the same file on every track,
  one for the road tracks and one for the dirt: gloves top left, the cockpit panel bottom
  right), scale 8.0 (0x4204b0). 0x462110 once a step: the shake g = (pitch × −0.25f
  [float 0x4d1070] − g) × 0.25f [float 0x4d0f88] + g (0x4ea4e4), pitch = 0x45ca00(bike).
  When drawn: X' = wheelie × 2/π (double 0x4d1068); A = (0, X' × −80f [0x4d1074] + susp
  × 10.0 [double 0x4d1078] − 350.0 [double 0x4d1080], susp × 5f [0x4d0f78] + 350f
  [0x4d1088]); Rg = R(g, 0, 0). Handlebar: N = Rg·R(0,0,−lean × 0.4 [double 0x4d1090])·
  R(0, steer × 0.000285714f [0x4d1098], 0), T = A·Rg. Cockpit: N = Rg·Rroll·R(0,0,0),
  T = A·Rg. Both in the camera's space. Their faces go through **0x49bca0**: the farthest
  corner, key = z × 0.2 − near; kind 0 stays, 1 and 0x10 become 0x10, every other kind
  (the flag-0x80000 kind 8 windscreen streaks) **0x2c** = 0x4a57f0, half and half
  (texel 0 skipped, else ((dst>>1)&0x7bef) + ((tex>>1)&0x7bef)). Then 0x49c590 as any
  race face (the detail remap: near faces to the perspective twins 31.., which read the
  face's float texel coordinates, stored by 0x420a50 as (float) of the MAP's ints, the
  args of 0x422ed0 / 0x422d20). Not drawn while Tab looks back (preset), in replays, or
  with the Pocket bikes. [asm, px: equal in look to the original's capture]

**Mode 7** -> 0x46f840, the chase camera.

##### 2.5 The chase camera 0x46f840(CAM, dt) [asm, test]

Inputs: k (+4; < 0 -> return), +0 = B; P = O(k)+0x30; the bike matrix M (+0xbc).

1. **Crashed** (O(k)+0x30c1): target = (P + O(k)+0x30b5)/2 (the bike and the saved
   body point), look-at from the unchanged position. Done.
2. **Preset** (+0x21c = i ≠ 0; Tab = 1 looks back): C = copy of the drawer's
   matrix O(k)+0x2e55; pos = P + preset_off[i]·M; C = inverse(R(preset_ang[i]) · Mbike)
   (0x40fd40, 0x403a80, 0x40fe80). T = 0 in the dumps. [asm]; dumps q6/q12/q24/q35/
   q36/q45/p7 show presets 3/5/6/7 with T = 0, the bike matrix was not dumped so
   C itself is unchecked.
3. **Chase** (preset 0):
   - old = +0xc8 (last frame's lerped position).
   - d = extra[+0x220] + (+0x58 = −484), so −484, −356, −228, +28 with F5.
   - Finish orbit (+0x34): yaw +0x3c += dt·(−0.010471975511965976) (−π/300) while
     yaw > −2.356194490192345 (−3π/4); and d −= (yaw · −0.4244131815783876 (−4/(3π))) ·
     (−484 · −0.5) → d goes from −484 to −726 as yaw goes to −3π/4.
   - ψ = −0x490220(bike matrix): the bike's heading only (lean and pitch ignored).
   - W = R(0,ψ,0) · R(cam angles +0x38..+0x40).
   - E0 = (preset_off[0].x = 0, g = +0x1dc[i] = 64.0, d)·W + T_W + P. (Compiled as
     ((g·W3 + px·W0) + d·W6) + T + P per component.) +0xc = 5.
   - **pos = old + (E0 − old)·0.2f [0x4d1404]** - per frame, **not** scaled by dt:
     the lag depends on the frame rate. The GOG copy is limited to 60 frames a
     second (-FrameRateMax60, §1.1 step 23), so the port's fixed 1/60 s steps
     give GOG's camera exactly; the CD's default limit is 30 (a looser camera).
     `tools/race_camera.py demo`: a bike at 2000 units/s turning 1 rad/s - the
     eye trails 3.8° off the heading at 60 fps, 16.9° at 13 fps.
   - target: +0x34 ? (0,64,0)·W + P : preset_off[0] (0,−160,1680)·W + P (1680 ahead,
     160 below). +0xc = 2.
   - target lerp from the old +0xa4: x 0.5f, y 0.3333333432674408 (float 1/3
     [0x4d140c]), z 0.5f.
   - **look-at 0x46e140 from the lerped pos** to the lerped target.
   - u = unit(pos − P) (length as sqrt((vy²+vz²)+vx²); skipped when 0).
   - +0xc8 = the lerped pos (kept for the next frame).
   - final pos = P − u·d... with d negative: P + u·|d|, i.e. exactly |d| from the bike
     along the lerped direction; pos.y += 191 (+0x54).
   - finish orbit: pos.y −= (191 · 0.6666666865348816) · (yaw · −0.4244131815783876)
     (the camera sinks by 191·2/3·(yaw/(−3π/4)): 127 at the end).
   - So the view direction is from the lerped point, but the eye is pushed back to
     exactly |d| and lifted 191: C does not look from +0x98, it looks from +0xc8.
     [test] C = lookat(+0xc8 → +0xa4) to 3e-8 in 18/19 chase dumps (q32 is off by
     15.8 units, probably dumped mid-frame).
   - [test] At rest (q44): +0xc8 − P = (0, 64, −484)·Ry and +0xa4 − P =
     (0, −160, 1680)·Ry exactly, |+0x98 − P| relation: |+0xc8 − pos0| = 4.2 =
     488.2 − 484.
   - Moving dumps show |+0xc8 − E0| of 600..1100 because the headless original ran
     at ~13 fps with the 0.2-per-frame lerp.

The camera never looks from the bike's matrix directly except presets and on-board;
the chase follows heading only, so the horizon never tilts in the chase view [px:
all chase captures have a level horizon].

##### 2.6 Keys (0x431250, the VK switch, tables 0x432668 / 0x4325c4) [asm]

| key | effect |
|-----|--------|
| Tab down | yaw +0x3c = π, +0x21c = 1 (look back: bike-mounted preset 1) |
| Tab up (0x4328c7) | both 0 |
| F2 / F3 | screen mode [0x4dc0c8] down / up (pixel counts 0x12c00 / 0x30000 / 0x4b000 → [0x4dc0cc] = 320x240 / 512x384 / 640x480), [0x4e9ce4] = 1 |
| F4 | toggle Mode_169 [0x4dc0d4] (16:9: picture height W·9/16, §1b) + screen reset |
| F5 | replay: next bike [0x508834]; else +0x220 = (+0x220+1) & 3 (chase distance) |
| F6 | toggle on-board [0x4dc00c] (needs [0x4e8944]; not in a .Sav replay) |
| F7 | 0x4847f0 ([0x4f0dc8]/[0x4f0dcc]) - see §3b |
| F8 | toggle [0x4f0e14] with a message for 2000 ms |
| F9 | [0x4dbdac] = (x+1) % 5 |
| F11 / F12 | the in-race windows |
| numpad + / − | detail [0x4dc098] 0..4 |
| numpad * and / | 0x409720(1) / 0x409720(0) |
| F1 | the help (`generic.c`, §5a) |
| Ctrl+F1, Ctrl+Shift+F1 | the debug lines [0x4dc124] and [0x4dc128], only with a command-line switch (0x4dc1a4, set at 0x414888), off in a normal game: left out |
| Shift+F1 | 0x484380: the screen's size for 6 s, or if showing, it slides away (below) |
| Ctrl+F | 0x4866d0: SmallFonts toggled, the screen set up again (below) |
| Ctrl+U | [0x4f0dd0] mph or km/h, no message (not with Shift) |
| T | [0x4e895c] toggled and "ON" / "OFF" for 2 s: **nobody reads it**, left out |
| A, Shift+A | [0x4e8958], [0x4dc0f4] (1, 2, 4): **nobody reads them**, left out |
| Ctrl+H | Direct3D's antialias (0x53bb2c, 0x53e014, 0x41a0a0); says "No antialias!" under -NoD3D: left out |
| Ctrl+D, Ctrl+L | only under Direct3D; Ctrl+T, Insert: only under -SnapShot: left out |

All of these after the seven control keys, which are looked at first and whatever else the key
does (0x43129a); also during a pause: the handler has no pause test but where a row says so. The
port had the control keys last and after its pause test, so a key let go during a pause stayed
held; moved first (`race_key`). While a dialog is open it asks first: Ctrl+F is eaten by every
dialog (0x433ec0: `vk == 0x46` with Ctrl) and does nothing then, which makes 0x4866d0's laying
out of open dialogs (0x4348f0) dead from this key.

Read by nobody [asm: the addresses appear only in 0x431250]: 0x4e8958, 0x4dc0f4 (0x47b420 sets it
to 1), 0x4e895c. T's words in the original (captured): "ON", "OFF", font 12, y 20, fading, the id
0x4e98f0 F8's message has, so one replaces the other.

**Numpad + and −** [asm 0x431c70, 0x431cca]: 0x4dc098 ± 1 within 0..4, not with Shift, on the
software path always. Its readers and where the port has them, each checked by running: the kinds
(`r3d.c` remap, below), LenBPlane for the riders (`race.c`), decor off at 0 (`land.c`), the
smoke's one in 8 (`effects.c`), the ambience stopping at 0 and coming back (`race_sound.c`,
0x405f30). The port's remap had three holes that only show below 3: flag 8 at detail 2 lost its
perspective twin (the road's zigzag), 38/39 went to 37 even near (0x28: **51**), 44 stayed textured
(far **12**, near 0). It now works from the table itself, 0x4995a0's properties and "the first kind
with those properties". And the rider's flat faces were black at 0 and 1: 0x462c60 turns the file's
colour with `(c & 0xffc1) >> 1 | (c & 0x1f)` before 0x402020 and only then looks in palette 0 (the
note in §4.15.3 called this "unused by textured kinds"; at detail 0, 1 they are flat, 0x49ca70).
Compared with the original at Speed Bay, a Practice, the bike standing, y >= 225 (no time digits
or waving palms there): detail 4: 0 pixels different; 3: 30; 2: 30; 1: 0; 0: 0. The 30 are in the
box's right-hand part and were not looked into further than that they are not the road or the bike.

**Numpad * and /: 0x409720** [asm]: the line by the modifiers (Shift wave, Ctrl CD, neither master),
step ±1 in 0..32 (no change out of range), then `[` + label (texts 77, 78, 76) + 0x82 + value x 0x81 +
(32 - value) x 0x80 + 0x83 + space + `]` through 0x4188b0: id 0x4e98ec, 2 s, align 0x22, y 30, font 32,
blink 4. No mixer (0x5d2958 == 0): text 75, blinking. A missing line: label + text 72 (captured under
wine: "CD AUDIO VOLUME NOT AVAILABLE"); the port's three lines all exist. `racekeys.c`. Compared:
the wave's bar at 32, 0 pixels different in x 200..440, y 28..40.

**Shift+F1: 0x484380** [decomp]: the first text (129, id 0x4e98f4) only with Link (0x4dbffc == 0): out.
The second (id 0x4e98f0, list 0x53b8e8): `[  %dX%d]`, `\n[` text 143 (LETTER BOXED FORMAT) if 0x5d93c0,
`\n[` text 148 (FULL SCREEN) if 0x5d43a8, then the memory line `\n[  VIDEO %4.2fM TXT %4.2fM]` and its
forms. Font 6, 6 s, x 2, y 2, align 0x81, motion 4, dodge 5,5, vx -1, k 1. Pressed while it shows:
0x417780, its time made up at once, so that it drifts off left. The memory line is DirectDraw's free
video and texture memory (under GOG: dgVoodoo's setting, "VIDEO 256.00M TXT 252.41M"): **the port has
no such thing and leaves the line out**. Compared with it put in for the test: 0 pixels different
(x 0..240, y 455..480), so the text object is right and only the line is missing.
0x4592a0 calls it too, at each set-up of the screen: at a race's load (seen for the first 0.3-0.4 s of a race, drifting off to the left: red
"640X480 / VIDEO 256.00M TXT 252.41M", dgVoodoo's numbers; not in the port, build/notes/list2_starts.md) and at Ctrl+F, F2, F3, F4: `racekeys_new_screen()` is that part for the race.

**Ctrl+F: 0x4866d0** [asm]: 0x4f0e08 ^= 1, all three text lists emptied, fonts swapped (0x474f10),
0x4e9ce4 = 1: 0x4592a0 at the end of the frame (lists again, the map again 0x40ed10, the text above).
So "3 LAPS", the countdown's words and the like vanish and do not come back: captured. Compared with the
original, with the memory line in for the test: 0 pixels different for y >= 225 at big fonts.

**Ctrl+U**: g_speed_unit, already read by the dials and saved. Standing still the picture does not change
(0 pixels different); the dial's mph conversion is §3b's.

**Alt+Return** [asm 0x43355f]: WM_SYSKEYDOWN, so no Alt+key reaches 0x431250 or the menus: only Return
does anything, 0x4dbff0 ^= 1 and 0x4e9ce4 = 1, unless 0x4e8934 != 1 (-1 on the podium, 0x452240,
back at 0x452eb0). The port: `main.c`, `g_alt_return`. It takes no account of a held key's repeats,
which in the original would flip it over and over.

0x46d750 sets [0x4dc138] = 1 (the per-frame 0x467b30 pass).

##### 2.7 The TV director 0x46e9f0 (brief) [decomp]

Off if [0x4dc038]. State [0x4ea5a8], timer [0x4dc13c] (−= dt; shots last 250, 600,
300, 400, 260, 400). Trackside cameras are decor objects named "CAMERA"/"camera"
(0x473240 counts them into [0x51a40c] and hides them); the nearest by x,z among
sections s..s+23 (0x40d4f0), checked by 0x470860 (visibility). Choice by the
game's random stream (Rand_Next):
- 0: trackside, mode 3, zoom 1;
- 4: trackside zoom = (H/2) / ((fx·256)/dist) (0x624b18 is the screen's height), clamped to
  [1, 3] (the zoom 3.0 seen in dumps);
- 1: the chase for 600;
- 2: orbit: angle +0x28 (starts ±π) += dt·(−0.015707963267948967) (−π/200), radius
  +0x58, y ≥ ground (0x40c8b0) + 260;
- 3: behind at π, y lerped 0.25 to ground + 560;
- 5: a bike-mounted preset chosen at random 2..7 (400 frames; −100 for 2/3) with roll.
0x473370 builds the trackside sign table 0x5d2eb8 from library names counter,
g_45, d_45, g_90, d_90, g_chica, d_chica (x 320, y 400) [decomp].
The other replay camera path 0x4583e0's "mode 1" is this; there is no separate
replay camera.

---------------------------------------------------------------------------
#### 3. The HUD: texts, countdown, messages

Marks: [asm] read in build/mr.asm, [decomp] Ghidra text only, [px] confirmed on a capture with tools/hud_text.py, [guess].

**Port** (`hud.c`, 2026-10-26): 3.1–3.5 for Solo with the GOG data: the countdown (laps,
number, GET READY, GO and their voices), the lap, best-lap, TIME / POS and practice panels,
the finishing place, TIME OUT / FINAL LAP / WRONG WAY / CHECK POINT. Its pictures pass
tools/hud_text.py as the captures do (q17's 12 texts exact, q14's countdown the same).
Not yet: the split line (0x412e50), labels (3.7), the CD data's sprite messages (GAME OVER
is written, 3.5) (the dials, F7's three states and the gear-change hint bar are written: 0 pixels differ from
tools/hud_dials.py, `r3d_screen_points/faces`). The port's single race has 24 bikes, as the original's (q14/q17's "/23" was the attract demo, §4.17.9).

##### 3.0 Fonts, coordinates, frame counters
- The HUD fonts are fetched every frame by 0x4816c0 through Font_Get 0x4903d0 [asm]:

  | Global | Font (hi-res / low-res) |
  |---|---|
  | 0x4f0d64 | 20 PosHiRes / 19 |
  | 0x4f0d68 | 7 TimeHiRes / 3 |
  | 0x4f0d6c | 4 ChronoHiRes / 0 |
  | 0x4f0d70 | 5 MotoHiRes / 1 |
  | 0x4f0d74 | 11 SpritesHiRes / 10 |
  | 0x4f0d78 | 14 GameMsgHiRes / 13 |
  | 0x4f0d7c | 6 PlayerHiRes / 2 |

- **Small fonts (0x4f0e08), 320x240 and 512x384:** the low-res fonts are used, and every DrawString with noscale 0 halves x and y.
  - Wrappers with noscale 0: 0x493e20, 0x493e60, 0x493ee0. Wrappers with noscale 1: 0x493f20, 0x493f60, 0x493fe0.
  - The HUD is written in 640-pixel units. With flags 8 it is right-aligned against the real screen width.
  - Widths that a routine measures itself are doubled first, wherever a following DrawString will halve them (0x482380, 0x482830).
  - [px] All the build/trackcap captures are 640x480 with small fonts on: every match below is the low-res fonts with halved positions.
- **Text objects** (TextList_Add 0x417d20, NOTES §4.8.3.6):
  - Their x and y are doubled and then halved, so in practice they are not scaled [px q14: "3 LAPS" at y=20 is exact].
  - Direct and Pal fonts ignore the effect level. FadeIn, FadeOut and BlinkTransp therefore change nothing on fonts 4/7/13/14/20; only lifetime and blinking show.
- **Blink counters count frames, not time:**
  - 0x4f0dc4 goes up by 1 each race frame (0x47ff90).
  - 0x4f0e1c goes up by 1 each call of 0x482380.
  - In the port these should count 60ths of a second (g_ticks) [guess about the original's frame rate].
- **Time split 0x481730(ms)**: 0x4f0d80 whole seconds, 0x4f0d84 hours, 0x4f0d88 minutes, 0x4f0d8c seconds % 60, 0x4f0d90 hundredths. 0x482830 and 0x482750 split their own copy into 0x4f0d94..0x4f0da4.
- **Characters that are pictures** [px, fnt.py sheets]:

  | Font | Character | Picture |
  |---|---|---|
  | 4/0 Chrono | `:` | ′ (minutes mark) |
  | 4/0 Chrono | `;` | ″ (seconds mark) |
  | 4/0 Chrono | `<` | `/` |
  | 11/10 Sprites | `A` | red "POS" pill |
  | 11/10 Sprites | `B` | "LAPTIME" |
  | 11/10 Sprites | `C` | "BESTLAP" |
  | 11/10 Sprites | `D` | "TIME" |
  | 11/10 Sprites | `E` | large "POS" (practice) |
  | 11/10 Sprites | `F`..`]` | status balls (results table) |
  | 11/10 Sprites | `^` | "R" (demo mark) |
  | 11/10 Sprites | `_` | EXTENDED TIME |
  | 11/10 Sprites | `` ` `` | WRONG WAY |
  | 11/10 Sprites | `a` | FINAL LAP |
  | 11/10 Sprites | `b` | CHECK POINT |
  | 11/10 Sprites | `c` | TIME OUT |
  | 25/26 FinalPos | `/` | the word "POSITION" |
  | 25/26 FinalPos | `:` `;` `<` `=` | ST, ND, RD, TH |

  Font 6/2 Player is the name-bar font:
  - `' '` is a red 16-pixel bar block, `]` the bar's right end, `[` its left end.
  - Digits, `:` and `A`..`Z` are white on red.
  - `+0x20` gives the orange set, used for computer riders: `` ` `` is the bar fill, `{` the left end, lower case the letters.
  - `+0x40` gives black letters on red, used for records: 0x80 is the fill.
  - `\` is a pill mark and `^` a tick.
- **Font 5/1 Moto** (rider labels): `)`..`@` are the number pills 1..24, `A`..`Z` the letters, `]` the down-pointer.

##### 3.1 Who draws what, each race frame (0x4806e0; 0x4804d0 is the variant used while the in-race window 0x4e8904 is open) [asm]
1. 0x4817a0 counts the loaded bikes into 0x4dc100. 0x481730(0x4dc0a8) splits the race clock.
2. If race+0xc is set and neither 0x509ba0 (everyone finished) nor 0x542a20 is set:
   - **Time ran out (0x52c238):** remove text 0x4e9884. In Link play only: results table 0x482f00, then GAME OVER through 0x482120.
   - **Otherwise, while racing (0x4dc0ac):** 0x413410 (profiling probe 'start'). It runs the countdown 0x413030 while 0x4dc0b4 is set, and the racing HUD 0x413180 after that.
3. When the player has finished:
   - **Solo:** remove 0x4e987c and 0x4e9880; best-lap panel 0x482830(0, 0x5d25b0); then 0x481970 (finishing position) unless the end-of-race condition (0x51601c > 0, a race mode, rank 0..2, state 2) holds; then the lap panel 0x483860.
   - **Link:** results table 0x482f00, maybe 0x481970.
4. While a demo plays (0x4e9834 == 1): 0x486660.
5. Then the text lists are drawn: 0x53ba68 (only if race+0xc), 0x53b8e8, 0x53b9a8.
- **0x4804d0 differences:** 0x483860(1) (no split line) and 0x482830(1) (record row only) in time-attack practice. It also calls 0x482750(0, &O(k)+0x2ddd): the TIME sprite `D` (font 11) at (2,5) flags 8, and under it the total race time "%02d:%02d;%02d " (font 4) at (2,30) flags 8.

##### 3.2 The countdown (0x413030, while 0x4dc0b4 = 1) [decomp + asm, px q14]
- s = 0x4dc0a8 / 1000. The race clock counts down here; q14 has clock 5434, so s = 5.
- **Laps text**, 0x482220(m, x 0, y 20): "%d LAP" or "%d LAPS" of 0x4dc074.
  - Text object in 0x53ba68, id 0x4e9868, font 14, align 0x22 (centred, 20 from the top). Own colours red (255,0,0) on black.
  - m = 0 when s ≥ 4: steady.
  - m = 1 when s ≥ 2: BlinkTransp with a 1000 ms period. On a Direct font that shows as steady.
  - m = 2 when s < 2: FadeOut (no visible effect), motion 2 drift with vy = 1 and s = 0.2, lifetime 4000 ms.
  - Recreated when m changes (0x4f0e18). Off-screen check off (+0xa4 = 1).
- **Best lap panel** 0x482830(1): the track record row only (3.4).
- **Countdown number**: n = s/2 + 1, so 03 / 02 / 01, two seconds each.
  - When n changes (0x542854): "%02d" through 0x4188b0(id 0x4e98d0, lifetime 2000, align 0x42, x 0, y 0, effect 2 FadeOut, font 14, blink 4). Centred both ways. FadeOut shows nothing on a Direct font.
  - Voice 0x4082f0(n): sound ids 0x33 / 0x32 / 0x31 for 3 / 2 / 1, and 0x30 for 0 (GO). Played only when n changes (0x504564).
  - While n > 1: "GET READY" through 0x4188b0(id 0x4e98e8, lifetime 0, align 0x42, y 30, effect 3 Blink, font 12 Messages, blink 4). Centred, 30 below the centre line, 4 frames on and 4 off.
- **0x4188b0(id*, text, life, align, x, y, effect, font, blink):**
  - First removes the object holding that id, then TextList_Add(0x53ba68, ..., motion 0, steps 5/5, recentre 1).
  - Always own colours: **red (255,0,0) on black.**
  - [px] The font colour calls just before it (0x490a30/0x490a60 on 0x4f0d78: (245,255,12)/(200,200,0) for the number, (0,200,0)/(0,180,0) for GO) are overridden by these own colours. "03" is exact in red in q14, so they have no effect.
- **GO**, in 0x413180 once the countdown has ended (0x47dd5d clears 0x4dc0b4 when the clock goes below 0):
  - while *(0x4dbe44) > 0 (the countdown set it to 500), and only when 0x4f0d8c/2 changes: "GO" through 0x4188b0(0x4e98e8, lifetime 500, align 0x42, y 15, effect 2, font 14, blink 4); voice 0x4082f0(0); then *(0x4dbe44) = 0. (0x4f0d8c is the lap panel's by then: §4.17's start, "s here is not the time left".)
  - Result: red GO, centred, 15 below the centre line, for 500 ms.

##### 3.3 Racing (0x413180) [decomp + asm]
- **Checkpoint passed**: 0x519fec > 0 and the player's short +0x2f79 (0x584191) changed while state (+0x2de5) is 1. The very first crossing is skipped: the rule uses laps done +0x2f7b, reversed when 0x4dc144 is set.
  - Effects: sound 6; texts 0x4e9874 and 0x4e9878 removed.
  - Practice without time attack: remove 0x4e985c and 0x4e9860, then CHECK POINT through 0x481f10.
  - Other modes: remove 0x4e9870 and play sound 0x34.
- **Rider labels**, Link or time-attack practice: 0x483670 (3.7).
- **Player out or finished** (state not 0/1):
  - Link: results table 0x482f00(1, 0, 0, 30).
  - Then 0x482830(0, 0x5d25b0) and return.
- **Otherwise:**
  - Solo practice without time attack: 0x482620 (practice position). All other cases: 0x482380 (TIME + POS).
  - Then best-lap panel 0x482830(0, 0x5d25b0), lap panel 0x483860(0), and GO (3.2).

##### 3.4 Panels drawn directly (no text objects)
Format: font (hi-res), x, y in 640-pixel units, flags (8 = from the right edge; 0x21 = left, no measuring).

**Lap panel, top left: 0x483860(flag)** [asm, px q17/p3 100%]
- Drawn only for the player's bike: when it is the shown bike 0x4ea4c0, or +0x68 && +4 && (+8 or 0x4ea4c4).
- **Header:** `B` LAPTIME, font 11 (colours set to white), at (2,5) 0x21. Its right edge r is taken from the returned rect.
- **Lap count:** "%d<%d" = min(laps done + 1, laps) / laps, font 4, at (r+10, 5) 0x21, through the noscale-1 wrapper (not halved).
  - Hidden when (finished ? 8 : 0) & 0x4f0dc4. After the finish it therefore blinks 8 frames on, 8 off.
- **Rows**, y = 25 + 16·row:
  - Racing: laps max(done−2, 0) .. min(done, laps−1), the last 3 laps. Finished: all laps.
  - Lap time = 0x5db290[bike·8 + lap] in ms. This per-bike table of 8 laps is incremented by dt (0x4e9810) in 0x47dae0.
  - Text "%02d:%02d;%02d" (or "00:00;00" for a lap not reached yet), font 4, at (60, y) 0x21.
  - After the finish, with more than one lap: the fastest lap's row is drawn green (0x490a10 on font 4), then the colour is put back.
- **Lap number:** " %c%c]" with the tens digit (or a space) and the units digit of lap+1, font 6, at (0, y) 0x21. This is the red bar with the white number.
  - The current lap's number blinks: hidden when 0x4f0dc4 & 4.
- **Record mark:** after the finish, on the lap that equals the track record (0x5db290 value == 0x5d26a5, and the name 0x5d25b0 equals the bike's +0x2dd4), unless 0x4f0dc4 & 4: "[]" in font 6 at (192, y).
- **Split line**, when flag == 0, Solo (0x4e984c), 0x519fec > 0 and 0x5dbc70 != 0:
  - y = rows·16 + 41.
  - The split string 0x5dbc71 in font 4 at (60, y). Colour by the sign 0x5dbc70: '+' red, ' ' orange, otherwise green.
  - "  X]" in font 6 at (0, y), where X is `\` (the pill mark) when '+', else a space.
  - The split is written by 0x412e50 at each checkpoint (from 0x47e3d0): this checkpoint's time minus a reference rider's at the same checkpoint (`+`/`-`/` %02d:%02d;%02d`).
  - The reference is the bike 0x516ab8 (copied into 0x581018/0x581025: name, 48 checkpoint times). There is none (0x7f) when the player leads [decomp].

**Best lap panel, top right: 0x482830(mode, entry*, -, -, rect*)** [asm, px q17/p3]
- Starts at y = 27 (0x1b) [asm].
- In Solo with mode 0 it loops over three cases. Otherwise it runs case `mode` only (the countdown uses 1).
  - **Case 0, this race's best lap among the bikes:** the bike with the lowest +0x2de1 (0x583ff9), sorted by qsort with 0x4817d0, cached in 0x4f0db8. Name +0x2dd4. A computer rider (+8 == 0) uses letters +0x20 (orange).
  - **Case 1, the track record:** time 0x58120d, name 0x581118. Letters +0x40 (black on red).
  - **Case 4, entry 0x5d25b0:** time at +0xf5 (0x5d26a5). Letters +0x40; hidden when 0x4f0dc4 & 4, so it blinks.
  - (Case 2, bike = param 2: best lap +0x2de1, name +0x2dd4.)
  - A row is drawn only when its time is not 0x7f7f7f7f.
- **First row drawn:** header `C` BESTLAP, font 11 (white), at (0,5) flags 8.
- **Each row:**
  - The bar "[" + "@"·(8 − len) + name, every character shifted as above, font 6, at (0, y−2) flags 8. It runs into the right edge with no end cap.
  - The time "%02d:%02d;%02d " (with a trailing space), font 4, at x = bar.left − width − 5 (doubled when halving applies), y, flags 0x21.
  - y += font 6 height + 4.
- [px q17] The record name is "DSI" and the time 00′43″32.

**TIME and POS, right side: 0x482380** (single race, championship, time attack, Link) [asm, px q17/p3 100%]
- **Number:** "%02d" of 0x4f0d80, or "00" when the clock 0x4dc0a8 < 0. Font 7, SetColors(fore, black), at (8, 115) flags 8.
- **Colour and blinking:** the number is drawn when (++0x4f0e1c & mask) != 0.

  | Condition | Number | Mask | TIME label |
  |---|---|---|---|
  | 0x519fec ≥ 4000 (just after a checkpoint) | yellow | −1 | yellow, blinking (mask 2) |
  | clock < 0 | red | 1 | red |
  | under 3 s | red | 2 | red |
  | under 10 s | orange (255,160,0) | 4 | white |
  | otherwise | green (0,255,0) | −1 | white |

  On frame-count blinking, see 3.0.
- **Label:** `D` TIME, font 11, at (8, 90) flags 8.
- **Position**, if 0x459f50(player) + 1 > 0:
  - `A` POS pill, font 11 (white), at (0, 150) flags 8.
  - "<%d" of 0x4dc100 (the bike count, shown as "/23"), font 4 (white), at (8, 171) flags 8.
  - Position "%d", font 20 in its script colour (0,200,0), at (max(pill width, "/n" width) + 1, doubled when halving) + 8, y 150, flags 8. It sits left of the pill. The position is also stored in 0x5f0f4c.

**Practice position: 0x482620** (Solo practice without time attack) [decomp]
- `E` (large POS) at (8, y+5); "<%d" in font 4 at (8, y+52); position in font 20 at (w+9, y+30); all with flags 8.

**Finishing position: 0x481970** [decomp]
- Text object in 0x53ba68, id 0x4e9884, added once. Text "%d%c/": the rank, then ':' (1st), ';' (2nd), '<' (3rd) or '=' (other), then '/', giving e.g. "1ST POSITION".
- Rank = +0x2f85 when finished (state 2), else 0x459f50.
- Font 25, x 0, y 5, align 0xc2: centred, bottom edge 5 above the centre line (y = H/2 − 5 − h). Lifetime 0, effect 0.

**Results table: 0x482f00** (Link) [decomp, not compared]
- One row per rider by position: name (font 6), times '%02d:%02d;%02d' (font 4).
- Header sprites `E`, `D`, `C` (font 11).
- Status ball: `F` finished (white), `N` out (red), `V`+(frame&7) racing (green, animated), `W` (cyan, state 0).
- The player's row is cyan. Centred when flag 0. Registers its rect through 0x417b90.

##### 3.5 Messages (text objects in 0x53ba68, each added only if its id is not alive)
- **0x4dc180 decides the form** [asm]:
  - 0x472c40 sets it to 1 when the height of Data\Design\Texts\LowRes.RAW (read by 0x45ac10) is 128, or when the command line has `-DataV2` (0x414080).
  - **GOG 3.22 data: 256x128, so 1: font 11 picture characters.**
  - **1997 CD data: 256x256, so 0: picture sprites** (handles loaded by 0x480930 from Texts\HiRes.RAW / LowRes.RAW), added through 0x418160 (sprite objects, up to 8 parts).
- **Font form**, TextList_Add(font 11, x 0, y 0, align 0x42 centred, effect 3 Blink, motion 2 or 4, frame-mask blink, vx 2, ty 90, s 2.0, k 5.0, own colours fore / black):

  | Routine | Message | Character | Life | Blink | Colour | Id | Trigger |
  |---|---|---|---|---|---|---|---|
  | 0x481af0 | TIME OUT | `c` | 2000 | 2 | red | 0x4e9858 | 0x47dae0: clock < 0 and > −500 ms, player state 0/1, not practice-without-TA, not 0x4e9830; sound 0x42 every 2000 ms (0x4f0c54) |
  | 0x481c50 | FINAL LAP | `a` | 1000 | 4 | yellow | 0x4e9864 | 0x47e3d0: player crosses the line with laps done == laps−1; first removes 0x4e9870/5c/60/74/78; sound 0x37 |
  | 0x481db0 | WRONG WAY | `` ` `` | 4000 | 4 | red | 0x4e986c | 0x47e3d0: the heading test against the track direction is below 0x4d1900 |
  | 0x481f10 | CHECK POINT | `b` | 1000 | 2 | yellow | 0x4e9870 | 3.3, practice only |

  0x481f10 also calls 0x482030, the sprite form with id 0x4e987c [decomp; arguments lost by Ghidra].
- Sprite form (the data's 0x4dc180 = 0) [asm 0x481b05.., 0x481f1c.., 0x480fc0..0x481480]: 0x418160 into 0x53ba68, x, y 0, align 0x42, **effect 3 Blink** (not 0, the earlier read took the wrong argument), motion 0, the five dodge arguments as GAME OVER's, one part { low-res, hi-res, 0, 0 }. CHECK POINT goes through 0x482030, the same call with four parts' handles as two groups of four (lows, then highs): the others are 0 and add nothing. So they stand in the middle and blink, they do not drift as the font's do.

  | Message | Life | Blink | Low-res 0x5d3b69 | High-res 0x5d3cb2 |
  |---|---|---|---|---|
  | TIME OUT | 2000 | **1** | 0x4eaacc (91, 40)-(176, 47) | 0x4eaaa4 (85, 28)-(237, 41) |
  | FINAL LAP | 1000 | 4 | 0x4eaac8 (0, 24)-(98, 31) | 0x4eaa9c (0, 56)-(168, 69) |
  | WRONG WAY | 4000 | 4 | 0x4eaadc (0, 8)-(103, 15) | 0x4eaab4 (0, 14)-(187, 27) |
  | CHECK POINT | 1000 | 2 | 0x4eaae0 (0, 32)-(124, 39) | 0x4eaab8 (0, 70)-(224, 83) |

  (Rectangles inclusive, offsets 0. **The key**: GOG's program says black in this form too, but the CD's TEXTS\\LOWRES.RAW, HIRES.RAW and GAMEOVER.RAW are 24-bit (palette count 0) on a pure green ground: the CD's own MOTO.EXE has the same rectangles and keys them **green** (its 0x4e77ac = 00 ff 00, asm 0x4a35d2), so GOG's form with the CD's data would show green boxes. The port on the CD's data does as MOTO.EXE. 0x482120 with n != 0 also comes to 0x482030: 0x20, and only from 0x4806e0's Link branch, so not in the port.) **Port**: `hud.c` `picture_message`, `show`, GOG's game.EXE on the CD's data folder (the CD's own MOTO.EXE is not read). TIME OUT, FINAL LAP, WRONG WAY and GAME OVER seen on the CD's data in the port, blinking in the middle as these arguments say; CHECK POINT not reached in a test. Not compared with an original: GOG's program on the CD's data is not what anyone played (the green), and the CD's MOTO.EXE was not run.
- **GAME OVER, 0x482120(0)**: a sprite object in **0x53b8e8**, id 0x4e987c (the arguments read from the push order at 0x482193..0x482203 [asm]; Ghidra's "effect 4" is the motion).
  - First removes 0x4e9858 and 0x4e986c (TIME OUT, WRONG WAY) from 0x53b8e8 [asm 0x482173..0x48218e].
  - Trigger: 0x47dae0 when the clock goes below −500 ms: sets 0x52c238, plays sound 0x39, sets player state 3. Also from the Link branch of 0x4806e0. (0x482120(n != 0) goes to 0x482030 instead.)
  - **0x418160 (cdecl, 31 dwords: `add esp,0x7c`) arguments** [asm 0x482193..0x482203; meaning from 0x418160's decomp, which passes its 2..9 and 15..22 to 0x416170 (§4.19 §1.3 order)]:

    | # | value | meaning |
    |---|---|---|
    | 1 | 0x53b8e8 | the list |
    | 2 | 3000 | lifetime ms |
    | 3, 4 | 0, 0 | x, y |
    | 5 | 0x42 | align: centred both ways (0x417900) |
    | 6 | 0 | **effect 0** (none) |
    | 7 | **4** | **motion 4**: wait the lifetime, then 2 (drift); 0x418790 then sets lifetime 0 |
    | 8 | 0 | blink |
    | 9 | 0x4e987c | int* id |
    | 10, 11 | 5, 5 | dodge steps x, y (0x417ac0 against the list's live boxes) |
    | 12 | 1 | no free place: re-align with 0x42 |
    | 13 | 0 | int[4]* that would receive the box (none) |
    | 14 | 0 | argument of the new object's vtable +0x1c |
    | 15..18 | 2, 0, 0, 0 | vx, vy, tx, ty |
    | 19+20 | 0, 0x40000000 | double s = 2.0 (drift's starting speed) |
    | 21+22 | 0, 0x40140000 | double k = 5.0 (drift's acceleration a frame) |
    | 23 | 2 | part count (clamped to 8) |
    | 24..27 | [0x4eaac4], [0x4eaa94], 0, 0 | part 0: low-res handle, hi-res handle, dx, dy |
    | 28..31 | 0, [0x4eaa98], 0, 0 | part 1: no low-res handle, hi-res handle, 0, 0 |

    So: centred, still for 3 s, then it drifts right (x += trunc(2·s), s += 5 each drawn frame) and dies once its box leaves the screen (0x4169b0).
  - **Parts, 16 bytes each** from the 24th argument [decomp 0x418160, the two copy loops]: +0 low-res handle, +4 hi-res handle, +8/+0xc an offset pair (copied to the part array local_b8). **Hi-res = ScreenMode 0x4dc0c8 == 2 and SmallFonts 0x4f0e08 == 0**: the +4 handles are used; else the +0 handles. A handle 0 finds no sprite (0x477270) and adds nothing. So **GOG's default (640x480, SmallFonts 1) shows the single "Game over LR" sprite**; hi-res shows GameOver1 HR and GameOver2 HR.
  - Box: w = Σ(x2 − x1 + 1), h = Σ(y2 − y1 + 1) over the parts found (both summed: for hi-res 370 x 317) [decomp]; aligned with 0x417900; the first part's (+0xc − +4, +0x10 − +8) record offsets are added before the dodge test and taken off again after.
  - **The sprites** (0x477790(name, x1, y1, x2, y2, +4, +8, +0xc, +0x10, +0x14, +0x18, frames, key on, key lo*, key hi*, unused): `ret 0x40`, fields at [asm 0x47780a..0x4778e8]), defined by 0x480930 [asm]:

    | Handle | Name | Picture (bank) | Rect (inclusive) | Size | Other |
    |---|---|---|---|---|---|
    | 0x4eaa94 | GameOver1 HR | Texts\GameOver.RAW (0x5d3dfb, both data forms) | (0, 89)–(255, 255) | 256x167 | [asm 0x480ce7..0x480d2e] |
    | 0x4eaa98 | GameOver2 HR | Texts\HiRes.RAW (0x5d3cb2) | (0, 98)–(113, 247) | 114x150 | +0x10 = 9; GOG form [asm 0x480d7f..0x480dca], CD form the same values [asm 0x481164..0x4811b2] |
    | 0x4eaac4 | Game over LR | Texts\LowRes.RAW (0x5d3b69) | GOG form (0x4dc180 = 1): (0, 15)–(255, 127); CD form: (0, 143)–(255, 255) | 256x113 | [asm 0x480e07..0x480e4b; 0x481343..0x48137e] |

    All: 1 frame, colour key on (+0x3a = 1), key low = high = 0x4f0dd8 (black). **In the GOG form the LowRes.RAW (256x128) holds only "Game over"** (its one sprite; bank count 1) and HiRes.RAW only GameOver2 (bank count 1, the name "Text1 HR" given to the bank) [asm 0x480d33..0x480e4b].
  - **The object (0x415e80, 0x178 bytes, vtable 0x4d0580, +0xc type 2)** [decomp 0x415e80, 0x416430, 0x4164b0]: +0x134[8] low-res handles, +0x154[8] hi-res handles, +0xf4[8] the (dx, dy) pairs (the part records' +8/+0xc), +0xb0 part count, +0xb4[8] the sprites found, +0xac how many (0 → the object deletes itself), +0xd4[8] frame numbers (0), +0x174 blit flags (0). 0x416430(obj, count, parts by value) copies the records, then **0x4164b0** resolves the handles (hi-res test as above) and sets **+0x80 w = Σ(x2 − x1 + 1 + dx)** and **+0x84 h = the first sprite's height**. (0x418160 placed it with the summed heights, so hi-res GAME OVER sits at y 81, not centred: (480 − 317)/2.)
  - **IsAlive 0x416a60** (slot 3) [decomp]: as 0x4169b0 (id, lifetime, the box against the screen) but needs only +0xac != 0 (no text, no font).
  - **Draw 0x417080** (slot 2) [asm 0x417080..0x417205]: the motion step 0x416b40 as for texts; **effects 3 and 4 only blink** (period or frame mask, as Blink), every other effect draws plainly (no see-through level). Then x = +0x70, and for each sprite: x += dx, y = +0x74 + dy, **0x477400**(sprite, x, y, +0x174, frame, &rect, 1.0f); the next x is **the drawn rectangle's right edge** (inclusive, [esp+0x28] = rect[2]), so parts sit side by side overlapping by one column (GameOver2 starts on GameOver1's last column; its +0x10 = 9 lines it up with GameOver1's rows 89 → 98). If a part is not drawn the next x is stale [asm].
  - **0x477400** [decomp]: source rect (x1, y1, x2, y2) (+ frame × (+0x14, +0x18) for frame 0 < f < count), x2/y2 kept inside the picture, x1/y1 ≥ 0, nothing unless y1 < y2 and x1 < x2; drawn at (x + (+0xc − +4), y + (+0x10 − +8)); flags 0 → the sprite's +0x4a (**1**, set by its constructor 0x4772f0). **0x4788b0** makes the screen rectangle (inclusive) at scale 1.0 (0x3f800000). **0x478990** [decomp]: flags & 1 or & 4 → clip to the screen rectangle (0x624374, 0x6d1040, 0x62e448, 0x625784), moving the source edges with it; nothing unless the clipped top < bottom and left < right; the rectangle written to the out pointer; then a DirectDraw Blt (software surfaces: 0x45ab60) with **DDBLT_KEYSRCOVERRIDE (0x10000)** when +0x3a is set: the key low/high are +0x34/+0x37 turned into the pixel format, black/black here, so **pixel 0 is see-through** (the RAW loader keeps index 0 at 0 and lifts near-black palette entries to red 8).
  - **What the player sees** (GOG data, 640x480, SmallFonts 1): "Game over" 256x113 at (192, 183), still for 3 s, then off to the right in about eight pictures. TIME OUT (2000 ms from clock 0, in 0x53ba68) is still alive under it for its last 1.5 s: GAME OVER's removals miss it, and 0x53b8e8 is drawn after 0x53ba68.
  - **Port** (2026-10-26): `sprite.c` (0x477b30 load, 0x477400/0x478990 keyed clipped blit), the sprite objects in `text.c` (`TextArgs.parts`; `list` keeps 0x53ba68's objects drawn first), `hud_game_over()` in `hud.c` called by `rules.c` at the time over; GAME OVER removed when a Solo player has finished. Not written: the dodge (only the screen-mode text can be in 0x53b8e8 then). Not compared with a capture of the original (none of a time over yet).
- **Loaded but never shown in this build** [asm: no reference besides the load]: sprites EXTENDED + TIME (0x4eaaa0, 0x4eaaa8), MISSED (0x4eaabc), R (0x4eaac0). Ids 0x4e9874/78/5c/60 are only ever removed. The `_` EXTENDED TIME character is unused.
- **DEMO, 0x486660** [decomp]:
  - While demo id 0x4e9898 is not alive: if 0x4e9830, "DEMO" in font 12, align 0x22, y 10, BlinkTransp with a 500 ms period, lifetime 2000.
  - Then `^` (the "R" mark) in font 11, align 0x81 (2 from the left, 2 from the bottom), Blink 500 ms, lifetime 2000.
  - Both go through 0x4188b0 with the same id, so the second call removes the first: "DEMO" itself never stays [decomp, inference].

##### 3.6 Not HUD
- 0x486240 / 0x486310 (flags 0x5d2edc..) light the turn signs, flat on the screen (§4.13.5 §2): six slots, 75.0 units of time, blinking on ftol parity, drawn through 0x49c590 in the 3D pass. No HUD text.
- 0x484380 is the "[ %dX%d]\n[ VIDEO %4.2fM TXT %4.2fM]" text after a screen-mode change (the "640X480 VIDEO …" line in q14).
- 0x483e90 and 0x4841c0 are debug texts (when 0x4dc124 is set).

##### 3.7 Rider labels: 0x483670 (Link, or time-attack practice)
- For every other loaded bike, projected through 0x4818a0 from camera 0x4dc0f8.
- Text "%c^%s\\\n]": the number pill (')' + position), the name, and on the next line the down-pointer `]`. Font 5, recoloured per bike (0x459240 → 0x490a90).
- Placed at (screen x − width/2, screen y − font field +0x286 − 0x5d893c − text height), flags 0x21, noscale [decomp].

##### 3.8 Check: tools/hud_text.py
`python3 -I tools/hud_text.py CAPTURE racing|countdown OUT.png --small --set k=v …`
- Draws each text alone with fnt.py's renderer.
- Counts exact RGB565 matches at the game's position, and the best shift within ±4.
- OUT.png is the capture with the tool's pixels on top; mismatches are magenta.
- **q17** (lap 1/3, 00:04;06, record DSI 00:43;32, TIME 31, POS 22/23): **all 12 texts 100% exact at shift (0,0)**, 4,331 pixels in total.
- **p3**: 11 of 12 texts 100%. The best-lap time is 96.2%: its digits were guessed from the picture.
- **q14** (clock 5434):
  - "3 LAPS" 1247/1247 and "03" 578/578 exact, in own red; BESTLAP 100%.
  - GET READY: 166/510 exact, which is exactly its 166 fully opaque pixels. The rest are see-through blends. Its shape fits best at (0,0).


##### 3.9 Open points (HUD)
1. **The hi-res layout (small fonts off) is unchecked.** Every capture uses small fonts. One 640x480 race capture with small fonts off would check fonts 4/6/7/11/20 at full size and the doubled widths.
2. **The sprite objects** (0x418160, item type 2): the arguments, the part records and GAME OVER's sprites are mapped (3.5). The object, its draw and the keyed blit too. Still open: a capture of the original's GAME OVER to compare. (0x482030's arguments: §3.5.)
3. **Split reference rider** (0x412e50: 0x516ab8, 0x50dbdc+0x400, 0x457050): who it is exactly, and when 0x5dbc70 is cleared.
4. **What sets 0x519fec** (the checkpoint display timer, ≥4000 → yellow TIME) and its starting value. (Partly answered: Game_Frame subtracts the frame's ms from it each frame, §1.1 step 8.) Also where the countdown's starting clock (0x4dc0a8, probably 6000) is set.
5. **The DEMO text** being removed by its own second call needs a demo capture to confirm.
6. **The frame-mask blinks** (0x4f0dc4, 0x4f0e1c, the text-object masks) depend on the original's frame rate. The GOG copy runs at most 60 frames a second (-FrameRateMax60, §1.1 step 23) and a modern PC holds that, so 60 Hz ticks (g_ticks) give GOG's blink rates; the CD default is 30.
7. **0x482620** (practice position) and **0x482f00** (results table) are from the decompiled text only; no capture covers them.

---------------------------------------------------------------------------
#### 3b. The dials

**Switches.** `ShowCounter` 0x4f0dc8 and `CounterType` 0x4f0dcc both come from CONFIG (GOG: 1 and 1). `SmallFonts` 0x4f0e08 (GOG: 1) chooses the layout.

F7 (handled at 0x432107, only when 0x4e8944 is set) cycles through three states [asm]:
- Show on and Type 1: Type becomes 2 (digits only).
- Show on and Type any other nonzero value: Type becomes 0 and Show becomes 0 (off).
- Otherwise: Show becomes 1 and Type becomes 1 (dials).

0x4847f0(n) only stores n in 0x4f0dcc.

**0x484810(param)** runs only when the race draw's flags have bit 8 [asm]:
- Calls (1) and (2) come before the 3D render 0x49ad90; call (4) comes after it.
- It returns early if 0x52c0d4 is 0, if [0x52c0d4+0xc] is 0, or if ShowCounter is 0.
- It reads the viewed player (0x508834), whose object is O = 0x581218 + player·0x3209:
  - **Speed** is the **int** at O+0x118, not a float. With SpeedUnit 0x4f0dd0 == 1 it is converted to mph as `((v*0x3e800)/0x649)>>8`.
  - **Gear** is O+0x7c.

**Layout.** It is rebuilt whenever CounterType, ScreenMode or SmallFonts changes; the last values are cached in 0x4f0e34, 0x4f0e30 and 0x4f0e40.
- Type 2 turns on only the digits (0x5f14e4).
- Type 1 turns on the needles (0x5f14e0) and the digits.

The constants below are [asm]. **hi** means ScreenMode 2 with SmallFonts 0; **lo** is every other case, including the GOG default at 640x480.

| | hi | lo |
|---|---|---|
| speed dial centre / start point / end point, max 0x12c (300) | (205,191) / (187,250) / (242,142) | (227,68) / (219,99) / (249,42) |
| rev dial centre / start point / end point, max 0x3e80 (16000) | (119,211) / (108,247) / (119,170) | (183,79) / (177,99) / (183,57) |
| origin subtracted from the points above (only when Type == 1) | (61,116) | (154,30) |
| speed digits, origin subtracted | (207,210) | (226,79) |
| gear digit, origin **not** subtracted | (67,106) | (31,51) |
| shadow offset 0x5dbc60/64 | −3 | −2 |
| base point (call 1): 0x5dbc80 = W − a, 0x5dbc84 = viewport bottom (0x6d1040 + 0x68b008) − b | a 0xc3, b 0x8c | a 0x66, b 0x4a |

- All values are screen pixels with no scaling, so at 320x240 the lo dashboard is still 102x74 pixels.
- At 640x480 lo:
  - speed centre (611,444)
  - rev centre (567,455)
  - speed digits at (610,455)
  - gear digit at (569,457)
  - all [px exact].

**0x485be0(dial, cx, cy, p1x, p1y, p2x, p2y, min, max, dir)** works in doubles [asm]:
- It stores cx, cy, min and max at +0, +4, +8 and +0xc.
- The radius at +0x28 is the distance from the centre to the start point.
- a1 = atan2(cy−p1y, p1x−cx) and a2 = atan2(cy−p2y, p2x−cx). Each is wrapped into [0, 2π), using the double 0x4d1a48 for 2π.
- If dir is 0 and a2 > a1, then a2 −= 2π. If dir is not 0 and a2 < a1, then a1 −= 2π.
- +0x10 = a1, and +0x20 = (a2−a1)/(max−min).
- The needle angle is v·k + a1; min is not subtracted, but it is always 0.
- The struct is 0x5ef020 for speed and 0x5f14b0 for revs.

**Call (2): the needles.** They are flat kind-0 triangles in screen space, put straight into the depth buckets with 0x49c590 [asm, px exact].
- Each corner is the centre plus (along, across) turned by the angle a:
  - x = cx + ftol(cos·along − sin·across)
  - y = cy − ftol(sin·along + cos·across)
  - Then the base point is added.
- **Speed needle**, with v clamped to 0..max (lo / hi values):
  - Corners: tail (−5,0) / (−10,0); tip (ftol(radius), 0); sides (0,−3) / (0,−6) and (0,+3) / (0,+6).
  - Triangle (tail, tip, side−) has colour 0x45a2e0(0x19,0x10,0).
  - Triangle (tail, side+, tip) has colour 0x45a2e0(0x1f,0x10,0).
- **0x45a2e0(r,g,b) = r<<11 | g<<6 | b.** Green is shifted by 6, not 5, so the two colours are the oranges (206,130,0) and (255,130,0).
- **Rev needle:**
  - The target is ftol(int O+0x11c ÷ float [0x5092ec + bike number·0x124] × 16000). 0x5092ec is C+0x74, RegimeMax, of the carac row 0x509278 (written here for a while as "bike+0x70" from the table 0x50927c, which the port read as C+0x70, NbSpeed: the needle stood at the end of its range as soon as the engine ran, in the port only, until 2026-11-01).
  - The shown value moves toward the target by at most ±3000 (0xbb8) per drawn picture, stays ≥ 0, and is stored in 0x4f0e10. That limit is per picture, not per 60th of a second.
  - One triangle with corners (−2,+2)/(−4,+4), the tip, and (−2,−2)/(−4,−4), colour (0x1f,0x10,0).
- **Depth:** the needles use sort key 4.0 (0x40800000). That puts them in front of the face (15.0) and of all 3D, which starts at Z 64.
- **Vertex records** are at 0x5f1328 (14 records of 0x1c bytes): Z 2048 (0x45000000), 1/Z 0x3a000000, light 0x3e000.
- **Shadows** are black copies moved by minus the shadow offset, with key 8.0. The set-up loop at 0x48553d gives their records **Z = 1.0**, which is behind the near plane at 64. So the software renderer never draws them, and q17 confirms it.

**Call (4): the digits**, drawn after the render with 0x493fe0, which does not scale [asm, px]:
- **Type 1:**
  - Font 16 SpeedLcdLowRes (15 in hi) draws `"%3d"` at base + speed-digit offset, flags 0x21.
  - Font 23 MotorLcdLowRes (22 in hi) draws the gear as `chr(gear + 0x30 + 7)`, which are glyphs 55..61, the red ones. Other Types add 0 instead of 7.
- **Type 2:**
  - `"%d\n%c"` in font 0x1c (28 Counter; 29 under SmallFonts) at (6,4) with flags 0x90: bottom-right corner, lines right-aligned. The character is ':' for km/h or ';' for mph (the unit glyphs).
  - Before that, a blinking gear-shift hint bar in font 11 Sprites (font 0x4f0d74; 10 under SmallFonts):
    - Shown only with the manual gearbox (0x4dc0a0 == 0), with 0x4dc0b4 == 0, and while the picture counter 0x4f0dc4 has bit 2 clear.
    - The bar is the character `'O' + (0x4f0dc4 & 3)`, repeated max(1, (W/8) ÷ [font+0x774]) times.
    - It is drawn with 0x493f20 at (0, H/16), centred. The flags are 0x80|2 for change down (near the bottom) or 0x20|2 for change up (top).
    - Change up: gear < the bike's gear count (float at bike+0x6c), and rpm (float O+0x12c) + 2 > O+0x90+4·gear.
    - Change down: gear > 1 and rpm < O+0x8c+4·gear.
    - No bar when the rider is crashed (O+0x30c1), when O+0x13c or O+0x84 is set, when O+0xec ≤ 0, when gear ≤ 0 or above the gear count, or when O+0x94 is 0.

**The dial face is a 3D model, not a picture.** It is the decor-library model **"counter"** in each track's .trk file. It sits next to the six turn-sign models g_45, d_45, g_90, d_90, g_chica and d_chica.
- At race load, 0x473370 fills 16 HUD slots at 0x5d2eb8 + i·0x18:
  - Slot fields: +0 library index, +4 x, +8 y, +0xc active flag, +0x14 timer.
  - Slot 0 is "counter" at (0x5d, 0).
  - Slots 1..6 are the six signs at (320, 400).
- Library records are 0x79 bytes, in 0x4dc0d8: +0 faces, +4 vertices (xyz floats), +0x2c face count, +0x30 vertex count, +0x70 name.
- 0x486310 draws 8 slots. Slot 0 is drawn when ShowCounter is on and its active flag 0x5d2ec4 is set; call (1) sets that flag to (Type == 1).
- Vertices are placed from the bottom-right corner:
  - X = ftol(vx·sx + (W−1) − x·sx)
  - Y = ftol(bottom − y·sy − vy·sy)
  - For the face, sx, sy are 1.9 and 2.0 (0x3ff33333, 0x40000000) in hi, otherwise 1.0.
- **Depth and kind:** the face uses key 15.0 (0x41700000). It also sets 0x4f1a18 = 1, which makes 0x49c590 **skip the detail-level kind remap** (0x49c64b). So the face stays kind 38 (texture page 7, texel 0 see-through) at every detail level.
- In lo at 640x480 the face covers 545..640 × 411..479 [px exact].

##### 3b.1 The tool
- `python3 -I tools/hud_dials.py draw ILE SPEED RPM GEAR out.png [--bg CAPTURE.png] [--hires]` draws the dashboard.
- `python3 -I tools/hud_dials.py fit CAPTURE.png SPEED GEAR [ILE]` finds the track and dial value that best match a capture.
- Results:
  - **q17:** track ile02, 0 pixels differ.
  - **q14:** 45 pixels differ. In the capture the speed digits are blank where "  0" was expected, and the gear shows only a small red mark (countdown).

---------------------------------------------------------------------------
#### 4. Bikes, shadows, level of detail, lighting

##### 4.1 The bikes 0x458b80(list, count) [asm]

For k = 1 .. [0x509bac]−1: O(k)+0 = 0 (drawn flag); skip if not loaded (+0x68).
- 0x46e8b0(CAM, O(k)+0x30, 512.0): visible if |P − campos| ≤ 2·LenBPlane and the
  plane tests pass with a margin of −4·512 = −2048 (planes +0xd4 > −2048, +0xe4 < 2048,
  +0x104 and +0xf4 > −2048).
- On-board ([0x4dc008]) and k == player: 0x462110(O(k), renderer, CAM, O(k)+0x307d),
  the cockpit: the handlebar model [0x504ed8] and cockpit model [0x51bf60] placed
  relative to the view, faces through 0x49bca0. Skipped when CAM+0x21c ≠ 0, in a
  replay, or in Pocket ([0x4dc078]).
- Else drawn only if its section O(k)+0x31c1 is in the track's visible list:
  0x45f290(O(k), renderer, CAM, −1) (the bike drawer, NOTES §4.15), [0x4f0dc0]++,
  **0x461f70 the shadow** (§4.7), O(k)+0 = 1.

##### 4.2 The level of detail 0x45f160 [asm]

d = |O(k)+0x3c − CAM(O(k)+0x31dd)+0x98| (computed as sqrt((dy²+dz²)+dx²)) /
zoom [0x6d1f74] → O(k)+0x634. Then (compares made on the float bits as integers:
0x451c4000 = 2500.0, 0x453b8000 = 3000.0):
- d ≥ 3000 → LOD 4;
- 2500 ≤ d < 3000 → 3;
- else, if [0x4ea4e0] (a "low detail" switch [guess]) → 2 for every nearer bike;
- 1000 (0x447a0000) ≤ d < 2500 → 2;
- d < 1000 → 0 (a separate test at 200.0 = 0x43480000 also gives 0: dead split).
(The zoom comes from 0x49a150, which returns [0x6d1f74].)
Result in O(k)+0x638. Zoom divides, so a zoomed TV camera keeps the near models.

0x467b30(O(k), flag): crashed or O(k)+0x319d → LOD 1 (the crash model); else
0x45f160. If LOD ≤ 1: flag → 0x45d810(O(k), PersoAnim) (the rider animation)
else copy pose #0 of the ROT. Called each step from 0x45d7d8, 0x45f12c, 0x46c783,
and from Game_Frame when [0x4dc138].

##### 4.3 Where the light comes from

- **0x465d60(obj)** returns the bike's light level [asm]. With no track loaded (0x52c10c == 0) it is 0x1f. Otherwise it reads the rider's section (track sections at 0x52c108, stride 0xfc, index obj+0x31c1) and looks at the section's flag +0xe0: 0 gives **0x1f** (lit), anything else gives **0x10** (dark). It never returns 0, so the drawer's "light ≠ 0" tests always pass.
- The drawer keeps this value in its local L0x128. 0x49b2c0 (the vertex transform) stores `light << 13` in each output vertex at +0x0c. So 0x1f becomes 0x3e000 (unlit) and 0x10 becomes 0x20000.
- **0x4dc130** ("race lighting"): starts at 0. 0x410120 (race load) sets it to 1. It is set back to 0 at 0x44e4e7 and 0x44ed16 (page 10 start and end) and at 0x4522b2 (podium 0x452240) [asm]. It is also read by the decor drawer 0x4036a0, which then calls a different lighting routine, 0x422810 (not followed).
- **0x4ea4d0** starts at 0. Its only writes store 0 (0x44e4f9, 0x44ed11) [asm]. So the skeleton lighting 0x462a40 never runs, and neither does the "kind 3 stays 3" branch of 0x462c30 (SKL face kinds: 3→1, 4→2 always) [decomp]. **Dead code in the shipped game.**
- **0x4ea4cc** = 0xff is written at 0x45f51d and 0x4619ed and never read [asm]. Dead.
- The light vector 0x52c228/22c/230 = (−0.5, −1.0, 0.0) is set in 0x410120.

##### 4.4 What the light word does to a pixel: only "< 0x3e000"

No span routine the bikes reach uses the light level itself. Each one draws either the full colour or **half = (c >> 1) & 0x7bef** (mask = word 0x50270c). The levels 8..24 that 0x422540 computes therefore never show [asm]. Which test applies depends on the face's path:

**Paletted path.** A face goes this way when the model is drawn with a skin ≥ 0 through 0x49c110 *and* its page has palettes (page record 0x4f9300 + page·0x338 ≠ 0; in practice page 0 = bnk00_00 of every family). 0x49ca70 puts it in a bucket entry whose +0x10 points to the skin's palette. The dispatcher 0x4aef20 then takes the 8-bit branch (jump tables 0x4af100/0x4af0e0) and switches on face+0x3a, which here holds the **raw kind** [asm]:

| kind | routine | pixel | light |
|---|---|---|---|
| 0 | 0x4a3cf0 → 0x4a34f0 | flat: colour < 256 → palette[colour], else the RGB565 value as is | **average of corners 0,1,2 (integer /3) < 0x3e000 → half** |
| 1 | vertex 0's word ≥ 0x3e000 ? 0x4a7b70 : **0x4a8c70** | 0x4a8c70 → triangle 0x4a85e0 → span 0x4a83d0 → pixel 0x4a8340: **half(palette[texel])**, opaque | the vertex tested is **vertex 0 of the whole transformed array** (bucket +0x0c), not a corner of the face |
| 2 | ≥ 0x3e000 ? 0x4ab070 : **0x4ac1f0** | both hold the pieces back in deferred batches (0x625790, 0x24 bytes), drawn by 0x4af130 after all buckets. Unlit: batch kind **40** → span 0x4aa6c0, palette[texel], skipped when the palette colour is 0. Lit: batch kind **41** → 0x4ab840: same, but **half(palette[texel])** | same test |
| 3 | 0x4a6a50 → 0x4a62d0 → 0x4a6080 | the span is **0x4a6070 = `ret`**: paints nothing, but opaque: it closes its free pieces, so the clear colour shows through it | — |
| 4 | 0x4ad410 | batch kind **19** (0x13) → span **0x4ac9c0 = `ret`**: paints nothing, closes nothing | — |
| 9 | 0x4ae730 | batch kind **20** → 0x4adbf0: shades along the span (step 0x655d30). Palette colour 0 → screen halved. Otherwise the red channel comes from table 0x4f62f0[red][level] (in BSS, no writer: read from the running program, 4100 bytes of zero but for the next variable's 4 at its end, so red is simply gone: c & 0x7ff) and green and blue are kept | interpolated |
| 42 | 0x4a9df0 | batch kind **42** → 0x4a9440: **flat colour wherever palette[texel] ≠ 0** (a silhouette of a see-through face), deferred | colour halved when the 3-corner average < 0x3e000 |

The kind table 0x4f1a70 is rearranged at start-up by 0x4a2ea0 → 0x4a2e00 so that row = kind id (0x47 rows, unused rows −1) [decomp]. That is why the batch kinds above index it directly through `0x4f1a7c + kind·16` (its span field).

Kinds 3, 4 and 9 never reach a bike. 0x420a50 makes only kinds 0/1/2/8/0xd/0x10/0x15/0x26/0x27 from a MAP, and 0x462c30 turns SKL kinds 3/4 into 1/2 because 0x4ea4d0 = 0 [asm, decomp]. In the 16-bit table, kinds 3, 18, 4, 9, 8, 29 and 5 are `ret` stubs too (0x4a4bd0/4bd0/5a80/5aa0/5a90/4be0/4bc0).

**16-bit path** (0x49c590, entry +0x10 = 0): spans of kinds 0, 16, 21, 38, 39, 44, 62 … **ignore the light** (NOTES §4.13.2.3). The only darkening there is the wheels' kind swap, **0x4221d0(model, flag)** [asm]:
- flag ≠ 0: every face of kind 0x10 → 0x15 (21: span 0x4a5e10 = half(texel)) and 0x26 → 0x27 (39: span 0x4a5580 = see-through halved);
- flag = 0: swaps back.
- It is called before each wheel with flag = (L0x128 ≠ 0x1f). It is called only on the two wheel models (0x45fcd6, 0x460ce6). Kind-0 wheel faces are never darkened.

**What gets darker, by part** (race, 0x4dc130 = 1):

| part | in a lit section (0x1f) | in a dark section (0x10) |
|---|---|---|
| skeleton, LOD 0/1: faces on page 0 (paletted, skin +0x5fc ≥ 0) | full | **half** (word 0x20000 on every vertex) |
| skeleton faces on page 1 (RGB, kinds 0x10/0x26, 16-bit path) | full | **full** (the light is ignored, no swap) |
| LOD 2–4 model (0x53a788/7f8/868), faces on page 0 | full | half |
| **road wheels** (kinds 1/2 on page 0; skin +0x2f4/+0x478 ≥ 0) | **always half** | half |
| dirt wheels (skin −1 because PersoAnim == 1 → 0x49b7f0, page 1 RGB kinds 0x10/0x26/0) | full | half for 0x10/0x26 through the swap; kind 0 stays full |
| shadow | 50 % blend, independent of the light (§4.7) | same |

The road wheels are always half because the race always takes the lit wheel drawer **0x48c6d0** (condition: 0x4dc130 alone, 0x45fcb6/0x460cc6). It runs 0x49b2c0 with 0x1f, then **0x422540** overwrites every vertex word with 8..24 (< 31). Its faces then go to 0x49c110 with the skin, or to 0x49b7f0(…, 1) from face index model+0x68 (= 0) without a skin [asm].

**[px] check** (build/trackcap/p1a.png: a road bike, near LOD, lit section). I matched each pixel's RGB565 value against the 8 SPEED liveries, full and halved. The body area (265..335, 165..235) has 3360 pixels that match only a full colour and 90 that match only a halved one. The rear tyre matches mostly halved colours. So the body is unlit and the wheel is halved, as the table says.

##### 4.5 The lighting routines

**0x422540(model; a0, light xyz, M[12], out vertices, pos xyz)**, thiscall, `ret 0x50` [asm]:
- It returns at once if **model+0x10 (vertex normals) == 0**. Only the wheel models have normals: 0x413450 calls 0x41fa10 on 0x52c240/0x51ac58 (+s·0x70) and nothing else. So for the LOD 2–4 models the drawer's lit branch (0x45f407: 0x4dc130 && light) calls 0x422540 for nothing, and their words stay light << 13 [asm, decomp].
- M is the model-to-world matrix (L0xdc, translation included). The routine builds the rigid inverse (the transpose, T' = −T·Mᵀ) and moves the **light vector, treated as a point**, into the model frame: P = (light − T)·Mᵀ. It then makes P unit length when it is non-zero (compared with 0x4d0664 = 0.0) and negates it: l = −P/|P|.
- Per vertex i < model+0x30, with n = model+0x10 + 12i:
  - d = (n.z·l.z + n.y·l.y) + n.x·l.x
  - a = trunc(16 − 16·d) (0x4d069c = 16.0, bits 0x41800000)
  - b = trunc((a + 1)·0.5) + 8 (0x4d06a0 = 1.0 0x3f800000, 0x4d0680 = 0.5 0x3f000000)
  - b ≥ 32 → 31; out[i]+0x0c = b << 13.
  - Range 8..24, so always below 0x3e000.
- a0 and pos are pushed but play no part in the formula as far as I read.

**Vertex normals 0x41fa10** [decomp]:
- Face normals at model+8 come from 0x41f920 when missing: n = (c−a)×(b−a) per component, in the order the reference writes, scaled by 1/sqrt(x² + z² + y²).
- Each vertex adds the normal of every face that uses it: 3 corners, or 4 when face+0x34 ≠ 0xffff.
- The sum is multiplied by 1.0/count, then made unit length unless its length is 0.0.
- A vertex no face uses would divide by zero (NaN). None does in the wheels.

**0x462a40 (dead).** For the record: light normalised (when its length ≠ 0), then per bone of LOD obj+0x638 rotated by the bone's 3x3 at obj+0x654 + … and negated. Per vertex (normals obj+0x2d60[lod], first index bone+0x690, count bone+0x694): word = min(trunc(20 − 10.666667·d), 31) << 13 (0x4d0f98 = 20.0 0x41a00000, 0x4d109c = 10.666667 0x412aaaab). No +8, no halving step [asm].

Reference: **tools/bike_light.py** (face_normal, vertex_normals, light_words, section_light, half, shadow_pixel). `python3 -I tools/bike_light.py wheels build/bkf SPEED` gives levels 8..24 on all 47 vertices of each wheel, all below 0x3e000.

##### 4.6 Face setup in the race: kind remap by detail

- **0x49c110** (skinned models) [asm]:
  - a face is skipped if flag bit 0 is set and its kind is 0xd;
  - key = the farthest corner's Z (no +1024);
  - back faces are culled with the area test "> 1.0" (an integer compare of the float, 0x3f800000);
  - a page with palettes goes to 0x49ca70(face, key, verts, skin, clipflags), any other page to 0x49c590(face, key, verts, clipflags, 0).
- **0x49ca70** (paletted) [asm]:
  - bucket = ftol(key·0.25) (0x4d1f9c 0x3e800000), clamped to 0x1f3e; a negative bucket is dropped; at most 0x1d4b faces;
  - face+0x3a = raw kind;
  - **if the pointer 0x4dc0c4 ≠ 0, *0x4dc0c4 == 0 (the race) and detail 0x4dc098 ≤ 1**: face+0x3a = 0, or **0x2a** when props[kind] & 9 == 9 (textured + see-through: kinds 2, 4, 0x13, 0x26–0x29, …).
  - So at detail 0/1 a paletted bike is drawn flat (kind 0, colour = the MAP's palette index), and its see-through faces as flat silhouettes (kind 42).
  - Entry +0x10 = [0x4f9304 + (page·206 + skin)·4] (palette of that skin), +0x14 = skin.
- **0x49c590** (16-bit; the shadow, dirt wheels, RGB-page faces) [asm]. Its race remap (jump table 0x49ca54 by detail 0..5) runs only when the kind has no 0x40 prop and 0x4f1a18 == 0:
  - details 0/1: textured faces nearer than 3072.0 (0x45400000) get props |0x20; then props &= ~0x85 (drop textured, half and RGB → flat);
  - detail 2: the same, except faces with flag bit 3 keep their texture;
  - detail 3: |0x20 if textured and key < 3072;
  - details 4/5: |0x20 if textured and key < 6144 (0x45c00000);
  - then face+0x3a = 0x624b58[props].
- The props bits come from 0x4995a0's switches [decomp]: 0x01 textured, 0x02 shaded (the stub kinds), 0x04 half, 0x08 see-through (texel 0), 0x10 blended, 0x20 perspective (sub 6), 0x40 not drawn/special, 0x80 RGB page.

##### 4.7 The shadow

**Points (0x461aa0, called each step by 0x45d580/0x45e940/0x46bf80)** [asm]:
- L54 = (40, 0, 0) through the inverse of R(heading angles); L48 = (0, 0, 40)·bike matrix (+0xbc).
- p0 = (front + L48) − L54 → +0x2ea9;
- p1 = front + L48 + L54 → +0x2eb5;
- p3 = (rear − L48) + L54 → +0x2ecd;
- p4 = (rear − L48) − L54 → +0x2ed9;
- p2 = (p3 + p1)/2 → +0x2ec1; p5 = (p0 + p4)·0.5 → +0x2ee5;
- front = +0x1b4, rear = +0x338.
- Each point is dropped to the ground (0x40c7e0 section, 0x40c960 height), then y += **4.0** (0x40800000) for p0, p1, p3, p4 and **8.0** (0x41000000) for p2 and p5.

**Faces (built once in 0x45bf70 with 0x422de0(kind, colour, c0..c3, u0,v0 … u3,v3, page))** [asm, pushes followed]:

| face | kind | colour | page | corner: (u, v) |
|---|---|---|---|---|
| A, obj+0x2ef1 | 0x2c (44) | 0 | 1 | p0 (114, 1), p1 (114, 35), p2 (57, 35), p5 (57, 1) |
| B, obj+0x2f31 | 0x2c | 0 | 1 | p5 (57, 1), p2 (57, 35), p3 (1, 35), p4 (1, 1) |

- uv constants: 0x42e40000 = 114, 0x42640000 = 57, 0x420c0000 = 35, 0x3f800000 = 1.
- uv are then ×1/256 (0x4232e0) and grown back to texels at setup.
- face+0x38 = 0, so no +1024 on the key.
- The front of the bike is at u = 114 and the rear at u = 1. v = 1 is the −L54 side, v = 35 the +L54 side.

**Crashed second shadow** (0x52c234 NewMotorEngine && crashed +0x30c1): 4 points at obj+0x30ed and two triangles built with 0x422f80 (kind 0x2c, colour 0, page 1):
- obj+0x311d = (0: 114,1), (1: 114,35), (2: 1,35);
- obj+0x315d = (0: 114,1), (2: 1,35), (3: 1,1).
- So the whole 1..114 × 1..35 picture is used.
- The quad is centred on the saved body point +0x30b5 and turned by the heading of +0x3085 (0x490220). Half sizes come from R's rows ×2.0 (0x40000000), ×−2.0 (0xc0000000) and ×32.0 (0x42000000); y = ground (0x40c8b0) + 2.0.

**Drawing (0x461f70, after the drawer, from 0x458b80)** [asm]:
- camera C (+0x68..+0x84), T = (−campos)·C + camera T;
- 0x49b2c0(matrix, obj+0x2ea9, 6, 0x1f);
- **0x4dc098 is saved, set to 3**, then 0x49b7f0(verts, obj+0x2ef1, 2, 1), then restored. Forcing detail 3 keeps the texture at every detail setting;
- the crashed pair: 4 vertices, 2 faces, done the same way.
- Through 0x49c590 at detail 3, kind 44 (props 0x91) becomes **kind 62** (props 0xb1, span 0x4a5890: perspective-correct, divide every 16 pixels) when key < 3072. Otherwise it stays **44** (affine, span 0x4a57f0).
- Both spans [asm]: texel = page[(v>>13 & 255)·256 + (u>>13 & 255)]; **texel 0 leaves the screen untouched**; otherwise screen = ((screen >> 1) & 0x7bef) + ((texel >> 1) & 0x7bef).

**The picture**: the family's **BNK00_01.RAW** (24-bit; page 1 in the race, 0x410590), texels (1..114, 1..35):
- a top view of the bike, pixels 0 (none) or RGB565 **2** (B = 16..23, R = G = 0);
- texel 2 halves to 1, so each shadow pixel = **half(screen) + 1 in blue**;
- SPEED/SPEEDPKT: 2351 shadow texels and 1639 empty; KTMCROSS/KTMPKT: 1639 shadow and 2351 empty (different shapes).
- tools/raw.py reads this 24-bit file as 8-bit (empty palette). Read it as 256×256 RGB after the 32-byte header (model.load_page does).

**[px]** In p1a.png the shadow beside the bike matches exactly. The road (16,32,15) gives (8,16,8), (17,34,16) gives (8,17,9), (14,28,13) gives (7,14,7) and (15,30,14) gives (7,15,8), in 5:6:5 units.

##### 4.8 Particles and skid marks [asm for the loops, decomp for the details]

**Particles 0x4575c0** [asm]: 128 records of 0x104 bytes at 0x52c498 (live when
+0xfc ≠ 0); each live one drawn by 0x4b0be0(record, renderer, CAM). 0x4b0be0
[decomp]: only when detail [0x4dc098] ≥ 2; a counter [0x4f258c] runs over the calls
and only every 8th particle is drawn at detail 2 (every one at 3 and 4). Kinds
+0 = 1..4 [decomp]: a camera-facing quad (4 points at +0xc0, 2 faces at +0x40,
through 0x49b2c0(..., 4, 0x1f) and 0x49b7f0(..., 2, 1)); its matrix is the camera's
C times a rotation built from the table (the camera roll +0xc0 and the particle's
own angle), so it stays flat to the screen. Spawned by 0x457620 → 0x4afe90 (from the
bike post-step 0x466d70 and 0x467c40) and 0x4576a0 → 0x4b00e0 (0x466d70); moved
each step by 0x457590 → 0x4b0730 (Game_Frame step 15). Which kinds are smoke, dust,
sparks or water and their pictures: not followed (the post-step helper's
0x466d70 decides when they are made).

**Skid marks 0x4573e0** [asm]: 16 strips of 0x1608 bytes at 0x542a28: 64 faces
(0x40 bytes each) at +0, 128 points (12 bytes) at +0x1000, face count +0x1600,
point count +0x1604. Each strip with faces: world → view (C, T' = −campos·C + T),
points through 0x49b2c0(..., count, 0x1f), faces 0x49b7f0(..., count, 1).
Built once by 0x457300 → 0x457120 [decomp]: 32 quads as pairs of triangles
(points 2i, 2i+1, 2i+2 / 2i+1, 2i+3, 2i+2), kind 0xc, colour 0x45a2e0(8,8,8) (near
black). 0x457350 starts the next strip (ring of 16, index [0x4e9bd0]);
0x457390(strip, ...) → 0x457200 adds a point pair; both called from the bike
post-step 0x466d70 (so: tyre marks laid while braking/sliding [guess, strong]).

##### 4.9 Open points (bike drawer)
1. Which track sections have +0xe0 ≠ 0 (tunnels? [guess]). A capture in one would confirm the skeleton/LOD halving and that RGB-page faces stay full.
2. Table 0x4f62f0 (kind 9's red table) has no writer found by its exact address, and is zero in the running program (§4.4 above). It does not matter for bikes (kind 9 never reaches them).
3. 0x422540's a0 and pos arguments: pushed, no use found in the formula. Worth one more read if the exact levels are ever needed (they do not reach pixels).
4. Decor lighting 0x4036a0 → 0x422810 (also gated by 0x4dc130) was not followed. It could darken decor the same way.
5. The detail value 0x4dc098 during a race (from Options) decides the flat (0/1) vs textured bikes and the near-perspective threshold. Confirm the default in the original.
6. The dirt families' wheel skin −1 rests on 0x45bf70's p4 = −1 when PersoAnim == 1 (NOTES §4.15.2.1). Not re-checked in a dirt capture.

##### 4.10 Rough C size (bike drawer race parts):
- section light and the lit flag ~15 lines;
- half variants of paletted kinds 0/1/2 plus the kind-42 silhouette and the deferred lit batch ~70;
- wheel kind swap ~10;
- 0x49ca70 detail remap ~10;
- shadow points 0x461aa0 (on top of the existing ground-height code) ~60;
- crashed quad ~40;
- shadow faces, uv and draw with detail forced to 3 ~40;
- blend spans 44/62 ~40 (62 may already exist for the tracks);
- LOD choice ~30;
- optional exact 0x422540 + 0x41fa10 ~70 (not needed for pixels: "lit wheel → half" is enough).
- **Total ≈ 310 lines, ≈ 380 with the exact lighting.**

---------------------------------------------------------------------------
#### 5. Summary for the port

##### 5.1 How sure
- Frame order (§1): [asm] throughout, all calls placed. No split screen: one camera
  block (0) and one full-screen viewport. Two players on one PC exist only as Link
  (DirectPlay), which goes against the scope in CLAUDE.md; Danik has to decide.
- Cameras (§2): chase, on-board and look-at [asm]. Their matrices were checked against
  every dump: 18/19 chase (q32 dumped mid-frame) and all mode-3 dumps to 5e-8;
  the rest pose q44 is exact; fx = 394·zoom. Bike-mounted presets were read in the asm
  but not checked (the bike matrix was not dumped). The TV director is from the decomp only.
- HUD (§3, §3b): q17 is exact in every text and in the whole dial box; p3 and q14 are
  exact apart from guessed values and see-through edges. All captures use small fonts,
  so the hi-res layout is unchecked.
- Bikes (§4): LOD, visibility and shadow [asm]; the shadow blend and "lit wheel = half"
  were checked on p1a.

##### 5.2 Open points across sections
1. The chase lerp (0.2) and the rev-needle limit (3000) are applied per frame, and the
   blink masks count frames. GOG is capped at 60 fps (-FrameRateMax60) and the CD at
   30, so the port's fixed 60 Hz steps match GOG. A CD mode would need 30 Hz steps for
   these to match.
2. The hi-res HUD (small fonts off) and F4 16:9: one capture of each.
3. Sections with +0xe0 ≠ 0 (dark: tunnels?) and the decor lighting 0x422810.
4. The CD data's sprite messages (0x4dc180 = 0).
5. Particle kinds and pictures (0x4afe90 / 0x4b00e0), and when 0x466d70 spawns them.
6. The finish figure 0x47b7a0's constants; the track map 0x40f2d0 (off by default).

##### 5.3 Rough size of a C port (on top of r3d.c, land.c, text.c, font.c)
- frame order and draw glue ~150 lines
- cameras: chase, on-board, look-at, presets, Tab/F5/F6, finish orbit ~300; TV director ~250
- HUD texts, countdown, messages, panels ~600
- dials, dial face, turn signs ~370
- screen set-up and viewport ~100
- bike visibility, LOD, shadow, lighting halves, kind remaps ~310
- particles ~60 to ~150, skid marks ~80, finish figure ~100, track map ~120
- In all, about 2,500 to 2,800 lines.

### 4.21 The computer riders (mapped and written 2026-10-26)

The full map is build/notes/cpu_riders.md; the port is port/src/cpu.c (the routines shared
with the player's bike through bikephys_int.h); the check is tools/cpucheck.sh.

- **Who**: 0x45d540 steps a rider with obj+8 == 0 through **0x46bf80**. In Solo riders 2..24
  (the attract demo: 1..23). All use carac row 0 of the track's family (road or dirt). They
  share the C library's rand() (msvcrt, per-thread seed) with the player's crashes. [asm]
- **The step 0x46bf80**: the driving 0x46b120, the walls (cpu_speed and engine halved,
  hit_timer 200), speed = engine, now and then the turbo on a straight (rand() & 0xff == 0
  and cpu_speed > top), a dirt bike flies above road + 8 (straight) / + 32 (bend), else it is
  put on the road (dirt bumps: rand() % (engine / top x 8)) with its matrix from the road
  under the axles, the wheel points, 0x469c60, a lean x 1.5 (dirt) / x 1.2 (road), smoothed
  by half, then the same tail as the player's step (spin, suspension, tumble, dials). [asm]
- **The driving 0x46b120**: cpu_speed grows by dt / 6 m/s to the lane's top (top x (1 +
  lane / 40)); the bike turns a quarter of the way a step towards a point on the racing line
  (section +0xb0) + lane_x, 16 sections ahead (dirt 9); its speed follows a mix of the motion
  and the heading, its motion turns towards a point 4 sections ahead (dirt 2), weighted by
  the hit timer (k = hit_timer x 0.005, at least 0.01); 1.28 units a step per m/s x dt. In
  the air a dirt bike falls (dt x 0.24525). During the countdown it does not move. [asm]
- **The lanes 0x46cd20**: three lanes 240 units apart; back towards the inside lane when free;
  behind a slower rider: to a free lane next to it, or to the lane whose nearest rider is the
  fastest if more than 6 sections ahead, else slow down to him (x 0.95; braking when that
  drops more than 40 km/h). lane_x slides at dt x 1.0667 a step. [asm]
- **Their top speeds 0x47bc60** (Solo): Level.Spt's ComputerSpeed (0x51be98, km/h) from the
  back of the grid, / 3.6, x (easy 0.9, medium 1, hard 1.05) x (1 / FactorPocket with
  Pocket), then opponents / 2 random swaps (not in a replay). Level.Spt's 13 AI rows
  (RATTRAPAGE_SPEED...) are read by nothing: no rubber-banding. [asm; the swap not recorded]
- **Bike against bike 0x47ecd0**, fixed in bikephys.c while checking: the speeds from a
  double length in the order (y² + z²) + x²; a hard crash first cuts both speeds to
  22.222221f (80 km/h) for a computer A too; B's new direction is sA nA + sB nB after that
  cut, with sB nB.z spilled to a float; the 5.5556 floors are 5.555555555555555. After it
  0x47e3d0 walks each rider's section again (race.c copies it back). [asm; test]
- **Not in cpu.c**: the drawing rewrites the front wheel point +0x1b4 of the riders drawn in
  detail after the step (the next step's front wheel spin reads it); the shadow, effects and
  pose are drawing.
- **Check** (tools/win/cpuwatch.c records riders 1..24 + rand() seed each frame; three attract
  demos, the original's sound to a null sink): whole frames (every rider, then 0x47ecd0 and
  0x47e3d0) bit-exact but the front point: West Way mid-race 930 / 931 (the one is a sample
  taken between the step and the drawing), Snow Ride (dirt, 101 airborne states)
  1250 / 1250, West Way from the countdown 1562 / 1562; the rand() seed after every frame
  3743 / 3743. [test]

### 4.21b Replays, demos, the TV director, the winner's lap (written 2026-10-30)

**The recording** (`replay.c`): 0x558aa8, 0x4fb0 = 20400 records of 8 bytes: word 0 = dt << 24 |
keys, with bit 7 = +0x31f9 (turbo held) and bit 6 = +0x3201 (turbo pressed); word 1 = x << 16 |
y (the stick). Clear 0x4577c0, rewind 0x4577e0, add 0x4577f0, play 0x4578a0. Everything else is
played again from the same start: the seed 0x4f0c48 is kept (0x47bf70: kind 1 reuses it, else
timeGetTime). `tools/replaycheck.sh`: a race and its replay give the same sum of every rider's
place and speed, step for step, on three tracks. [test]

**The race's kind 0x4e9834**: 0 a race, 1 its replay, 2 a demo by the track's name. 0x4e9830 =
playback (demos only: YES on the results gives kind 1 with playback 0, *so kind 1 without
playback is reachable*). 0x4e8944 the hotkeys F5, F6, F7 (off in a demo and the winner's lap);
F5 with Shift: the local player finished or out → the next loaded rider in 0x508834; F6 not in
kind 1. A replay running out with no playback: message text 116 (0x53b7b8) for 5 s, kind → 0,
the race goes on, the player's again. Results saved and put back around a replay
(0x463c30/0x463ca0: time, best, state, checkpoint, laps, place). Best lap written only in kind
0; the podium's data not in kind 1.

**The mark** 0x486660 (kind 1 only): "^" (0x4f1220, which font 11 draws as an R) through 0x4188b0
(id 0x4e9898, life 2000, align 0x81, x 2, y 2, effect 3, blink −500); in playback "DEMO"
(0x4f1224, align 0x22, y 10, effect 4, font 12).

**The demos** (`Data\Demo\EA.dem`: `NbItems: 2`, `DemoN: TRACK ms picture`): 0x409a30 after 30 s
on page 14 (menu+0x24). Kind 2: mode 1, 3 laps, no reverse or pocket, automatic, bike 0, 23
riders in all, 1..23 (0x410a50 skips the player's own, so the dashboard says /23, seen in the
original; the 24th grid place stays empty), rider 1 the computer's (+8 = 0), the screen follows rider 2 (0x508834), the riders'
speeds not shuffled (0x47bd82), detail forced to 4 (0x472d2e). Ends at clock > 90000: events
0x120000 (the next item and load: the same track restarts in place, else unload and page 1;
none left → page 14). Pause rows: demo [NEXT DEMO] 107, [QUIT DEMO] 106, [CONTINUE DEMO] 108;
replay [CONTINUE REPLAY] 109, [QUIT REPLAY] 111 (championship only).
*The port's deviation*: the player's choices and detail are put back when the demos end (in the
original they stay as the demo left them).

**The director** 0x46e9f0 (§4.20 §2.7; `camera.c`): state 0x4ea5a8 (image 1), timer 0x4dc13c
(image 600), snap 0x4ea510, stream Rand_Next(0); timer = trunc(timer − dt). On when playback ||
0x504ea0 (winner's lap) || kind 1, and cameras 0x51a40c > 0. Then the view's section by the whole
track (0x40c740), not by the bike's. 0x4e8968 and 0x4dc134 are never set.

**The split** 0x412e50 (`rules.c`): reference 0x581018 from bestscor: the row with the player's
name (0x465290, stricmp), else the last row (0x4654e0, 0x465070), else 0x7f. Not in playback,
nothing while 0x504ea0. On the dashboard in Solo with 0x519fec > 0: y = rows · 16 + 0x29, font 4 at
x 0x3c; colour ' ' orange 0x4f0dfc, '+' red 0x4f0dec, else green 0x4f0df4; "  X]" font 6 at x 0,
X = '\' for '+'.

**The winner's lap** (§4.17's 0x412960 and 0x412a50; `race.c`, `generic.c`): as written there.
What the original's capture adds (`build/ref/winner_*_orig.png`; to reach it in the original:
start a single race, memtool poke 0x4129c2 eb05 (the place test), then 0x509ba0 = 1 before the
time runs out):
- For the 14 s the dials are off, the laps and "1ST POSITION" stay; with the credits (race+0xc =
  0) the whole dashboard and the messages go. The TV's cameras all along.
- 14000 ms is exactly 875 of the port's even 16 ms steps: 0x4e9824 would land on 0 and skip the
  credits (`jns`: only below 0 goes to 1). The original's measured frame times almost never do
  (seen under wine: 11525, then 1). The port goes to 1 on 0 while the lap is on.
- Not in the port yet: 0x5dabd0 (200 at the start, −dt while 0x504ea0 and state 2), the rider's
  salute in the pose 0x45d810 (from the pose 0x51d584 to 0x51da48 between 200 and 175, held to 25,
  back to 0x51d2a8): the race's rider poses are not written (see the handover).

**The sequence** 0x48aca0/0x48acd0/0x48ae40/0x48b070 (`generic.c`): 0x48aca0(rand() > 0xfff, the C
library's stream): 0x4f1634, index 0x4f1638 = −1, running 0x4f15f4 = 1, CD track 12. Each drawn
frame (0x47ff90, flags & 0x10, no help open) 0x48acd0: the next script when none is open, up to
index 10: 0x4f1640[i] (Short1..7, Short8 ×4), or 0x4f16a0[i] when 0 (Start, Dsi1-3, EA1-3, End,
Short8 ×3); the GP edition's 0x4f1670, 0x4f16d0. A script that fails or is empty (Short7) only
loses that frame. Short: 36 s in all.

**A script's life** (§4.8.4.3, the format): 0x48a4a0 finds `{...}` blocks; 0x48a7c0 splits a
block into lines; 0x488a30 reads a line's tags and texts: a text before a tag or at the end
becomes a text object (0x416170 align 0x21, 0x416300, 0x4163e0 with the tags' colours; *no*
layout slot 6, so no line gap in its height), +0xa4 = 1 (alive off the screen until first seen).
A line without a text, or empty, is dropped, tags kept: all the tags carry on to the end of the
file. The block's fields are the tags' values at its end: L and H/V hold for every line in it
(0x48a000). 0x489cf0: a line's pieces 10 pixels apart ([W] a piece's own width), centred or
right by 0x4f1610; 0x48a000 the lines' heights added up, and centred or at the bottom.
0x48a9e0: a block starts when the one before has nothing alive; 0x48a150 draws it, moved by
MX·(W·dt/MH), MY·(H·dt/MV), on a darkened box ([D], 0x49a5a0) in the software renderer. The help
(F1, 0x48b120, 0x48b180) is the same parser with its blocks stacked: §4.19 §5a.

### 4.21c The time attack's ghost (mapped and written 2026-10-30)

Practice with TIME ATTACK on (0x4dc148): the player's lap, when first over the line, is saved and
rides as rider 2 the next time. `port/src/ghost.c`, the follower in `cpu.c`, the rest in `race.c`,
`rules.c`, `hud.c`, `page_top5.c`, `saves.c`.

**The objects.** Class at 0x419020 (ctor: count 0, at 0, max 10000, a 520000-byte buffer, total 0,
clock 0): 0x52c0d8 the ghost, 0x52c0f0 the run being recorded. +0 count, +4 at, +8 max, +0xc buffer,
+0x10 total (float, ms), +0x14 clock (dt units). 0x4191a0 reset (+0, +4, +0x14), 0x4191b0 rewind
(+4, +0x14), 0x4191c0 record, 0x4192a0 play, 0x4190d0 copy (the samples, count, total; at, clock 0).

**A sample, 0x34 bytes** (0x4191c0, the player's values): +0 clock (the dts added, float), +4 speed
(+0x124), +8 position (+0x30), +0x14 where it points (+0xd4 + +0xe0, the Z row and T of the matrix),
+0x20 turbo (+0x2e85), +0x24 crash flag (+0x31ad, which 0x47df40 clears after recording), +0x28 how
it was thrown (+0x31a1). Past 10000 samples nothing is kept: 160 s at 62.5 steps a second. The
recorder runs through the countdown and on past the line until the end of the race: the first
samples are the countdown's (the original's first clocks: 0.1, 0.6, 2.2, then 1.6 apart).

**The file** (0x4138c0 "wb", 0x4136a0 "rb"), in the track's folder: u32 pocket 0x4dc078, u32 reverse
0x4dc068, u32 laps 0x4dc074, the 0xf9-byte record 0x581018 (name "SHADOW\0" at +0, the splits 0xc0
bytes at +0xd, the rest 0x7f or what was there), u32 count, u32 max, f32 total ms, count x 0x34 samples.
Name: `<0x4e8380 ""><track>\timatak` + ("R" reverse, else "_") + ("_" unless Pocket) + `%1d` laps +
`.bin`. With Pocket the game appends its "P" to the wrong buffer 0x53f3c0 (loader, writer and Reset
alike), so the name just lacks a letter: `timatak_1.bin`, `timatakR1.bin`. The port makes the same names.

**When it is loaded and used.** 0x47b420 (the race's load): both reset, then if time attack the file
(in other modes too, harmless). 0x47bf70 (the start, mode 0): time attack off: R(2)+0x68 = 0; on:
rewind the ghost, R(2)+0x68 = count != 0, reset the recorder. So Continue / YES uses the new ghost
without reading the file. 0x411010: time attack in practice keeps 0x581018 (the ghost's splits, for the
split line) if the ghost has samples, else fills it with 0x7f; everything else fills it and Solo
loads it from bestscor.dat. Port: `rules_restart`, `rules_reference`.

**A step** (0x47df40, Solo, mode 0, time attack): only rider 1 steps with 0x45d540. If R(2) is loaded:
the old target (+0x2df5) goes to +0x2e01, 0x4192a0(ghost, dt, &+0x124, &+0x2df5, &+0x198, &+0x2e85, &crash,
v), +0xec = +0x124, a crash 0x467e30(R(2), v), then the passive step 0x45e940(R(2), dt). Then the recorder
and R(1)+0x31ad = 0. The ghost is not driven: no keys, no computer rider's logic.

**0x4192a0** [asm]: clock += dt; while at < count and s[at].clock < clock: at++. At the end: nothing
changes, so the ghost stays where it was (and the passive step keeps it there). Otherwise a = s[max(at-1, 0)],
b = s[at], t = (clock - a.clock) / (b.clock - a.clock) stored in a float, 0 when negative or not a number
(`fcom 0; test ah,1`), 1 when above 1 by the bits. The mix is the x87's, and the second half of it goes
through float for all but x of the position:
x = a.x(1-t) + b.x t; y, z = a(1-t) + (float)(b t); dir[i] = a(1-t) + (float)(b t); speed = a(1-t) + b t;
turbo from a if t < 0.5, else b; a crash in any sample from the old at to the new at, inclusive.

**The passive step 0x45e940 and 0x45e560** (`cpu_passive_step`, `follow`): the heading blended to the
sample's by 0.9 (0x45b1b0), steering 0, the turn 0x45cb00, +0x18c eased with hit_timer * 0.005 when the
target moved more than 1e-5, position = target, the section walk. Then the wall check (halves the speed,
hit_timer 200, no event bit), speed = engine, the bends (sec, 8); no bend: a rand() draw for the turbo;
the turbo; the same road settling as a computer rider (0x46bf80: `settle`, taken out of `cpu_step`), the frame and
lean; lean x 1.5 on dirt, x 1.2 on the road when not hit (else 0), no clamp, smoothed by 0.5; wheel spin,
suspension, tumble, the pose 0x467b30(1), dials, the lane 0x46d340.

**Not see-through.** 0x458b80 has no case for it: it is drawn solid as any rider (bike row 0 from +0x78,
skin k-1, or 0 if that is the player's bike), which the captures show. **No collisions**: 0x47ecd0 does
nothing in mode 0. **Its sound**: 0x4079e0 for every loaded rider, so race_sound.c has its engine already.
**Places**: rider 2 counts, so POS x/2 and the "2nd position". Rider 2 over the last line (0x47e3d0: R(2),
loaded, laps == 0x4dc074, mode 0, time attack, state 1): time = ftol(0x52c0e8, the file's total); with 1 lap
lap[0] = best = time too. It takes the first place when it is ahead, so the player's place at the end decides.

**The label over it** 0x483670, called from 0x413180 before the state test, in Link or a time attack
(§4.20 3.7): for every other loaded rider with +0 set (drawn, by 0x458b80): pos - eye + (0, 125 [0x4d1a20], 0)
through the camera (0x4818a0, 0x49b230), z >= 0; text 0x4f10bc `"%c^%s\\\n]"` with c = ')' + place and the
name (+0x2dd4), font 5 (0x4f0d70: its low-res partner under small fonts), at x = sx - width[']'] / 2,
y = sy - font height (+0xa18) - 0x5d893c - the text's height (0x492b10, one line = 1), flags 0x21, no halving.
The recolouring 0x490a90 has no effect here: fonts 1 and 5 use their own palette.
The port asks `race_label_place`: 0x46e8b0 (no further than 2 x LenBPlane; forward plane, far plane and the
two side planes of 0x46e320 with a margin of 2048) and the section on 0x40ce60's list (`land_section_seen`).
The bikes ask the same (`bike_seen`, 2026-10-31; until then all were drawn, the label alone asking, see §4.24 below).

**The end** 0x412a50 (asm 0x412d42..0x412dd0): mode 0, time attack, R(1) place 0x5873a6 == 0: 0x52c100 =
(float)R(1)'s time 0x5871fe, copy 0x4190d0 (run -> ghost), 0xc0 bytes R(1)+0x2f89 -> 0x581025, strcpy
0x581018 = "SHADOW", 0x581018 -> 0x581118, 0x4138c0. No other comparison: the file is overwritten when the
player's first; the file keeps the samples past the line up to the end. A time over does not get there.

**Reset** (page 9's "Reset time attack" button, through 0x4b5190): deletes the current choices' file (laps too),
page 6. The race's table stays. Seen in the original: the file gone, page 6.

**Port's differences**: saves go to the home folder like the others (Danik has not decided otherwise);
read from there, else the game's data/. Reset removes the home copy; if the game's folder still has one,
it leaves an empty file in the home folder in front of it (`save_forget`), so that copy no longer counts
and the player's folder is never written. `ghost_load` refuses a file with another header, a count over
10000 or a wrong size (the original would read whatever was there).

**Checked against the original** (GOG's build, -NoD3D -windowed -FrameRateMax60, Speed Bay, 1 lap):
- A file written by the port, in the original: read, ghost count 3402, total 41376.0, plays. Its passive
  step, from cpuwatch's recording of rider 2 (2059 steps), through `ghost_play` and `cpu_passive_step`: the
  target 2059 of 2059 bit-exact, and every field cpucheck compares bit-exact on every step, but the front
  wheel point (2018: the drawer rewrites it, as for the computer riders). The rand() seed: all.
- A file written by the original (rider 1 given to the computer by poking +8 = 0, the time left topped up,
  0x542a20 poked at the end, since a rider not driven never sets it): 520273 bytes, 10000 samples, total
  41376.0 (the port's autopilot makes the same lap: 41376), same layout, plays in the port.
- The picture: the port's frame with the ghost at the capture's place: the label equal to the pixel; the
  bike and rider equal in the frame before. The difference at that very moment is the far model: see below.
- A ghost over the line while the idle player still rides (the port's file with its clocks x 0.8): in both, the
  best-lap panel gets a new first row, 00'33"10 COMPUTER over 00'43"33 DSI, POS 2/2, then GAME OVER at the time limit.
- Reset in the original: the file goes. Countdown: both bikes on the same spot, the ghost on top, the same in both.
- In the port: a faster ghost's file stays, a slower one's is replaced, a time over saves nothing, Continue
  rides the new ghost at once; a ghost in the game's folder is read, Reset hides it; reverse and Pocket
  names as above; a race and its replay with a ghost are the same to the bit.

**Seen on the way, not changed**: `race_frame`'s `detail_of` picks the far model from the shown position,
one step ahead of the game's +0x638 (0x45f160 from +0x3c, in the step): any rider goes to the far model a
step early at 1000 units. `g_phys[k].lod` holds the game's value. Not ghost-specific, the pictures would change.

### 4.22 The joysticks (mapped and written 2026-10-26)

The block **0x51ae88** (`this` of every routine below), WinMM's joystick API [asm + decomp]:
- +0 count (joyGetNumDevs, at most 8); +4 + 0x34·j the JOYINFOEX of joystick j (dwSize
  0x34, dwFlags 0x483 = X, Y, buttons, RETURNCENTERED...; X at +0xc, Y +0x10, buttons
  +0x24); +0x1a4 the dead zone; +0x1a8 + 0x194·j the JOYCAPSA (wXmin +0x24, wXmax +0x28,
  wYmin +0x2c, wYmax +0x30, wNumButtons +0x3c).
- per joystick at +0xe48 + 0x38·j: +0 present (answered, or JOYERR_UNPLUGGED 0xa7), +4
  working (answered), +8 "digital" (X and Y only ever 0, 0x8000 or 0xffff; written by
  0x418ba0 for joystick 0 only and read by nothing), +0xc..+0x14 the centres (0x8000),
  +0x18..+0x20 the dead zones, +0x24 X, +0x28 Y (rescaled), +0x30 buttons, +0x34 the
  direction bits (1 up, 2 down, 4 left, 8 right).
- **0x4189c0** detection: counts, caps, then each joystick read once; centre 0x8000, dead
  zone **0x4dbf7c** (the command line's `-jN`, atoi, 0x414cb0) or 0x800. Called at the
  start (0x4189b0), by 0x418b60 (known? else detect again), 0x4311c0 (the Options'
  controller), page 13 every frame, Ctrl+J.
- **0x418ba0** reads joystick 0 or 1 (any other: 0). Error 0xb (JOYERR_NOCANDO?) logs.
- **0x418da0** = read, copy X, Y, buttons, **0x418e40 / 0x418f30** rescale X / Y, then
  **0x418d20** the direction bits. Rescale: inside centre ± dead → centre; above:
  `c + ftol((v − c − dz) / (max − c − dz) · (max − c))`; below:
  `min + ftol((v − min) / (c − min − dz) · (c − min))` (x87, divisors as unsigned 64-bit
  loads). Direction bits test the rescaled value against centre ± dead again (so they
  fire only about twice the dead zone out).
- **0x418c60** for page 12: the lowest button held → n + 1; else from the **raw**
  position X < 0x2000 → −1, X > 0xdfff → −2, Y < 0x2000 → −3, Y ≥ 0xe000 → −4; else 0.
  The page subtracts 1: buttons n, directions −2 left, −3 right, −4 up, −5 down (the
  binding codes, §4.14).
- **0x411b30** (from 0x4116f0, the race's input, for each rider whose +0x31e1 is 2 or 4;
  joystick +0x31e5): **0x431190(code)** = the action whose button binding (0x53eae4 + 8a)
  is code. Up (−4): accelerate → bit 1; brake → bit 2 and **Y = 0xffff − Y**; gear up/down
  → 0x10/0x20 when not counting down (0x4dc0b4) and running (0x5429b0); turbo → +0x31fd =
  1 when running. Down (−5): accelerate → bit 1, Y flipped; brake → 2; gears only need
  running; turbo. Left (−2): left → 4; right → 8, **X flipped**. Right (−3): left → 4, X
  flipped; right → 8. Buttons (while bits remain, below wNumButtons): accelerate → Y =
  centre, bit 1; brake / left / right (running) → that axis centred, the bit; gears (not
  counting, running); turbo (running); after each button, brake clears accelerate.
  Then: joystick (0x4dc11c = 2): accelerate on a button → Y < 0x8000 cut to 0x8000;
  brake on a button → Y > 0x8000 cut; joypad (4): X = Y = 0x8000 (the bits alone).
  +0x31fd changed: 1 → +0x31f9 (turbo held) = +0x3201 (pressed) = 1; 0 → +0x31f9 = 0.
- 0x4116f0 stores bits +0x31ed, Y +0x31f1, X +0x31f5; when 0x5429b0 != 1 X = 0x8000 and
  only bit 1 is kept. The step or's +0x31e9 (the keyboard's) with +0x31ed.
- **0x411a10** (race held by a dialog, §4.17 part 5): joystick 0's direction bits and
  button 0 posted as WM_KEYDOWN/UP of the arrows and Return on each change.
- Ctrl+J (0x431922, Ctrl held = 0x4dc108): device = JoyType 0x4dc120; 2 or 4: detect,
  none working → 1. Device != JoyType → message T98 "JOYSTICK NOT CONNECTED", else T32
  "JOYSTICK" / T31 "JOYPAD" (0x4188b0 slot 0x4e98d8, 3000 ms, align 0x22, y 0x14, fade
  out, font 12). Ctrl+K: device 1, T30 "KEYBOARD" (slot 0x4e98dc).

Port (`joy.c`): SDL joysticks in place of WinMM; axis 0 and 1 + 0x8000 as X, Y (0..0xffff),
a hat (a pad's cross) pushes them to the ends as the joypads of 1997 did with their axes
(WinMM gave a cross as the POV, which the game never reads: the port's choice), buttons
as bits (32 at most). No real joystick is read under `--shot`; `OMR_INPUT`'s
`joy:X,Y,BUTTONS` holds a fake joystick 0 for the tests, `OMR_DEVICE` sets 0x4dc11c,
`-jN` / `OMR_JOYDEAD` the dead zone.

**What the original has (Danik asked, 2026-10-31).** Analog steering and an analog throttle, but only with the controller
set to JOYSTICK (0x4dc11c = 2) and only WinMM's X and Y of one stick; JOYPAD (4) throws both away. No triggers: it asks for
0x483 = X, Y, buttons, and nothing else of the JOYINFOEX. Its brake isn't analog either: 0x465da0 divides a negative Y by 2000
and the engine routine's `hold = t < 0` (`engine_step`): past 2000 from the centre it is the brake, whole. The Y's half way holds the engine to half its
revs (target = t / scale x the top revs), so that half a stick is a low speed in first gear (8.93 m/s, 32 km/h, at
Speed Bay): the gearbox never changes up.

**A gamepad, the port's own (2026-10-31).** Danik's report: the pad works, Start does not pause, the triggers can't be
bound. Everything below is outside the game; the keyboard's and a plain joystick's ways are untouched (replays, physics and
computer riders as before, the pictures the same).
- `joy.c`: a joystick SDL has a layout for (SDL_IsGameController) is read through SDL_GameController. Buttons numbered A 0, B 1,
  X 2, Y 3, LB 4, RB 5, Back 6, Start 7, Guide 8, L3 9, R3 10 (as such pads come raw, so saved bindings keep meaning), LT 11,
  RT 12: the triggers count as buttons in CONFIG.BPT's binding numbers (the original would see a button it has not). The
  gamepads are opened first: on Danik's machine a keyboard's "System Control" is also a joystick. A larger dead zone (0x1800).
  SDL_JOYDEVICEADDED/REMOVED detect again.
- **Analog** (`g_analog`, the port's openmotoracer.ini, on by default; on page 12). On: the stick's X and Y as the game's
  JOYSTICK path, for a gamepad under JOYPAD too; the cross held gives the keys' bits, not full lock at once. Off: the bits
  alone, under either name. **A trigger's amount goes in the key bits 8..15 (brake) and 16..23 (accelerate)**, which the replay's
  word keeps (`keys & 0xffffff`) and nobody else reads: not in Y, whose half way is the trap above. 0 is the game's own press.
  `engine_step`: the acceleration x gas / 255 (free revs held to gas / 255 of the top), the brake's force x brake / 255. Half a
  trigger at Speed Bay: 33.6 m/s at step 640 against 50.4 at full; a tenth: 18 m/s. An original reading such a replay sees full presses.
- **Rumble** (`g_rumble`, the same; on page 12): `rumble` in race.c, from the player's events at each step, as race_sound.c's:
  a crash 500 ms whole; ev 0x2a a hard hit 280 ms; ev 0x15 a bump 180 ms by speed (in the air too: 0x1 is also set then); the
  fallen rider coming down or meeting a wall 160 ms (`thud`: 0x40 alone is rarely set, since `tumble_dir[1]` is under 0.05 after a fall);
  a landing by `kick` from 12. Only for the player's own riding, with a joystick chosen: not in replays, demos, the winner's lap.
  SDL_JoystickRumble (SDL 2.0.9).
- **pad.c**, menus and race's dialogs: the cross = arrow keys with repeat, the left stick the pointer (square law, 800 px/s whole),
  A a click when the pointer is on something, else Return; B and Back Escape; Start Return in a menu, Escape in a race (the pause,
  and back). In a demo A and B give way too. The race's own 0x411a10 route stays for a joystick that isn't a gamepad.
- Page 12 with a gamepad: the cross and stick now move things, so a press can't be a binding at once as it was: A chooses the
  row ("Press..."), the next thing pressed goes to it (a trigger takes a firm press), Start cancels. Back, Start, Guide can't be
  bound. The cross moves up and down the rows and from Accept to the two options. On a plain joystick the page is as it was.
- RT accelerates and LT brakes without being set (`joy_spare_trigger`), by degrees with Analog on, firm presses with it off, beside what the rows say, as long as that action is on no trigger and that trigger is on no row; the row then says "A + RT". A button held for the same action is a full press. Why: Danik's report "everything works except the analog triggers": he had never assigned them, the defaults are A and B, and his saved settings were kept, so a new default binding would not have reached him. Pad and SDL were cleared first: `tools/padtest.sh` on his pad gives LT, RT smooth from 0 to 32767.
- Real route tested with a virtual pad (uinput, evdev; the same axes and ranges as his, Xbox 360's ids; `SDL_GAMECONTROLLER_IGNORE_DEVICES` hides the real one; a real window on Xvfb). It is what the pretend pad (`OMR_INPUT` pad:) can't reach: pad_pos, SDL's layout, the evdev ranges. Its script is not kept: it needs access to uinput and makes a new input device on the PC while it runs, so it is not one of the checks.
- Tests: `OMR_INPUT` `F:pad:X,Y,BUTTONS,LT,RT,CROSS`; pictures pad_settings, pad_pause, pad_ride; the soak's extra_pad*.
- `--shot` reads no real controller, now truly: `joy_detect` used to switch SDL's joysticks on itself when it found them off, so test runs saw whatever was plugged in (here an "ASRock LED Controller" as joystick 0). The ARM build's check found it: under qemu there is no device, pad_settings opened on the Keyboard page. The pretend pad is now there from the first picture when `OMR_INPUT` holds a `pad:`. No other picture depended on it.
  Not tested by me on the real pad: SDL listed it ("Generic X-Box pad", a layout known, 6 axes), nothing more; the rumble has never been felt. Danik's feel is the check.

### 4.23 The MiSTer's ARM (2026-10-30)
`tools/arm.sh` builds a static ARM program in the hybrid toolchain's container; `tools/arm.sh check` runs every test
picture and replay on it under qemu-arm. Result: all 59 pictures the same as the PC's in 5-6-5, the replays the same,
and the three race logs the same as the PC's byte for byte, so the physics and the riders are bit-exact on ARM (the
floats through VFP, `-ffp-contract=off`, `-fsigned-char`). SDL's conversion to a BMP rounds differently on ARM (at most
1 in a channel over the whole picture), which is why the check compares 5-6-5 and not the files' sums.
**Speed:** qemu tells nothing about time. `OMR_PERF=1` prints every 5 s the pictures a second, the time to make one
and the slowest, the screen's wait left out. PC, Speed Bay, computer riding, 8000 pictures: 0.69 ms a picture, slowest
4.40 ms. Callgrind over 1500 pictures (with the load): the painters take most of it, span6 34%, row 12%, span 11%,
horizon_draw 9%, row6 6%, faces 4%, triangle 4%, rows6 4%. **Not measured on a MiSTer yet**: nothing has been made faster,
and the skill's Step 8 starts from that measurement. The command, on the MiSTer's shell, with the program and the game copied there:
`SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy OMR_PERF=1 OMR_HOME=/tmp/omr OMR_PAGE=1 OMR_CHOICES=1,0,4,0,0 OMR_AUTOPILOT=1 ./openmotoracer -data "Moto Racer" --shot /tmp/x.bmp 6000`.


### 4.24 The Windows build and the crash soak (2026-10-31)
**Windows.** `tools/windows.sh` builds `build/win/openmotoracer.exe` with MinGW-w64 (x86-64, this machine's toolchain links
the UCRT: Windows 10 has it, older Windows needs Microsoft's redistributable), SDL2.dll beside it, SDL's own package for MinGW
(2.32.10) from ReNFS's download or from GitHub. A window program (`-mwindows`, SDL2main), no terminal. Not a release: no
packaging. The differences: `saves.c` home in `%APPDATA%\OpenMotoRacer`; `files.c` the program's folder through
SDL_GetBasePath, turned from UTF-8 to the ANSI code page that fopen and opendir take (a folder named outside the
machine's code page cannot be opened, the message says where it looked); `page_options.c` page 13's CPU from CPUID and its speed
from the TSC over 50 ms, in place of /proc/cpuinfo; `main.c` `--help` in a message window. `video_error` (`video.c`) is what a player must
see: stderr, and a message window when there is a window. On every system the game's folder, if not told, is `Moto Racer` here,
else beside the program, else the program's own folder. `long` being 32 bits matters nowhere (lengths of files only);
no libc `rand` or `qsort` in the game's logic.
`tools/windows.sh check` under wine, its own prefix, no window or sound, against the native build. Result (wine 11.20):
**all 59 pictures 0 pixels different in 5-6-5**; the sums all differ, because SDL2.dll's RGB565 to 24-bit in the BMP rounds
differently from the system's SDL (by at most 1 in a channel, as on ARM), so 5-6-5 is the comparison (`tools/shotdiff.py`). The three replays the same as themselves, and their race logs the same as
the native ones line for line (3246, 5121 and 3871 lines) so the physics and the riders are bit-exact on Windows' libm; the physics and riders' check reports (`physcheck`, `cpucheck` compiled
for Windows) the same as the native reports. A race's sound recorded (OMR_WAV) the same byte for byte. By hand under wine on a private Xvfb: every SDL render driver
draws the window, keys work, a race rides, the "no data folder" message window shows, a folder named in Greek under a Greek
code page works, `C:\...` paths for -data and -home work. **Not run: real Windows, a real joystick, real sound output.**

**The soak.** `python3 tools/soak.py` builds with `-fsanitize=address,undefined,float-cast-overflow` in `build/asan` and runs, no window,
each in its own home folder, sound recorded to a file so the mixer runs: all ten tracks (both lists, new switch `OMR_LIST`) in the four shapes (plain, reverse,
Pocket, both) ridden by the computer and won (`OMR_AUTOPILOT=win`) to the winner's lap, results and replay; practice and championship on each track; riding with random keys; every page
0..26 in the three modes plain and with random keys and clicks; the languages; 40 monkeys from the logo; the demos in a row; the CD's files the same way; the dialogs' other answers, a record typed in, the race's Ctrl keys and a pretend joystick (`extra_*`, added after
a gcov build showed they had not run). `-fwrapv` is the port's on purpose, so signed overflow is not checked. Uninitialised values: clang's MemorySanitizer with a do-nothing SDL (`--msan`, `tools/sdlstub.c`: the real library's
memory can't be followed) over the pages, the riding, the monkeys and the won races: 335 runs, 2.09 million pictures asked for, nothing.
**Result on the fixed code: 510 runs + 72 `extra_*`, 3.77 million pictures asked for (a run that quits ends early): no sanitizer complaint, none ended otherwise than 0.** A gcov build over the same runs: 92% of the lines. What has not run: binding a joystick on page 12
(`joy_bind`, `joy_code`), the pause menu's row 111 (`act_quit_replay`, shown only with `g_race_kind` 1 in a championship, not in playback), the sound column's engine row and its row on to the display column (`race_sound_toggle_engine`, `act_side_display`), and what only a window has (`vk_of`, the fullscreen key, the real event loop): `--shot` never opens one.
What it found, each fixed at the game's address with every check unchanged (pictures, replays, physics, riders):
1. **Writes past and before the screen, in ordinary races** (`r3d.c`; Speed Bay, Rock Forest and Red City had them). 0x4a04d0 stores a see-through piece's end in 16 bits (`mov [edx+0xa],si`): an edge far off to the
   left, -54887, comes back as an end at 10649. The game's painter then runs on over the following rows and off its surface; the port's went past `g_pixels` over the globals behind it (`g_screen` among them). And a reversed row
   splits a free piece so that it starts left of the row (`row6`, `flat_row`, 0x4a0260, 0x4a2f10), which paints the row above, and on row 0 what lies before the screen. The rows on the screen are painted as before; `span6`, `span`, `flat_row` stop at the screen's ends.
   This is the game's own behaviour bounded, not altered.
2. **0x46d270's unordered compare** (`bikephys.c` `angles`). A matrix's m[6] a little past 1 makes the sqrt a NaN; `fcom` + `test ah,0x41` falls into the "not above 1e-5" branch (the straight-up case), the port's `c <= 1e-5f` did not, and went on with NaN angles: index 0 of the
   cosine table, a computer rider's heading jumping (Fun Fair reversed). Now `!(c > 1e-5f)`. A divergence from the original that no recording had met.
3. **0x4b3048** (`ftol.h`). The C library's float to int is `fistp` into 64 bits, low half kept; past 2^63 and NaN, 0. `r3d.c` and `race_sound.c` took 0x80000000 for everything out of int's range, `triangle` and `land.c`'s corners a plain C cast, which is undefined (x86 and ARM differ). Those now share `ftol.h`.
   `hud.c` and `horizon.c` keep their own copies for the moment (being rewritten for the screen sizes); nothing out of range has come through them.
4. **Page 21, button 9** (`page_champ.c`). Continue's neighbour on an arrow key is 9 (0x1ffff09, as 0x450f40 has it), which doesn't exist; Return then presses it and `pressed < 10` holds. The game saves to Champ8.sav, which nothing lists, and copies the title past its eight (0x451323) over the variables at 0x540ad0. The file is written still, the title not.
5. Not a bug but a false alarm: a paletted face of kind 8 (Pocket bikes) said "not written yet". The table 0x4af100 sends it to the plain return: it draws nothing.

Noted, left as they are: pressing **Play Link** ends the program with "The page 2 (network) is not written yet" (scope: no network; Danik to say if it should do nothing instead). Pages 2, 7, 8, 19, 22 and such end the same way, reached only through `OMR_PAGE`. `OMR_PODIUM` above 4 overruns; the game clamps its place to 4 and only the switch can.
**The rest of the kinds written, and what checking them found (2026-10-31).** `r3d.c` now has every row of 0x4f1a70 and 0x4af100: paletted 3, 4, 9 and unpaletted 2, 3, 4, 8, 9, 0x12, 0x13, 0x14, 0x1d (`no_paint`: those that paint nothing, which of them close their pieces as `see_through` says) and 69 (68 without blue, from the code's run-time patches: 0x624b04 = 0x7c0, 0x624af8 = 0xf800, 0x625770 = 0x1f0000). None is met in play: 0x420a50 and 0x462c30 can't make them (§4.15), and a census of 48 runs (both lists, normal, reversed, pocket, 1500 pictures on autopilot) saw paletted 0, 1, 2, 8 and plain 0, 12, 16, 21, 31, 32, 38, 39, 44, 53, 54, 62, 68. Checked by forcing them in both programs: the original with its tables poked (0x4af101, 0x4af102 = 5, 3, 4; kind 38's span 0x4f1cdc = kind 69's), the port built likewise; equal but for what moves. The sub-5 and sub-7 ones go through the ordinary triangle as a stand-in, which paints nothing either way; the free pieces they close were not compared.

Comparing those pictures found two faults of the port's in every race, neither seen by a check, because `build/shots.txt` is the port against itself:
1. **The shadows' faces were one pair for all bikes** (since b8d650c, when the kind drawn moved into the face, §4.13.4: right for models' copies). The game's are each rider's own (0x45bf70, obj+0x2ef1). The last bike queued is far away, kind 44, so the nearest bike's shadow lost its kind 62 (perspective): 358 pixels at Speed Bay's start. Now `g_pair[RIDERS]`, `g_crash[RIDERS]`.
2. **All bikes were drawn**, wherever they were: far ones showed as specks over the hills past the road's end (they have no road there to stand behind). 0x458b80 asks 0x46e8b0 and the list of 0x40ce60 (§4.1 of the bike drawer). The port had this test for the race's labels, but had the wrong distance: the file's order is LenBPlane (0x51d1bc) first, then LenRLPlane (0x51d1b8), so `detail[1]` was 3500, the side planes', not 18000. In the original at Speed Bay's start riders 1..13 have +0 set, 14..24 not (read from its memory, 0x584421 + k x 0x3209): rider 14 is 20457 ahead, 2457 past the far plane, rider 13 18912. With `detail[0]` the port draws the same. The labels (Link, time attack) had the same wrong distance.

Memory: demos in a row hold 56 MB at most from 16000 to 128000 pictures; the leak checker finds nothing over 12000 pictures.


### 4.25 The MiSTer hybrid core (2026-10-31, branch `mister`, the core `~/MiSTer-openmotoracer`)

All of it behind `MISTER_HYBRID` (`port/src/mister.c`, `mister.h`); the PC's pictures are unchanged
(`tools/shots.sh`, 91 of 91). The game runs on the ARM, the core shows it and has the sound and input.

- **Which copy**: one menu entry. `Moto Racer/` holds GOG's or the CD's, `Moto Racer CD/` the CD's; GOG's
  first when both are there, known by what is in the folder (`game.EXE`, `DISK1/data.z`). Game Behaviour stays.
  `MH_PickGame()`'s message takes `%s` once only: a second `%s` is garbage or a crash.
- **Resolutions** (Danik: no 200-line modes, the original has none; 2026-11-01: no 640x480 race): the race at
  320x240 (default) and 640x240; menus always 640x480 (480i at 15 kHz, Flicker Filter Strong by default: `FLICKER` 2), taking a frame each field
  (`MH_SetFieldRate`), races not. No 512x384 (the core has no such mode) and no 16:9. F2, F3, the F12
  column and the Options row change it; the OSD wins when it is moved.
- **640x240**: the game believes it is at 320x240 (texts, dials' digits, dialogs at low resolution), and only
  the 3D phase is drawn at 640x480 on the even rows (`g_half_rows`: `r3d.c`, `horizon.c`). The 2D is drawn into
  a 320x240 side buffer over a sample of the 3D, and the pixels it changed are merged doubled across. The
  first try, the 640x480 HUD with rows dropped, had unreadable small fonts.
- **Time**: `g_now` from the core's field counter (59.64 Hz), 1000 + fields x 100000/5964.
- **Input**: the core's joystick 1 becomes the game's pretend gamepad (`joy_fake_pad`). The stick steers by
  degrees, `MH_StickDpad` drops the d-pad bits the stick causes. In menus Menu OK and Menu Back are the
  MiSTer menu's (hybrid/CLAUDE.md), with no mapping the bottom button is OK, the right one Back. A
  first press where nothing is selected does what the game's Return does: nothing without a Return
  button on the page, an arrow selects first.
- **A pad alone**: name entry page 15 takes A as Space (picks the letter under the cursor,
  `page_name_picks`), the record's name on TOP 5: up and down turn the last letter, right adds one, left takes one
  away, OK ends (`page_top5_letter`). A demo's dialog does not hold the race (`dialog_holds()` is 0), so the pad
  counts it as a menu through `dialog_open()`: before that a pad could not leave a demo.
- **The settings page's buttons**: in menus the core's buttons reach the game only as OK, Back and Select, so page 12 waiting at "Press..." heard nothing else (found on the real core, 2026-11-01). While `g_pad_listen` is set `mister_pad` gives them as in a race, and Y as PAD_Y; a Y that an action uses no longer changes the camera. Data on a USB drive (`TEST_USB`), settings written there at the end, Flicker Filter usable at 480i, the `_Test` entry through the launcher: all tried on the real core.
- **HUD margins**: `g_hud_top` moves the top texts, `g_hud_bottom` the dials and bottom texts up, and the main menu's Options and Exit (Danik, overscan cut them); the signs and the
  place's 3D figure go down through `r3d_screen_down()`.
- **The clock's 60ths** (`g_ticks`) were never worked out on the core (`ticks` unset on that path of `clock_frame`, 0 in practice from the first build to 2026-11-01): no bike change on page 10, no drifting dialog rows, tiles or blinking. `--shot` and the replays take the other path, so no check could see it; only playing on the core did.
- **Cheats on the OSD**: the game's three follow `g_island_unlocked`, `g_reverse_unlocked2`,
  `g_pocket_unlocked2` only when moved, typing them still works; No Time Limit; Slow Rivals, latched at the
  race's start, not in demos.

**Speed, measured on the real MiSTer** (Speed Bay, 24 riders, autopilot, the settings of a new player; the time to make a picture without the wait; stretches of 300 pictures, 2026-11-01):

| Mode | Pictures a second | ms a picture | Slowest | Of 300: never shown / shown twice |
|---|---|---|---|---|
| 320x240 | 59.5-59.8 | 10-14 | 16-23 | 0/0 in 6 stretches of 7, 1/1 in the other |
| 640x240 | 52.8-60 | 14-19 | 21-27 | 0-10 / 0-29 |

Before this work: 320x240 57-60, 640x240 33-45. (A 640x480 race, measured then at 28.5-30, was dropped afterwards.) So 320x240 was a steady 60 (the field rate, 59.64) and 640x240 near it, dropping in the heaviest stretches. A picture may take up to about two fields as long as the average stays under 16.7 ms, which is what drawing ahead buys. Those are one CPU's numbers; the two CPUs' are below.

**Both CPUs draw the race (2026-11-02): 640x240 a steady 60.** Measured on the real core with the new player's settings (detail 4, the highest), 24 riders, autopilot, a race on each of the ten tracks (both lists), 48 s each; the first 300 pictures, the race's load, apart:

| Mode | Pictures a second | ms a picture, stretches of 5 s | Slowest | Of the pictures after the first 300: never shown / shown twice |
|---|---|---|---|---|
| 640x240, every track | 59.6-59.9 | 9.2-14.7 | 42.6 once, else at most 22.7 | 1 / 1 of 21000 |
| 320x240, Speed Bay | 59.6-59.7 | 7.0-9.8 | 14.0 | 1 / 1 of 2700 |
| 640x240 on one CPU (`OMR_ONE_CPU=1`), Speed Bay | 55.5-59.9 | 12.5-17.6 | 25.5 | 29 / 64 of 2700 |

The first 300 pictures have 1 or 2 of each at every track: the load's end. A race of 9600 pictures (autopilot, 160 s) stayed the same, and the on-board view (F6) too, on the heaviest track 11-13 ms. The one slow picture of the table, 42.6 ms, came once in those ten races and not again on that track's other runs: taken for something else on the MiSTer, not looked into further.

- **How.** A profile showed the drawing at two thirds of a picture, and the s-buffer's rows do not depend on each other: a row's pixels come from its free pieces alone. So `r3d_render` (`R3D_TWO`, on with `MISTER_HYBRID`) has a second thread on CPU1 walk the same faces and draw every other row (at 640x240, of its even ones, rows 0 mod 4 and 2 mod 4). The see-through pieces each puts off are its own (`Drawer`). It sleeps between pictures on a futex. `triangle` and `flat_triangle` step over the other's rows at once; `rows6` keeps stepping each row, its floats running on as the game has them.
- **CPU1 looks full and is not**: `/proc/stat` shows no idle time there, but that is Main_MiSTer's polling at nice 0, which gives way to our nice -20. The present thread's copy comes first in a picture, during the steps and the scene, so it should hardly meet the drawing (reasoned from the code's order, not measured).
- **Cache lines.** A row's count sits beside its first free pieces (`FreeRow`, aligned): `g_nfree` as an array of ints would have had the two threads writing one line. Each row's list gets a line that no other row's uses.
- **What only one thread may do**: a paletted page's texels are made in `queue_kind` now, not on first drawing in `face`, where two threads would both have made them. `face` only reads them.
- **Floats through doubles**: `(float)((double)a + b)` is `a + b` in float, always (double rounding does no harm when 53 >= 2 x 24 + 2), so `rows6` and `span6` use the float sum without the two conversions.
- **The sound.** With the second drawer the audio ring dipped lower: in a race at least 1344-1376 frames left against the old 1408, and in the first 10 s of sound as low as 96, twice empty (-192, -224) in 10 races, against 256-1024 with one CPU. Two fixes. The mixer's thread is `SCHED_FIFO` from its first callback (`sound.c`; it takes little and sleeps in between): in a race at least 1472 (800 once, at the 42.6 ms picture above). What was left, at the race's start, was no CPU's doing: `sound_race_load` held the sound's lock through reading its files, 30 ms in which the mixer waited (found with a probe on the lock, which also shows why the old builds dipped there). Only `new_slot` and `set_volume` hold it now, the files read with the mixer running: first 10 s at least 608, mostly 1024, none empty in any run since.
- **The picture is the same.** `tools/shots.sh` 91 of 91 with one thread and with two (`-DOMR_R3D_TWO=ON` on the PC, `OMR_R3D_TWO=1`), `tools/arm.sh check` 91 and 3 replays (`SDLPREFIX` and `HYBRID` this core's: with its default ECWolf prefix the build found no `SDL.h`). 240 pictures one thread against two, the 16-bit pixels, all ten tracks plain, reversed and Pocket, both resolutions, four moments each: all the same, in a host build of the core (`./build.sh host`, F3 in `OMR_INPUT` for 640x240; `--shot` there saves the merged 640x240 picture now). ThreadSanitizer and AddressSanitizer + UBSan over 60 races each, two threads, both resolutions: nothing. On the real core its own screenshots (`echo screenshot > /dev/MiSTer_cmd`) of a race, on board, the pause dialog after F3 and page 10's bike: right.
- **What can differ, and has not been seen**: each thread counts its own pieces and batches, so a frame that ran out of them (10240, 1000) drops fewer see-through faces; and a painter running off its row (the game's own fault, under `span6`) can meet the other thread. Both only ever in such a fault's frame.
- `DRAW_AHEAD` measured again, as the framework's rule asks: 55 s on the heaviest track (list 0, track 4), two runs each in turns, after the first 300 pictures: 640x240 0/0 and 0/0 ahead, 418/418 and 406/406 of 2700 without; 320x240 0/0 and 0/0 ahead, 8/8 and 10/10 of 2400 without. It stays 1.
- **How to measure it.** The OSD's Resolution from the test's settings: `/media/fat/config/OpenMotoRacer_TEST.CFG`, 16 bytes, the fourth 1 for 640x240; `TEST_ENV="OMR_PERF=1 MISTER_HYBRID_PACE=1 MISTER_HYBRID_STATS=1 OMR_PAGE=1 OMR_LIST=0 OMR_CHOICES=1,4,4,0,0 OMR_AUTOPILOT=1" tools/test_on_mister.sh MotoRacer`, `tools/test_on_mister.sh log`. Limit to 30 FPS stays on the OSD as Danik decided it.

What the profile showed and what was done (`-DOMR_PROF=ON`: 1 kHz sampling and marks per stage, `tools/prof.py`; slower than the release, so compare stages only between profile runs and take the real numbers from a release):
- **The sound's lock.** The game thread waited up to 8-20 ms for the device lock, held by the mixer through a whole callback (1024 slots a sample, with 64-bit division). `sound.c` has its own lock, taken per block of 256 samples (`MIX_BLOCK`), and `mix_voice()` runs voice by voice into float arrays. The WAV's sum is unchanged.
- **The music's file reads** on the game thread: now `music_thread` on CPU1 (`MH_PinThreadToCPU1`). Without a thread (`OMR_WAV`) it fills directly as before.
- **Drawing straight into the core's next buffer** (`SDL_MISTER_DIRECT`) in place of a copy a picture.
- **640x240's 2D rows only as needed.** The sample of the 3D and the merge took every row; now `VIDEO_ROWS(y0,y1)` says which rows a writer touches (`font.c`, `bitmap.c`, `sprite.c`, `dialog.c`, `pointer.c`), only those are taken and merged. `OMR_ROWS_CHECK` in the profile build compares and prints any row drawn on without saying; 0 through help, map, dialogs, F7, pause and the winner's credits.
- **The horizon's clear**: only its strip's rows and the last one (`horizon.c`), not the whole screen a picture.
- **The drawing loops split by kind**: `span` and `span6` chose the kind per pixel through a jump table the compiler did not lift out of the loop. `TEXELS` writes a loop for each kind. `row6`'s common cases return early; a prefetch of the next face in `r3d_render`; `span`'s kind 0x10 in assembler on the ARM.
- **The build**: Thumb's smaller code stays in the Cortex-A9's cache and gave the best pacing, with `-O3 -flto -flto-partition=none` (`port/CMakeLists.txt`, also in `tools/arm.sh`'s build). Measured against ARM mode with the same sources, in turns.

What did not help: prefetching textures (no gain, undone); profile-guided optimisation about 2%, not worth keeping counts files in the repository; whole-program optimisation alone little. An earlier remark here, that splitting the loops gained nothing, was wrong: that first try split only two kinds.

Checked after all of it: `tools/shots.sh` 91 of 91, `tools/arm.sh check` 91 pictures and 3 replays, all the same; on the real core the audio ring never empty (`MISTER_HYBRID_STATS`, at least 1408 frames in it).

**Drawing ahead** (`DRAW_AHEAD`, hybrid/CLAUDE.md's rule), pictures of 300 shown never / twice, two runs each in turns:

| Mode | Ahead | Without (`MISTER_HYBRID_AHEAD=0`) |
|---|---|---|
| 320x240 | 2/5, 0/0, 4/6, 13/22, 0/0, 11/24 | 39/39, 51/51, 35/36, 29/42, 54/53, 25/39 |
| 640x240 | 0/131, 0/120, 0/190, 0/152, 0/117 | 0/108, 0/132, 3/156, 1/142 |

So `DRAW_AHEAD` 1: a clear gain at 320x240, nothing lost at 640x240. Not in an interlaced race (the framework's rule). (Those two tables' runs are from before the speed work above.)

### 4.26 The ESP32-S3 (2026-11-08/09, branch `esp32`, the project `esp32/`)

The board: **Waveshare ESP32-S3-Touch-LCD-1.83** (ESP32-S3R8: 2 LX7 cores at 240 MHz, 8 MB octal
PSRAM at 80 MHz, 16 MB flash). From Waveshare's schematic and BSP 2.0.0, all checked on the board:
ST7789P 240x284 on SPI2 (MOSI 7, SCLK 6, CS 5, DC 4, RST 38, backlight 40), run at **80 MHz** (66
full frames a second measured); I2C 14/15 at 400 kHz with the CST816D touch (0x15, INT 13, RST 39),
QMI8658 (0x6b), ES8311 (0x18), AXP2101 (0x34); I2S MCLK 16, BCLK 9, WS 45, DOUT 8, amplifier 46;
SD in SDMMC **1 bit** (CLK 2, CMD 1, D0 3, D3 42 held high), 40 MHz; BOOT on GPIO0 (low pressed),
PWR through the AXP2101 seen on **GPIO41, high while pressed** (measured). The AXP2101's register
0x27 already had the 10 s power-off press (0x1c). The first start after flashing sometimes finds
the card not ready: mounted with retries.

**Tilt**: the accelerometer's **X** is along the LCD's long side (held sideways, USB on top: left
end down -0.6 g, right +0.7 g, Y unmoved, measured 2026-11-09); steering = X minus X at the race's
start, full lock at 0.45 g, a 0.03 g dead zone, to `joy_fake_pad`'s stick (as the MiSTer's pad).

**The build** (`esp32/main/CMakeLists.txt`): `port/src/*.c` as they are (not `mister.c`, `prof.c`)
with `OMR_ESP32`, an SDL subset on FreeRTOS (`esp32/main/sdl/SDL.h`, `sdl_esp32.c`: threads,
locks, `esp_timer`, the sound device, the picture handed to `esp32_present`, events from the
board), `board.c`, `omr_esp32.c`. `main.c`'s `main` is `omr_main`. The game's static arrays go to
PSRAM by a linker fragment (`linker.lf`: `* (extram_bss)`), the 3D's painters to IRAM.
`SPIRAM_MALLOC_ALWAYSINTERNAL` 512: at 16384 the game's small allocations filled the internal RAM
and file opens failed.

**Fitting 8 MB** (each step checked on the PC: 91 pictures and 3 race logs unchanged unless said):
- Banks keep only their table; a file is read out of the bank when asked for (all builds). The
  track (1.3 MB) is read through a 32 KB window (`dsi_fopen`, all builds).
- The program image keeps only what its file holds, without the code and relocations, which
  nothing reads (all builds: 0.3 MB in place of 3.1).
- Menu pictures (244, 20 MB as 16-bit): only their sizes at the start, their pixels when drawn,
  kept 8-bit with their palette, the least lately drawn given up past 1.5 MB, all at a race's
  load (`bitmap.c`); other pictures 8-bit too. `bitmap_loaded()` where pages tested `pix`.
- Fonts read when first used; the ghost's 1 MB only in a time attack; one horizon (320 mode).
- Textures (`r3d.c`, `Tex`): a bike's liveries are its 8-bit page through its palettes (no
  128 KB copy each); a track's pages are made 8-bit with 256 colours each (exact up to 256,
  else a median cut, colour 0 kept exactly 0). Next to the PC's, practically the same.
- A face keeps its corners once (floats; the affine u = `ftolf(u x 8192.0f)`, the same integer),
  its narrow fields narrow (all builds; 104 to 52 bytes).
- Sounds: the game's own **8-bit set** (`data\sfx\8_Bits`, its option for slow PCs), kept
  8-bit; the mixer at 22050 Hz, music `music/trackNN.wav` 22 kHz mono from `tools/esp32_card.py`.
A race then holds 4.5-5.2 MB of the ~5.3 MB of PSRAM heap.

**Speed** (Speed Bay, 24 riders, autopilot), in order:
| Step | Pictures a second |
|---|---|
| first race, doubles in software, one core | 5.7 |
| **physics in single precision** (`OMR_FAST_PHYS`: `R` a float, tgmath, `-fsingle-precision-constant`) | 7.5 |
| what is seen and heard in single precision (`fastfloat.h`) | 9 |
| both cores draw, every other row (`R3D_TWO` on FreeRTOS) | 16.5 |
| the race drawn at 284x216, the LCD's size; sent by DMA from the game's screen (the ST7789 little-endian: RAMCTRL 0xb0, 0xf8) | 22.5 |
| **the pipeline** (`R3D_PIPE`): core 1 draws a picture from a copy of the lists while core 0 steps and builds the next on another of three pictures | 24.8 |
The physics in single precision is **not the original's x87 to the bit** any more on the board (the
user's choice, 2026-11-09): a PC's replay or ghost may part from it. The PC is unchanged. A race
steps at most 4 times a picture there (it was falling behind for ever at 6 a second).
The pipeline shows a picture behind the game (~35 ms); a picture's HUD is the newer state.
Tried, no better, left out: the rows in two bands with the line moving to the busier core (Jet's;
the road's triangles cross both), `-O3`, scaling on core 1 (it competed for PSRAM). 120 MHz PSRAM
needs the flash at 120 MHz.
Now: core 1 draws ~33 ms a picture (the limit), core 0 steps ~17 ms and builds ~14 ms a picture.

**Sound**: 96% of a race's sound was below 200 Hz, which the small speaker cannot play (the engines
seemed silent at low revs): a 200 Hz high-pass and x1.6 before the codec, no clipping (measured
from the board's own output, the console's `W`).

**Touch**: menu buttons and the race's dialog rows take the nearest within 90 of 640 pixels
(`menu_hit`, `row_at`); the press is where the finger lifts. A tap skips the logo and starts from
the title, and ends a demo.

**Tools**: `tools/esp32_card.py` (the card from the CD's .cue/.bin), `tools/esp32_log.py` (the
log), `tools/esp32_shot.py` (the console: `S` a picture, `n e u d l r 1..9` keys, `a b t` held
controls, `cXXXYYY` a touch, `X settings` a restart with test settings such as `OMR_PAGE=1
OMR_CHOICES=1,0,4,0,0 OMR_AUTOPILOT=1`, `W` 3 s of sound, `I` the accelerometer). The readout every
5 s: pictures, step parts, 3D, memory, the input, what is playing.

## 5. Running the original (for checking)
`tools/orig.sh start|shot|key|stop`, unseen, nobody's screen needed (2026-10-26):
- A private copy in `build/orig` (with GOG's dgVoodoo `DDraw.dll`, `D3DImm.dll` and
  `dgVoodoo.conf`, which sets DesktopBitDepth=16: the game demands a 16-bit screen) and a
  private wine prefix `build/wineprefix`, run with `WINEDLLOVERRIDES=ddraw,d3dimm=n,b`.
- The hidden display is a headless **Weston with Xwayland** (`weston --backend=headless
  --xwayland --socket=mrwl`, 1024x768; its X display number read from
  `build/logs/weston.log`, its pid in `build/logs/weston.pid`). Weston is a window manager,
  so `xdotool windowactivate` can give the game's window focus: **the game stops moving
  whenever its window lacks focus**, and that is the whole reason the plain Xvfb stayed black.
- `start` waits for the window "Moto Racer - © 1997" (640x480 at -windowed) and focuses it;
  `shot FILE` takes that window (`import`); `key KEYSYM...` focuses it and types with
  xdotool (real key events: `key --window` would send synthetic ones); `stop` ends wine and
  Weston. `ORIG_DISPLAY=:1` uses Danik's own screen instead (he agreed, 2026-10-26).
- Checked: Escape skips the Delphine logo (page 25) to the title (page 14, which shows
  "FULL D3D VERSION" even with -NoD3D), Return goes to the name entry (page 15). Left alone,
  the logo leads to the attract-mode demo race (West Way). Reference pictures in `build/ref/`.

`tools/win/memtool.c` (MinGW, run under wine) reads and writes the running game's memory
(`/proc/<pid>/mem` is closed by the kernel's ptrace rule; wine's ReadProcessMemory works).
`tools/origshot.sh` (the back surface from memory, 0x4eea14) and `tools/patch_orig.py` (the
16-bit test 0x45a7c9 jumped over) are left from the attempts below; neither is needed now.

Dead ends on the hidden display (2026-10-26; never resurrect), every one black:
- Xvfb at 24 bits, windowed: "16 bit screen mode required !".
- Xvfb at 16 bits, windowed or full screen, wine's ddraw (OpenGL or GDI renderer).
- The depth test patched out, full screen on 24 bits: "Cannot initialise screen mode"
  (Xvfb cannot change mode); the same inside `explorer /desktop`.
- dgVoodoo on Xvfb; reading the back surface (0x4eea14) out of memory: 0 or black.
