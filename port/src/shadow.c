/* A bike's shadow (NOTES §4.15.9): six points on the road round the wheels, kept once a step
   (0x461aa0), and two faces of kind 0x2c on them that halve the road where the top view of
   the bike on the family's page 1 has a texel (0x461f70; the faces made by 0x45bf70). A
   crashed bike has a second, flat shadow where the rider's body lies. */
#include "shadow.h"
#include "bikes.h"
#include "land.h"
#include "r3d.h"
#include "config.h"
#include "race.h"
#include <math.h>
#include <string.h>
#include "fastfloat.h"      /* (last: the ESP32's single precision) */

/* 0x45bf70 with 0x422de0: the top view's left half (from the front) on points 0 1 2 5, its
   right half on 5 2 3 4; the rear of the bike is at u = 1 */
static Face g_pair[RIDERS][2], g_crash[RIDERS][2];     /* each bike's own: its kind drawn too */
static Face g_flat[RIDERS][2];  /* 0x47d550: the 1997 CD's bike shadow, the crash's form */
static int g_made;

static void corner(Face *f, int k, int idx, int u, int v)
{
    f->idx[k] = idx;
    f->ut[k] = (float)u;                /* (x 8192: x 1/256 (0x4232e0), x 2097152) */
    f->vt[k] = (float)v;
}

static void make_faces(void)
{
    static const int quad[2][4][3] = {
        { { 0, 114, 1 }, { 1, 114, 35 }, { 2, 57, 35 }, { 5, 57, 1 } },
        { { 5, 57, 1 }, { 2, 57, 35 }, { 3, 1, 35 }, { 4, 1, 1 } },
    };
    static const int tri[2][3][3] = {   /* 0x422f80: the whole picture on four corners */
        { { 0, 114, 1 }, { 1, 114, 35 }, { 2, 1, 35 } },
        { { 0, 114, 1 }, { 2, 1, 35 }, { 3, 1, 1 } },
    };
    int b, i, k;
    memset(g_pair, 0, sizeof g_pair);
    memset(g_crash, 0, sizeof g_crash);
    memset(g_flat, 0, sizeof g_flat);
    for (b = 0; b < RIDERS; b++)
        for (i = 0; i < 2; i++) {
            g_pair[b][i].kind = g_crash[b][i].kind = g_flat[b][i].kind = 0x2c;
            g_pair[b][i].page = g_crash[b][i].page = g_flat[b][i].page = 1;
            g_pair[b][i].n = 4;
            g_crash[b][i].n = g_flat[b][i].n = 3;
            for (k = 0; k < 4; k++)
                corner(&g_pair[b][i], k, quad[i][k][0], quad[i][k][1], quad[i][k][2]);
            for (k = 0; k < 3; k++) {
                corner(&g_crash[b][i], k, tri[i][k][0], tri[i][k][1], tri[i][k][2]);
                corner(&g_flat[b][i], k, tri[i][k][0], tri[i][k][1], tri[i][k][2]);
            }
        }
    g_made = 1;
}

/* 0x40c960: straight above or below p onto the section's road (the height 0x40c8b0 gives), + up */
static void onto_road(float p[3], int sec, float up)
{
    p[1] = (float)(bikephys_road_y(sec, p) + up);
}

