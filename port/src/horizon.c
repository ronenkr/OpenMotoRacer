/* The horizon picture (build/notes/track_horizon.md): load 0x410350 / 0x477b30, the fill
   colours 0x4782e0, the draw 0x402960. A picture for each screen size (§4.13.3). */
#include "config.h"
#include "horizon.h"
#include "ftol.h"
#include "files.h"
#include "font.h"
#include "r3d.h"
#include "video.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fastfloat.h"      /* (last: the ESP32's single precision) */

#define K 2607.594587618272     /* 0x4d00d0: 16384 / 2pi */
#define PI100 314.1592653589793 /* 0x4d00d8 */
#define TWO_PI 6.283185307179586        /* 0x4d00e0 */
#define HALF_PI 1.5707963267948966      /* 0x4d00e8 */
#define FAR 50000.0             /* 0x4d00f0: the ground point the picture's edge sits on */

typedef struct {
    uint16_t *pix;              /* the picture, 16-bit with 6 bits of green */
    int pw;
    uint16_t above, below;      /* sprite A +0x46, +0x48 */
} Picture;

static Picture g_pics[3];       /* by the screen's mode: HorizLow, HorizMed, HorizHi */
static const int g_rects[3][2][4] = {
    { { 0, 0, 255, 63 }, { 0, 0, 255, 63 } },           /* 0x4eaa4c, both tiles */
    { { 0, 0, 203, 101 }, { 0, 102, 203, 203 } },       /* 0x4eaa44/48 */
    { { 0, 0, 255, 127 }, { 0, 128, 255, 255 } } };     /* 0x4eaa3c/40 */
static uint16_t *g_pix;         /* the screen's picture */
static int g_pw;
static const int (*g_rect)[4];

void horizon_free(void)
{
    int i;
    for (i = 0; i < 3; i++) {
        free(g_pics[i].pix);
        g_pics[i].pix = 0;
    }
    g_pix = 0;
}

/* 0x478300..: a pixel's channel bytes */
static void chans(unsigned p, unsigned c[3])
{
    c[0] = ((p & 0xf800) >> 11 << 3) & 0xff;
    c[1] = ((p & 0x7e0) >> 5 << 2) & 0xff;
    c[2] = ((p & 0x1f) << 3) & 0xff;
}

static uint16_t pack(const unsigned c[3])
{
    return (uint16_t)(((c[0] & 0xff) >> 3) << 11 | ((c[1] & 0xff) >> 2) << 5 | (c[2] & 0xff) >> 3);
}

/* 0x4782e0: above = the mean of row y1's first pixels from x1; below = row y2's last ones
   from x2 leftwards added onto that mean (the sums are not emptied), over their count */
static void fill_colours(Picture *o, const int r[4])
{
    unsigned acc[3] = { 0, 0, 0 }, c[3];
    int n = r[2] - r[0], i, k;
    if (behaviour_cd()) {       /* 0x465a02: the one pixel at each corner */
        o->above = o->pix[r[1] * o->pw + r[0]];
        o->below = o->pix[r[3] * o->pw + r[2]];
        return;
    }
    n = n < 1 ? 1 : n > 10 ? 10 : n;
    for (i = 0; i < n; i++) {
        chans(o->pix[r[1] * o->pw + r[0] + i], c);
        for (k = 0; k < 3; k++)
            acc[k] += c[k];
    }
    for (k = 0; k < 3; k++)
        acc[k] /= (unsigned)n;
    o->above = pack(acc);
    n = r[3] - r[1];
    n = n < 1 ? 1 : n > 10 ? 10 : n;
    for (i = 0; i < n; i++) {
        chans(o->pix[r[3] * o->pw + r[2] - i], c);
        for (k = 0; k < 3; k++)
            acc[k] += c[k];
    }
    for (k = 0; k < 3; k++)
        acc[k] /= (unsigned)n;
    o->below = pack(acc);
}

/* 0x477b30: 24-bit rows of R, G, B, or a palette (index 0 stays 0, a near-black entry
   gets red 8, off the colour key) */
