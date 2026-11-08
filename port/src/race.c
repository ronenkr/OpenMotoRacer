/* The race (NOTES §4.16): its load (0x47b420), the riders on the starting grid (0x410a50,
   0x47bf70), and its frame (Game_Frame 0x47d660). Written so far: the track, its sky, the
   riders on the grid, the player's bike (bikephys.c) and the chase camera (camera.c),
   the dashboard, the race's rules, the pause menu (Escape), the computer riders
   (cpu.c). */
#include "prof.h"
#include "race.h"
#include "bitmap.h"
#include "esp32.h"
#include "pointer.h"
#include "bikephys.h"
#include "bikes.h"
#include "camera.h"
#include "choices.h"
#include "clock.h"
#include "config.h"
#include "mister.h"
#include "cpu.h"
#include "dialog.h"
#include "effects.h"
#include "exe.h"
#include "files.h"
#include "generic.h"
#include "ghost.h"
#include "horizon.h"
#include "font.h"
#include "hud.h"
#include "input.h"
#include "joy.h"
#include "land.h"
#include "menu.h"
#include "pages.h"
#include "pose.h"
#include "r3d.h"
#include "race_sound.h"
#include "replay.h"
#include "rules.h"
#include "scores.h"
#include "shadow.h"
#include "signs.h"
#include "sound.h"
#include "text.h"
#include "racekeys.h"
#include "trackmap.h"
#include "video.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FOCAL ((float)(g_scr_w * 394 / 640))    /* 0x6d20c4: 0x4592a0's, 394 at 640x480 (build/trackcap) */

Rider g_rider[RIDERS];
int g_racing;
static int g_player = 1;        /* 0x4ea4c0: the local player's record */
static int g_follow = 1;        /* 0x508834: the rider the camera and the dashboard follow */
int g_race_kind;                /* 0x4e9834: 0 a race, 1 its replay, 2 a demo */
int g_playback;                 /* 0x4e9830: the demos run */
static int g_hotkeys = 1;       /* 0x4e8944: F5, F6, F7 answer (not in a demo) */
static unsigned g_seed;         /* 0x4f0c48: the race's random start, its replay's too */
static int g_demo_clock;        /* 0x4e9814 */
static int g_demo_len;          /* 0x542995: a demo's time, ms */
unsigned g_keys[RIDERS];        /* +0x31e9: the control bits (RK_*) */
static BikePhys g_phys[RIDERS]; /* the riders' bikes */
static unsigned g_events;       /* 0x4e8918 */
static int g_finish_timer, g_timeover_timer;    /* 0x4e9824, 0x4e9828 */
int g_winner_lap;               /* 0x504ea0: the computer rides the winner's bike on */
static int g_winner_sequence;   /* 0x504eac: its credits have begun */
static float g_winner_top;      /* 0x504ea4 */
static int g_winner_counter;    /* 0x4dbe38: ShowCounter, off meanwhile */
static int g_paused;            /* 0x4dc038: Pause, App_OnIdle leaves the race alone */
static int g_first_step;
static unsigned g_joy_keys;     /* +0x31ed: the bits from the joystick */
static int g_joy_y = 0x8000, g_joy_x = 0x8000;  /* +0x31f1, +0x31f5 */
static int g_joy_turbo;         /* +0x31fd: a turbo binding held */
int g_ctrl_held;                /* 0x4dc108: Ctrl is down */
int g_shift_held;               /* 0x4dc10c: Shift is down */
static int g_onboard;           /* 0x4dc00c: F6, the view from the rider's eyes */

/* Ctrl+J (0x431922): the joystick or joypad chosen in the options, if one answers, else
   the keyboard; Ctrl+K: the keyboard */
static void controller(int joystick)
{
    if (!joystick) {
        g_input_device = 1;
        hud_notice(1, exe_text(30));
        return;
    }
    g_input_device = g_joy_type;
    if (g_joy_type == 2 || g_joy_type == 4) {
        joy_detect();
        if (!g_joy[0].working)
            g_input_device = 1;
    }
    if (g_input_device != g_joy_type)
        hud_notice(0, exe_text(98));
    else if (g_input_device == 2 || g_input_device == 4)
        hud_notice(0, exe_text(g_input_device == 2 ? 32 : 31));
}

/* 0x431250 (WM_KEYDOWN, the repeats of a held key too) and 0x432700 (WM_KEYUP): the keys
   bound to the seven actions (0x53eae0) set and clear the player's bits; pressing one
   way clears the other, as the original's handlers do */
static int g_new_screen;        /* 0x4e9ce4 */

/* 0x4592a0 with the game's three sizes (0x4dc0d0 -1: not Windows' list of modes): the picture
   W x H, or W x W 9/16 in 16:9 with the rest of the surface black; 0x499ba0's view, the focal
   from the size (the full height's even in 16:9); the messages go, the dialogs' places again */
static void screen_set(void)
{
    static const int w[3] = { 320, 512, 640 }, h[3] = { 240, 384, 480 };
    int m = g_race_screen;
#ifdef MISTER_HYBRID
    video_set_half(g_mister_res == MR_640x240);
#endif
#ifdef OMR_ESP32
    m = g_race_screen = 0;      /* the board's LCD: 320x240 only (284x216 shown) */
#endif
    g_new_screen = 0;
    g_screen_mode = m;
    video_set_size(w[m], h[m], g_mode169 ? w[m] / 16 * 9 : h[m]);
    r3d_screen();
    text_clear();
    hud_screen();
    dialog_screen();
}

#ifdef MISTER_HYBRID
/* the OSD's Resolution, F2, F3, Options and the F12 column: to the game always 320x240 */
void race_set_resolution(int r)
{
    g_mister_res = r;
    g_race_screen = 0;
    if (g_racing)
        g_new_screen = 1;
}
#endif

void race_ask_screen(int mode)   /* 0x43a2d0, 0x43a300, 0x43a320: L, M, H */
{
#ifdef MISTER_HYBRID
    race_set_resolution(mode);
    return;
#endif
    g_race_screen = mode;
    g_new_screen = 1;
}

void race_ask_169(void)         /* 0x43a370: B */
{
#ifdef MISTER_HYBRID
    return;
#endif
    g_mode169 ^= 1;
    g_new_screen = 1;
}

int race_is_169(void)
{
    return g_mode169;
}

