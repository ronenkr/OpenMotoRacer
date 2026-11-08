/* The software 3D renderer (NOTES §4.9.3): the menus' (focal length 1911, near plane 256)
   and the race's (0x410e10: focal 394, near 64, the kinds remapped by the detail level and
   the near road drawn with perspective-correct spans, build/notes/track_raster.md).
   Centre (320, 240). Every step of the transform is a float, as the game runs it with the
   x87 at 24-bit precision (0x49b2c0); the drawing runs at 53 bits, its stored values
   floats. */
#include "r3d.h"
#include "files.h"
#include "ftol.h"
#include "video.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef R3D_TWO
#ifdef OMR_ESP32
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#else
#include <linux/futex.h>
#include <pthread.h>
#include <sched.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif
#endif
#include "esp32.h"
#if defined(OMR_ESP32) && defined(R3D_TWO)
#define R3D_PIPE 1      /* the board: a scene drawn on core 1 from a copy while the next is built */
#endif
#include "fastfloat.h"      /* (last: the ESP32's single precision) */

#define W g_scr_w
#define H g_scr_h
#define CX g_cx
#define CY g_cy
#define TO_INDEX 2607.594587618272      /* 0x4d1bf8, 0x4d0300: 16384 / 2pi as the game stores it */

#define VERTS 16384             /* transformed points (0x6d20d8, 0x1c bytes each) */
#define BUCKETS 7999            /* 0x655d38: 4 units of depth each */
#define ENTRIES 7499            /* 0x65f0e0 */
#define FREE_MAX 256            /* free pieces a screen row can hold */
#if defined(OMR_ESP32) && defined(R3D_TWO)
#define SPANS 0x1400            /* (the board: two of them in the room of one; at 320x240) */
#else
#define SPANS 0x2800            /* see-through pieces put off till last (0x68b040) */
#endif
#define BATCHES 1000            /* their triangles (0x625790) */
#define FACES_MAX 2000          /* 0x62e438: faces drawn a frame, the 2001st on dropped */
#define HALF 0x7bef             /* 0x50270c: each channel without its low bit, halved */

float g_cos[16384];
uint16_t *g_texpage[R3D_PAGES];
static uint8_t *g_pageidx[R3D_PAGES];     /* a paletted page's 8-bit texels (page +0x330) */
static uint16_t *g_pagepal[R3D_PAGES];    /* and its palettes, 256 colours each (page +0xc) */
static int g_npal[R3D_PAGES];             /* how many (page +8) */
static uint16_t *g_skintex[R3D_PAGES][8]; /* the texels through palette k, made when needed */
#ifdef OMR_ESP32
/* The board (NOTES §4.26): a texture is a page's 16-bit texels, or its 8-bit ones and a palette:
   a bike's liveries are its one page through 8 palettes (no 128 KB copy each), and a track's
   pages are made 8-bit with their own 256 colours (r3d_page_to8). Half the memory, and half the
   PSRAM a texel read brings into the cache. */
typedef struct {
    const uint16_t *rgb;        /* 16-bit texels, or 0 */
    const uint8_t *idx;         /* else 8-bit ones */
    const uint16_t *pal;        /* and their colours */
} Tex;
typedef const Tex *TEX;
static Tex g_tex[R3D_PAGES][8];
static uint8_t *g_q8idx[R3D_PAGES];      /* a page made 8-bit: its texels */
static uint16_t *g_q8pal[R3D_PAGES];     /* and palette */
#define TEX_LOCALS(t) const uint16_t *t_rgb = (t) ? (t)->rgb : 0; \
    const uint8_t *t_idx = (t) ? (t)->idx : 0; const uint16_t *t_pal = (t) ? (t)->pal : 0; \
    (void)t_rgb, (void)t_idx, (void)t_pal
#define S16(i) t_rgb[i]
#define S8(i) t_pal[t_idx[i]]
/* a loop over texels, written twice: 16-bit texels and indices into a palette */
#define TWICE(LOOP) do { if (t_rgb) { LOOP(S16) } else { LOOP(S8) } } while (0)
#else
typedef const uint16_t *TEX;
#define TEX_LOCALS(t) const uint16_t *t_rgb = (t); (void)t_rgb
#define S16(i) t_rgb[i]
#define TWICE(LOOP) do { LOOP(S16) } while (0)
#endif

typedef struct {                /* 0x49b2c0's output record */
    float x, y, z;              /* in the view, x and y times the focal length */
    int sx, sy;                 /* on the screen when z >= NEAR */
} Point;

typedef struct {
    const Face *f;
    int base;                   /* its object's first point */
    int skin;                   /* 0x49ca70: the palette of a paletted page's face; -1 none */
    int dark;                   /* its points' light word (0x49b2c0) is below 0x3e000 */
    int next;
#ifdef R3D_PIPE
    int kind;                   /* its face's kind as last queued, taken when drawing starts */
#endif
} Entry;

typedef struct {                /* a see-through piece put off till last */
    int16_t y, x0, x1;
    int u, v, du, dv;           /* the affine kinds: 13-bit fixed texels */
    float e[3];                 /* sub 6: u/z, v/z, 1/z at x0 */
} Span;

typedef struct {                /* 0x625790: what its pieces share */
    int first;
    int kind;
    uint16_t colour;
    TEX tex;
    float g16[3];               /* sub 6: the steps over 16 pixels */
} Batch;

/* who draws a set of rows (those with (y & mask) == val), with the see-through pieces it puts off.
   One does all the rows, or (R3D_TWO, the MiSTer) one on each CPU every other row: a row's pixels
   depend on that row's free pieces and nothing else, so the picture is the same. (But for what
   runs out: each counts its own pieces and batches, so with two a frame that ran out of room for
   see-through faces draws more of them; and a painter that runs off its row, span6's note, can
   meet the other thread. Neither was met in 240 pictures compared, NOTES 4.25.) */
typedef struct {
    Span *span;
    Batch *batch;
    int nspan, nbatch;
    int room;                   /* 0x6d20c0: 0 once the pieces or batches ran out */
    int mask, val;
} __attribute__((aligned(32))) Drawer;

/* 0x623bd8: a row's free pieces [start, end), the count beside the first of them (the same cache
   line, and none shared with another row's) */
typedef struct {
    int n;
    int16_t p[2 * FREE_MAX];
} __attribute__((aligned(32))) FreeRow;

static Point g_pt[VERTS];
static int g_npt;
static int g_head[BUCKETS];
static int g_bucket_lo, g_bucket_hi = BUCKETS - 1;   /* the ones in use */
static Entry g_entry[ENTRIES];
static int g_nentry;
static FreeRow g_row[SCREEN_MAX_H];
#ifdef R3D_PIPE
/* what the drawing reads: the lists themselves, or (r3d_render_async) a copy of them, so the next
   scene can be built meanwhile; and the picture it draws on */
static Point *g_dpt = g_pt;
static Entry *g_dentry = g_entry;
static int *g_dhead = g_head;
static int g_dlo, g_dhi = -1;
static uint16_t *g_dscreen;
#else
#define g_dpt g_pt
#define g_dentry g_entry
#define g_dhead g_head
#define g_dlo g_bucket_lo
#define g_dhi g_bucket_hi
#define g_dscreen g_screen
#endif
/* (int)nearbyintf(x): the ARM has it in one instruction, the library a call */
static inline int round_int(float x)
{
#if defined(__arm__) && defined(__ARM_FP)
    float r;
    int i;
    __asm__("vcvtr.s32.f32 %0, %1" : "=t"(r) : "t"(x));
    memcpy(&i, &r, sizeof i);
    return i;
#else
    return (int)nearbyintf(x);
#endif
}

#ifdef __arm__
static uint32_t g_reciprocal[1024];     /* the MiSTer's ARM divides in software: small divisors here */
#endif

static float g_cx = 320.0f, g_cy = 240.0f;      /* 0x60d3c4, 0x624f88: half the view (0x499ba0) */
static Span g_span[SPANS];
static Batch g_batch[BATCHES + 1];
#ifdef R3D_TWO
static Span g_span2[SPANS];
static Batch g_batch2[BATCHES + 1];
static int g_two;                         /* the second CPU draws every other row */
static Drawer g_draw[2] = { { g_span, g_batch, 0, 0, 1, 0, 0 }, { g_span2, g_batch2, 0, 0, 1, 0, 0 } };
#else
static Drawer g_draw[1] = { { g_span, g_batch, 0, 0, 1, 0, 0 } };
#endif
static float g_cam[3][3], g_campos[3];    /* the camera block 0x5161c0 */
static float g_focal = 1911.0f;           /* 0x49a0e0/0x49a100: the menu's 1911 (0x40a003) */
static float g_near = 256.0f;             /* 0x625774: the menu's 256, the race's 64 */
static int g_detail = -1;                 /* the race's detail row; -1 the menus' (0x4dc0c4) */
static int g_dark;                        /* r3d_dark: the light word of what is queued now */
static int g_swap;                        /* and the wheels' kinds swapped (0x4221d0) */

void r3d_race(int detail)
{
    g_detail = detail;
    g_near = detail < 0 ? 256.0f : 64.0f;
}

void r3d_dark(int dark, int swap)
{
    g_dark = dark;
    g_swap = swap;
}

void r3d_focal(float f)
{
    g_focal = f;
}

void r3d_screen(void)
{
    g_cx = (float)(g_scr_w >> 1);
    g_cy = (float)(g_scr_h >> 1);
}

#ifdef MISTER_HYBRID
void r3d_screen_down(int y)
{
    g_cy += (float)y;
}
#endif

void r3d_init(void)
{
    int k;
#ifdef __arm__
    for (k = 1; k < 1024; k++)
        g_reciprocal[k] = 0xffffffffu / (uint32_t)k;
#endif
    for (k = 0; k < 16384; k++)                         /* 0x4901e0 */
        g_cos[k] = (float)cos(k * 6.283185307178 * (double)6.103515625e-05f);
    r3d_begin();
#ifdef R3D_TWO
    if (getenv("OMR_R3D_TWO"))                          /* (to compare pictures without a core) */
        r3d_two_cpus(NULL);
#endif
}

uint16_t *r3d_page(int n)
{
    if (n < 0 || n >= R3D_PAGES)
        return 0;
    if (!g_texpage[n])
        g_texpage[n] = calloc(65536, sizeof(uint16_t));
#ifdef OMR_ESP32
    if (g_texpage[n]) {                 /* 16-bit again (a page made 8-bit is given up) */
        free(g_q8idx[n]);
        free(g_q8pal[n]);
        g_q8idx[n] = 0;
        g_q8pal[n] = 0;
        g_tex[n][0].rgb = g_texpage[n];
        g_tex[n][0].idx = 0;
        g_tex[n][0].pal = 0;
    }
#endif
    return g_texpage[n];
}

void r3d_free_page(int n)
{
    int k;
    if (n >= 0 && n < R3D_PAGES) {
        free(g_texpage[n]);
        g_texpage[n] = 0;
        free(g_pageidx[n]);
        g_pageidx[n] = 0;
        free(g_pagepal[n]);
        g_pagepal[n] = 0;
        g_npal[n] = 0;
        for (k = 0; k < 8; k++) {
            free(g_skintex[n][k]);
            g_skintex[n][k] = 0;
        }
#ifdef OMR_ESP32
        free(g_q8idx[n]);
        free(g_q8pal[n]);
        g_q8idx[n] = 0;
        g_q8pal[n] = 0;
        memset(g_tex[n], 0, sizeof g_tex[n]);
#endif
    }
}

/* the DirectDraw format's shifts applied as "top 5 bits": green keeps only 5 */
static uint16_t texel(const unsigned char *rgb)
{
    return (uint16_t)((rgb[0] >> 3) << 11 | (rgb[1] >> 3) << 6 | rgb[2] >> 3);
}

