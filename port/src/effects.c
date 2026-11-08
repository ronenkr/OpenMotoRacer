/* Dust, spray, smoke and tyre marks (NOTES §4.15.10): what a bike's step starts (0x466d70,
   0x467c40), the 128 pieces each moving by itself (0x457620, 0x4576a0, 0x4b0730) and drawn
   as upright squares turned to the camera (0x4b0be0), the 16 strips of tyre marks (0x457200,
   0x4573e0). Pictures on the family's page 1. They draw the game's random stream 1 as the
   game does; nothing here goes back to the physics. */
#include "effects.h"
#include "land.h"
#include "r3d.h"
#include "rand.h"
#include <math.h>
#include <string.h>
#include "fastfloat.h"      /* (last: the ESP32's single precision) */

#define PIECES 128
#define STRIPS 16
#define TO_INDEX 2607.594587618272      /* 0x4d2080 */

typedef struct {                /* 0x104 bytes at 0x52c498 */
    int type;                   /* +0: 1..4 the ground's spray, 5 smoke */
    int live;                   /* +0xfc */
    float pos[3], before[3];    /* +4 */
    float dir[3];               /* +0x10: smoke its speed, spray its direction */
    float speed;                /* +0x34 */
    float life;                 /* +0x3c: smoke's dt units */
    int size, height, wide;     /* +0xf0, +0xf4, +0xf8 */
    float v[4][3];              /* +0xc0 */
    Face f[2];                  /* +0x40, +0x80 */
} Piece;

typedef struct {                /* 0x1608 bytes at 0x542a28 */
    float v[128][3];            /* +0x1000 */
    int nf, nv;                 /* +0x1600, +0x1604 */
} Strip;

static Piece g_piece[PIECES];
static Strip g_strip[STRIPS];
static Face g_strip_face[64];   /* 0x457120: the same in every strip */
static int g_next_strip;        /* 0x4e9bd0 */
static float g_spray_clock;     /* 0x4ea508: the followed bike's; the spray 30 of each 50 */

static unsigned pack(int r, int g, int b)       /* 0x45a2e0 */
{
    return (unsigned)(r << 11 | g << 6 | b);
}

/* 0x422d20 + 0x423060 + 0x4232e0: a square, its other side the other way round */
static void square(Piece *p, int kind, const int uv[4][2], unsigned colour)
{
    int i, k;
    for (i = 0; i < 2; i++) {
        Face *f = &p->f[i];
        memset(f, 0, sizeof *f);
        f->kind = kind;
        f->colour = (int)colour;
        f->page = 1;
        f->n = 4;
        for (k = 0; k < 4; k++) {
            f->idx[k] = i ? 3 - k : k;
            f->ut[k] = (float)uv[k][0];
            f->vt[k] = (float)uv[k][1];
        }
    }
}

static Piece *free_piece(void)
{
    int i;
    for (i = 0; i < PIECES; i++)
        if (!g_piece[i].live)
            return &g_piece[i];
    return 0;
}

/* 0x457620 -> 0x4afe90: smoke; each move (0x4b0730, the first before anything is drawn) puts
   the same frame of the page's strip on it: the frames are never stepped through */
static void smoke(int size, const float pos[3], const float v[3], int life)
{
    static const int uv[4][2] = { { 224, 160 }, { 255, 160 }, { 255, 191 }, { 224, 191 } };
    Piece *p = free_piece();
    if (!p)
        return;
    p->type = 5;
    p->live = 1;
    p->size = size;
    p->height = 0;
    memcpy(p->pos, pos, sizeof p->pos);
    memcpy(p->before, pos, sizeof p->before);
    memcpy(p->dir, v, sizeof p->dir);
    p->life = (float)life;
    square(p, 0x44, uv, pack(0x1f, 0x3f, 0x1f));
}

