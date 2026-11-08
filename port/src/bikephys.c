/* A driven bike's race step (0x45d580) and what it reaches: build/notes/bike_physics.md.
   Arithmetic as the game's x87 at 53 bits: R (a double) is a value in a register, a store
   to a float field is the game's store. The order of the sums is the game's. */
#include "bikephys.h"
#include "bikes.h"
#include "ftol.h"
#include "land.h"
#include "r3d.h"
#include "rand.h"
#include <float.h>
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

BikeWorld g_bikeworld = { 1.6f, 0, 1, 16.0f, 0, 0, 1, 0, 0, 0, { 0 }, 0, 0, 0, 0, 0, 0, 0, 0 };
#define W g_bikeworld

/* 1: the driven bike as the 1997 CD's MOTO.EXE rides it (addresses after a / are MOTO.EXE's);
   0: as GOG's game.EXE. Set at the race's load. */
int g_phys_cd = 0;

#define PI_F 1.5707963705062866         /* 0x3fc90fdb: pi/2 as a float */
#define TO_INDEX 2607.594587618272      /* 0x4d0300 / 0x4d0ed0: 16384 / 2pi */
#define TWO_PI 6.283185307179586        /* 0x4d0e20, 0x4d0e58 */
#define PI 3.141592653589793

/* the carac row (0x509278 + bike x 0x124) */
enum {
    C_FREIN = 0x14, C_MVM = 0x18, C_MASSE = 0x1c, C_RETARD = 0x20, C_RAA = 0x24, C_RMAA = 0x28,
    C_RVR = 0x2c, C_ANGSUSP = 0x30, C_RLA = 0x34, C_AXE = 0x38, C_RAV = 0x3c, C_RAR = 0x40,
    C_RSM = 0x48, C_VMB = 0x4c, C_ROT = 0x50, C_HAUT = 0x58, C_RDEC = 0x5c, C_RMIN = 0x60,
    C_RFAC = 0x64, C_NBSPEED = 0x70, C_RM = 0x74, C_RCM = 0x90, C_CM = 0x94, C_C1000 = 0x98,
    C_CRM = 0x9c, C_MINADH = 0xa4
};

static int bits(float f)
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

static const Bike *bk(const BikePhys *b)
{
    return &g_bikes[b->bike >= 0 && b->bike < BIKES ? b->bike : 0];
}

static float carac(const BikePhys *b, int off)
{
    const Bike *k = bk(b);
    float v = k->phys[(off - 0x14) / 4];
    if (off == C_RAA && k->type == 1 && !g_phys_cd) {   /* 0x42a7c3: the dirt bikes' clamp, not in /0x4978c0 */
        if (bits(v) > 0x40c00000)
            v = 6.0f;
        else if (bits(v) > 0x40400000)
            v = 3.0f;
    }
    return v;
}

static float ratio(const BikePhys *b, int g)
{
    return carac(b, C_RM + 4 * g);
}

static int dirt(const BikePhys *b)
{
    return bk(b)->type == 1;
}

/* the engine brake by gear (0x4f17dc + 4g; /0x4dcb28 + 4g starts one earlier, so on the CD a
   gear brakes as the next one, the top gear not at all) */
static const float k_engine_brake[8] = { 0.0f, 0.25f, 0.2f, 0.16f, 0.12f, 0.08f, 0.05f, 0.0f };
static const float k_engine_brake_cd[8] = { 0.25f, 0.2f, 0.16f, 0.12f, 0.08f, 0.05f, 0.0f, 0.0f };

/* ------------------------------------------------------------------ vectors */

static void normalise(float v[3])               /* 0x40fcd0 */
{
    float n = (float)sqrt(((R)v[0] * v[0] + (R)v[1] * v[1]) + (R)v[2] * v[2]);
    if (n != 0.0f) {
        R k = 1.0 / n;
        v[0] = (float)(v[0] * k), v[1] = (float)(v[1] * k), v[2] = (float)(k * v[2]);
    }
}

static R length(const float v[3])               /* 0x40fcb0 */
{
    return sqrt(((R)v[0] * v[0] + (R)v[1] * v[1]) + (R)v[2] * v[2]);
}

/* 0x410010: v M + T, rows of M */
static void mulvec(float o[3], const float v[3], const float *m)
{
    float x = (float)((((R)m[6] * v[2] + (R)m[3] * v[1]) + (R)m[0] * v[0]) + m[9]);
    float y = g_phys_cd ? (float)((((R)m[1] * v[0] + (R)m[4] * v[1]) + (R)m[7] * v[2]) + m[10])  /* /0x405e30 */
                        : (float)((((R)m[7] * v[2] + (R)m[4] * v[1]) + (R)m[1] * v[0]) + m[10]);
    float z = (float)((((R)m[8] * v[2] + (R)m[5] * v[1]) + (R)m[2] * v[0]) + m[11]);
    o[0] = x, o[1] = y, o[2] = z;
}

/* 0x46d610: a x b */
static void cross(float o[3], const float a[3], const float b[3])
{
    float x = (float)((R)a[1] * b[2] - (R)b[1] * a[2]);
    float y = (float)((R)b[0] * a[2] - (R)b[2] * a[0]);
    float z = (float)((R)b[1] * a[0] - (R)b[0] * a[1]);
    o[0] = x, o[1] = y, o[2] = z;
}

static R dot3(const float a[3], const float b[3])       /* 0x46d5e0 */
{
    return (R)b[0] * a[0] + ((R)a[2] * b[2] + (R)a[1] * b[1]);
}

/* the cosine table through an angle: index = ftol(a x k) */
static void cos_sin(R a, R *c, R *s)
{
    int i = ftol(a * TO_INDEX);
    *c = g_cos[i & 0x3fff];
    *s = g_cos[(i - 0x1000) & 0x3fff];
}

/* 0x40fd40: R(a0, a1, a2), T = 0 */
static void rot(float *m, R a0, R a1, R a2)
{
    R c0, s0, c1, s1, c2, s2;
    float A, B, S2;
    cos_sin(a0, &c0, &s0);
    cos_sin(a1, &c1, &s1);
    cos_sin(a2, &c2, &s2);
    S2 = (float)s2;
    A = (float)(s1 * s0);
    B = (float)(s1 * c0);
    m[0] = (float)(c1 * c2);
    m[1] = (float)((s1 * s0) * c2 + S2 * c0);
    m[2] = (float)(S2 * s0 - B * c2);
    m[3] = (float)-(S2 * c1);
    m[4] = (float)(c0 * c2 - A * S2);
    m[5] = (float)(c2 * s0 + B * S2);
    m[6] = (float)s1;
    m[7] = (float)-(c1 * s0);
    m[8] = (float)(c1 * c0);
    m[9] = m[10] = m[11] = 0.0f;
}

/* 0x403a80: o = A B (rows), the game's sum orders */
static void matmul(float *o, const float *a, const float *b)
{
    float t[12];
    t[0] = (float)((R)a[2] * b[6] + ((R)a[1] * b[3] + (R)a[0] * b[0]));
    t[1] = (float)(((R)a[0] * b[1] + (R)a[2] * b[7]) + (R)a[1] * b[4]);
    t[2] = (float)(((R)a[0] * b[2] + (R)a[1] * b[5]) + (R)a[2] * b[8]);
    t[3] = (float)(((R)a[3] * b[0] + (R)a[5] * b[6]) + (R)a[4] * b[3]);
    t[4] = (float)(((R)a[5] * b[7] + (R)a[3] * b[1]) + (R)a[4] * b[4]);
    t[5] = (float)(((R)a[5] * b[8] + (R)a[4] * b[5]) + (R)a[3] * b[2]);
    t[6] = (float)(((R)a[6] * b[0] + (R)a[7] * b[3]) + (R)a[8] * b[6]);
    t[7] = (float)(((R)a[6] * b[1] + (R)a[8] * b[7]) + (R)a[7] * b[4]);
    t[8] = (float)(((R)a[7] * b[5] + (R)a[6] * b[2]) + (R)a[8] * b[8]);
    t[9] = (float)((((R)a[9] * b[0] + (R)a[11] * b[6]) + (R)a[10] * b[3]) + b[9]);
    t[10] = (float)((((R)a[9] * b[1] + (R)a[11] * b[7]) + (R)a[10] * b[4]) + b[10]);
    t[11] = (float)((((R)a[9] * b[2] + (R)a[10] * b[5]) + (R)a[11] * b[8]) + b[11]);
    memcpy(o, t, sizeof t);
}

/* 0x46d270: the angles of a rotation */
static void angles(const float *m, float a[3])
{
    float c = (float)sqrt(1.0 - (R)m[6] * m[6]);
    /* 0x46d28b: not above 1e-5 (0x4d0e50), and a matrix a little past 1 gives a NaN here */
    if (!(c > 1e-5f)) {
        a[0] = 0.0f;
        a[1] = NEG(m[6]) ? (float)-PI_F : (float)PI_F;
        a[2] = (float)atan2(m[1], m[4]);
        return;
    }
    a[1] = (float)atan2(m[6], c);
    a[0] = (float)atan2(-(m[7] / (R)c), m[8] / (R)c);
    a[2] = (float)atan2((float)-(m[3] / (R)c), (float)(m[0] / (R)c));
}

/* 0x45cb00: turn the bike by r in its own frame, then rebuild the matrix from its angles
   through the table (the heading moves in steps of 2pi/16384) */
static void turn(BikePhys *b, const float *r)
{
    float a[3], S2;
    R c0, s0, c1, s1, c2, s2, A, B;
    float *m = b->m;
    matmul(m, r, m);
    angles(m, a);
    cos_sin(a[0], &c0, &s0);
    cos_sin(a[1], &c1, &s1);
    cos_sin(a[2], &c2, &s2);
    S2 = (float)s2;
    A = (float)(s1 * c2);
    B = (float)(S2 * s1);
    m[0] = (float)(c2 * c1);
    m[1] = (float)(S2 * c0 + A * s0);
    m[2] = (float)(S2 * s0 - A * c0);
    m[3] = (float)-(S2 * c1);
    m[4] = (float)(c2 * c0 - B * s0);
    m[5] = (float)(c2 * s0 + B * c0);
    m[6] = (float)s1;
    m[7] = (float)-(c1 * s0);
    m[8] = (float)(c1 * c0);
    m[9] = m[10] = m[11] = 0.0f;
}

static void turn_yaw(BikePhys *b, float yaw)
{
    float r[12];
    rot(r, 0.0, yaw, 0.0);
    turn(b, r);
}

/* 0x490220: minus the heading of a matrix's Z row */
static R heading(const float *m)
{
    float z0 = (float)((R)m[6] + m[9]), z1 = (float)((R)m[7] + m[10]),
          z2 = (float)((R)m[8] + m[11]);
    float y0 = (float)((R)m[3] + m[9]), y1 = (float)((R)m[4] + m[10]);
    float n = (float)sqrt(((R)z1 * z1 + (R)z2 * z2) + (R)z0 * z0);
    float c = (float)(sqrt(1.0) * n);
    if (fabs(c) < 1e-4)
        return bits(z0) > 0 ? PI_F : -PI_F;
    return atan2(((R)y0 * z1 + (R)-z0 * y1) / c, z2 / (R)c);
}

/* 0x46bdc0: the matrix looking from a to b (rows X, Y, Z, T = 0) */
static void look(float *o, const float a[3], const float b[3])
{
    float f[3], x[3], y[3], nx;
    R l;
    int i;
    for (i = 0; i < 3; i++)
        f[i] = (float)((R)b[i] - a[i]);
    l = (float)length(f);
    if (l != 0.0) {
        R r = 1.0 / l;
        for (i = 0; i < 3; i++)
            f[i] = (float)(f[i] * r);
    }
    nx = -f[0];
    y[0] = (float)((R)nx * f[1]);
    y[1] = (float)((R)f[2] * f[2] - (R)nx * f[0]);
    y[2] = (float)-((R)f[2] * f[1]);
    x[0] = f[2], x[1] = 0.0f, x[2] = nx;
    l = sqrt((R)f[2] * f[2] + (R)nx * nx);
    if (l != 0.0) {
        R r = 1.0 / (float)l;
        x[0] = (float)(f[2] * r), x[2] = (float)(r * nx);
    }
    l = sqrt(((R)y[1] * y[1] + (R)y[2] * y[2]) + (R)y[0] * y[0]);
    if (l != 0.0) {
        R r = 1.0 / (float)l;
        for (i = 0; i < 3; i++)
            y[i] = (float)(y[i] * r);
    }
    memcpy(o, x, 12), memcpy(o + 3, y, 12), memcpy(o + 6, f, 12);
    o[9] = o[10] = o[11] = 0.0f;
}

/* the inline [0, 2pi) wrap: each loop runs in the register and is stored once, the
   second starting from the first one's store */
static float wrap_ge(float a)
{
    R x;
    if (a < 0.0f) {
        x = a;
        do
            x += TWO_PI;
        while (x < 0.0);
        a = (float)x;
    }
    if (a >= TWO_PI) {                  /* from the stored value: float(2pi) > 2pi */
        x = a;
        do
            x -= TWO_PI;
        while (x >= TWO_PI);
        a = (float)x;
    }
    return a;
}

static float wrap_pi(R a)                       /* 0x458da0 */
{
    while (a < -PI)
        a += TWO_PI;
    while (a > PI)
        a -= TWO_PI;
    return (float)a;
}

int bikephys_crt_rand(void)                     /* 0x4b3600 */
{
    W.crt = W.crt * 214013u + 2531011u;
    return (int)((W.crt >> 16) & 0x7fff);
}

/* ------------------------------------------------------------------ the track */

typedef struct {
    float pos[3], m[12];
    float p40[4], p60[4], p70[4], p80[4], p90[4], pa0[4];
    float edge[2], line;
} PSec;

static PSec *s_sec;
static int s_nsec;

static void plane_through(float pl[4], const float n[3], const float p[3])    /* 0x4b0f20 */
{
    pl[0] = n[0], pl[1] = n[1], pl[2] = n[2];
    pl[3] = (float)-(((R)n[1] * p[1] + (R)n[2] * p[2]) + (R)n[0] * p[0]);
}

static R plane_dist(const float pl[4], const float p[3])        /* 0x4b10c0 */
{
    return (((R)pl[1] * p[1] + (R)pl[2] * p[2]) + (R)pl[0] * p[0]) + pl[3];
}

/* 0x4b0f60: where the line a -> b meets the plane (a when they are one point or the line
   runs along the plane) */
static void line_plane(float o[3], const float pl[4], const float a[3], const float b[3])
{
    R dx, den, t;
    float dy, dz, dxf, denf;
    if (fabs((R)a[0] - b[0]) <= 1e-5f && fabs((R)a[1] - b[1]) <= 1e-5f
        && fabs((R)a[2] - b[2]) <= 1e-5f) {
        memmove(o, a, 12);
        return;
    }
    dx = (R)b[0] - a[0];
    dy = (float)((R)b[1] - a[1]), dz = (float)((R)b[2] - a[2]);
    dxf = (float)dx;
    den = ((R)pl[0] * dx + (R)pl[1] * dy) + (R)pl[2] * dz;
    denf = (float)den;
    if (denf == 0.0f) {
        memmove(o, a, 12);
        return;
    }
    t = -(((((R)pl[0] * a[0] + (R)pl[2] * a[2]) + (R)pl[1] * a[1]) + pl[3]) / denf);
    {
        float x = (float)(dxf * t + a[0]), y = (float)(dy * t + a[1]),
              z = (float)(dz * t + a[2]);
        o[0] = x, o[1] = y, o[2] = z;
    }
}

