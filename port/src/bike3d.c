/* The bike and its rider (NOTES §4.15): the family's skeleton (.SKL, 0x462c60), the rider's
   pose (.ROT, 0x45b340), the wheels (0x413450), the bank pages with the liveries'
   palettes (0x401d90), and the drawer 0x45f290 at detail 0 as page 10 calls it: no lean,
   pitch, steering, speed, suspension or wheel spin, not crashed, unlit. */
#include "bike3d.h"
#include "config.h"
#include "bikes.h"
#include "files.h"
#include "land.h"
#include "r3d.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TO_INDEX 2607.594587618272      /* 0x4d0300: 16384 / 2pi */
#define BONES 20                        /* records a skeleton has (n <= 18, then the extra) */

typedef float V[3];
typedef float M[3][3];

typedef struct {                /* 100 bytes at obj +0x63c + slot x 100 */
    int parent;                 /* +4: its parent's slot, -1 a root */
    float ang[3];               /* +0xc: the pose's angles */
    V pivot;                    /* +0x48 */
    int first, count;           /* +0x54, +0x58: its vertices */
    int nf;                     /* +0x5c */
    Face *f;                    /* +0x60 */
} Bone;

typedef struct {
    int n;                      /* +0x2d4c: the bones */
    int slot[BONES];            /* +0x2d74: file bone -> slot */
    int nv;
    V *v;
    Bone b[BONES];              /* slot n holds the extra faces (the rider's joints) */
} Skeleton;
static Skeleton g_skls[2];      /* detail 0, and detail 1 (FileSklHand): the raised hand */
static Skeleton *g_cur = &g_skls[0];
#define g_skl (*g_cur)
static Model g_wheel[2];        /* rear 0x52c240, front 0x51ac58 */
static Model g_lod[3];          /* medium, low, very low: 0x53a788, 0x53a7f8, 0x53a868 */
static Model g_bar, g_pit;      /* the on-board view's handlebar 0x504ed8, cockpit 0x51bf60 */
static float g_shake;           /* 0x4ea4e4: the cockpit's smoothed pitch */
static int g_loaded;

/* ---------------------------------------------------------------- reading */

typedef struct {
    const unsigned char *d;
    long len, p;
} Rd;

static int rd16(Rd *r)          /* 0x458d40: -1 at the end of the file */
{
    int v;
    if (r->p + 2 > r->len) {
        r->p = r->len + 1;
        return -1;
    }
    v = (int16_t)(r->d[r->p] | r->d[r->p + 1] << 8);
    r->p += 2;
    return v;
}

static float rdf(Rd *r)         /* 0x458d70 */
{
    uint32_t u;
    float f;
    if (r->p + 4 > r->len) {
        r->p = r->len + 1;
        return 0.0f;
    }
    u = (uint32_t)r->d[r->p] | (uint32_t)r->d[r->p + 1] << 8
        | (uint32_t)r->d[r->p + 2] << 16 | (uint32_t)r->d[r->p + 3] << 24;
    memcpy(&f, &u, 4);
    r->p += 4;
    return f;
}

static void rdv(Rd *r, float v[3], int new_format)
{
    int k;
    for (k = 0; k < 3; k++)
        v[k] = new_format ? (float)(rdf(r) * 3.5f)
                          : (float)(int)(rd16(r) / 10.0);      /* the old format, truncated */
}

/* the file's 5:6:5 to 5:5:5 and back through 0x402020, which loses green's lowest bit */
static int from_file(int c)
{
    c = (c & 0xffc1) >> 1 | (c & 0x1f);
    return (c & 0x7c00) << 1 | (c & 0x3e0) << 1 | (c & 0x1f);
}

/* the faces of a bone (bones = 1) or the extra group: 36 bytes each, the colour settled
   as 0x462c60 leaves it on a paletted page 0 */