/* 0x4576a0 -> 0x4b00e0: the ground's spray, m = the ground's kind 1..4 */
static void spray(int m, int size, const float pos[3], const float dir[3], float speed)
{
    static const int uv[4][4][2] = {
        { { 113, 174 }, { 186, 174 }, { 186, 230 }, { 113, 230 } },
        { { 2, 109 }, { 108, 109 }, { 108, 206 }, { 2, 206 } },
        { { 113, 109 }, { 186, 109 }, { 186, 170 }, { 113, 170 } },
        { { 133, 43 }, { 216, 43 }, { 216, 105 }, { 133, 106 } },
    };
    static const int colour[4][3] = { { 9, 8, 6 }, { 0x14, 0xd, 5 }, { 0, 0xb, 4 }, { 0x13, 0x14, 0x15 } };
    Piece *p;
    if (m < 1 || m > 4 || !(p = free_piece()))
        return;
    p->type = m;
    p->live = 1;
    p->wide = size;
    p->size = size / 2;
    p->height = 1;
    memcpy(p->pos, pos, sizeof p->pos);
    memcpy(p->before, pos, sizeof p->before);
    memcpy(p->dir, dir, sizeof p->dir);
    p->speed = speed;
    p->life = 40.0f;
    square(p, 0x26, uv[m - 1], pack(colour[m - 1][0], colour[m - 1][1], colour[m - 1][2]));
}

/* 0x4b0730 */
static void piece_step(Piece *p, float dt)
{
    int i;
    float dtf = dt;
    memcpy(p->before, p->pos, sizeof p->before);
    if (p->type == 5) {
        int s = p->size * 2;
        /* cut to an int: at the 16 ms a frame of -FrameRateMax60 (dt 1.6) 0.96 is lost and the
           smoke stays its size; at the 33 ms of the slow machines (dt 3.3) it grows by 1 a move */
        p->size = (int)(dtf * 0.6 + p->size);
        p->v[0][0] = (float)-s, p->v[0][1] = (float)s, p->v[0][2] = 0.0f;
        p->v[1][0] = (float)s, p->v[1][1] = (float)s, p->v[1][2] = 0.0f;
        p->v[2][0] = (float)s, p->v[2][1] = (float)-s, p->v[2][2] = 0.0f;
        p->v[3][0] = (float)-s, p->v[3][1] = (float)-s, p->v[3][2] = 0.0f;
        p->dir[1] = (float)(dtf * -0.0864 + p->dir[1]);
        p->pos[1] = p->dir[1] * dtf + p->pos[1];
        p->dir[0] = (float)(p->dir[0] * 0.8);
        p->pos[0] = p->dir[0] * dtf + p->pos[0];
        p->dir[2] = (float)(p->dir[2] * 0.8);
        p->pos[2] = p->dir[2] * dtf + p->pos[2];
        p->life -= dtf;
        if (p->life < 0.0f)
            p->live = 0;
        return;
    }
    if (p->size <= p->wide * 2 && p->size <= 0x100) {   /* opening up */
        p->size = (int)(p->size + (double)dtf * 3.8399999141693115);
        p->height = (int)(p->height + (double)dtf * 3.8399999141693115);
    } else {                                            /* then settling */
        p->height = (int)(p->height - (double)dtf * 1.2799999713897705);
        p->size = (int)(p->size + (double)dtf * 1.2799999713897705);
    }
    if (p->height < 0)
        p->live = 0;
    else {
        float w = (float)(p->size / 2), h = (float)p->height;
        p->v[0][0] = -w, p->v[0][1] = h, p->v[0][2] = 0.0f;
        p->v[1][0] = w, p->v[1][1] = h, p->v[1][2] = 0.0f;
        p->v[2][0] = w, p->v[2][1] = 0.0f, p->v[2][2] = 0.0f;
        p->v[3][0] = -w, p->v[3][1] = 0.0f, p->v[3][2] = 0.0f;
    }
    {
        double x = (double)p->dir[0] * p->speed, z = (double)p->dir[2] * p->speed;
        double y = (double)p->dir[1] * p->speed + (double)dtf * -0.024525, l;
        l = sqrt(y * y + x * x + z * z);
        p->speed = (float)l;
        if (l != 0.0)
            p->dir[0] = (float)(x / l), p->dir[1] = (float)(y / l), p->dir[2] = (float)(z / l);
        l = (double)p->speed * dtf * 0.009999999776482582 * 128.0;
        for (i = 0; i < 3; i++)
            p->pos[i] = (float)(p->dir[i] * l + p->pos[i]);
    }
}