void race_key(int vk, int down)
{
    unsigned *b = &g_keys[g_player];
    int a;
    if (!g_racing)
        return;
    if (dialog_key(vk, down))   /* (every dialog in turn, before the race) */
        return;
    if (vk == VK_CONTROL) {     /* 0x4dc108 */
        g_ctrl_held = down;
        return;
    }
    if (vk == VK_SHIFT) {       /* 0x4dc10c */
        g_shift_held = down;
        return;
    }
    /* the control keys first, whatever else the key does, and in a pause too */
    for (a = 0; a < ACTIONS; a++)
        if (g_binding[a][0] == vk)
            bikephys_key(&g_phys[g_player], b, a, down, g_control);
    /* Pause, Ctrl+P (0x4314f7): the race stands. (The pause menu leaves the music going.) */
    if ((vk == 0x13 || (vk == 'P' && g_ctrl_held)) && down && g_solo) {
        g_paused ^= 1;
        music_pause(g_paused);                  /* 0x4096e0 */
        sound_pause_all(g_paused);              /* 0x409700 -> 0x424090 */
        return;
    }
    if (vk == 0x78) {                           /* F9: the track map (0x4e8944 is set in a race) */
        if (down && g_hotkeys)
            trackmap_key();
        return;
    }
    if (vk == 0x6a || vk == 0x6f) {             /* * and /: the volumes */
        if (down)
            racekeys_volume(vk == 0x6a, g_shift_held, g_ctrl_held);
        return;
    }
    if (vk == VK_ADD || vk == VK_SUBTRACT) {    /* + and -: the detail */
        if (down && !g_shift_held) {
            if (vk == VK_ADD && g_detail < 4)
                g_detail++;
            else if (vk == VK_SUBTRACT && g_detail > 0)
                g_detail--;
        }
        return;
    }
    /* (the 1997 CD's 0x441690 asks a little differently of Ctrl+F, Ctrl+U and F12) */
    if (g_ctrl_held && vk == 'F' && !(behaviour_cd() && g_shift_held)) {        /* the small fonts */
        if (down)
            racekeys_small_fonts();
        return;
    }
    if (g_ctrl_held && vk == 'U') {             /* the speed in mph or km/h (0x4f0dd0) */
        if (down && (!g_shift_held || behaviour_cd()))
            g_speed_unit = !g_speed_unit;
        return;
    }
    if (vk == 0x70 && g_shift_held && !g_ctrl_held) {   /* Shift+F1: the screen's size */
        if (down)
            racekeys_info();
        return;
    }
    if (g_paused)
        return;
    if (g_ctrl_held && down && (vk == 'J' || vk == 'K')) {
        controller(vk == 'J');
        return;
    }
    if (g_ctrl_held && down && (vk == 'S' || vk == 'V' || vk == 'A')) {
        /* 0x431250 0x53, 0x56, 0x41: the switches of the F11 column, with their messages */
        if (vk == 'S')
            race_sound_toggle_effects();
        else if (vk == 'V')
            race_sound_toggle_voices();
        else
            race_sound_toggle_music();
        return;
    }
    if (vk == VK_ESCAPE && down) {
        if (g_winner_lap) {     /* 0x431533: on to the results */
            g_winner_sequence = 1;
            credits_stop();
            g_finish_timer = 1;
            return;
        }
        dialog_pause(1);
        return;
    }
    if (vk >= 0x71 && vk <= 0x73) {             /* F2, F3 the screen's size, F4 16:9 (0x431250) */
        if (down) {
#ifdef MISTER_HYBRID
            if (vk != 0x73)
                race_set_resolution((g_mister_res + (vk == 0x71 ? MR_COUNT - 1 : 1)) % MR_COUNT);
            return;
#endif
            if (vk == 0x73)
                g_mode169 ^= 1;
            else
                g_race_screen = (g_race_screen + (vk == 0x71 ? 2 : 1)) % 3;
            g_new_screen = 1;
        }
        return;
    }
    if (vk == 0x70) {                           /* F1 (0x431cf2): the help, in Solo */
        if (down && !g_ctrl_held && !g_shift_held && g_solo && !g_playback)
            dialog_help();
        return;
    }
    if (vk == 0x7a || vk == 0x7b) {             /* F11 and F12 (0x432200): the columns */
        if (down && vk == 0x7b && behaviour_cd())
            dialog_side(1);
        else if (down && !g_ctrl_held && g_shift_held && vk == 0x7b)
            dialog_modes();
        else if (down && !g_ctrl_held && !g_shift_held)
            dialog_side(vk == 0x7a ? 3 : 1);
        return;
    }
    if (vk == VK_TAB) {                         /* the look back */
        camera_key(vk, down);
        return;
    }
    if (vk == 0x74) {                           /* F5: the distance; Shift: another rider */
        if (down && g_hotkeys) {
            int st = g_racer[g_player].state;
            if (!g_shift_held)
                camera_key(vk, down);
            else if (st == RS_FINISHED || st == RS_OUT) {
                do
                    if (++g_follow >= g_riders)
                        g_follow = 1;
                while (!g_racer[g_follow].loaded);
                g_bikeworld.followed = g_follow;
            }
        }
        return;
    }
    if (vk == 0x79) {                           /* F10 (the port's): smooth pictures */
        if (down)
            g_smooth = !g_smooth;
        return;
    }
    if (vk == 0x75) {                           /* F6: on board (0x4e8944 is set in a race) */
        if (down && g_hotkeys && g_race_kind != 1)
            g_onboard ^= 1;
        return;
    }
    if (vk == 0x76) {                           /* F7: the dials */
        if (down && g_hotkeys)
            hud_key(vk);
        return;
    }
    if (vk == 0x77) {                           /* F8: the turn signs */
        if (down && g_hotkeys)
            signs_key();
        return;
    }
}

#define STEP_MS 16
static unsigned g_step_at;      /* the clock's time the steps have reached */
static unsigned g_last_now;     /* the clock at the last picture */
/* smooth pictures (the port's, g_smooth): the riders as the step before the last left
   them, and what is drawn, set between the two by the time since; the game's state is
   never touched by it */
static Rider g_before[RIDERS], g_shown[RIDERS];

static void race_end(int ms);
static void winner_give_back(void);
static int g_test_win;          /* OMR_AUTOPILOT=win */

/* the drawing's copy of a bike: the frame's matrix +0x2e55, the place, the wheels, the
   section, the lean, pitch, steering, suspension and spins, the crash */
static void to_rider(Rider *r, const BikePhys *b)
{
    memcpy(r->m, b->frame, sizeof r->m);
    memcpy(r->t, b->frame + 9, sizeof r->t);
    memcpy(r->pos, b->pos, sizeof r->pos);
    memcpy(r->rear, b->rear, sizeof r->rear);
    memcpy(r->front, b->front, sizeof r->front);
    r->sec = b->sec;
    r->lean = b->lean, r->pitch = b->pitch, r->steer = b->steer, r->speed = b->speed;
    r->wheelie = b->wheelie;
    r->dive = b->dive, r->susp = b->susp;
    r->spin_rear = b->spin_rear, r->spin_front = b->spin_front;
    r->crashed = b->crashed, r->roll_side = b->roll_side;
    r->pose = b->pose, r->hand = b->detail, r->lod = b->lod;
    memcpy(r->shadow, b->shadow, sizeof r->shadow);
    memcpy(r->crash_shadow, b->crash_shadow, sizeof r->crash_shadow);
    if (b->crashed) {
        memcpy(r->body, b->tumble, sizeof r->body);
        memcpy(r->body_pt, b->tumble_pt, sizeof r->body_pt);
    }
}

/* what the drawer leaves for a crash to start from (+0x3085, +0x30b5) */
static void from_rider(BikePhys *b, const Rider *r)
{
    if (b->crashed)
        return;
    memcpy(b->tumble, r->body, sizeof r->body);
    memset(b->tumble + 9, 0, 3 * sizeof(float));
    memcpy(b->tumble_pt, r->body_pt, sizeof r->body_pt);
}

/* the on-board view draws the cockpit in place of the player's bike (0x458b80), unless
   Tab looks back or the bikes are the Pocket ones (0x462110) */
static int onboard_now(void)
{
    return g_onboard && !g_phys[g_follow].crashed;
}