static Face *read_faces(Rd *r, int count, int bones)
{
    const uint8_t *tx = r3d_page_index(0);
    const uint16_t *pal0 = r3d_page_palette(0, 0);
    Face *out = calloc(count > 0 ? count : 1, sizeof *out);
    int i, k;
    if (!out)
        return 0;
    for (i = 0; i < count; i++) {
        Face *f = &out[i];
        int kind, colour, idx[5], u[5], v[5];
        rd16(r);                                        /* not used */
        kind = rd16(r);
        colour = rd16(r) & 0xffff;
        for (k = 0; k < 5; k++) {
            idx[k] = rd16(r) & 0xffff;
            u[k] = rd16(r);
            v[k] = rd16(r);
        }
        f->n = idx[4] != 0xffff ? 4 : 3;                /* a triangle's 4th is its 1st */
        f->kind = kind == 3 ? 1 : kind == 4 ? 2 : kind; /* 0x462c30 */
        f->flags = 4;
        f->page = 0;
        for (k = 0; k < f->n; k++) {
            f->idx[k] = idx[k];
            f->ut[k] = (float)u[k];                     /* (x 8192: x 1/256 (0x4232e0), x 2097152) */
            f->vt[k] = (float)v[k];
        }
        if (!tx) {                                      /* no texels: 0x402020, 555 -> 565 */
            if (bones && !behaviour_cd())
                f->colour = from_file(colour);
            else
                f->colour = colour;
        } else if (u[0] == u[1] && u[1] == u[2] && v[0] == v[1] && v[1] == v[2]) {
            /* one texel for all: a face of its colour. The bones' loop passes the texel
               through 0x402020 as if it were 5:5:5, which loses its bit 5; the extra
               group's loop keeps it, and so do both on the 1997 CD (0x483d40) */
            int t = tx[((v[0] << 8) + u[0]) & 0xffff];
            f->kind = 0;
            f->colour = bones && !behaviour_cd() ? t & 0xdf : t;
        } else {                /* the index of its 5:6:5 colour in palette 0, else 0 */
            int c = from_file(colour);
            f->colour = 0;
            for (k = 0; k < 256; k++)
                if (pal0[k] == c) {
                    f->colour = k;
                    break;
                }
        }
    }
    return out;
}

static void skl_free(void)
{
    int i;
    for (i = 0; i < BONES; i++)
        free(g_skl.b[i].f);
    free(g_skl.v);
    memset(&g_skl, 0, sizeof g_skl);
}

/* 0x462c60, then 0x45bf70's part: centred on slot 0's pivot, slot 3 (the rider) unlinked */
static int skl_load(const char *path)
{
    long len;
    unsigned char *d = dsi_load(path, &len);
    Rd r = { d, len, 0 };
    int parent[BONES], i, n, new_format, nf, sum = 0;
    skl_free();
    if (!d)
        return 0;
    n = rd16(&r);
    new_format = n < 0;
    if (new_format)
        n = rd16(&r);
    if (n < 1 || n > 18)
        goto bad;
    g_skl.n = n;
    for (i = 0; i < n; i++)
        parent[i] = rd16(&r);
    for (i = 0; i < n; i++)
        g_skl.slot[i] = new_format ? rd16(&r) : i;
    for (i = 0; i < n; i++)
        if (g_skl.slot[i] < 0 || g_skl.slot[i] >= BONES - 1 || parent[i] >= n)
            goto bad;
    g_skl.nv = rd16(&r);
    if (g_skl.nv < 0 || !(g_skl.v = malloc(sizeof *g_skl.v * (g_skl.nv + 1))))
        goto bad;
    for (i = 0; i < g_skl.nv; i++)
        rdv(&r, g_skl.v[i], new_format);
    for (i = 0; i < BONES; i++)
        g_skl.b[i].parent = -1;
    for (i = 0; i < n; i++) {
        Bone *b = &g_skl.b[g_skl.slot[i]];
        rdv(&r, b->pivot, new_format);
        b->first = rd16(&r);
        b->count = rd16(&r);
        nf = rd16(&r);
        b->nf = nf > 0 ? nf : 0;
        if (!(b->f = read_faces(&r, b->nf, 1)))
            goto bad;
        /* the drawer puts the bones' vertices one after another in file order and the
           faces index that: the ranges must follow each other */
        if (b->count < 0 || (b->count && b->first != sum))
            goto bad;
        sum += b->count;
    }
    nf = rd16(&r);
    g_skl.b[n].nf = nf > 0 ? nf : 0;
    if (!(g_skl.b[n].f = read_faces(&r, g_skl.b[n].nf, 0)) || r.p > r.len || sum > g_skl.nv)
        goto bad;
    for (i = 0; i <= n; i++) {
        int k, j;
        for (k = 0; k < g_skl.b[i].nf; k++)
            for (j = 0; j < g_skl.b[i].f[k].n; j++)
                if (g_skl.b[i].f[k].idx[j] >= sum)
                    goto bad;
    }
    for (i = 0; i < n; i++)
        if (g_skl.slot[i] > 0 && parent[i] >= 0)
            g_skl.b[g_skl.slot[i]].parent = g_skl.slot[parent[i]];
    g_skl.b[3].parent = -1;
    {
        V p0;
        memcpy(p0, g_skl.b[0].pivot, sizeof p0);
        for (i = 0; i < g_skl.nv; i++)
            for (n = 0; n < 3; n++)
                g_skl.v[i][n] -= p0[n];
        for (i = 0; i < BONES; i++)
            for (n = 0; n < 3; n++)
                g_skl.b[i].pivot[n] -= p0[n];
    }
    free(d);
    return 1;
bad:
    fprintf(stderr, "openmotoracer: the skeleton %s could not be read\n", path);
    free(d);
    skl_free();
    return 0;
}

