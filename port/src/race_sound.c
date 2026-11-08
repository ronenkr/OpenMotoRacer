/* The race's sounds (NOTES §4.10.6.3 and "Race rules, part C"): the engines of the followed
   bike and of the 4 nearest others (0x4079e0) with their distance, Doppler and pan, what
   each bike's engine object plays (0x406290: the gear's loop and its pitch, the idle, the
   start, the skids and the surfaces, brakes, landings, hits and crashes), the rider's
   shouts (0x407600), and the set-up at the race's start (0x4084e0).

   The sounds go through the sound server's own calls (sound_server_play, sound_volume,
   sound_pan, sound_frequency), as the game's do, with every volume, pan and frequency an
   integer computed as the game's x87 computes it (doubles, __ftol truncating). */
#include "race_sound.h"
#include "ftol.h"
#include "hud.h"
#include "bikes.h"
#include "choices.h"
#include "clock.h"
#include "config.h"
#include "exe.h"
#include "land.h"
#include "r3d.h"
#include "rand.h"
#include "rules.h"
#include "script.h"
#include "sound.h"
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "fastfloat.h"      /* (last: the ESP32's single precision) */

#define RAND() rand_next(4)     /* 0x4b1160(4) */

/* the carac row's floats (0x509278 + bike x 0x124, from +0x14) */
#define CARAC(b, off) (g_bikes[(b)->bike].phys[((off) - 0x14) / 4])

typedef struct {                /* an engine object, 0x78 bytes (0x5034b0, 0x5032c0 + i x 0x78) */
    int state;                  /* +0x00: 1 the gear's loop plays, 2 too slow (stopped) */
    int fast;                   /* +0x04: was faster than 5 km/h */
    int water;                  /* +0x08: the water loops started */
    int skid_on;                /* +0x0c */
    int skid;                   /* +0x10: the slide's loop */
    int surf;                   /* +0x14: the road bike's surface loop */
    int wpas, wdeb;             /* +0x18, +0x1c: the water's */
    int idle;                   /* +0x20: motostp */
    int gear, gear2;            /* +0x24, +0x28: the gear's loop */
    int start;                  /* +0x2c: depart1..3 */
    int brake;                  /* +0x30: the brake's loop */
    int vol;                    /* +0x34: the volume the caller gave */
    int brake_fade;             /* +0x38 */
    int brake_hz, brake_vol;    /* +0x3c, +0x40 */
    int skid_vol;               /* +0x44 */
    int last_s;                 /* +0x48: the surface of the last call */
    int ramp;                   /* +0x4c: 0..3000 Hz off the pitch */
    int skid_hz;                /* +0x50 */
    int off_vol;                /* +0x54: the volume with the throttle closed */
    unsigned last_keys;         /* +0x58 */
    float last_r;               /* +0x5c */
    unsigned off_time;          /* +0x60 */
    int last_gear;              /* +0x64 */
    float last_wheelie;         /* +0x68 */
    float max_wheelie;          /* +0x6c */
    int detune;                 /* +0x70: 0, 100, 200, 300 for the 4 others */
    int last_crashed;           /* +0x74 */
} Engine;

static Engine g_own;            /* 0x5034b0 */
static Engine g_near[4];        /* 0x5032c0 */
static struct { int used, idx; float dist; } g_slot[4];         /* 0x504210 */
static int g_last_followed;     /* 0x5028bc */
static float g_cam_prev[3];     /* 0x5032b0 */
static int g_engine_on, g_ambience_on;  /* 0x4db65c, 0x4db660 (0 at the start, 0x408720) */

/* the shouts' timers */
static int g_slide_on, g_slide_said;    /* 0x5034a8, 0x503548 */
static unsigned g_slide_t, g_slide_last;        /* 0x503534, 0x5028ac */
static int g_air_on, g_air_said;        /* 0x5034a0, 0x5028b4 */
static unsigned g_air_t, g_air_last;    /* 0x504560, 0x503540 */
static int g_flat_on, g_flat_said;      /* 0x5034a4, 0x50352c */
static unsigned g_flat_t;               /* 0x503558 */

static unsigned fbits(float f)
{
    unsigned u;
    memcpy(&u, &f, 4);
    return u;
}

/* ------------------------------------------------------------------ the engine object */

/* 0x406190 */
static void stop_all(Engine *e)
{
    int *h[9] = { &e->skid, &e->surf, &e->wpas, &e->wdeb, &e->idle, &e->start, &e->gear,
                  &e->gear2, &e->brake };
    int i;
    for (i = 0; i < 9; i++)
        if (*h[i] != -1) {
            sound_stop(*h[i]);
            *h[i] = -1;
        }
}

/* the reset 0x406290, 0x4079e0 and 0x4084e0 make: the handles forgotten, the states 0
   (the volume, timers and detune kept) */
static void reset(Engine *e)
{
    e->brake = e->gear2 = e->gear = e->start = e->idle = e->wdeb = e->wpas = e->surf =
        e->skid = -1;
    e->last_gear = 0;
    e->last_wheelie = e->max_wheelie = 0.0f;
    e->last_crashed = e->state = e->brake_fade = e->fast = e->water = e->skid_on = 0;
    e->last_s = 0;
    e->last_keys = 0;
    e->off_vol = 0;
}

/* 0x465d20 (rear point, +9): the ground under a point by the section's sides (0x40ec60):
   on the road +0xe4, else +0xe8; the sections' T (+0x30) is 0 on every track */
static int surface(const BikePhys *b, const float p[3])
{
    const Section *s;
    double x, y, z, l;
    if (!g_land.sec || g_land.nsec <= 0 || b->sec < 0 || b->sec >= g_land.nsec)
        return 9;
    s = &g_land.sec[b->sec];
    x = (double)p[0] - s->pos[0];
    y = (double)p[1] - s->pos[1];
    z = (double)p[2] - s->pos[2];
    l = (z * s->axes[0][2] + y * s->axes[0][1]) + x * s->axes[0][0];
    if (l > s->edge[0] && s->edge[1] > (float)l)
        return s->ground_on + 9;
    return s->ground_off + 9;
}

/* 0x48cf20: the velocity's up part in the bike's frame (0x40fe80 + 0x410010 on the unit
   direction times the speed; the same as bikephys.c's local_velocity) */
static double vertical(const BikePhys *b)
{
    float d[3], v[3], t1;
    const float *m = b->m;
    double l;
    int i;
    memcpy(d, b->dir, sizeof d);
    l = (float)sqrt(((double)d[1] * d[1] + (double)d[2] * d[2]) + (double)d[0] * d[0]);
    if (l != 0.0) {
        double k = 1.0 / (float)l;
        for (i = 0; i < 3; i++)
            d[i] = (float)(d[i] * k);
    }
    for (i = 0; i < 3; i++)
        v[i] = (float)((double)d[i] * b->speed);
    t1 = (float)-(((double)m[5] * m[11] + (double)m[4] * m[10]) + (double)m[9] * m[3]);
    return (((double)m[5] * v[2] + (double)m[4] * v[1]) + (double)m[3] * v[0]) + t1;
}

