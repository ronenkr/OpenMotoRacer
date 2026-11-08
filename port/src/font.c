/* The bitmap fonts (NOTES §4.8): the script FONTS.BPT (0x426460), the .FNT files
   (0x490d80), the colour tables (0x490b90, 0x495bd0), measuring (0x492b10) and drawing
   into the 16-bit screen (0x493820, 0x493490 and the classes' DrawChar). The port draws
   the three kinds the script uses for loaded fonts: Direct, AntiAlias8Transp and Pal. */
#include "config.h"
#include "font.h"
#include "exe.h"
#include "files.h"
#include "script.h"
#include "video.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FONTS 48

static Font *g_fonts[MAX_FONTS];    /* *0x4f1810 */
static int g_nfonts;                /* 0x4f180c */
int g_small_fonts = 1;              /* 0x4f0e08, from the settings (1 by default) */
int g_screen_mode = 2;              /* 0x4dc0c8: 0 320x240, 1 512x384, 2 640x480 */

static const unsigned char black[3];

const unsigned char *named_colour(int c)
{
    /* the 1997 program's table has no grey, which it writes as a number (NOTES §3.5a) */
    static const unsigned char grey[3] = { 127, 127, 127 };
    const unsigned char *p;
    if (exe_is_cd()) {
        if (c == COL_GREY)
            return grey;
        if (c > COL_GREY)
            c--;
    }
    p = exe_at(exe_addr(X_COLOURS) + 4 * (unsigned)c, 3);
    return p ? p : black;
}

static unsigned u32(const unsigned char *p)
{
    return (unsigned)p[0] | (unsigned)p[1] << 8 | (unsigned)p[2] << 16 | (unsigned)p[3] << 24;
}

/* 0x473a90 */
static uint16_t rgb565(unsigned r, unsigned g, unsigned b)
{
    return (uint16_t)((r & 0xff) >> 3 << 11 | (g & 0xff) >> 2 << 5 | (b & 0xff) >> 3);
}

/* slot 1 called as (1, 0xff): a pixel value's colour */
static void build_table(Font *f)
{
    int v, c;
    if (f->type == FT_PAL || f->type == FT_MOTO) {
        /* 0x495bd0: the font's own palette */
        for (v = 1; v < 256; v++)
            if (f->pal && f->npal) {
                const unsigned char *e = f->pal + 3 * (v % f->npal);
                f->table[v] = rgb565(e[0], e[1], e[2]);
            }
    } else if (f->type == FT_DIRECT || f->type == FT_DIRECT_TRAME || f->type == FT_AA8
               || f->type == FT_AA8_TRANSP) {
        /* 0x490b90: 32 steps from the back colour toward the fore colour, 8.16 fixed
           point, the steps taken on 16 bits so that going down works too */
        uint16_t ramp[32];
        unsigned acc[3], step[3];
        int i;
        for (c = 0; c < 3; c++) {
            acc[c] = (unsigned)f->back[c] << 16;
            step[c] = ((unsigned)(f->fore[c] - f->back[c]) & 0xffff) * 0x800;
        }
        for (i = 0; i < 32; i++) {
            ramp[i] = rgb565(acc[0] >> 16, acc[1] >> 16, acc[2] >> 16);
            for (c = 0; c < 3; c++)
                acc[c] += step[c];
        }
        for (v = 1; v < 256; v++)
            f->table[v] = ramp[v & 31];
    }
}

void font_set_colors(Font *f, const unsigned char *fore, const unsigned char *back)
{
    /* 0x490b00 (CustomColor's does nothing); a NULL keeps that colour */
    if (f->type == FT_CUSTOM || f->type == FT_CUSTOM_TRAME)
        return;
    if (!fore)
        fore = f->fore;
    if (!back)
        back = f->back;
    if (memcmp(f->fore, fore, 3) || memcmp(f->back, back, 3)) {
        memcpy(f->fore, fore, 3);
        memcpy(f->back, back, 3);
        build_table(f);
    }
}

void font_store_rgb(Font *f, const unsigned char *fore, const unsigned char *back)
{
    /* 0x490a30, 0x490a60 alone: the table is not rebuilt */
    memcpy(f->fore, fore, 3);
    memcpy(f->back, back, 3);
}

