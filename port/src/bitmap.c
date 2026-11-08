/* Menu pictures (NOTES §3.3, §4.7): .LEZ files, a packed 8-bit TGA, made 16-bit at load
   (0x43eb00), listed in the game's table of menu pictures and drawn by 0x43ede0. */
#include "bitmap.h"
#include "clock.h"
#include "exe.h"
#include "files.h"
#include "video.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

/* X_BITMAP_TABLE (GOG 0x4e3e20): records of 20 bytes, u32 ?, language folder, id, name,
   mode; ended by id -1; copied out by id at 0x415ad2. X_LANG_FOLDERS (GOG 0x4e8b60):
   "data\MenuUs\", "data\MenuFr\", ... */

int g_lang;

static unsigned u32(const unsigned char *p)
{
    return (unsigned)p[0] | (unsigned)p[1] << 8 | (unsigned)p[2] << 16 | (unsigned)p[3] << 24;
}

/* 0x43eb00, the unpacking: "LEZ1", u32 size, u32 offset of the copy words, u32 offset of
   the literal bytes, u32, then u32 flag words from 0x14. A flag word is read from its top
   bit down, a 1 put after its last bit marking its end: 1 = one literal byte, 0 = a u16
   copy word, back (low 10 bits) and length (high 6 bits + 3). */
static unsigned char *lez_unpack_n(const unsigned char *d, long len, unsigned *outlen, unsigned limit);
static unsigned char *lez_unpack(const unsigned char *d, long len, unsigned *outlen)
{
    return lez_unpack_n(d, len, outlen, 0);
}

/* the first `limit` bytes only, if limit (the TGA's header, for its size) */
static unsigned char *lez_unpack_n(const unsigned char *d, long len, unsigned *outlen, unsigned limit)
{
    unsigned size, words, lits, fp = 0x18, bits, bit, n = 0;
    unsigned char *out;
    if (len < 0x18 || memcmp(d, "LEZ1", 4) != 0)
        return 0;
    size = u32(d + 4);
    words = u32(d + 8);
    lits = u32(d + 12);
    bits = u32(d + 0x14);
    bit = bits >> 31;
    bits = bits * 2 + 1;
    if (size == 0 || size > 0x1000000)
        return 0;
    if (limit && limit < size)
        size = limit;
    if (!(out = malloc(size)))
        return 0;
    for (;;) {
        if (bit) {
            if (lits >= (unsigned)len)
                break;
            out[n++] = d[lits++];
        } else {
            unsigned w, back, k;
            if (words + 2 > (unsigned)len)
                break;
            w = d[words] | d[words + 1] << 8;
            words += 2;
            back = w & 0x3ff;
            if (back > n)
                break;
            for (k = (w >> 10) + 3; k && n < size; k--, n++)
                out[n] = out[n - back];
        }
        if (n >= size) {
            *outlen = size;
            return out;
        }
        bit = bits >> 31;
        bits <<= 1;
        if (bits == 0) {
            unsigned w;
            if (fp + 4 > (unsigned)len)
                break;
            w = u32(d + fp);
            fp += 4;
            bit = w >> 31;
            bits = w * 2 + 1;
        }
    }
    free(out);
    return 0;
}

/* 0x43eb00, the rest: the TGA inside (palette length at +5, width +0xc, height +0xe,
   palette from 0x12 as B,G,R, then a byte a pixel, the rows as stored), each palette
   colour cut to the screen's 5-6-5 bits (no palette: a grey ramp). */