static int load_picture(Picture *o, const char *path, const int r[4])
{
    unsigned char *d;
    long len;
    int w, h, npal, i;
    if (!(d = dsi_load(path, &len)))
        return 0;
    w = d[8] << 8 | d[9];
    h = d[10] << 8 | d[11];
    npal = d[12] << 8 | d[13];
    if (len < 32 || memcmp(d, "mhwanh", 6) || w <= r[2] || h <= r[3]
        || len < 32 + 3L * npal + (long)w * h * (npal ? 1 : 3)
        || !(o->pix = malloc(sizeof(uint16_t) * (size_t)w * (size_t)h))) {
        free(d);
        return 0;
    }
    o->pw = w;
    for (i = 0; i < w * h; i++) {
        unsigned char rgb[3];
        if (npal) {
            int k = d[32 + 3 * npal + i];
            if (!k && !behaviour_cd()) {
                o->pix[i] = 0;
                continue;
            }
            memcpy(rgb, d + 32 + 3 * k, 3);
            if (rgb[0] < 8 && rgb[1] < 4 && rgb[2] < 8 && !behaviour_cd())     /* (0x46595a: none) */
                rgb[0] = 8;
        } else
            memcpy(rgb, d + 32 + 3 * i, 3);
        o->pix[i] = (uint16_t)((rgb[0] >> 3) << 11 | (rgb[1] >> 2) << 5 | rgb[2] >> 3);
    }
    free(d);
    fill_colours(o, r);
    return 1;
}

