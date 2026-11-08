/* A race track (build/notes/track.md §2, §5): the .trk file read as 0x40d650 reads it,
   the level's texture pages (0x410590), the section a point is in (0x40c740, 0x40c7e0)
   and the pieces a camera sees (0x40ce60), drawn through the race's renderer. */
#include "land.h"
#include "bikephys.h"
#include "choices.h"
#include "config.h"
#include "files.h"
#include "ftol.h"
#include "horizon.h"
#include "script.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BLOCKS_AHEAD 10         /* track +0x24, from the constructor 0x40c560 */
#define LEVEL_BANKS 20          /* Level.Spt's texture banks: pages 0..19 */

Land g_land;

/* the track file read through a window of it (it is 1.3 MB: the ESP32 has no room for it
   whole, NOTES §4.26) */
#define WINDOW 32768
typedef struct {
    FILE *f;
    long start, len, p;
    int bad;
    long wbase, wlen;
    unsigned char *buf;
} Reader;

static const unsigned char *take(Reader *r, long n)
{
    const unsigned char *q;
    if (r->bad || n < 0 || n > r->len - r->p || n > WINDOW) {
        r->bad = 1;
        return 0;
    }
    if (r->p < r->wbase || r->p + n > r->wbase + r->wlen) {
        long want = r->len - r->p < WINDOW ? r->len - r->p : WINDOW;
        if (fseek(r->f, r->start + r->p, SEEK_SET) != 0
            || (r->wlen = (long)fread(r->buf, 1, (size_t)want, r->f)) < n) {
            r->bad = 1;
            return 0;
        }
        r->wbase = r->p;
    }
    q = r->buf + (r->p - r->wbase);
    r->p += n;
    return q;
}

static int rd_i32(Reader *r)
{
    const unsigned char *q = take(r, 4);
    return q ? (int)((uint32_t)q[0] | (uint32_t)q[1] << 8 | (uint32_t)q[2] << 16
                     | (uint32_t)q[3] << 24) : 0;
}

static int rd_u16(Reader *r)
{
    const unsigned char *q = take(r, 2);
    return q ? q[0] | q[1] << 8 : 0;
}

static float rd_f32(Reader *r)
{
    union { uint32_t u; float f; } x;
    x.u = (uint32_t)rd_i32(r);
    return x.f;
}

/* a count that the rest of the file can hold, at least min bytes each */
static int rd_count(Reader *r, long min)
{
    int n = rd_i32(r);
    if (n < 0 || (long)n > (r->len - r->p) / min)
        r->bad = 1;
    return r->bad ? 0 : n;
}

/* 0x422a20 with its faces (last argument 1) and 0x423240 for a face before version 6:
   the points as they are, (x, y, z) and a word dropped; per face kind, flags, colour,
   page, five corners (the fifth 0xffff: a triangle) and five (u, v) in texels */
static void read_model(Reader *r, Model *m)
{
    int i, k;
    memset(m, 0, sizeof *m);
    m->nv = rd_count(r, 16);
    if (m->nv && !(m->v = calloc((size_t)m->nv, sizeof *m->v))) {
        r->bad = 1;
        return;
    }
    for (i = 0; i < m->nv; i++) {
        for (k = 0; k < 3; k++)
            m->v[i][k] = rd_f32(r);
        rd_i32(r);
    }
    m->nf = rd_count(r, 58);
    if (m->nf && !(m->f = calloc((size_t)m->nf, sizeof *m->f))) {
        r->bad = 1;
        return;
    }
    for (i = 0; i < m->nf; i++) {
        Face *f = &m->f[i];
        int idx[5];
        f->kind = rd_u16(r);
        f->flags = rd_u16(r);
        f->colour = rd_u16(r);
        f->page = rd_u16(r);
        for (k = 0; k < 5; k++)
            idx[k] = rd_u16(r);
        f->n = idx[4] != 0xffff ? 4 : 3;
        for (k = 0; k < 5; k++) {
            float u = rd_f32(r), v = rd_f32(r);
            if (k < 4) {
                f->ut[k] = u, f->vt[k] = v;     /* (the affine u: ftol(u x 8192.0), FACE_U) */
                /* (the files have rubbish in faces' unused corners) */
            }
        }
        for (k = 0; k < f->n; k++) {
            f->idx[k] = idx[k];
            if (idx[k] >= m->nv)
                f->kind = 0xd;          /* (a corner out of the model: not drawn) */
        }
    }
}