void font_set_rgb(Font *f, const unsigned char *fore, const unsigned char *back)
{
    /* 0x490a30, 0x490a60, then slot 1 (1, 0xff): set and rebuilt every time */
    memcpy(f->fore, fore, 3);
    memcpy(f->back, back, 3);
    build_table(f);
}

/* 0x490d80: one .FNT: u32 first, u32 count (the script's are used), u32 height, u32
   palette entries (0..256), the palette (R,G,B), count u32 widths, the glyphs */
static int load_fnt(Font *f, int k)
{
    char path[160], *dot;
    long len = 0;
    unsigned char *d;
    unsigned npal, p, i;
    snprintf(path, sizeof path, "%s", f->file[k]);
    dot = strrchr(path, '.');
    if (dot)
        strcpy(dot, ".FNT");
    d = dsi_load(path, &len);
    if (!d || len < 16)
        goto bad;
    npal = u32(d + 12);
    p = 16 + 3 * npal;
    if (npal > 256 || f->count[k] <= 0 || f->first[k] + f->count[k] > 256
        || (unsigned long)len < p + 4ul * f->count[k])
        goto bad;
    if (npal && f->own_palette && !f->pal) {
        f->pal = d + 16;
        f->npal = (int)npal;
    }
    for (i = 0; i < (unsigned)f->count[k]; i++)
        f->width[f->first[k] + i] = (int)u32(d + p + 4 * i);
    f->height = (int)u32(d + 8);
    f->data[k] = d;
    f->block[k] = d + p + 4 * f->count[k];
    return 1;
bad:
    free(d);
    fprintf(stderr, "openmotoracer: the font file %s could not be read\n", path);
    return 0;
}

/* 0x492170: the files, then each glyph's place in its file's block */
static void load_font(Font *f)
{
    int k, c;
    for (k = 0; k < f->nfiles; k++)
        if (!load_fnt(f, k))
            return;
    for (k = 0; k < f->nfiles; k++) {
        long at = 0;
        for (c = f->first[k]; c < f->first[k] + f->count[k]; c++) {
            f->glyph[c] = f->block[k] + at * f->height;
            at += f->width[c];
        }
    }
    build_table(f);
    f->loaded = f->nfiles > 0;
}

static int type_of(const char *s)
{
    static const char *const names[] = { "Direct", "DirectTrame", "CustomColor",
        "CustomColorTrame", "AntiAlias8", "AntiAlias8Transp", "Pal", "Moto" };
    int i;
    for (i = 0; i < 8; i++)
        if (strcmp(s, names[i]) == 0)
            return i + 1;
    return 0;
}

/* 0x426911: these two are Direct whatever the script says; 0x4939ee, the 1997 CD's, leaves
   them smooth and see-through as it says. Again at each race, the setting may have changed */
void font_follow_behaviour(void)
{
    int i;
    for (i = 0; i < g_nfonts; i++)
        if (g_fonts[i] && (strcmp(g_fonts[i]->name, "GameMsgLowRes") == 0
                           || strcmp(g_fonts[i]->name, "GameMsgHiRes") == 0))
            g_fonts[i]->type = behaviour_cd() ? g_fonts[i]->script_type : FT_DIRECT;
}

