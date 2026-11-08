/* The computer riders: their step 0x46bf80, the driving 0x46b120, the lanes 0x46cd20 and
   their speeds at the start 0x47bc60. The map is build/notes/cpu_riders.md (NOTES §4.21).
   Arithmetic as the game's x87 at 53 bits, as in bikephys.c: R (a double) is a value in a
   register, a store to a float is the game's store, the sums in the game's order. */
#include "cpu.h"
#include "bikephys_int.h"
#include "land.h"
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef OMR_FAST_PHYS
/* the ESP32 (NOTES §4.26): the in-betweens in single precision, which its FPU does (doubles are
   software there, 13 ms a step for the 24 bikes); with -fsingle-precision-constant and tgmath's
   float functions. Not the original's x87 to the bit any more: the riding the same but for
   roundings, so a PC's replay or ghost may part from it in time. */
typedef float R;
#include <tgmath.h>
#else
typedef double R;
#endif

#define W g_bikeworld
#define PI 3.141592653589793            /* 0x4d0e60 */
#define TWO_PI 6.283185307179586        /* 0x4d0e58 */
#define QPI 0.7853981633974483          /* 0x4d12c8, -0x4d1218 */
#define QPI_F 0.785398185253143         /* 0x3f490fdb */
#define KMH_MS 0.2777777777777778       /* 0x4d0f70, 0x4d1938 */

/* the carac row's fields this file reads */
enum { C_AXE = 0x38, C_RAV = 0x3c, C_RAR = 0x40, C_HAUT = 0x58 };

static int32_t bits(float f)
{
    int32_t i;
    memcpy(&i, &f, 4);
    return i;
}

static uint32_t ubits(float f)
{
    uint32_t i;
    memcpy(&i, &f, 4);
    return i;
}

#define NEG(f) (ubits(f) > 0x80000000u)         /* negative, -0.0 not */
#define ZERO(f) ((ubits(f) & 0x7fffffffu) == 0)

static int dirt_rider(const BikePhys *b)       /* C+0x120 (0x509398) */
{
    return bp_perso(b) != 0;
}

/* the lane's speed limit: top x (1 + lane / 40), kept as a double (0x46b181, 0x46b7f1) */
static R lane_top(const BikePhys *b)
{
    return (R)b->lane * b->top * 0.025 + b->top;
}

/* ------------------------------------------------------------------ the lanes (0x46cd20) */

/* 0x46c970: is section o within [mine - behind, mine + ahead), around the track */
static int near(int o, int mine, int ahead, int behind)
{
    int n = bp_nsec(), lo, hi;
    if (n == 0)
        return 0;
    lo = bp_wrap(mine - behind);
    hi = bp_wrap(ahead + mine);
    if (hi < lo)
        return (lo <= o && o < n) || (o >= 0 && o < hi);
    return lo <= o && o < hi;
}

/* 0x46c9f0: how many sections from a forward to b */
static int ahead_of(int a, int b)
{
    return a <= b ? b - a : b - a + bp_nsec();
}

static const BikePhys *other(const BikePhys *b, int i)
{
    const BikePhys *o = i > 0 && i < 25 ? W.rider[i] : 0;
    return o && o != b && b->number != i ? o : 0;
}

/* the inline speed comparison of 0x46cd20: 1 when the other's shown speed is above mine,
   -1 when mine is above, 0 (km/h x 0.2777: the other's a float, mine a double, then a
   float) */
static int compare(const BikePhys *b, const BikePhys *o)
{
    float a = (float)((R)o->kmh_shown * KMH_MS);
    R m = (R)b->kmh_shown * KMH_MS;
    float mf = (float)m;
    if (m < a)
        return 1;
    return a < mf ? -1 : 0;
}

/* the nearest rider in lane `lane` within [sec - behind, sec + ahead) (0x46ca10, and the
   loop of 0x46cd20): the smallest forward distance, the first one on a tie; -1 none */
