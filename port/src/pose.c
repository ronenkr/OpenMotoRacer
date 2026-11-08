/* The rider's body in the race (NOTES §4.18 "8.16"): 0x45b340 reads the family's poses,
   0x45b4e0 blends two of them, 0x467b30 picks the detail and 0x45d810 builds the body of a
   step: sitting up at a walk, the crouch with the speed, the foot down at the start, standing
   and sitting on the jumps (dirt), the look at a passing rival, the lean, the stunts (dirt,
   the local player), the salute of the winner's lap, the gestures past the line, the fall.
   Only the drawing reads it. */
#include "pose.h"
#include "bike3d.h"
#include "bikephys.h"
#include "bikes.h"
#include "camera.h"
#include "choices.h"
#include "files.h"
#include "race.h"
#include "rand.h"
#include "rules.h"
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fastfloat.h"      /* (last: the ESP32's single precision) */

#define PI 3.141592653589793
#define TWO_PI 6.283185307179586

/* the poses' places in the family's list */
enum {
    ROT_STATIC = 0, ROT_LEFT = 1, ROT_RIGHT = 2, ROT_UP = 3, ROT_FOOT = 4, ROT_LEFT_LOW = 5,
    ROT_RIGHT_LOW = 6, ROT_STAND = 7, ROT_WIN = 8, ROT_SIT = 9, ROT_SECOND = 10, ROT_THIRD = 11,
    ROT_NEXT_LAST = 12, ROT_LAST = 13, ROT_OTHER = 14, ROT_FALL = 15, ROT_FALL_END = 16,
    ROT_LOOK_LEFT = 29, ROT_LOOK_RIGHT = 30, ROT_GO = 31, ROT_GO_2 = 32
};

static Pose g_rot[FAMILY_ROTS];
static float g_salute;          /* 0x5dabd0 */

static int32_t bits(float f)
{
    int32_t i;
    memcpy(&i, &f, 4);
    return i;
}

/* 0x45b340 + 0x45b480: "ROT", three numbers passed over, then 3 angles a bone slot */
static int rot_load(Pose *p, const char *path)
{
    long len;
    unsigned char *d = dsi_load(path, &len);
    int i, k;
    p->n = 0;
    if (!d || len < 10 || memcmp(d, "ROT", 3)) {
        fprintf(stderr, "openmotoracer: the pose %s could not be read\n", path);
        free(d);
        return 0;
    }
    p->n = (int)((len - 10) / 6);
    if (p->n > POSE_BONES)
        p->n = POSE_BONES;
    for (i = 0; i < p->n; i++)
        for (k = 0; k < 3; k++) {
            const unsigned char *q = d + 10 + 6 * i + 2 * k;
            p->a[i][k] = (float)((int16_t)(q[0] | q[1] << 8) * 0.0015339807878856412);  /* 0x4d0e68 */
        }
    free(d);
    return 1;
}

void pose_free(void)
{
    memset(g_rot, 0, sizeof g_rot);
}

int pose_load(void)
{
    int k, still = 0;
    pose_free();
    for (k = 0; k < g_fam.nrot; k++) {
        int ok = rot_load(&g_rot[k], g_fam.rot[k]);
        if (k == ROT_FOOT)
            still = ok;
    }
    return still;
}

const Pose *pose_rot(int k)
{
    return &g_rot[k];
}

/* one angle, the short way round, brought back into 0..2pi */
static float mix1(float a, float b, float t)
{
    float v;
    if (a + (float)PI < b)
        b -= (float)TWO_PI;
    if (b < a - (float)PI)
        b += (float)TWO_PI;
    v = (b - a) * t + a;
    while (v < 0.0f)
        v += (float)TWO_PI;
    while ((float)TWO_PI < v)
        v -= (float)TWO_PI;
    return v;
}