/* 0x401160: an Image Alchemy RAW, the first 256 texels of each row */
int r3d_load_page(int n, const char *path)
{
    long len;
    unsigned char *d = dsi_load(path, &len);
    uint16_t *t;
    int w, h, npal, x, y;
    if (!d)
        return 0;
    w = d[8] << 8 | d[9];
    h = d[10] << 8 | d[11];
    npal = d[12] << 8 | d[13];
    if (len < 32 || memcmp(d, "mhwanh", 6) || w < 256
        || len < 32 + 3L * npal + (long)w * h * (npal ? 1 : 3) || !(t = r3d_page(n))) {
        free(d);
        return 0;
    }
    if (h > 256)
        h = 256;
    for (y = 0; y < h; y++)
        for (x = 0; x < 256; x++)
            t[y * 256 + x] = npal ? texel(d + 32 + 3 * d[32 + 3 * npal + (long)y * w + x])
                                  : texel(d + 32 + 3 * ((long)y * w + x));
    free(d);
    return 1;
}

int r3d_load_page_pal(int n, const char *path)
{
    char name[256];
    long len;
    unsigned char *d = 0;
    int w, h, npal, y, k;
    char *dot;
    if (n < 0 || n >= R3D_PAGES || !(d = dsi_load(path, &len)) || len < 32
        || memcmp(d, "mhwanh", 6)) {
        free(d);
        return 0;
    }
    w = d[8] << 8 | d[9];
    h = d[10] << 8 | d[11];
    npal = d[12] << 8 | d[13];
    if (npal == 0 || npal > 256) {              /* 24 bits: the usual page */
        free(d);
        return r3d_load_page(n, path);
    }
    r3d_free_page(n);
    if (w < 256 || len < 32 + 3L * npal + (long)w * h
        || !(g_pageidx[n] = calloc(65536, 1)) || !(g_pagepal[n] = calloc(8 * 256, 2))) {
        free(d);
        r3d_free_page(n);
        return 0;
    }
    if (h > 256)
        h = 256;
    for (y = 0; y < h; y++)
        memcpy(g_pageidx[n] + y * 256, d + 32 + 3 * npal + (long)y * w, 256);
    for (k = 0; k < npal; k++)
        g_pagepal[n][k] = texel(d + 32 + 3 * k);
    free(d);
    /* the others: whole RAW files of which only the 0x20-byte header is passed over */
    snprintf(name, sizeof name, "%s", path);
    if ((dot = strrchr(name, '.')) && !strpbrk(dot, "\\/"))
        *dot = 0;
    for (g_npal[n] = 1; g_npal[n] < 8; g_npal[n]++) {
        char pal[300];
        snprintf(pal, sizeof pal, "%s.%d", name, g_npal[n]);     /* 0x4db1e4 */
        if (!(d = dsi_load(pal, &len)))
            break;
        if (len >= 32 + 768)
            for (k = 0; k < 256; k++)
                g_pagepal[n][256 * g_npal[n] + k] = texel(d + 32 + 3 * k);
        free(d);
    }
    return 1;
}

const uint8_t *r3d_page_index(int n)
{
    return n >= 0 && n < R3D_PAGES ? g_pageidx[n] : 0;
}

const uint16_t *r3d_page_palette(int n, int k)
{
    if (n < 0 || n >= R3D_PAGES || !g_pageidx[n])
        return 0;
    return g_pagepal[n] + 256 * (k >= 0 && k < g_npal[n] ? k : 0);
}

#ifdef OMR_ESP32
/* ---------------------------------------------------------------- 8-bit pages (the board) */
typedef struct { uint16_t c; uint16_t n; } Count;   /* a colour and how many texels have it */

static int by_colour(const void *a, const void *b)
{
    return (int)*(const uint16_t *)a - (int)*(const uint16_t *)b;
}

/* the texel layout r5 g5 (at bit 6) b5 as three 5-bit channels */
#define CH(c, k) ((k) == 0 ? (c) >> 11 & 31 : (k) == 1 ? (c) >> 6 & 31 : (c) & 31)

static int by_ch0(const void *a, const void *b) { return CH(((const Count *)a)->c, 0) - CH(((const Count *)b)->c, 0); }
static int by_ch1(const void *a, const void *b) { return CH(((const Count *)a)->c, 1) - CH(((const Count *)b)->c, 1); }
static int by_ch2(const void *a, const void *b) { return CH(((const Count *)a)->c, 2) - CH(((const Count *)b)->c, 2); }

/* A page's 16-bit texels made 8-bit and a palette of 256: exactly when it has 256 colours or
   fewer, else by median cut. Colour 0 (see-through to the kinds that test it) stays exactly 0
   and nothing else becomes 0. */
void r3d_page_to8(int n)
{
    uint16_t *src, *sorted = 0, *pal = 0;
    uint8_t *idx = 0, *map = 0;
    Count *u = 0;
    int nu = 0, i, j, k, has0 = 0, first, boxes;
    struct { int lo, hi; } box[256];
    if (n < 0 || n >= R3D_PAGES || !(src = g_texpage[n]))
        return;
    if (!(sorted = malloc(65536 * 2)) || !(idx = malloc(65536)) || !(pal = calloc(256, 2)))
        goto out;
    memcpy(sorted, src, 65536 * 2);
    qsort(sorted, 65536, 2, by_colour);
    for (i = 0; i < 65536; i++)                 /* the colours, each once, with their counts */
        if (!i || sorted[i] != sorted[i - 1])
            nu++;
    if (!(u = malloc(sizeof *u * (size_t)nu)) || !(map = malloc((size_t)nu)))
        goto out;
    for (i = 0, j = -1; i < 65536; i++) {
        if (!i || sorted[i] != sorted[i - 1])
            u[++j].c = sorted[i], u[j].n = 0;
        if (u[j].n < 0xffff)
            u[j].n++;
    }
    has0 = u[0].c == 0;
    first = has0;                               /* the colours that are not 0 */
    if (nu <= 256) {
        for (i = 0; i < nu; i++)
            pal[i] = u[i].c, map[i] = (uint8_t)i;
    } else {
        /* median cut over the non-zero colours into 256 - has0 boxes: the box with the widest
           channel cut at its texels' middle along that channel */
        boxes = 1;
        box[0].lo = first, box[0].hi = nu;
        while (boxes < 256 - has0) {
            int best = -1, bw = 0, bk = 0;
            for (i = 0; i < boxes; i++) {
                int mn[3] = { 31, 31, 31 }, mx[3] = { 0, 0, 0 };
                if (box[i].hi - box[i].lo < 2)
                    continue;
                for (j = box[i].lo; j < box[i].hi; j++)
                    for (k = 0; k < 3; k++) {
                        int v = CH(u[j].c, k);
                        mn[k] = v < mn[k] ? v : mn[k];
                        mx[k] = v > mx[k] ? v : mx[k];
                    }
                for (k = 0; k < 3; k++)
                    if (mx[k] - mn[k] > bw)
                        bw = mx[k] - mn[k], best = i, bk = k;
            }
            if (best < 0)
                break;
            {
                int lo = box[best].lo, hi = box[best].hi, half, acc = 0, total = 0, cut;
                qsort(u + lo, (size_t)(hi - lo), sizeof *u, bk == 0 ? by_ch0 : bk == 1 ? by_ch1 : by_ch2);
                for (j = lo; j < hi; j++)
                    total += u[j].n;
                half = total / 2;
                for (cut = lo; cut < hi - 1 && acc + u[cut].n <= half; cut++)
                    acc += u[cut].n;
                if (cut == lo)
                    cut = lo + 1;
                box[boxes].lo = cut, box[boxes].hi = hi;
                box[best].hi = cut;
                boxes++;
            }
        }
        if (has0)
            pal[0] = 0, map[0] = 0;
        for (i = 0; i < boxes; i++) {           /* each box's colour: its texels' mean */
            long t = 0, sum[3] = { 0, 0, 0 };
            int c;
            for (j = box[i].lo; j < box[i].hi; j++) {
                for (k = 0; k < 3; k++)
                    sum[k] += (long)CH(u[j].c, k) * u[j].n;
                t += u[j].n;
            }
            c = (int)((sum[0] + t / 2) / t) << 11 | (int)((sum[1] + t / 2) / t) << 6 | (int)((sum[2] + t / 2) / t);
            pal[has0 + i] = (uint16_t)(c ? c : 1);
        }
        qsort(u + first, (size_t)(nu - first), sizeof *u, by_colour);   /* (back in colour order) */
        /* each colour to its nearest palette colour (not always its own box's), the zero apart */
        for (j = first; j < nu; j++) {
            int bestd = 1 << 30, bi = has0;
            for (i = has0; i < has0 + boxes; i++) {
                int d = 0;
                for (k = 0; k < 3; k++) {
                    int e = CH(u[j].c, k) - CH(pal[i], k);
                    d += e * e;
                }
                if (d < bestd)
                    bestd = d, bi = i;
            }
            map[j] = (uint8_t)bi;
        }
    }
    for (i = 0; i < 65536; i++) {               /* each texel by its colour's place in u */
        int lo = 0, hi = nu - 1, c = src[i];
        while (lo < hi) {
            int mid = (lo + hi) / 2;
            if (u[mid].c < c)
                lo = mid + 1;
            else
                hi = mid;
        }
        idx[i] = map[lo];
    }
    free(g_texpage[n]);
    g_texpage[n] = 0;
    free(g_q8idx[n]);
    free(g_q8pal[n]);
    g_q8idx[n] = idx;
    g_q8pal[n] = pal;
    g_tex[n][0].rgb = 0;
    g_tex[n][0].idx = idx;
    g_tex[n][0].pal = pal;
    idx = 0, pal = 0;
out:
    free(sorted);
    free(u);
    free(map);
    free(idx);
    free(pal);
}
#endif

/* a paletted page seen through palette k: what 0x4a7240 reads, palette[texel] */
static TEX skin_texels(int n, int k)
{
    const uint16_t *pal = r3d_page_palette(n, k);
#ifndef OMR_ESP32
    int i;
#endif
    if (!pal)
        return 0;
    k = (int)(pal - g_pagepal[n]) / 256;
#ifdef OMR_ESP32
    g_tex[n][k].rgb = 0;                /* (the indices through the palette, as they are) */
    g_tex[n][k].idx = g_pageidx[n];
    g_tex[n][k].pal = pal;
    return &g_tex[n][k];
#else
    if (!g_skintex[n][k] && (g_skintex[n][k] = malloc(65536 * 2)))
        for (i = 0; i < 65536; i++)
            g_skintex[n][k][i] = pal[g_pageidx[n][i]];
    return g_skintex[n][k];
#endif
}

/* ---------------------------------------------------------------- the camera, objects */

void r3d_camera(const float pos[3], const float target[3])
{
    double d[3], l, nx, ny, nz, hz;
    int k;
    for (k = 0; k < 3; k++) {
        d[k] = (double)target[k] - pos[k];
        g_campos[k] = pos[k];
    }
    l = sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
    nx = d[0] / l, ny = d[1] / l, nz = d[2] / l;
    hz = sqrt(nx * nx + nz * nz);
    /* a yaw (nz/h, nx/h) then a pitch (h, ny), for row vectors */
    g_cam[0][0] = (float)(nz / hz);
    g_cam[0][1] = (float)(-nx / hz * ny);
    g_cam[0][2] = (float)(nx / hz * hz);
    g_cam[1][0] = 0.0f;
    g_cam[1][1] = (float)hz;
    g_cam[1][2] = (float)ny;
    g_cam[2][0] = (float)(-nx / hz);
    g_cam[2][1] = (float)(-nz / hz * ny);
    g_cam[2][2] = (float)(nz / hz * hz);
}

void r3d_get_camera(float c[3][3], float pos[3])
{
    memcpy(c, g_cam, sizeof g_cam);
    memcpy(pos, g_campos, sizeof g_campos);
}

void r3d_camera_pos(const float pos[3])
{
    memcpy(g_campos, pos, sizeof g_campos);
}