static int cockpit_shown(void)
{
    return onboard_now() && !camera_preset() && !g_pocket;
}

/* The port's: the gamepad shakes as the player's bike is knocked (the same events as the
   sounds, race_sound.c). A shake in progress isn't cut short by a weaker one. */
static void rumble(BikePhys *b)
{
    static int was_crashed;
    static unsigned until;
    static int level;
    int low = 0, high = 0, ms = 0, on = g_rumble && g_race_kind == 0 && !g_playback
                                         && !g_winner_lap
                                         && (g_input_device == 2 || g_input_device == 4);
    unsigned ev = (unsigned)b->events;
    float k = b->speed / 40.0f;
    k = k < 0.4f ? 0.4f : k > 1.0f ? 1.0f : k;
    if (b->crashed && !was_crashed)
        low = 0xffff, high = 0xffff, ms = 500;                  /* the crash */
    else if (ev & 0x2a)
        low = 0xd000, high = 0xb000, ms = 280;                  /* a hard hit */
    else if (ev & 0x15)
        low = (int)(0xc000 * k), high = (int)(0x8000 * k), ms = 180;    /* a bump, in the air too */
    else if ((ev & 0x40) || b->thud)
        low = 0xa000, high = 0x5000, ms = 160;                  /* the fallen rider comes down, or meets a wall */
    else if (ev & 0x80) {                                       /* a landing, by the fall */
        float f = fabsf(b->kick) / 80.0f;
        if (f > 0.15f) {
            f = f > 1.0f ? 1.0f : f;
            low = (int)(0xb000 * f), high = (int)(0x4000 * f), ms = 120;
        }
    }
    b->thud = 0;
    was_crashed = b->crashed;
    if (!on || !ms)
        return;
    if (g_now < until && low + high < level)
        return;
    until = g_now + (unsigned)ms;
    level = low + high;
    joy_rumble(low, high, ms);
}

/* 0x4116f0, a step's inputs: a replay's record, a demo's clock, or the player's own, which
   are recorded. 0 when the demos' time is up: this step does not happen. */
static int inputs(int ms)
{
    BikePhys *p = &g_phys[g_player];
    g_joy_keys = 0, g_joy_x = g_joy_y = 0x8000;
    if (g_race_kind == 1 || (g_playback && g_race_kind == 2)) {
        int ok;
        /* 0x4121e0, 0x411759: the demo's time up, or (a replay's end, the demos' lists
           could name .Sav files) */
        if (g_playback)
            ok = !(g_demo_len > 0 && g_demo_clock > g_demo_len);
        else
            ok = 1;
        if (ok && g_race_kind == 1) {
            int m, held, pressed;
            unsigned keys;
            ok = replay_play(&m, &keys, &held, &pressed, &g_joy_x, &g_joy_y);
            if (ok) {
                g_keys[g_player] = keys & ~0xc0u;
                p->turbo_held = held;
                if (pressed)
                    p->turbo_pressed = 1;
                g_demo_clock += m;
            }
        } else if (ok)
            g_demo_clock += ms;
        if (!ok) {
            if (g_playback) {
                g_events = EV_DEMO_NEXT | EV_DEMO_LOAD;
                return 0;
            }
            /* the recording ran out: the race goes on, the player's again */
            hud_notice(0, exe_text(116));
            g_race_kind = 0;
            g_bikeworld.replay = 0;
        }
    }
    if (g_race_kind == 0) {
        if (g_input_device == 2 || g_input_device == 4) {       /* 0x411b30 */
            int was = g_joy_turbo;
            g_joy_keys = joy_race(0, &g_joy_x, &g_joy_y, &g_joy_turbo, g_counting, g_control,
                                  g_input_device);
            if (g_control != 1)
                g_joy_x = 0x8000, g_joy_keys &= 1;
            if (g_joy_turbo != was) {
                p->turbo_held = g_joy_turbo;
                if (g_joy_turbo)
                    p->turbo_pressed = 1;
            }
        }
        replay_record(ms, g_keys[g_player] | g_joy_keys, p->turbo_held, p->turbo_pressed,
                      g_joy_x, g_joy_y);        /* 0x4577f0 */
    }
    return 1;
}

/* OMR_RACELOG: a line a step, the sum of every rider's place and speed (tools/replaycheck.sh:
   a race and its replay give the same lines) */
static FILE *g_log;
static void race_log(void)
{
    static int n;
    uint32_t h = 2166136261u;
    int k;
    if (!g_log)
        return;
    for (k = 1; k < g_riders; k++) {
        const unsigned char *p = (const unsigned char *)g_phys[k].pos;
        size_t i;
        for (i = 0; i < sizeof g_phys[k].pos; i++)
            h = (h ^ p[i]) * 16777619u;
        p = (const unsigned char *)&g_phys[k].speed;
        for (i = 0; i < sizeof g_phys[k].speed; i++)
            h = (h ^ p[i]) * 16777619u;
    }
    fprintf(g_log, "%d %d %08x\n", g_race_kind, n++, h);
    /* the check's way in */
    if (g_race_kind == 0 && getenv("OMR_WATCH_AT") && n == atoi(getenv("OMR_WATCH_AT"))) {
        g_events |= EV_WATCH | EV_RESTART;
        n = 0;
    }
}

/* 0x47df40's time-attack branch: the ghost (rider 2) as its recording says */
static float g_ghost_at[3];     /* +0x2df5: where the recording has put it */
static void ghost_rider(float dt)
{
    BikePhys *g = &g_phys[2];
    float before[3], v[3];
    int crash = 0;
    memcpy(before, g_ghost_at, sizeof before);
    ghost_play(dt, &g->speed, g_ghost_at, g->fwd, &g->turbo_req, &crash, v);
    g->engine = g->speed;
    if (crash)
        bikephys_crash(g, v);
    cpu_passive_step(g, g_ghost_at, before, dt, g_counting);
}

/* 0x4dc100, 0x4817a0's count */
static int riders_loaded(void)
{
    int k, n = 0;
    for (k = 1; k < g_riders; k++)
        n += g_racer[k].loaded != 0;
    return n;
}

/* one step of Game_Frame (0x47d660) of ms milliseconds: the inputs, the timers, the
   countdown and the time limit, each driven bike's step (0x45d540) and bike against bike
   (0x47ecd0), the race's rules, the camera */
/* OMR_SHOT_STEP: the race's step count, for a picture at a step (--shot) */
int g_race_steps;