static void point(float o[3], const PSec *s, float x)    /* pos + (x, 0, 0) M + T */
{
    float v[3] = { x, 0.0f, 0.0f }, w[3];
    int i;
    mulvec(w, v, s->m);
    for (i = 0; i < 3; i++)
        o[i] = (float)((R)s->pos[i] + w[i]);
}

/* 0x40e5c0: the start plane, the two road triangles from wall to wall and the plane
   between them, from the sections' axes and walls (the walls are the file's) */
int bikephys_track(void)
{
    int n = g_land.nsec, i, k;
    bikephys_track_free();
    if (n <= 0)
        return 0;
    if (!(s_sec = calloc((size_t)n, sizeof *s_sec)))
        return 0;
    s_nsec = n;
    for (i = 0; i < n; i++) {
        const Section *l = &g_land.sec[i];
        PSec *s = &s_sec[i];
        memcpy(s->pos, l->pos, 12);
        memcpy(s->m, l->axes, 36);               /* T (+0x30) is 0 in every .trk */
        memcpy(s->p60, l->plane[2], 16);
        memcpy(s->p70, l->plane[3], 16);
        s->edge[0] = l->edge[0], s->edge[1] = l->edge[1];
        s->line = l->line;
    }
    for (i = 0; i < n; i++) {
        PSec *s = &s_sec[i], *t = &s_sec[(i + 1) % n];
        static const float z1[3] = { 0.0f, 0.0f, 1.0f };
        float nz[3], li[3], lj[3], ri[3], rj[3], a[3], b[3], na[3], e1[3], e2[3], nn[3];
        mulvec(nz, z1, s->m);
        normalise(nz);
        plane_through(s->p40, nz, s->pos);
        memcpy(g_land.sec[i].plane[0], s->p40, 16);     /* 0x40e5c0's over the file's */
        point(a, s, 0.0f), point(b, s, -2048.0f);
        line_plane(li, s->p60, a, b);
        point(a, t, 0.0f), point(b, t, -2048.0f);
        line_plane(lj, s->p60, a, b);
        point(b, t, 2048.0f);
        line_plane(rj, s->p70, a, b);
        for (k = 0; k < 3; k++)
            a[k] = (float)((R)s->pos[k] + s->m[9 + k]);
        point(b, s, 2048.0f);
        line_plane(ri, s->p70, a, b);
        na[0] = (float)((R)ri[2] - lj[2]), na[1] = 0.0f, na[2] = (float)-((R)ri[0] - lj[0]);
        normalise(na);
        plane_through(s->pa0, na, lj);
        for (k = 0; k < 3; k++)
            e1[k] = (float)((R)lj[k] - li[k]), e2[k] = (float)((R)ri[k] - li[k]);
        nn[0] = (float)((R)e1[1] * e2[2] - (R)e1[2] * e2[1]);
        nn[1] = (float)((R)e1[2] * e2[0] - (R)e1[0] * e2[2]);
        nn[2] = (float)((R)e1[0] * e2[1] - (R)e1[1] * e2[0]);
        normalise(nn);
        plane_through(s->p80, nn, li);
        for (k = 0; k < 3; k++)
            e2[k] = (float)((R)ri[k] - rj[k]), e1[k] = (float)((R)lj[k] - rj[k]);
        nn[0] = (float)((R)e1[1] * e2[2] - (R)e1[2] * e2[1]);
        nn[1] = (float)((R)e1[2] * e2[0] - (R)e1[0] * e2[2]);
        nn[2] = (float)((R)e1[0] * e2[1] - (R)e1[1] * e2[0]);
        normalise(nn);
        plane_through(s->p90, nn, rj);
    }
    return 1;
}

void bikephys_track_free(void)
{
    free(s_sec);
    s_sec = 0;
    s_nsec = 0;
}

static int wrap_sec(int i)                      /* 0x40ec30 */
{
    if (s_nsec <= 0)
        return 0;
    while (i < 0)
        i += s_nsec;
    while (i >= s_nsec)
        i -= s_nsec;
    return i;
}

/* 0x40c7e0: the section whose start plane is behind p and its next one's ahead */
static int walk(int hint, const float p[3])
{
    int n = s_nsec, i, k;
    if (n <= 0 || hint >= n)
        return 0;
    i = hint < 0 ? 0 : hint;
    if (plane_dist(s_sec[i].p40, p) < 0.0f) {
        for (k = 0; k < n; k++) {
            i = i > 0 ? i - 1 : n - 1;
            if (plane_dist(s_sec[i].p40, p) >= 0.0f)
                break;
        }
        return i;
    }
    for (k = 0; k < n; k++) {
        int j = i + 1 < n ? i + 1 : 0;
        if (plane_dist(s_sec[j].p40, p) < 0.0f)
            break;
        i = j;
    }
    return i;
}

/* 0x40c8b0: the road's height under p, on the section's two triangles */
static R road_y(int sec, const float p[3])
{
    const PSec *s;
    const float *pl;
    if (s_nsec <= 0)
        return 0.0;
    s = &s_sec[wrap_sec(sec)];
    pl = plane_dist(s->pa0, p) < 0.0f ? s->p90 : s->p80;
    if (ZERO(pl[1]))
        return 0.0;
    return -((((R)pl[2] * p[2] + (R)pl[0] * p[0]) + pl[3]) / pl[1]);
}

double bikephys_road_y(int sec, const float p[3])
{
    return (double)road_y(sec, p);
}

/* the wall crossing as 0x40ca80 mirrors v on it */
static void mirror(float v[3], const float n[3])
{
    R k = (R)n[2] * v[2] + ((R)v[0] * n[0] + (R)n[1] * v[1]);
    float ky = (float)(k * (2.0 * n[1])), kz = (float)(k * (2.0 * n[2]));
    float l;
    v[0] = (float)(v[0] - k * (2.0 * n[0]));
    v[1] = (float)((R)v[1] - ky);
    v[2] = (float)((R)v[2] - kz);
    l = (float)length(v);
    if (l != 0.0f) {
        R r = 1.0 / l;
        v[0] = (float)(v[0] * r), v[1] = (float)(v[1] * r), v[2] = (float)(r * v[2]);
    }
}

/* 0x40ca80: did a -> b cross the section's left (+0x60), then right (+0x70) wall: hit,
   the wall's normal, v mirrored */
static int walls(int sec, const float a[3], const float b[3], float hit[3], float v[3],
                 float n[3])
{
    const PSec *s;
    int r = 0, w;
    if (fabs((R)b[0] - a[0]) <= 1e-5f && fabs((R)b[1] - a[1]) <= 1e-5f
        && fabs((R)b[2] - a[2]) <= 1e-5f)
        return 0;
    s = &s_sec[wrap_sec(sec)];
    for (w = 0; w < 2; w++) {
        const float *pl = w ? s->p70 : s->p60;
        if (plane_dist(pl, b) < 0.0f) {
            line_plane(hit, pl, a, b);
            memcpy(n, pl, 12);
            mirror(v, n);
            r = 1;
        }
    }
    return r;
}

/* 0x40cd90: p back between the walls, along the line from the section's point */
static void keep_in(int sec, float p[3])
{
    const PSec *s = &s_sec[wrap_sec(sec)];
    if (plane_dist(s->p60, p) < 0.0f)
        line_plane(p, s->p60, s->pos, p);
    if (plane_dist(s->p70, p) < 0.0f)
        line_plane(p, s->p70, s->pos, p);
}

/* 0x46c7a0: does the road bend within n sections (0: straight to 0.995) */
static int bends(int sec, int n)
{
    int dir = W.reverse < 0 ? -1 : 1, a = wrap_sec(n * dir + sec), c = wrap_sec(n * dir / 2 + sec);
    float u[3], v[3];
    R l, d;
    int k;
    if (s_nsec <= 0)
        return 1;
    for (k = 0; k < 3; k += 2) {
        u[k] = (float)((R)s_sec[a].pos[k] - s_sec[sec].pos[k]);
        v[k] = (float)((R)s_sec[c].pos[k] - s_sec[sec].pos[k]);
    }
    u[1] = v[1] = 0.0f;
    l = (float)sqrt((R)u[2] * u[2] + (R)u[0] * u[0]);
    if (l != 0.0)
        u[0] = (float)(u[0] * (1.0 / l)), u[2] = (float)(u[2] * (1.0 / l));
    l = (float)sqrt((R)v[2] * v[2] + (R)v[0] * v[0]);
    if (l != 0.0)
        v[0] = (float)(v[0] * (1.0 / l)), v[2] = (float)(v[2] * (1.0 / l));
    d = (float)(((R)v[1] * u[1] + (R)v[2] * u[2]) + (R)v[0] * u[0]);
    return fabs(d) > 0.995 ? 0 : 1;
}

/* ------------------------------------------------------------------ the body's velocity */

/* the velocity in the bike's frame: x across, y up, z forward (0x40fe80 + 0x410010 on
   the unit direction times the speed) */
static void local_velocity(const BikePhys *b, R out[3])
{
    float d[3], v[3], t[3];
    const float *m = b->m;
    int i;
    memcpy(d, b->dir, 12);
    normalise(d);
    for (i = 0; i < 3; i++)
        v[i] = (float)((R)d[i] * b->speed);
    t[0] = (float)-(((R)m[2] * m[11] + (R)m[1] * m[10]) + (R)m[9] * m[0]);
    t[1] = (float)-(((R)m[5] * m[11] + (R)m[4] * m[10]) + (R)m[9] * m[3]);
    t[2] = (float)-(((R)m[8] * m[11] + (R)m[7] * m[10]) + (R)m[9] * m[6]);
    out[0] = (((R)m[2] * v[2] + (R)m[1] * v[1]) + (R)m[0] * v[0]) + t[0];
    out[1] = (((R)m[5] * v[2] + (R)m[4] * v[1]) + (R)m[3] * v[0]) + t[1];
    out[2] = (((R)m[8] * v[2] + (R)m[7] * v[1]) + (R)m[6] * v[0]) + t[2];
}

static R lateral(const BikePhys *b)             /* 0x48d0f0 */
{
    R l[3];
    if (g_phys_cd) {                            /* /0x41c650: nothing stored on the way */
        const float *m = b->m;
        float d0 = b->dir[0], d1 = b->dir[1], d2 = b->dir[2];
        float n = (float)sqrt(((R)d1 * d1 + (R)d2 * d2) + (R)d0 * d0);
        R S = b->speed, v0, v1, v2, T;
        if (n != 0.0f) {
            R k = 1.0 / n;
            d0 = (float)(d0 * k), d1 = (float)(d1 * k), d2 = (float)(k * d2);
        }
        v0 = d0 * S, v1 = d1 * S, v2 = d2 * S;
        T = ((R)m[10] * m[1] + (R)m[11] * m[2]) + (R)m[9] * m[0];
        return ((v1 * m[1] + v2 * m[2]) + v0 * m[0]) + -T;
    }
    local_velocity(b, l);
    return l[0];
}

static R longitudinal(const BikePhys *b)        /* 0x48ca10 */
{
    R l[3];
    if (g_phys_cd) {                            /* /0x41c0f0: nothing stored on the way */
        const float *m = b->m;
        float d0 = b->dir[0], d1 = b->dir[1], d2 = b->dir[2];
        float n = (float)sqrt(((R)d2 * d2 + (R)d1 * d1) + (R)d0 * d0);
        R S = b->speed, v0, v1, v2, T;
        if (n != 0.0f) {
            R k = 1.0 / n;
            d0 = (float)(d0 * k), d1 = (float)(d1 * k), d2 = (float)(k * d2);
        }
        v0 = d0 * S, v1 = d1 * S, v2 = d2 * S;
        T = ((R)m[9] * m[6] + (R)m[10] * m[7]) + (R)m[11] * m[8];
        return -T + (v2 * m[8] + (v1 * m[7] + m[6] * v0));
    }
    local_velocity(b, l);
    return l[2];
}

/* the grip factor between the wheel and the road speed (inline, and 0x48eb40) */
static R grip(const BikePhys *b)
{
    if (dirt(b)) {
        R a = fabs(lateral(b));
        if (g_phys_cd) {                /* /0x41e2d5: this one through the routines, stored */
            R l[3];
            local_velocity(b, l);
            a = fabs((R)(float)l[0]);
        }
        if (a > 0.01 && a < 20.0)
            return (float)(a * -0.05 + 1.0);
        return 1.0;
    }
    return bits(carac(b, C_MINADH)) > 0x3f800000 ? carac(b, C_MINADH) : 1.0;
}

/* 0x48cbe0: the forward part of the velocity becomes w */
static void set_forward(BikePhys *b, float w)
{
    R l[3];
    const float *m = b->m;
    float nx, ny, nz, sp;
    float lx, ly;
    if (b->top < w)
        w = b->top;
    else if (-b->top > w)
        w = -b->top;
    local_velocity(b, l);
    lx = (float)l[0], ly = (float)l[1];
    nx = (float)((((R)m[3] * ly + (R)m[0] * lx) + (R)m[6] * w) + m[9]);
    ny = (float)((((R)m[1] * lx + (R)m[4] * ly) + (R)m[7] * w) + m[10]);
    nz = (float)(((R)m[8] * w + ((R)m[5] * ly + (R)m[2] * lx)) + m[11]);
    sp = (float)sqrt(((R)ny * ny + (R)nz * nz) + (R)nx * nx);
    b->speed = sp;
    if (!ZERO(sp)) {
        R k = 1.0 / sp;
        nx = (float)(nx * k), ny = (float)(ny * k), nz = (float)(nz * k);
    }
    if (sp > 1e-5)
        b->dir[0] = nx, b->dir[1] = ny, b->dir[2] = nz;
    else
        b->dir[0] = b->dir[1] = b->dir[2] = 0.0f;
}

static void wheel_to_forward(BikePhys *b)
{
    R l[3], k;
    local_velocity(b, l);
    k = grip(b);
    set_forward(b, (float)((b->wheel - (R)(float)l[2]) * k + (float)l[2]));
}

static void forward_to_wheel(BikePhys *b)
{
    R l[3], k;
    local_velocity(b, l);
    k = grip(b);
    b->wheel = (float)(((R)(float)l[2] - b->wheel) * k + b->wheel);
}

static void clamp_wheel(BikePhys *b)
{
    if (b->wheel > b->top)
        b->wheel = b->top;
    else if (-b->top > b->wheel)
        b->wheel = -b->top;
}

static void wheel_from_speed(BikePhys *b)       /* 0x48edd0 */
{
    b->wheel = b->speed;
    clamp_wheel(b);
}