void r3d_rotation(float m[3][3], const float ang[3], omr_double k)
{
    int i[3], j;
    double c[3], s[3];
    for (j = 0; j < 3; j++) {
        i[j] = (int)(ang[j] * k);
        c[j] = g_cos[i[j] & 0x3fff];
        s[j] = g_cos[(i[j] - 0x1000) & 0x3fff];
    }
    m[0][0] = (float)(c[2] * c[1]);
    m[0][1] = (float)(s[2] * c[0] + c[2] * s[1] * s[0]);
    m[0][2] = (float)(s[2] * s[0] - c[2] * s[1] * c[0]);
    m[1][0] = (float)(-s[2] * c[1]);
    m[1][1] = (float)(c[2] * c[0] - s[2] * s[1] * s[0]);
    m[1][2] = (float)(c[2] * s[0] + s[2] * s[1] * c[0]);
    m[2][0] = (float)s[1];
    m[2][1] = (float)(-c[1] * s[0]);
    m[2][2] = (float)(c[1] * c[0]);
}

void r3d_set_camera(const float c[3][3], const float pos[3])
{
    memcpy(g_cam, c, sizeof g_cam);
    memcpy(g_campos, pos, sizeof g_campos);
}

void r3d_object(const Model *m, const float ang[3], const float pos[3])
{
    float r[3][3];
    r3d_rotation(r, ang, TO_INDEX);
    r3d_object_m(m, (const float (*)[3])r, pos);
}

void r3d_place(float n[3][3], float t[3], const float r[3][3], const float pos[3])
{
    float d[3];
    int i, j;
    for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++)
            n[i][j] = (float)((double)r[i][0] * g_cam[0][j] + (double)r[i][1] * g_cam[1][j]
                              + (double)r[i][2] * g_cam[2][j]);
    for (i = 0; i < 3; i++)
        d[i] = pos[i] - g_campos[i];
    for (j = 0; j < 3; j++)
        t[j] = (float)((double)d[0] * g_cam[0][j] + (double)d[1] * g_cam[1][j]
                       + (double)d[2] * g_cam[2][j]);
}

void r3d_object_m(const Model *m, const float r[3][3], const float pos[3])
{
    float n[3][3], t[3];
    r3d_place(n, t, r, pos);
    r3d_view(m->nv, (const float (*)[3])m->v, m->nf, m->f, (const float (*)[3])n, t);
}

/* 0x479eb0 / 0x47af50: M = Rz(c) Ry(b) Rx(a), with the partial products cc*sb and sc*sb
   stored as floats as the code keeps them */
void r3d_menu_matrix(float m[3][3], const float ang[3])
{
    double c[3], s[3];
    float t, t2;
    int j;
    for (j = 0; j < 3; j++) {
        int i = (int)(ang[j] * TO_INDEX);
        c[j] = g_cos[i & 0x3fff];
        s[j] = g_cos[(i - 0x1000) & 0x3fff];
    }
    t = (float)(c[2] * s[1]);
    t2 = (float)(s[2] * s[1]);
    m[0][0] = (float)(c[2] * c[1]);
    m[0][1] = (float)(s[2] * c[0] + t * s[0]);
    m[0][2] = (float)(s[2] * s[0] - t * c[0]);
    m[1][0] = (float)-(s[2] * c[1]);
    m[1][1] = (float)(c[2] * c[0] - t2 * s[0]);
    m[1][2] = (float)(c[2] * s[0] + t2 * c[0]);
    m[2][0] = (float)s[1];
    m[2][1] = (float)-(c[1] * s[0]);
    m[2][2] = (float)(c[1] * c[0]);
}

static float g_mcam[3][3], g_mcamt[3];    /* the menu's camera object 0x504bc8 */

/* 0x47af50: the view is the inverse of the camera's placement */
void r3d_menu_camera(const float ang[3], const float pos[3])
{
    float r[3][3];
    double px = pos[0], py = pos[1], pz = pos[2];
    int i, j;
    r3d_menu_matrix(r, ang);
    for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++)
            g_mcam[i][j] = r[j][i];
    g_mcamt[0] = (float)-((r[0][2] * pz + py * r[0][1]) + r[0][0] * px);
    g_mcamt[1] = (float)-((pz * r[1][2] + r[1][0] * px) + py * r[1][1]);
    g_mcamt[2] = (float)-((py * r[2][1] + r[2][0] * px) + pz * r[2][2]);
}

/* 0x479eb0(object, camera): N = M C, T = pos C + Ct */
static void menu_nt(float n[3][3], float t[3], const float ang[3], const float pos[3])
{
    float m[3][3];
    int i, j;
    r3d_menu_matrix(m, ang);
    for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++)
            n[i][j] = (float)(((double)m[i][0] * g_mcam[0][j] + (double)m[i][1] * g_mcam[1][j])
                              + (double)m[i][2] * g_mcam[2][j]);
    for (j = 0; j < 3; j++)
        t[j] = (float)((((double)pos[0] * g_mcam[0][j] + (double)pos[1] * g_mcam[1][j])
                        + (double)pos[2] * g_mcam[2][j]) + g_mcamt[j]);
}

void r3d_menu_object(const Model *m, const float ang[3], const float pos[3])
{
    float n[3][3], t[3];
    menu_nt(n, t, ang, pos);
    r3d_view(m->nv, (const float (*)[3])m->v, m->nf, m->f, (const float (*)[3])n, t);
}

/* 0x49b2c0 on one point through the menu's camera: where it lands on the screen */
int r3d_menu_project(const float v[3], int *sx, int *sy)
{
    const float (*n)[3] = g_mcam, *t = g_mcamt;     /* the camera itself (0x488383) */
    float x, y, z, rz;
    x = v[0] * (n[0][0] * g_focal) + v[1] * (n[1][0] * g_focal) + v[2] * (n[2][0] * g_focal)
        + t[0] * g_focal;
    y = v[0] * (n[0][1] * g_focal) + v[1] * (n[1][1] * g_focal) + v[2] * (n[2][1] * g_focal)
        + t[1] * g_focal;
    z = v[0] * n[0][2] + v[1] * n[1][2] + v[2] * n[2][2] + t[2];
    if (!(z >= g_near))
        return 0;
    rz = 1.0f / z;
    *sx = (int)nearbyintf(x * rz + CX);
    *sy = round_int(CY - y * rz);
    return 1;
}

/* ---------------------------------------------------------------- transform and sort */

void r3d_begin(void)
{
    int y;
    unsigned i;
    g_npt = 0;
    g_nentry = 0;
#ifdef R3D_PIPE
    /* (the board: the lists only; the drawing's own state at the start of each, draw_start) */
    for (y = g_bucket_lo; y <= g_bucket_hi; y++)
        g_head[y] = -1;
    g_bucket_lo = BUCKETS, g_bucket_hi = -1;
    (void)i;
    return;
#endif
    for (i = 0; i < sizeof g_draw / sizeof g_draw[0]; i++) {
        g_draw[i].nspan = 0;
        g_draw[i].nbatch = 0;
        g_draw[i].room = 1;
    }
    for (y = g_bucket_lo; y <= g_bucket_hi; y++)
        g_head[y] = -1;
    g_bucket_lo = BUCKETS, g_bucket_hi = -1;
    for (y = 0; y < H; y++) {
        g_row[y].p[0] = 0;
        g_row[y].p[1] = (int16_t)W;
        g_row[y].n = 1;
    }
}

/* 0x49c590's remap in the race (*0x4dc0c4 == 0), by the detail row (CurrentDetail): the near
   textured faces take the perspective-correct twins (16 -> 31, 21 -> 32, 38 -> 53, 39 -> 54,
   44 -> 62: nearer than 6144 at 4, than 3072 below), below 2, and at 2 without flag 8, the
   textures go (16, 21 -> 0 flat; 38, 39 -> 37 the stencil or near 51; 44 -> 12 or near 0) */
static int remap(int kind, float key, int flags)
{
    /* the properties of the kinds, made in 0x4995a0 (as far as 0x45): 1 textured, 2 half
       the texel's brightness, 4 RGB page?, 8 stencil, 0x10 see-through, 0x20 perspective,
       0x40 left alone, 0x80 16 bits; the kind of a set of properties is the first with it */
    static const unsigned char prop[0x46] = {
        0x20, 0x01, 0x09, 0x03, 0x0b, 0x02, 0x40, 0x40, 0x11, 0x13, 0x40, 0x40, 0x10, 0x40,
        0x40, 0x40, 0x81, 0x50, 0x03, 0x0b, 0x13, 0x85, 0x00, 0x02, 0x01, 0x03, 0x03, 0x22,
        0x21, 0x23, 0x23, 0xa1, 0xa5, 0x22, 0x21, 0x23, 0x23, 0x08, 0x89, 0x8d, 0x09, 0x09,
        0x08, 0x13, 0x91, 0x95, 0x10, 0x12, 0x11, 0x13, 0x29, 0x28, 0x2b, 0xa9, 0xad, 0x29,
        0x2b, 0x28, 0x32, 0x31, 0x33, 0x33, 0xb1, 0xb5, 0x32, 0x31, 0x31, 0x33, 0x40, 0x40
    };
    int p, i;
    if (g_detail < 0 || kind < 0 || kind >= 0x46 || prop[kind] & 0x40)
        return kind;
    p = prop[kind];
    if (p & 1 && key < (g_detail >= 4 ? 6144.0f : 3072.0f))
        p |= 0x20;
    if (g_detail < 2 || (g_detail == 2 && !(flags & 8)))
        p &= ~0x85;
    for (i = 0; p && i < 0x46; i++)
        if (prop[i] == p)
            return i;
    return 0;
}

/* 0x49c590 (skin < 0) and 0x49ca70: into the bucket of its depth, the newest first */
static void queue_kind(const Face *f, int base, float key, int skin, int kind)
{
    int b = (int)(key * 0.25f);
    if (b < 0)
        return;
    ((Face *)f)->drawn = kind;
    if (g_nentry >= ENTRIES)
        return;
    if (skin >= 0)              /* (made here, where no other thread draws) */
        skin_texels(f->page, skin);
    if (b > BUCKETS - 1)
        b = BUCKETS - 1;
    g_entry[g_nentry].f = f;
    g_entry[g_nentry].base = base;
    g_entry[g_nentry].skin = skin;
    g_entry[g_nentry].dark = g_dark;
    g_entry[g_nentry].next = g_head[b];
    g_head[b] = g_nentry++;
    if (b < g_bucket_lo)
        g_bucket_lo = b;
    if (b > g_bucket_hi)
        g_bucket_hi = b;
}

static void queue(const Face *f, int base, float key, int skin)
{
    int kind = f->kind;
    if (skin >= 0) {            /* 0x49ca70: in a race at detail 0, 1 flat, see-through 42 */
        if (g_detail == 0 || g_detail == 1)
            kind = kind == 2 ? 42 : 0;
    } else {
        if (g_swap)             /* 0x4221d0 */
            kind = kind == 0x10 ? 0x15 : kind == 0x26 ? 0x27 : kind;
        kind = remap(kind, key, f->flags);
    }
    queue_kind(f, base, key, skin, kind);
}

/* the dashboard's points (0x486310, 0x484810): already on the screen, at depth z */
int r3d_screen_points(int n, const int (*xy)[2], float z)
{
    int i, base = g_npt;
    if (g_npt + n > VERTS)
        return -1;
    for (i = 0; i < n; i++) {
        Point *p = &g_pt[g_npt++];
        p->sx = xy[i][0];
        p->sy = xy[i][1];
        p->z = z;
        p->x = ((float)p->sx - CX) * z;
        p->y = (CY - (float)p->sy) * z;
    }
    return base;
}

/* 0x49c590 with 0x4f1a18 = 1: every face at one key, its kind as it is, none culled */
void r3d_screen_faces(int base, int nf, const Face *f, float key)
{
    int i;
    if (base < 0)
        return;
    for (i = 0; i < nf; i++)
        if (f[i].kind != 0xd)
            queue_kind(&f[i], base, key, -1, f[i].kind);
}

/* 0x49c590 with 0x4f1a18 = 0, called on its own: every face at one key, none culled, its
   kind through a detail row (detail < 0: the race's own) */
void r3d_key_faces(int base, int nf, const Face *f, float key, int detail)
{
    int i, keep = g_detail;
    if (base < 0)
        return;
    if (detail >= 0)
        g_detail = detail;
    for (i = 0; i < nf; i++)
        if (f[i].kind != 0xd)
            queue(&f[i], base, key, -1);
    g_detail = keep;
}