void pose_mix(Pose *o, const Pose *a, const Pose *b, float t)
{
    int n = a->n < b->n ? a->n : b->n, i, k;
    o->n = n;
    if (bits(t) < 1) {
        if (o != a)
            memmove(o->a, a->a, sizeof o->a[0] * (size_t)n);
    } else if (bits(t) >= 0x3f800000) {
        if (o != b)
            memmove(o->a, b->a, sizeof o->a[0] * (size_t)n);
    } else
        for (i = 0; i < n; i++)
            for (k = 0; k < 3; k++)
                o->a[i][k] = mix1(a->a[i][k], b->a[i][k], t);
}

void pose_restart(void)
{
    g_salute = 200.0f;
}

void pose_start(BikePhys *b)
{
    b->pose_prev = ROT_FOOT;
}

static float clamp01(float v)
{
    if (bits(v) < 1)
        return 0.0f;
    return bits(v) > 0x3f7fffff ? 1.0f : v;
}

static float clamp11(float v)
{
    if (v < -1.0f)
        return -1.0f;
    return bits(v) > 0x3f800000 ? 1.0f : v;
}

static int loaded_riders(void)  /* 0x519fe4 = 0x4dc100, at a finish */
{
    int k, n = 0;
    for (k = 1; k < g_riders; k++)
        n += g_racer[k].loaded != 0;
    return n;
}

/* the top half of the body by the speed and what the rider has done (0x45d810 up to 0x45db88) */
static void by_speed(BikePhys *b, Pose *p, const Racer *r, int dirt)
{
    float top = dirt ? 60.0f : 120.0f;  /* km/h: 0x78 / 2, 0xf0 / 2 */
    float t = b->speed;
    if (t < (float)5.555555555555555) {
        const Pose *from = &g_rot[b->pose_prev], *to = &g_rot[ROT_UP];
        t = (float)(t * 0.18);
        if (b->pose_prev == ROT_GO) {   /* the first move off the start's pose, in two parts */
            int i;
            t = clamp01(t) * 2.0f;
            i = (int)t;
            if (i > 0)
                i = 1;
            t -= (float)i;
            if (i > 0)
                from = &g_rot[ROT_GO_2];
            to = i > 0 ? &g_rot[ROT_UP] : &g_rot[ROT_GO_2];
        }
        pose_mix(p, from, to, t);
        return;
    }
    b->pose_prev = ROT_FOOT;
    if (r->place == 0 && r->state == RS_FINISHED && g_winner_lap) {
        if (bits(g_salute) >= 1) {
            if (bits(g_salute) > 0x432f0000)            /* 175 < T: the arm goes up */
                pose_mix(p, &g_rot[ROT_WIN], &g_rot[ROT_UP], (g_salute - 175.0f) * 0.04f);
            else if (bits(g_salute) > 0x41c80000)       /* 25 < T: and stays */
                pose_mix(p, &g_rot[ROT_WIN], &g_rot[ROT_WIN], 0.0f);
            else                                        /* and comes down */
                pose_mix(p, &g_rot[ROT_STATIC], &g_rot[ROT_WIN], g_salute * 0.04f);
            b->detail = 1;
            return;
        }
        g_salute = 0.0f;
    } else if (b->driven && r->state == RS_FINISHED && bits(b->speed) >= 0x40a00000) {
        const Pose *g;          /* past the line: the gesture of the place */
        int count = loaded_riders();
        if (r->place == 0)
            g = &g_rot[ROT_WIN];
        else if (r->place - count == -1)
            g = &g_rot[ROT_LAST];
        else if (count - r->place == 2)
            g = &g_rot[ROT_NEXT_LAST];
        else if (r->place == 1)
            g = &g_rot[ROT_SECOND];
        else if (r->place == 2)
            g = &g_rot[ROT_THIRD];
        else
            g = &g_rot[ROT_OTHER];
        pose_mix(p, &g_rot[ROT_UP], g, (b->speed - 5.0f) * 0.1f);
        b->detail = 1;
        return;
    }
    /* up at 20 km/h, the full crouch at the top: 60 km/h on dirt, 120 on the road */
    t = (float)((b->speed - 5.555555555555555)
                / (float)(top * 0.2777777777777778 - 5.555555555555555));
    pose_mix(p, &g_rot[ROT_UP], &g_rot[ROT_STATIC], t);
}