/* 0x48deb0: the engine from the wheel speed (*wheel clamped at the red line) */
static float engine_from_wheel(BikePhys *b, float *wheel, int gear, float e)
{
    R mvm = carac(b, C_MVM), r;
    float out;
    if (b->shift)
        return e;
    if (gear < 1)
        gear = b->gear;
    r = (R)ratio(b, gear) * carac(b, C_VMB);
    out = NEG(*wheel) ? e : (float)(mvm / r * *wheel);
    if (NEG(out))
        out = 0.0f;
    if (mvm < out) {
        out = (float)mvm;
        *wheel = (float)(r / mvm * out);
    }
    return out;
}

/* 0x48dd90: the wheel speed from the engine (*e clamped) */
static float wheel_from_engine(BikePhys *b, float *e, int gear, float wheel)
{
    R mvm = carac(b, C_MVM), w80;
    float w, lim;
    if (b->shift)
        return wheel;
    if (gear < 1)
        gear = b->gear;
    if ((float)gear > carac(b, C_NBSPEED) || *e < 0.0001)
        return wheel;
    w80 = *e / mvm * ratio(b, gear) * carac(b, C_VMB);
    w = w80 < 0.0 ? 0.0f : (float)w80;
    lim = b->top;                       /* (b->top - 9 when 0x4dc1fc is 0: never) */
    if (w > lim) {
        w = lim;
        *e = (float)(mvm / ((R)ratio(b, gear) * carac(b, C_VMB)) * lim);
    }
    return w;
}

static void eng_sync(BikePhys *b)       /* engine <- wheel through 0x48deb0 */
{
    b->engine = engine_from_wheel(b, &b->wheel, b->gear, b->engine);
}

static void wheel_sync(BikePhys *b)     /* wheel <- engine through 0x48dd90 */
{
    b->wheel = wheel_from_engine(b, &b->engine, b->gear, b->wheel);
}

/* 0x48ee30: the torque at engine speed e */
static R torque(const BikePhys *b, R e)
{
    R rm = carac(b, C_RM), mvm = carac(b, C_MVM), rcm = carac(b, C_RCM), cm = carac(b, C_CM);
    R c1000 = carac(b, C_C1000), crm = carac(b, C_CRM), r80 = rm / mvm * e, s80, t80;
    float r = (float)r80, slope, icpt, t;
    if (r80 < rcm) {
        s80 = (cm - c1000) / (rcm - 1000.0);
        slope = (float)s80;
        icpt = (float)(s80 * -1000.0 + c1000);
    } else {
        s80 = (crm - cm) / (rm - rcm);
        slope = (float)s80;
        icpt = (float)(cm - rcm * s80);
    }
    t80 = ((R)slope * r + icpt) * 9.81;
    t = (float)t80;
    if (t80 < 0.2)
        t = 0.2f;
    if (!b->automatic)                  /* 0x4dc1f8 is 1 */
        t = (float)(t * 1.1);
    return (R)b->power * t;
}

/* 0x48e3a0: the automatic box's upshift speeds */
static void build_table(BikePhys *b)
{
    float nb = carac(b, C_NBSPEED), top = carac(b, C_RSM), v = 0.0f;
    int n = (int)nb, g = 1;
    R p = (R)ratio(b, n) * carac(b, C_VMB);
    if (p < top)
        top = (float)p;
    if (bits(nb) > 0x3f800000)
        while (v < top && (float)g < nb) {
            float c = v, r1, r2, p1;
            r1 = engine_from_wheel(b, &c, g, 0.0f);
            r2 = engine_from_wheel(b, &c, g + 1, 0.0f);
            p1 = (float)(torque(b, r1) * r1);
            if (torque(b, r2) * r2 > p1 || (g < 4 && (R)carac(b, C_MVM) * 0.97 < r1)) {
                b->table[g] = v;
                g++;
            }
            v = (float)(v + 0.1);
        }
    while ((float)g < nb && g < 7)
        b->table[g++] = top;
}

/* ------------------------------------------------------------------ the gearbox */

static void gear_up(BikePhys *b)                /* 0x48e0a0 */
{
    float nb = carac(b, C_NBSPEED);
    if (b->shift)
        return;
    if (nb <= (float)b->gear)
        b->gear = (int)nb;
    else if ((float)(b->gear + b->pending) < nb) {
        b->pending++;
        b->shift_timer = b->automatic ? 200 : 150;
        b->shift = 1;
    }
}

static void gear_down(BikePhys *b)              /* 0x48e120 */
{
    if (b->shift != 0 && b->shift != 2)
        return;
    if (b->gear < 2)
        b->gear = 1;
    else if (b->gear + b->pending > 1) {
        b->pending--;
        if (b->shift != 2) {
            b->shift_timer = b->automatic ? 300 : 200;
            b->shift = 2;
        }
    }
}

static void auto_box(BikePhys *b)               /* 0x48e550 */
{
    int g = b->gear;
    if (!b->automatic) {
        if (ZERO(b->table[1]))
            build_table(b);
        return;
    }
    if (b->slide || b->shift)
        return;
    if (bits(b->engine) < 1) {
        b->gear = 1;
        return;
    }
    if (W.countdown)
        return;
    if (ZERO(b->table[1]))
        build_table(b);
    if ((float)g < carac(b, C_NBSPEED) && b->table[g] < b->wheel)
        gear_up(b);
    else if (g > 1 && (R)b->table[g - 1] - 2.0 > b->wheel && (b->shift == 0 || b->shift == 2))
        gear_down(b);
}

/* 0x48e6e0: a shift in progress, ms of it gone */
static void shift_step(BikePhys *b, int ms)
{
    float nb = carac(b, C_NBSPEED);
    if (!b->shift)
        return;
    b->shift_timer -= ms;
    if (b->shift_timer >= 1)
        return;
    if (b->shift == 1) {
        b->pending = 0;
        b->shift = 0;
        if ((float)b->gear < nb) {
            b->gear++;
            if (!W.countdown) {
                eng_sync(b);
                wheel_sync(b);
                if (ZERO(b->air))
                    wheel_to_forward(b);
            }
            b->table[0] = b->engine;
        } else
            b->gear = (int)nb;
    } else if (b->shift == 2) {
        int n = abs(b->pending);
        b->shift = 0;
        b->pending = 0;
        b->table[0] = b->engine;
        if (b->gear < 2)
            b->gear = 1;
        else {
            if (n > b->gear - 1)
                n = b->gear - 1;
            if (n > 0) {
                if (b->automatic)
                    while (b->gear - n > 1 && (R)b->table[b->gear - n - 1] - 2.0 > b->wheel)
                        n++;
                b->gear -= n;
                if (!W.countdown) {
                    eng_sync(b);
                    wheel_sync(b);
                }
            }
        }
    } else
        b->shift = b->pending = 0;
}

/* ------------------------------------------------------------------ throttle and engine */

/* 0x48ef30: the engine, the brake, the slope, the rolling drag and the air; d = the
   bike's forward axis */
static void engine_step(BikePhys *b, const float d[3], int t, int scale, float dt)
{
    R mvm = carac(b, C_MVM), rm = carac(b, C_RM), masse = carac(b, C_MASSE);
    int free_ = W.countdown || b->shift != 0 || b->slide != 0;
    int air = !ZERO(b->air);
    int hold = t < 0 || W.countdown;
    int cpu = !b->driven;
    float dt10 = (float)(dt * 10.0f), saved, fwd;
    R l[3];
    if (dt10 > 500.0f)
        dt10 = 500.0f;
    if (b->speed > b->top) {            /* over the top speed */
        b->speed = b->top;
        if (!air)
            forward_to_wheel(b);
        clamp_wheel(b);
        if (!free_)
            eng_sync(b);
    }
    if (b->wheel > b->top) {
        b->wheel = b->top;
        if (!air)
            wheel_to_forward(b);
        if (!free_)
            eng_sync(b);
    }
    saved = b->speed;
    if (t >= 0) {
        R target80 = ((R)t / (R)scale) * mvm;
        float target = (float)target80;
        b->brake_ms = 0;
        if (!(target80 < b->engine) && t > 0) {
            if (free_ || air) {         /* revving freely */
                b->engine = (float)(mvm * dt10 * 0.0006 + b->engine);
                if (b->engine < 0.0f)
                    b->engine = 0.0f;
                if (b->engine > mvm)
                    b->engine = (float)mvm;
                if (W.gas && b->engine > mvm * W.gas / 255.0)
                    b->engine = (float)(mvm * W.gas / 255.0);
                if (air)
                    wheel_sync(b);
            } else {                    /* the drive */
                R idle = mvm / rm * 1200.0, tq, a;
                float f, acc, r;
                if (idle > b->engine)
                    b->engine = (float)idle;
                tq = torque(b, b->engine);
                f = (float)((((rm / mvm) * b->engine) * 2.0) * 0.05235987755982988 * tq);
                a = b->wheel > 0.0001 ? (float)(f / (R)b->wheel) : (float)(f * 10000.0);
                acc = (float)(dt10 * a / masse * 0.001);
                if (dt10 * 0.03 < acc)
                    acc = (float)(dt10 * 0.03);
                if (W.gas && W.gas < 255)
                    acc = (float)(acc * W.gas / 255.0f);
                b->wheel = (float)((R)b->wheel + acc);
                if (b->wheel > b->top)
                    b->wheel = b->top;
                r = engine_from_wheel(b, &b->wheel, b->gear, b->engine);
                b->engine = r;
                if (r > mvm)
                    b->engine = (float)mvm;
                else if (b->engine > target)
                    b->engine = target;
                wheel_sync(b);
                if (!air)
                    wheel_to_forward(b);
            }
        } else if (free_ || air) {
            b->engine = (float)((dt10 * -0.0006 + 1.0) * b->engine);
            if (b->engine < 0.0f)
                b->engine = 0.0f;
            if (b->engine > mvm)
                b->engine = (float)mvm;
            if (air)
                wheel_sync(b);
        } else if (mvm / rm * 1000.0 < b->engine) {     /* the engine brake */
            R eb = (g_phys_cd ? k_engine_brake_cd : k_engine_brake)[b->gear & 7];
            b->engine = (float)(eb * b->engine * dt10 * -0.001 + b->engine);
            if (b->engine < 0.0f)
                b->engine = 0.0f;
            wheel_sync(b);
            if (!air)
                wheel_to_forward(b);
        }
    }
    b->slide = 0;
    if (hold) {                         /* the brake */
        if (b->brakes_grip == 0 && !cpu && !W.countdown) {
            b->brake_ms = 0;
            if (dirt(b))
                b->slide = 1;
        } else {
            float fr = carac(b, C_FREIN), dec;
            if (bits(fr) < 0x41200000)
                fr = 10.0f;
            else if (fr > 140.0)
                fr = 140.0f;
            if ((float)b->brake_ms < 200.0f && !cpu)
                fr = (float)(fr * (1.0f / 6.0f));
            if (W.brake && W.brake < 255)
                fr = (float)(fr * W.brake / 255.0f);
            dec = (float)(fr * 0.2777777777777778 * dt10 * 0.001);
            if (bits(b->wheel) > 0) {
                b->wheel = (float)((R)b->wheel - dec);
                if (b->wheel < 0.0f)
                    b->wheel = 0.0f;
            } else if (NEG(b->wheel)) {
                b->wheel = (float)((R)b->wheel + dec);
                if (b->wheel > 0.0f)
                    b->wheel = 0.0f;
            }
            if (!free_)
                eng_sync(b);
            if (!air)
                wheel_to_forward(b);
            b->brake_ms = (int)((R)b->brake_ms + dt10);
        }
    }
    if (!air) {                         /* the slope and the rolling drag */
        float r = 0.02f, rr;
        local_velocity(b, l);
        fwd = (float)l[2];
        if (g_phys_cd) {                /* /0x41f44c: always, the axis' height over its length */
            R n = sqrt(((R)d[1] * d[1] + (R)d[2] * d[2]) + (R)d[0] * d[0]);
            float g = (float)(-((R)d[1] / n) * dt10 * 0.004905);
            if (g > 0.55f)
                g = 0.55f;
            fwd = (float)((R)g + fwd);
        } else if (!hold || b->speed > 0.1) {
            float g = (float)(-(R)d[1] * dt10 * 0.004905);
            if (g > 0.55f)
                g = 0.55f;
            fwd = (float)((R)g + fwd);
        }
        if (dirt(b))
            r = (float)(r * 0.5);
        rr = (float)(r * (R)dt10 * 0.00981);
        if (bits(b->speed) < 0x41a00000)
            rr = (float)(rr * (R)b->speed * 0.05);
        if (NEG(fwd)) {
            R s = (R)rr + fwd;
            fwd = s > 0.0 ? 0.0f : (float)s;
        } else {
            fwd = (float)((R)fwd - rr);
            if (fwd < 0.0f)
                fwd = 0.0f;
        }
        set_forward(b, fwd);
        forward_to_wheel(b);
        clamp_wheel(b);
        if (!free_)
            eng_sync(b);
    }
    {                                   /* the air */
        R dd = (R)b->speed * b->speed * dt10 * (air ? 1.6f : 0.3f);
        dd = (float)(dd / masse * 0.001);
        b->speed = (float)(b->speed - dd);
        if (b->speed < 0.0f)
            b->speed = 0.0f;
    }
    if (!air) {
        R k;
        local_velocity(b, l);
        k = grip(b);
        b->wheel = (float)(((R)(float)l[2] - b->wheel) * k + b->wheel);
        clamp_wheel(b);
        if (!free_)
            eng_sync(b);
    }
    b->speed_in = saved;
}

/* 0x465da0: the keys or the joystick to a throttle */
static void throttle(BikePhys *b, unsigned keys, int joy_y, float dt)
{
    int s = b->state, t, scale;
    float d[3];
    if (s != 0 && s != 1 && b->driven)
        keys = 2;
    if (W.countdown)
        keys |= 2;
    else if (!ZERO(b->wheelie))
        keys |= 1;
    if (keys & 2) {
        b->turbo_req = 0;
        if (b->driven && b->human)
            b->turbo_held = 0;
    }
    d[0] = (float)((R)b->m[9] + b->m[6]);
    d[1] = (float)((R)b->m[10] + b->m[7]);
    d[2] = (float)((R)b->m[11] + b->m[8]);
    {
        float l = (float)sqrt(((R)d[1] * d[1] + (R)d[2] * d[2]) + (R)d[0] * d[0]);
        if (l != 0.0f) {
            R k = 1.0 / l;
            d[0] = (float)(d[0] * k), d[1] = (float)(d[1] * k), d[2] = (float)(d[2] * k);
        }
    }
    if (joy_y != 0x8000 && (s == 0 || s == 1)) {
        t = 0x8000 - joy_y;
        if (t < 0)
            t /= 2000;
        scale = 0x8000;
    } else if (keys & 3) {
        t = (keys & 2) && !W.countdown ? -1 : (keys & 1) ? 1 : 0;
        scale = 1;
    } else
        t = 0, scale = 1;
    /* (the port's: a gamepad's triggers; 0 the game's own full press) */
    W.brake = (keys >> 8) & 0xff;
    W.gas = (keys >> 16) & 0xff;
    engine_step(b, d, t, scale, dt);
    W.brake = W.gas = 0;
}