void effects_step(float dt)
{
    int i;
    for (i = 0; i < PIECES; i++)
        if (g_piece[i].live)
            piece_step(&g_piece[i], dt);
}

/* ----------------------------------------------------------------- the tyre marks */

void effects_restart(void)
{
    int i;
    g_next_strip = 0;
    for (i = 0; i < STRIPS; i++)
        g_strip[i].nf = g_strip[i].nv = 0;
}

/* 0x457350 */
static int next_strip(void)
{
    int i = g_next_strip++;
    if (g_next_strip >= STRIPS)
        g_next_strip = 0;
    g_strip[i].nf = g_strip[i].nv = 0;
    return i;
}

/* 0x457390 -> 0x457200: two points more, on the road + 2 */
static void strip_add(int id, const float a[3], const float b[3], int sec)
{
    Strip *s;
    if (id < 0 || id >= STRIPS || (s = &g_strip[id])->nf >= 62 || g_land.nsec <= 0)
        return;
    memcpy(s->v[s->nv], a, 12);
    s->v[s->nv][1] = (float)(bikephys_road_y(land_walk(a, sec, 0), a) + 2.0f);
    s->nv++;
    memcpy(s->v[s->nv], b, 12);
    s->v[s->nv][1] = (float)(bikephys_road_y(land_walk(b, sec, 0), b) + 2.0f);
    s->nv++;
    if (s->nv > 2)
        s->nf += 2;
}

/* 0x457120 */
static void strip_faces(void)
{
    int i;
    for (i = 0; i < 32; i++) {
        Face *f = &g_strip_face[2 * i];
        memset(f, 0, 2 * sizeof *f);
        f[0].kind = f[1].kind = 0xc;
        f[0].colour = f[1].colour = (int)pack(8, 8, 8);
        f[0].n = f[1].n = 3;
        f[0].idx[0] = 2 * i, f[0].idx[1] = 2 * i + 1, f[0].idx[2] = 2 * i + 2;
        f[1].idx[0] = 2 * i + 1, f[1].idx[1] = 2 * i + 3, f[1].idx[2] = 2 * i + 2;
    }
}

/* ----------------------------------------------------------------- 0x466d70 */

/* 0x40ec60: the ground's kind under p, on the road or beside it */
static int ground(const BikePhys *b, const float p[3])
{
    const Section *s;
    double l;
    if (!g_land.sec || b->sec < 0 || b->sec >= g_land.nsec)
        return 0;
    s = &g_land.sec[b->sec];
    l = ((double)p[2] - s->pos[2]) * s->axes[0][2] + ((double)p[1] - s->pos[1]) * s->axes[0][1]
        + ((double)p[0] - s->pos[0]) * s->axes[0][0];
    return l > s->edge[0] && l < s->edge[1] ? s->ground_on : s->ground_off;
}

static int bits(float f)        /* the game's comparisons of a float's bits as an int */
{
    union { float f; int i; } u;
    u.f = f;
    return u.i;
}

/* 0x40ec30 */
static int wrap_section(int s)
{
    int n = g_land.nsec;
    return n > 0 ? ((s % n) + n) % n : 0;
}

static int in_window(const BikePhys *b)
{
    const BikePhys *v = g_bikeworld.rider[g_bikeworld.followed];
    int s = b->sec, lo, hi;
    if (!v)
        return 0;
    lo = wrap_section(v->sec - 10), hi = wrap_section(v->sec + 30);
    if (hi < lo)
        return s >= lo ? s < g_land.nsec : s >= 0 && s < hi;
    return s >= lo && s < hi;
}