static void read_name(Reader *r, char *out, int size)
{
    int n = rd_count(r, 1);
    const unsigned char *q = take(r, n);
    int i;
    for (i = 0; q && i < n && i < size - 1 && q[i]; i++)
        out[i] = (char)q[i];
    out[i] = 0;
}

/* 0x48b7e0: "AnimDecor\0" (else the ten bytes are given back), a version, then the records:
   0x20 bytes at version 1, else 0x40, the times x 0.1 (the step's units); a record of kind
   7 from version 3 is followed by its frames of points */
static void read_anims(Reader *r)
{
    Land *l = &g_land;
    const unsigned char *q = take(r, 10);
    int v, i, k, f;
    if (!q)
        return;
    if (memcmp(q, "AnimDecor", 10)) {
        r->p -= 10;
        return;
    }
    v = rd_i32(r);
    if (v <= 0)
        return;
    l->nanim = rd_count(r, 0x20);
    if (!(l->anim = calloc((size_t)l->nanim + 1, sizeof *l->anim))) {
        r->bad = 1;
        return;
    }
    for (i = 0; i < l->nanim && !r->bad; i++) {
        DecorAnim *a = &l->anim[i];
        unsigned w = (unsigned)rd_i32(r);
        float x[15];
        for (k = 0; k < (v == 1 ? 7 : 15); k++)
            x[k] = rd_f32(r);
        a->decor = (int)(w & 0xffff);
        a->type = (int)(w >> 16 & 0xff);
        a->sub = (int)(w >> 24);
        a->t = (float)(x[0] * 0.1);
        a->a = x[1];
        a->b = x[2];
        a->c = x[3];
        a->period = (float)(x[4] * 0.1);
        if (v > 1)
            for (k = 0; k < 3; k++) {
                a->from[k] = x[5 + k];
                a->to[k] = x[8 + k];
            }
        if (a->type == 8)
            a->c = (float)(a->c * 0.1);
        if (v > 2 && a->type == 7) {
            a->nframes = rd_i32(r);
            if (a->nframes <= 0) {
                a->nframes = 0;
                continue;
            }
            a->npoints = rd_count(r, 12);
            if (a->nframes >= 0x50 || !(a->frame = calloc((size_t)a->nframes, sizeof *a->frame))) {
                r->bad = 1;
                return;
            }
            for (f = 0; f < a->nframes; f++) {
                if (!(a->frame[f] = calloc((size_t)a->npoints + 1, sizeof *a->frame[f]))) {
                    r->bad = 1;
                    return;
                }
                for (k = 0; k < a->npoints; k++) {
                    a->frame[f][k][0] = rd_f32(r);
                    a->frame[f][k][1] = rd_f32(r);
                    a->frame[f][k][2] = rd_f32(r);
                }
            }
        }
    }
}