static void vol_pan(int h, int vol, int k, int pan)
{
    sound_volume(h, (vol * k) >> 8);
    sound_pan(h, pan);
}

/* 0x406290: one bike's sounds. vol 0..255 (the distance), pan -64..64, own: the followed
   bike (its start sounds), f the Doppler's pitch factor */
static void engine(Engine *e, const BikePhys *b, int vol, int pan, int own, float f)
{
    static const int gear_ninja[8] = { 0, 0x22, 0x22, 0x26, 0x26, 0x26, 0x26, 0 };  /* 0x4db618 */
    static const int gear_ktm[8] = { 0, 0x22, 0x23, 0x24, 0x25, 0x26, 0x26, 0xff }; /* 0x4db638 */
    static const int hz_ninja[7][2] = {         /* 0x4db5a8: low, high by gear */
        { 0, 0 }, { 8000, 48000 }, { 10000, 48000 }, { 2000, 40000 }, { 2000, 40000 },
        { 2000, 40000 }, { 2000, 40000 } };
    static const int hz_ktm[7][2] = {           /* 0x4db5e0 */
        { 0, 0 }, { 9000, 29000 }, { 1000, 26000 }, { 1000, 26000 }, { 1500, 16000 },
        { 1000, 16000 }, { 1000, 16000 } };
    const int *gears;
    const int (*hz)[2];
    float speed = b->speed, top = CARAC(b, 0x48), r;
    unsigned keys = (unsigned)b->keys1, ev = (unsigned)b->events;
    int s = surface(b, b->rear), dirt, base, id, started, gear, k, h;
    double sd;

    if (!b->crashed && e->last_crashed) {       /* back up after a crash */
        stop_all(e);
        reset(e);
    }
    if ((int)fbits(f) < 0x3f000000)
        f = 0.5f;
    if ((int)fbits(f) > 0x3fc00000)
        f = 1.5f;
    e->vol = vol;
    dirt = g_bikes[b->bike].perso != 0;
    gears = dirt ? gear_ktm : gear_ninja;
    hz = dirt ? hz_ktm : hz_ninja;
    r = (float)(b->rpm / (double)CARAC(b, 0x74));      /* MoteurVitesseMax? (+0x74) */
    if (!b->driven)
        own = 0;
    if ((int)fbits(r) > 0x3f800000)
        r = 1.0f;
    else if ((double)r < 0.05)
        r = 0.05f;
    if (!b->ground)
        s = 0;
    switch (s) {
    case 1: s = 2; break;
    case 9: s = 6; break;
    case 0xb: s = 5; break;
    case 0xc: s = 4; break;
    }

    /* the events of the step: a crash bounce, a wall, the hits; the player's voices */
    if (ev & 0x40) {
        h = sound_server_play(0x11, 2);         /* rouleman */
        vol_pan(h, e->vol, 0xcd, pan);
    }
    if (ev & 0x2a) {
        h = sound_server_play(0x12, 2);         /* crash */
        vol_pan(h, e->vol, ev & 2 ? 0xff : 200, pan);
    }
    if ((ev & 0x14) && b->number == race_sound_player()) {
        if (RAND() % 4 == 0 && sound_voices_on()) {
            sound_set_last_voice(0x3d);         /* oouups */
            sound_volume(sound_server_play(0x3d, 0), 0xff);
        }
    }
    if ((ev & 2) && b->number == race_sound_player()) {
        if (RAND() % 4 == 0 && sound_voices_on()) {
            sound_set_last_voice(0x4b);         /* ouch */
            sound_volume(sound_server_play(0x4b, 0), 0xff);
        }
    }
    if (ev & 0x15) {                            /* choc1 / choc2 */
        int x = RAND();
        k = ftol((double)speed * 7.2 + 170.0);
        if (k > 0xe1)
            k = 0xe1;
        h = sound_server_play(0xe + x % 2, 2);
        vol_pan(h, e->vol, k, pan);
        sound_frequency(h, ftol((double)f * 19050.0f));
    }
    if (b->ground && !b->ground1) {             /* a landing */
        float v = (float)-vertical(b);
        id = dirt ? RAND() % 3 + 0xb : 0xb;
        h = sound_server_play(id, 2);
        k = ftol((double)v * 3.0f + 180.0f);
        if (k > 0xff)
            k = 0xff;
        vol_pan(h, e->vol, k, pan);
    }

    /* the slide */
    if (b->sliding) {
        if (!e->skid_on) {
            id = -1;
            switch (s) {
            case 3: case 5: case 8: case 10: case 12:
                e->skid_hz = 18000, id = 0x17;  /* gravier */
                break;
            case 4:
                e->skid_hz = 8000, id = 0x19;   /* herbe */
                break;
            case 6: case 7:
                e->skid_hz = 5000, id = 0x0a;   /* derap */
                break;
            case 13:
                e->skid_hz = 20000, id = 0x18;  /* glace */
                break;
            }
            if (id != -1) {
                e->skid_vol = 0x50;
                if (e->skid != -1) {
                    sound_stop(e->skid);
                    e->skid = -1;
                }
                e->skid = sound_server_play(id, 3);
                vol_pan(e->skid, e->vol, e->skid_vol, pan);
                e->skid_on = 1;
            }
        }
        if (e->skid != -1) {
            sound_frequency(e->skid, ftol(((double)e->skid_hz + (double)speed * 200.0f) * f));
            vol_pan(e->skid, e->vol, e->skid_vol, pan);
            if (e->skid_vol < 200 && (e->skid_vol += 30) > 200)
                e->skid_vol = 200;
        }
    } else if (e->skid != -1) {
        vol_pan(e->skid, e->vol, e->skid_vol, pan);
        e->skid_vol -= 30;
        if (e->skid_vol < 0x50) {
            sound_stop(e->skid);
            e->skid = -1;
        }
        e->skid_on = 0;
    }

    /* water, and a road bike off the road */
    base = ftol((double)speed * 400.0f + 1050.0f);
    id = 0x17;
    started = 0;
    switch (s) {
    case 2:
        if (e->last_s != 2 && !e->water) {
            e->wdeb = sound_server_play(0x14, 2);       /* wterdeb */
            sound_frequency(e->wdeb, ftol((double)base * f));
            e->wpas = sound_server_play(0x15, 3);       /* wterpas */
            e->water = 1;
        }
        if (e->water) {
            sound_frequency(e->wpas, ftol((double)base * f));
            k = 0xff;
            if ((double)speed < 2.7777777777777777)
                k = ftol((double)speed * 255.0f * 0.36);
            vol_pan(e->wpas, e->vol, k, pan);
        }
        break;
    case 4:
        id = 0x19;
        /* fall through */
    case 3: case 5: case 10:
        if (dirt)
            break;
        if (e->last_s != s) {
            if (e->surf != -1) {
                sound_stop(e->surf);
                e->surf = -1;
            }
            e->surf = sound_server_play(id, 3);
            started = 1;
        }
        sound_frequency(e->surf, ftol((double)speed * 20.0f + 12000.0f));
        k = 200;
        if ((double)speed < 8.333333333333334)
            k = ftol((double)speed * 200.0f * 0.12);
        vol_pan(e->surf, e->vol, k, pan);
        break;
    }
    if (e->surf != -1 && !started && e->last_s != s) {
        sound_stop(e->surf);
        e->surf = -1;
    }
    if (e->last_s == 2 && s != 2) {
        sound_stop(e->wdeb);
        e->wdeb = -1;
        sound_stop(e->wpas);
        e->wpas = -1;
        sound_frequency(sound_server_play(0x16, 2), base);       /* wterfin */
        e->water = 0;
    }

    /* the front wheel comes down after a wheelie */
    if ((fbits(e->last_wheelie) & 0x7fffffff) && !(fbits(b->wheelie) & 0x7fffffff)
        && b->ground) {
        h = sound_server_play(0x13, 2);         /* wheel */
        if ((double)e->max_wheelie > 1.5707963267948966)
            e->max_wheelie = 1.57079637f;
        vol_pan(h, e->vol, ftol((double)e->max_wheelie * 121.59437652220804) + 0x40, pan);
        e->max_wheelie = 0.0f;
    }

    /* the idle below 29 km/h (8.0556 m/s) */
    gear = b->gear;
    sd = speed;
    if (speed < 8.055555555555555) {
        if (e->idle == -1)
            e->idle = sound_server_play(0x21, 3);       /* motostp */
        k = ftol((8.055555555555555 - sd) * 48.31578947368421);
        if (k > 0xff)
            k = 0xff;
        if (k < 0)
            k = 0;
        vol_pan(e->idle, e->vol, k, pan);
        sound_frequency(e->idle, ftol((((double)e->detune + (double)r * 8000.0f) + 16050.0f)
                                      * f));
        if (sd < 1.3888888888888888) {           /* 5 km/h: the gear's loop stops */
            sound_stop(e->gear);
            e->gear = -1;
            sound_stop(e->gear2);
            e->gear2 = -1;
            e->state = 2;
        } else if (e->state == 2)
            e->state = 0;
    } else if (e->idle != -1) {
        sound_stop(e->idle);
        e->idle = -1;
        if (e->state == 2)
            e->state = 0;
    }

    /* the suspension creaks as the bike comes to a stop */
    if (e->brake_fade && e->fast && sd < 1.3888888888888888) {
        if (RAND() % 2 == 0) {
            h = sound_server_play(0x1c, 2);     /* grince / suspens1 */
            vol_pan(h, e->vol, 230, pan);
            sound_frequency(h, ftol((double)(RAND() % 4000 + 14000) * f));
        }
        e->fast = 0;
    }
    if (sd > 1.3888888888888888)
        e->fast = 1;

    /* braking on the ground above 30 km/h */
    if ((keys & 2) && b->ground) {
        if (e->brake == -1 && sd > 8.333333333333334) {
            id = -1;
            e->brake_vol = 0xe6;
            e->brake_hz = 22050;
            switch (s) {
            case 3: case 5: case 8: case 10:
                id = 0x17;
                break;
            case 4:
                id = 0x19;
                break;
            case 6: case 7:
                e->brake_hz = 12050;
                id = RAND() % 2 + 0x1a;         /* brake1 / brake2 */
                break;
            case 13:
                id = 0x18;
                break;
            }
            if (id != -1) {
                if (e->brake != -1) {
                    sound_stop(e->brake);
                    e->brake = -1;
                }
                e->brake = sound_server_play(id, 3);
            }
            e->brake_fade = 0;
        }
        if (e->brake != -1) {
            int t = ftol((double)e->brake_hz + (double)speed * 4000.0f / top);
            sound_frequency(e->brake, ftol((double)t * f));
            vol_pan(e->brake, e->vol, e->brake_vol, pan);
        }
    }
    if ((e->brake != -1 && !(keys & 2)) || (double)r < 0.2)
        e->brake_fade = 1;
    if (e->brake_fade) {
        e->brake_vol -= 50;
        if (e->brake_vol < 0) {
            sound_stop(e->brake);
            e->brake = -1;
            e->brake_fade = 0;
        } else
            vol_pan(e->brake, e->vol, e->brake_vol, pan);
    }

    /* the throttle opened: the start sound */
    if (own && (keys & 1) && !(e->last_keys & 1)) {
        if (!dirt) {
            k = ftol((double)speed / top * 5.0f);
            if (k > 2)
                k = 2;
            id = 0x1f - k;
        } else
            id = RAND() % 3 + 0x1d;
        e->start = sound_server_play(id, 2);
        vol_pan(e->start, e->vol, 200, pan);
    }

    /* the gear's loop: another sample when the gear's id changes */
    if (e->state != 2 && gear > 0 && gear < 8) {
        int old = -1;
        if (e->gear != -1 && gear != e->last_gear && e->last_gear >= 0 && e->last_gear < 8
            && gears[e->last_gear] != gears[gear]) {
            old = e->gear;
            e->gear = e->gear2 = -1;
        }
        if (e->gear == -1) {
            e->gear = sound_server_play(gears[gear], 3);
            e->state = 1;
            sound_stop(old);
        }
    }
    if (e->state == 1) {
        int g = gear < 0 ? 0 : gear > 6 ? 6 : gear, x, y;
        int f0 = ftol((double)(hz[g][1] - hz[g][0]) * r + (double)hz[g][0]);
        if ((keys & 1) && !(e->last_keys & 1) && RAND() % 4 == 0)
            f0 += RAND() % 3000 + 2000;         /* a misfire as the throttle opens */
        if (s == 3) {
            if (e->ramp < 3000)
                e->ramp += 1000;
        } else if (e->ramp > 0)
            e->ramp -= 1000;
        x = ftol((double)b->wheelie * 4000.0f + (double)(f0 - e->ramp));
        y = ftol(((double)b->front[1] - b->rear[1]) / ((double)CARAC(b, 0x38) * 48.0f)
                 * 1800.0f);
        sound_frequency(e->gear, ftol((double)(x - y + e->detune) * f));
        k = 250;
        if (b->driven) {
            if (!(keys & 1) && !(fbits(b->wheelie) & 0x7fffffff)) {
                if (g_now - e->off_time > (dirt ? 100u : 50u)) {
                    k = 240 - RAND() % 50;
                    e->off_vol = k;
                    e->off_time = g_now;
                } else
                    k = e->off_vol;
            } else
                k = 255;
        }
        if (s == 3) {
            k += RAND() % 50 - 30;
            if (k < 0)
                k = 0;
        }
        if (sd < 5.555555555555555)
            k = ftol((sd - 0.2777777777777777) * 0.18947368421052632 * k);
        vol_pan(e->gear, e->vol, k, pan);
    }

    e->last_r = r;
    e->last_keys = keys;
    e->last_gear = gear;
    e->last_crashed = b->crashed;
    e->last_s = s;
    e->last_wheelie = b->wheelie;
    if (e->max_wheelie < b->wheelie)
        e->max_wheelie = b->wheelie;
}