int r3d_points(int nv, const float (*v)[3], const float n[3][3], const float t[3])
{
    float nx[4], ny[4], nz[4];
    int i, k, base = g_npt;
    if (g_npt + nv > VERTS)
        return -1;
    for (k = 0; k < 3; k++) {
        nx[k] = n[k][0] * g_focal;
        ny[k] = n[k][1] * g_focal;
        nz[k] = n[k][2];
    }
    nx[3] = t[0] * g_focal;
    ny[3] = t[1] * g_focal;
    nz[3] = t[2];
    for (i = 0; i < nv; i++) {                          /* 0x49b2c0 */
        Point *p = &g_pt[g_npt++];
        p->x = v[i][0] * nx[0] + v[i][1] * nx[1] + v[i][2] * nx[2] + nx[3];
        p->y = v[i][0] * ny[0] + v[i][1] * ny[1] + v[i][2] * ny[2] + ny[3];
        p->z = v[i][0] * nz[0] + v[i][1] * nz[1] + v[i][2] * nz[2] + nz[3];
        if (p->z >= g_near) {
            float rz = 1.0f / p->z;
            p->sx = round_int(p->x * rz + CX);
            p->sy = round_int(CY - p->y * rz);
        }
    }
    return base;
}

/* skin -2: 0x49bca0, the cockpit's: the farthest corner, the key z x 0.2 - near, every
   kind but 0 drawn as 0x10 (1 and 0x10) or 0x2c */
static void faces(int base, int nf, const Face *f, int skin)
{
    int i, k;
    for (i = 0; i < nf; i++) {                          /* 0x49b7f0, last argument 1 */
        const Face *fc = &f[i];
        const Point *p[4], *far;
        double area;
        int near = 0;
        if (skin < 0 ? fc->kind == 0xd : (fc->flags & 1) && fc->kind == 0xd)
            continue;
        for (k = 0; k < 4; k++)
            p[k] = &g_pt[base + fc->idx[k < fc->n ? k : 0]];
        far = p[0];
        for (k = 1; k < 4; k++)
            if (far->z < p[k]->z)
                far = p[k];
        if (!(far->z > g_near))
            continue;
        for (k = 0; k < 4; k++)
            if (p[k]->z <= g_near)
                near = 1;
        if (near) {             /* the back-face test in the view's space */
            const Point *a = p[1], *b = p[2], *c = p[0];
            double f6 = (double)a->y - b->y, f2 = (double)a->z - b->z;
            double f3 = (double)a->y - c->y, f4 = (double)a->z - c->z;
            area = far->z * (f6 * ((double)a->x - c->x) - ((double)a->x - b->x) * f3)
                   + (((double)a->x - b->x) * f4 - f2 * ((double)a->x - c->x)) * far->y
                   + far->x * (f2 * f3 - f6 * f4);
        } else {                /* wholly beyond a side of the screen, then the winding */
            int l = 1, r = 1, u = 1, d = 1;
            for (k = 0; k < 4; k++) {
                l &= p[k]->sx < 0;
                r &= p[k]->sx > W - 1;
                u &= p[k]->sy < 0;
                d &= p[k]->sy > H - 1;
            }
            if (l || r || u || d)
                continue;
            area = (double)(p[2]->sy - p[1]->sy) * (p[1]->sx - p[0]->sx)
                   + (double)(p[0]->sy - p[1]->sy) * (p[2]->sx - p[1]->sx);
        }
        if (!(area > 1.0))
            continue;
        if (skin == -2) {
            int kind = fc->kind == 0 ? 0 : fc->kind == 1 || fc->kind == 0x10 ? 0x10 : 0x2c;
            float key = far->z * 0.2f - g_near;
            queue_kind(fc, base, key, -1, remap(kind, key, fc->flags));
        } else if (skin < 0)
            queue(fc, base, far->z + (fc->flags & 2 ? 1024.0f : 0.0f), -1);
        else            /* 0x49c110: the farthest corner alone; a paletted page's own path */
            queue(fc, base, far->z, r3d_page_index(fc->page) ? skin : -1);
    }
}

void r3d_faces(int base, int nf, const Face *f, int skin)
{
    faces(base, nf, f, skin);
}

void r3d_faces_detail(int base, int nf, const Face *f, int detail)
{
    int keep = g_detail;
    g_detail = detail;
    faces(base, nf, f, -1);
    g_detail = keep;
}

void r3d_faces_cockpit(int base, int nf, const Face *f)
{
    faces(base, nf, f, -2);
}

void r3d_view(int nv, const float (*v)[3], int nf, const Face *f, const float n[3][3],
              const float t[3])
{
    int base = r3d_points(nv, v, n, t);
    if (base >= 0)
        r3d_faces(base, nf, f, -1);
}

/* ---------------------------------------------------------------- drawing */

static inline int cdiv(int a, int b)    /* C's division, as the game's idiv */
{
#ifdef __arm__
    if ((unsigned)(b - 1) < 1023) {
        uint32_t ua = a < 0 ? 0u - (uint32_t)a : (uint32_t)a, ub = (uint32_t)b;
        uint32_t q = (uint32_t)(((uint64_t)ua * g_reciprocal[b]) >> 32), r = ua - q * ub;
        while (r >= ub)
            q++, r -= ub;
        return a < 0 ? -(int)q : (int)q;
    }
#endif
    return a / b;
}

static int wrap(long long v)    /* a 32-bit register's sum or product */
{
    return (int)(uint32_t)v;
}

enum { K_SEE = 1, K_NO_PAINT = 2, K_SUB6 = 4 };

#define K_TABLE_SIZE 0x100
static const unsigned char g_kinds[K_TABLE_SIZE] = {
    [2] = K_SEE | K_NO_PAINT, [3] = K_NO_PAINT, [4] = K_SEE | K_NO_PAINT, [8] = K_SEE | K_NO_PAINT,
    [9] = K_SEE | K_NO_PAINT, [12] = K_SEE, [0x12] = K_NO_PAINT, [0x13] = K_SEE | K_NO_PAINT,
    [0x14] = K_SEE, [0x1d] = K_NO_PAINT, [28] = K_SUB6, [31] = K_SUB6, [32] = K_SUB6, [37] = K_SEE,
    [38] = K_SEE, [39] = K_SEE, [44] = K_SEE, [50] = K_SEE | K_SUB6, [51] = K_SEE | K_SUB6,
    [53] = K_SEE | K_SUB6, [54] = K_SEE | K_SUB6, [62] = K_SEE | K_SUB6, [68] = K_SEE, [69] = K_SEE,
};

static inline int kind_is(int kind, int what)
{
    return (unsigned)kind < K_TABLE_SIZE && g_kinds[kind] & what;
}

static inline int see_through(int kind)
{
    return kind_is(kind, K_SEE);
}

/* kind table 0x4f1a70: a span that is only a `ret`. The opaque ones (3, 0x12, 0x1d, and 1 once
   `face` has made it 3: here 1 is the palette's opaque face) still close their free pieces, so
   what lies behind them is not drawn; the see-through ones take pieces only. In the game 3, 4,
   9, 0x12 and 0x1d go through sub 5 and 7 (0x49fa10, 0x4a2630) where 1 goes through sub 4;
   nothing reaches them, so `triangle` stands in for all */
static inline int no_paint(int kind)
{
    return kind_is(kind, K_NO_PAINT);
}

static inline int sub6(int kind)        /* kind table 0x4f1a70: sub 6, the perspective-correct ones */
{
    return kind_is(kind, K_SUB6);
}

/* a batch closed: its pieces are g_span[first..] */
static void batch_end(Drawer *w, int first, int kind, TEX tex, uint16_t colour,
                      const float g16[3])
{
    Batch *b;
    if (w->nspan == first)
        return;
    if (w->nbatch >= BATCHES) {
        w->nspan = first;
        return;
    }
    b = &w->batch[w->nbatch++];
    b->first = first, b->kind = kind, b->tex = tex, b->colour = colour;
    b->g16[0] = g16 ? g16[0] : 0.0f, b->g16[1] = g16 ? g16[1] : 0.0f;
    b->g16[2] = g16 ? g16[2] : 0.0f;
    if (w->nbatch == BATCHES)
        w->room = 0;
}

static void span(int y, int x0, int x1, int u, int v, int du, int dv, int kind,
                 TEX tex, uint16_t colour)
{
    uint16_t *o = g_dscreen + y * g_pitch;
    int x, top = -y * g_pitch;
    TEX_LOCALS(tex);
    if (x0 < top) {                                     /* (above the screen: see span6) */
        u += (top - x0) * du, v += (top - x0) * dv;
        x0 = top;
    }
    if (no_paint(kind))
        return;
    if (kind == 12) {                                   /* 0x4a4110: half the screen + colour */
        for (x = x0; x < x1; x++)
            o[x] = (uint16_t)(((o[x] >> 1) & HALF) + colour);
        return;
    }
    if (kind == 0x10 || kind == 68 || kind == 69) {
        /* 0x4a5ab0, 0x4a4dc0, 0x4a4f30 keep u and v turned round, the fraction on top, and add
           with the carry: a slope below zero runs a little above a plain sum, and that shows
           on a texel's exact edge */
        uint32_t e = (uint32_t)u >> 13 | (uint32_t)u << 19, f = (uint32_t)v >> 13 | (uint32_t)v << 19;
        uint32_t de = (uint32_t)du >> 13 | (uint32_t)du << 19;
        uint32_t df = (uint32_t)dv >> 13 | (uint32_t)dv << 19;
        unsigned bl = e & 0xff, bh = f & 0xff;
#if defined(__arm__)
        if (kind == 0x10 && x0 < x1) {  /* the ARM adds with the carry as the x86 does */
            uint16_t *to = o + x0;
            unsigned dl = de & 0xff, dh = df & 0xff, t0, t1;
            int count = x1 - x0;
            __asm__ volatile(
                "1:\n"
                "uxtb %[t0], %[bl]\n"
                "uxtb %[t1], %[bh]\n"
                "orr %[t0], %[t0], %[t1], lsl #8\n"
#ifdef __thumb__
                "ldrh %[t0], [%[tex], %[t0], lsl #1]\n"
#else
                "add %[t0], %[tex], %[t0], lsl #1\n"
                "ldrh %[t0], [%[t0]]\n"
#endif
                "adds %[e], %[e], %[de]\n"
                "adc %[bl], %[bl], %[dl]\n"
                "adds %[f], %[f], %[df]\n"
                "adc %[bh], %[bh], %[dh]\n"
                "strh %[t0], [%[to]], #2\n"
                "subs %[n], %[n], #1\n"
                "bne 1b\n"
                : [e] "+r"(e), [f] "+r"(f), [bl] "+r"(bl), [bh] "+r"(bh), [to] "+r"(to),
                  [n] "+r"(count), [t0] "=&r"(t0), [t1] "=&r"(t1)
                : [de] "r"(de), [df] "r"(df), [dl] "r"(dl), [dh] "r"(dh), [tex] "r"(tex)
                : "cc", "memory");
            return;
        }
#endif
        if (kind == 0x10) {             /* (the road: most of a picture's pixels) */
#define LOOP10(S) \
            for (x = x0; x < x1; x++) { \
                uint32_t t; \
                o[x] = (uint16_t)S(bh << 8 | bl); \
                t = e + de, bl = (bl + (de & 0xff) + (t < e)) & 0xff, e = t; \
                t = f + df, bh = (bh + (df & 0xff) + (t < f)) & 0xff, f = t; \
            }
            TWICE(LOOP10);
#undef LOOP10
            return;
        }
#define LOOP68(S) \
        for (x = x0; x < x1; x++) { \
            unsigned c = S(bh << 8 | bl); \
            uint32_t t; \
            unsigned s = o[x], a = c & 0x1f;        /* each channel towards white by a/32 */ \
            o[x] = (uint16_t)(s + (((31 - (s >> 11)) * a >> 5) << 11) \
                              + (((31 - ((s >> 6) & 31)) * a >> 5) << 6) \
                              + (kind == 68 ? (31 - (s & 31)) * a >> 5 : 0)); /* (no blue in 69) */ \
            t = e + de, bl = (bl + (de & 0xff) + (t < e)) & 0xff, e = t; \
            t = f + df, bh = (bh + (df & 0xff) + (t < f)) & 0xff, f = t; \
        }
        TWICE(LOOP68);
#undef LOOP68
        return;
    }
#define TEXLOOP_A(S, what) \
    for (x = x0; x < x1; x++, u += du, v += dv) { \
        unsigned c = S(((v >> 13) * 256 + (u >> 13)) & 0xffff); \
        what; \
    }
#ifdef OMR_ESP32
#define TEXELS(what) \
    if (t_rgb) { TEXLOOP_A(S16, what) } else { TEXLOOP_A(S8, what) } \
    return
#else
#define TEXELS(what) TEXLOOP_A(S16, what) return
#endif
    switch (kind) {
    case 0x25: TEXELS(if (c) o[x] = colour);                    /* 0x4a50c0: the face's colour */
    case 0x26: TEXELS(if (c) o[x] = (uint16_t)c);               /* 0x4a5330 */
    case 0x27: TEXELS(if (c) o[x] = (uint16_t)((c >> 1) & HALF));   /* 0x4a5580 */
    case 0x2c:                                                  /* 0x4a57f0: half and half */
        TEXELS(if (c) o[x] = (uint16_t)(((o[x] >> 1) & HALF) + ((c >> 1) & HALF)));
    case 0x14:          /* 0x4adbf0: red from the table 0x4f62f0, which nothing writes; the
                           palette's colour 0 halves the screen */
        TEXELS(o[x] = (uint16_t)(c ? c & 0x7ff : (o[x] >> 1) & HALF));
    case 0x15: TEXELS(o[x] = (uint16_t)((c >> 1) & HALF));      /* 0x4a5e10 */
    default: TEXELS(o[x] = (uint16_t)c);
    }
#undef TEXELS
#undef TEXLOOP_A
}