static void step(int ms)
{
    g_race_steps++;
    BikePhys *all[RIDERS];
    int k, attack = g_solo && g_mode == MODE_PRACTICE && g_time_attack;
    g_control = 1;              /* Game_Frame: the player's keys are read */
    if (!inputs(ms))
        return;
    if (g_extended > 0)
        g_extended -= ms;
    rules_time(ms);
    if (g_test_win && !g_counting)
        g_time_left = 60000;
#ifdef MISTER_HYBRID
    if (mister_cheat(MISTER_NO_TIME_LIMIT) && !g_counting && g_race_kind != 2)
        g_time_left = 60000;
#endif
    /* 0x4dc150, the frame time smoothed by an eighth */
    g_bikeworld.frame_ms = (float)((ms - (double)g_bikeworld.frame_ms) * 0.125
                                   + g_bikeworld.frame_ms);
    g_bikeworld.countdown = g_counting;
    g_bikeworld.clock = g_step_at;
    for (k = 0; k < RIDERS; k++)
        all[k] = &g_phys[k];
    for (k = 1; k < g_riders; k++)      /* 0x47df40: dt = ms x 0.1; 0x45d540 */
        if (!g_racer[k].loaded)
            continue;
        else if (k == 2 && attack)
            ghost_rider((float)(ms * 0.1));
        else if (g_phys[k].driven)
            bikephys_step(&g_phys[k], g_keys[k] | (k == g_player ? g_joy_keys : 0),
                          k == g_player ? g_joy_x : 0x8000, k == g_player ? g_joy_y : 0x8000,
                          (float)(ms * 0.1), g_counting, g_time_left);
        else                            /* a computer rider (0x46bf80) */
            cpu_step(&g_phys[k], (float)(ms * 0.1), g_counting);
    if (attack) {                       /* 0x47e0bd: the player's run, for the next ghost */
        BikePhys *p = &g_phys[1];
        float dir[3];
        for (k = 0; k < 3; k++)
            dir[k] = (float)((double)p->m[6 + k] + p->m[9 + k]);
        ghost_record((float)(ms * 0.1), p->speed, p->pos, dir, p->turbo_req, p->crash_flag,
                     p->crash_v);
        p->crash_flag = 0;
    }
    bikephys_collide(all, g_riders, (float)(ms * 0.1));
    rumble(&g_phys[g_player]);
    effects_step((float)(ms * 0.1));    /* 0x457590 */
    for (k = 1; k < g_riders; k++)
        to_rider(&g_rider[k], &g_phys[k]);
    land_anim_step((float)(ms * 0.1));          /* 0x48b7a0 */
    rules_laps();
    for (k = 1; k < g_riders; k++)      /* 0x47e3d0 walks each bike's own +0x31c1 */
        g_phys[k].sec = g_rider[k].sec;
    rules_places();
    race_end(ms);
    camera_onboard(g_onboard && !g_phys[g_follow].crashed);         /* 0x4583e0 */
    camera_step(&g_phys[g_follow], (float)(ms * 0.1), g_counting,
                g_racer[g_follow].state == RS_FINISHED,
                g_playback || g_race_kind == 1 || g_winner_lap);
    for (k = 1; k < g_riders; k++) {
        if (camera_cut() && g_racer[k].loaded)  /* 0x47d660: the bodies again, from the new eye */
            pose_step(&g_phys[k]);
        g_rider[k].pose = g_phys[k].pose, g_rider[k].hand = g_phys[k].detail, g_rider[k].lod = g_phys[k].lod;
    }
    if (cockpit_shown())        /* 0x462110's shake, once a picture of the original */
        bike3d_cockpit_step(bikephys_frame_pitch(&g_phys[g_follow]));
    signs_step((float)(ms * 0.1), g_race_kind); /* 0x486310, 0x47b7a0: drawn once a step */
    figure_step((float)(ms * 0.1),
                g_mode != MODE_PRACTICE && g_racer[g_follow].state == RS_FINISHED
                ? g_racer[g_follow].place : -1, g_winner_sequence);
    race_sound_step(all, g_follow, camera_eye(), camera_target(), (float)(ms * 0.1));
    if (g_race_kind == 0 && !g_race_over && !g_time_over && g_mode != MODE_PRACTICE
        && g_racer[race_local()].state == RS_RACING && race_local() == g_follow)
        race_sound_commentary(rules_place_of(race_local()), riders_loaded());
    race_log();
}

/* ------------------------------------------------------------------ the demos */

#define DEMOS 16
static struct {
    char name[33];              /* a track's name (or a .Sav file's: none in the game's lists) */
    int ms;
    char picture[260];
} g_demo[DEMOS];
static int g_ndemos, g_demo_at; /* 0x53e4d0 +0x11e, +0x11a */
static const char *g_demo_pic;  /* 0x542891 */
static struct {                 /* the player's choices, while the demos use them */
    int mode, track, laps, reverse, pocket, gearbox, bike, detail;
} g_kept;

/* 0x42e370: "NbItems: n", then "Demo<i>: NAME MS PICTURE" (the picture's \\ are one) */
void race_demos_load(void)
{
    long n = 0;
    unsigned char *f = dsi_load("Data\\Demo\\EA.dem", &n);
    char *d, *p;
    g_ndemos = 0;
    if (!f)
        return;
    d = malloc((size_t)n + 1);
    if (d) {
        memcpy(d, f, (size_t)n);
        d[n] = 0;
        for (p = strtok(d, "\r\n"); p && g_ndemos < DEMOS; p = strtok(0, "\r\n")) {
            char name[64], pic[260];
            int i, ms, j = 0;
            if (sscanf(p, "Demo%*d: %63s %d %259s", name, &ms, pic) != 3)
                continue;
            snprintf(g_demo[g_ndemos].name, sizeof g_demo[0].name, "%.32s", name);
            g_demo[g_ndemos].ms = ms;
            for (i = 0; pic[i]; i++)
                if (pic[i] != '\\' || pic[i + 1] != '\\')
                    g_demo[g_ndemos].picture[j++] = pic[i];
            g_demo[g_ndemos].picture[j] = 0;
            g_ndemos++;
        }
    }
    free(d);
    free(f);
}

void race_demos_off(void)
{
    g_ndemos = 0;
}

int race_demos_exist(void)
{
    return g_ndemos > 0;
}

const char *race_demo_picture(void)
{
    return g_playback ? g_demo_pic : 0;
}

/* 0x457bf0 for a track's name: a single race of 3 laps against 23, nobody steering; 0 when
   no track has that name */
static int demo_setup(void)
{
    int i;
    for (i = 0; i < g_ntracks; i++)
        if (strncmp(g_tracks[i].name, g_demo[g_demo_at].name, 0x20) == 0)
            break;
    if (i >= g_ntracks)
        return 0;
    g_race_kind = 2;
    g_track = i;
    g_laps = 3;
    g_mode = MODE_SINGLE;
    g_reverse = g_pocket = 0;
    g_gearbox = 1;
    g_bike = 0;
    g_demo_len = g_demo[g_demo_at].ms;
    g_demo_pic = g_demo[g_demo_at].picture;
    return 1;
}

static void demos_end(void)     /* (the port's: the player's choices back) */
{
    g_playback = g_race_kind = 0;
    g_hotkeys = 1;
    g_mode = g_kept.mode, g_track = g_kept.track, g_laps = g_kept.laps;
    g_reverse = g_kept.reverse, g_pocket = g_kept.pocket;
    g_gearbox = g_kept.gearbox, g_bike = g_kept.bike, g_detail = g_kept.detail;
}

/* 0x409a30: the title page has waited 30 s */
int race_demo_start(void)
{
    g_kept.mode = g_mode, g_kept.track = g_track, g_kept.laps = g_laps;
    g_kept.reverse = g_reverse, g_kept.pocket = g_pocket;
    g_kept.gearbox = g_gearbox, g_kept.bike = g_bike, g_kept.detail = g_detail;
    g_playback = 1;
    g_hotkeys = 0;
    if (!g_ndemos || !demo_setup()) {
        demos_end();
        return 0;
    }
    g_solo = 1;
    g_detail = 4;               /* 0x472d2e */
    menu_change_page(1);
    return 1;
}