/* ------------------------------------------------------------------ the rider's shouts */

static int g_player = 1;        /* 0x4ea4c0 */

int race_sound_player(void)
{
    return g_player;
}

/* 0x407600: the local player's own rider */
static void shouts(const BikePhys *b)
{
    static const int air_ids[4] = { 0x4c, 0x4a, 0x51, 0x3a };   /* 0x4db978 */
    int dirt = g_bikes[b->bike].perso != 0;
    if (b->sliding) {                           /* a long slide: "yeees" */
        if (!g_slide_on) {
            g_slide_on = 1;
            g_slide_said = 0;
            g_slide_t = g_now;
        }
        if (g_now - g_slide_t > 3500) {
            if (g_slide_last > g_now)
                g_slide_last = g_now;
            if (!g_slide_said && g_now - g_slide_last > 4000) {
                if (RAND() % 3 == 0 && sound_voices_on()) {
                    sound_set_last_voice(0x4e);
                    sound_volume(sound_server_play(0x4e, 0), 0xff);
                }
                g_slide_said = 1;
                g_slide_last = g_now;
            }
        }
    } else
        g_slide_on = 0;
    if (dirt) {                                 /* a long jump */
        if (!b->ground) {
            if (!g_air_on) {
                g_air_on = 1;
                g_air_said = 0;
                g_air_t = g_now;
            }
            if (g_now - g_air_t > 1200 && !g_air_said && g_now - g_air_last > 4000) {
                if (RAND() % 3 == 0) {
                    int id = air_ids[RAND() & 3];
                    if (id >= 0x2f && id <= 0x5c) {
                        if (sound_voices_on() && sound_last_voice() != id) {
                            sound_set_last_voice(id);
                            sound_volume(sound_server_play(id, 4), 0xff);
                        }
                    } else if (sound_effects_on() && sound_last_effect() != id) {
                        sound_set_last_effect(id);
                        sound_volume(sound_server_play(id, 4), 0xee);
                    }
                }
                g_air_said = 1;
                g_air_last = g_now;
            }
        } else
            g_air_on = 0;
    } else if ((double)b->speed / CARAC(b, 0x48) > 0.93f) {    /* flat out: "what a rocket" */
        if (!g_flat_on) {
            g_flat_on = 1;
            g_flat_t = g_now;
        }
        if (g_now - g_flat_t > 15000 && !g_flat_said) {
            sound_queue_voice(0x45, 500);
            g_flat_said = 1;
        }
    } else
        g_flat_on = 0;
}