/* 0x49d9e0 (opaque: draw the free pieces and close them) and 0x49dbf0 (see-through:
   keep the free pieces for the end) */
static void row(Drawer *w, int y, int x0, int x1, int u, int v, int du, int dv, int kind,
                TEX tex, int deferred)
{
    int16_t out[2 * FREE_MAX], *fr = g_row[y].p;
    int i, n, nf = g_row[y].n, first;
    if (deferred) {
        for (i = 0; i < nf; i++) {
            int a = fr[2 * i], b = fr[2 * i + 1];
            int s = a > x0 ? a : x0, e = b < x1 ? b : x1;
            if (s < e && w->nspan < SPANS) {
                Span *p = &w->span[w->nspan++];
                p->y = (int16_t)y, p->x0 = (int16_t)s, p->x1 = (int16_t)e;
                p->u = u + (s - x0) * du, p->v = v + (s - x0) * dv, p->du = du, p->dv = dv;
            }
        }
        return;
    }
    for (first = 0; first < nf; first++) {      /* (most pieces are out of its way, and stay) */
        int a = fr[2 * first], b = fr[2 * first + 1];
        if ((a > x0 ? a : x0) < (b < x1 ? b : x1))
            break;
    }
    if (first == nf)
        return;
    n = first;
    for (i = first; i < nf; i++) {
        int a = fr[2 * i], b = fr[2 * i + 1];
        int s = a > x0 ? a : x0, e = b < x1 ? b : x1;
        if (s < e) {
            span(y, s, e, u + (s - x0) * du, v + (s - x0) * dv, du, dv, kind, tex, 0);
            if (n + 2 <= FREE_MAX) {
                if (a < s)
                    out[2 * n] = (int16_t)a, out[2 * n + 1] = (int16_t)s, n++;
                if (e < b)
                    out[2 * n] = (int16_t)e, out[2 * n + 1] = (int16_t)b, n++;
            }
            continue;
        }
        if (n < FREE_MAX)
            out[2 * n] = (int16_t)a, out[2 * n + 1] = (int16_t)b, n++;
    }
    if (n > first)
        memcpy(fr + 2 * first, out + 2 * first, sizeof(int16_t) * 2 * (size_t)(n - first));
    g_row[y].n = n;
}

typedef struct {
    int sx, sy, u, v;
} Corner;

/* 0x49dde0: an affine triangle, 13-bit fixed point, one du/dx and dv/dx */
static void triangle(Drawer *w, Corner c0, Corner c1, Corner c2, int kind, TEX tex,
                     uint16_t colour)
{
    int x0, y0, u0, v0, x1, y1, u1, v1, x2, y2, u2, v2, A, B, D, du, dv, r, nrow = 0, deferred;
    int first = w->nspan, step = w->mask + 1;
    struct { int y, cnt, xl, sl, xr, sr, ul, dul, vl, dvl; } rows[2];
    while (c1.sy < c0.sy || c2.sy < c0.sy) {            /* the top corner first */
        Corner t = c0;
        c0 = c1, c1 = c2, c2 = t;
    }
    x0 = c0.sx, y0 = c0.sy, u0 = c0.u, v0 = c0.v;
    x1 = c1.sx, y1 = c1.sy, u1 = c1.u, v1 = c1.v;
    x2 = c2.sx, y2 = c2.sy, u2 = c2.u, v2 = c2.v;
    A = y1 - y2, B = y0 - y2;
    D = (x0 - x2) * A - (x1 - x2) * B;
    if (D < 1)
        return;
    du = ftol((double)(u0 - u2) / D * A - (double)(u1 - u2) / D * B);
    dv = ftol((double)(v0 - v2) / D * A - (double)(v1 - v2) / D * B);
    if (y2 < y1) {                                      /* the bend on corner 2's side */
        int sr, h;
        if (!(y0 < y1 && y1 >= 0))
            return;
        sr = cdiv((x1 - x0) * 0x2000, y1 - y0);
        h = y2 - y0;
        if (h >= 1 && y2 >= 0) {
            rows[nrow].y = y0, rows[nrow].cnt = h;
            rows[nrow].xl = x0 * 0x2000, rows[nrow].sl = cdiv((x2 - x0) * 0x2000, h);
            rows[nrow].xr = x0 * 0x2000, rows[nrow].sr = sr;
            rows[nrow].ul = u0, rows[nrow].dul = cdiv(u2 - u0, h);
            rows[nrow].vl = v0, rows[nrow].dvl = cdiv(v2 - v0, h);
            nrow++;
        }
        h = y1 - y2;
        if (h >= 1) {
            rows[nrow].y = y2, rows[nrow].cnt = h;
            rows[nrow].xl = x2 * 0x2000, rows[nrow].sl = cdiv((x1 - x2) * 0x2000, h);
            rows[nrow].xr = x0 * 0x2000 + (y2 - y0) * sr, rows[nrow].sr = sr;
            rows[nrow].ul = u2, rows[nrow].dul = cdiv(u1 - u2, h);
            rows[nrow].vl = v2, rows[nrow].dvl = cdiv(v1 - v2, h);
            nrow++;
        }
    } else {
        int sl, dul, dvl, h, h1, h2;
        if (!(y0 < y2 && y2 >= 0))
            return;
        h = y2 - y0;
        sl = cdiv((x2 - x0) * 0x2000, h);
        dul = cdiv(u2 - u0, h), dvl = cdiv(v2 - v0, h);
        h1 = y1 - y0;
        if (h1 >= 1 && y1 >= 0) {
            rows[nrow].y = y0, rows[nrow].cnt = h1;
            rows[nrow].xl = x0 * 0x2000, rows[nrow].sl = sl;
            rows[nrow].xr = x0 * 0x2000, rows[nrow].sr = cdiv((x1 - x0) * 0x2000, h1);
            rows[nrow].ul = u0, rows[nrow].dul = dul;
            rows[nrow].vl = v0, rows[nrow].dvl = dvl;
            nrow++;
        }
        h2 = y2 - y1;
        if (h2 >= 1) {
            rows[nrow].y = y1, rows[nrow].cnt = h2;
            rows[nrow].xl = x0 * 0x2000 + (y1 - y0) * sl, rows[nrow].sl = sl;
            rows[nrow].xr = x1 * 0x2000, rows[nrow].sr = cdiv((x2 - x1) * 0x2000, h2);
            rows[nrow].ul = u0 + (y1 - y0) * dul, rows[nrow].dul = dul;
            rows[nrow].vl = v0 + (y1 - y0) * dvl, rows[nrow].dvl = dvl;
            nrow++;
        }
    }
    deferred = see_through(kind);
    for (r = 0; r < nrow; r++) {
        int y = rows[r].y, cnt = rows[r].cnt, xl = rows[r].xl, xr = rows[r].xr;
        int ul = rows[r].ul, vl = rows[r].vl, sl = rows[r].sl * step, sr = rows[r].sr * step;
        int dul = rows[r].dul * step, dvl = rows[r].dvl * step, skip;
        if (y < 0) {                                    /* rows above the screen skipped */
            xl -= y * rows[r].sl, xr -= y * rows[r].sr;
            ul -= y * rows[r].dul, vl -= y * rows[r].dvl;
            cnt += y, y = 0;
        }
        if (cnt > H - y)
            cnt = H - y;
        skip = (w->val - y) & w->mask;                  /* on to this one's first row */
        xl += skip * rows[r].sl, xr += skip * rows[r].sr;
        ul += skip * rows[r].dul, vl += skip * rows[r].dvl;
        cnt -= skip, y += skip;
        for (; cnt > 0; cnt -= step, y += step) {
            if (g_row[y].n)
                row(w, y, xl >> 13, xr >> 13, ul, vl, du, dv, kind, tex, deferred);
            xl += sl, xr += sr, ul += dul, vl += dvl;
        }
    }
    if (deferred)
        batch_end(w, first, kind, tex, colour, 0);
}

/* 0x49d8f0 + 0x49e510: where the edge from B (behind) to F (in front) meets z = near */
static Corner clip(const Point *b, int ub, int vb, const Point *f, int uf, int vf)
{
    Corner c;
    float x = (g_near - b->z) * ((f->x - b->x) / (f->z - b->z)) + b->x;
    float y = (g_near - b->z) * ((f->y - b->y) / (f->z - b->z)) + b->y;
    int dz = (int)(f->z - b->z), t = (int)(g_near - b->z);
    c.sx = (int)CX + (int)(x / g_near);
    c.sy = (int)CY - (int)(y / g_near);
    c.u = ub + t * (dz ? cdiv(uf - ub, dz) : uf - ub);
    c.v = vb + t * (dz ? cdiv(vf - vb, dz) : vf - vb);
    return c;
}

/* 0x4a2f10: [xl, xr) into the row's free pieces, filled where free. A reversed span
   (xl > xr) fills nothing but splits a piece as the game's list does. */
static void flat_row(int y, int xl, int xr, uint16_t c)
{
    int16_t out[2 * FREE_MAX];
    uint16_t *o = g_dscreen + y * g_pitch;
    int i = 0, n = 0, nf = g_row[y].n, x, top = -y * g_pitch;
#define KEEP(a, b) do { if (n < FREE_MAX) out[2 * n] = (int16_t)(a), out[2 * n + 1] = (int16_t)(b), n++; } while (0)
#define FILL(from, to) for (x = (from) < top ? top : (from); x < (to); x++) o[x] = c    /* (see span6) */
    while (i < nf) {
        int a = g_row[y].p[2 * i], b = g_row[y].p[2 * i + 1];
        if (xl > a) {
            if (xl > b) {
                KEEP(a, b);
                i++;
                continue;
            }
            if (xr <= b) {
                FILL(xl, xr);
                KEEP(a, xl);
                if (xr < b)
                    KEEP(xr, b);
                i++;
                break;
            }
            FILL(xl, b);
            KEEP(a, xl);
            i++;
            continue;
        }
        if (xr < a)
            break;
        if (xr < b) {
            FILL(a, xr);
            KEEP(xr, b);
            i++;
            break;
        }
        FILL(a, b);                          /* the piece used up */
        i++;
        if (xr == b)
            break;
    }
    for (; i < nf; i++)
        KEEP(g_row[y].p[2 * i], g_row[y].p[2 * i + 1]);
    memcpy(g_row[y].p, out, sizeof(int16_t) * 2 * n);
    g_row[y].n = n;
#undef FILL
}