int font_init(char *err, int errsize)
{
    Script s;
    char name[64], type[64], cond[64];
    int i, n;
    if (!script_open(&s, "Data\\Fonts.Spt")) {
        snprintf(err, errsize, "data\\FONTS.BPT could not be read.");
        return 0;
    }
    /* FontNumber, twice (the game reads it twice) */
    n = script_int(&s);
    if (script_int(&s) != n || n < 0 || n > MAX_FONTS)
        goto bad;
    g_nfonts = n;
    /* FontTypes: index, name, type, condition */
    while ((i = script_int(&s)) != -1 && !s.bad) {
        Font *f;
        script_str(&s, name, sizeof name);
        script_str(&s, type, sizeof type);
        script_str(&s, cond, sizeof cond);
        if (i < 0 || i >= n || !(f = calloc(1, sizeof *f)))
            goto bad;
        f->type = f->script_type = type_of(type);
        if (!f->type)
            goto bad;
        snprintf(f->name, sizeof f->name, "%.15s", name);
        snprintf(f->cond, sizeof f->cond, "%.23s", cond);
        f->line = f->letter = 1;                            /* 0x490840 */
        f->fore[0] = 255, f->fore[1] = 50, f->fore[2] = 20;
        f->back[0] = 100, f->back[1] = 100, f->back[2] = 10;
        f->own_palette = f->type == FT_PAL || f->type == FT_MOTO;
        g_fonts[i] = f;
    }
    /* FontSpacings: index, letter, line */
    while ((i = script_int(&s)) != -1 && !s.bad)
        if (i >= 0 && i < n && g_fonts[i]) {
            g_fonts[i]->letter = script_int(&s);
            g_fonts[i]->line = script_int(&s);
        }
    /* FontForeColors, FontBackColors: index, R, G, B */
    while ((i = script_int(&s)) != -1 && !s.bad) {
        int r = script_int(&s), g = script_int(&s), b = script_int(&s);
        if (i >= 0 && i < n && g_fonts[i])
            g_fonts[i]->fore[0] = (unsigned char)r, g_fonts[i]->fore[1] = (unsigned char)g,
            g_fonts[i]->fore[2] = (unsigned char)b;
    }
    while ((i = script_int(&s)) != -1 && !s.bad) {
        int r = script_int(&s), g = script_int(&s), b = script_int(&s);
        if (i >= 0 && i < n && g_fonts[i])
            g_fonts[i]->back[0] = (unsigned char)r, g_fonts[i]->back[1] = (unsigned char)g,
            g_fonts[i]->back[2] = (unsigned char)b;
    }
    /* FontData: index, first, last, frame colour, file */
    while ((i = script_int(&s)) != -1 && !s.bad) {
        int first = script_int(&s), last = script_int(&s);
        Font *f;
        script_int(&s);                 /* the frame colour: for the .RAW pictures */
        script_str(&s, name, sizeof name);
        if (i < 0 || i >= n || !(f = g_fonts[i]) || f->nfiles == 4)
            goto bad;
        snprintf(f->file[f->nfiles], sizeof f->file[0], "%s", name);
        f->first[f->nfiles] = first;
        f->count[f->nfiles] = last - first + 1;
        f->nfiles++;
    }
    if (!script_at_end(&s))
        goto bad;
    script_close(&s);
    /* (the game loads only the fonts its condition allows, and changes them with the
       screen mode, 0x4961d0; the port has them all) */
#ifndef OMR_ESP32
    for (i = 0; i < n; i++)
        if (g_fonts[i])
            load_font(g_fonts[i]);
#endif
    return 1;
bad:
    script_close(&s);
    snprintf(err, errsize, "data\\FONTS.BPT is not as expected.");
    return 0;
}

static const int pairs[][2] = { { 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 }, { 10, 11 },
    { 13, 14 }, { 16, 15 }, { 19, 20 }, { 23, 22 }, { 26, 25 }, { 29, 28 }, { 33, 32 } };

/* 0x4903d0's second answer: a font with a low / high resolution partner */
int font_partnered(int n)
{
    int i;
    for (i = 0; i < (int)(sizeof pairs / sizeof pairs[0]); i++)
        if (n == pairs[i][0] || n == pairs[i][1])
            return 1;
    return 0;
}

/* the ESP32: a font's files read the first time it is asked for (35 fonts, 0.85 MB, most never
   used in a race; NOTES §4.26). Elsewhere all are read at the start. */
#ifdef OMR_ESP32
static unsigned char g_font_tried[MAX_FONTS];
static int need(int n)
{
    if (n >= 0 && n < MAX_FONTS && g_fonts[n] && !g_fonts[n]->loaded && !g_font_tried[n]) {
        g_font_tried[n] = 1;
        load_font(g_fonts[n]);
    }
    return n;
}
#else
static int need(int n)
{
    return n;
}
#endif

/* a font by its own number, no partner (*0x4f1810 + 4n), or 0 */
Font *font_raw(int n)
{
    need(n);
    return n >= 0 && n < MAX_FONTS && g_fonts[n] && g_fonts[n]->loaded ? g_fonts[n] : 0;
}

/* 0x4903d0: at 640x480 (and big fonts) a low-resolution font gives its partner */
Font *font_get(int n)
{
    int m = n, i, hires = !g_small_fonts && g_screen_mode == 2;
    if (n < 0 || n >= g_nfonts)
        n = m = 0;
    for (i = 0; i < (int)(sizeof pairs / sizeof pairs[0]); i++)
        if (n == pairs[i][!hires])
            m = pairs[i][hires];
    if (g_fonts[need(m)] && g_fonts[m]->loaded)
        return g_fonts[m];
    if (g_fonts[need(n)] && g_fonts[n]->loaded)
        return g_fonts[n];
    need(8);
    return g_fonts[8];
}