/* 0x4660f0: the turbo lifts the front wheel and multiplies the power */
static void turbo(BikePhys *b, float dt)
{
    if (b->crashed) {
        b->wheelie = b->turbo_time = 0.0f;
        b->turbo_req = 0;
        b->power = 1.0f;
        if (b->driven && b->human)
            b->turbo_held = 0;
        return;
    }
    if (b->turbo_req && bits(b->turbo_time) < 0x43480000) {
        b->wheelie = (float)(dt * 0.0471238898038469 + b->wheelie);
        if (b->wheelie > 1.5707963267948966)
            b->wheelie = (float)PI_F;
        b->turbo_time = (float)((R)b->turbo_time + dt);
    } else if (bits(b->wheelie) > 0) {
        b->wheelie = (float)(dt * -0.025132741228718346 + b->wheelie);
        if (NEG(b->wheelie)) {
            b->kick = -32.0f;
            b->wheelie = 0.0f;
        }
    }
    if (!ZERO(b->wheelie))
        b->power = 2.0f;                /* 0x4dc1f8 is 1 (else 1.2) */
    else {
        b->power = 1.0f;
        if (!b->turbo_req)
            b->turbo_time = 0.0f;
    }
}

/* ------------------------------------------------------------------ steering */

/* 0x466240: the handlebar from the keys or the joystick, and the bike turned about its
   own up axis by it */
static void steering(BikePhys *b, unsigned keys, int joy_x, float dt)
{
    const R K = 0.5555555555555556;     /* 2 km/h */
    float lat0 = (float)lateral(b), s = b->steer, r = b->steer_rate, T, T2, m, yaw;
    float raa = carac(b, C_RAA), rla = carac(b, C_RLA), rvr = carac(b, C_RVR);
    /* /0x486fbb, /0x487054: the CD holds the rate to RoueMaxAccelerationAngulaire */
    R rmax = g_phys_cd ? (R)carac(b, C_RMAA) : HUGE_VAL;
    R v = b->speed;
    float rot_[12];
    if (b->forced)
        keys |= (unsigned)b->forced_keys;
    if (joy_x != 0x8000) {
        int n = v < K ? 0 : joy_x - 0x8000;
        b->analog = 1;
        s = (float)(((R)n * rla) * 3.0517578125e-05f);
    } else if ((keys & 8) && v > K) {
        R nw;
        b->analog = 0;
        if (NEG(s))
            s = 0.0f;
        nw = (R)raa * dt + r;
        r = (float)(rmax <= nw ? rmax : nw);
        nw = (R)r * dt + s;
        s = (float)(rla <= nw ? (R)rla : nw);
    } else if ((keys & 4) && v > K) {
        R nw;
        b->analog = 0;
        if (bits(s) > 0)
            s = 0.0f;
        nw = (R)raa * dt + r;
        r = (float)(rmax <= nw ? rmax : nw);
        nw = s - (R)r * dt;
        s = (float)(-(R)rla >= nw ? -(R)rla : nw);
    } else {                            /* back to the centre */
        R ret80 = (R)b->accel * rvr * 0.16666666666666666;
        float ret = ret80 < rvr ? rvr : (float)ret80;
        r = 0.0f;
        if (bits(s) > 0) {
            s = (float)(s - (R)ret * dt);
            if (NEG(s))
                s = 0.0f;
        } else if (NEG(s)) {
            s = (float)((R)ret * dt + s);
            if (bits(s) > 0)
                s = 0.0f;
        }
    }
    b->steer = s, b->steer_rate = r;
    if (g_phys_cd) {                    /* /0x487135: a turn by +0x2e45, only ever 0: it still
                                           puts the matrix through the table once more */
        rot(rot_, 0.0, 0.0, 0.0);
        turn(b, rot_);
    }
    if (!b->driven)
        T = 0.004f;
    else {
        R rt = carac(b, C_ROT), f = v / carac(b, C_VMB) * 0.5, t;
        f = f < 0.0 ? 0.0 : f < 0.5 ? f : 0.5;
        t = rt - (f * rt) * 0.5f;
        if (!ZERO(b->wheelie))
            t = t * 0.5f;
        T = (float)t;
    }
    T2 = (float)(((R)T * dt) * 0.01f);
    b->brakes_grip = 1;
    if (b->driven && dirt(b)) {
        if (joy_x != 0x8000)
            b->brakes_grip = joy_x < 0x4000 || joy_x > 0xc000 ? 0 : 1;
        else
            b->brakes_grip = (keys & 2) && (keys & 0xc) ? 0 : 1;
        if (bits(b->speed) < 0x40000000)
            b->brakes_grip = 1;
    }
    m = 1.0f;
    if (b->sliding) {
        R g = fabs((R)lat0) * 0.1 + 1.0;
        g = g < 1.0 ? 1.0 : g < 1.6 ? g : 1.6;
        m = (float)g;
    }
    if ((keys & 2) && (keys & 0xc) && b->sliding)      /* 0x46661a, 0x4872a9: in the slide's part only */
        m = 1.6f;
    yaw = (float)(((R)m * T2) * s);
    rot(rot_, 0.0, yaw, 0.0);
    turn(b, rot_);
}

/* ------------------------------------------------------------------ the sideways slide */

/* 0x45cd10: the signed radius of the circle through three points (x, z); FLT_MAX when
   they are in line */
static R turn_radius(R x0, R z0, R x1, R z1, R x2, R z2)
{
    float A = (float)(z1 - z0), B = (float)(x2 - x1), C = (float)(x1 - x0),
          D = (float)(z2 - z1);
    float d1, d2, l1, l2, q1z, q1x, q0z, q0x, G, H, E, cx, cz, rr;
    R J, K, num, den, cxe, side;
    if ((fabs(D) < 1e-4 && fabs(B) < 1e-4) || (fabs(A) < 1e-4 && fabs(C) < 1e-4))
        return FLT_MAX;
    d1 = (float)sqrt((R)C * C + (R)A * A);
    d2 = (float)sqrt((z1 - z2) * (z1 - z2) + (x1 - x2) * (x1 - x2));
    l1 = d1, l2 = d2;
    if (bits(l1) > 0x44480000)
        l1 = 800.0f;
    if (bits(l1) < 0x43000000)
        l1 = 128.0f;
    if (bits(l2) > 0x44480000)
        l2 = 800.0f;
    if (bits(l2) < 0x43000000)
        l2 = 128.0f;
    q1z = (float)((((R)l2 / d2) * D) * 0.5 + z1);
    q1x = (float)((((R)l2 / d2) * B) * 0.5 + x1);
    q0z = (float)((((z0 - z1) / d1) * l1) * 0.5 + z1);
    q0x = (float)((((x0 - x1) / d1) * l1) * 0.5 + x1);
    G = (float)((R)q1z - z1);
    H = (float)((R)q1x - x1);
    J = (R)q0x - x0;
    E = (float)((R)q1z - q0z);
    K = (R)G * E + (R)H * q1x;
    num = K * ((R)q0z - z0) - (R)q0x * (J * G);
    den = ((R)q0z - z0) * H - J * G;
    if (fabs((float)den) <= 1e-4 || fabs(G) <= 1e-4)
        return FLT_MAX;
    cxe = num / den;
    cx = (float)cxe;
    cz = (float)((((R)q1x - cxe) / G) * H + q1z);
    rr = (float)sqrt((x1 - cx) * (x1 - cx) + (z1 - cz) * (z1 - cz));
    side = ((R)cz - q0z) * ((R)q1x - q0x) - ((R)cx - q0x) * E;
    if (side < 0.0)
        rr = -rr;
    return (float)(rr * 2.0);
}

/* 0x48d2c0: the sideways speed becomes lat */
static void set_lateral(BikePhys *b, float lat)
{
    const float *m = b->m;
    float d[3], w[3], ly, lz, x, y, z, sp;
    int i;
    if (b->top < lat)
        lat = b->top;
    else if (-b->top > lat)
        lat = -b->top;
    memcpy(d, b->dir, 12);
    normalise(d);
    for (i = 0; i < 3; i++)
        w[i] = (float)((R)b->speed * d[i]);
    ly = (float)((((R)m[3] * w[0] + (R)m[4] * w[1]) + (R)m[5] * w[2]) + -0.0);
    lz = (float)((((R)m[8] * w[2] + (R)m[7] * w[1]) + (R)m[6] * w[0]) + -0.0);
    if (g_phys_cd) {                            /* /0x41c740's orders; z squared half stored */
        R zr;
        ly = (float)((((R)m[5] * w[2] + (R)m[4] * w[1]) + (R)m[3] * w[0]) + -0.0);
        x = (float)((((R)m[3] * ly + (R)m[6] * lz) + (R)m[0] * lat) + m[9]);
        y = (float)((((R)m[7] * lz + (R)m[4] * ly) + (R)m[1] * lat) + m[10]);
        zr = (((R)m[5] * ly + (R)m[8] * lz) + (R)m[2] * lat) + m[11];
        z = (float)zr;
        sp = (float)sqrt((zr * z + (R)y * y) + (R)x * x);
    } else {
        x = (float)((((R)m[0] * lat + (R)m[3] * ly) + (R)m[6] * lz) + m[9]);
        y = (float)((((R)m[1] * lat + (R)m[7] * lz) + (R)m[4] * ly) + m[10]);
        z = (float)((((R)m[2] * lat + (R)m[5] * ly) + (R)m[8] * lz) + m[11]);
        sp = (float)sqrt(((R)y * y + (R)z * z) + (R)x * x);
    }
    b->speed = sp;
    if (!ZERO(sp)) {
        R k = 1.0 / sp;
        x = (float)(x * k), y = (float)(y * k), z = (float)(k * z);
    }
    if (fabs((R)sp) > 1e-5)
        b->dir[0] = x, b->dir[1] = y, b->dir[2] = z;
    else
        b->dir[0] = b->dir[1] = b->dir[2] = 0.0f;
}

/* 0x45d110: the turn radius from the last three positions, the road's camber and (dirt)
   the bike sliding out of a bend; the sideways speed decays by g */
static void slide(BikePhys *b, float dt)
{
    float dts = (float)(dt * 0.01), lon, lat, rloc = FLT_MAX, s, a = 0.0f, s9, g9, nw, dec,
          py, q[3], h;
    int d = dirt(b);
    if (b->air > 2.592)
        return;
    lon = (float)longitudinal(b);
    lat = (float)lateral(b);
    /* (0x4dc1f8, a genuine-copy flag, is set to 1 here in the championship) */
    if (!d || g_phys_cd)                /* /0x47e850: the CD, every bike, and a road bike keeps it */
        b->radius = FLT_MAX;
    if (b->forced == 0 && bits(b->speed) > 0x3f800000 && (d || !g_phys_cd)) {
        R r = turn_radius(b->pos2[0], b->pos2[2], b->pos1[0], b->pos1[2], b->pos[0],
                          b->pos[2]) * 0.0078125;
        b->radius = (float)r;
        if (d)
            rloc = b->radius;
    } else if (d)
        rloc = FLT_MAX;
    py = s_nsec ? (float)road_y(b->sec, b->pos) : 0.0f;
    q[0] = (float)(((R)b->m[9] + b->m[0]) + b->pos[0]);
    q[1] = (float)(((R)b->m[1] + b->m[10]) + py);
    q[2] = (float)(((R)b->m[2] + b->m[11]) + b->pos[2]);
    h = s_nsec ? (float)((R)q[1] - road_y(b->sec, q)) : q[1];
    s = (float)(h / sqrt((R)h * h + 1.0));
    if (d)
        a = (float)((R)lon * lon / rloc);
    if (b->slide)
        a = (float)(a * 2.0);
    s9 = (float)(s * 9.81);
    g9 = (float)(1.0f * 9.81);          /* 0x459f80: the grip, always 1.0 */
    nw = (float)(((d ? (R)s9 + a : (R)s9) * dts) + lat);
    dec = (float)((R)g9 * dts);
    if (bits(nw) > 0) {
        nw = (float)((R)nw - dec);
        if (nw < 0.0f)
            nw = 0.0f;
    } else if (nw < 0.0) {
        nw = (float)((R)dec + nw);
        if (nw > 0.0f)
            nw = 0.0f;
    }
    set_lateral(b, nw);
    if (g_phys_cd && fabs((R)nw - lat) > 0.1) {     /* /0x47ebfd: a slide's change costs speed */
        set_forward(b, (float)((R)lon * 0.994));
        if (ZERO(b->air)) {
            R l[3], k;
            local_velocity(b, l);
            k = grip(b);
            b->wheel = (float)(((R)(float)l[2] - b->wheel) * k + b->wheel);
        }
        clamp_wheel(b);
        eng_sync(b);
    }
}

/* ------------------------------------------------------------------ moving */

/* 0x48d620: one step of the body: on the ground the motion turns towards the bike's
   forward axis and a downward push keeps it on the road; in the air gravity */