/* 0x40ec30: a section number brought into 0 .. count - 1 */
static int wrap(int s)
{
    while (s < 0)
        s += g_land.nsec;
    while (s >= g_land.nsec)
        s -= g_land.nsec;
    return s;
}

/* 0x46d610: a x b */
static void cross(float o[3], const float a[3], const float b[3])
{
    float r[3];
    r[0] = a[1] * b[2] - a[2] * b[1];
    r[1] = a[2] * b[0] - a[0] * b[2];
    r[2] = a[0] * b[1] - a[1] * b[0];
    memcpy(o, r, sizeof r);
}

static void unit(float v[3])    /* 0x40fcd0 */
{
    float l = (float)sqrt((double)v[0] * v[0] + (double)v[1] * v[1] + (double)v[2] * v[2]);
    if (l != 0.0f) {
        double k = 1.0 / l;     /* kept at the FPU's width */
        v[0] = (float)(v[0] * k), v[1] = (float)(v[1] * k), v[2] = (float)(v[2] * k);
    }
}

/* 0x47bf70, the Solo branch: from the section 3 x (riders + 1) ahead of the start, each
   rider 3 sections further back, the last record first, so the player (record 1) starts
   at the back; 10 along the section, alternately 128 right and left (not in practice,
   where they all share one section) */
static void grid(void)
{
    int dir = g_reverse ? -1 : 1;               /* 0x4dc140 */
    int s = wrap(dir * g_riders * 3), k;
    if (g_reverse)
        s = wrap(s - dir * 3);
    for (k = g_riders - 1; k >= 1; k--) {
        Rider *r = &g_rider[k];
        const Section *sc;
        float p[3];
        int i;
        if (g_mode != 0)
            s = wrap(s - dir * 3);
        sc = &g_land.sec[s];
        for (i = 0; i < 3; i++)                 /* (0, 0, 10) through +0xc, then +0 */
            p[i] = (float)(10.0 * sc->axes[2][i]) + sc->pos[i];
        if (g_mode != 0) {
            double off = k & 1 ? -128.0 : 128.0;
            for (i = 0; i < 3; i++)
                p[i] = (float)(off * sc->axes[0][i] + p[i]);
        }
        /* 0x45bdf0: the rider's state cleared (nothing the drawing reads yet) */
        if (!g_reverse) {
            memcpy(r->m, sc->axes, sizeof r->m);
            memset(r->t, 0, sizeof r->t);
        } else {                /* facing back: z = T - axes[2] (T is 0), x = up x z, y = z x x */
            static const float up[3] = { 0.0f, 1.0f, 0.0f };
            float z[3], x[3], y[3];
            for (i = 0; i < 3; i++)
                z[i] = -sc->axes[2][i];
            unit(z);
            cross(x, up, z);
            cross(y, z, x);
            unit(x);
            unit(y);
            memcpy(r->m[0], x, sizeof x);
            memcpy(r->m[1], y, sizeof y);
            memcpy(r->m[2], z, sizeof z);
            memset(r->t, 0, sizeof r->t);
        }
        bike3d_place(r, p);
        r->sec = s;                             /* +0x31c1 */
        /* 0x411010: player 1's gearbox is the menu's, the others automatic */
        bikephys_init(&g_phys[k], r->bike, (const float (*)[3])r->m, p, s,
                      k == g_player && g_race_kind != 2, k == g_player ? g_gearbox : 1);
        g_phys[k].number = k;
        pose_start(&g_phys[k]);
        r->pose.n = 0;
        if (k != g_player || g_race_kind == 2 || getenv("OMR_AUTOPILOT"))
            g_phys[k].driven = 0;               /* a computer rider (cpu.c) */
        rules_progress(k);
    }
}

/* 0x411010 and the start 0x47bf70: the riders back on the grid, every sound stopped, the
   race's sound init 0x4084e0, the track's music */
static void start(void)
{
    int k;
    if (g_winner_lap)
        winner_give_back();
    text_clear();
    /* 0x47bf70: in practice the second record is the time attack's ghost, when there is one */
    if (g_mode == MODE_PRACTICE) {
        if (g_time_attack)
            ghost_restart();
        g_racer[2].loaded = g_time_attack && ghost_count() != 0;
    }
    /* 0x411010: srand(0), the riders' records for bike against bike */
    memset(&g_bikeworld.rider, 0, sizeof g_bikeworld.rider);
    for (k = 1; k < g_riders; k++)
        g_bikeworld.rider[k] = &g_phys[k];
    g_bikeworld.nriders = g_riders;
    g_phys_cd = behaviour_cd();
    g_bikeworld.crt = 0;
    g_bikeworld.reverse = g_reverse ? -1 : 1;
    g_bikeworld.mode = g_mode;
    g_follow = g_race_kind == 2 ? 2 : 1;        /* 0x410d3d (Link: the local player) */
    g_racer[g_player].human = g_race_kind != 2; /* 0x47c07b */
    g_bikeworld.followed = g_follow;
    g_bikeworld.replay = g_race_kind;
    g_bikeworld.gear_latch = 0;
    g_bikeworld.countdown = 1;
    g_bikeworld.dt = 1.6f;
    g_bikeworld.frame_ms = STEP_MS;
    g_bikeworld.pose = pose_step;
    g_bikeworld.shadow = shadow_step;
    g_bikeworld.effects = effects_bike;
    g_bikeworld.debris = effects_debris;
    pose_restart();
    signs_start();
    figure_start();
    effects_restart();
    memset(g_phys, 0, sizeof g_phys);
    grid();
    memcpy(g_ghost_at, g_phys[2].pos, sizeof g_ghost_at);
    for (k = 1; k < g_riders; k++) {    /* (the first picture may come before the first step) */
        shadow_step(&g_phys[k]);
        memcpy(g_rider[k].shadow, g_phys[k].shadow, sizeof g_rider[k].shadow);
    }
    /* 0x47bf70: srand(timeGetTime()), kept for the race's replay; the computer riders'
       speeds (0x47bc60) */
    if (g_race_kind == 1)
        g_onboard = 0;
    else
        g_seed = g_now;
    /* (0x542874 stays 23 in a demo, rider 1 not among them) */
    cpu_start(g_bikeworld.rider, g_riders + (g_race_kind == 2), g_difficulty, g_pocket, g_seed,
              !g_playback);
    if (getenv("OMR_AUTOPILOT") && strcmp(getenv("OMR_AUTOPILOT"), "win") == 0)
        for (k = 2, g_test_win = 1; k < g_riders; k++)  /* the test's way to the winner's lap */
            g_phys[k].top *= 0.3f;
#ifdef MISTER_HYBRID
    {
        static int slow;                /* a replay of the race as the race was */
        if (g_race_kind == 0)
            slow = mister_cheat(MISTER_SLOW_RIVALS);
        if (slow && g_race_kind != 2)
            for (k = 2; k < g_riders; k++)
                g_phys[k].top *= 0.3f;
    }
#endif
    /* 0x411010 */
    if (g_race_kind == 0)
        replay_clear();
    else
        replay_rewind();
    g_demo_clock = 0;
    camera_reset(&g_phys[g_follow]);
    sound_stop_all();
    sound_race_load(g_bikes[g_rider[1].bike].perso != 0);
    race_sound_start(camera_eye(), g_player);   /* 0x4084e0 */
    font_follow_behaviour();
    music_play(g_land.audio_track, 0);
    memset(g_keys, 0, sizeof g_keys);
    bikephys_key(&g_phys[g_player], &g_keys[g_player], 0, 0, 0);     /* 0x4e8964 = 0 */
    rules_restart();
    hud_restart();
    g_finish_timer = g_timeover_timer = -1;
    g_step_at = g_last_now = g_now;
    g_first_step = 1;
    g_paused = 0;
    g_joy_turbo = g_ctrl_held = g_shift_held = 0;
    memcpy(g_before, g_rider, sizeof g_before);
}