/* 0x4a34f0: a triangle of one colour, 13-bit fixed-point edges, no area test */
static void flat_triangle(const Drawer *w, Corner c0, Corner c1, Corner c2, uint16_t c)
{
    int x0, y0, x1, y1, x2, y2, r, nrow = 0, step = w->mask + 1;
    struct { int y, cnt, xl, sl, xr, sr; } rows[2];
    while (c1.sy < c0.sy || c2.sy < c0.sy) {
        Corner t = c0;
        c0 = c1, c1 = c2, c2 = t;
    }
    x0 = c0.sx, y0 = c0.sy, x1 = c1.sx, y1 = c1.sy, x2 = c2.sx, y2 = c2.sy;
    if (y2 < y1) {                                      /* the bend on the left */
        int sr, h;
        if (!(y0 < y1 && y1 >= 0))
            return;
        sr = cdiv((x1 - x0) * 0x2000, y1 - y0);
        h = y2 - y0;
        if (h >= 1 && y2 >= 0) {
            rows[nrow].y = y0, rows[nrow].cnt = h;
            rows[nrow].xl = x0 * 0x2000, rows[nrow].sl = cdiv((x2 - x0) * 0x2000, h);
            rows[nrow].xr = x0 * 0x2000, rows[nrow].sr = sr;
            nrow++;
        }
        h = y1 - y2;
        if (h >= 1) {
            rows[nrow].y = y2, rows[nrow].cnt = h;
            rows[nrow].xl = x2 * 0x2000, rows[nrow].sl = cdiv((x1 - x2) * 0x2000, h);
            rows[nrow].xr = x0 * 0x2000 + (y2 - y0 > 0 ? y2 - y0 : 0) * sr, rows[nrow].sr = sr;
            nrow++;
        }
    } else {                                            /* the bend on the right */
        int sl, h;
        if (!(y0 < y2 && y2 >= 0))
            return;
        sl = cdiv((x2 - x0) * 0x2000, y2 - y0);
        h = y1 - y0;
        if (h >= 1 && y1 >= 0) {
            rows[nrow].y = y0, rows[nrow].cnt = h;
            rows[nrow].xl = x0 * 0x2000, rows[nrow].sl = sl;
            rows[nrow].xr = x0 * 0x2000, rows[nrow].sr = cdiv((x1 - x0) * 0x2000, h);
            nrow++;
        }
        h = y2 - y1;
        if (h >= 1) {
            rows[nrow].y = y1, rows[nrow].cnt = h;
            rows[nrow].xl = x0 * 0x2000 + (y1 - y0 > 0 ? y1 - y0 : 0) * sl, rows[nrow].sl = sl;
            rows[nrow].xr = x1 * 0x2000, rows[nrow].sr = cdiv((x2 - x1) * 0x2000, h);
            nrow++;
        }
    }
    for (r = 0; r < nrow; r++) {
        int y = rows[r].y, cnt = rows[r].cnt, xl = rows[r].xl, xr = rows[r].xr;
        int sl = rows[r].sl * step, sr = rows[r].sr * step, skip;
        if (y < 0) {
            xl -= y * rows[r].sl, xr -= y * rows[r].sr;
            cnt += y, y = 0;
        }
        if (cnt > H - y)
            cnt = H - y;
        skip = (w->val - y) & w->mask;
        xl += skip * rows[r].sl, xr += skip * rows[r].sr;
        cnt -= skip, y += skip;
        for (; cnt > 0; cnt -= step, y += step) {
            if (g_row[y].n)
                flat_row(y, xl >> 13, xr >> 13, c);
            xl += sl, xr += sr;
        }
    }
}

/* 0x4a3950: a fan of one colour, cut at the
   near plane as 0x49e510 cuts */
static void flat_face(const Drawer *w, const Face *f, int base, int colour)
{
    uint16_t c = (uint16_t)colour;
    int k;
    for (k = 1; k < f->n - 1; k++) {
        int j[3] = { 0, k, k + 1 }, behind[3], nb = 0, i;
        const Point *p[3];
        Corner q[3];
        for (i = 0; i < 3; i++) {
            p[i] = &g_dpt[base + f->idx[j[i]]];
            q[i].sx = p[i]->sx, q[i].sy = p[i]->sy, q[i].u = q[i].v = 0;
            behind[i] = p[i]->z < g_near;
            nb += behind[i];
        }
        if (nb == 0)
            flat_triangle(w, q[0], q[1], q[2], c);
        else if (nb == 2) {
            int r = !behind[0] ? 0 : !behind[1] ? 1 : 2, a = (r + 1) % 3, b = (r + 2) % 3;
            flat_triangle(w, q[r], clip(p[a], 0, 0, p[r], 0, 0), clip(p[b], 0, 0, p[r], 0, 0), c);
        } else if (nb == 1) {
            int r = behind[0] ? 0 : behind[1] ? 1 : 2, a = (r + 1) % 3, b = (r + 2) % 3;
            Corner p1 = clip(p[r], 0, 0, p[a], 0, 0), p2 = clip(p[r], 0, 0, p[b], 0, 0);
            flat_triangle(w, q[a], q[b], p1, c);
            flat_triangle(w, q[b], p2, p1, c);
        }
    }
}

/* ---------------------------------------------------------------- sub 6: perspective */

typedef struct {
    int sx, sy;
    float rz, uz, vz;           /* 1/z, u/z, v/z (vertex +0x18 and the polygon's) */
} Corner6;

typedef struct {                /* what one triangle's rows share */
    int kind;
    TEX tex;
    uint16_t colour;
    float g[3], g16[3];         /* d(u/z, v/z, 1/z)/dx and 16 times it */
} Tri6;

/* 0x4a5c40 and its twins: (u, v) = 8192 (u/z, v/z) / (1/z) exactly every 16 pixels,
   stepped linearly in between; u and v run on from block to block */
static void span6(int y, int x0, int x1, const float e[3], const float g16[3], int kind,
                  TEX tex, uint16_t colour)
{
    TEX_LOCALS(tex);
    uint16_t *o = g_dscreen + y * g_pitch + x0;
    int n = x1 - x0, k, room = (g_scr_h - y) * g_pitch - x0, skip = -y * g_pitch - x0;
    float uz = e[0], vz = e[1], rz = e[2];
    double z, rze;
    uint32_t u, v, du, dv;
    /* 0x4a04d0 keeps a piece's end in 16 bits: an edge far off to the left comes back as an end
       up to 32767 to the right, and the game's painters run on over the rows below and past
       their surface. The free lists can hold a piece that starts left of the row, too (a
       reversed row splits one: 0x4a0260, 0x4a2f10), which paints the row above. Those rows are
       painted the same here, but nothing outside the screen. */
    if (n > room)
        n = room;
    if (n <= 0 || kind == 28 || kind == 50)             /* 0x4a5090, 0x4a50b0: ret */
        return;
    z = 8192.0 / rz;                                    /* 0x4f1a24 */
    u = (uint32_t)ftol(uz * z);
    v = (uint32_t)ftol(vz * z);
    while (n) {
        if (n >= 16) {
            uz = g16[0] + uz;   /* (a float sum through a double is the same float: 53 >= 2 x 24 + 2) */
            vz = vz + g16[1];
            rze = (double)g16[2] + rz;
            rz = (float)rze;
            z = 8192.0 / rze;
            du = (uint32_t)((int)((uint32_t)ftol(uz * z) - u) >> 4);
            dv = (uint32_t)((int)((uint32_t)ftol(vz * z) - v) >> 4);
            k = 16;
        } else {
            double m = 0.0625 * n;                      /* 0x4d1fb8 */
            k = n;
            uz = (float)(uz + g16[0] * m);
            vz = (float)(vz + g16[1] * m);
            rze = rz + g16[2] * m;
            rz = (float)rze;
            z = 8192.0 / rze;
            du = (uint32_t)ftol((uz * z - (int)u) * (1.0 / k));   /* 0x4d1fc0 */
            dv = (uint32_t)ftol((vz * z - (int)v) * (1.0 / k));
        }
        n -= k;
        for (; k > 0 && skip > 0; k--, skip--, o++, u += du, v += dv)
            ;
#define TEXLOOP_B(S, what) \
        for (; k > 0; k--, o++, u += du, v += dv) { \
            unsigned c = S(((v >> 13) * 256 + (u >> 13)) & 0xffff); \
            what; \
        }
#ifdef OMR_ESP32
#define TEXELS(what) \
        if (t_rgb) { TEXLOOP_B(S16, what) } else { TEXLOOP_B(S8, what) } \
        break
#else
#define TEXELS(what) TEXLOOP_B(S16, what) break
#endif
        switch (kind) {
        case 31: TEXELS(*o = (uint16_t)c);                          /* 0x4a5c40 */
        case 32: TEXELS(*o = (uint16_t)((c >> 1) & HALF));          /* 0x4a5e90 */
        case 53: TEXELS(if (c) *o = (uint16_t)c);                   /* 0x4a53b0 */
        case 54: TEXELS(if (c) *o = (uint16_t)((c >> 1) & HALF));   /* 0x4a5610 */
        case 51: TEXELS(if (c) *o = colour);                        /* 0x4a5150 */
        case 62: TEXELS(if (c) *o = (uint16_t)(((*o >> 1) & HALF) + ((c >> 1) & HALF)));   /* 0x4a5890 */
        default: o += k, u += (uint32_t)k * du, v += (uint32_t)k * dv;
        }
#undef TEXELS
#undef TEXLOOP_B
    }
}

/* 0x4a0260: the opaque edge, the row's free pieces walked as the game walks them */
static void row6(int y, int xl, int xr, const float e[3], const Tri6 *t)
{
    int16_t out[2 * FREE_MAX];
    int i = 0, n, first, nf = g_row[y].n;
    const int16_t *fr = g_row[y].p;
    float es[3];
    /* (most pieces are out of its way, and stay) */
    while (i < nf && fr[2 * i] < xl && fr[2 * i + 1] < xl)
        i++;
    if (i == nf || (fr[2 * i] >= xl && fr[2 * i] > xr))
        return;
    n = first = i;
#define KEEP(a, b) do { if (n < FREE_MAX) out[2 * n] = (int16_t)(a), out[2 * n + 1] = (int16_t)(b), n++; } while (0)
    while (i < nf) {
        int a = fr[2 * i], b = fr[2 * i + 1], d;
        if (a < xl) {
            if (b < xl) {
                KEEP(a, b);
                i++;
                continue;
            }
            if (b < xr) {
                span6(y, xl, b, e, t->g16, t->kind, t->tex, t->colour);
                KEEP(a, xl);
                i++;
                continue;
            }
            span6(y, xl, xr, e, t->g16, t->kind, t->tex, t->colour);
            KEEP(a, xl);
            if (b > xr)
                KEEP(xr, b);
            i++;
            break;
        }
        if (a > xr)
            break;
        d = a - xl;
        es[0] = (float)(e[0] + d * (double)t->g[0]);
        es[1] = (float)(e[1] + d * (double)t->g[1]);
        es[2] = (float)(e[2] + d * (double)t->g[2]);
        if (b > xr) {
            span6(y, a, xr, es, t->g16, t->kind, t->tex, t->colour);
            KEEP(xr, b);
            i++;
            break;
        }
        span6(y, a, b, es, t->g16, t->kind, t->tex, t->colour);    /* the piece used up */
        i++;
        if (b == xr)
            break;
    }
    for (; i < nf; i++)
        KEEP(fr[2 * i], fr[2 * i + 1]);
#undef KEEP
    if (n > first)
        memcpy(g_row[y].p + 2 * first, out + 2 * first, sizeof(int16_t) * 2 * (size_t)(n - first));
    g_row[y].n = n;
}