static int lez_to_bitmap(Bitmap *b, const unsigned char *t, unsigned len)
{
    uint16_t pal[256];
    int npal, w, h, i;
    const unsigned char *px;
    if (len < 0x12)
        return 0;
    npal = (int16_t)(t[5] | t[6] << 8);
    w = (int16_t)(t[12] | t[13] << 8);
    h = (int16_t)(t[14] | t[15] << 8);
    if (npal < 0 || npal > 256 || w <= 0 || h <= 0 || 0x12u + 3u * npal + (unsigned)(w * h) > len)
        return 0;
    if (npal == 0)
        for (i = 0; i < 256; i++)
            pal[i] = (uint16_t)((i >> 3) << 11 | (i >> 2) << 5 | i >> 3);
    for (i = 0; i < npal; i++) {
        const unsigned char *c = t + 0x12 + 3 * i;
        pal[i] = (uint16_t)((c[2] >> 3) << 11 | (c[1] >> 2) << 5 | c[0] >> 3);
    }
    for (i = npal ? npal : 256; i < 256; i++)
        pal[i] = 0;                    /* (the game reads past its palette here) */
    px = t + 0x12 + 3 * npal;
    b->w = w;
    b->h = h;
    if (b->mode == BM_TILE) {
        int x, y;
        if (w * h < 128 * 128)
            return 0;
        b->pix = malloc(256 * 128 * 2);
        if (!b->pix)
            return 0;
        for (y = 0; y < 128; y++)
            for (x = 0; x < 128; x++)
                b->pix[y * 256 + x] = b->pix[y * 256 + 128 + x] = pal[px[y * 128 + x]];
        return 1;
    }
    b->pix = malloc((size_t)w * h * 2);
    if (!b->pix)
        return 0;
    for (i = 0; i < w * h; i++)
        b->pix[i] = pal[px[i]];
    return 1;
}

#ifdef OMR_ESP32
static int to8(Bitmap *b, unsigned char *t, unsigned tlen);
#endif

int bitmap_load_lez(Bitmap *b, const char *path, int mode)
{
    long len = 0;
    unsigned tlen = 0;
    unsigned char *d = dsi_load(path, &len), *t;
    int ok;
    memset(b, 0, sizeof *b);
    b->mode = mode;
    if (!d)
        return 0;
    t = lez_unpack(d, len, &tlen);
    free(d);
    if (!t)
        return 0;
#ifdef OMR_ESP32
    if (mode != BM_TILE && tlen >= 0x12) {      /* the board: kept 8-bit, half the memory */
        b->w = (int16_t)(t[12] | t[13] << 8);
        b->h = (int16_t)(t[14] | t[15] << 8);
        if (b->w > 0 && b->h > 0 && to8(b, t, tlen))
            return 1;
        free(t);
        return 0;
    }
#endif
    ok = lez_to_bitmap(b, t, tlen);
    free(t);
    return ok;
}

int bitmap_load_id(Bitmap *b, int id)
{
    unsigned a;
    for (a = exe_addr(X_BITMAP_TABLE); exe_at(a, 20) && exe_u32(a + 8) != 0xffffffffu; a += 20) {
        const char *name = exe_str(exe_u32(a + 12)), *dot = strrchr(name, '.');
        unsigned lang = exe_u32(a + 4), mode = exe_u32(a + 16);
        char path[256];
        if (exe_u32(a + 8) != (unsigned)id)
            continue;
        if (!dot || strcasecmp(dot, ".lez") != 0) {
            fprintf(stderr, "bitmap %d: %s is not a .lez (24-bit TGA, 0x43e280: not yet)\n", id, name);
            return 0;
        }
        if (lang) {
            /* the language's folder, else the US one */
            snprintf(path, sizeof path, "%s%s", exe_str(exe_u32(exe_addr(X_LANG_FOLDERS) + 4 * g_lang)), name);
            if (bitmap_load_lez(b, path, (int)mode))
                return 1;
            snprintf(path, sizeof path, "%s%s", exe_str(exe_u32(exe_addr(X_LANG_FOLDERS))), name);
        } else
            snprintf(path, sizeof path, "data\\menu\\bitmap\\%s", name);
        if (bitmap_load_lez(b, path, (int)mode))
            return 1;
        fprintf(stderr, "bitmap %d: %s not found\n", id, path);
        return 0;
    }
    fprintf(stderr, "bitmap %d: not in the table\n", id);
    return 0;
}

/* ------------------------------------------------------------------ the ESP32's cache */
#ifdef OMR_ESP32
#define CACHE_BUDGET (1536u * 1024)     /* bytes of pixels kept */
#define CACHE_MAX 64
static Bitmap *g_cached[CACHE_MAX];
static int g_ncached;
static unsigned g_cache_bytes, g_clock;

static unsigned pix_bytes(const Bitmap *b)
{
    return b->mode == BM_TILE ? 128u * 128 + 512 : (unsigned)b->w * (unsigned)b->h + 512;
}