/* 0x492b10 */
void font_measure(const Font *f, const char *text, int *w, int *h, int one_line)
{
    int line = 0, best = 0;
    const unsigned char *s = (const unsigned char *)text;
    *h = f->height;
    for (; *s; s++) {
        if (*s == '\n') {
            if (one_line)
                break;
            if (line > best)
                best = line;
            line = 0;
            *h += f->line + f->height;
        } else if (f->width[*s] > 0)
            line += f->width[*s] + f->letter;
    }
    *w = line > best ? line : best;
}

/* one glyph (the classes' slot 6), clipped to the 640x480 screen */
static void draw_char(const Font *f, int c, int x, int y, int level)
{
    int w = f->width[c], h = f->height, row, col;
    const unsigned char *g = f->glyph[c];
    if (!g)
        return;
    VIDEO_ROWS(y, y + h - 1);
    for (row = 0; row < h; row++) {
        uint16_t *dst;
        if (y + row < 0 || y + row >= g_scr_h)
            continue;
        dst = g_screen + (y + row) * g_pitch;
        for (col = 0; col < w; col++) {
            unsigned b = g[row * w + col];
            if (!b || x + col < 0 || x + col >= g_scr_w)
                continue;
            if (f->type == FT_AA8_TRANSP) {
                /* 0x495280: the top 3 bits and the level make it see-through */
                unsigned t = (unsigned)level + (b & 0xe0), a, out = 0, S, D;
                static const unsigned masks[3] = { 0xf800, 0x07e0, 0x001f };
                int m;
                if (t > 224)
                    continue;
                a = (t << 8) / 224;
                S = f->table[b];
                D = dst[x + col];
                for (m = 0; m < 3; m++) {
                    int s = (int)(S & masks[m]), d = (int)(D & masks[m]);
                    out |= (unsigned)(s - (((s - d) * (int)a) >> 8)) & masks[m];
                }
                dst[x + col] = (uint16_t)out;
            } else
                dst[x + col] = f->table[b];     /* Direct 0x493100, Pal 0x495840 */
        }
    }
}

/* 0x492c20: the block's place */
void font_place(const Font *f, const char *text, int *x, int *y, int flags, int *w, int *h)
{
    int bw, bh;
    font_measure(f, text, &bw, &bh, 0);
    switch (flags & 0x1f) {
    case 2: case 4: *x = (g_scr_w - bw) / 2; break;
    case 8: case 0x10: *x = g_scr_w - *x - bw; break;
    }
    switch (flags & 0xe0) {
    case 0x40: *y = (g_scr_h - bh) / 2; break;
    case 0x80: *y = g_scr_h - *y - bh; break;
    }
    if (w)
        *w = bw;
    if (h)
        *h = bh;
}

void font_draw(Font *f, const char *text, int x, int y, int flags, int level)
{
    if (!g_draw_2d)             /* 0x493d49 */
        return;
    int bw, left, lw, lh;
    const unsigned char *s = (const unsigned char *)text;
    if (!f || !f->loaded || !*s)
        return;
    if (f->type == FT_AA8)
        level = (level << 11) / 100 >> 8;
    else if (f->type == FT_AA8_TRANSP)
        level = level * 0xe000 / 100 >> 8;
    if (flags != 0x21)
        font_place(f, text, &x, &y, flags, &bw, 0);
    else
        bw = 0;
    /* 0x493490: each line, aligned in the block for flags 4 and 0x10 */
    left = x;
    for (;;) {
        int lx = left;
        if ((flags & 0x1f) == 4 || (flags & 0x1f) == 0x10) {
            font_measure(f, (const char *)s, &lw, &lh, 1);
            lx += (flags & 0x1f) == 4 ? (bw - lw) / 2 : bw - lw;
        }
        for (; *s && *s != '\n'; s++)
            if (f->width[*s] > 0) {
                draw_char(f, *s, lx, y, level);
                lx += f->width[*s] + f->letter;
            }
        if (!*s)
            break;
        s++;
        y += f->height + f->line;
    }
}
