# Handover — where OpenMotoRacer stands
Read this first in a new session, then CLAUDE.md, then the NOTES sections for the area.

## Done (2026-10-26, session 1)
- Decisions agreed with Danik (CLAUDE.md "Decisions").
- `~/openmotoracer`: git, `.gitignore` (game folders, `CD/`, `build/` kept out), the toolkit
  (the LE-only parts left out), and for this Windows program: `tools/pe2flat.py`,
  `tools/rebuild.sh` (Ghidra's PE loader, seeds, the MSVC split-function repair),
  `tools/imprefs.py` (who calls which Windows function), `tools/vtables.py`.
- The CD (`CD/*.BIN/CUE`, Danik's): `tools/cdbin.py` → `build/cd/data.iso` + audio tracks;
  files extracted to `Moto Racer CD/`; `tools/is3.py` unpacks the installer's `data.z`
  (the CD's own program `MOTO.EXE`, 1997-07-21, a different build from GOG's). NOTES §1, §3.5.
- Formats read, each tool checked on every file: BKF banks (`tools/bkf.py`), RAW pictures
  (`tools/raw.py`), LEZ pictures (`tools/lez.py`, unpacker 0x43eb00), BPT scripts
  (`tools/bpt.py`, the cipher 0x45a4a0). 3DF/MAP models read from the loader (NOTES §4.5),
  no tool yet.
- Map: the MFC app, `OnIdle` = the frame, the menu's page table, the race object, the
  variable-step clock (NOTES §4). The picture is 16-bit RGB565.
- The original runs unseen (`tools/orig.sh`, headless Weston + Xwayland, its window given
  focus with xdotool, keys reach it): logo, title and name entry pictured (NOTES §5).

- The port's skeleton (`port/`, CLAUDE.md "The port"): C/SDL2, reads the player's
  game.EXE for the game's tables, the banks, the LEZ pictures and the game's own drawing
  routine; its first picture, the settled title page, equals the original's to the pixel
  (5-6-5) apart from the Start button (NOTES §4.7).

- The menu core (NOTES §4.4, mapped from the machine code: pages, buttons, keys, mouse,
  slides, the glass transition's timing) and the main menu (page 3) in the port: equal
  to the original's main menu apart from the pointer; arrows, Tab, Return, Escape and the
  mouse work as in the game. The text system mapped (NOTES §4.8, `tools/fnt.py`).

- The text system in the port (fonts, text objects): "FULL D3D VERSION" equal to the
  original's pixel for pixel, blinking at its 500 ms (NOTES §4.8.3.7).
- A progress tracker for Danik: https://claude.ai/artifact/CCupuhZmsCRgz7FA5eFMxz (its
  numbers are a JSON file uploaded to it; update it after each step).

- The 3D renderer and pages 25 (logo) and 14 (title) with the breaking-glass transition
  between them (NOTES §4.9): the logo within ~1100 px of the original, the glass matching
  its captures, the settled title equal to the original. The port now starts on 25.

- Sound (NOTES §4.10, mapped by a helper into build/notes/sound.md, then into NOTES):
  `tools/sfx.py` decodes SFX.BKF; `port/src/sound.c` plays the effects, the voice queues
  and the music as GOG does; a recorded run equals 13.wav sample for sample.
- Page 15, the name entry, in the port (NOTES §4.11): equal to the original's picture
  (through model.py) apart from the pointer; the chain logo → title → name → main menu runs.
- The Solo pages mapped (NOTES §4.12, from build/notes/solo_pages.md; captures
  `build/ref/orig_p*`); pages 5 (SELECT MODE) and 6 (SELECT RACE / CHAMPIONSHIP) in the
  port with the race settings and track lists (`choices.c`): six screens equal to the
  original's captures but for the pointer. The track format is mapped part-way
  (`build/notes/track.md`).
- The speed (Danik, 2026-10-26: "runs a bit too fast"): the clock is the real time in a
  window, per-picture counts go by 60ths (NOTES §4.2). `tools/shots.sh` sums the test
  pictures (`tools/shots.sh | diff build/shots.txt -` after every change);
  `tools/cmpshot.py` compares a picture with a capture and finds the tile's scroll.
- Pages 10 CHOOSE BIKE (2D; the 3D bike not yet), 9 TOP 5 (bestscor.dat read) and 1
  loading in the port (NOTES §4.12.12): TOP 5 single race and championship, and the loading
  screen, equal to the original's captures but the pointer; page 10 equal outside the bike.
  The whole Solo chain runs logo → … → Start → loading, then back to the main menu (no
  race yet). Correction in NOTES §4.12.10: the loading logo and badges are drawn by the
  load's steps (0x4470d0).
- Pages 20/21, championship Load/Save: equal to the original's captures; the original
  loads a save the port wrote. Saves go to ~/.config/openmotoracer for now (saves.c),
  read from there or else from the game's data/.
- The race's track in the port (NOTES §4.13, the track notes folded in there): the .trk,
  the level's pages, the section walk, what a camera sees, the race's perspective-correct
  spans and see-through kinds, the horizon. `OMR_RACEVIEW` draws a track from a still
  camera: the same to the pixel as tools/track.py on eight captures of the original;
  only the bikes, riders, shadows and HUD differ from the original's screens.
- Pages 11 Options, 12 controls, 13 diagnostic, 23 data disk (NOTES §4.14): equal to the
  original's captures but for the pointer (and the CD volume, which wine never offered).
  The settings (CONFIG.BPT) are saved to the home folder when the program ends; sound
  switches and the three volumes work. No joystick yet. Page 13 shows the port's machine.
- The 3D bike on page 10 (NOTES §4.15, mapped by a helper with tools/bikeparts.py as the
  reference): skeleton, pose, wheels, the liveries' palettes, the drawer's menu part.
  Equal to the original's captures (dirt: 8 edge pixels; road: all). `OMR_TURN` holds the
  turntable for such checks.
- The race starts: page 1 loads it (track, bikes, 24 riders on the grid as 0x47bf70
  places them). The rules, laps, places, countdown, time limit, the dashboard and dials,
  the pause menu, the end of the race (results, continue, bestscor.dat) are written.
- The player's bike rides (bikephys.c, bit-exact on the original's recorded steps) under
  the chase camera (camera.c). The computer riders stand on their grid places.
- The tracker shows Claude's messages at its bottom ("From Claude", the `messages` dataset)
  and Danik writes from it (comments sent to Claude): keep it watched.

## Done (2026-10-26, session 2: the race made whole)
- The bike's drawing leans, pitches and steers; the chase camera turns with it; F7 the
  dashboard's three states and the gear hint; smooth pictures between the 62.5 Hz steps
  (F10; Danik asked for "unlocked framerate, correct speed").
- TOP 5: a new record's name typed and saved.
- The computer riders drive (`cpu.c`, NOTES §4.21, bit-exact on three recordings of the
  original: `tools/cpucheck.sh`, the recorder `tools/win/cpuwatch.c`).
- The race's sounds (`race_sound.c`, NOTES §4.10.6): engines, skids, crowd, voices.
- GAME OVER (a sprite object, `sprite.c`, NOTES §4.20 §3.5); Pause.
- Joysticks (`joy.c`, NOTES §4.22): detection, dead zone, page 12 binding, page 13, the
  race's bits through the bindings, the dialogs' arrows, Ctrl+J / Ctrl+K.
- The on-board view (F6) with the cockpit (NOTES §4.20 mode 6): looks the same as the
  original's capture (`build/ref_onboard_orig.png`, Speed Bay; retake it as below). The fix that made the cockpit textured also gave the
  player's rear wheel its tyre texture (it was black near the camera).

### Driving the original to the on-board view (Speed Bay, Practice)
Keep its sound off Danik's speakers: `pactl load-module module-null-sink sink_name=mrnull`
and `export PULSE_SINK=mrnull` before `tools/orig.sh start`. Keys with `tools/orig.sh key`:
Escape (retry until the title shows, not the logo), Return (name), Return (main menu),
Left + Return (Solo), Left Left + Return (Practice), Return (track page), Return (bike page:
Start), then F6 once the race runs. `tools/orig.sh shot FILE`; afterwards
`tools/orig.sh stop` and `pactl unload-module module-null-sink`.

## Done (2026-10-30, session 3: replays, the TV, the winner's lap, then the rest of the race, the CD and ARM)
- Replays (`replay.c`, NOTES §4.21b): YES on the results watches the race again under the TV's
  cameras, to the bit (`tools/replaycheck.sh`). The demos start after 30 s on the title.
- The TV director (`camera.c`), the record split line on the dashboard (`rules.c`).
- The winner's lap with the credits (`generic.c`, the .GEN scripts): compared with a capture of
  the original (`build/ref/winner_*_orig.png`; how to force it in the original: NOTES §4.21b).
- The championship's end: pages 16, 17, 18 and the podium (`page_podium.c`, NOTES §4.12.14): the
  stand the same as the original's capture to the pixel. The player's 4th place not captured.
- The riders' bodies (`pose.c`, NOTES §4.18 8.16a): every pose by speed, lean, jump, stunt, rival,
  place, the salute and the fall, with the hand skeleton. Same as the original's captures by eye.
- The roadside objects (`signs.c`, `land.c`, NOTES §4.13.5): the turn signs (0 pixels different inside
  the diamond in all six) and the place's figure, compared at Speed Bay; boats and gulls move. Not
  checked: later signs of a ride, AnimDecor types 1, 4, 8, 7, other tracks' moving objects, the signs
  on a 16:9 view; four figure captures differ, probably older pictures than the memory read.
- The time attack's ghost (`ghost.c`, NOTES §4.21c): the movement exact on a recording of the original,
  files pass both ways. Not tested: a person beating a ghost on the original (its file came from a
  computer-ridden lap with pokes), Pocket and reverse files in the original, a crash inside a recording.
- The race's small things: the dialogs' mouse and the pointer, F1, F11/F12 with Ctrl+S/V/A, the CD's sprite
  messages, texts keeping apart, the music's pause. Compared with the original: F1's list and key caps, the
  columns, the pause menu's box, two messages together. Not compared: the CD's messages (the port only; CHECK
  POINT never reached), the pointer. Not written: F12 L/M/H and B (fixed 640x480), Shift+F12, the music's pause
  on losing focus, Ctrl+U and the numpad volumes, the column staying open across races.
- Shadows, lighting, effects (`shadow.c`, `effects.c`, NOTES §4.15.9-10): the lit bike and its shadow and the dark
  bike 0 pixels different from the original's captures. Smoke, marks and spray compared by eye and through the
  original's memory (two rides never run in step; compare smoke only with `-FrameRateMax60`, which `tools/orig.sh`
  doesn't pass). Not compared: computer riders' marks, the crash's debris, spray at detail 2, a real dark section.
  The ghost gets its shadow and effects as 0x45e940 calls them (seen in the port, not compared).
- The CD (`exe.c`, NOTES §3.5a): the port runs on the CD's files alone, every track started. MOTO.EXE itself runs
  under wine (how: NOTES): title and main menu differ in the pointer only; Solo can't be pressed there, so no race
  or later page compared. It is GOG's logic with plain colours on the CD's data. No music from the CD yet.
- The ARM build (`tools/arm.sh`, NOTES §4.23): pictures and physics the same as the PC's under qemu; `OMR_PERF=1`. Speed on the
  real core: NOTES §4.25 (320x240 a steady 60, 640x240 53-60, after the profile's work).
- The glass after a race (the old question of its page 2 against the level's pages 2..19): a race, back through the menus,
  Options: the glass whole, no track picture in it (seen in the port).

## Waiting for Danik
- The music under Pause: the port holds it, as the game's code asks; GOG's copy lets it run on (its winmm.dll
  ignores the request, recorded under wine). A one-line change in `sound.c` (`music_pause` empty) if he prefers GOG's.
- The CD's MOTO.EXE: GOG's logic on the CD's data for now (NOTES §3.5a). Enough, or should the CD's own look be
  ported too? And the CD's music: read the audio tracks from the image, or have the player rip them to `NN.wav`?
- Music: loop for ever (GOG, now) or the CD's "play a repeating track twice"?
- Where saves go (bestscor.dat, championship saves, ghosts): the game's data folder, as
  the original, or a user folder? (For now a user folder, the game's read as a fallback.)

## Done (2026-10-31, session 4: the PC version finished, all but the split screen)
- The race at 320x240, 512x384 and 16:9 (F2, F3, F4, the F12 column, Shift+F12, Options): picture, column, pause box,
  help and messages compared with captures at each. `tools/win/keypost.c`: keys from inside wine where the hidden display
  won't give them.
- **The kind drawn lives in the model's face** (NOTES §4.13.4): the palm at Speed Bay now 0 pixels different. Found by
  reading the original's tables of see-through pieces (a way to check any later drawing question).
- Every track's start of the first list against the original, the locked ones with 0x4dc058 poked, compared again on
  2026-10-31 after two faults of the port were found (the near bike's shadow, far bikes drawn; NOTES §4.24): at Speed Bay
  outside the big LAPS text 95 pixels differ, the boat, a gull and 7 single ones. On the other road tracks riders' idle
  movement and a line or so of rounding: West Way's roof (the 24 pixels), Lost Ruins' rock, under 30-80 pixels a track.
  Dirt bikes rock in both, the helicopter flies; there what differs is those. The second list (Fun Fair and so on) was
  compared on 2026-10-31, below. **`build/shots.txt` is the port against itself: it cannot see a fault like these two; only
  `build/ref/` and fresh captures can. After a change of the race's picture, compare against one.**
- F9 the map, numpad detail and volumes, Ctrl+F, Ctrl+U, Shift+F1, focus, `-windowed`, `-fullscreen`, `-NoSound`, Alt+Return,
  Link stays on the menu (NOTES §4.20 §2.6, §5b, §4.10.5.3). A demo has 23 riders as the original.
- The Windows build (`tools/windows.sh`, checks under wine; never run on real Windows) and the soak (`tools/soak.py`,
  NOTES §4.24: 510 + 72 runs clean, 4 real bugs fixed, among them writes past the screen in ordinary races).
- The 4th place on the podium compared. Demos are live races, not recordings: they can't be compared picture for picture.

Danik then found West Way on easy never said GO and kept GET READY blinking: the port tested the time left where the game's
variable holds the lap panel's clock by then (NOTES, "s here is not the time left"). Fixed, compared with the original on that
track and difficulty; `race_go`, `race_after_go` in the pictures. The tests had never run on easy: `OMR_DIFFICULTY` now.

The port now saves the track list as the game does (it never had, so unlocks were lost on quitting); Danik asked for every
track unlocked on both discs to test them: `tools/unlock_all.sh` did it in his folder, `--undo` goes back to the game's lists.

## Next, in order
1. The MiSTer core is built and tested on the real MiSTer (`~/MiSTer-openmotoracer`, this branch `mister`, NOTES §4.25); `~/GAMEDATA/OpenMotoRacer.tar.zst` holds the data. Waiting: Danik's playing on a CRT (rumble, triggers, flicker), his OLED verdict, his word on publishing. Nothing pushed to GitHub.
2. Merging `mister` into `master`: not done; the PC's pictures are unchanged by it.
3. **Last: the split screen** for two players on one PC (the original has none). Danik, 2026-10-30: leave it for last and
   **stop before starting work on it**.
Not compared with the original, for want of a way to run both in step: computer riders' marks, the crash's debris, spray at
detail 2, a real dark section, later signs of a ride, the CD's pages past the main menu. Not run: real Windows, a real
joystick, a real full screen. Left in the code: T and Ctrl+H don't show their messages (they change nothing here).
The helpers' worktrees in `.claude/worktrees/` are merged and empty of anything unique (one holds a 690 MB private wine
prefix; `git worktree remove -f -f` once the session has ended, its `dosdevices` links first).

## 2026-10-31: the rare ways of drawing written; two real faults found by comparing them
Every way of drawing a face in the game's tables is in `r3d.c` (no model reaches 3, 4, 9 and the like; checked by forcing
them in both programs, NOTES §4.24). That comparison showed the near bike's shadow wrong since b8d650c and all 24 bikes
drawn, whatever their distance. Both fixed; every check passes; `build/shots.txt` remade (9 + 15 race pictures changed).

## 2026-10-31: the gamepad
Danik's real pad works; he asked for Start to pause, the triggers to be bindable and analog, rumble, and the cross and stick in
the menus. All done (NOTES §4.22), on by default, tried with a pretend pad (`OMR_INPUT` pad:) and a virtual copy of his pad; RT and LT work unset; Danik judges the real one. The
original has analog steering and throttle only on one stick under JOYSTICK, a brake that is on or off, no triggers.

## 2026-10-31: the second list's starts; Game Behaviour
- All 8 tracks of LIST01, each four ways, two shots a run, the original under wine against the port (build/notes/list2_starts.md, build/agent_list2/):
  four real faults, every track, now written. The road painter adds with the carry (r3d.c span: "rounding"); the sections' start planes rebuilt, not the file's
  (bikephys_track: the walk, so the places and what the computer riders aim at); a race's first step of 1 ms (race.c: what the original's clock says, as a rule);
  the camera starts above and drops in (camera.c). What is left, 4705 pixels over 66 pictures, mostly the helicopter, gulls and flags at another moment between two steps (proved right by moving their time a fraction, NOTES §3.5c), and 1-7 pixels under the shadow and single ones not explained.
  Not in the port: the red "640X480 / VIDEO ..." strip of a race's first 0.3 s (Danik's to decide; the figures are dgVoodoo's).
- Game Behaviour (NOTES §3.5c): Options, 1997 CD / GOG RELEASE, with any files. Bike browsing's slide counts 60ths; a click on + or - is one step (the port's, menu.c).
- The player's bike under the setting (g_phys_cd), bit for bit on MOTO.EXE's recorded steps (`tools/physcheck_cd.sh`). A GOG fault found on the way and fixed: steering's 1.6 under brake only while sliding (0x46661a).
- Open: whether the 1997 handling feels right after jumps and in slides (MOTO has no frame cap, its feel depended on the PC), Danik's by feel; the red mode strip of a race's first 0.3 s; the mixer (recommended leaving). Split screen last, not started.

## 2026-11-02: 640x240 a steady 60
Both CPUs draw the race, every other row each (`R3D_TWO`, `r3d.c`; the same pixels), and the sound no longer waits on anything: the mixer `SCHED_FIFO`, the race's sound files read outside its lock. On the real core all ten tracks hold 59.6 at 640x240, 1 picture never shown and 1 shown twice of 21000 after the race's load; `DRAW_AHEAD` measured again, stays 1. NOTES §4.25. `hybrid/` went to its main in the same step, which `package.sh` needed for the covers. The build is in the MiSTer's *Test* menu; not released, not published. Danik's to judge: the feel of a real race with a pad, and the OSD and input while both CPUs draw (only autopilot was measured).

## 2026-11-09: the ESP32-S3 (branch `esp32`)
The game runs on the Waveshare ESP32-S3-Touch-LCD-1.83: menus, races at ~25 pictures a second, music
and sounds, tilt steering, touch and buttons (NOTES §4.26). The user tried it: steering, picture and
engine sound fine after their fixes. Open: more speed (core 1's drawing is the limit: detail,
interlacing were offered), the ULP (optional, not started), the split screen still last.