static void uncache(int i)
{
    Bitmap *b = g_cached[i];
    g_cache_bytes -= pix_bytes(b);
    free(b->block);
    b->block = 0;
    b->px8 = 0;
    b->pal = 0;
    g_cached[i] = g_cached[--g_ncached];
}

void bitmap_cache_flush(void)
{
    while (g_ncached)
        uncache(0);
}

/* the picture numbered id's unpacked TGA (as bitmap_load_id finds it), its first `limit` bytes
   if limit */
static unsigned char *find_tga(int id, unsigned limit, unsigned *tlen, int *mode)
{
    unsigned a;
    for (a = exe_addr(X_BITMAP_TABLE); exe_at(a, 20) && exe_u32(a + 8) != 0xffffffffu; a += 20) {
        const char *name = exe_str(exe_u32(a + 12));
        unsigned lang = exe_u32(a + 4);
        char path[256];
        int k;
        if (exe_u32(a + 8) != (unsigned)id)
            continue;
        *mode = (int)exe_u32(a + 16);
        for (k = lang ? 0 : 1; k < 2; k++) {
            long len = 0;
            unsigned char *d, *t;
            if (lang)
                snprintf(path, sizeof path, "%s%s", exe_str(exe_u32(exe_addr(X_LANG_FOLDERS) + 4 * (k ? 0 : g_lang))), name);
            else
                snprintf(path, sizeof path, "data\\menu\\bitmap\\%s", name);
            if (!(d = dsi_load(path, &len)))
                continue;
            t = lez_unpack_n(d, len, tlen, limit);
            free(d);
            if (t)
                return t;
        }
        fprintf(stderr, "bitmap %d: %s not found\n", id, name);
        return 0;
    }
    return 0;
}

/* the pixels of a lazily loaded picture as palette indices, making room first: the TGA kept,
   its header and palette made the 16-bit palette in place */
/* a TGA (its header, palette and indices) made a picture's 8-bit pixels and 16-bit palette, in
   place: the TGA is kept (freed with the picture), or freed */
static int to8(Bitmap *b, unsigned char *t, unsigned tlen)
{
    uint16_t pal[256];
    int i, npal, at;
    npal = (int16_t)(t[5] | t[6] << 8);
    at = 0x12 + 3 * npal;
    if (npal < 0 || npal > 256 || (unsigned)at + (unsigned)(b->w * b->h) > tlen) {
        free(t);
        return 0;
    }
    if (npal == 0)
        for (i = 0; i < 256; i++)
            pal[i] = (uint16_t)((i >> 3) << 11 | (i >> 2) << 5 | i >> 3);
    for (i = 0; i < npal; i++) {
        const unsigned char *c = t + 0x12 + 3 * i;
        pal[i] = (uint16_t)((c[2] >> 3) << 11 | (c[1] >> 2) << 5 | c[0] >> 3);
    }
    for (i = npal ? npal : 256; i < 256; i++)
        pal[i] = 0;
    if (at < 512) {                     /* no room for the palette before the pixels */
        unsigned char *n = malloc(512 + (size_t)b->w * b->h);
        if (!n) {
            free(t);
            return 0;
        }
        memcpy(n + 512, t + at, (size_t)b->w * b->h);
        free(t);
        t = n;
        at = 512;
    }
    memcpy(t, pal, 512);
    b->block = t;
    b->pal = (uint16_t *)t;
    b->px8 = t + at;
    return 1;
}

static void need(Bitmap *b)
{
    unsigned tlen = 0;
    unsigned char *t;
    int i, old, mode;
    if (b->pix || b->px8 || !b->id)
        return;
    while (g_ncached && (g_ncached == CACHE_MAX || g_cache_bytes + pix_bytes(b) > CACHE_BUDGET)) {
        for (old = 0, i = 1; i < g_ncached; i++)
            if (g_cached[i]->used < g_cached[old]->used)
                old = i;
        uncache(old);
    }
    if (!(t = find_tga(b->id - 1, 0, &tlen, &mode)))
        return;
    if (!to8(b, t, tlen))
        return;
    g_cached[g_ncached++] = b;
    g_cache_bytes += pix_bytes(b);
}