void shadow_step(BikePhys *b)
{
    float dirz[3], R[12], front[3], l48[3], l54[3];
    double dx, dy, dz, l, h, a0 = 0.0, a1 = 0.0;
    int i, sec;
    static const float z40[3] = { 0.0f, 0.0f, 40.0f }, z50[3] = { 0.0f, 0.0f, 50.0f };

    /* the bike's way, and across it level with the road: (40, 0, 0) through the inverse of
       R(0x458e20(pos, pos + (0, 0, 50) M)): the first column of R */
    bp_mulvec(dirz, z50, b->m);
    dx = dirz[0], dy = dirz[1], dz = dirz[2];
    l = sqrt(dx * dx + dy * dy + dz * dz);
    if (l != 0.0)
        dx /= l, dy /= l, dz /= l;
    h = sqrt(dz * dz + dx * dx);
    if (h != 0.0) {
        a0 = atan2(dy, h);
        a1 = atan2(-dx / h, dz / h);
    }
    bp_rot(R, a0, a1, 0.0);
    for (i = 0; i < 3; i++)
        l54[i] = R[3 * i] * 40.0f;
    bp_mulvec(l48, z40, b->m);

    /* the front point as the last picture's drawer left it (+0x1b4): a wheelbase from the
       rear along the line between them */
    {
        double d[3], n, wb = (double)bp_carac(b, 0x38) * 96.0f;
        for (i = 0; i < 3; i++)
            d[i] = (double)b->front[i] - b->rear[i];
        n = sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
        for (i = 0; i < 3; i++)
            front[i] = n != 0.0 ? (float)(b->rear[i] + d[i] / n * wb) : b->front[i];
    }
    if (behaviour_cd()) {               /* 0x487aa0: four corners, one height from the bike's section */
        for (i = 0; i < 3; i++) {
            b->shadow[0][i] = front[i] + l48[i] - l54[i];
            b->shadow[1][i] = front[i] + l48[i] + l54[i];
            b->shadow[2][i] = b->rear[i] - l48[i] + l54[i];
            b->shadow[3][i] = b->rear[i] - l48[i] - l54[i];
        }
        for (i = 0; i < 4; i++)
            onto_road(b->shadow[i], b->sec, 2.0f);
    } else {
    for (i = 0; i < 3; i++) {
        b->shadow[0][i] = front[i] + l48[i] - l54[i];
        b->shadow[1][i] = front[i] + l48[i] + l54[i];
        b->shadow[3][i] = b->rear[i] - l48[i] + l54[i];
        b->shadow[4][i] = b->rear[i] - l48[i] - l54[i];
        b->shadow[2][i] = (b->shadow[3][i] + b->shadow[1][i]) * 0.5f;
        b->shadow[5][i] = (b->shadow[0][i] + b->shadow[4][i]) * 0.5f;
    }
    for (i = 0; i < 6; i++) {           /* the two in the middle higher: the road may bend */
        sec = land_walk(b->shadow[i], b->sec, 0);
        onto_road(b->shadow[i], sec, i == 2 || i == 5 ? 8.0f : 4.0f);
    }
    }
    if (b->crashed) {                   /* (0x52c234 NewMotorEngine: 1 on all ten tracks) */
        double hd = bp_heading(b->tumble);
        bp_rot(R, 0.0, -hd, 0.0);
        for (i = 0; i < 3; i++) {
            float x = R[i], z = R[6 + i] * 2.0f, p = R[9 + i] + b->tumble_pt[i];
            b->crash_shadow[0][i] = (z - x) * 32.0f + p;
            b->crash_shadow[1][i] = (z + x) * 32.0f + p;
            b->crash_shadow[2][i] = (x - z) * 32.0f + p;
            b->crash_shadow[3][i] = (-z - x) * 32.0f + p;
        }
        sec = land_walk(b->tumble_pt, b->sec, 0);
        for (i = 0; i < 4; i++)
            onto_road(b->crash_shadow[i], sec, 2.0f);
    }
}

void shadow_draw(const Rider *r, int k)
{
    static const float I[3][3] = { { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } };
    static const float zero[3];
    float n[3][3], t[3];
    int base;
    if (!g_made)
        make_faces();
    r3d_place(n, t, I, zero);
    base = r3d_points(behaviour_cd() ? 4 : 6, (const float (*)[3])r->shadow, (const float (*)[3])n, t);
    if (base >= 0)                      /* detail 3 for these faces: always the texture */
        r3d_faces_detail(base, 2, behaviour_cd() ? g_flat[k] : g_pair[k], 3);
    if (r->crashed) {
        base = r3d_points(4, (const float (*)[3])r->crash_shadow, (const float (*)[3])n, t);
        if (base >= 0)
            r3d_faces_detail(base, 2, g_crash[k], 3);
    }
}