/* ------------------------------------------------------------------ the frame */

static float g_list_d[RIDERS];   /* obj+0x3049: each rider's distance to the camera */

static int by_distance(unsigned a, unsigned b)  /* 0x4079a0 */
{
    if (g_list_d[b] > g_list_d[a])
        return -1;
    if (g_list_d[b] < g_list_d[a])
        return 1;
    return 0;
}

/* (1 + u.vc C1)(1 + u.vb C2): the camera's and the bike's speeds along the line between
   them, in units a second; the sums y, z, then x as the asm forms them */
static float doppler(const BikePhys *b, const float cam[3], double k, const float vc[3])
{
    float vb[3], u[3], a, l;
    double sa, sb;
    int i;
    for (i = 0; i < 3; i++)
        vb[i] = (float)(((double)b->pos[i] - b->pos1[i]) * k);
    for (i = 0; i < 3; i++)
        u[i] = (float)((double)cam[i] - b->pos[i]);
    l = (float)sqrt(((double)u[2] * u[2] + (double)u[1] * u[1]) + (double)u[0] * u[0]);
    if (l != 0.0f) {
        double inv = 1.0 / l;
        for (i = 0; i < 3; i++)
            u[i] = (float)(u[i] * inv);
    }
    sa = ((double)u[1] * vc[1] + (double)u[2] * vc[2]) + (double)u[0] * vc[0];
    sb = ((double)vb[1] * u[1] + (double)u[2] * vb[2]) + (double)u[0] * vb[0];
    a = (float)((sa * -2.170138888888889e-05 + 1.0) * (sb * 2.170138888888889e-05 + 1.0));
    return a;
}

/* ftol((10240 - d) x 0.033203125) in 0..255 */
static int volume_at(float d)
{
    int v = ftol(((double)10240.0f - d) * 0.033203125f);
    return v > 0xff ? 0xff : v < 0 ? 0 : v;
}

/* 0x4079e0 */
static void engines(BikePhys *const *all, int followed, const float cam[3],
                    const float look[3], float dt, int replay)
{
    unsigned list[RIDERS];
    double k;
    float vc[3], fz, fx, f;
    int n = 0, count = 0, i, j, cur;

    for (i = 1; i < g_riders && i < RIDERS; i++) {     /* each rider's distance (+0x3049) */
        float d[3];
        if (!g_racer[i].loaded)
            continue;
        for (j = 0; j < 3; j++)
            d[j] = (float)((double)all[i]->pos[j] - cam[j]);
        g_list_d[i] = (float)sqrt(((double)d[1] * d[1] + (double)d[2] * d[2])
                                  + (double)d[0] * d[0]);
    }
    if (followed != g_last_followed) {
        stop_all(&g_own);
        reset(&g_own);
        g_last_followed = followed;
    }
    k = 1.0 / (dt * 0.01);                      /* a step's move to units a second */
    for (j = 0; j < 3; j++)
        vc[j] = (float)(((double)cam[j] - g_cam_prev[j]) * k);
    f = replay ? doppler(all[followed], cam, k, vc) : 1.0f;
    engine(&g_own, all[followed], volume_at(g_list_d[followed]), 0, 1, f);
    if (followed == g_player && !replay)
        shouts(all[followed]);

    /* the 4 nearest others within 10240 */
    for (i = 1; i < g_riders && i < RIDERS; i++)
        if (i != followed && g_racer[i].loaded)
            list[n++] = (unsigned)i;
    msvc_qsort(list, n, by_distance);
    while (count < n && count < 4 && fbits(g_list_d[list[count]]) < 0x46200000u)
        count++;
    for (i = 0; i < 4; i++)
        g_slot[i].used = 0;
    for (i = 0; i < count; i++)                 /* the ones already heard keep their slot */
        for (j = 0; j < 4; j++)
            if (g_slot[j].idx == (int)list[i]) {
                g_slot[j].dist = g_list_d[list[i]];
                g_slot[j].used = 1;
                list[i] = 0;
                break;
            }
    for (j = 0; j < 4; j++)
        if (!g_slot[j].used) {
            g_slot[j].idx = 0;
            stop_all(&g_near[j]);
        }
    for (i = 0, cur = 0; i < count; i++) {
        if (!list[i])
            continue;
        while (cur < 4 && g_slot[cur].used)
            cur++;
        if (cur >= 4)
            break;
        g_slot[cur].used = 1;
        g_slot[cur].idx = (int)list[i];
        g_slot[cur].dist = g_list_d[list[i]];
        reset(&g_near[cur]);
        cur++;
    }

    /* the camera's forward on the ground */
    {
        double dx = (double)look[0] - cam[0], dz = (double)look[2] - cam[2];
        double l = sqrt(dz * dz + dx * dx);
        fz = (float)(dz / l);
        fx = (float)(dx / l);
    }
    for (j = 0; j < 4; j++) {
        const BikePhys *b;
        double x, z, inv;
        float p;
        int vol;
        if (!g_slot[j].used)
            continue;
        b = all[g_slot[j].idx];
        f = doppler(b, cam, k, vc);
        vol = volume_at(g_slot[j].dist);
        x = (double)b->pos[0] - cam[0];
        z = (double)b->pos[2] - cam[2];
        inv = 1.0 / sqrt(z * z + x * x);
        p = (float)(inv * x * fz - inv * z * fx);       /* the sine off the camera's axis */
        if (!((double)p < 0.15f))
            p = (float)((double)p - 0.15f);
        else if (fbits(p) >= 0xbe19999au)
            p = (float)((double)p + 0.15f);
        else
            p = 0.0f;
        engine(&g_near[j], b, vol, ftol((double)p * 75.29412f), 0, f);
    }
}