static int nearest(const BikePhys *b, int lane, int ahead, int behind)
{
    int i, best = 0x7ffffffe, who = -1;
    for (i = 1; i < W.nriders; i++) {
        const BikePhys *o = other(b, i);
        int d;
        if (!o || o->lane != lane || !near(o->sec, b->sec, ahead, behind))
            continue;
        d = ahead_of(b->sec, o->sec);   /* ftol(|(float)d|): d itself */
        if (d < best)
            best = d, who = i;
    }
    return who;
}

/* 0x46cb40: the lane whose nearest rider ahead is the fastest (km/h x 0.2777, the best
   kept truncated); *who = that rider; -1 when no lane has one */
static int best_lane(const BikePhys *b, int ahead, int behind, int *who)
{
    int k, best = 0, lane = -1;
    for (k = 0; k < 3; k++) {
        int r = nearest(b, k, ahead, behind);
        if (r >= 0) {
            R s = (R)W.rider[r]->kmh_shown * KMH_MS;
            float sf = (float)s;
            if ((R)best < sf) {         /* the int, then its float, against sf */
                best = (int)s;
                lane = k;
                *who = r;
            }
        }
    }
    return lane;
}

/* 0x46cd20: the lane: back towards lane 0 when it is free, out of the way to pass a slower
   rider ahead, or else slow down behind him; then lane_x slides to lane x 240 */
static void lanes(BikePhys *b, float dt)
{
    int rev = W.reverse < 0;            /* 0x4dc144 */
    int a = rev ? 12 : 8, bb = rev ? 8 : 12, c = rev ? 0 : 12, d = rev ? 12 : 0;
    int i, k, k2, bi, follow;
    if (b->lane > 0) {
        for (i = 1; i < W.nriders; i++) {
            const BikePhys *o = other(b, i);
            if (o && o->lane == b->lane - 1 && near(o->sec, b->sec, bb / 2, a / 2))
                break;
        }
        if (i >= W.nriders) {
            b->lane--;
            goto slide;
        }
    }
    bi = nearest(b, b->lane, c, d);     /* the rider ahead in my lane */
    if (bi < 0 || compare(b, W.rider[bi]) >= 0)
        goto slide;                     /* nobody, or not slower than me */
    for (k = 0; k < 3; k++) {           /* the first lane free 12 sections either way */
        for (i = 1; i < W.nriders; i++) {
            const BikePhys *o = other(b, i);
            if (o && o->lane == k && near(o->sec, b->sec, 12, 12))
                break;
        }
        if (i >= W.nriders)
            break;
    }
    if (k < 3 && abs(k - b->lane) < 2) {
        b->lane = k;
        goto slide;
    }
    follow = bi;
    k2 = best_lane(b, c, d, &follow);
    if (k2 == b->lane || k2 < 0 || abs(k2 - b->lane) >= 2)
        follow = bi;
    else {
        int r = nearest(b, b->lane, d, c / 2);      /* 0x46ca10: one close behind */
        if (r >= 0 && compare(b, W.rider[r]) > 0)
            follow = bi;
        else if (ahead_of(b->sec, W.rider[follow]->sec) * W.reverse > 6)
            b->lane = k2;
        else
            follow = bi;
    }
    if (compare(b, W.rider[follow]) < 0) {         /* faster than him: ease off */
        float t = (float)((R)W.rider[follow]->kmh_shown * KMH_MS), e0 = b->engine, diff;
        R e;
        b->engine = (float)(((R)t - b->engine) * 0.03125f + b->engine);
        b->cpu_speed = b->engine;
        e = (R)b->engine * 0.95;
        b->engine = b->cpu_speed = (float)e;
        diff = (float)((R)e0 - e);
        if (fabs((R)diff) > 11.11111111111111)
            b->keys1 = 2;               /* braking (the wheels stop turning) */
    }
slide:
    if (b->ground) {
        float x = (float)(b->lane * 240);
        if (x > b->lane_x) {
            b->lane_x = (float)(dt * 1.0666666666666667 + b->lane_x);
            if (b->lane_x > x)
                b->lane_x = x;
        } else if (b->lane_x > x) {
            b->lane_x = (float)((R)b->lane_x - dt * 1.0666666666666667);
            if (b->lane_x < x)
                b->lane_x = x;
        }
    }
}