/* 0x413450's wheels: 0x420a50, then centred (0x420250) */
static int wheel_load(Model *m, int which, int dirt)
{
    if (!model_load_bike(m, g_fam.model[which][0], g_fam.model[which][1], dirt))
        return 0;
    model_centre(m);
    return 1;
}

void bike3d_free(void)
{
    int i;
    for (i = 0; i < 2; i++) {
        g_cur = &g_skls[i];
        skl_free();
    }
    g_cur = &g_skls[0];
    pose_free();
    model_free(&g_wheel[0]);
    model_free(&g_wheel[1]);
    for (i = 0; i < 3; i++)
        model_free(&g_lod[i]);
    model_free(&g_bar);
    model_free(&g_pit);
    r3d_free_page(0);
    r3d_free_page(1);
    g_loaded = 0;
}

int bike3d_load(int rgb1)
{
    int i, dirt = g_bikes[0].perso == 1;           /* the flag of 0x420a50 */
    bike3d_free();
    /* 0x401d90: bank 0 paletted (the liveries), bank 1 in RGB when the flag says so (page
       10: the dirt bikes; the race: Level.Spt's bank 1); a road family's bank 1 (24 bits,
       256x256) with the flag 0 is not read */
    if (!r3d_load_page_pal(0, g_fam.bank[0]) || (rgb1 && !r3d_load_page(1, g_fam.bank[1]))
        || !skl_load(g_fam.skl[0]) || !pose_load()
        || !wheel_load(&g_wheel[0], FM_REAR, dirt) || !wheel_load(&g_wheel[1], FM_FRONT, dirt)) {
        bike3d_free();
        return 0;
    }
    g_cur = &g_skls[1];
    if (!skl_load(g_fam.skl[1])) {
        bike3d_free();
        return 0;
    }
    g_cur = &g_skls[0];
    /* 0x413450: the handlebar and the cockpit x 8 (the flag 0), then the three far models,
       x 0.95 (0x4204b0) */
    if (!model_load_bike(&g_bar, g_fam.model[FM_HANDLEBAR][0], g_fam.model[FM_HANDLEBAR][1], 0)
        || !model_load_bike(&g_pit, g_fam.model[FM_COCKPIT][0], g_fam.model[FM_COCKPIT][1], 0)) {
        bike3d_free();
        return 0;
    }
    model_scale(&g_bar, 8.0f, 8.0f, 8.0f);
    model_scale(&g_pit, 8.0f, 8.0f, 8.0f);
    for (i = 0; i < 3; i++) {
        if (!model_load_bike(&g_lod[i], g_fam.model[FM_MEDIUM + i][0],
                             g_fam.model[FM_MEDIUM + i][1], dirt)) {
            bike3d_free();
            return 0;
        }
        model_scale(&g_lod[i], 0.95f, 0.95f, 0.95f);
    }
    g_loaded = 1;
    return 1;
}

/* ---------------------------------------------------------------- the arithmetic */

static float f32(double x)
{
    return (float)x;
}