/* ------------------------------------------------------------------ the track's ambience */

#define DESCS 32                /* 0x502930: at most 32 */
#define AMB 40                  /* the set 0x503568: at most 40 */
#define NAMES 48                /* 0x502718 */

enum {
    SD_LOOPED = 1, SD_PAN = 2, SD_RANDPAN = 4, SD_VOL = 8, SD_FADEIN = 0x10, SD_FADEOUT = 0x20,
    SD_FADEFREQ = 0x40, SD_INFPLAY = 0x80, SD_SINVOL = 0x100, SD_SINFREQ = 0x200
};

typedef struct {                /* a soundDesc of Sound.Spt (0x4c bytes) */
    int id, first, count, flags;        /* +0x00..+0x0c */
    int delay, delay_r;         /* +0x10, +0x14: ms */
    int play, play_r;           /* +0x18, +0x1c: ms (SINVOL / SINFREQ: the period, depth) */
    int fadein, fadeout;        /* +0x20, +0x24 */
    int rep, rep_r;             /* +0x28, +0x2c */
    int freq, freq_r;           /* +0x30, +0x34: base <= 0 is added to the file's rate */
    int ffreq;                  /* +0x38: FADEFREQ / SINFREQ depth */
    int vol, pan;               /* +0x3c, +0x40 */
    float near_, far_;          /* +0x44, +0x48: metres */
} SoundDesc;

typedef struct {                /* a sound object (0x40 bytes, 0x5037ec + i x 0x40) */
    const SoundDesc *d;         /* +0x00 */
    int h;                      /* +0x04 */
    int sample;                 /* +0x08 */
    int vol;                    /* +0x0c */
    unsigned start;             /* +0x10 */
    unsigned delay, play, fadein, fadeout;      /* +0x14..+0x20 */
    int repeat;                 /* +0x24 */
    int freq;                   /* +0x28 */
    int state;                  /* +0x2c: 0 wait, 1 play, 2 fade in, 3 stop, 4 fade out, 5 hold */
    float far128, scale, lastd; /* +0x30, +0x34, +0x38 */
    int stopped;                /* +0x3c */
} AmbObj;

static SoundDesc g_desc[DESCS];
static int g_ndesc;             /* 0x5028b0 */
static char g_name[NAMES][16];  /* 0x504570: the sample names, ids 0x5d.. */
static int g_nnames;            /* 0x503538 */
static struct { int kind, sec, signal, decor; } g_amb[AMB];     /* the set's entries */
static AmbObj g_obj[AMB];
static int g_namb;              /* 0x503568 */
static int g_amb_detail;        /* 0x503550 */
static int *g_zone;             /* section +0xf8: the signals' zone bits */
static int g_cam_sec = -1;      /* the track +0x20: the camera's section (last picture's) */

static const SoundDesc *desc_of(int id)
{
    int i;
    for (i = 0; i < g_ndesc; i++)
        if (g_desc[i].id == id)
            return &g_desc[i];
    return 0;
}

static int name_id(const char *name)            /* 0x4056c0 */
{
    int k;
    for (k = 0; k < g_nnames; k++)
        if (strcmp(g_name[k], name) == 0)
            return 0x5d + k;
    if (g_nnames == NAMES)
        return -1;
    snprintf(g_name[g_nnames], sizeof g_name[0], "%.15s", name);
    g_nnames++;
    return 0x5c + g_nnames;
}

static void add(int kind, const SoundDesc *d, int sec, int signal, int decor)
{
    if (!d || g_namb >= AMB)
        return;
    g_amb[g_namb].kind = kind;
    g_amb[g_namb].sec = sec;
    g_amb[g_namb].signal = signal;
    g_amb[g_namb].decor = decor;
    memset(&g_obj[g_namb], 0, sizeof g_obj[0]);
    g_obj[g_namb].d = d;
    g_obj[g_namb].h = -1;
    g_obj[g_namb].stopped = 1;
    g_namb++;
}

/* 0x405ea0 at the track's load: the zone mask (0x40e0da), Sound.Spt (0x42e850), the
   decor objects with a sound (0x405790) */