/* 0x40d650 */
static int read_trk(FILE *f, long len)
{
    Reader r = { f, ftell(f), len, 0, 0, 0, 0, 0 };
    Land *l = &g_land;
    const unsigned char *magic;
    int i, k, v, n;
    if (len < 12 || r.start < 0 || !(r.buf = malloc(WINDOW)) || !(magic = take(&r, 4))
        || memcmp(magic, "TRK", 4)) {
        free(r.buf);
        return 0;
    }
    v = l->version = rd_i32(&r);
    l->nsec = rd_count(&r, 0xb4);
    if (!(l->sec = calloc((size_t)l->nsec + 1, sizeof *l->sec)))
        { free(r.buf); return 0; }
    for (i = 0; i < l->nsec; i++) {
        Section *s = &l->sec[i];
        for (k = 0; k < 3; k++)
            s->pos[k] = rd_f32(&r);
        for (k = 0; k < 9; k++)
            s->axes[k / 3][k % 3] = rd_f32(&r);
        for (k = 0; k < 3; k++)
            rd_f32(&r);                         /* +0x30, always 0 */
        s->block = rd_i32(&r);
        for (k = 0; k < 16; k++)                /* (0x40e5c0 makes the first again: bikephys_track) */
            s->plane[k / 4][k % 4] = rd_f32(&r);
        s->line = rd_f32(&r);
        if (v > 0) {
            n = rd_count(&r, 2);
            for (k = 0; k < n; k++) {
                int di = rd_u16(&r);
                if (s->ndecor < LAND_SECDECOR)
                    s->decor[s->ndecor++] = (uint16_t)di;
            }
        }
        s->edge[0] = -1024.0f, s->edge[1] = 1024.0f;
        if (v > 3) {
            s->edge[0] = rd_f32(&r);
            s->edge[1] = rd_f32(&r);
        }
        s->checkpoint = -1;
    }
    l->nblock = rd_count(&r, 8);
    if (!(l->block = calloc((size_t)l->nblock + 1, sizeof *l->block)))
        { free(r.buf); return 0; }
    for (i = 0; i < l->nblock; i++)
        read_model(&r, &l->block[i]);
    if (v > 0) {
        l->nlib = rd_count(&r, 28);
        if (!(l->lib = calloc((size_t)l->nlib + 1, sizeof *l->lib)))
            { free(r.buf); return 0; }
        for (i = 0; i < l->nlib; i++) {
            read_name(&r, l->lib[i].name, sizeof l->lib[i].name);
            for (k = 0; k < 3; k++)
                read_model(&r, &l->lib[i].m[k]);
            l->lib[i].v0 = l->lib[i].m[0].v;
        }
        l->ndecor = rd_count(&r, 32);
        if (!(l->decor = calloc((size_t)l->ndecor + 1, sizeof *l->decor)))
            { free(r.buf); return 0; }
        for (i = 0; i < l->ndecor; i++) {
            Decor *o = &l->decor[i];
            char name[64];
            read_name(&r, name, sizeof name);
            for (k = 0; k < 3; k++)
                o->pos[k] = rd_f32(&r);
            for (k = 0; k < 3; k++)
                o->ang[k] = rd_f32(&r);
            o->lib = rd_i32(&r);
            o->anim = v > 4 ? rd_i32(&r) : -1;
            if (o->lib < 0 || o->lib >= l->nlib)
                o->lib = -1;
            o->camera = strncmp(name, "CAMERA", 6) == 0 || strncmp(name, "camera", 6) == 0;
            if (o->lib >= 0 && strncmp(l->lib[o->lib].name, "CAMERA", 6) == 0)
                l->ncam++;                      /* 0x473240 */
        }
    }
    if (v > 1) {
        n = rd_count(&r, 6);
        for (i = 0; i < n; i++) {
            Section dummy, *s = i < l->nsec ? &l->sec[i] : &dummy;
            s->checkpoint = (int16_t)rd_u16(&r);
            s->time = rd_u16(&r);
            rd_u16(&r);                         /* +0xde */
            if (v > 3) {
                s->event = rd_i32(&r);
                s->ground_off = rd_i32(&r);
                s->ground_on = rd_i32(&r);
                s->flag = rd_i32(&r);
            }
        }
        if (v > 2) {
            l->horizon_offset = rd_i32(&r);
            l->horizon_scale = rd_f32(&r);
            read_anims(&r);
        }
    }
    for (i = 0; i < l->nsec; i++)
        if (l->sec[i].block < 0 || l->sec[i].block >= l->nblock)
            l->sec[i].block = 0;
    free(r.buf);
    return !r.bad && l->nsec > 0 && l->nblock > 0;
}

static void free_model(Model *m)
{
    free(m->v);
    free(m->f);
    memset(m, 0, sizeof *m);
}

void land_free(void)
{
    int i, k;
    for (i = 0; i < g_land.nblock; i++)
        free_model(&g_land.block[i]);
    for (i = 0; i < g_land.nlib; i++) {
        if (g_land.lib[i].v0)
            g_land.lib[i].m[0].v = g_land.lib[i].v0;
        for (k = 0; k < 3; k++)
            free_model(&g_land.lib[i].m[k]);
    }
    for (i = 0; i < g_land.nanim; i++) {
        if (g_land.anim[i].frame)
            for (k = 0; k < g_land.anim[i].nframes; k++)
                free(g_land.anim[i].frame[k]);
        free(g_land.anim[i].frame);
    }
    free(g_land.anim);
    free(g_land.sec);
    free(g_land.block);
    free(g_land.lib);
    free(g_land.decor);
    memset(&g_land, 0, sizeof g_land);
    bikephys_track_free();
    for (i = 2; i < LEVEL_BANKS; i++)
        r3d_free_page(i);
    horizon_free();
}

/* Level.Spt (0x427e80): NewMotorEngine, FileLand and nine more names, then the texture
   banks, each a name, its Direct3D twin and a number */