/* 0x40fd40: Rz(c) Ry(b) Rx(a) for row vectors, through the cosine table */
static void rmat(M m, double a, double b, double c)
{
    double ang[3] = { a, b, c }, co[3], si[3];
    float t1, t2;
    int j;
    for (j = 0; j < 3; j++) {
        int i = (int)((double)(float)ang[j] * TO_INDEX);
        co[j] = g_cos[i & 0x3fff];
        si[j] = g_cos[(i - 0x1000) & 0x3fff];
    }
    t1 = f32(si[1] * co[0]);
    t2 = f32(si[1] * si[0]);
    m[0][0] = f32(co[1] * co[2]);
    m[0][1] = f32(t2 * co[2] + si[2] * co[0]);
    m[0][2] = f32(si[2] * si[0] - t1 * co[2]);
    m[1][0] = f32(-(si[2] * co[1]));
    m[1][1] = f32(co[0] * co[2] - t2 * si[2]);
    m[1][2] = f32(co[2] * si[0] + t1 * si[2]);
    m[2][0] = (float)si[1];
    m[2][1] = f32(-(co[1] * si[0]));
    m[2][2] = f32(co[1] * co[0]);
}

static void mm(M out, const M a, const M b)     /* 0x403a80: a then b */
{
    M r;
    int i, j;
    for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++)
            r[i][j] = f32((double)a[i][0] * b[0][j] + (double)a[i][1] * b[1][j]
                          + (double)a[i][2] * b[2][j]);
    memcpy(out, r, sizeof r);
}

static void vm(V out, const V v, const M a, const V t)  /* 0x410010: v a + t */
{
    V r;
    int j;
    for (j = 0; j < 3; j++)
        r[j] = f32((double)v[0] * a[0][j] + (double)v[1] * a[1][j] + (double)v[2] * a[2][j]
                   + (t ? t[j] : 0.0));
    memcpy(out, r, sizeof r);
}

static void sub(V out, const V a, const V b)
{
    int k;
    for (k = 0; k < 3; k++)
        out[k] = a[k] - b[k];
}

static void add(V out, const V a, const V b)
{
    int k;
    for (k = 0; k < 3; k++)
        out[k] = a[k] + b[k];
}

static void scl(V out, float s, const V a)
{
    int k;
    for (k = 0; k < 3; k++)
        out[k] = s * a[k];
}

static float length(const V a)
{
    return f32(sqrt((double)a[0] * a[0] + (double)a[1] * a[1] + (double)a[2] * a[2]));
}

static void norm(V out, const V a)
{
    float l = length(a);
    int k;
    for (k = 0; k < 3; k++)
        out[k] = l != 0.0f ? f32((double)a[k] / l) : a[k];
}

/* 0x46bd00 / 0x46d270: the angles rmat would take to build a */
static void angles_of(double ang[3], const M a)
{
    double sb = a[2][0] < -1.0f ? -1.0 : a[2][0] > 1.0f ? 1.0 : a[2][0];
    ang[1] = asin(sb);
    if (fabs(cos(ang[1])) > 1e-5) {
        ang[0] = atan2(-(double)a[2][1], a[2][2]);
        ang[2] = atan2(-(double)a[1][0], a[0][0]);
    } else {
        ang[0] = atan2(a[1][2], a[1][1]);
        ang[2] = 0.0;
    }
}

static void rmat_of(M out, const M a)
{
    double ang[3];
    angles_of(ang, a);
    rmat(out, ang[0], ang[1], ang[2]);
}

/* the rigid inverse (0x463d90, the vertex loop): R transposed, T = -T R^T */
static void inv(M r, V t, const M a, const V at)
{
    M ri;
    V ti;
    int i, j;
    for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++)
            ri[i][j] = a[j][i];
    for (j = 0; j < 3; j++)
        ti[j] = f32(-((double)at[0] * a[j][0] + (double)at[1] * a[j][1]
                      + (double)at[2] * a[j][2]));
    memcpy(r, ri, sizeof ri);
    memcpy(t, ti, sizeof ti);
}

/* ---------------------------------------------------------------- the camera of page 10 */

void bike3d_menu_camera(float theta, const float target[3])
{
    static const float zero[3];
    M cam, obj, m;
    V eye;
    /* 0x46dab0: the camera's angles (0, theta, 0) after the bike's (0, 0, 0); the eye
       1500 along the third row, from the bike's place (0, 0, 0) */
    rmat(cam, 0.0, theta, 0.0);
    rmat(obj, 0.0, 0.0, 0.0);
    mm(m, obj, cam);
    scl(eye, 1500.0f, m[2]);
    add(eye, zero, eye);
    eye[1] += 200.0f;                           /* 0x4d0c44 */
    r3d_camera(eye, target);                    /* 0x46e140 */
    eye[1] += 40.0f;                            /* 0x4d0bac: the view from 40 higher */
    r3d_camera_pos(eye);
}

/* ---------------------------------------------------------------- 0x45f290 */