static void integrate(BikePhys *b, float dt, int flat)
{
    float dtkf = (float)(dt * 0.01f), old = b->speed, F[3], V[3], P[3], Pm = b->push, d[3];
    static const float z1[3] = { 0.0f, 0.0f, 1.0f };
    R dtk = dtkf, k, kP;
    int i;
    mulvec(F, z1, b->m);
    normalise(F);
    memcpy(b->dir, F, 12);
    memcpy(V, b->vdir, 12);
    memcpy(P, b->push_dir, 12);
    if (b->air > 2.592) {               /* in the air */
        float sp;
        normalise(V);
        for (i = 0; i < 3; i++)
            V[i] = (float)((R)V[i] * b->speed);
        V[1] = (float)(V[1] - dtk * 24.525);
        sp = (float)sqrt(((R)V[1] * V[1] + (R)V[2] * V[2]) + (R)V[0] * V[0]);
        b->speed = sp;
        normalise(V);
        if (!ZERO(Pm)) {
            R r;
            for (i = 0; i < 3; i++)
                P[i] = (float)((R)P[i] * Pm);
            P[1] = (float)(dtk * -24.525 + P[1]);
            Pm = (float)sqrt(((R)P[1] * P[1] + (R)P[2] * P[2]) + (R)P[0] * P[0]);
            r = 1.0 / Pm;
            P[0] = (float)(P[0] * r), P[1] = (float)(P[1] * r), P[2] = (float)(r * P[2]);
        }
        k = ((R)b->speed * 128.0) * dtk;
        d[0] = (float)(V[0] * k), d[1] = (float)(V[1] * k), d[2] = (float)(k * V[2]);
        if (b->top < b->speed)
            b->speed = b->top;
    } else {                            /* on the ground */
        R c = ((R)V[1] * F[1] + (R)V[2] * F[2]) + (R)V[0] * F[0], g80, r80, w;
        float g, r;
        if (g_phys_cd)                  /* /0x41cd85 */
            c = ((R)V[2] * F[2] + (R)V[0] * F[0]) + (R)V[1] * F[1];
        if (c < 0.0)
            b->speed = (float)(b->speed * fabs((float)c));
        if (g_phys_cd)                  /* /0x41cda5: only going sideways loses ground, no radius */
            g = (float)fabs((float)c);
        else {
            g80 = (dirt(b) ? 11.0 : 6.0) / fabs((R)b->radius);
            g = (float)(1.0 - g80);
            if (bits(g) > 0x3f800000)
                g = 1.0f;
            else if (g < 0.5)
                g = 0.5f;
        }
        r80 = (R)b->speed / b->top * 2.0f;
        r = (float)r80;
        if (r80 > 0.9)
            r = 0.9f;
        if (r < 0.85)
            r = 0.85f;
        w = 1.0f - (R)r;
        V[0] = (float)(F[0] * w + (R)V[0] * r);
        V[2] = (float)(w * F[2] + (R)V[2] * r);
        V[1] = F[1];
        normalise(V);
        k = (((R)b->speed * g) * 128.0) * dtk;
        d[0] = (float)(V[0] * k), d[1] = (float)(V[1] * k), d[2] = (float)(k * V[2]);
        if (flat == 0 || bits(d[1]) <= 0)
            d[1] = (float)(dtk * -1255.68 + d[1]);
        if (!ZERO(Pm)) {
            R c2 = ((R)P[2] * F[2] + (R)P[1] * F[1]) + (R)P[0] * F[0];
            P[1] = c2 < 0.0 ? -F[1] : F[1];
            normalise(P);
        }
    }
    kP = ((R)Pm * 128.0) * dtk;
    d[0] = (float)(P[0] * kP + d[0]);
    d[1] = (float)(P[1] * kP + d[1]);
    d[2] = (float)(kP * P[2] + d[2]);
    for (i = 0; i < 3; i++)
        b->pos[i] = (float)((R)d[i] + b->pos[i]);
    memcpy(b->move, d, 12);
    if (fabs((float)((R)old - b->speed)) > 0.001) {
        if (ZERO(b->air)) {
            R l[3];
            float a;
            local_velocity(b, l);       /* (/0x41e040: stored on the way) */
            a = (float)l[2];
            b->wheel = (float)(((R)a - b->wheel) * grip(b) + b->wheel);
        }
        clamp_wheel(b);
        eng_sync(b);
    }
    {
        float dec = (float)(dtk * 33.333333333333336);
        if (bits(Pm) > 0) {
            Pm = (float)((R)Pm - dec);
            if (NEG(Pm))
                Pm = 0.0f;
        } else if (NEG(Pm)) {
            Pm = (float)((R)Pm + dec);
            if (bits(Pm) > 0)
                Pm = 0.0f;
        }
    }
    memcpy(b->vdir, V, 12);
    memcpy(b->push_dir, P, 12);
    b->push = Pm;
}

/* 0x45ca00: the frame's pitch, from its matrix +0x2e55 */
static R frame_pitch(const BikePhys *b)
{
    const float *f = b->frame;
    float fx = (float)((R)f[6] + f[9]), fy = (float)((R)f[7] + f[10]),
          fz = (float)((R)f[8] + f[11]);
    float xx = (float)((R)f[0] + f[9]), xz = (float)((R)f[2] + f[11]);
    float h = (float)((R)fz * fz + (R)fx * fx);
    float n = (float)sqrt(((R)fy * fy + (R)fz * fz) + (R)fx * fx);
    float r = (float)(sqrt(h) * n);
    if (fabs(r) < 1e-4)
        return bits(fy) > 0 ? PI_F : -PI_F;
    if (g_phys_cd) {                    /* /0x47e073: the products stored first */
        float a0 = (float)((R)fz * fy), a2 = (float)-((R)fx * fy);
        return atan2((((R)a2 * xz) + (R)a0 * xx) / r, h / (R)r);
    }
    return atan2(((((R)fz * fy) * xx) + (-((R)fx * fy)) * xz) / r, h / (R)r);
}

/* 0x466660: the time in the air, the nose dipping under the brake, the gear keys and the
   shift in progress */
static void gearbox(BikePhys *b, unsigned keys)
{
    if (!b->ground)
        b->air = (float)(b->air + 2.592);
    else {
        float lon;
        b->air = 0.0f;
        lon = (float)longitudinal(b);
        if ((keys & 2) && lon > 0.1) {
            float a = wrap_pi(0.024543693f), r[12];
            R p = frame_pitch(b);
            if (p > 1.4726215563702154 || p < -1.4726215563702154)
                a = 0.0f;               /* 0x45d0c0 */
            rot(r, a, 0.0, 0.0);
            turn(b, r);
            b->brake_dive = (float)(W.dt * -0.001963495408493621 + b->brake_dive);
            if (b->brake_dive < -0.19634954084936207)
                b->brake_dive = -0.19634955f;
        }
        if (g_phys_cd && (keys & 2) && bits(b->speed) > 0x40400000)
            b->shift = 0;               /* /0x487442: no box above 3 m/s under the brake */
        else
            auto_box(b);
    }
    if (b->driven) {
        int up = keys & 0x10, down = keys & 0x20;
        if (up && down)
            W.gear_latch = 1;
        else if (up) {
            if (!W.gear_latch && !b->automatic)
                gear_up(b);
            W.gear_latch = 1;
        } else if (down) {
            if (!W.gear_latch && !b->automatic)
                gear_down(b);
            W.gear_latch = 1;
        } else
            W.gear_latch = 0;
    }
    shift_step(b, (int)((R)W.dt * 10.0f));
}

/* ------------------------------------------------------------------ crashes */

/* 0x467c40: debris pieces (effects.c); it draws the game's stream 1 3 + 5n times, kept
   without the pieces so that the stream stays the game's */
static void debris(int n, const float pos[3], int base, int mask, const float *m)
{
    int i;
    if (W.debris) {
        W.debris(n, pos, base, mask, m);
        return;
    }
    for (i = 0; i < 3 + 5 * n; i++)
        rand_next(1);
}

/* the crashed bike laid on its side at its heading (inline in 0x467e30 and 0x4683e0)
   [the x87 order of these two products is not transcribed] */
static void lay_flat(BikePhys *b)
{
    float r1[12], r2[12];
    rot(r1, -1.5707963267948966, 0.0, 0.0);
    rot(r2, 0.0, -heading(b->tumble), 0.0);
    matmul(b->tumble, r1, r2);
}

/* 0x467e30: the crash starts, thrown along v */
static void crash(BikePhys *b, const float v[3])
{
    if (b->crashed)
        return;
    memcpy(b->crash_v, v, 12);
    b->crash_flag = 1;
    b->gear = 1;
    b->crashed = 1;
    b->crash_timer = 400.0f;
    b->tumble_dir[0] = v[0];
    b->tumble_dir[1] = (float)(v[1] + 0.6);
    b->tumble_dir[2] = v[2];
    normalise(b->tumble_dir);
    b->tumble_speed = b->tumble_speed0 = (float)(b->speed * 0.15);
    b->tumble_pt[0] = (float)((R)b->tumble_dir[0] * b->speed + b->pos[0]);
    b->tumble_pt[1] = (float)((R)b->tumble_dir[1] * b->speed + b->pos[1]);
    b->tumble_pt[2] = (float)((R)b->speed * b->tumble_dir[2] + b->pos[2]);
    if (!ZERO(b->wall_spin))
        b->roll_side = NEG(b->wall_spin) ? (float)-PI_F : (float)PI_F;
    else if (!ZERO(b->lean))
        b->roll_side = NEG(b->lean) ? (float)-PI_F : (float)PI_F;
    else
        b->roll_side = bikephys_crt_rand() & 1 ? (float)-PI_F : (float)PI_F;
    lay_flat(b);
    b->roll[0] = 0.0f;
    b->roll[1] = b->roll[2] = b->roll_side;
    debris(6, b->pos, 15, 16, b->m);
}

/* 0x4681d0: the end of a crash; the bikes get up together when the followed one does */
static void get_up(BikePhys *b)
{
    BikePhys *v = W.followed > 0 && W.followed < 25 ? W.rider[W.followed] : 0;
    int me = v == b || !v;
    int i;
    if (!me && v->crashed)
        return;                         /* stay down while the followed rider is */
    if (b->driven) {
        b->push = 0.0f;
        b->speed = 0.0f;
        set_forward(b, 0.0f);
        wheel_from_speed(b);
        eng_sync(b);
        b->crash_timer = 0.0f;
        b->crashed = 0;
    } else {
        b->engine = b->cpu_speed = 0.0f;
        b->crash_timer = 0.0f;
        b->crashed = 0;
        b->hit_timer = 200.0f;
    }
    W.snap = 1;
    if (me)
        for (i = 1; i < 25; i++)
            if (W.rider[i] && W.rider[i] != b && W.rider[i]->crashed)
                get_up(W.rider[i]);
}

/* 0x4683e0: the crashed bike tumbles along tumble_dir; the body itself stays and slows */
static void tumble(BikePhys *b, float dt)
{
    float k = (float)(dt * 0.01f), step, old[3], hit[3], n[3], r[12];
    R f;
    int sec, i;
    float g;
    if (!b->crashed)
        return;
    b->crash_timer = (float)((R)b->crash_timer - dt);
    if (NEG(b->crash_timer)) {
        get_up(b);
        return;
    }
    step = (float)((R)dt * 0.01f * b->tumble_speed * 128.0);     /* k before its store */
    memcpy(old, b->tumble_pt, 12);
    for (i = 0; i < 3; i++)
        b->tumble_pt[i] = (float)((R)b->tumble_pt[i] + (float)((R)b->tumble_dir[i] * step));
    sec = walk(b->sec, b->tumble_pt);
    if (walls(sec, old, b->tumble_pt, hit, b->tumble_dir, n)) {
        memcpy(b->tumble_pt, hit, 12);
        sec = walk(b->sec, b->tumble_pt);
        b->tumble_speed = (float)(b->tumble_speed * 0.5);
        b->thud = 1;
    }
    g = (float)(int)road_y(sec, b->tumble_pt);
    if (b->tumble_pt[1] < (R)g + 8) {   /* on the ground */
        if (b->tumble_air)
            b->thud = 1;
        b->tumble_air = 0;
        b->tumble_pt[1] = g;
        b->tumble_dir[1] = (float)(-(R)b->tumble_dir[1] * (g_phys_cd ? 0.1 : 0.01));
        normalise(b->tumble_dir);
        lay_flat(b);
        b->roll[2] = 0.0f;
        b->tumble_speed = (float)(k * -4.166666666666667 + b->tumble_speed);
        if (b->tumble_speed < 0.0f)
            b->tumble_speed = 0.0f;
        if (b->tumble_dir[1] > 0.05) {
            debris(4, b->tumble_pt, 7, 4, b->tumble);
            b->events |= 0x40;
        }
    } else {                            /* in the air */
        float d[3], s2;
        R s = b->tumble_speed, rr;
        b->tumble_air = 1;
        d[0] = (float)(b->tumble_dir[0] * s);
        d[1] = (float)(b->tumble_dir[1] * s + dt * -0.0981);
        d[2] = (float)(b->tumble_dir[2] * s);
        s2 = (float)sqrt(((R)d[2] * d[2] + (R)d[0] * d[0]) + (R)d[1] * d[1]);
        rr = 1.0 / s2;
        for (i = 0; i < 3; i++)
            b->tumble_dir[i] = (float)(d[i] * rr);
        b->tumble_speed = (float)(s2 + k * -0.5555555555555556);
        if (b->tumble_speed < 0.0f)
            b->tumble_speed = 0.0f;
    }
    f = ((R)b->tumble_speed / b->tumble_speed0) * k;
    rot(r, (float)(b->roll[0] * f), (float)(b->roll[1] * f), (float)(b->roll[2] * f));
    if (g_phys_cd) {                    /* /0x489a25: written out, its own sum orders */
        const float *a = b->tumble;
        float t[12];
        t[0] = (float)(((R)a[2] * r[6] + (R)a[0] * r[0]) + (R)a[1] * r[3]);
        t[1] = (float)(((R)a[0] * r[1] + (R)a[1] * r[4]) + (R)a[2] * r[7]);
        t[2] = (float)(((R)a[1] * r[5] + (R)a[0] * r[2]) + (R)a[2] * r[8]);
        t[3] = (float)(((R)a[4] * r[3] + (R)a[5] * r[6]) + (R)a[3] * r[0]);
        t[4] = (float)(((R)a[4] * r[4] + (R)a[5] * r[7]) + (R)a[3] * r[1]);
        t[5] = (float)(((R)a[4] * r[5] + (R)a[3] * r[2]) + (R)a[5] * r[8]);
        t[6] = (float)(((R)a[7] * r[3] + (R)a[8] * r[6]) + (R)a[6] * r[0]);
        t[7] = (float)(((R)a[7] * r[4] + (R)a[8] * r[7]) + (R)a[6] * r[1]);
        t[8] = (float)(((R)a[7] * r[5] + (R)a[6] * r[2]) + (R)a[8] * r[8]);
        t[9] = t[10] = t[11] = 0.0f;    /* (the tumble's T is 0, as r's) */
        memcpy(b->tumble, t, sizeof t);
    } else
        matmul(b->tumble, b->tumble, r);
    if (b->tumble_speed < 0.2777777777777778 && bits(b->crash_timer) < 0x42c80000)
        get_up(b);
    else if (!b->driven) {
        b->engine = (float)(b->engine * 0.9);
        b->cpu_speed = (float)(b->cpu_speed * 0.9);
    } else {
        b->speed = (float)(b->speed * 0.9);
        set_forward(b, b->speed);
        wheel_from_speed(b);
        eng_sync(b);
        b->push = (float)(b->push * 0.9);
    }
    b->detail = 1;
}

/* ------------------------------------------------------------------ the road */

/* 0x46ad10: after a wall hit, the yaw that turns the bike's side onto the wall (0 when
   it already heads away; then the normal n is cleared) */
static float wall_turn(float n[3], float yaw)
{
    float r[12], fwd[3], left[3], nl[3], v[3], A[12], B[12], al, be, de;
    static const float z1[3] = { 0.0f, 0.0f, 1.0f }, xm[3] = { -1.0f, 0.0f, 0.0f },
                       zero[3] = { 0.0f, 0.0f, 0.0f };
    R c, s, c0 = g_cos[0], c3 = g_cos[0x3000];
    memcpy(nl, n, 12);
    rot(r, 0.0, yaw, 0.0);
    mulvec(fwd, z1, r);
    if (dot3(fwd, nl) > 0.0f) {
        n[0] = n[1] = n[2] = 0.0f;
        return 0.0f;
    }
    mulvec(left, xm, r);
    if (dot3(left, nl) < 0.0f)
        nl[0] = -nl[0], nl[1] = -nl[1], nl[2] = -nl[2];
    look(A, zero, nl);
    cos_sin(yaw, &c, &s);
    v[0] = (float)(c * c0 * -100.0f);
    v[1] = (float)(((1.0 + s) * c3 * c0) * -100.0f);
    v[2] = (float)((c3 * c3 - s * c0 * c0) * -100.0f);
    look(B, zero, v);
    al = wrap_ge((float)heading(A));
    be = wrap_ge((float)heading(B));
    if ((R)al + PI < be)
        be = (float)((R)be - TWO_PI);
    if ((R)al - PI > be)
        be = (float)((R)be + TWO_PI);
    de = wrap_ge((float)((R)be - al));
    if (de > PI)
        de = (float)((R)de - TWO_PI);
    return de;
}