static void spray_step(BikePhys *b, int m)
{
    int q = (int)((double)b->speed / b->top * 64.0f);
    if (q > 4) {
        double d = ((double)b->m[6] + b->m[9]) * b->vdir[0] + ((double)b->m[7] + b->m[10]) * b->vdir[1]
                   + ((double)b->m[8] + b->m[11]) * b->vdir[2];
        int n = (int)((1.0 - fabs((float)d)) * q * 5.0f);
        q = (q + 2 * n) / 3;
        if (bits(g_spray_clock) < 0x41f00000) { /* 30.0 */
            float dir[3];
            double x = (double)b->rear[0] - b->rear1[0], y = (double)b->rear[1] - b->rear1[1],
                   z = (double)b->rear[2] - b->rear1[2];
            double l = sqrt(z * z + y * y + x * x);
            if (l != 0.0)
                x /= l, y /= l, z /= l;
            dir[0] = (float)x, dir[1] = (float)y, dir[2] = (float)z;
            spray(m, q, b->pos, dir, (float)((double)b->speed * 0.55));
        }
    }
    if (b->number == g_bikeworld.followed) {
        g_spray_clock += g_bikeworld.dt;
        if (g_spray_clock > 50.0f)
            g_spray_clock -= 50.0f;
        if (bits(g_spray_clock) > 0x42480000)
            g_spray_clock = 0.0f;
    }
}

/* 24 puffs along the rear wheel's last move, 4 for a computer's */
static void puffs(BikePhys *b, unsigned keys)
{
    int i, n = b->driven ? 24 : 4, k;
    for (i = 0; i < n; i++) {
        int life = ((rand_next(1) & 1) * 2 + 2) * 15;
        float a, c, pos[3], v[3];
        rand_next(1), rand_next(1), rand_next(1);       /* (the spin, drawn by nothing) */
        c = (float)(rand_next(1) & 3);
        a = keys & 4 ? (float)(rand_next(1) & 3) : (float)-(rand_next(1) & 3);
        for (k = 0; k < 3; k++) {
            v[k] = (float)((double)b->m[k] * a + (double)b->m[6 + k] * c + (double)b->m[3 + k] * 0.0f
                           + b->m[9 + k]);
            pos[k] = (float)(((double)b->rear1[k] - b->rear[k]) * 0.03125f * i + b->rear[k]);
        }
        pos[1] += 10.0f;
        smoke(14, pos, v, life);
    }
}

void effects_bike(BikePhys *b, unsigned keys)
{
    float lat, alat;
    int m;
    if (g_land.nsec <= 0 || !in_window(b))
        return;
    lat = (float)bp_lateral(b);
    alat = fabsf(lat);
    m = ground(b, b->pos);
    if (b->ground && m > 0)
        spray_step(b, m);
    if (bits(b->speed) <= 0x40a00000)   /* 5 m/s */
        return;
    if ((!(b->keys1 & 2) && (keys & 2)) || (!b->sliding1 && b->sliding)) {
        b->skid_id[0] = next_strip();
        b->skid_id[1] = next_strip();
        b->skid_w[1] = b->driven ? 1.0f : 5.0f;
    }
    if (((keys & 2) || (b->sliding && bits(alat) > 0x3f000000)) && b->ground) {
        float x[3], y[3], a[3], c[3], w0[3], w1[3];
        int k;
        w0[0] = b->skid_w[1], w0[1] = w0[2] = 0.0f;
        w1[0] = -b->skid_w[1], w1[1] = w1[2] = 0.0f;
        bp_mulvec(x, w0, b->m);
        bp_mulvec(y, w1, b->m);
        for (k = 0; k < 3; k++)
            a[k] = b->rear[k] + x[k], c[k] = b->rear[k] + y[k];
        strip_add(b->skid_id[0], a, c, b->sec);
        for (k = 0; k < 3; k++)
            a[k] = b->front[k] + x[k], c[k] = b->front[k] + y[k];
        strip_add(b->skid_id[1], a, c, b->sec);
        if (b->driven) {
            double w = alat <= b->skid_w[0] ? (double)b->skid_w[1] - 1.0 : (double)b->skid_w[1] + 2.0;
            b->skid_w[1] = (float)w;
            if (bits(b->skid_w[1]) < 0x3f800000)
                b->skid_w[1] = 1.0f;
            if (bits(b->skid_w[1]) > 0x41400000)
                b->skid_w[1] = 12.0f;
            b->skid_w[0] = alat;
        }
    }
    if (((bits(b->wheelie) & 0x7fffffff) != 0 && bits(b->turbo_time) < 0x41c80000)
        || (keys & 2) || (b->sliding && bits(alat) > 0x3f000000)) {
        if (b->ground)
            puffs(b, keys);
    }
}