static float carac(int bike, int off)           /* the bike's float at +off */
{
    return g_bikes[bike].phys[(off - 0x14) / 4];
}

/* 0x45c7c0(obj, x, y, z): the body there, and the wheels' ground points under the bike's
   matrix: rear (0, c40 x -96, c38 x -48), front (0, c3c x -96, c38 x 48) */
void bike3d_place(Rider *r, const float p[3])
{
    V v, w;
    int bike = r->bike >= 0 && r->bike < BIKES ? r->bike : 0;
    memcpy(r->pos, p, sizeof r->pos);
    v[0] = 0.0f, v[1] = carac(bike, 0x40) * -96.0f, v[2] = carac(bike, 0x38) * -48.0f;
    vm(w, v, (const float (*)[3])r->m, r->t);
    add(r->rear, r->pos, w);
    v[1] = carac(bike, 0x3c) * -96.0f, v[2] = carac(bike, 0x38) * 48.0f;
    vm(w, v, (const float (*)[3])r->m, r->t);
    add(r->front, r->pos, w);
}

static M g_bm[BONES];           /* each bone's matrix (+0x18) */
static V g_bt[BONES];
static int g_made[BONES];

/* the pose's local turn about the pivot: R, with T = to - pivot R */
static void local(M r, V t, int slot, const float *to)
{
    const Bone *b = &g_skl.b[slot];
    V mp;
    rmat(r, b->ang[0], b->ang[1], b->ang[2]);
    scl(mp, -1.0f, b->pivot);
    vm(t, mp, (const float (*)[3])r, to ? to : b->pivot);
}

/* 0x463d90 for a root (M = P^-1 L), 0x464190 for the others (M = parent's M, then L) */
static M g_rinv;                 /* the rider's root (slot 3): +0x3085 inverted */
static V g_rinvt;

static void bone_matrix(int slot, const M pinv, const V pinvt)
{
    M r;
    V t;
    int p = g_skl.b[slot].parent;
    if (g_made[slot])
        return;
    local(r, t, slot, 0);
    if (p < 0) {                /* 0x463d90: slot 0 by the frame, slot 3 by +0x3085 */
        mm(g_bm[slot], slot == 3 ? (const float (*)[3])g_rinv : pinv, (const float (*)[3])r);
        vm(g_bt[slot], slot == 3 ? g_rinvt : pinvt, (const float (*)[3])r, t);
    } else {
        bone_matrix(p, pinv, pinvt);
        mm(g_bm[slot], (const float (*)[3])g_bm[p], (const float (*)[3])r);
        vm(g_bt[slot], g_bt[p], (const float (*)[3])r, t);
    }
    g_made[slot] = 1;
}

static int g_lighting;          /* 0x4dc130 */

void bike3d_lighting(int on)
{
    g_lighting = on;
}

/* 0x465d60 != 0x1f: the rider's section is a dark one (+0xe0) */
static int dark_section(const Rider *rd)
{
    return g_lighting && g_land.sec && rd->sec >= 0 && rd->sec < g_land.nsec
           && g_land.sec[rd->sec].flag != 0;
}

/* 0x48c400; in the race 0x4221d0(dark section) and 0x48c6d0, whose 0x422540 leaves every
   point's word at 8..24 << 13 (the normals only decide which, and no span reads more than
   "below 0x3e000"): a wheel's paletted faces are always halved */
static void wheel(const Model *m, const M r, const V at, const V d, int skin, int dark)
{
    M n;
    V p, t;
    int base;
    add(p, at, d);
    r3d_place(n, t, (const float (*)[3])r, p);
    r3d_dark(g_lighting, dark);
    if ((base = r3d_points(m->nv, (const float (*)[3])m->v, (const float (*)[3])n, t)) >= 0)
        r3d_faces(base, m->nf, m->f, skin);
    r3d_dark(0, 0);
}

static void part(const M r, const V pos, int i, int *base)
{
    const Bone *b = &g_skl.b[g_skl.slot[i]];
    M n;
    V t;
    int at;
    if (!b->count)
        return;
    r3d_place(n, t, (const float (*)[3])r, pos);
    at = r3d_points(b->count, (const float (*)[3])g_skl.v + b->first, (const float (*)[3])n, t);
    if (*base == -2 && at >= 0)
        *base = at - b->first;
    else if (at < 0)
        *base = -1;
}