/* 0x4a04d0: the see-through edge, the free pieces under the row kept for the end */
static void row6s(Drawer *w, int y, int xl, int xr, const float e[3], const Tri6 *t)
{
    int i;
    if (w->nspan == SPANS) {
        w->room = 0;
        return;
    }
    for (i = 0; i < g_row[y].n; i++) {
        int a = g_row[y].p[2 * i], b = g_row[y].p[2 * i + 1], s;
        Span *p;
        if (a < xl) {
            if (b < xl)
                continue;
            s = xl;
            p = &w->span[w->nspan];
            memcpy(p->e, e, sizeof p->e);
        } else {
            if (xr < a)
                return;
            s = a;
            p = &w->span[w->nspan];
            p->e[0] = (float)(e[0] + (a - xl) * (double)t->g[0]);
            p->e[1] = (float)(e[1] + (a - xl) * (double)t->g[1]);
            p->e[2] = (float)(e[2] + (a - xl) * (double)t->g[2]);
        }
        w->nspan++;
        p->y = (int16_t)y, p->x0 = (int16_t)s, p->x1 = (int16_t)(xr <= b ? xr : b);
        if (xr <= b || w->nspan == SPANS)
            return;
    }
}

/* the row loop of 0x4a0700 (0x4a0ce9): rows of one part; a see-through part's pieces
   make one batch */
static void rows6(Drawer *w, int y, int cnt, int *xl, int sl, int *xr, int sr, float e[3],
                  const float de[3], const Tri6 *t)
{
    int see = see_through(t->kind), first = w->nspan, l = *xl, r = *xr, mask = w->mask, val = w->val;
    float now[3], d0 = de[0], d1 = de[1], d2 = de[2];
    if (see && !w->room)
        return;
    now[0] = e[0], now[1] = e[1], now[2] = e[2];
    for (; cnt > 0; cnt--, y++) {
        if (!((y ^ val) & mask) && g_row[y].n) {
            if (see)
                row6s(w, y, l >> 13, r >> 13, now, t);
            else
                row6(y, l >> 13, r >> 13, now, t);
        }
        now[0] += d0;                   /* (the same float as through a double: span6) */
        now[1] += d1;
        now[2] += d2;
        l += sl;
        r += sr;
    }
    e[0] = now[0], e[1] = now[1], e[2] = now[2];
    *xl = l, *xr = r;
    if (see)
        batch_end(w, first, t->kind, t->tex, t->colour, t->g16);
}

static void steps6(float de[3], const float *a, const float *b, int h)
{
    int k;
    for (k = 0; k < 3; k++)
        de[k] = (float)(((double)b[k] - a[k]) / h);
}

/* rows above the screen skipped: the values at row 0 */
static void back6(float e[3], const float *from, const float de[3], int y)
{
    int k;
    for (k = 0; k < 3; k++)
        e[k] = (float)(from[k] - (double)de[k] * y);
}

static int bottom6(int cnt, int ylast)  /* rows past the last (0x625784) dropped */
{
    return H < ylast ? cnt + (H - ylast) : cnt;
}

/* 0x4a0700: a triangle with u/z, v/z and 1/z stepped along its left edge and across */
static void tri6(Drawer *w, Corner6 c0, Corner6 c1, Corner6 c2, Tri6 *t)
{
    int x0, y0, x1, y1, x2, y2, A, C, sl = 0, sr, xl = 0, xr, h, y, k;
    float e0[3], e1[3], e2[3], e[3], de[3];
    long long D;
    while (c1.sy < c0.sy || c0.sy > c2.sy) {
        Corner6 s = c0;
        c0 = c1, c1 = c2, c2 = s;
    }
    x0 = c0.sx, y0 = c0.sy, x1 = c1.sx, y1 = c1.sy, x2 = c2.sx, y2 = c2.sy;
    e0[0] = c0.uz, e0[1] = c0.vz, e0[2] = c0.rz;
    e1[0] = c1.uz, e1[1] = c1.vz, e1[2] = c1.rz;
    e2[0] = c2.uz, e2[1] = c2.vz, e2[2] = c2.rz;
    D = (long long)(x0 - x2) * (y1 - y2) - (long long)(x1 - x2) * (y0 - y2);
    if (D < 1)
        return;
    A = y1 - y2, C = y2 - y0;
    for (k = 0; k < 3; k++) {       /* 0x655d1c, 0x655d18, 0x65e1c0; 16x: 0x68b020 ... */
        t->g[k] = (float)((((double)e0[k] - e2[k]) * A + ((double)e1[k] - e2[k]) * C) / D);
        t->g16[k] = 16.0f * t->g[k];
    }
    if (y1 <= y2) {                                     /* the long edge 0-2 on the left */
        if (y0 >= y2 || y2 < 0)
            return;
        sl = cdiv(wrap((long long)(x2 - x0) << 13), C);
        steps6(de, e0, e2, C);
        xl = wrap((long long)x0 << 13);
        memcpy(e, e0, sizeof e);
        h = y1 - y0;
        if (h > 0 && y1 >= 0) {
            xr = wrap((long long)x0 << 13);
            sr = cdiv(wrap((long long)(x1 - x0) << 13), h);
            y = y0;
            if (y0 < 0) {
                xl = wrap(xl - (long long)y0 * sl);
                back6(e, e0, de, y0);
                xr = wrap(xr - (long long)y0 * sr);
                h += y0, y = 0;
            }
            rows6(w, y, bottom6(h, y1), &xl, sl, &xr, sr, e, de, t);
        }
        h = y2 - y1;
        if (h <= 0)
            return;
        sr = cdiv(wrap((long long)(x2 - x1) << 13), h);
        xr = wrap((long long)x1 << 13);
        y = y1;
        if (y1 < 0) {
            xl = wrap(((long long)x0 << 13) - (long long)y0 * sl);
            back6(e, e, de, y0);
            xr = wrap(xr - (long long)y1 * sr);
            h += y1, y = 0;
        }
        rows6(w, y, bottom6(h, y2), &xl, sl, &xr, sr, e, de, t);
    } else {                                            /* the long edge 0-1 on the right */
        if (y1 <= y0 || y1 < 0)
            return;
        sr = cdiv(wrap((long long)(x1 - x0) << 13), y1 - y0);
        xr = wrap((long long)x0 << 13);
        h = C;
        if (h > 0 && y2 >= 0) {
            steps6(de, e0, e2, C);
            sl = cdiv(wrap((long long)(x2 - x0) << 13), C);
            xl = wrap((long long)x0 << 13);
            memcpy(e, e0, sizeof e);
            y = y0;
            if (y0 < 0) {
                xl = wrap(xl - (long long)y0 * sl);
                back6(e, e0, de, y0);
                xr = wrap(xr - (long long)y0 * sr);
                h += y0, y = 0;
            }
            rows6(w, y, bottom6(h, y2), &xl, sl, &xr, sr, e, de, t);
        }
        h = y1 - y2;
        if (h <= 0)
            return;
        sl = cdiv(wrap((long long)(x1 - x2) << 13), h);
        steps6(de, e2, e1, h);
        xl = wrap((long long)x2 << 13);
        memcpy(e, e2, sizeof e);
        y = y2;
        if (y2 < 0) {
            xl = wrap(xl - (long long)y2 * sl);
            back6(e, e2, de, y2);
            xr = wrap(((long long)x0 << 13) - (long long)y0 * sr);
            h += y2, y = 0;
        }
        rows6(w, y, bottom6(h, y1), &xl, sl, &xr, sr, e, de, t);
    }
}

/* 0x49d8f0 for the point, and the texture point in float: u = (uF - uB) / (zF - zB)
   (near - zB) + uB, stored over near. spill: the values 0x4a1535 keeps in float
   temporaries for the first point when one corner is in front */
static Corner6 clip6(const Point *b, const Corner6 *cb, const Point *f, const Corner6 *cf,
                     int spill)
{
    Corner c = clip(b, 0, 0, f, 0, 0);
    Corner6 o;
    double dz = (double)f->z - b->z, t = (double)g_near - b->z;
    double ub = (double)cb->uz / cb->rz, vb = (double)cb->vz / cb->rz;
    double uf = (double)cf->uz / cf->rz, vf = (double)cf->vz / cf->rz;
    if (spill)
        ub = (float)ub, vb = (float)vb, dz = (float)dz, t = (float)t;
    o.sx = c.sx, o.sy = c.sy;
    o.rz = (float)(1.0 / g_near);
    o.uz = (float)(((uf - ub) / dz * t + ub) / g_near);
    o.vz = (float)(((vf - vb) / dz * t + vb) / g_near);
    return o;
}

/* 0x4a0f90: a fan, each triangle cut at the near plane */
static void face6(Drawer *w, const Face *f, int kind, int base, TEX tex)
{
    Tri6 t;
    int k;
    t.kind = kind, t.tex = tex, t.colour = (uint16_t)(f->colour & 0xffdf);
    for (k = 1; k < f->n - 1; k++) {
        int j[3] = { 0, k, k + 1 }, behind[3], nb = 0, i;
        const Point *p[3];
        Corner6 c[3];
        for (i = 0; i < 3; i++) {
            p[i] = &g_dpt[base + f->idx[j[i]]];
            c[i].sx = p[i]->sx, c[i].sy = p[i]->sy;
            c[i].rz = (float)(1.0 / p[i]->z);
            c[i].uz = (float)((double)f->ut[j[i]] * c[i].rz);
            c[i].vz = (float)((double)f->vt[j[i]] * c[i].rz);
            behind[i] = p[i]->z < g_near;
            nb += behind[i];
        }
        if (nb == 0)
            tri6(w, c[0], c[1], c[2], &t);
        else if (nb == 1) {                             /* two in front */
            int r = behind[0] ? 0 : behind[1] ? 1 : 2, a = (r + 1) % 3, b = (r + 2) % 3;
            Corner6 p1 = clip6(p[r], &c[r], p[a], &c[a], 0);
            Corner6 p2 = clip6(p[r], &c[r], p[b], &c[b], 0);
            tri6(w, c[a], c[b], p1, &t);
            tri6(w, c[b], p2, p1, &t);
        } else if (nb == 2) {                           /* one in front */
            int r = !behind[0] ? 0 : !behind[1] ? 1 : 2, a = (r + 1) % 3, b = (r + 2) % 3;
            tri6(w, c[r], clip6(p[a], &c[a], p[r], &c[r], 1), clip6(p[b], &c[b], p[r], &c[r], 0),
                 &t);
        }
    }
}

/* ---------------------------------------------------------------- 0x4aef20 */