/* the wheel points from the matrix: rear/front at -+48 x wheelbase along Z, 96 x
   (HauteurCadre - 2 x radius) along Y (inline in 0x468ca0) */
static void wheel_points(BikePhys *b)
{
    const float *m = b->m;
    R axe = carac(b, C_AXE), haut96 = (R)carac(b, C_HAUT) * 96.0f;
    float sr = (float)((R)carac(b, C_RAR) * -192.0f + haut96);
    float sf = (float)(haut96 + (R)carac(b, C_RAV) * -192.0f);
    R kr = axe * -48.0f, kf = axe * 48.0f;
    float krf = (float)kr, kff = (float)kf;
    b->rear[0] = (float)((((kr * m[6] + (R)m[3] * sr) + m[9]) + b->pos[0]));
    b->rear[1] = (float)((((R)m[4] * sr + (R)m[7] * krf) + m[10]) + b->pos[1]);
    b->rear[2] = (float)((((R)m[8] * krf + (R)m[5] * sr) + m[11]) + b->pos[2]);
    b->front[0] = (float)((((kf * m[6] + (R)m[3] * sf) + m[9]) + b->pos[0]));
    b->front[1] = (float)((((R)m[7] * kff + (R)m[4] * sf) + m[10]) + b->pos[1]);
    b->front[2] = (float)((((R)m[5] * sf + (R)m[8] * kff) + m[11]) + b->pos[2]);
}

/* the rows X = up x Z, Y = Z x X, both normalised (0x468ca0's two matrices) */
static void rows_from_z(BikePhys *b, const float z[3])
{
    static const float up[3] = { 0.0f, 1.0f, 0.0f };
    float x[3], y[3];
    cross(x, up, z);
    cross(y, z, x);
    normalise(x);
    normalise(y);
    memcpy(b->m, x, 12), memcpy(b->m + 3, y, 12), memcpy(b->m + 6, z, 12);
    b->m[9] = b->m[10] = b->m[11] = 0.0f;
}

/* 0x468ca0: the walls, the road under the bike, the air and the landing, the matrix from
   the road under the two axles, the wheel points, the wall spin */
static int contact(BikePhys *b, int flat)
{
    int old = b->sec, hit = 0, i;
    float move[3], dir[3], hp[3], h, tol;
    R l2, dz;
    b->rear_touch = b->front_touch = 0;
    memcpy(b->rear2, b->rear1, 12), memcpy(b->rear1, b->rear, 12);
    memcpy(b->front2, b->front1, 12), memcpy(b->front1, b->front, 12);
    memcpy(b->third2, b->third1, 12), memcpy(b->third1, b->third, 12);
    b->ground1 = b->ground;
    b->sec = walk(old, b->pos);
    for (i = 0; i < 2; i++)
        move[i] = (float)((R)b->pos[i] - b->pos1[i]);
    dz = (R)b->pos[2] - b->pos1[2];
    move[2] = (float)dz;
    l2 = sqrt((dz * move[2] + (R)move[1] * move[1]) + (R)move[0] * move[0]);
    memcpy(dir, move, 12);
    if (l2 != 0.0) {
        float l = (float)l2;
        R r = 1.0 / l;
        dir[0] = (float)(move[0] * r), dir[1] = (float)(dir[1] * r), dir[2] = (float)(r * dir[2]);
    }
    /* (0x468e7a: a copy-protection check in the championship, nothing to port) */
    if (old == b->sec)
        hit = walls(b->sec, b->pos1, b->pos, hp, dir, b->wall_n);
    else {
        float keep[3];
        memcpy(keep, dir, 12);
        hit = walls(old, b->pos1, b->pos, hp, dir, b->wall_n);
        if (!hit || walk(old, hp) != old) {
            memcpy(dir, keep, 12);
            hit = walls(b->sec, b->pos1, b->pos, hp, dir, b->wall_n);
        }
    }
    if (hit) {                          /* a wall */
        for (i = 0; i < 3; i++)
            b->pos[i] = (float)((R)b->wall_n[i] * 32.0f + hp[i]);
        b->sec = walk(b->sec, b->pos);
        memcpy(b->push_dir, dir, 12);
        b->wall_spin = wall_turn(b->wall_n, (float)-heading(b->m));
        if (!b->ground1) {              /* in the air */
            if ((bikephys_crt_rand() & 3) == 0) {
                crash(b, move);
                b->events |= 2;
            } else
                b->events |= 1;
            b->push = (float)(b->speed * 0.8);
            if (b->push < 0.5555555555555556)
                b->push = 0.5555556f;
            b->speed = 0.0f;
        } else {
            float a = (float)(((R)b->wall_n[2] * dir[2] + (R)b->wall_n[0] * dir[0])
                              + (R)b->wall_n[1] * dir[1]);
            R q;
            a = fabsf(a);
            if (a > 0.98 && b->speed > 27.77777777777778) {
                crash(b, dir);
                b->events |= 2;
            } else {
                int r = bikephys_crt_rand();
                if (((r & 3) == 0 && bits(a) > 0x3f000000 && b->speed > 22.22222222222222)
                    || (!ZERO(b->wheelie) && b->speed > 5.555555555555555)) {
                    crash(b, dir);
                    b->events |= 2;
                } else
                    b->events |= 1;
            }
            b->speed = (float)(b->speed * 0.75);
            q = ((R)dir[1] * move[1] + (R)dir[2] * move[2]) + (R)dir[0] * move[0];
            if (q < 0.0f) {             /* head on */
                R s = b->speed;
                b->push = (float)(s * 0.5);
                if (b->push < 1.3888888888888888)
                    b->push = 1.3888888f;
                b->speed = (float)(s * 0.3);
            } else {                    /* along the wall */
                memcpy(b->vdir, dir, 12);
                b->speed = (float)((1.0f - (R)a) * b->speed);
                b->speed = (float)(b->speed * 0.9);
                set_lateral(b, 0.0f);
            }
            if (b->speed < 0.2777777777777778)
                b->speed = 0.2777778f;
        }
        set_forward(b, b->speed);
        wheel_from_speed(b);
        eng_sync(b);
        b->hit_timer = 200.0f;
        b->turbo_req = 0;
        if (b->driven && b->human)
            b->turbo_held = 0;
    }
    keep_in(b->sec, b->pos);
    h = (float)road_y(b->sec, b->pos);
    tol = flat ? 8.0f : 32.0f;
    if ((R)tol + h < b->pos[1]) {       /* in the air */
        b->ground = 0;
        if (b->driven && b->human)
            b->turbo_held = 0;
        if (!ZERO(b->speed)) {          /* the nose follows the flight */
            float f[3], z[3];
            memcpy(f, b->vdir, 12);
            normalise(f);
            z[0] = (float)((R)b->m[9] + b->m[6]);
            z[1] = f[1];
            z[2] = (float)((R)b->m[8] + b->m[11]);
            normalise(z);
            rows_from_z(b, z);
        }
    } else {                            /* on the ground */
        float rr[3], ff[3], z[3];
        R axe = carac(b, C_AXE), kr = axe * -48.0f, kf = axe * 48.0f;
        float krf = (float)kr, kff = (float)kf;
        const float *m = b->m;
        int sf = b->sec, sr = b->sec;
        if (!b->ground1) {              /* landing */
            b->kick = (float)(((R)b->pos1[1] - h) * -2.0f);
            b->speed = (float)((R)b->kmh * 0.2777777777777778);
            set_forward(b, b->speed);
            wheel_from_speed(b);
            eng_sync(b);
            b->events |= 0x80;
        }
        b->pos[1] = h;
        b->ground = 1;
        b->air = 0.0f;
        b->rear_touch = b->front_touch = 1;
        rr[0] = (float)((kr * m[6] + m[9]) + b->pos[0]);
        rr[1] = (float)(((R)m[7] * krf + m[10]) + h);
        rr[2] = (float)(((R)m[8] * krf + m[11]) + b->pos[2]);
        ff[0] = (float)((kf * m[6] + m[9]) + b->pos[0]);
        ff[1] = (float)(((R)m[7] * kff + m[10]) + h);
        ff[2] = (float)(((R)m[8] * kff + m[11]) + b->pos[2]);
        if (bk(b)->perso == 0 || flat) {
            sf = walk(b->sec, ff);
            sr = walk(b->sec, rr);
        }
        ff[1] = (float)road_y(sf, ff);
        rr[1] = (float)road_y(sr, rr);
        for (i = 0; i < 3; i++)
            z[i] = (float)((R)ff[i] - rr[i]);
        normalise(z);
        rows_from_z(b, z);
    }
    wheel_points(b);
    b->rear_h = (float)road_y(walk(b->sec, b->rear), b->rear);
    if (b->ground) {
        b->front[1] = (float)road_y(walk(b->sec, b->front), b->front);
        b->rear[1] = b->rear_h;
    }
    if (!ZERO(b->wall_spin)) {          /* the wall's spin, 135 degrees a second */
        R d = bits(b->wall_spin) > 0 ? W.dt * 0.023561944901923447
                                     : W.dt * -0.023561944901923447;
        float df = (float)d;
        b->wall_spin = (float)(b->wall_spin - d);
        if (d > 0 ? NEG(b->wall_spin) : bits(b->wall_spin) > 0)
            b->wall_spin = 0.0f;
        turn_yaw(b, df);
    }
    return 1;
}

/* ------------------------------------------------------------------ after the contact */

/* 0x4667e0: the forward acceleration and the frame's dive from it */
static void dive(BikePhys *b)
{
    R fwd = longitudinal(b), s;
    b->accel = (float)((fwd - b->lon0) / (W.dt * 0.01));
    s = ((fwd - b->lon0) + b->half) * 0.5f;
    if (s != b->half) {
        float sf = (float)s;
        if (!ZERO(sf)) {
            float v = (float)(sf * -0.09817477042468103);
            if (fabsf(b->dive) < fabsf(v))
                b->dive = v;
        }
        if (b->dive > 0.19634954084936207)
            b->dive = 0.19634955f;
        if (b->dive < -0.19634954084936207)
            b->dive = -0.19634955f;
    }
    b->half = (float)s;
    b->rear_air = b->rear_touch ? 0.0f : (float)(W.dt * 6.272 + b->rear_air);
    b->front_air = b->front_touch ? 0.0f : (float)(W.dt * 6.272 + b->front_air);
}

/* 0x466940: the wheels turn by the way their points moved */
static void wheel_spin(BikePhys *b, unsigned keys)
{
    float f[3];
    R dx, dy, dz;
    int i;
    if (b->driven ? (keys & 2) : (b->keys1 & 2))
        return;                         /* braking locks the wheels */
    for (i = 0; i < 3; i++)
        f[i] = (float)((R)b->m[9 + i] + b->m[6 + i]);
    dx = (R)b->rear[0] - b->rear1[0], dy = (R)b->rear[1] - b->rear1[1];
    dz = (R)b->rear[2] - b->rear1[2];
    b->spin_rear = (float)(b->spin_rear + ((dz * f[2] + dy * f[1]) + dx * f[0])
                           / (((R)carac(b, C_RAR) * W.dt) * 96.0f));
    dx = (R)b->front[0] - b->front1[0], dy = (R)b->front[1] - b->front1[1];
    dz = (R)b->front[2] - b->front1[2];
    b->spin_front = (float)(b->spin_front + ((dx * f[0] + dy * f[1]) + dz * f[2])
                            / (((R)carac(b, C_RAV) * W.dt) * 96.0f));
}

/* 0x466a80: the rear spring and the frame's pitch on it */
static void suspension(BikePhys *b)
{
    float d = W.dt, k, a0, a, p, diff, as;
    R x;
    if (bits(d) > 0x40a00000)
        d = 5.0f;
    if (bits(d) < 0x40400000 && !g_phys_cd)     /* (/0x4877db: no floor on the CD) */
        d = 3.0f;
    b->amp = (float)((R)carac(b, C_RDEC) * b->amp);
    b->kick = (float)(d * 5.184 + b->kick);
    if (bits(b->kick) > 0)
        b->kick = 0.0f;
    k = (float)((R)b->kick / d);
    b->phase = (float)((R)b->amp / carac(b, C_RFAC) + b->phase);
    if (b->amp > k) {
        b->phase = (float)PI_F;
        b->amp = (float)(((R)k - b->amp) * 0.5f + b->amp);
    }
    if (carac(b, C_RMIN) > b->amp)
        b->amp = carac(b, C_RMIN);
    b->pitch = 0.0f;
    b->susp = (float)(sin(b->phase) * b->amp);
    a0 = (float)((frame_pitch(b) - b->brake_dive) + b->dive);
    a = a0;                             /* [0, 2pi], the second loop while > 2pi */
    if (NEG(a)) {
        for (x = a; x < 0.0; x += TWO_PI)
            ;
        a = (float)x;
    }
    if (a > TWO_PI) {
        for (x = a; x > TWO_PI; x -= TWO_PI)
            ;
        a = (float)x;
    }
    p = b->pitch_s;
    while (NEG(p))
        p = (float)(p + TWO_PI);
    while (p > TWO_PI)
        p = (float)(p - TWO_PI);
    diff = (float)((R)a - p);
    if (fabs((R)diff) > PI) {
        R A = p;
        if (A + PI < a)
            a = (float)(a - TWO_PI);
        if (A - PI > a)
            a = (float)(a + TWO_PI);
        diff = (float)((R)a - p);
    }
    b->pitch_s = (float)((R)diff / carac(b, C_RETARD) + p);
    as = carac(b, C_ANGSUSP);
    b->pitch = wrap_pi((float)((R)b->pitch_s - a0));
    if (b->pitch > as)
        b->pitch = as;
    if (b->pitch < -as)
        b->pitch = -as;
}

/* the wheelie's turn of the front point about the rear one by th = sin(wheelie) x -pi/4,
   about the frame m's X axis. A road bike (m = the travel matrix): inline in 0x469c60
   [asm 0x46a1a1..0x46a4fe], the rotation from the cosine table written out with the
   table's COS[0] and COS[0x3000]. A dirt bike (m = the bike's matrix) goes through
   0x40fe80 / 0x40fd40 / 0x410010 [that order not transcribed: approx] */