/* its size and mode now (the TGA's header), its pixels when drawn */
int bitmap_load_id_lazy(Bitmap *b, int id)
{
    unsigned tlen = 0;
    int mode;
    unsigned char *t = find_tga(id, 0x12, &tlen, &mode);
    if (!t)
        return 0;
    memset(b, 0, sizeof *b);
    b->mode = mode;
    b->w = (int16_t)(t[12] | t[13] << 8);
    b->h = (int16_t)(t[14] | t[15] << 8);
    b->id = id + 1;
    free(t);
    if (b->mode == BM_TILE && (b->w < 128 || b->h < 128))
        return 0;
    if (b->mode == BM_TILE)
        b->w = 128;                     /* (its rows of indices: the tile's own 128) */
    return b->w > 0 && b->h > 0;
}
#else
void bitmap_cache_flush(void)
{
}
#endif

int bitmap_loaded(const Bitmap *b)
{
#ifdef OMR_ESP32
    if (b->px8)
        return 1;
#endif
    return b->pix != 0;
}

void bitmap_free(Bitmap *b)
{
#ifdef OMR_ESP32
    if (b->block && !b->id) {           /* (a cached one is the cache's to give up) */
        free(b->block);
        b->block = 0;
        b->px8 = 0;
        b->pal = 0;
    }
#endif
    free(b->pix);
    b->pix = 0;
}

void bitmap_place(Bitmap *b, int x, int y, int align)
{
    if (align == 1)
        x = (640 - b->w) >> 1;
    else if (align == 2) {
        x -= b->w >> 1;
        y -= b->h >> 1;
    }
    b->x = x;
    b->y = y;
}

void bitmap_slide(Bitmap *b, int x, int y, float seconds, int ease, int align)
{
    b->t0 = g_now;
    b->dur = (unsigned)(int)((double)seconds * 1000.0);    /* 0.7 s: 699 ms */
    b->t1 = b->t0 + b->dur;
    if (align == 1)
        x = (640 - b->w) >> 1;
    else if (align == 2) {
        y -= b->h >> 1;
        x -= b->w >> 1;
    }
    b->dx = x - b->x;
    b->dy = y - b->y;
    b->from_x = b->x;
    b->from_y = b->y;
    b->sliding = 1;
    b->ease = ease;
}

void bitmap_step(Bitmap *b, int dx, int dy)
{
    unsigned n;
    float f;
    double s;
    if (b->mode == BM_TILE) {
        /* 0x43f840: the scroll, each frame (it also moves a wobble the drawing never
           reads, NOTES §4.7); here each 60th of a second */
        b->sx = (b->sx + dx * g_ticks) & 0x7f;
        b->sy = (b->sy + dy * g_ticks) & 0x7f;
        return;
    }
    if (!b->sliding)
        return;
    n = g_now < b->t1 ? g_now : b->t1;
    f = (float)((double)(n - b->t0) / (double)b->dur);
    if (b->ease == 0)
        s = f;
    else if (b->ease == 1)
        s = (sin(f * 3.141592653589793 - 1.5707963267948966) + 1.0) * 0.5;
    else
        return;
    b->x = (int)(b->dx * s + b->from_x);
    b->y = (int)(b->dy * s + b->from_y);
    if (n == b->t1)
        b->sliding = 0;
}

/* 0x43f0f0: the tile over the whole 640x480 screen, 128 pixels at a time, rows going
   round the tile from sy, each row from sx */
static void draw_tile(const Bitmap *b)
{
    int y, k, row = b->sy;
    const uint16_t *src;
    uint16_t *dst = g_screen;
    for (y = 0; y < 480; y++) {
        src = b->pix + (row & 0x7f) * 256 + (b->sx & 0x7f);
        for (k = 0; k < 5; k++)
            memcpy(dst + 128 * k, src, 128 * 2);
        row = (row & 0x7f) + 1;
        dst += g_pitch;
    }
}