/* ------------------------------------------------------------------ the driving (0x46b120) */

/* the [0, 2pi) wrap of 0x46b3d6: the first test on the value before its store */
static float wrap_unrounded(R v)
{
    float a = (float)v;
    R x;
    if (v < 0.0) {
        x = a;
        do
            x += TWO_PI;
        while (x < 0.0);
        a = (float)x;
    }
    if (a >= TWO_PI) {
        x = a;
        do
            x -= TWO_PI;
        while (x >= TWO_PI);
        a = (float)x;
    }
    return a;
}

/* 0x40fe80 on a matrix with T = 0: its transpose (T = -0) */
static void transpose(float *o, const float *m)
{
    int i, k;
    for (i = 0; i < 3; i++)
        for (k = 0; k < 3; k++)
            o[3 * i + k] = m[3 * k + i];
    o[9] = o[10] = o[11] = -0.0f;
}

static void drive(BikePhys *b, float dt)
{
    int dirt = dirt_rider(b), s, i;
    float x, T[3], P[3], D[3], k;
    const float *M;
    b->keys1 = 1;                       /* accelerating */
    if (!b->crashed) {
        if (!W.countdown && (b->ground || !dirt)) {
            R lim;
            b->cpu_speed = (float)(dt * 0.16666666666666669 + b->cpu_speed);
            lim = lane_top(b);
            if (lim < b->cpu_speed)
                b->cpu_speed = (float)lim;
        }
        lanes(b, dt);
    }

    /* the target: on the racing line + lane_x, 16 sections ahead (a dirt bike 9) */
    s = bp_wrap(b->sec + W.reverse * (dirt ? 9 : 16));
    M = bp_sec_m(s);
    x = (float)((R)bp_sec_line(s) + b->lane_x);
    {
        float v[3] = { x, 0.0f, 0.0f }, w[3];
        const float *p = bp_sec_pos(s);
        bp_mulvec(w, v, M);
        for (i = 0; i < 3; i++)
            T[i] = (float)((R)p[i] + w[i]);
    }
    if (!b->crashed) {                  /* turn a quarter of the way towards it */
        float L[12], Li[12], A, B, d, yaw;
        R dA;
        bp_look(L, b->pos, T);
        transpose(Li, L);
        A = (float)-bp_heading(b->m);
        B = (float)bp_heading(Li);
        A = bp_wrap_ge(A);
        B = bp_wrap_ge(B);
        dA = A;
        if (dA + PI < B)
            B = (float)((R)B - TWO_PI);
        if (dA - PI > B)
            B = (float)((R)B + TWO_PI);
        d = wrap_unrounded(((R)B - A) * 0.25f);
        yaw = d;
        if (d > PI)
            yaw = (float)((R)d - TWO_PI);
        if (dirt) {                     /* the handlebar shown */
            R st = (R)yaw * 12.0f;
            float f = (float)st;
            if (st < -QPI)
                f = (float)-QPI_F;
            if (f > QPI)
                f = (float)QPI_F;
            b->steer = (float)((R)f * 1303.7972938088067);
        } else
            b->steer = 0.0f;
        bp_turn_yaw(b, yaw);
    }

    /* where it points; the motion turned round if it runs against that */
    for (i = 0; i < 3; i++)
        b->fwd[i] = (float)((R)b->m[9 + i] + b->m[6 + i]);
    bp_normalise(b->fwd);
    if (((R)b->fwd[2] * b->vdir[2] + (R)b->fwd[1] * b->vdir[1]) + (R)b->vdir[0] * b->fwd[0]
        < 0.0) {
        for (i = 0; i < 3; i++)
            b->vdir[i] = -b->vdir[i];
        b->engine = -b->engine;
    }

    /* the near point: on the line + lane_x, 4 sections ahead (a dirt bike 2) */
    s = bp_wrap(b->sec + W.reverse * (dirt ? 2 : 4));
    M = bp_sec_m(s);
    {
        const float *p = bp_sec_pos(s);
        R xd = (R)bp_sec_line(s) + b->lane_x;
        float xf = (float)xd;
        P[0] = (float)((xd * M[0] + M[9]) + p[0]);
        P[1] = (float)(((R)M[1] * xf + M[10]) + p[1]);
        P[2] = (float)(((R)M[2] * xf + M[11]) + p[2]);
    }

    /* the speed: back to 0 by 1/18 m/s a dt unit after a hit or in a crash */
    if ((ZERO(b->hit_timer) && !ZERO(b->engine)) || b->crashed) {
        if (bits(b->engine) > 0) {
            b->engine = (float)(dt * -0.05555555555555555 + b->engine);
            if (NEG(b->engine))
                b->engine = 0.0f;
        } else {
            b->engine = (float)(dt * 0.05555555555555555 + b->engine);
            if (bits(b->engine) > 0)
                b->engine = 0.0f;
        }
    }
    {
        R kk = (R)b->hit_timer * 0.004999999888241291;     /* 0x4d100c, a float */
        k = kk < 0.01 ? 0.01f : (float)kk;
    }
    if (NEG(b->engine))
        b->cpu_speed = 0.0f;
    else if (b->ground || !dirt) {
        /* the speed of the mix of the motion (at engine) and the heading (at cpu_speed),
           weighed by how far they agree, at most the lane's top; then followed at 1 - k */
        float c = (float)(((R)b->fwd[2] * b->vdir[2] + (R)b->fwd[1] * b->vdir[1])
                          + (R)b->vdir[0] * b->fwd[0]), Xf, Yf, Zf, v1, len;
        R Pm, Qe, X, Y, Z, lim;
        c = fabsf(c);
        if (bits(c) < 0x3f000000)
            c = 0.5f;
        Pm = (R)b->cpu_speed * c;
        Qe = (1.0f - (R)c) * b->engine;
        X = (R)b->vdir[0] * Qe + b->fwd[0] * Pm;
        v1 = (float)((R)b->vdir[1] * Qe);
        Y = b->fwd[1] * Pm + v1;
        Z = Qe * b->vdir[2] + Pm * b->fwd[2];
        Xf = (float)X, Yf = (float)Y, Zf = (float)Z;
        len = (float)sqrt((Z * Zf + (R)Yf * Yf) + (R)Xf * Xf);
        lim = lane_top(b);
        if (lim < len)
            len = (float)lim;
        b->engine = (float)((1.0f - (R)k) * len + (R)b->engine * k);
    }

    /* towards the near point */
    D[0] = (float)((R)P[0] - b->pos[0]);
    D[1] = (float)((R)P[1] - b->pos[1]);
    D[2] = (float)((R)P[2] - b->pos[2]);
    {
        R l = sqrt(((R)D[1] * D[1] + (R)D[2] * D[2]) + (R)D[0] * D[0]);
        if (l != 0.0) {
            R r = 1.0 / (float)l;
            D[0] = (float)(D[0] * r), D[1] = (float)(D[1] * r), D[2] = (float)(r * D[2]);
        }
    }
    b->sec = bp_walk(b->sec, b->pos);
    if (!b->ground && dirt) {           /* a dirt bike in the air: a ballistic flight */
        float e = b->engine, L;
        static const float up[3] = { 0.0f, 1.0f, 0.0f };
        float X[3], Y[3], Z[3];
        b->vdir[0] = (float)((R)b->vdir[0] * e);
        b->vdir[1] = (float)((R)b->vdir[1] * e);
        b->vdir[2] = (float)((R)e * b->vdir[2]);
        b->vdir[1] = (float)((R)b->vdir[1] - dt * 0.24525000000000002);
        L = (float)sqrt(((R)b->vdir[1] * b->vdir[1] + (R)b->vdir[0] * b->vdir[0])
                        + (R)b->vdir[2] * b->vdir[2]);
        b->engine = L;
        if (!ZERO(L)) {
            R r = 1.0 / L;
            b->vdir[0] = (float)(b->vdir[0] * r);
            b->vdir[1] = (float)(b->vdir[1] * r);
            b->vdir[2] = (float)(r * b->vdir[2]);
        }
        b->fwd[1] = b->vdir[1];
        {
            R l = sqrt(((R)b->vdir[1] * b->fwd[1] + (R)b->fwd[2] * b->fwd[2])
                       + (R)b->fwd[0] * b->fwd[0]);
            if (l != 0.0) {
                R r = 1.0 / (float)l;
                b->fwd[0] = (float)(b->fwd[0] * r);
                b->fwd[1] = (float)(b->fwd[1] * r);
                b->fwd[2] = (float)(r * b->fwd[2]);
            }
        }
        memcpy(Z, b->fwd, 12);
        bp_normalise(Z);
        bp_cross(X, up, Z);
        bp_cross(Y, Z, X);
        bp_normalise(X);
        bp_normalise(Y);
        memcpy(b->m, X, 12), memcpy(b->m + 3, Y, 12), memcpy(b->m + 6, Z, 12);
        b->m[9] = b->m[10] = b->m[11] = 0.0f;
        if (b->top < b->engine)
            b->engine = b->top;
    } else {                            /* on the road: the motion turns towards the point */
        if (!b->crashed) {
            R w = 1.0f - (R)k;
            for (i = 0; i < 3; i++)
                b->vdir[i] = (float)((R)b->vdir[i] * k + D[i] * w);
        }
        if (dirt)
            b->vdir[1] = b->fwd[1];
        {
            R l = sqrt(((R)b->vdir[1] * b->vdir[1] + (R)b->vdir[0] * b->vdir[0])
                       + (R)b->vdir[2] * b->vdir[2]);
            if (l != 0.0) {
                R r = 1.0 / (float)l;
                b->vdir[0] = (float)(b->vdir[0] * r);
                b->vdir[1] = (float)(b->vdir[1] * r);
                b->vdir[2] = (float)(r * b->vdir[2]);
            }
        }
    }
    if (!W.countdown) {                 /* 128 units a metre, dt in 1/100 s */
        R f = ((R)b->engine * dt) * 1.28;
        for (i = 0; i < 3; i++)
            b->pos[i] = (float)((R)b->vdir[i] * f + b->pos[i]);
    } else
        b->engine = b->cpu_speed = 0.0f;
    b->sec = bp_walk(b->sec, b->pos);
}