int race_player(void)
{
    return g_follow;
}

int race_local(void)
{
    return g_player;
}

int race_load(char *err, int errsize)
{
    bitmap_cache_flush();       /* (the ESP32: the menu's pictures make room for the race's) */
    int k, opponents;
    g_race_steps = 0;
    if (!land_load(g_track, err, errsize))
        return 0;
    if (!bikes_load(g_track) || !bike3d_load(g_land.bank1_rgb)) {
        snprintf(err, (size_t)errsize, "The bikes of %s could not be read.",
                 g_tracks[g_track].name);
        land_free();
        return 0;
    }
    figure_load(g_tracks[g_track].folder);      /* 0x47b600 */
    screen_set();                               /* 0x410e10 -> 0x4592a0 */
    trackmap_build();                           /* 0x40ed10 */
    /* 0x47b420: practice 1, else 23 opponents, or 11 with "OPPONENTS: 12"; 0x410a50 */
    opponents = g_mode == 0 ? 1 : g_opponents && !g_playback ? 11 : 23;
    g_riders = (g_race_kind == 2 ? 1 : 2) + opponents;  /* 0x410a50: a demo has no player of its own */
    memset(g_rider, 0, sizeof g_rider);
    g_player = 1;
    g_rider[1].bike = g_bike >= 0 && g_bike < BIKES ? g_bike : 0;
    g_rider[1].skin = g_rider[1].bike & 7;
    for (k = 2; k < g_riders; k++) {
        /* +0x78 = 0; the liveries in turn, the player's own given to the one that would
           have had livery 0 */
        g_rider[k].bike = 0;
        g_rider[k].skin = (k - 1 == g_rider[1].bike ? 0 : k - 1) & 7;
    }
    memset(g_racer, 0, sizeof g_racer);
    for (k = 1; k < g_riders; k++)
        g_racer[k].loaded = 1;
    ghost_reset();              /* 0x47b420 */
    if (g_mode == MODE_PRACTICE && g_time_attack)
        ghost_load(rules_reference());
    g_racer[g_player].human = 1;
    g_events = 0;
    g_onboard = 0;              /* 0x410120 */
    if (!g_log && getenv("OMR_RACELOG"))
        g_log = fopen(getenv("OMR_RACELOG"), "w");
    start();
    g_racing = 1;
    return 1;
}

void race_release_keys(void)
{
    g_keys[g_player] = 0;
    g_phys[g_player].turbo_held = 0;
    bikephys_key(&g_phys[g_player], &g_keys[g_player], 0, 0, 0);
}

void race_event(unsigned bits)
{
    g_events |= bits;
}

/* 0x4580f0, a championship's advance (no points in Solo): the top three go to the next
   race, unlocking what it unlocks; the page after it (page 9 or the podium, 24; 16 and 17
   for the unlocks, 18 at the end); 0 when the championship is over */
static int champ_advance(int *page)
{
    const Racer *r = &g_racer[g_player];
    *page = r->state != RS_FINISHED ? 9 : 24;
    if (r->place <= 2 && g_champ_race < g_nchamp) {
        const ChampRace *c = &g_champ[g_champ_race];
        int i;
        if (c->unlock_pocket) {
            g_pocket_unlocked = c->unlock_pocket;
            *page = 17;
        }
        if (c->unlock_reverse) {
            g_reverse_unlocked = c->unlock_reverse;
            *page = 16;
        }
        for (i = 0; i < g_ntracks; i++)
            if (strcmp(g_tracks[i].name, c->unlock) == 0)
                g_tracks[i].available = 1;
        g_champ_race++;
    }
    if (g_champ_race >= g_nchamp) {
        *page = 18;
        return 0;
    }
    return 1;
}

/* 0x4587e0's part for the race's events */
static int events(void)
{
    unsigned e = g_events;
    int page = -1;
    g_events = 0;
    if (e & EV_DEMO_NEXT) {     /* 0x42e550 */
        if (++g_demo_at >= g_ndemos)
            g_demo_at = 0;
    } else if (e & EV_CLOSE_DEMOS) {
        g_demo_at = 0;
        demos_end();
    }
    if (e & EV_WATCH)
        g_race_kind = 1;
    else if (e & EV_NORMAL)
        g_race_kind = 0;
    if (e & EV_RESTART)
        start();
    if (e & EV_UNLOAD)
        race_unload();
    if (e & EV_CONTINUE)
        dialog_continue();
    else if (e & EV_PAUSE_AFTER)
        dialog_pause(0);
    if (e & EV_TOP5) {
        if (g_mode == MODE_CHAMPIONSHIP)
            g_champ_on = champ_advance(&page);
        else
            page = 9;
    }
    if (e & EV_PAGE6)
        page = 6;
    if (e & EV_TITLE)
        page = 14;
    if (e & EV_DEMO_LOAD) {
        int track = g_track;
        if (!demo_setup()) {
            race_unload();
            demos_end();
            page = 14;
            e |= EV_UNLOAD;
        } else if (track == g_track)
            start();
        else {
            race_unload();
            page = 1;
            e |= EV_UNLOAD;
        }
    }
    if (page >= 0)
        menu_change_page(page);
    return !(e & EV_UNLOAD);
}

/* 0x412960: a single race won, the computer rides on with the player's bike */
static int winner_take(void)
{
    if (g_race_kind != 0) {
        g_winner_lap = 0;
        return 0;
    }
    if (!g_solo || g_mode != MODE_SINGLE || g_racer[race_local()].place > 0)
        return 0;
    g_winner_top = g_phys[1].top;
    g_winner_counter = g_show_counter;
    bikephys_winner_take(&g_phys[1]);
    g_winner_lap = 1;
    race_release_keys();
    g_show_counter = g_hotkeys = g_onboard = 0;
    return 1;
}

/* 0x412a50's end of it, and 0x411010 on a restart */
static void winner_give_back(void)
{
    bikephys_winner_give_back(&g_phys[1], g_winner_top);
    g_winner_lap = g_winner_sequence = 0;
    g_show_counter = g_winner_counter;
    g_hotkeys = 1;
}

/* 0x412a50, the end of the race once a step: 7 s after the player's finish (a single race
   won: 14 s of the winner's lap, then its credits) the results (practice: the table and
   the continue dialog) and a new best lap kept; 3 s after the time ran out the continue
   dialog. */