static void wheelie_front(BikePhys *b, const float *m, int road)
{
    R th = sin(b->wheelie) * -0.7853981633974483;
    float d[3], p[3], t[3];
    int i;
    for (i = 0; i < 3; i++)
        d[i] = (float)((R)b->front[i] - b->rear[i]);
    /* into the frame: m transposed, its translation -(row . T) */
    t[0] = (float)-(((R)m[1] * m[10] + (R)m[2] * m[11]) + (R)m[9] * m[0]);
    t[1] = (float)-(((R)m[5] * m[11] + (R)m[9] * m[3]) + (R)m[10] * m[4]);
    t[2] = (float)-(((R)m[9] * m[6] + (R)m[10] * m[7]) + (R)m[11] * m[8]);
    for (i = 0; i < 3; i++)
        p[i] = (float)((((R)d[2] * m[3 * i + 2] + (R)d[1] * m[3 * i + 1]) + (R)d[0] * m[3 * i])
                       + t[i]);
    if (road) {
        int k = (int)(th * TO_INDEX);
        R c = g_cos[k & 0x3fff], s = g_cos[(k - 0x1000) & 0x3fff], c0 = g_cos[0],
          e = g_cos[0x3000], q2;
        float q0, q1, q2f;
        q0 = (float)(((-(e * c0)) * p[1] + (c0 * c0) * p[0]) + (R)p[2] * e);
        q1 = (float)(((c0 * c - (e * s) * e) * p[1] + ((c + c0 * s) * e) * p[0])
                     + (-(c0 * s)) * p[2]);
        q2 = ((((c * e) * e) + c0 * s) * p[1] + ((s - c0 * c) * e) * p[0]) + (c0 * c) * p[2];
        q2f = (float)q2;
        b->front[0] = (float)((((q2 * m[6] + (R)q1 * m[3]) + (R)q0 * m[0]) + m[9]) + b->rear[0]);
        b->front[1] = (float)(((((R)q2f * m[7] + (R)q1 * m[4]) + (R)q0 * m[1]) + m[10])
                              + b->rear[1]);
        b->front[2] = (float)(((((R)q1 * m[5] + (R)q2f * m[8]) + (R)q0 * m[2]) + m[11])
                              + b->rear[2]);
    } else {
        float r[12], q[3], f[3];
        rot(r, (float)th, 0.0, 0.0);
        mulvec(q, p, r);
        mulvec(f, q, m);
        for (i = 0; i < 3; i++)
            b->front[i] = (float)((R)f[i] + b->rear[i]);
    }
}

/* 0x469c60: the frame's matrix (between where the bike points and where it goes), the
   wheel points on the road, and the lean */
static void frame_and_lean(BikePhys *b)
{
    static const float z1[3] = { 0.0f, 0.0f, 1.0f }, zero[3] = { 0.0f, 0.0f, 0.0f };
    float s = 0.0f, w, U[3], D[3], mix[3], Mt[12], A, B, e, e2;
    int d = bk(b)->perso != 0, i;      /* C+0x120, the dirt rider */
    if (!ZERO(b->hit_timer)) {
        b->hit_timer = (float)((R)b->hit_timer - W.dt);
        if (b->hit_timer < 0.0f)
            b->hit_timer = 0.0f;
        s = (float)(b->hit_timer * 0.005f);
    }
    w = (float)(1.0f - ((R)b->speed / b->top) * 8.0f);
    if (w < 0.0f)
        w = 0.0f;
    if (s > w)
        w = s;
    mulvec(U, z1, b->m);
    memcpy(D, b->vdir, 12);
    if (dot3(D, U) < 0.0)
        D[0] = -D[0], D[1] = -D[1], D[2] = -D[2];
    {
        float w1 = (float)(1.0f - (R)w);
        for (i = 0; i < 3; i++)
            mix[i] = (float)((R)(float)((R)U[i] * w) + (float)((R)D[i] * w1));
    }
    look(Mt, zero, mix);
    if (!d) {                           /* road: the wheel points from Mt, on the road */
        R a = (R)carac(b, C_HAUT) * 96.0f;
        float br = (float)((R)carac(b, C_RAR) * -192.0f + a), cr = (float)((R)carac(b, C_AXE) * -48.0f);
        float bf = (float)((R)carac(b, C_RAV) * -192.0f + a), cf = (float)((R)carac(b, C_AXE) * 48.0f);
        float x[3], y[3];
        static const float up[3] = { 0.0f, 1.0f, 0.0f };
        int sf, sr;
        if (g_phys_cd) {                /* /0x48b03f: through a float, the rear by a routine */
            float a32 = (float)a, v[3], t[3];
            R kf = (R)carac(b, C_AXE) * 48.0f;
            br = (float)((R)carac(b, C_RAR) * -192.0f + a32);
            bf = (float)((R)carac(b, C_RAV) * -192.0f + a32);
            v[0] = 0.0f, v[1] = br, v[2] = cr;
            mulvec(t, v, Mt);
            for (i = 0; i < 3; i++)
                b->rear[i] = (float)((R)t[i] + b->pos[i]);
            b->front[0] = (float)(((kf * Mt[6] + (R)bf * Mt[3]) + Mt[9]) + b->pos[0]);
            b->front[1] = (float)(((((R)cf * Mt[7] + (R)bf * Mt[4])) + Mt[10]) + b->pos[1]);
            b->front[2] = (float)((((R)cf * Mt[8] + (R)bf * Mt[5]) + Mt[11]) + b->pos[2]);
        } else
            for (i = 0; i < 3; i++) {
                b->rear[i] = (float)((((R)cr * Mt[6 + i] + (R)br * Mt[3 + i]) + Mt[9 + i]) + b->pos[i]);
                b->front[i] = (float)((((R)cf * Mt[6 + i] + (R)bf * Mt[3 + i]) + Mt[9 + i]) + b->pos[i]);
            }
        sf = walk(b->sec, b->front);
        sr = walk(b->sec, b->rear);
        b->front[1] = (float)road_y(sf, b->front);
        b->rear[1] = (float)road_y(sr, b->rear);
        if (bits(b->wheelie) > 0)
            wheelie_front(b, Mt, 1);
        for (i = 0; i < 3; i++)
            D[i] = (float)((R)b->front[i] - b->rear[i]);
        normalise(D);
        cross(x, up, D);
        cross(y, D, x);
        normalise(x);
        normalise(y);
        memcpy(b->frame, x, 12), memcpy(b->frame + 3, y, 12), memcpy(b->frame + 6, D, 12);
        b->frame[9] = b->frame[10] = b->frame[11] = 0.0f;
    } else {                            /* dirt: the points stay 0x468ca0's */
        if (bits(b->wheelie) > 0)
            wheelie_front(b, b->m, 0);
        look(b->frame, b->rear, b->front);
    }
    A = (float)heading(Mt);             /* the lean */
    B = (float)heading(b->m);
    A = wrap_ge(A);
    B = wrap_ge(B);
    if ((R)A + PI < B)
        B = (float)(B - TWO_PI);
    if ((R)A - PI > B)
        B = (float)(B + TWO_PI);
    if (b->driven && b->human && d && !g_phys_cd) {     /* a player's dirt bike */
        float v = bits(b->speed) > 0x42480000 ? 50.0f : b->speed, L, k;
        e = wrap_ge((float)((R)B - A));
        if (e > PI)
            e = (float)(e - TWO_PI);
        L = (float)atan(((-(R)e / W.frame_ms) * v) * 1.445912045170292);
        if (L > 1.04)
            L = 1.04f;
        else if (L < -1.04)
            L = -1.04f;
        k = (float)((R)W.frame_ms * 0.02 > 1.0 ? 1.0 : (R)W.frame_ms * 0.02);
        b->lean = (float)(((R)L - b->lean1) * k + b->lean1);
        b->lean1 = b->lean;
        return;
    }
    e = wrap_ge((float)(((R)B - A) * (float)(1.0f - (R)w)));
    e2 = wrap_ge((float)(e * 2.0f));
    if (e2 > PI)
        e2 = (float)(e2 - TWO_PI);
    if (e2 > 1.0471975511965976)
        e2 = 1.0471976f;
    if (e2 < -1.0471975511965976)
        e2 = -1.0471976f;
    if (d)                              /* a computer's dirt bike */
        b->lean = (float)(((R)e2 / (3.0 - ((R)b->speed * 0.045) * 0.5)) * -1.3);
    else if (!b->driven || !b->human || W.replay == 1 || W.replay == 2 || g_phys_cd)
        b->lean = -e2;                  /* (/0x48ba34: on the CD, everyone) */
    else {
        float f = W.frame_ms, L;
        if (!b->analog && bits(f) < 0x41a00000)
            f = 20.0f;
        L = (float)(((R)e2 / f) * -30.0f);
        if (L > 0.3)
            L = (float)(L * 1.4 - 0.11999999999999997);
        else if (L < -0.3)
            L = (float)(L * 1.4 + 0.11999999999999997);
        if (L > 1.04)
            L = 1.04f;
        else if (L < -1.04)
            L = -1.04f;
        b->lean = L;
    }
}

/* 0x48e180: a computer bike's gear from its speed; the fraction of that gear's range */
static float auto_gear(BikePhys *b)
{
    static const double road[5] = { 0.18, 0.33, 0.48, 0.62, 0.74 },
                        dirt_[4] = { 0.15, 0.3, 0.45, 0.75 };
    static const float froad[6] = { 0.23f, 0.38f, 0.53f, 0.67f, 0.79f, 1.0f },
                       fdirt[5] = { 0.2f, 0.35f, 0.5f, 0.8f, 1.0f };
    R v = b->engine, top = carac(b, C_RSM), q;
    int d = dirt(b), n = d ? 4 : 5, g;
    float f;
    for (g = 0; g < n; g++)
        if (v < top * (d ? dirt_[g] : road[g]))
            break;
    b->gear = g + 1;
    f = d ? fdirt[g] : froad[g];
    q = (float)(v / ((R)f * top));
    return q < 0.0 ? 0.0f : q > 1.0 ? 1.0f : (float)q;
}

/* 0x467930: the speedometer from the rear wheel's move, the rev counter */
static void dials(BikePhys *b)
{
    R dx = (R)b->rear[0] - b->rear1[0], dy = (R)b->rear[1] - b->rear1[1],
      dz = (R)b->rear[2] - b->rear1[2];
    R dist = sqrt((dz * dz + dy * dy) + dx * dx);
    int old = b->kmh, kmh = (int)(dist / (W.dt * 0.01) * 0.028125);
    b->kmh_shown = b->kmh = kmh;
    if (!b->driven) {
        b->cpu_accel = (float)((((R)(float)kmh - (float)old) / W.dt * 100.0 + b->cpu_accel) * 0.5);
        if (bits(b->cpu_accel) < 0 && b->cpu_accel < -50.0f)
            b->cpu_accel = -50.0f;
        if (b->cpu_accel > 40.0f)
            b->cpu_accel = 40.0f;
        b->kmh = (int)((R)b->cpu_accel * W.dt * 0.01) + old;
        if (b->kmh < 0)
            b->kmh = 0;
    }
    if (b->driven && b->human)
        b->rpm = (int)((R)carac(b, C_RM) / carac(b, C_MVM) * b->engine);
    else {                              /* gear and revs from the shown speed */
        float e = b->engine, r;
        b->engine = (float)((R)b->kmh * 0.2777777777777778);
        if (NEG(b->engine))
            b->engine = 0.0f;
        r = auto_gear(b);
        b->engine = e;
        b->rpm = (int)((R)r * carac(b, C_RM));
        if (b->rpm < 800.0)
            b->rpm = 800;
    }
}

/* 0x46d340: which of three lanes 240 units wide from the racing line */
static int lane(const BikePhys *b)
{
    const PSec *s;
    R l, line;
    int k;
    if (s_nsec <= 0)
        return -1;
    s = &s_sec[wrap_sec(b->sec)];
    l = (((R)b->pos[0] - s->pos[0]) * s->m[0] + ((R)b->pos[1] - s->pos[1]) * s->m[1]
         + ((R)b->pos[2] - s->pos[2]) * s->m[2]);
    line = s->line;
    for (k = 0; k < 3; k++) {
        float c = (float)((R)(240 * k) + line);
        int hi = (int)((R)c + 120.0f), lo = (int)((R)c - 120.0f);
        if (lo < l && l < hi)
            return k;
    }
    if (l <= (int)(line - 120.0f))
        return 0;
    if (l > (int)(line + 600.0f))
        return 2;
    return -1;
}

/* ------------------------------------------------------------------ the step */

/* 0x465fa0: what the step keeps of the last one */
static void start_step(BikePhys *b)
{
    memcpy(b->m1, b->m, sizeof b->m);
    b->lon0 = (float)longitudinal(b);
    memcpy(b->pos2, b->pos1, 12), memcpy(b->pos1, b->pos, 12);
    b->pos_dt = W.dt;
    b->sliding1 = b->sliding;
    memcpy(b->rear1, b->rear, 12);
    memcpy(b->front1, b->front, 12);
    b->ground1 = b->ground;
    if (NEG(b->speed))
        b->speed = 0.0f;
}

/* 0x45d540 -> 0x45d580 */
void bikephys_step(BikePhys *b, unsigned keys, int joy_x, int joy_y, float dt, int countdown,
                   int time_left)
{
    int flat;
    (void)time_left;
    W.dt = dt;
    W.countdown = countdown;
    if (!b->driven)
        return;                         /* the computer riders' model (0x46bf80): not here */
    b->detail = 0;
    b->events = 0;
    b->kick = 0.0f;
    if (bits(dt) < 1)
        dt = 1.0f;
    start_step(b);
    if (!(keys & 2) || bits(b->speed) <= 0) {   /* the brake's dive comes back */
        if (NEG(b->brake_dive)) {
            b->brake_dive = (float)(W.dt * 0.0025 + b->brake_dive);     /* may pass 0 */
        } else
            b->brake_dive = 0.0f;
    }
    if (b->dive > 0.01)                 /* 0.01 a step, whatever dt */
        b->dive = (float)(b->dive - 0.01);
    else if (b->dive < -0.01)
        b->dive = (float)(b->dive + 0.01);
    else
        b->dive = 0.0f;
    b->sliding = b->ground && fabs(lateral(b)) >= 0.04;
    if (!W.countdown) {
        if (b->driven && b->human)
            b->turbo_req = b->turbo_held;
        turbo(b, dt);
    }
    if (!b->crashed)
        throttle(b, keys, joy_y, dt);
    if (!W.countdown && !b->crashed)
        steering(b, keys, joy_x, dt);
    flat = bk(b)->perso && bends(b->sec, 8) == 0;
    integrate(b, dt, flat);
    if (ZERO(b->push))
        b->forced = b->forced_keys = 0;
    slide(b, dt);
    contact(b, flat);
    gearbox(b, keys);
    dive(b);
    wheel_spin(b, keys);
    suspension(b);
    if (W.shadow)                       /* 0x461aa0 */
        W.shadow(b);
    frame_and_lean(b);
    tumble(b, dt);
    if (!b->crashed)                    /* [approx] the drawer 0x460dbe keeps its matrix */
        memcpy(b->tumble, b->m, sizeof b->m);
    /* 0x467b30 / 0x45d810: the rider's pose (drawing); its end moves the turbo latch */
    if (W.pose)
        W.pose(b);
    b->turbo_pressed1 = b->turbo_pressed;
    b->turbo_pressed = 0;
    if (W.effects)                      /* 0x466d70: they draw stream 1 */
        W.effects(b, keys);
    dials(b);
    b->lane = lane(b);
    b->keys1 = (int)keys;
}

/* ------------------------------------------------------------------ the start */