/* 0x45f290 at detail 2-4: one model on the rear wheel's ground point, its radius up */
static void draw_far(const Rider *rd, int lod)
{
    const Model *m = &g_lod[lod - 2];
    M a, b, mm1;
    V v, t;
    float n[3][3], tt[3];
    int base;
    rmat(a, 0.0, 1.5707963267948966, 0.0);
    rmat(b, 0.0, 0.0, -(rd->crashed ? rd->roll_side : rd->lean));
    mm(mm1, (const float (*)[3])a, (const float (*)[3])b);
    mm(mm1, (const float (*)[3])mm1, (const float (*)[3])rd->m);
    v[0] = 0.0f, v[1] = carac(rd->bike, 0x40) * 96.0f, v[2] = 0.0f;
    vm(t, v, (const float (*)[3])mm1, rd->rear);
    r3d_place(n, tt, (const float (*)[3])mm1, t);
    r3d_dark(dark_section(rd), 0);
    if ((base = r3d_points(m->nv, (const float (*)[3])m->v, (const float (*)[3])n, tt)) >= 0)
        r3d_faces(base, m->nf, m->f, rd->skin);
    r3d_dark(0, 0);
}

void bike3d_draw(int bike, int skin)
{
    static const float zero[3];
    Rider r;
    memset(&r, 0, sizeof r);
    r.bike = bike, r.skin = skin;
    rmat(r.m, 0.0, 0.0, 0.0);           /* (0x40a710: built from COS[0] and COS[0x3000]) */
    bike3d_place(&r, zero);
    bike3d_draw_rider(&r, 0);
}

static void rider(Rider *rd, int lod, int draw);

void bike3d_draw_rider(Rider *rd, int lod)
{
    rider(rd, lod, 1);
}

void bike3d_body(Rider *rd)
{
    if (!rd->crashed)
        rider(rd, 0, 0);
}