int land_load(int track, char *err, int errsize)
{
    Script s;
    char spt[0x120], land[64], name[64], path[0x180];
    FILE *f;
    long len;
    int i, k, ok;
    land_free();
    if (track < 0 || track >= g_ntracks) {
        snprintf(err, (size_t)errsize, "No track %d.", track);
        return 0;
    }
    snprintf(spt, sizeof spt, "%sLevel.Spt", g_tracks[track].folder);
    if (!script_open(&s, spt)) {
        snprintf(err, (size_t)errsize, "%s could not be read.", spt);
        return 0;
    }
    script_int(&s);
    script_str(&s, land, sizeof land);
    for (i = 0; i < 9; i++)
        script_str(&s, name, sizeof name);
    for (i = 0; i < LEVEL_BANKS && !s.bad; i++) {
        script_str(&s, name, sizeof name);
        script_str(&s, path, sizeof path);
        k = script_int(&s);                     /* 0x51d168 + i x 4 */
        if (i == 1)
            g_land.bank1_rgb = (k & 4) != 0;
        if (i >= 2 && !s.bad) {                 /* 0, 1: the bikes' (0x518b20, 0x518b60) */
            snprintf(path, sizeof path, "%s%s", g_tracks[track].folder, name);
            if (!r3d_load_page(i, path))
                fprintf(stderr, "openmotoracer: %s could not be read\n", path);
#ifdef OMR_ESP32
            r3d_page_to8(i);                    /* (half the memory: the board's 8 MB) */
#endif
        }
    }
    /* the int fields (0x425bb0 / 0x425080) and the float ones (0x4257e0 / 0x424c00) in
       the order 0x427e80 reads them */
    g_land.step_global = script_int(&s);
    g_land.yscale = script_int(&s);
    for (i = 0; i < 3; i++)
        g_land.start[i] = script_int(&s);
    g_land.optimise = script_int(&s);
    for (i = 0; i < 9; i++)
        for (k = 0; k < 5; k++)         /* MediumZone and HightZone (4, 5) are ints */
            g_land.detail[i][k] = i == 4 || i == 5 ? (float)script_int(&s) : script_float(&s);
    for (i = 0; i < 13; i++)
        for (k = 0; k < 3; k++)
            g_land.ai[i][k] = script_float(&s);
    g_land.ncomputer = script_int(&s);
    for (i = 0; i < g_land.ncomputer; i++) {
        float v = script_float(&s);
        if (i < 32)
            g_land.computer_speed[i] = v;
    }
    for (i = 0; i < 3; i++)
        g_land.time_skill[i] = script_int(&s);
    g_land.ncheck = script_int(&s);
    for (i = 0; i < g_land.ncheck; i++) {
        k = script_int(&s);
        if (i < 32)
            g_land.check_time[i] = k;
    }
    g_land.factor_pocket = script_float(&s);
    g_land.audio_track = script_at_end(&s) ? 0 : script_int(&s);   /* 0x4db67c */
    ok = !s.bad;
    script_close(&s);
    if (!ok) {
        snprintf(err, (size_t)errsize, "%s is not as expected.", spt);
        return 0;
    }
    snprintf(path, sizeof path, "%s%s", g_tracks[track].folder, land);
    if (!(f = dsi_fopen(path, &len))) {
        snprintf(err, (size_t)errsize, "%s could not be read.", path);
        return 0;
    }
    ok = read_trk(f, len);
    fclose(f);
    /* 0x40df3c: the checkpoint sections, in the track's order, take TimeCheckPoint's times
       one after the other */
    for (i = k = 0; ok && i < g_land.nsec; i++)
        if (g_land.sec[i].checkpoint != -1 && k < 32)
            g_land.sec[i].time = g_land.check_time[k++];
    if (ok && !horizon_load(g_tracks[track].folder))
        fprintf(stderr, "openmotoracer: %sBank\\HorizHi.raw could not be read\n",
                g_tracks[track].folder);
    if (!ok) {
        land_free();
        snprintf(err, (size_t)errsize, "%s is not a track this game knows.", path);
        return 0;
    }
    bikephys_track();           /* 0x40e5c0: the section planes the bikes use */
    return 1;
}

static double plane_dist(const float pl[4], const float p[3])   /* 0x4b10c0 */
{
    return (double)pl[0] * p[0] + (double)pl[1] * p[1] + (double)pl[2] * p[2] + pl[3];
}

int land_section(const float pos[3], int hint)
{
    int n = g_land.nsec, i;
    if (n <= 0)
        return 0;
    if (hint < 0 || hint >= n) {                /* 0x40c740: the nearest by distance */
        double best = 0.0;
        hint = 0;
        for (i = 0; i < n; i++) {
            double dx = (double)g_land.sec[i].pos[0] - pos[0];
            double dy = (double)g_land.sec[i].pos[1] - pos[1];
            double dz = (double)g_land.sec[i].pos[2] - pos[2];
            double dd = dx * dx + dy * dy + dz * dz;
            if (i == 0 || dd < best)
                best = dd, hint = i;
        }
    }
    return land_walk(pos, hint, 0);
}