static void face(Drawer *w, const Entry *e)
{
    const Face *f = e->f;
    TEX tex;
    uint16_t colour = (uint16_t)(f->colour & 0xffdf);
#ifdef R3D_PIPE
    int k, kind = e->kind, base = e->base;
#else
    int k, kind = f->drawn, base = e->base;
#endif
    if (e->skin >= 0) {         /* 0x4aef20 by face +0x3a, with the face's palette */
        const uint16_t *pal = r3d_page_palette(f->page, e->skin);
        colour = f->colour < 256 ? pal[f->colour] : (uint16_t)f->colour;
        if (e->dark)                                    /* the light < 0x3e000: half */
            colour = (uint16_t)((colour >> 1) & HALF);
        if (kind == 0) {                                /* 0x4a3cf0 */
            flat_face(w, f, base, colour);
            return;
        }
        if (kind != 1 && kind != 2 && kind != 3 && kind != 4 && kind != 9 && kind != 42)
            return;                                     /* (0x4af100: the others, as 8) */
        /* 1: 0x4a7b70, palette[texel] opaque; 2: 0x4ab070, the same put off till last and
           skipping a palette colour of 0, as the kind 0x26; dark, each halved (0x4a8c70,
           0x4ac1f0); 42: 0x4a9df0, the face's colour where the palette's is not 0, as the
           kind 0x25. No model makes 3 (0x4a6a50), 4 (0x4ad410, batch kind 0x13) or 9 (0x4ae730,
           batch kind 0x14), which take no light: NOTES §4.20 "Paletted path" */
        tex = skin_texels(f->page, e->skin);
        if (kind == 4 || kind == 9)
            kind = kind == 4 ? 0x13 : 0x14;
        else if (kind != 3)
            kind = kind == 42 ? 0x25 : kind == 1 ? (e->dark ? 0x15 : 1) : (e->dark ? 0x27 : 0x26);
    } else {
        if (kind == 0) {
            flat_face(w, f, base, f->colour & 0xffdf);    /* (the 5:5:5 form: green loses its low bit) */
            return;
        }
        if (kind == 5)                                  /* 0x4a4bc0: ret */
            return;
        if (kind == 1)                                  /* 0x4a4bf0: ret */
            kind = 3;
        if (kind == 12) {       /* 0x4a4820: the colour halved, as it adds to half the screen */
            unsigned c = ((f->colour >> 10) & 31) << 11 | ((f->colour >> 5) & 31) << 6
                         | (f->colour & 31);            /* 0x402020 */
            colour = (uint16_t)((((c & 0xffc3) >> 1 | (c & 0x1e)) >> 1) & HALF);
        } else if (!sub6(kind) && !no_paint(kind) && kind != 0x10 && kind != 0x15 && kind != 0x25
                   && kind != 0x26 && kind != 0x27 && kind != 0x2c && kind != 68 && kind != 69)
            return;                                     /* (no row in 0x4f1a70, as 0xd) */
#ifdef OMR_ESP32
        tex = f->page < R3D_PAGES
              && (g_tex[f->page][0].rgb || g_tex[f->page][0].idx) ? &g_tex[f->page][0] : 0;
#else
        tex = f->page < R3D_PAGES ? g_texpage[f->page] : 0;
#endif
    }
    if (!tex && kind != 12 && !no_paint(kind))
        return;
    if (sub6(kind)) {
        face6(w, f, kind, base, tex);
        return;
    }
    if (see_through(kind) && !w->room)
        return;
    for (k = 1; k < f->n - 1; k++) {
        int j[3] = { 0, k, k + 1 }, behind[3], nb = 0, i;
        const Point *p[3];
        Corner c[3];
        for (i = 0; i < 3; i++) {
            p[i] = &g_dpt[base + f->idx[j[i]]];
            c[i].sx = p[i]->sx, c[i].sy = p[i]->sy;
            c[i].u = FACE_U(f, j[i]), c[i].v = FACE_V(f, j[i]);
            behind[i] = p[i]->z < g_near;
            nb += behind[i];
        }
        if (nb == 0)
            triangle(w, c[0], c[1], c[2], kind, tex, colour);
        else if (nb == 2) {                             /* one corner in front */
            int r = !behind[0] ? 0 : !behind[1] ? 1 : 2, a = (r + 1) % 3, b = (r + 2) % 3;
            triangle(w, c[r], clip(p[a], c[a].u, c[a].v, p[r], c[r].u, c[r].v),
                     clip(p[b], c[b].u, c[b].v, p[r], c[r].u, c[r].v), kind, tex, colour);
        } else if (nb == 1) {                           /* two in front */
            int r = behind[0] ? 0 : behind[1] ? 1 : 2, a = (r + 1) % 3, b = (r + 2) % 3;
            Corner p1 = clip(p[r], c[r].u, c[r].v, p[a], c[a].u, c[a].v);
            Corner p2 = clip(p[r], c[r].u, c[r].v, p[b], c[b].u, c[b].v);
            triangle(w, c[a], c[b], p1, kind, tex, colour);
            triangle(w, c[b], p2, p1, kind, tex, colour);
        }
    }
}

/* the faces, nearest first, then the see-through pieces, last first: of the rows this one draws */
static void draw_rows(Drawer *w)
{
    int b, e, i, n = 0;
    for (b = g_dlo; b <= g_dhi; b++)
        for (e = g_dhead[b]; e >= 0; e = g_dentry[e].next) {
#ifdef __arm__
            if (g_dentry[e].next >= 0)                  /* (the next face asked for ahead) */
                __builtin_prefetch(g_dentry[g_dentry[e].next].f);
#endif
            if (n++ <= FACES_MAX)
                face(w, &g_dentry[e]);
        }
    w->batch[w->nbatch].first = w->nspan;
    for (b = w->nbatch - 1; b >= 0; b--) {              /* 0x4af130: the last first */
        const Batch *t = &w->batch[b];
        for (i = w->batch[b + 1].first - 1; i >= t->first; i--) {
            const Span *p = &w->span[i];
            if (sub6(t->kind))
                span6(p->y, p->x0, p->x1, p->e, t->g16, t->kind, t->tex, t->colour);
            else
                span(p->y, p->x0, p->x1, p->u, p->v, p->du, p->dv, t->kind, t->tex,
                     t->colour);
        }
    }
}

#if defined(R3D_TWO) && defined(OMR_ESP32)
/* The ESP32's second core draws every other row (NOTES §4.26), woken by a semaphore each
   picture; the same rows and pixels as the MiSTer's below. */
static SemaphoreHandle_t g_go_sem, g_done_sem;
static Drawer *volatile g_job;          /* what core 1 draws next: half the rows, or all */
static int g_async;                     /* a whole picture being drawn by core 1 alone */
extern unsigned long long g_esp_r1_us;

static void second_core(void *arg)
{
    (void)arg;
    for (;;) {
        unsigned long long t0;
        xSemaphoreTake(g_go_sem, portMAX_DELAY);
        t0 = esp32_us();
        draw_rows(g_job);
        g_esp_r1_us += esp32_us() - t0;
        xSemaphoreGive(g_done_sem);
    }
}

void r3d_two_cpus(int (*pin)(void))
{
    (void)pin;
    if (g_two)
        return;
    g_go_sem = xSemaphoreCreateBinary();
    g_done_sem = xSemaphoreCreateBinary();
    /* above the music's reading and the input, below the sound's mixing */
    if (g_go_sem && g_done_sem
        && xTaskCreatePinnedToCore(second_core, "r3d2", 12288, NULL, 8, NULL, 1) == pdPASS)
        g_two = 1;
}

unsigned long long g_esp_r0_us, g_esp_r1_us, g_esp_rwait_us;   /* the readout: each core's rows, the wait */

static void draw_both(int rows)
{
    unsigned long long t0, t1;
    g_draw[0].mask = g_draw[1].mask = 2 * rows - 1;
    g_draw[0].val = 0;
    g_draw[1].val = rows;
    g_job = &g_draw[1];
    xSemaphoreGive(g_go_sem);
    t0 = esp32_us();
    draw_rows(&g_draw[0]);
    t1 = esp32_us();
    g_esp_r0_us += t1 - t0;
    xSemaphoreTake(g_done_sem, portMAX_DELAY);
    g_esp_rwait_us += esp32_us() - t1;
}

/* a drawing's start: its drawers' pieces and the rows' free lists afresh */
static void draw_start(void)
{
    unsigned i;
    int y;
    for (i = 0; i < sizeof g_draw / sizeof g_draw[0]; i++) {
        g_draw[i].nspan = 0;
        g_draw[i].nbatch = 0;
        g_draw[i].room = 1;
    }
    for (y = 0; y < H; y++) {
        g_row[y].p[0] = 0;
        g_row[y].p[1] = (int16_t)W;
        g_row[y].n = 1;
    }
}

/* what the drawing reads: the lists (copy 0), or a copy of what they hold (1); each entry's kind
   as its face was last queued (the copies of a model share it, as the game's do) */
static Point *g_cpt;
static Entry *g_centry;
static int g_ncpt, g_ncentry;
static int g_chead[BUCKETS];

static int use_lists(int copy)
{
    int i;
    for (i = 0; i < g_nentry; i++)
        g_entry[i].kind = g_entry[i].f->drawn;
    if (!copy) {
        g_dpt = g_pt, g_dentry = g_entry, g_dhead = g_head;
    } else {
        if (g_npt > g_ncpt) {           /* (grown as needed: a race holds some 3000 points) */
            Point *n = realloc(g_cpt, sizeof *n * (size_t)(g_npt + 512));
            if (!n)
                return 0;
            g_cpt = n, g_ncpt = g_npt + 512;
        }
        if (g_nentry > g_ncentry) {
            Entry *n = realloc(g_centry, sizeof *n * (size_t)(g_nentry + 512));
            if (!n)
                return 0;
            g_centry = n, g_ncentry = g_nentry + 512;
        }
        memcpy(g_cpt, g_pt, sizeof *g_pt * (size_t)g_npt);
        memcpy(g_centry, g_entry, sizeof *g_entry * (size_t)g_nentry);
        if (g_bucket_lo <= g_bucket_hi)
            memcpy(g_chead + g_bucket_lo, g_head + g_bucket_lo,
                   sizeof *g_head * (size_t)(g_bucket_hi - g_bucket_lo + 1));
        g_dpt = g_cpt, g_dentry = g_centry, g_dhead = g_chead;
    }
    g_dlo = g_bucket_lo, g_dhi = g_bucket_hi;
    g_dscreen = g_screen;
    draw_start();
    return 1;
}

/* The race on the board (NOTES §4.26): core 1 draws the scene from a copy of the lists while core
   0 builds the next one; r3d_finish waits for it. */
void r3d_render_async(void)
{
    r3d_finish();
    if (!g_two || video_no_3d() || !use_lists(1)) {
        r3d_render();
        return;
    }
    g_draw[0].mask = 0;
    g_draw[0].val = 0;
    g_job = &g_draw[0];
    g_async = 1;
    xSemaphoreGive(g_go_sem);
    r3d_begin();                        /* (the lists free for the next scene) */
}

void r3d_finish(void)
{
    if (!g_async)
        return;
    xSemaphoreTake(g_done_sem, portMAX_DELAY);
    g_async = 0;
}

int r3d_band_split(void)
{
    return -1;                          /* (rows interleaved: bands measured no better, NOTES §4.26) */
}
#elif defined(R3D_TWO)
/* The second CPU (hybrid/CLAUDE.md: the helpers' CPU, Main_MiSTer's, which gives way to our
   priority) draws every other row. It sleeps between pictures. */
static uint32_t g_go, g_done;
static int g_second_ok;

static void futex(uint32_t *at, int op, uint32_t val)
{
    syscall(SYS_futex, at, op | FUTEX_PRIVATE_FLAG, val, NULL, NULL, 0);
}

static void *second_cpu(void *arg)
{
    uint32_t seen = 0;
    int (*pin)(void) = (int (*)(void))arg;
    __atomic_store_n(&g_second_ok, !pin || pin() ? 1 : -1, __ATOMIC_RELEASE);
    for (;;) {
        uint32_t go;
        while ((go = __atomic_load_n(&g_go, __ATOMIC_ACQUIRE)) == seen)
            futex(&g_go, FUTEX_WAIT, seen);
        seen = go;
        draw_rows(&g_draw[1]);
        __atomic_store_n(&g_done, seen, __ATOMIC_RELEASE);
        futex(&g_done, FUTEX_WAKE, 1);
    }
    return NULL;
}

void r3d_two_cpus(int (*pin)(void))
{
    pthread_t t;
    if (g_two || pthread_create(&t, NULL, second_cpu, (void *)pin))
        return;
    pthread_detach(t);
    while (!__atomic_load_n(&g_second_ok, __ATOMIC_ACQUIRE))
        sched_yield();
    g_two = g_second_ok > 0;
}

static void draw_both(int rows)
{
    uint32_t go = g_go + 1, d;
    int i;
    g_draw[0].mask = g_draw[1].mask = 2 * rows - 1;     /* (rows 1, or 2 at 640x240: its even ones) */
    g_draw[0].val = 0;
    g_draw[1].val = rows;
    __atomic_store_n(&g_go, go, __ATOMIC_RELEASE);
    futex(&g_go, FUTEX_WAKE, 1);
    draw_rows(&g_draw[0]);
    for (i = 0; i < 2000 && __atomic_load_n(&g_done, __ATOMIC_ACQUIRE) != go; i++)
        ;
    while ((d = __atomic_load_n(&g_done, __ATOMIC_ACQUIRE)) != go)
        futex(&g_done, FUTEX_WAIT, d);
}
#endif

void r3d_render(void)
{
#ifdef R3D_PIPE
    r3d_finish();                       /* (one being drawn in the background first) */
    use_lists(0);
#endif
    if (video_no_3d()) {                                /* 0x49ad90: another program in front */
        r3d_begin();
        return;
    }
#ifdef R3D_TWO
    if (g_two) {
        draw_both(g_half_rows ? 2 : 1);
        r3d_begin();
        return;
    }
#endif
    g_draw[0].mask = g_half_rows ? 1 : 0;
    g_draw[0].val = 0;
    draw_rows(&g_draw[0]);
    r3d_begin();
}