static void rider(Rider *rd, int lod, int draw)
{
    static const V zero;
    M B, L298, Mframe, Mfront, Mrear, Msteer, Rphi, swing, Pinv, r;
    V rear, front0, front, mid, dir, v, Pbody, head, grip, fdir, axle_f, axle_r, piv15s, d, w;
    V p3, k, nn, a, Pinvt, t, Pref;
    const Bone *b;
    float c38, c3c, c40, c58, c6c, y1, h, ln, reach, q, phi, fade, st, tl;
    double cr[3], ka, th3;
    int i, base = -2, bike = rd->bike, skin = rd->skin;
    if (!g_loaded || bike < 0 || bike >= BIKES)
        return;
    if (draw && lod >= 2 && lod <= 4) {
        draw_far(rd, lod);
        return;
    }
    /* the bones of this detail take the body's angles (0x45d810's end) */
    g_cur = &g_skls[lod == 1];
    b = g_skl.b;
    {
        const Pose *p = rd->pose.n ? &rd->pose : pose_rot(4);
        for (i = 0; i < g_skl.n && i < p->n; i++)
            memcpy(g_skl.b[i].ang, p->a[i], sizeof g_skl.b[i].ang);
    }
    c38 = carac(bike, 0x38), c3c = carac(bike, 0x3c), c40 = carac(bike, 0x40);
    c58 = carac(bike, 0x58), c6c = carac(bike, 0x6c);
    memcpy(B, rd->m, sizeof B);                 /* +0x2e55 = +0xbc */
    memcpy(rear, rd->rear, sizeof rear);        /* +0x338, +0x1b4 */
    memcpy(front0, rd->front, sizeof front0);

    /* 2-4: the lean (the crash's roll when crashed), the pitch, the steering faded out
       with the speed, the wheels' spins */
    tl = rd->crashed ? rd->roll_side : rd->lean;
    rmat(r, 0.0, 0.0, -tl);
    mm(L298, (const float (*)[3])r, (const float (*)[3])B);
    {
        M rp;
        rmat(rp, -rd->pitch, 0.0, 0.0);
        mm(Mframe, (const float (*)[3])r, (const float (*)[3])rp);
        mm(Mframe, (const float (*)[3])Mframe, (const float (*)[3])B);
    }
    if (g_bikes[bike].perso)                    /* dirt */
        fade = f32(1.0 - rd->speed / ((double)carac(bike, 0x48) * 2.0f));
    else {
        fade = f32(1.0 - rd->speed * 2.0f / (double)carac(bike, 0x48));
        if (fade < 0.0f)
            fade = 0.0f;
        if (fade > 1.0f)
            fade = 1.0f;
    }
    st = f32(rd->steer * (double)fade * 0.0007669903939428206);
    rmat(r, rd->spin_rear, 0.0, 0.0);
    {
        M Mbody0, rs;
        mm(Mbody0, (const float (*)[3])r, (const float (*)[3])L298);
        rmat(r, rd->spin_front, 0.0, 0.0);
        rmat(rs, 0.0, st, 0.0);
        mm(Mfront, (const float (*)[3])r, (const float (*)[3])rs);
        mm(Mfront, (const float (*)[3])Mfront, (const float (*)[3])Mframe);

        /* 5-6: the front wheel a wheelbase from the rear, the body over the middle */
        sub(v, front0, rear);
        norm(dir, v);
        scl(v, c38 * 96.0f, dir);
        add(front, rear, v);
        scl(v, c38 * 48.0f, dir);
        add(mid, rear, v);
        y1 = f32((double)c58 * 96.0f + (double)rd->dive * 40.0f);
        h = f32((double)c40 * 48.0f + (double)c40 * 0.125f * 96.0f * rd->susp);
        v[0] = 0.0f, v[1] = y1, v[2] = 0.0f;
        vm(Pbody, v, (const float (*)[3])Mframe, 0);
        add(Pbody, mid, Pbody);
        v[1] = h;
        vm(w, v, (const float (*)[3])L298, 0);
        add(Pbody, Pbody, w);
        if (!draw) {
            memcpy(rd->body, Mframe, sizeof rd->body);
            memcpy(rd->body_pt, Pbody, sizeof rd->body_pt);
            return;
        }
        rmat_of(Mrear, (const float (*)[3])L298);

        /* 8: the fork, stretched to the front axle */
        vm(head, b[1].pivot, (const float (*)[3])Mrear, Pbody);
        rmat(r, 0.0, st, 0.0);
        mm(Msteer, (const float (*)[3])r, (const float (*)[3])Mframe);
        sub(v, b[15].pivot, b[1].pivot);
        vm(grip, v, (const float (*)[3])Msteer, head);
        sub(v, grip, head);
        ln = length(v);
        norm(fdir, v);
        sub(v, front, head);
        reach = length(v) - c3c * 96.0f;
        scl(v, reach, fdir);
        add(axle_f, head, v);
        sub(d, axle_f, front);
        wheel(&g_wheel[1], (const float (*)[3])Mfront, front, d, skin, dark_section(rd));
        sub(v, b[15].pivot, b[1].pivot);
        q = f32((double)length(v) * ln / reach);
        norm(w, v);
        scl(w, q, w);
        add(piv15s, b[1].pivot, w);
        g_skl.b[1].ang[0] = 0.0f, g_skl.b[1].ang[1] = -st, g_skl.b[1].ang[2] = 0.0f;

        /* 9: the swing arm, by slot 3's pivot (the rider's root, as the game has it) */
        vm(p3, b[3].pivot, (const float (*)[3])Mrear, Pbody);
        v[0] = b[3].pivot[0], v[1] = b[3].pivot[1], v[2] = b[3].pivot[2] - 1.0f;
        vm(k, v, (const float (*)[3])Mrear, Pbody);
        sub(k, k, p3);
        v[0] = b[3].pivot[0] - 1.0f, v[2] = b[3].pivot[2];
        vm(nn, v, (const float (*)[3])Mrear, Pbody);
        sub(nn, nn, p3);
        norm(nn, nn);
        sub(a, rear, p3);
        cr[0] = (double)a[2] * k[1] - (double)a[1] * k[2];
        cr[1] = (double)a[0] * k[2] - (double)a[2] * k[0];
        cr[2] = (double)a[1] * k[0] - (double)a[0] * k[1];
        ka = (double)length(k) * length(a);
        th3 = atan2((nn[0] * cr[0] + nn[1] * cr[1] + nn[2] * cr[2]) / ka,
                    ((double)a[0] * k[0] + (double)a[1] * k[1] + (double)a[2] * k[2]) / ka);
        phi = f32(c6c + th3);
        rmat(Rphi, phi, 0.0, 0.0);
        mm(swing, (const float (*)[3])Rphi, (const float (*)[3])Mrear);
        vm(axle_r, b[16].pivot, (const float (*)[3])swing, Pbody);
        sub(d, axle_r, rear);
        wheel(&g_wheel[0], (const float (*)[3])Mbody0, rear, d, skin, dark_section(rd));
    }

    /* 10: the rider's reference: the frame while riding (kept for a crash), else what the
       crash has made of it, 32 (dirt) or 64 higher */
    if (!rd->crashed) {
        memcpy(rd->body, Mframe, sizeof rd->body);
        memcpy(rd->body_pt, Pbody, sizeof rd->body_pt);
        memcpy(Pref, Pbody, sizeof Pref);
    } else {
        memcpy(Pref, rd->body_pt, sizeof Pref);
        Pref[1] = f32((double)Pref[1] + (g_bikes[bike].perso ? 32.0f : 64.0f));
    }
    /* 11: the bones' matrices, then the fork's end again with its stretched pivot */
    inv(Pinv, Pinvt, (const float (*)[3])Mframe, zero);
    inv(g_rinv, g_rinvt, (const float (*)[3])rd->body, zero);
    memset(g_made, 0, sizeof g_made);
    for (i = 0; i < g_skl.n; i++)
        bone_matrix(g_skl.slot[i], (const float (*)[3])Pinv, Pinvt);
    local(r, t, 15, piv15s);
    mm(g_bm[15], (const float (*)[3])g_bm[1], (const float (*)[3])r);
    vm(g_bt[15], g_bt[1], (const float (*)[3])r, t);

    /* 4.4: every bone's vertices, in file order, into one array */
    for (i = 0; i < g_skl.n && base != -1; i++) {
        int s = g_skl.slot[i];
        if (i == 0) {
            rmat_of(r, (const float (*)[3])Mframe);
            part((const float (*)[3])r, Pbody, i, &base);
        } else if (i == 3)
            part((const float (*)[3])swing, Pbody, i, &base);
        else {
            inv(r, t, (const float (*)[3])g_bm[s], g_bt[s]);
            add(t, t, s >= 3 && s <= 14 ? Pref : Pbody);
            part((const float (*)[3])r, t, i, &base);
        }
    }
    if (base < 0)
        return;
    r3d_dark(dark_section(rd), 0);
    for (i = 0; i <= g_skl.n; i++)
        r3d_faces(base, g_skl.b[i].nf, g_skl.b[i].f, skin);
    r3d_dark(0, 0);
}