/* a look over the shoulder at a rival coming past faster, within 512 units */
static void look(BikePhys *b, Pose *q)
{
    const BikePhys *near = 0;
    float best = FLT_MAX;
    int k;
    if (b->look) {
        int dt = (int)(g_bikeworld.clock - b->look_at);
        pose_mix(q, q, &g_rot[b->look], (float)sin((double)dt / b->look_len * PI));
        if (b->look_len < (float)dt)
            b->look = 0;
        return;
    }
    for (k = 1; k < g_riders; k++) {
        const BikePhys *o = g_bikeworld.rider[k];
        float dx, dy, dz, d;
        if (!o || o == b || !g_racer[k].loaded)
            continue;
        dx = o->pos[0] - b->pos[0], dy = o->pos[1] - b->pos[1], dz = o->pos[2] - b->pos[2];
        d = dx * dx + dy * dy + dz * dz;
        if (d < best)
            best = d, near = o;
    }
    if (near && b->ground && b->rival == near->number && bits(best) < 0x48800000
        && 20.0f < (float)(near->kmh - b->kmh)) {
        float ux = b->front[0] - b->rear[0], uz = b->front[2] - b->rear[2];
        float vx = near->rear[0] - b->rear[0], vz = near->rear[2] - b->rear[2];
        float l = 1.0f / (float)sqrt(ux * ux + uz * uz), dot;
        ux *= l, uz *= l;
        l = 1.0f / (float)sqrt(vz * vz + vx * vx);
        vx *= l, vz *= l;
        dot = uz * vz + vx * ux;
        if (b->look_dot < dot && fabsf(dot) < 0.7f) {
            b->look = bits(vx * uz - vz * ux) > 0 ? ROT_LOOK_RIGHT : ROT_LOOK_LEFT;
            b->look_len = 400.0f;
        }
        b->look_at = g_bikeworld.clock;
        b->look_dot = dot;
    }
    b->rival = near ? near->number : 0;
}

/* a jump's stunt: dirt, the local player, the turbo key newly pressed high in the air */
static void stunt(BikePhys *b, Pose *q)
{
    const FamilyStunt *s;
    int dt, n, i;
    float u;
    if (!b->ground && g_fam.nstunts > 0 && (float)18.055555555555554 < b->speed
        && (float)((double)b->rear[1] - b->rear_h) >= 64.0f
        && b->turbo_pressed && !b->turbo_pressed1 && !b->stunt_on) {
        b->stunt_k = g_fam.stunt_list[b->stunt_next];
        b->stunt_on = 1;
        b->stunt_at = g_bikeworld.clock;
        /* the game draws stream 5 and keeps the clock's (0x45e087); the CD keeps the draw
           (/0x47f7d9). Nothing else draws stream 5, so GOG's undrawn one changes nothing */
        b->stunt_next = g_phys_cd ? rand_next(5) % g_fam.nstunts
                                  : (int)(g_bikeworld.clock % (unsigned)g_fam.nstunts);
    }
    if (!b->stunt_on)
        return;
    s = &g_fam.stunt[b->stunt_k];
    dt = (int)(g_bikeworld.clock - b->stunt_at);
    n = s->frames;
    u = clamp01((float)(sin((double)dt / s->ms * PI) * 1.2));
    i = (int)((float)n * u);
    if (i > n - 1)
        i = n - 1;
    pose_mix(q, i > 0 ? &g_rot[b->stunt_k + i - 1] : q, &g_rot[b->stunt_k + i],
             (float)n * u - (float)i);
    if (s->ms < dt)
        b->stunt_on = 0;
    b->detail = s->hand;
}