/* ------------------------------------------------------------------ the step (0x46bf80) */

/* pos + (0, 0, z) M + T, the first coordinate with z before its store (0x46c2a3) */
static void along(float o[3], const BikePhys *b, R z)
{
    float zf = (float)z;
    o[0] = (float)((z * b->m[6] + b->m[9]) + b->pos[0]);
    o[1] = (float)(((R)b->m[7] * zf + b->m[10]) + b->pos[1]);
    o[2] = (float)(((R)b->m[8] * zf + b->m[11]) + b->pos[2]);
}

/* 0x46bf80 at 0x46c1xx and 0x45e940 at 0x45eb0e, the same code: a dirt bike above the road
   flies; else the bike on the road (a landing, a dirt bike's bumps), its matrix from the road
   under the wheels' axles; then the wheel points */
static void settle(BikePhys *b, int dirt, int bend, int flat)
{
    float road;
    int i;
    road = (float)bp_road_y(b->sec, b->pos);
    if (dirt && (flat ? (R)road + 8.0f < b->pos[1] : (R)road + 32.0f < b->pos[1]))
        b->ground = 0;                  /* a dirt bike above the road flies */
    else {
        static const float up[3] = { 0.0f, 1.0f, 0.0f };
        float rp[3], fp[3], X[3], Y[3], Z[3];
        int drop = 0, sf, sr;
        if (!b->ground1) {              /* landing */
            b->kick = (float)(((R)b->pos1[1] - road) * -2.0f);
            b->hit_timer = 200.0f;
            b->vdir[1] = 0.0f;
            {
                R l = sqrt((R)b->vdir[0] * b->vdir[0] + (R)b->vdir[2] * b->vdir[2]);
                if (l != 0.0) {
                    R r = 1.0 / (float)l;
                    b->vdir[0] = (float)(b->vdir[0] * r);
                    b->vdir[1] = 0.0f;
                    b->vdir[2] = (float)(r * b->vdir[2]);
                }
            }
            b->events |= 0x80;
        }
        if (dirt && !bend) {            /* a dirt bike's bumps on the straights */
            int n = (int)(((R)b->engine / b->top) * 8.0f);
            if (n)
                drop = bikephys_crt_rand() % n;
        }
        b->pos[1] = (float)((R)road - drop);
        b->sec = bp_walk(b->sec, b->pos);
        b->ground = 1;
        b->air = 0.0f;
        b->rear_touch = b->front_touch = 1;
        /* the matrix from the road under the wheels' axles */
        along(rp, b, (R)bp_carac(b, C_AXE) * -48.0f);
        along(fp, b, (R)bp_carac(b, C_AXE) * 48.0f);
        sf = bp_walk(b->sec, fp);
        sr = bp_walk(b->sec, rp);
        fp[1] = (float)bp_road_y(sf, fp);
        rp[1] = (float)bp_road_y(sr, rp);
        for (i = 0; i < 3; i++)
            Z[i] = (float)((R)fp[i] - rp[i]);
        bp_normalise(Z);
        bp_cross(X, up, Z);
        bp_cross(Y, Z, X);
        bp_normalise(X);
        bp_normalise(Y);
        memcpy(b->m, X, 12), memcpy(b->m + 3, Y, 12), memcpy(b->m + 6, Z, 12);
        b->m[9] = b->m[10] = b->m[11] = 0.0f;
    }

    /* the wheel points (0x46c529): (0, 96 x HauteurCadre - 192 x radius, -+48 x axle) */
    {
        R h = (R)bp_carac(b, C_HAUT) * 96.0f;
        float yr = (float)((R)bp_carac(b, C_RAR) * -192.0f + h),
              zr = (float)((R)bp_carac(b, C_AXE) * -48.0f),
              yf = (float)((R)bp_carac(b, C_RAV) * -192.0f + h);
        R zfd = (R)bp_carac(b, C_AXE) * 48.0f;
        float zf = (float)zfd;
        const float *m = b->m;
        for (i = 0; i < 3; i++)
            b->rear[i] = (float)((((R)m[3 + i] * yr + (R)m[6 + i] * zr) + m[9 + i]) + b->pos[i]);
        b->front[0] = (float)((((R)m[3] * yf + zfd * m[6]) + m[9]) + b->pos[0]);
        for (i = 1; i < 3; i++)
            b->front[i] = (float)((((R)m[3 + i] * yf + (R)m[6 + i] * zf) + m[9 + i])
                                  + b->pos[i]);
    }
}