/* ---------------------------------------------------------------- the on-board view */

void bike3d_cockpit_step(double pitch)
{
    g_shake = f32((pitch * -0.25f - g_shake) * 0.25f + g_shake);
}

/* 0x462110: the handlebar turned by the steering and the cockpit, both in the view's frame,
   rolled by the lean, shaken by the pitch, lowered by the suspension and the wheelie */
void bike3d_cockpit(const Rider *rd, float lean)
{
    M rg, rs, rr, r0, n;
    V a, t, p;
    float x = f32(rd->wheelie * 0.6366197723675814);           /* 2 / pi */
    float roll = f32(-(double)lean * 0.4);
    int base;
    a[0] = 0.0f;
    a[1] = f32(((double)x * -80.0f + (double)rd->susp * 10.0) - 350.0);
    a[2] = f32((double)rd->susp * 5.0f + 350.0f);
    rmat(rg, g_shake, 0.0, 0.0);
    rmat(rs, 0.0, f32(rd->steer * 0.0002857142826542258f), 0.0);
    rmat(rr, 0.0, 0.0, roll);
    vm(t, a, (const float (*)[3])rg, 0);       /* the handlebar's place */
    mm(n, (const float (*)[3])rg, (const float (*)[3])rr);
    mm(n, (const float (*)[3])n, (const float (*)[3])rs);
    base = r3d_points(g_bar.nv, (const float (*)[3])g_bar.v, (const float (*)[3])n, t);
    if (base >= 0)
        r3d_faces_cockpit(base, g_bar.nf, g_bar.f);
    vm(p, a, (const float (*)[3])rg, 0);       /* the cockpit's (the inline roll of the
                                                   game, R(0, 0, roll) R(0, 0, 0)) */
    rmat(r0, 0.0, 0.0, 0.0);
    mm(n, (const float (*)[3])rg, (const float (*)[3])rr);
    mm(n, (const float (*)[3])n, (const float (*)[3])r0);
    base = r3d_points(g_pit.nv, (const float (*)[3])g_pit.v, (const float (*)[3])n, p);
    if (base >= 0)
        r3d_faces_cockpit(base, g_pit.nf, g_pit.f);
}