static void race_end(int ms)
{
    if (g_playback)
        return;
    if (g_race_over) {
        if (g_finish_timer == -1) {
            g_finish_timer = 7000;
            sound_queue_voice(0x3e, 500);       /* RACECMPL */
            if (!g_winner_lap && winner_take())
                g_finish_timer = 14000;
        } else if (g_finish_timer > 1) {
            /* (14000 ms is 875 of the port's even steps: landing on 0 would skip the credits,
               which the original's uneven frame times almost never do) */
            if ((g_finish_timer -= ms) < 0 || (g_finish_timer == 0 && g_winner_lap))
                g_finish_timer = 1;
        } else if (g_finish_timer == 1) {
            if (!g_winner_lap)
                g_finish_timer = 0;
            else if (!g_winner_sequence) {
                credits_start(bikephys_crt_rand() > 0xfff);
                g_winner_sequence = 1;
                g_onboard = 0;
            } else if (!credits_running()) {
                winner_give_back();
                g_finish_timer = 0;
            }
        } else {
            podium_data();
            if (g_mode != MODE_PRACTICE)
                dialog_results();
            else {
                /* 0x412d4f: the player first over the line (a ghost beaten, or none): this run
                   is the new ghost, the old one's file overwritten */
                if (g_time_attack && g_racer[g_player].place == 0) {
                    ghost_take(g_racer[g_player].time);
                    rules_take_ghost(g_player);
                    ghost_save(rules_reference());
                }
                scores_write_race();
                dialog_continue();
            }
            if (g_race_kind == 0 && g_race_best_lap != NO_TIME && g_race_best_lap != 0) {
                Score s;                        /* 0x5d25b0 */
                size_t n = strlen(g_player_name);
                memset(&s, 0x7f, sizeof s);
                memset(s.b, 0, 9);
                memcpy(s.b, g_player_name, n < 8 ? n : 8);
                memcpy(s.b + 9, "TINI", 4);
                s.b[0xd] = 0;
                memcpy(s.b + 0xf5, &g_race_best_lap, 4);
                best_lap_write(&s);
                g_race_best_lap = NO_TIME;
            }
        }
    } else if (g_time_over) {
        if (g_timeover_timer == -1)
            g_timeover_timer = 3000;
        else if (g_timeover_timer > 0) {
            if ((g_timeover_timer -= ms) < 0)
                g_timeover_timer = 0;
        } else if (g_solo)
            dialog_continue();
    }
}

void race_unload(void)
{
    credits_stop();             /* 0x4113c0 */
    dialog_free();
    text_clear();
    sound_race_free();
    music_stop();
    bike3d_free();
    figure_free();
    trackmap_free();
    land_free();
    r3d_race(-1);
    r3d_focal(1911.0f);
    g_screen_mode = 2;                          /* 0x409f70: the menus' screen */
#ifdef MISTER_HYBRID
    video_direct_end();
    video_set_half(0);
#endif
    video_set_size(640, 480, 480);
    r3d_screen();
    g_racing = 0;
}

static float mix(float a, float b, float t)
{
    return a + (b - a) * t;
}

static void mixv(float *o, const float *a, const float *b, int n, float t)
{
    int i;
    for (i = 0; i < n; i++)
        o[i] = mix(a[i], b[i], t);
}

static float mix_angle(float a, float b, float t)       /* the short way round */
{
    double d = fmod((double)b - a, 6.283185307179586);
    if (d > 3.141592653589793)
        d -= 6.283185307179586;
    else if (d < -3.141592653589793)
        d += 6.283185307179586;
    return (float)(a + d * t);
}

/* a rider t of the way from a (the step before) to b (the last step); a crash starting
   or ending, or a bike put back, is drawn as the step left it */
static void blend(Rider *o, const Rider *a, const Rider *b, float t)
{
    double dx = b->pos[0] - a->pos[0], dy = b->pos[1] - a->pos[1], dz = b->pos[2] - a->pos[2];
    int i, k;
    *o = *b;
    if (t >= 1.0f || a->crashed != b->crashed || a->bike != b->bike
        || dx * dx + dy * dy + dz * dz > 1000.0 * 1000.0)
        return;
    mixv(&o->m[0][0], &a->m[0][0], &b->m[0][0], 9, t);
    mixv(o->t, a->t, b->t, 3, t);
    mixv(o->pos, a->pos, b->pos, 3, t);
    mixv(o->rear, a->rear, b->rear, 3, t);
    mixv(o->front, a->front, b->front, 3, t);
    o->lean = mix(a->lean, b->lean, t);
    o->pitch = mix(a->pitch, b->pitch, t);
    o->steer = mix(a->steer, b->steer, t);
    o->speed = mix(a->speed, b->speed, t);
    o->dive = mix(a->dive, b->dive, t);
    o->susp = mix(a->susp, b->susp, t);
    o->wheelie = mix(a->wheelie, b->wheelie, t);
    o->roll_side = mix(a->roll_side, b->roll_side, t);
    o->spin_rear = mix_angle(a->spin_rear, b->spin_rear, t);
    o->spin_front = mix_angle(a->spin_front, b->spin_front, t);
    mixv(&o->body[0][0], &a->body[0][0], &b->body[0][0], 9, t);
    mixv(o->body_pt, a->body_pt, b->body_pt, 3, t);
    mixv(&o->shadow[0][0], &a->shadow[0][0], &b->shadow[0][0], 18, t);
    mixv(&o->crash_shadow[0][0], &a->crash_shadow[0][0], &b->crash_shadow[0][0], 12, t);
    if (a->pose.n == b->pose.n)
        for (i = 0; i < b->pose.n; i++)
            for (k = 0; k < 3; k++)
                o->pose.a[i][k] = mix_angle(a->pose.a[i][k], b->pose.a[i][k], t);
}

/* 0x467b30: a fallen rider, or one with a raised hand, the hand skeleton; else 0x45f160: the
   detail by the distance over the zoom (0x6d1f74) */
static int g_eye_sec;           /* the section the camera is in, this picture */

/* 0x46e8b0, with the planes of 0x46e320 (0.6 x the zoom each side of straight ahead) and
   a margin of 2048: is the point near enough the view? */
static int near_the_view(const float c[3][3], const float eye[3], const float p[3])
{
    double v[3], ahead = 0, across = 0, x, y, len;
    int i;
    for (i = 0; i < 3; i++)
        v[i] = (double)p[i] - eye[i];
    if (sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]) > g_land.detail[0][g_detail] * 2.0)
        return 0;
    for (i = 0; i < 3; i++) {
        ahead += v[i] * c[i][2];
        across += v[i] * c[i][0];
    }
    if (ahead <= -2048.0 || ahead - g_land.detail[0][g_detail] >= 2048.0)
        return 0;
    x = 0.6 * camera_zoom();
    len = sqrt(1.0 + x * x);
    y = (ahead - x * across) / len;
    if (y <= -2048.0)
        return 0;
    y = (ahead + x * across) / len;
    return y > -2048.0;
}

/* 0x458b80: a bike is drawn only near the view and on a section the camera sees */
static int bike_seen(const float c[3][3], const float eye[3], const Rider *r)
{
    return near_the_view(c, eye, r->pos) && land_section_seen(g_eye_sec, c, r->sec);
}

/* Rider k's label: is the bike drawn (+0, set by 0x458b80) and where does the point above
   it fall (0x4818a0)? In the camera's coordinates, 125 up (0x4d1a20). */