void cpu_step(BikePhys *b, float dt, int countdown)
{
    int dirt = dirt_rider(b), bend, flat = 0, keys0;
    float old[3], hit[3];
    W.dt = dt;                          /* 0x45d540: 0x581014 */
    W.countdown = countdown;
    keys0 = b->keys1;
    b->detail = 0;
    b->events = 0;
    bp_start_step(b);
    memcpy(old, b->pos, 12);
    drive(b, dt);
    b->kick = 0.0f;
    if (bp_walls(b->sec, old, b->pos, hit, b->vdir, b->wall_n)) {
        memcpy(b->pos, hit, 12);
        b->sec = bp_walk(b->sec, b->pos);
        b->cpu_speed = (float)(b->cpu_speed * 0.5f);
        b->engine = (float)(b->engine * 0.5f);
        b->hit_timer = 200.0f;
        b->events |= 1;
    }
    b->speed = b->engine;
    bend = bp_bends(b->sec, 8);
    if (bend)
        b->turbo_req = 0;
    else {                              /* on a straight: now and then the turbo */
        flat = dirt;
        if ((bikephys_crt_rand() & 0xff) == 0 && b->cpu_speed > b->top)
            b->turbo_req = 1;
    }
    if (!W.countdown)
        bp_turbo(b, dt);
    settle(b, dirt, bend, flat);
    bp_frame_and_lean(b);
    if (dirt || ZERO(b->hit_timer)) {   /* the lean: half again (a road bike a fifth more) */
        b->lean = (float)(b->lean * (dirt ? 1.5 : 1.2));
        if (b->lean < -QPI)
            b->lean = (float)-QPI_F;
        if (b->lean > QPI)
            b->lean = (float)QPI_F;
    } else
        b->lean = 0.0f;
    b->lean = (float)(((R)b->lean - b->lean1) * 0.5 + b->lean1);
    b->lean1 = b->lean;
    bp_wheel_spin(b);                   /* reads this step's keys (+0x2dc8) */
    bp_suspension(b);
    if (W.shadow)                       /* 0x461aa0 */
        W.shadow(b);
    {
        int keys = b->keys1;            /* the step's keys go to the effects only */
        b->keys1 = keys0;
        if (W.effects)                  /* 0x466d70 */
            W.effects(b, (unsigned)keys);
    }
    bp_tumble(b, dt);
    if (!b->crashed)                    /* as bikephys_step: the drawer keeps its matrix */
        memcpy(b->tumble, b->m, sizeof b->m);
    /* 0x467b30(1) / 0x45d810: the rider's pose (drawing); the turbo latch moves */
    if (W.pose)
        W.pose(b);
    b->turbo_pressed1 = b->turbo_pressed;
    b->turbo_pressed = 0;
    bp_dials(b);
}