static void ambience_parse(void)
{
    static const char *const flag[10] = { "LOOPED", "PAN", "RANDPAN", "VOL", "FADEIN",
        "FADEOUT", "FADEFREQ", "INFPLAY", "SINVOL", "SINFREQ" };
    Script s;
    char spt[0x140], w[64];
    int i, j, mask = 0;
    g_ndesc = g_nnames = g_namb = 0;
    g_amb_detail = g_detail;
    free(g_zone);
    g_zone = calloc((size_t)(g_land.nsec > 0 ? g_land.nsec : 1), sizeof *g_zone);
    for (i = 0; g_zone && i < g_land.nsec; i++) {
        int e = g_land.sec[i].event;
        if (e >= 6 && e <= 12)
            mask ^= 1 << (e - 6);
        g_zone[i] = mask;
    }
    snprintf(spt, sizeof spt, "%sSound.Spt", g_tracks[g_track].folder);
    if (script_open(&s, spt)) {
        while (!script_at_end(&s) && !s.bad) {
            script_str(&s, w, sizeof w);
            if (s.bad)
                break;
            if (strcmp(w, "soundDesc") == 0) {
                SoundDesc d;
                int *v = &d.delay, n;
                memset(&d, 0, sizeof d);
                d.id = script_int(&s);
                d.count = n = script_int(&s);
                for (j = 0; j < n && !s.bad; j++) {
                    script_str(&s, w, sizeof w);
                    i = name_id(w);
                    if (j == 0)
                        d.first = i;
                }
                script_str(&s, w, sizeof w);
                for (i = 0; w[i]; i++)
                    if (w[i] == '|' || i == 0) {
                        const char *p = w + i + (w[i] == '|');
                        int k;
                        for (k = 0; k < 10; k++)
                            if (strncmp(p, flag[k], strlen(flag[k])) == 0) {
                                d.flags |= 1 << k;
                                break;
                            }
                    }
                for (i = 0; i < 13; i++)
                    v[i] = script_int(&s);
                d.near_ = script_float(&s);
                d.far_ = script_float(&s);
                if (!s.bad && g_ndesc < DESCS)
                    g_desc[g_ndesc++] = d;
            } else if (strcmp(w, "ambiance") == 0) {            /* 0x406030 */
                int n = script_int(&s);
                for (j = 0; j < n && !s.bad; j++)
                    add(2, desc_of(script_int(&s)), 0, 0, 0);
            } else if (strcmp(w, "signal") == 0) {              /* 0x4060a0 */
                int sig = script_int(&s), n = script_int(&s);
                for (j = 0; j < n && !s.bad; j++) {
                    int id = script_int(&s);
                    if (sig >= 6 && sig <= 12)
                        add(1, desc_of(id), 0, sig, 0);
                }
            }
        }
        script_close(&s);
    }
    for (i = 0; i < g_land.nsec && g_namb < AMB; i++)  /* 0x405790 */
        for (j = 0; j < g_land.sec[i].ndecor; j++) {
            int o = (int16_t)g_land.sec[i].decor[j];
            if (o < 0 || o >= g_land.ndecor || g_land.decor[o].anim == -1)
                continue;
            add(0, desc_of(g_land.decor[o].anim), i, 0, o);
        }
}

/* 0x4059a0: the set's samples, from Divers\ by their names */
static void ambience_load(void)
{
    int i, j;
    for (i = 0; i < g_namb; i++)
        for (j = 0; j < g_obj[i].d->count; j++) {
            int id = g_obj[i].d->first + j;
            if (id >= 0x5d && id < 0x5d + g_nnames)
                sound_load_named(id, g_name[id - 0x5d]);
        }
}

static void rearm(AmbObj *o)    /* 0x404ee0 */
{
    const SoundDesc *d = o->d;
    o->sample = d->first;
    if (d->count)
        o->sample += RAND() % d->count;
    o->h = -1;
    o->stopped = 1;
    o->freq = d->freq;
    if (o->freq <= 0)
        o->freq += sound_rate(o->sample);
    if (d->freq_r)
        o->freq += RAND() % d->freq_r;
    o->delay = (unsigned)d->delay;
    if (d->delay_r)
        o->delay += (unsigned)(RAND() % d->delay_r);
    o->play = (unsigned)d->play;
    if (d->play_r)
        o->play += (unsigned)(RAND() % d->play_r);
    o->repeat = d->rep;
    if (d->rep_r)
        o->repeat += RAND() % d->rep_r;
    o->fadein = (unsigned)d->fadein;
    o->fadeout = (unsigned)d->fadeout;
    o->vol = (d->vol * 0xff) >> 8;              /* x 0x5028c0 */
    o->start = g_now;
    o->state = 0;
    o->lastd = 0.0f;
    o->far128 = (float)(d->far_ * 128.0);
    o->scale = (float)(255.0 / (((double)d->far_ - d->near_) * 128.0));
}

static void amb_stop(AmbObj *o)
{
    sound_stop(o->h);
    o->h = -1;
    o->stopped = 1;
}

static void amb_play(AmbObj *o)
{
    o->h = -1;
    o->stopped = 1;
    if (sound_effects_on()) {
        o->h = sound_server_play(o->sample, (o->d->flags & 1) | 2);
        o->stopped = 0;
    }
}

/* 0x405030: one object's state machine at (x, y, z) as heard from the camera */
static void amb_object(AmbObj *o, int audible, float x, float y, float z, const float cam[3],
                       const float look[3])
{
    const SoundDesc *d = o->d;
    int att = 255, pan = d->pan, v;
    unsigned t, xi;
    if (!sound_effects_on())
        return;
    if (o->start > g_now)
        o->start = g_now;
    t = g_now - o->start;
    if (!audible) {
        if (!o->stopped)
            amb_stop(o);
        return;
    }
    if (d->flags & SD_VOL) {
        float dx = (float)((double)x - cam[0]), dy = (float)((double)y - cam[1]);
        float dz = (float)((double)z - cam[2]);
        double dd = sqrt(((double)dy * dy + (double)dz * dz) + (double)dx * dx);
        if (dd > o->far128) {
            if (o->lastd > o->far128 && !o->stopped)
                amb_stop(o);
            o->lastd = (float)dd;
            return;
        }
        att = ftol(((double)o->far128 - (float)dd) * o->scale);
        att = att > 255 ? 255 : att < 0 ? 0 : att;
        o->lastd = (float)dd;
    }
    if ((d->flags & SD_LOOPED) && o->state == 5 && o->stopped)
        amb_play(o);                            /* back within hearing while it holds */
    if (d->flags & SD_PAN) {
        double lz = (double)look[2] - cam[2], lx = (double)look[0] - cam[0];
        double inv1 = 1.0 / sqrt(lz * lz + lx * lx), fz = inv1 * lz, fx = inv1 * lx;
        double X = (double)x - cam[0], Zd = (double)z - cam[2];
        float zf = (float)Zd, p;
        double inv2 = 1.0 / sqrt(X * X + Zd * zf);
        double xn = X * inv2;
        float zn = (float)((float)inv2 * (double)zf);
        p = (float)(fz * xn - fx * zn);
        if (!((double)p < 0.15f))
            p = (float)((double)p - 0.15f);
        else if (fbits(p) >= 0xbe19999au)
            p = (float)((double)p + 0.15f);
        else
            p = 0.0f;
        pan = ftol((double)p * 75.29412f);
    }
    switch (o->state) {
    case 0:
        if (o->delay < t)
            o->state = 1;
        break;
    case 1:
        amb_play(o);
        sound_frequency(o->h, o->freq);
        o->start = g_now;
        if (d->flags & SD_FADEIN) {
            o->state = 2;
            v = 0;
        } else {
            v = o->vol;
            o->state = 5;
        }
        sound_volume(o->h, (v * att) >> 8);
        if (d->flags & SD_RANDPAN)
            pan = RAND() % 128 - 64;
        sound_pan(o->h, pan);
        break;
    case 2:                                     /* fade in */
        xi = o->fadein ? t * (unsigned)o->vol / o->fadein : 0;
        if ((int)xi > o->vol)
            xi = (unsigned)o->vol;
        v = ftol((double)att * ((double)o->vol * sin((double)(int)xi / (double)o->vol
                                                      * 1.5707963267948966))) >> 8;
        sound_volume(o->h, v);
        sound_pan(o->h, pan);
        if ((d->flags & SD_FADEFREQ) && o->fadein)
            sound_frequency(o->h, (int)((unsigned)d->ffreq * t / o->fadein) + o->freq);
        if (o->fadein < t) {
            o->start = g_now;
            o->state = 5;
        }
        break;
    case 3:                                     /* stop */
        amb_stop(o);
        if (--o->repeat == 0)
            rearm(o);
        o->state = 0;
        break;
    case 4:                                     /* fade out */
        xi = o->fadeout ? (o->fadeout - t) * (unsigned)o->vol / o->fadeout : 0;
        if ((int)xi < 0)
            xi = 0;
        v = ftol((double)att * ((double)o->vol * sin((double)(int)xi / (double)o->vol
                                                      * 1.5707963267948966))) >> 8;
        sound_volume(o->h, v);
        sound_pan(o->h, pan);
        if ((d->flags & SD_FADEFREQ) && o->fadeout)
            sound_frequency(o->h, (int)((o->fadeout - t) * (unsigned)d->ffreq / o->fadeout)
                            + o->freq);
        if (o->fadeout < t) {
            o->start = g_now;
            o->state = 3;
        }
        break;
    case 5:                                     /* hold */
        v = o->vol;
        if (!(d->flags & SD_INFPLAY) && o->play < t) {
            o->state = d->flags & SD_FADEOUT ? 4 : 3;
            o->start = g_now;
        }
        if (d->flags & SD_SINVOL)
            v = (ftol(sin((double)t / (double)d->play * 6.283185307179586)
                      * (double)(d->play_r * 0xff)) >> 8) + o->vol;
        if (d->flags & SD_SINFREQ)
            sound_frequency(o->h, ftol(sin((double)t / (double)d->play * 6.283185307179586)
                                       * (double)d->ffreq + (double)o->freq));
        sound_volume(o->h, (v * att) >> 8);
        sound_pan(o->h, pan);
        break;
    }
}