int race_label_place(int k, int *sx, int *sy)
{
    const Rider *r = &g_shown[k];
    float c[3][3], eye[3], v[3], x, y, z, focal = FOCAL * camera_zoom();
    r3d_get_camera(c, eye);
    if (!bike_seen(c, eye, r))
        return 0;
    v[0] = r->pos[0] - eye[0];
    v[1] = r->pos[1] - eye[1] + 125.0f;
    v[2] = r->pos[2] - eye[2];
    x = c[0][0] * v[0] + c[2][0] * v[2] + c[1][0] * v[1];
    y = c[0][1] * v[0] + c[2][1] * v[2] + c[1][1] * v[1];
    z = c[0][2] * v[0] + c[2][2] * v[2] + c[1][2] * v[1];
    if (!(z >= 64.0f))          /* (0..64: the original keeps whatever was on its stack) */
        return 0;
    *sx = (int)nearbyintf((float)(g_scr_w >> 1) + x * focal * (1.0f / z));
    *sy = (int)nearbyintf((float)(g_scr_h >> 1) - y * focal * (1.0f / z));
    return 1;
}

void race_frame(void)
{
#ifdef MISTER_HYBRID
    int small = 0;
#endif
    const Rider *me = &g_shown[g_follow];
    const BikePhys *b = &g_phys[g_follow];
    float eye[3], c[3][3], a, focal;
    static const float zero[3];
    int i, k, sec, flags;
    if (g_paused) {             /* the last picture stays; no time passes */
        g_step_at += g_now - g_last_now;
        g_last_now = g_now;
        pointer_undraw();
        pointer_draw();
        return;
    }
    if (g_new_screen) {         /* 0x47ff90's end: 0x4e9ce4 */
        screen_set();
        racekeys_new_screen();
    }
    if (g_events && !events())
        return;
    text_clock(g_now);
    pointer_step((int)(g_now - g_last_now));    /* 0x4120e0 */
    dialog_frame();
    flags = dialog_flags();
    if (dialog_holds()) {       /* (the time since the last step kept) */
        g_step_at += g_now - g_last_now;
        if ((g_input_device == 2 || g_input_device == 4) && !joy_is_pad(0))
            joy_menu(race_key); /* 0x411a10: the stick as the arrows and Return (a gamepad: pad.c) */
    }
    g_last_now = g_now;
    /* the original steps once a picture by the time it took, and GOG's -FrameRateMax60
       makes every picture 16 ms (1000 / 60 in integers): 62.5 steps a second of dt 1.6.
       The port runs one such step for every 16 ms of the clock. */
#ifdef OMR_ESP32
    {   /* the board: at most 4 steps a picture; behind more than that, the race slows down
           rather than chasing the clock for ever (a step can take longer than 16 ms there) */
        long behind = (long)(g_now - g_step_at) / STEP_MS;
        if (behind > 4) {
            g_esp_dropped += (unsigned)(behind - 4);
            g_step_at = g_now - 4 * STEP_MS;
        }
    }
    unsigned long long t_steps = esp32_us();
#endif
    while (g_now - g_step_at >= STEP_MS) {
#ifdef OMR_ESP32
        g_esp_steps++;
#endif
        g_step_at += STEP_MS;
        memcpy(g_before, g_rider, sizeof g_before);
        camera_keep();
        /* 0x411010: the game's first frame takes what the clock says since the start, as a
           rule a millisecond (the rear wheel's turn, measured on the original) */
        step(g_first_step ? 1 : STEP_MS);
        g_first_step = 0;
        if (g_events & EV_DEMO_LOAD)
            return;
        /* what the drawer leaves for a crash (+0x3085, +0x30b5), from the step's state,
           so the pictures in between never reach the game */
        for (k = 1; k < g_riders; k++) {
            bike3d_body(&g_rider[k]);
            from_rider(&g_phys[k], &g_rider[k]);
        }
    }
    prof_mark(MARK_STEPS);
#ifdef OMR_ESP32
    g_esp_step_us += esp32_us() - t_steps;
#endif
    a = g_smooth ? (float)(g_now - g_step_at) / STEP_MS : 1.0f;
    for (k = 1; k < g_riders; k++)
        blend(&g_shown[k], &g_before[k], &g_rider[k], a);
    camera_apply(a);
    r3d_get_camera(c, eye);
    /* the TV's eye can be far from the bike: the whole track searched (0x480103) */
    sec = land_section(eye, (g_race_kind || g_playback || g_winner_lap) && g_land.ncam > 0
                       ? -1 : me->sec);
    g_eye_sec = sec;
#ifdef MISTER_HYBRID
    video_direct_begin();
    /* the 3D at 640x480, its dials and horizon that screen's */
    if (g_half_rows) {
        video_half_3d(1);
        g_screen_mode = 2;
        small = g_small_fonts, g_small_fonts = 0;
        g_hud_top *= 2, g_hud_bottom *= 2;
        r3d_screen();
    }
#endif
    focal = FOCAL * camera_zoom();
    horizon_draw((const float (*)[3])c, zero, eye[1], g_land.horizon_offset,
                 g_land.horizon_scale, focal);
    r3d_race(g_detail);
    r3d_focal(focal);
    land_draw(sec, (const float (*)[3])c, eye);
    bike3d_lighting(1);
    for (i = 1; i < g_riders; i++)
        if (i == g_follow && onboard_now()) {
            if (cockpit_shown())
                bike3d_cockpit(&g_shown[i], g_shown[g_follow].lean);
        } else if (g_racer[i].loaded && bike_seen((const float (*)[3])c, eye, &g_shown[i])) {
            bike3d_draw_rider(&g_shown[i], g_shown[i].lod);   /* +0x638: the step's, 0x45f160 */
            shadow_draw(&g_shown[i], i);
        }
    bike3d_lighting(0);
    effects_draw(g_detail, a);
    hud_bike(b);
    if (flags & RACE_DRAW_DIALS) {      /* the drawing flags' bit 3: off under a dialog */
        hud_dials_3d();
        if (!g_winner_sequence) {               /* 0x40f2d0 */
            trackmap_draw(g_shown, g_follow, FOCAL);
            r3d_focal(focal);
        }
#ifdef MISTER_HYBRID
        r3d_screen_down(g_hud_top);             /* (the top margin takes the 3D HUD along) */
#endif
        signs_draw();
        r3d_focal(FOCAL);                       /* 0x47b7a0: the zoom 1.0 */
        figure_draw();
        r3d_focal(focal);
#ifdef MISTER_HYBRID
        r3d_screen();
#endif
    }
    prof_mark(MARK_SCENE);
#ifdef OMR_ESP32
    unsigned long long t_draw = esp32_us();
#endif
    r3d_render();
#ifdef OMR_ESP32
    g_esp_draw_us += esp32_us() - t_draw;
#endif
    prof_mark(MARK_RENDER);
#ifdef MISTER_HYBRID
    if (g_half_rows) {
        video_half_3d(0);
        g_screen_mode = 0;
        g_small_fonts = small;
        g_hud_top /= 2, g_hud_bottom /= 2;
        r3d_screen();
    }
#endif
    if (!(flags & RACE_DRAW_HUD)) {
        text_update();
        if (flags & RACE_DRAW_LIST1)
            text_draw_list(1);
    } else if (!g_winner_sequence) {    /* race+0xc: none of it under the credits */
        hud_draw(flags & RACE_DRAW_DIALS);
        text_update();
        text_draw();            /* the race's messages */
    }
    dialog_draw();
    credits_frame();
    pointer_draw();
    sound_update();             /* 0x405f30: the voice queue */
}