/* ------------------------------------------------------------------ the passive step */

/* 0x45e560 in practice (the other branch, the eased one, is Link's): the bike turned nine
   tenths of the way to where the point +0x198 says, the motion's direction eased towards the
   way the target moved, the bike put on the target */
static void follow(BikePhys *b, const float target[3], const float before[3])
{
    static const float zero[3] = { 0.0f, 0.0f, 0.0f };
    float L[12], Li[12], A, B, d, yaw, v[3];
    R dA;
    b->engine = b->speed;
    bp_look(L, zero, b->fwd);
    transpose(Li, L);
    A = (float)-bp_heading(b->m);
    B = (float)bp_heading(Li);
    A = bp_wrap_ge(A);                  /* 0x45b1b0(A, B, 0.9) */
    B = bp_wrap_ge(B);
    dA = A;
    if (dA + PI < B)
        B = (float)((R)B - TWO_PI);
    if (dA - PI > B)
        B = (float)((R)B + TWO_PI);
    d = wrap_unrounded(((R)B - A) * 0.9f);
    yaw = d;
    if (d > PI)
        yaw = (float)((R)d - TWO_PI);
    b->steer = 0.0f;
    bp_turn_yaw(b, yaw);
    v[0] = (float)((R)target[0] - before[0]);
    v[1] = (float)((R)target[1] - before[1]);
    v[2] = (float)((R)target[2] - before[2]);
    if (fabs(v[0]) > (R)1e-5f || fabs(v[1]) > (R)1e-5f || fabs(v[2]) > (R)1e-5f) {
        R f, g;
        float l = (float)sqrt(((R)v[0] * v[0] + (R)v[1] * v[1]) + (R)v[2] * v[2]);
        if (l != 0.0f) {
            R k = 1.0 / l;
            v[0] = (float)(v[0] * k), v[1] = (float)(v[1] * k), v[2] = (float)(k * v[2]);
        }
        f = (R)b->hit_timer * 0.005f;
        g = 1.0 - f;
        b->vdir[0] = (float)((R)v[0] * g + (R)b->vdir[0] * f);
        b->vdir[1] = (float)((R)v[1] * g + (R)b->vdir[1] * f);
        b->vdir[2] = (float)(g * v[2] + f * b->vdir[2]);
    }
    memcpy(b->pos, target, 12);
    b->sec = bp_walk(b->sec, b->pos);
}