/* 0x40c7e0: forward while in front of the next one's start plane, back while behind its
   own; *dist = how far in front of the section's plane (going back) or minus how far
   from the next one (going forward) */
int land_walk(const float pos[3], int hint, float *dist)
{
    int n = g_land.nsec, i, k;
    double d;
    if (n <= 0)
        return 0;
    i = hint >= 0 && hint < n ? hint : 0;
    d = plane_dist(g_land.sec[i].plane[0], pos);
    if (d >= 0.0) {
        for (k = 0; k < n; k++) {
            d = plane_dist(g_land.sec[(i + 1) % n].plane[0], pos);
            if (d < 0.0)
                break;
            i = (i + 1) % n;
        }
    } else
        for (k = 0; k < n; k++) {
            i = (i + n - 1) % n;
            d = plane_dist(g_land.sec[i].plane[0], pos);
            if (d >= 0.0)
                break;
        }
    if (dist)
        *dist = (float)d;
    return i;
}

/* 0x403880: the decor's rotation from its angles (a, b, c), every entry a float, two
   products kept in floats on the way */
static void decor_matrix(float m[3][3], const float ang[3])
{
    double c[3], s[3];
    float scsb, ccsb;
    int j;
    for (j = 0; j < 3; j++) {
        int i = (int)(ang[j] * 2607.594587618272);        /* 0x4d00d0 */
        c[j] = g_cos[i & 0x3fff];
        s[j] = g_cos[(i - 0x1000) & 0x3fff];
    }
    scsb = (float)(s[2] * s[1]);
    ccsb = (float)(c[2] * s[1]);
    m[0][0] = (float)(c[2] * c[1] + scsb * s[0]);
    m[0][1] = (float)(s[2] * c[0]);
    m[0][2] = (float)(s[2] * s[0] * c[1] - ccsb);
    m[1][0] = (float)(ccsb * s[0] - s[2] * c[1]);
    m[1][1] = (float)(c[2] * c[0]);
    m[1][2] = (float)(c[2] * s[0] * c[1] + scsb);
    m[2][0] = (float)(s[1] * c[0]);
    m[2][1] = (float)-s[0];
    m[2][2] = (float)(c[0] * c[1]);
}

static unsigned char *g_drawn;  /* each decor object once a frame */
static int g_ndrawn;

static void section_decor(int si)
{
    int i;
    for (i = 0; i < g_land.sec[si].ndecor; i++) {
        int di = g_land.sec[si].decor[i];
        const Decor *o;
        float m[3][3];
        if (di >= g_ndrawn || g_drawn[di])
            continue;
        g_drawn[di] = 1;
        o = &g_land.decor[di];
        if (o->lib < 0 || strncmp(g_land.lib[o->lib].name, "CAMERA", 6) == 0)
            continue;                           /* 0x473240 hides the cameras */
        decor_matrix(m, o->ang);
        r3d_object_m(&g_land.lib[o->lib].m[0], (const float (*)[3])m, o->pos);
    }
}

/* 0x48b370, one record: the clock run on and wrapped by fprem (the sum of a period and a
   wait at kind 8, one period more than the frames at 7); what it gives goes to the decor
   object's angle +0x14 + sub x 4, its place +0x20 or its near model's points +4 */