#ifdef OMR_ESP32
/* the same drawing from a cached picture's indices and palette */
static void draw8(const Bitmap *b, int mode, long off, int rows, int cols, uint16_t *dst)
{
    const uint8_t *src = b->px8 + off;
    const uint16_t *pal = b->pal;
    int k;
    switch (mode) {
    case BM_OPAQUE:
        for (; rows; rows--, src += b->w, dst += g_pitch)
            for (k = 0; k < cols; k++)
                dst[k] = pal[src[k]];
        break;
    case BM_KEYED:
        for (; rows; rows--, src += b->w, dst += g_pitch)
            for (k = 0; k < cols; k++)
                if (pal[src[k]])
                    dst[k] = pal[src[k]];
        break;
    case BM_HALF:
    case BM_HALF_KEYED: {
        int step = 1, n = cols >> 1, even = cols & ~1;
        for (; rows; rows--) {
            for (k = 0; k < n; k++, src += 2, dst += 2)
                if (mode == BM_HALF || pal[*src])
                    *dst = pal[*src];
            src += b->w + step - even;
            dst += g_pitch + step - even;
            step = -step;
        }
        break;
    }
    }
}

static void draw_tile8(const Bitmap *b)
{
    int y, x, row = b->sy;
    uint16_t *dst = g_screen;
    for (y = 0; y < 480; y++) {
        const uint8_t *src = b->px8 + (row & 0x7f) * 128;
        int sx = b->sx & 0x7f;
        for (x = 0; x < 640; x++)
            dst[x] = b->pal[src[(sx + x) & 0x7f]];
        row = (row & 0x7f) + 1;
        dst += g_pitch;
    }
}
#endif

void bitmap_draw(const Bitmap *b, int mode, int pct_h, int pct_w)
{
    if (!g_draw_2d)             /* 0x43edf3 */
        return;
    int x, y, rows, cols, skip, k;
    long off;
    const uint16_t *src;
    uint16_t *dst;
#ifdef OMR_ESP32
    if (b) {
        ((Bitmap *)b)->used = ++g_clock;
        need((Bitmap *)b);
    }
    if (b && !b->pix && b->px8) {
        if (b->mode == BM_TILE) {
            draw_tile8(b);
            return;
        }
    } else
#endif
    if (!b || !b->pix)
        return;
    if (b->mode == BM_TILE) {
        draw_tile(b);
        return;
    }
    if (pct_h < 0)
        pct_h = 0;
    if (pct_h > 100)
        pct_h = 100;
    if (pct_w < 0)
        pct_w = 0;
    if (pct_w > 100)
        pct_w = 100;
    skip = (100 - pct_h) * b->h / 100;
    x = b->x;
    y = b->y + skip;
    off = (long)skip * b->w;
    rows = b->h - skip;
    cols = b->w + pct_w * b->w / -100;
    if (x >= g_scr_w || x + b->w < 0 || y >= g_scr_h || y + rows < 0)
        return;
    if (x < 0) {
        cols += x;
        off -= x;
        x = 0;
    }
    if (y < 0) {
        off -= (long)y * b->w;
        rows += y;
        y = 0;
    }
    if (b->w + x > g_scr_w)
        cols = cols - x + (g_scr_w - b->w);
    if (rows + y > g_scr_h)
        rows = g_scr_h - y;
    dst = g_screen + y * g_pitch + x;
    if (rows <= 0 || cols <= 0)
        return;
    VIDEO_ROWS(y, y + rows - 1);
    if (mode == BM_OWN)
        mode = b->mode;
#ifdef OMR_ESP32
    if (!b->pix) {
        draw8(b, mode, off, rows, cols, dst);
        return;
    }
#endif
    src = b->pix + off;
    switch (mode) {
    case BM_OPAQUE:
        for (; rows; rows--, src += b->w, dst += g_pitch)
            memcpy(dst, src, cols * 2);
        break;
    case BM_KEYED:
        for (; rows; rows--, src += b->w, dst += g_pitch)
            for (k = 0; k < cols; k++)
                if (src[k])
                    dst[k] = src[k];
        break;
    case BM_HALF:
    case BM_HALF_KEYED: {
        /* every other pixel, the row after starting one pixel later, then one earlier */
        int step = 1, n = cols >> 1, even = cols & ~1;
        for (; rows; rows--) {
            for (k = 0; k < n; k++, src += 2, dst += 2)
                if (mode == BM_HALF || *src)
                    *dst = *src;
            src += b->w + step - even;
            dst += g_pitch + step - even;
            step = -step;
        }
        break;
    }
    }
}