void cpu_passive_step(BikePhys *b, const float target[3], const float before[3], float dt,
                      int countdown)
{
    int dirt = dirt_rider(b), bend, flat = 0;
    float old[3], hit[3];
    W.dt = dt;
    W.countdown = countdown;
    b->detail = 0;
    b->events = 0;
    bp_start_step(b);
    memcpy(old, b->pos, 12);
    follow(b, target, before);
    b->kick = 0.0f;
    if ((fabs((R)old[0] - b->pos[0]) > (R)1e-5f || fabs((R)old[1] - b->pos[1]) > (R)1e-5f
         || fabs((R)old[2] - b->pos[2]) > (R)1e-5f)
        && bp_walls(b->sec, old, b->pos, hit, b->vdir, b->wall_n)) {
        memcpy(b->pos, hit, 12);
        b->sec = bp_walk(b->sec, b->pos);
        b->cpu_speed = (float)(b->cpu_speed * 0.5f);
        b->engine = (float)(b->engine * 0.5f);
        b->hit_timer = 200.0f;
    }
    b->speed = b->engine;
    bend = bp_bends(b->sec, 8);
    if (bend)
        b->turbo_req = 0;
    else {
        flat = dirt;
        if ((bikephys_crt_rand() & 0xff) == 0 && b->cpu_speed > b->top)
            b->turbo_req = 1;
    }
    if (!W.countdown)
        bp_turbo(b, dt);
    settle(b, dirt, bend, flat);
    bp_frame_and_lean(b);
    if (dirt)                           /* the lean: half again; a road bike's only when not hit */
        b->lean = (float)(b->lean * 1.5);
    else if (ZERO(b->hit_timer))
        b->lean = (float)(b->lean * 1.2);
    else
        b->lean = 0.0f;
    b->lean = (float)(((R)b->lean - b->lean1) * 0.5 + b->lean1);
    b->lean1 = b->lean;
    bp_wheel_spin(b);                   /* (+0x2dc8: a passive rider has no keys) */
    bp_suspension(b);
    if (W.shadow)                       /* 0x461aa0 */
        W.shadow(b);
    if (W.effects)                      /* 0x466d70 */
        W.effects(b, (unsigned)b->keys1);
    bp_tumble(b, dt);
    if (!b->crashed)                    /* as bikephys_step: the drawer keeps its matrix */
        memcpy(b->tumble, b->m, sizeof b->m);
    if (W.pose)                         /* 0x467b30(1) */
        W.pose(b);
    b->turbo_pressed1 = b->turbo_pressed;
    b->turbo_pressed = 0;
    bp_dials(b);
    b->lane = bp_lane(b);
}