/* 0x467c40 */
void effects_debris(int n, const float pos[3], int base, int mask, const float *m)
{
    float p[3];
    int i, k;
    rand_next(1), rand_next(1), rand_next(1);           /* (the spin, drawn by nothing) */
    memcpy(p, pos, sizeof p);
    p[1] += 10.0f;
    for (i = 0; i < n; i++) {
        int life = ((rand_next(1) & 1) * 5 + 5) * 8;
        float f1 = (float)(rand_next(1) & 3), f2 = (float)(rand_next(1) & 3),
              f3 = (float)(rand_next(1) & 3), v[3];
        int size = (rand_next(1) & mask) + base;
        for (k = 0; k < 3; k++)
            v[k] = (float)((double)m[3 + k] * f2 + (double)m[6 + k] * f1 + (double)m[k] * f3 + m[9 + k]);
        smoke(size, p, v, life);
    }
}

/* ----------------------------------------------------------------- the drawing */

void effects_draw(int detail, float smooth)
{
    static const float I[3][3] = { { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } };
    static const float zero[3];
    float c[3][3], eye[3], r[3][3], n[3][3], t[3];
    int i, count = 0;
    r3d_get_camera(c, eye);
    if (detail >= 2) {          /* 0x4b0be0: at detail 2 one in 8 */
        /* the camera's heading (+0xc0 = 0x490220): upright squares turned towards it */
        double nn = sqrt((double)c[2][0] * c[2][0] + (double)c[2][1] * c[2][1]
                         + (double)c[2][2] * c[2][2]);
        double h = atan2(((double)c[1][0] * c[2][1] - (double)c[2][0] * c[1][1]) / nn, c[2][2] / nn);
        int idx = (int)(h * TO_INDEX);
        float A = g_cos[idx & 0x3fff], B = g_cos[(idx - 0x1000) & 0x3fff];
        float C0 = g_cos[0], S3 = g_cos[0x3000];
        r[0][0] = C0 * A, r[0][1] = (1.0f + B) * S3 * C0, r[0][2] = S3 * S3 - C0 * B * C0;
        r[1][0] = -A * S3, r[1][1] = C0 * C0 - S3 * B * S3, r[1][2] = (1.0f + B) * C0 * S3;
        r[2][0] = B, r[2][1] = -A * S3, r[2][2] = C0 * A;
        for (i = 0; i < PIECES; i++) {
            Piece *p = &g_piece[i];
            float at[3];
            int k, base;
            if (!p->live || ++count < (detail < 3 ? 8 : 1))
                continue;
            count = 0;
            for (k = 0; k < 3; k++)
                at[k] = p->before[k] + (p->pos[k] - p->before[k]) * smooth;
            r3d_place(n, t, (const float (*)[3])r, at);
            base = r3d_points(4, (const float (*)[3])p->v, (const float (*)[3])n, t);
            if (base >= 0)
                r3d_faces(base, 2, p->f, -1);
        }
    }
    if (!g_strip_face[0].n)
        strip_faces();
    r3d_place(n, t, I, zero);
    for (i = 0; i < STRIPS; i++) {
        const Strip *s = &g_strip[i];
        int base;
        if (s->nf <= 0)
            continue;
        base = r3d_points(s->nv, (const float (*)[3])s->v, (const float (*)[3])n, t);
        if (base >= 0)
            r3d_faces(base, s->nf, g_strip_face, -1);
    }
}