/* 0x40d350: s within range sections of the camera's, either way round the track */
static int near_camera(int s, int range)
{
    int n = g_land.nsec, c = g_cam_sec, lo, hi;
    if (n <= 0)
        return 0;
    lo = (n - range + c) % n;
    hi = (c + range) % n;
    if (hi < lo)
        return (lo < s && s < n) || (s >= 0 && hi > s);
    return lo < s && hi > s;
}

/* 0x40d3b0: for each of the 7 signals, the nearest section to the camera's (within
   range either way) in its zone */
static void signal_sections(int out[7], int range)
{
    int n = g_land.nsec, c = g_cam_sec, start, i, k;
    float best[7];
    const float *r;
    for (k = 0; k < 7; k++)
        best[k] = 3.40282347e38f, out[k] = -1;
    if (n <= 0 || !g_zone || c < 0 || c >= n)
        return;
    start = (n - range + c) % n;
    r = g_land.sec[c].pos;
    for (i = 0; i < 2 * range; i++) {
        int s = start + i;
        float dx, dy, dz, d2;
        if (s >= n)
            s -= n;
        if (s < 0 || s >= n)
            continue;
        dx = (float)((double)g_land.sec[s].pos[0] - r[0]);
        dy = (float)((double)g_land.sec[s].pos[1] - r[1]);
        dz = (float)((double)g_land.sec[s].pos[2] - r[2]);
        d2 = (float)(((double)dy * dy + (double)dz * dz) + (double)dx * dx);
        for (k = 0; k < 7; k++)
            if ((g_zone[s] & 1 << k) && best[k] > d2) {
                out[k] = s;
                best[k] = d2;
            }
    }
}

/* 0x405bc0: each entry's place; a decor's at its near model's first point turned by the
   decor's angles (the cosine table) */
static void ambience_place(const float cam[3], const float look[3])
{
    int near7[7], i;
    signal_sections(near7, 0x28);
    for (i = 0; i < g_namb; i++) {
        if (g_amb[i].kind == 0) {
            const Decor *o;
            const Model *m;
            double ca, sa, cb, sb, cc, sc, v0, v1, v2;
            float k, x, y, z;
            int a, aud;
            if (g_detail == 0)
                continue;
            aud = near_camera(g_amb[i].sec, 0x3c);
            o = &g_land.decor[g_amb[i].decor];
            m = o->lib >= 0 && o->lib < g_land.nlib ? &g_land.lib[o->lib].m[0] : 0;
            if (!m || m->nv < 1)
                continue;
            v0 = m->v[0][0], v1 = m->v[0][1], v2 = m->v[0][2];
            a = ftol((double)o->ang[0] * 2607.594587618272);
            ca = g_cos[a & 0x3fff], sa = g_cos[(a - 0x1000) & 0x3fff];
            a = ftol((double)o->ang[1] * 2607.594587618272);
            cb = g_cos[a & 0x3fff], sb = g_cos[(a - 0x1000) & 0x3fff];
            a = ftol((double)o->ang[2] * 2607.594587618272);
            cc = g_cos[a & 0x3fff], sc = g_cos[(a - 0x1000) & 0x3fff];
            k = (float)(sc * sb);
            x = (float)(((-(sc * cb) * v1 + (cc * cb) * v0) + v2 * sb) + o->pos[0]);
            y = (float)((((cc * ca - k * sa) * v1 + (sc * ca + sa * (sb * cc)) * v0)
                         + -(cb * sa) * v2) + o->pos[1]);
            z = (float)((((k * ca + cc * sa) * v1 + (sc * sa - ca * (sb * cc)) * v0)
                         + (cb * ca) * v2) + o->pos[2]);
            amb_object(&g_obj[i], aud, x, y, z, cam, look);
        } else if (g_amb[i].kind == 1) {
            int s = near7[g_amb[i].signal - 6];
            if (s == -1)
                amb_object(&g_obj[i], 0, 0, 0, 0, cam, look);
            else
                amb_object(&g_obj[i], 1, g_land.sec[s].pos[0], g_land.sec[s].pos[1],
                           g_land.sec[s].pos[2], cam, look);
        } else if (g_amb[i].kind == 2)
            amb_object(&g_obj[i], 1, 0, 0, 0, cam, look);
    }
}