/* ------------------------------------------------------------------ the start (0x47bc60) */

void cpu_start(BikePhys *rider[], int n, int difficulty, int pocket, unsigned seed, int shuffle)
{
    int opp = n - 2, i;                 /* 0x542874 */
    float f = 1.0f;
    if (opp <= 0)
        return;
    if (pocket && g_land.factor_pocket != 0.0f)
        f = (float)(1.0 / g_land.factor_pocket);
    if (difficulty == 0)
        f = (float)(f * 0.9);
    else if (difficulty == 2)
        f = (float)(f * 1.05);
    for (i = 0; i < opp; i++) {         /* the slowest at the back of the grid */
        int idx = opp == 23 ? 22 - i : 22 - 2 * i;
        float v = idx >= 0 && idx < 32 ? g_land.computer_speed[idx] : 0.0f;
        if (rider[i + 2])
            rider[i + 2]->top = (float)(((R)v * KMH_MS) * f);
    }
    W.crt = seed;                       /* 0x47bf70: srand(timeGetTime()) */
    /* 0x47bd82: 0x4dbfd4 = 1, shuffled (not in the demos) */
    for (i = 0; shuffle && i < opp / 2; i++) {
        int a = bikephys_crt_rand() % opp + 2, c = bikephys_crt_rand() % opp + 2;
        if (a != c && rider[a] && rider[c]) {
            float t = rider[a]->top;
            rider[a]->top = rider[c]->top;
            rider[c]->top = t;
        }
    }
}