/* the wheel points as 0x45c7c0 and 0x47ecd0 put them: pos + (0, -96 r, -+48 axe) M */
static void wheels_from_matrix(BikePhys *b)
{
    const float *m = b->m;
    float yr = (float)((R)carac(b, C_RAR) * -96.0f), yf = (float)((R)carac(b, C_RAV) * -96.0f);
    float zr = (float)((R)carac(b, C_AXE) * -48.0f), zf = (float)((R)carac(b, C_AXE) * 48.0f);
    int i;
    for (i = 0; i < 3; i++) {
        b->rear[i] = (float)((((R)m[3 + i] * yr + (R)m[6 + i] * zr) + m[9 + i]) + b->pos[i]);
        b->front[i] = (float)((((R)m[3 + i] * yf + (R)m[6 + i] * zf) + m[9 + i]) + b->pos[i]);
    }
}

void bikephys_init(BikePhys *b, int bike, const float m[3][3], const float pos[3],
                   int section, int human, int automatic)
{
    int i;
    memset(b, 0, sizeof *b);
    /* 0x45bf70 with the body's constructor 0x48bce0 */
    b->driven = 1;
    b->human = human;
    b->bike = bike;
    b->gear = 1;
    b->brakes_grip = 1;
    b->power = 1.0f;
    b->automatic = automatic;           /* 0x411010: 0x4dc0a0 for player 1, else 1 */
    b->top = carac(b, C_RSM);
    if (human && !automatic)
        b->top = (float)(b->top + 2.0);
    /* 0x45bdf0, 0x45bb30(obj, 1) */
    b->radius = FLT_MAX;
    b->ground = b->ground1 = 1;
    b->lane = 0;
    /* the grid (0x47bf70): the section's axes, then 0x45c7c0 */
    for (i = 0; i < 3; i++)
        memcpy(b->m + 3 * i, m[i], 12);
    b->m[9] = b->m[10] = b->m[11] = 0.0f;
    memcpy(b->m1, b->m, sizeof b->m);
    memcpy(b->frame, b->m, sizeof b->m);
    memcpy(b->tumble, b->m, sizeof b->m);
    memcpy(b->pos, pos, 12), memcpy(b->pos1, pos, 12), memcpy(b->pos2, pos, 12);
    wheels_from_matrix(b);
    memcpy(b->rear1, b->rear, 12), memcpy(b->rear2, b->rear, 12);
    memcpy(b->front1, b->front, 12), memcpy(b->front2, b->front, 12);
    for (i = 0; i < 3; i++)
        b->third[i] = (float)(((R)b->m[3 + i] * (float)((R)carac(b, C_AXE) * 48.0f)
                               + b->m[9 + i]) + pos[i]);
    memcpy(b->third1, b->third, 12), memcpy(b->third2, b->third, 12);
    b->sec = walk(section, pos);
    b->pos_dt = W.dt;
}

/* ------------------------------------------------------------------ the keys */

/* 0x431250 (down) and 0x432700 (up) */
void bikephys_key(BikePhys *b, unsigned *keys, int action, int down, int race_running)
{
    static int accel_held;              /* 0x4e8964 */
    if (W.replay)
        return;
    if (down)
        switch (action) {
        case 0: accel_held = 1; *keys = (*keys & ~2u) | 1; break;
        case 1: if (race_running) *keys = (*keys & ~1u) | 2; break;
        case 2: if (race_running) *keys = (*keys & ~8u) | 4; break;
        case 3: if (race_running) *keys = (*keys & ~4u) | 8; break;
        case 4: if (!W.countdown && race_running) *keys |= 16; break;
        case 5: if (!W.countdown && race_running) *keys |= 32; break;
        case 6: if (race_running) b->turbo_held = b->turbo_pressed = 1; break;
        }
    else
        switch (action) {
        case 0: accel_held = 0; *keys &= ~1u; break;
        case 1: *keys &= ~2u; if (accel_held) *keys |= 1; break;
        case 2: *keys &= ~4u; break;
        case 3: *keys &= ~8u; break;
        case 4: *keys &= ~16u; break;
        case 5: *keys &= ~32u; break;
        case 6: b->turbo_held = 0; break;
        }
}

/* ------------------------------------------------------------------ bike against bike */
/* 0x47ecd0 [from the decompiler: the order of its sums is not the game's x87 order] */

/* 0x411f00: when (0..1 of this step) two circles of radius 48 moving from p0 to p1 touch;
   -1 when they do not */
static R sweep(const float *a0, const float *b0, const float *a1, const float *b1)
{
    R d[3], v[3], A = 0, B = 0, C = 0, disc;
    int i;
    for (i = 0; i < 3; i++) {
        d[i] = (R)a0[i] - b0[i];
        v[i] = ((R)a1[i] - a0[i]) - ((R)b1[i] - b0[i]);
        A += v[i] * v[i], B += d[i] * v[i], C += d[i] * d[i];
    }
    if (A < 1e-5)
        return -1.0;
    B *= 2.0f;
    C -= 96.0 * 96.0;
    disc = B * B - 4.0f * A * C;
    if (disc < 0.0)
        return -1.0;
    return (-B - sqrt(disc)) / (2.0f * A);
}

static void turbo_off(BikePhys *b)              /* 0x466220 */
{
    b->turbo_req = 0;
    if (b->driven && b->human)
        b->turbo_held = 0;
}

static void unit(float o[3], const float v[3])
{
    memcpy(o, v, 12);
    normalise(o);
}

void bikephys_collide(BikePhys *all[], int n, float dt)
{
    int i, j, k;
    if (!W.mode)
        return;
    W.dt = dt;
    for (i = 1; i < n; i++)
        for (j = i + 1; j < n; j++) {
            BikePhys *p[2] = { all[i], all[j] }, *A, *B;
            float s[2], nn[2][3], sA, sB, nA[3], nB[3], mix[3];
            int d, hard, fall, headon;
            R t;
            if (!p[0] || !p[1])
                continue;
            d = p[1]->sec - p[0]->sec;      /* 0x46c9f0: j at most 3 sections ahead */
            if (d < 0)
                d += s_nsec;
            if (d >= 4)
                continue;
            t = sweep(p[0]->pos1, p[1]->pos1, p[0]->pos, p[1]->pos);
            if (t < 0.05 || t > 1.0)
                continue;
            for (k = 0; k < 2; k++) {
                float dd[3];
                R l;
                int c;
                for (c = 0; c < 3; c++)
                    dd[c] = (float)((R)p[k]->pos[c] - p[k]->pos1[c]);
                /* not 0x40fcd0: (y y + z z) + x x, the length kept as a double */
                l = sqrt(((R)dd[1] * dd[1] + (R)dd[2] * dd[2]) + (R)dd[0] * dd[0]);
                s[k] = (float)(l / dt * 0.78125);
                memcpy(nn[k], dd, 12);
                if (l != 0.0) {
                    R r = 1.0 / l;
                    for (c = 0; c < 3; c++)
                        nn[k][c] = (float)(dd[c] * r);
                }
            }
            k = s[0] > s[1] ? 0 : 1;
            A = p[k], B = p[1 - k];
            sA = s[k], sB = s[1 - k];
            memcpy(nA, nn[k], 12), memcpy(nB, nn[1 - k], 12);
            {
                float ds = fabsf(sA - sB);
                hard = bits(ds) > 0x41c80000
                    || (ds > 12.5 && (fabsf(A->lean) > 0.6981317 || fabsf(B->lean) > 0.6981317))
                    || !ZERO(A->wheelie) || !ZERO(B->wheelie);
            }
            fall = sA > 22.222 || sB > 22.222 || !ZERO(A->wheelie) || !ZERO(B->wheelie);
            if ((!A->ground || !B->ground) && (A->driven || B->driven))
                hard = fall = 1;
            if (bikephys_crt_rand() & 3)
                hard = 0;
            headon = dot3(nA, nB) < 0.0;
            for (k = 0; k < 2; k++) {       /* back to where they touched */
                BikePhys *q = k ? B : A;
                const float *nq = k ? nB : nA;
                int c;
                for (c = 0; c < 3; c++)
                    q->pos[c] = (float)((((R)q->pos[c] - q->pos1[c]) * t + q->pos1[c]) - nq[c]);
                wheels_from_matrix(q);
            }
            A->hit_timer = 200.0f;          /* the faster one */
            turbo_off(A);
            if (A->driven) {
                if (!headon) {
                    if (hard) {
                        if (A->speed > 22.22222222222222)   /* 80 km/h at most */
                            A->speed = 22.222221f;
                        if (sB > 22.22222222222222)
                            sB = 22.222221f;
                        crash(A, nA);
                        A->events |= 8;
                    } else
                        A->events |= 4;
                    A->speed = (float)(sB * 0.95);
                } else {
                    memcpy(A->push_dir, nB, 12);
                    A->push = 5.5555553f;
                    if (fall) {
                        crash(A, nA);
                        A->events |= 0x20;
                    } else
                        A->events |= 0x10;
                    A->speed = 0.0f;
                }
                set_forward(A, A->speed);
                wheel_from_speed(A);
                eng_sync(A);
            } else {
                if (!headon) {
                    if (hard) {
                        if (A->speed > 22.22222222222222)   /* 80 km/h at most */
                            A->speed = 22.222221f;
                        if (sB > 22.22222222222222)
                            sB = 22.222221f;
                        crash(A, nA);
                        A->events |= 8;
                    } else
                        A->events |= 4;
                    A->speed = (float)(sB * 0.9);
                } else {
                    memcpy(A->vdir, nB, 12);
                    if (fall) {
                        crash(A, nA);
                        A->events |= 0x20;
                    } else
                        A->events |= 0x10;
                    A->speed = 5.5555553f;
                }
                A->engine = A->cpu_speed = A->speed;
            }
            /* with sB as A's crash may have cut it; the game spills sB nB[2] to a float */
            mix[0] = (float)((R)sA * nA[0] + (R)sB * nB[0]);
            mix[1] = (float)((R)sA * nA[1] + (R)sB * nB[1]);
            mix[2] = (float)((R)sA * nA[2] + (float)((R)sB * nB[2]));
            normalise(mix);
            turbo_off(B);                   /* the slower one */
            if (B->driven) {
                if (!headon) {
                    unit(B->push_dir, nA);
                    B->push = (float)((R)sA * 1.1 - B->speed);
                    if (NEG(B->push))
                        B->push = 0.0f;
                    B->events |= 4;
                } else {
                    memcpy(B->push_dir, mix, 12);
                    if (fall) {
                        crash(B, nB);
                        B->events |= 0x20;
                    } else
                        B->events |= 0x10;
                    B->push = (float)((R)sA - sB);
                    if (B->push < 5.555555555555555)
                        B->push = 5.5555553f;
                    B->speed = 0.0f;
                    set_forward(B, 0.0f);
                    wheel_from_speed(B);
                    eng_sync(B);
                }
            } else {
                memcpy(B->vdir, mix, 12);
                if (!headon) {
                    if (hard) {
                        crash(B, mix);
                        B->events |= 8;
                    } else
                        B->events |= 4;
                    B->speed = sA;
                } else {
                    if (fall) {
                        crash(B, mix);
                        B->events |= 0x20;
                    } else
                        B->events |= 0x10;
                    B->speed = (float)((R)sA - sB);
                }
                B->speed = (float)(B->speed * 1.1);
                if (B->speed < 5.555555555555555)
                    B->speed = 5.5555553f;
                B->engine = B->cpu_speed = B->speed;
            }
            B->hit_timer = 200.0f;
            frame_and_lean(A);
            frame_and_lean(B);
        }
}

/* ------------------------------------------------------------------ for cpu.c */
/* The same routines, reached by the computer riders' step (bikephys_int.h) */
#include "bikephys_int.h"

int bp_nsec(void) { return s_nsec; }
int bp_wrap(int sec) { return wrap_sec(sec); }
const float *bp_sec_pos(int sec) { return s_sec[wrap_sec(sec)].pos; }
const float *bp_sec_m(int sec) { return s_sec[wrap_sec(sec)].m; }
float bp_sec_line(int sec) { return s_sec[wrap_sec(sec)].line; }
int bp_walk(int hint, const float p[3]) { return walk(hint, p); }
double bp_road_y(int sec, const float p[3]) { return road_y(sec, p); }
int bp_walls(int sec, const float a[3], const float b[3], float hit[3], float v[3],
             float n[3])
{
    return s_nsec > 0 && walls(sec, a, b, hit, v, n);
}
int bp_bends(int sec, int n) { return bends(sec, n); }
void bp_mulvec(float o[3], const float v[3], const float *m) { mulvec(o, v, m); }
void bp_cross(float o[3], const float a[3], const float b[3]) { cross(o, a, b); }
void bp_normalise(float v[3]) { normalise(v); }
void bp_look(float *o, const float a[3], const float b[3]) { look(o, a, b); }
double bp_heading(const float *m) { return heading(m); }
float bp_wrap_ge(float a) { return wrap_ge(a); }
void bp_turn_yaw(BikePhys *b, float yaw) { turn_yaw(b, yaw); }
float bp_carac(const BikePhys *b, int off) { return carac(b, off); }
int bp_perso(const BikePhys *b) { return bk(b)->perso; }
void bp_start_step(BikePhys *b) { start_step(b); }
void bp_turbo(BikePhys *b, float dt) { turbo(b, dt); }
void bp_frame_and_lean(BikePhys *b) { frame_and_lean(b); }
/* 0x48df90: the engine from the wheel speed, the gear found to suit it */
static float engine_finding_gear(BikePhys *b, float *wheel, float e)
{
    R mvm = carac(b, C_MVM), vmb = carac(b, C_VMB);
    float nb = carac(b, C_NBSPEED), out;
    if (b->shift)
        return e;
    do {
        int g = b->gear;
        out = (float)(mvm / ((R)ratio(b, g) * vmb) * *wheel);
        if ((R)out < 0.1) {
            if (g < 2)
                break;
            b->gear = g - 1;
        } else if (mvm < out) {
            if ((float)g >= nb)
                break;
            b->gear = g + 1;
        }
    } while (mvm < out);
    if (NEG(out))
        return 0.0f;
    if (mvm < out) {
        out = (float)mvm;
        *wheel = (float)((R)ratio(b, b->gear) / mvm * carac(b, C_VMB) * out);
    }
    return out;
}

void bikephys_winner_take(BikePhys *b)
{
    b->driven = 0;
    b->engine = b->cpu_speed = b->speed;
    b->top = (float)((R)b->top * 0.8);
    b->keys1 = 0;
}

void bikephys_winner_give_back(BikePhys *b, float top)
{
    b->driven = 1;
    b->top = top;
    b->shift = b->pending = 0;
    b->speed = b->engine;
    wheel_from_speed(b);
    b->engine = engine_finding_gear(b, &b->wheel, b->engine);
}

void bp_rot(float *m, double a0, double a1, double a2) { rot(m, a0, a1, a2); }
double bp_lateral(const BikePhys *b) { return lateral(b); }
void bp_wheel_spin(BikePhys *b) { wheel_spin(b, (unsigned)b->keys1); }
void bp_suspension(BikePhys *b) { suspension(b); }
void bp_tumble(BikePhys *b, float dt) { tumble(b, dt); }
void bp_dials(BikePhys *b) { dials(b); }
int bp_lane(const BikePhys *b) { return lane(b); }
void bikephys_crash(BikePhys *b, const float v[3]) { crash(b, v); }
double bikephys_frame_pitch(const BikePhys *b) { return frame_pitch(b); }