/* 0x410350 keeps the screen's own; the port has the three, F2 and F3 changing it in a race */
int horizon_load(const char *folder)
{
    static const char *const names[3] = { "HorizLow", "HorizMed", "HorizHi" };
    char path[0x180];
    int i;
    horizon_free();
#ifdef OMR_ESP32
    for (i = 0; i < 1; i++) {           /* (the board's race is 320x240 only) */
#else
    for (i = 0; i < 3; i++) {
#endif
        snprintf(path, sizeof path, "%sBank\\%s.raw", folder, names[i]);
        if (!load_picture(&g_pics[i], path, g_rects[i][0])) {
            horizon_free();
            return 0;
        }
    }
    return 1;
}

/* 0x477400 -> 0x478990: an opaque copy, clipped to the screen; nothing when the clipped
   rectangle (inclusive) is a single row or column */
static void blit(const int r[4], int x, int y)
{
    int L = x, T = y, R = x + (r[2] - r[0]), B = y + (r[3] - r[1]), sl = r[0], st = r[1], yy;
    if (T < 0)
        st -= T, T = 0;
    if (B > g_scr_h - 1)
        B = g_scr_h - 1;
    if (L < 0)
        sl -= L, L = 0;
    if (R > g_scr_w - 1)
        R = g_scr_w - 1;
    if (!(T < B && L < R))
        return;
    for (yy = T; yy <= B; yy++)
        if (!g_half_rows || !(yy & 1))
            memcpy(g_screen + yy * g_pitch + L, g_pix + (st + yy - T) * g_pw + sl,
               sizeof(uint16_t) * (size_t)(R - L + 1));
}

static void fill(int y0, int y1, uint16_t c)    /* rows y0 .. y1 - 1 */
{
    int x, y, w = g_scr_w;
    for (y = y0 < 0 ? 0 : y0; y < y1 && y < g_scr_h; y++)
        if (!g_half_rows || !(y & 1)) {
            uint16_t *to = g_screen + y * g_pitch;
            for (x = 0; x < w; x++)
                to[x] = c;
        }
}

void horizon_draw(const float c[3][3], const float t[3], float cam_y, int offset,
                  float scale, float fx)
{
    const float q = g_cos[0x3000];              /* 0x608b80: cos 3pi/2 by the table */
    float vx, vy, vz, l, yaw, s, cc, a, b, px, py, pz, sphi, phi;
    double ym, l2, p, r, cf, sf, cf1, sf1, a1, b1, yp, zp;
    int n, i, tw, th, k, m, x, ytop, cnt;
    const int *first, *second;
    const Picture *o = &g_pics[g_screen_mode];
    if (!o->pix) {
        fill(0, g_scr_h, 0);
        return;
    }
    g_pix = o->pix, g_pw = o->pw, g_rect = g_rects[g_screen_mode];
    /* the heading: atan2 of the camera's third row (0x402990) */
    vx = (float)((double)c[2][0] + t[0]);
    vy = (float)((double)c[2][1] + t[1]);
    vz = (float)((double)c[2][2] + t[2]);
    l = (float)sqrt((double)vy * vy + (double)vz * vz + (double)vx * vx);
    yaw = 0.0f;
    if (l != 0.0f) {
        float nx = (float)((double)vx / l), nz = (float)((double)vz / l);
        yaw = (float)atan2((double)nz / l, (double)nx / l);
    }
    if (yaw < 0.0f) {
        double y = yaw;
        do
            y += PI100;
        while (y < 0.0);
        yaw = (float)y;
    }
    ym = fmod(yaw, TWO_PI);                     /* 0x4b30ca */
    /* the pitch: the level direction of that heading, into the camera */
    n = ftol((ym - HALF_PI) * K);
    cc = g_cos[n & 0x3fff];
    s = g_cos[(n - 0x1000) & 0x3fff];
    a = (float)-((double)q * cc);
    b = (float)((double)cc * g_cos[0]);
    px = (float)((double)c[0][0] * s + (double)c[1][0] * a + (double)c[2][0] * b + t[0]);
    py = (float)((double)c[0][1] * s + (double)c[1][1] * a + (double)c[2][1] * b + t[1]);
    pz = (float)((double)c[0][2] * s + (double)c[1][2] * a + (double)c[2][2] * b + t[2]);
    l2 = sqrt((double)py * py + (double)pz * pz + (double)px * px);
    sphi = l2 != 0.0 ? (float)(py / l2) : 0.0f;
    phi = l2 == 0.0 ? 0.0f : (float)asin(sphi);  /* 0x4b3092 */
    /* x: the two-tile strip k times around a turn */
    tw = g_rect[0][2] - g_rect[0][0] + 1;
    th = g_rect[0][3] - g_rect[0][1] + 1;
    if (behaviour_cd())         /* 0x41a956: the corner itself, a pixel less: the tiles overlap */
        tw = g_rect[0][2], th = g_rect[0][3];
    k = (g_scr_w - 1) * 6 / 640;
    p = TWO_PI / k;
    r = ym - ftol(ym / p) * p;
    m = ftol(2 * tw * (r / p));
    x = -(2 * tw + m);
    /* y: a ground point FAR ahead and cam_y below, turned by the pitch alone */
    i = ftol(phi * K);
    cf = g_cos[i & 0x3fff];
    sf = g_cos[(i - 0x1000) & 0x3fff];
    cf1 = g_cos[0] * cf, sf1 = g_cos[0] * sf;
    a1 = cf1 - (sf * q) * q;
    b1 = (cf * q) * q + sf1;
    yp = cam_y * a1 + -sf1 * FAR;
    zp = cam_y * b1 + FAR * cf1;
    ytop = ftol(fx / zp * yp * scale) - th - offset + g_scr_h / 2;
    /* the tiles, the first whole ones left of the screen counted: odd, A first */
    for (cnt = 0; x + tw < 0; cnt++)
        x += tw;
    first = g_rect[cnt & 1 ? 0 : 1], second = g_rect[cnt & 1 ? 1 : 0];
    /* black where nothing below comes: under the strip (a sliver at its right end is not drawn), and
       the last row */
    fill(ytop, ytop + th, 0);
    fill(g_scr_h - 1, g_scr_h, 0);
    while (x <= g_scr_w - 1) {
        blit(first, x, ytop);
        x += tw;
        blit(second, x, ytop);
        x += tw;
    }
    /* the fills (Blt COLORFILL, bottoms exclusive): the last row is never filled */
    fill(ytop + th > 0 ? ytop + th : 0, g_scr_h - 1, o->below);
    fill(0, ytop < g_scr_h - 1 ? ytop : g_scr_h - 1, o->above);
}