static void anim_step(DecorAnim *a, float dt)
{
    Decor *o = &g_land.decor[a->decor];
    float x;
    int k;
    a->t += dt;
    switch (a->type) {
    case 1:
    case 4:
        a->t = fmodf(a->t, a->period);
        x = a->t / a->period * 2.0f;
        if (x > 1.0f)
            x = (float)(2.0 - x);
        if (a->type == 1) {
            x = (a->b - a->a) * x + a->a;
            break;
        }
        for (k = 0; k < 3; k++)
            o->pos[k] = (a->to[k] - a->from[k]) * x + a->from[k];
        return;
    case 2:
        a->t = fmodf(a->t, a->period);
        x = (float)(((double)a->a + a->b
                     + sin((double)a->t / a->period * 6.2831854820251465 + a->c)
                       * ((double)a->b - a->a)) * 0.5f);
        break;
    case 3:
    case 5:
        a->t = fmodf(a->t, a->period);
        x = a->t / a->period * a->a + a->c;
        break;
    case 7: {
        LandLib *l = o->lib >= 0 ? &g_land.lib[o->lib] : 0;
        int n = a->nframes, i = 0;
        a->t = fmodf(a->t, (float)(n + 1) * a->period);
        if (a->period > 0.0f)
            i = (int)(a->t / a->period);
        /* (frame 0 is the medium model's points: the rest pose, as many as the near's) */
        if (l && i == 0 && l->m[1].nv >= l->m[0].nv)
            l->m[0].v = l->m[1].v;
        else if (l && i > 0 && i <= n && a->npoints >= l->m[0].nv)
            l->m[0].v = a->frame[i - 1];
        return;
    }
    case 8:
        a->t = fmodf(a->t, a->c + a->period);
        x = a->t > a->period ? 1.0f : a->t / a->period;
        for (k = 0; k < 3; k++)
            o->pos[k] = (a->to[k] - a->from[k]) * x + a->from[k];
        return;
    default:            /* (the game stops with "BAD AnimDecor type"; no track has one) */
        return;
    }
    if (a->sub >= 0 && a->sub < 3)
        o->ang[a->sub] = x;
}

void land_anim_step(float dt)
{
    int i;
    for (i = 0; i < g_land.nanim; i++)
        if (g_land.anim[i].decor < g_land.ndecor)
            anim_step(&g_land.anim[i], dt);
}

/* 0x40ce60's list of sections from where the camera is: is section other on it? */
int land_section_seen(int sec, const float cm[3][3], int other)
{
    const Section *s;
    double d;
    int ahead, behind, n = g_land.nsec, k;
    if (n <= 0 || sec < 0 || sec >= n)
        return 0;
    s = &g_land.sec[sec];
    d = (double)cm[0][2] * s->plane[0][0] + (double)cm[1][2] * s->plane[0][1]
        + (double)cm[2][2] * s->plane[0][2];
    if (d >= 0.25)
        ahead = BLOCKS_AHEAD, behind = 1;
    else if (d > -0.25)
        ahead = behind = BLOCKS_AHEAD / 2;
    else
        ahead = 1, behind = BLOCKS_AHEAD;
    for (k = 0; k <= 4 * ahead + 4; k++)
        if ((sec + k) % n == other)
            return 1;
    for (k = 1; k <= 4 * behind + 4; k++)
        if (((sec - k) % n + n) % n == other)
            return 1;
    return 0;
}

void land_draw(int sec, const float cm[3][3], const float pos[3])
{
    static const float one[3][3] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };
    static const float zero[3] = { 0, 0, 0 };
    const Section *s;
    double d;
    int ahead, behind, nb = g_land.nblock, n = g_land.nsec, b0, k;
    if (n <= 0 || sec < 0 || sec >= n)
        return;
    r3d_set_camera(cm, pos);
    s = &g_land.sec[sec];
    d = (double)cm[0][2] * s->plane[0][0] + (double)cm[1][2] * s->plane[0][1]
        + (double)cm[2][2] * s->plane[0][2];
    if (d >= 0.25)                              /* looking along the road (0x4d02f0) */
        ahead = BLOCKS_AHEAD, behind = 1;
    else if (d > -0.25)                         /* sideways (0x4d02f8) */
        ahead = behind = BLOCKS_AHEAD / 2;
    else
        ahead = 1, behind = BLOCKS_AHEAD;
    b0 = s->block;
    r3d_object_m(&g_land.block[b0], one, zero);
    for (k = 1; k <= ahead; k++)
        r3d_object_m(&g_land.block[(b0 + k) % nb], one, zero);
    for (k = 1; k <= behind; k++)
        r3d_object_m(&g_land.block[((b0 - k) % nb + nb) % nb], one, zero);
    if (g_detail == 0)                          /* no decor at the lowest detail */
        return;
    if (g_ndrawn < g_land.ndecor) {
        free(g_drawn);
        g_ndrawn = 0;
        if (!(g_drawn = calloc((size_t)g_land.ndecor, 1)))
            return;
        g_ndrawn = g_land.ndecor;
    }
    memset(g_drawn, 0, (size_t)g_ndrawn);
    /* the decor of the sections sec .. sec + 4 ahead + 4, then sec - 1 .. sec - 4 behind - 4 */
    for (k = 0; k <= 4 * ahead + 4; k++)
        section_decor((sec + k) % n);
    for (k = 1; k <= 4 * behind + 4; k++)
        section_decor(((sec - k) % n + n) % n);
}