/* 0x45d810 */
static void body(BikePhys *b)
{
    const Racer *r = &g_racer[b->number];
    int dirt = g_bikes[b->bike].perso != 0;
    Pose p, *q = &b->pose;
    if (dirt && g_bikeworld.countdown) {
        pose_mix(q, &g_rot[ROT_FOOT], &g_rot[ROT_GO], (float)(2000 - g_time_left) * 0.001f);
        b->pose_prev = ROT_GO;
    } else {
        float s = 0.0f;
        const Pose *a = &p;
        by_speed(b, &p, r, dirt);
        if (dirt && (bits(b->wheelie) & 0x7fffffff) == 0) {
            /* the wheels' heights: standing up on a rise, sitting back on a drop */
            float x = (float)(((double)(b->front[1] - b->rear[1])
                               / ((double)g_bikes[b->bike].phys[(0x38 - 0x14) / 4] * 48.0f)) * 4.0f);
            x = clamp11(x);
            b->stand = (x - b->stand) * 0.25f + b->stand;
            s = b->stand;
            a = &g_rot[ROT_STAND];
            if (bits(s) < 1) {
                s = -s;
                a = &g_rot[ROT_SIT];
            }
        }
        pose_mix(q, &p, a, s);
        if (40.0f < (float)b->kmh)
            look(b, q);
        if (bits(b->lean) & 0x7fffffff) {
            const Pose *low, *high;
            Pose t;
            float f;
            if (bits(b->lean) > 0) {
                low = &g_rot[ROT_RIGHT_LOW], high = &g_rot[ROT_RIGHT], f = b->lean;
            } else {
                low = &g_rot[ROT_LEFT_LOW], high = &g_rot[ROT_LEFT], f = -b->lean;
            }
            if (!dirt) {
                pose_mix(&t, low, high, (float)((b->speed - 8.333333333333334) * 0.12));
                if (g_phys_cd)          /* /0x47f6b7, /0x47f6f2 */
                    f *= b->speed < (float)16.666666666666668 ? 4.5454545f : 2.27272725f;
                else
                    f *= b->speed < (float)16.666666666666668 ? 4.3478260040283f : 2.1739130020142f;
            } else if (g_phys_cd) {     /* /0x47f6c3 */
                f *= 2.0f;
                pose_mix(&t, high, low, (f - 0.439999998f) * 1.13636363f);
                f *= 2.27272725f;
            } else {
                f *= 2.0f;
                pose_mix(&t, high, low, (f - 0.46000000834465f) * 1.0869565010071f);
                f *= 2.1739130020142f;
            }
            pose_mix(q, q, &t, f);
        }
        if (dirt && b->number == race_local())
            stunt(b, q);
    }
    if (!b->crashed) {
        if (bits(b->dist) > 0x44610000 && bits(b->dist) < 0x447a0000)   /* back to the still pose, far off */
            pose_mix(q, q, &g_rot[ROT_STATIC], (b->dist - 900.0f) * 0.01f);
    } else if (bits(b->crash_timer) < 0x43c80000) {
        float s0;
        pose_mix(q, q, &g_rot[ROT_FALL], clamp11((400.0f - b->crash_timer) * 0.02f));
        s0 = (float)(b->tumble_speed0 * 0.75);
        if (b->tumble_speed < s0)
            pose_mix(q, q, &g_rot[ROT_FALL_END],
                     (float)-sin((double)clamp11((s0 - b->tumble_speed) / s0 * 3.0f)
                                 * 1.5707963267948966 + PI));
        b->detail = 1;
    }
    if (b->detail)
        b->lod = 1;
}

/* 0x45f160: the detail by the distance over the zoom */
static int detail(BikePhys *b)
{
    const float *eye = camera_eye();
    float x = b->pos1[0] - eye[0], y = b->pos1[1] - eye[1], z = b->pos1[2] - eye[2];
    b->dist = (float)(sqrt(x * x + z * z + y * y) / camera_zoom());
    return b->dist >= 3000.0f ? 4 : b->dist >= 2500.0f ? 3 : b->dist < 1000.0f ? 0 : 2;
}

void pose_step(BikePhys *b)
{
    const Racer *r = &g_racer[b->number];
    if (r->place == 0 && r->state == RS_FINISHED && g_winner_lap) {
        if (bits(g_salute) < 1)
            g_salute = 0.0f;
        else
            g_salute -= g_bikeworld.dt;
    }
    b->lod = b->crashed || b->detail ? 1 : detail(b);
    if (b->lod < 2)
        body(b);
    if (b->detail)
        b->lod = 1;
}