/* 0x405f30's ambience part */
static void ambience(const float cam[3], const float look[3])
{
    int i;
    ambience_place(cam, look);
    if (g_amb_detail != g_detail) {
        for (i = 0; i < g_namb; i++)
            if (g_amb[i].kind == 0) {
                if (g_detail == 0)
                    amb_stop(&g_obj[i]);
                else if (g_amb_detail == 0)
                    rearm(&g_obj[i]);
            }
        g_amb_detail = g_detail;
    }
}

/* ------------------------------------------------------------------ the calls */

static unsigned g_comm_clock;   /* 0x54299c */

/* the engines and the shouts from nothing (0x4084e0, and 0x408900, 0x408b30 switching on) */
static void engines_afresh(void)
{
    int i;
    g_last_followed = 0;
    g_slide_on = g_air_on = g_flat_on = 0;
    g_flat_said = 0;
    g_comm_clock = g_now + 36000;
    g_slide_last = g_now;
    g_slide_said = 0;
    g_air_said = 0;
    reset(&g_own);
    for (i = 0; i < 4; i++) {
        g_slot[i].used = 0;
        reset(&g_near[i]);
        g_near[i].detune = i * 100;
    }
}

/* 0x408900, 0x408800 and 0x408b30 ask for the server first: no sound, this says so */
static int no_sound(void)
{
    if (sound_running())
        return 0;
    hud_sound_notice(exe_text(134), 1);
    return 1;
}

void race_sound_toggle_engine(void)
{
    int i;
    if (no_sound())
        return;
    g_engine_on ^= 1;
    if (g_engine_on)
        engines_afresh();
    else {                      /* 0x406190 */
        stop_all(&g_own);
        for (i = 0; i < 4; i++)
            stop_all(&g_near[i]);
    }
}

void race_sound_toggle_ambience(void)
{
    int i;
    if (no_sound())
        return;
    g_ambience_on ^= 1;
    for (i = 0; i < g_namb; i++)
        if (g_ambience_on)
            rearm(&g_obj[i]);
        else
            amb_stop(&g_obj[i]);
}

/* in a race: the engines and the ambience go with it */
void race_sound_toggle_effects(void)
{
    int i;
    if (no_sound())
        return;
    sound_toggle_effects();
    g_engine_on = g_ambience_on = sound_effects_on();
    if (g_engine_on) {
        engines_afresh();
        for (i = 0; i < g_namb; i++)
            rearm(&g_obj[i]);
        sound_set_last_voice(-1);
        sound_set_last_effect(-1);
    }
    hud_sound_notice(exe_text(sound_effects_on() ? 120 : 121), 0);
}

void race_sound_toggle_voices(void)
{
    if (no_sound())
        return;
    sound_toggle_voices();
    hud_sound_notice(exe_text(sound_voices_on() ? 122 : 123), 0);
}

void race_sound_toggle_music(void)
{
    int fx, voices, music;
    if (!sound_running()) {
        hud_sound_notice(exe_text(135), 1);
        return;
    }
    music_toggle();
    sound_get_switches(&fx, &voices, &music);
    hud_sound_notice(exe_text(music ? 124 : 125), 0);
}

int race_sound_engine_on(void)
{
    return g_engine_on;
}

int race_sound_ambience_on(void)
{
    return g_ambience_on;
}

int race_sound_music_on(void)
{
    int fx, voices, music;
    sound_get_switches(&fx, &voices, &music);
    return music;
}

void race_sound_start(const float cam[3], int player)
{
    int i;
    g_player = player;
    sound_queue_reset();        /* 0x4086b0..0x4086e7 */
    engines_afresh();
    memcpy(g_cam_prev, cam, sizeof g_cam_prev);
    /* (0x405ea0 runs at the track's load; the port reads Sound.Spt again at each start) */
    ambience_parse();
    ambience_load();
    for (i = 0; i < g_namb; i++)
        rearm(&g_obj[i]);
    g_cam_sec = land_section(cam, -1);
    sound_set_last_voice(-1);
    sound_set_last_effect(-1);
    if (sound_effects_on() && !g_engine_on && !g_ambience_on)
        g_engine_on = g_ambience_on = 1;
}

/* 0x407420, what the commentator says of the player's place: looked at once a minute, the first
   time 96 s after the start. (The ids are 0x4db988's and 0x4db990's.) */
void race_sound_commentary(int place, int count)
{
    static const int leading[2] = { 0x41, 0x40 };       /* stillrun, running4 */
    static const int behind[3] = { 0x38, 0x3c, 0x3f };  /* fuel, onegear, runlast */
    int left = count - place;
    if (!sound_voices_on() || g_comm_clock + 60000 >= g_now)
        return;
    g_comm_clock = g_now;
    if (count < 2)
        return;
    if (place == 0)
        sound_queue_voice(leading[RAND() & 1], 500);
    else if (left == 1) {
        if (!(RAND() & 1) && sound_last_voice() != 0x49) {
            sound_set_last_voice(0x49);                 /* yurdalst */
            sound_volume(sound_server_play(0x49, 0), 0xff);
        }
    } else if (left == 2 || (count > 3 && left == 3))
        sound_queue_voice(behind[RAND() % 3], 500);
}

void race_sound_step(BikePhys *const *all, int followed, const float cam[3],
                     const float look[3], float dt)
{
    if (g_engine_on && sound_effects_on())
        engines(all, followed, cam, look, dt, g_bikeworld.replay);
    memcpy(g_cam_prev, cam, sizeof g_cam_prev);
    if (sound_effects_on() && g_ambience_on)    /* 0x405f30 (its queues: sound_update) */
        ambience(cam, look);
    /* the track's +0x20 is the drawing's: the next step hears from this picture's */
    g_cam_sec = land_section(cam, g_cam_sec);
}

/* ------------------------------------------------------------------ the podium */

/* its four objects (0x540080..0x540140, NOTES §4.10.6.2): podium1 and, 7 s on, podium2, each
   twice for a chorus, with the same state machine, nowhere in particular */
static SoundDesc g_pod_desc[4];
static AmbObj g_pod[4];

void podium_sound_start(void)
{
    int i, k;
    for (i = 0; i < 4; i++) {
        for (k = 0; k < (int)(sizeof g_pod_desc[0] / 4); k++)
            ((uint32_t *)&g_pod_desc[i])[k] = exe_u32(exe_addr(X_PODIUM_SOUNDS) + 0x50 * i + 4 * k);
        memset(&g_pod[i], 0, sizeof g_pod[i]);
        g_pod[i].d = &g_pod_desc[i];
        rearm(&g_pod[i]);
    }
}

void podium_sound_frame(void)
{
    static const float zero[3];
    int i;
    for (i = 0; i < 4; i++)
        amb_object(&g_pod[i], 1, 0.0f, 0.0f, 0.0f, zero, zero);
}

void podium_sound_stop(void)    /* 0x405000 */
{
    int i;
    for (i = 0; i < 4; i++)
        if (!g_pod[i].stopped)
            amb_stop(&g_pod[i]);
}
