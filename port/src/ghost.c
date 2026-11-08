/* The time attack's ghost (NOTES §4.21c): the objects 0x52c0d8 (the ghost) and 0x52c0f0 (the
   run being recorded), the class at 0x419020, and the file 0x4136a0 / 0x4138c0. */
#include "ghost.h"
#include "choices.h"
#include "menu.h"
#include "saves.h"
#include "scores.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define MAX_SAMPLES 10000       /* 0x419020 */

typedef struct {                /* 0x34 bytes, as in the file */
    float clock;                /* +0x00: the dt units since the start */
    float speed;                /* +0x04: +0x124 */
    float at[3];                /* +0x08: +0x30 */
    float dir[3];               /* +0x14: +0xd4 + +0xe0, where the bike points */
    int turbo;                  /* +0x20: +0x2e85 */
    int crash;                  /* +0x24: +0x31ad, in the step a crash began */
    float v[3];                 /* +0x28: +0x31a1, how it was thrown */
} Sample;

typedef struct {
    int count;                  /* +0x00 */
    int at;                     /* +0x04: the sample the clock has reached */
    int max;                    /* +0x08 */
    Sample *s;                  /* +0x0c */
    float total;                /* +0x10: the race time, ms */
    float clock;                /* +0x14 */
} Recording;

#ifdef OMR_ESP32
/* the ESP32: the 1 MB taken only once a time attack runs (NOTES §4.26) */
static Recording g_ghost = { 0, 0, MAX_SAMPLES, 0, 0.0f, 0.0f };   /* 0x52c0d8 */
static Recording g_run = { 0, 0, MAX_SAMPLES, 0, 0.0f, 0.0f };     /* 0x52c0f0 */
static int store(void)
{
    if (!g_ghost.s) {
        Sample *s = calloc(2 * MAX_SAMPLES, sizeof *s);
        if (!s)
            return 0;
        g_ghost.s = s;
        g_run.s = s + MAX_SAMPLES;
    }
    return 1;
}
#else
static Sample g_store[2][MAX_SAMPLES];
static Recording g_ghost = { 0, 0, MAX_SAMPLES, g_store[0], 0.0f, 0.0f };   /* 0x52c0d8 */
static Recording g_run = { 0, 0, MAX_SAMPLES, g_store[1], 0.0f, 0.0f };     /* 0x52c0f0 */
#define store() 1
#endif

static void reset(Recording *t)     /* 0x4191a0 */
{
    t->count = t->at = 0;
    t->clock = 0.0f;
}

void ghost_reset(void)
{
    reset(&g_ghost);
    reset(&g_run);
}

void ghost_restart(void)
{
    g_ghost.at = 0;             /* 0x4191b0 */
    g_ghost.clock = 0.0f;
    reset(&g_run);
}

int ghost_count(void)
{
    return g_ghost.count;
}

float ghost_total(void)
{
    return g_ghost.total;
}

/* ------------------------------------------------------------------ the file */

/* <track folder>\timatak + R or _ + _ unless Pocket + <laps>.bin. With Pocket the game adds its
   "P" to another buffer (0x53f3c0), so the name just has one letter less, in the load, the save
   and page 9's Reset alike; the file's first words tell Pocket apart anyway. */
static void file_name(char *out, size_t size)
{
    const char *folder = g_tracks[g_track].folder;
    size_t n;
    if (strncasecmp(folder, "data\\", 5) == 0)
        folder += 5;
    n = strlen(folder);
    while (n > 0 && folder[n - 1] == '\\')
        n--;
    snprintf(out, size, "%.*s\\timatak%s%s%1d.bin", (int)n, folder, g_reverse ? "R" : "_",
             g_pocket ? "" : "_", g_laps);
}

static int read_word(FILE *f, int *v)
{
    return fread(v, 4, 1, f) == 1;
}

void ghost_load(unsigned char *record)
{
    if (!store())
        return;
    char name[300];
    FILE *f;
    int v, count = 0, max = 0, i;
    float total = 0.0f;
    file_name(name, sizeof name);
    f = save_open(name, "rb");
    if (!f)
        return;
    if (read_word(f, &v) && v == g_pocket && read_word(f, &v) && v == g_reverse
        && read_word(f, &v) && v == g_laps) {
        unsigned char r[SCORE_SIZE];
        /* (the game reads on whatever it finds; the port takes a whole file or none) */
        if (fread(r, sizeof r, 1, f) == 1 && read_word(f, &count) && read_word(f, &max)
            && fread(&total, 4, 1, f) == 1 && count > 0 && count <= MAX_SAMPLES) {
            for (i = 0; i < count; i++)
                if (fread(&g_ghost.s[i], sizeof(Sample), 1, f) != 1)
                    break;
            if (i == count) {
                memcpy(record, r, sizeof r);
                g_ghost.count = count;
                g_ghost.max = max;
                g_ghost.total = total;
            }
        }
    }
    fclose(f);
}

void ghost_save(const unsigned char *record)
{
    char name[300];
    FILE *f;
    int i;
    file_name(name, sizeof name);
    f = save_open(name, "wb");
    if (!f)
        return;
    if (fwrite(&g_pocket, 4, 1, f) == 1 && fwrite(&g_reverse, 4, 1, f) == 1
        && fwrite(&g_laps, 4, 1, f) == 1) {
        fwrite(record, SCORE_SIZE, 1, f);
        fwrite(&g_ghost.count, 4, 1, f);
        fwrite(&g_ghost.max, 4, 1, f);
        fwrite(&g_ghost.total, 4, 1, f);
        for (i = 0; i < g_ghost.count; i++)
            fwrite(&g_ghost.s[i], sizeof(Sample), 1, f);
    }
    fclose(f);
}

void ghost_forget(void)
{
    char name[300];
    file_name(name, sizeof name);
    save_forget(name);
}

/* ------------------------------------------------------------------ recording, playing */

void ghost_record(float dt, float speed, const float at[3], const float dir[3], int turbo,
                  int crash, const float v[3])
{
    Sample *s;
    if (g_run.count >= g_run.max || !store())
        return;
    g_run.clock = (float)((double)g_run.clock + dt);
    s = &g_run.s[g_run.count++];
    s->clock = g_run.clock;
    s->speed = speed;
    memcpy(s->at, at, sizeof s->at);
    memcpy(s->dir, dir, sizeof s->dir);
    s->turbo = turbo;
    s->crash = crash;
    memcpy(s->v, v, sizeof s->v);
}

void ghost_take(int ms)
{
    if (!store())
        return;
    g_run.total = (float)ms;
    memcpy(g_ghost.s, g_run.s, (size_t)g_run.count * sizeof(Sample));   /* 0x4190d0 */
    g_ghost.count = g_run.count;
    g_ghost.at = 0;
    g_ghost.max = g_run.max;
    g_ghost.total = g_run.total;
    g_ghost.clock = 0.0f;
}

void ghost_play(float dt, float *speed, float at[3], float dir[3], int *turbo, int *crash,
                float v[3])
{
    const Sample *s = g_ghost.s, *a, *b;
    int from = g_ghost.at, i;
    double t, f;
    float ft;
    *crash = 0;
    if (g_ghost.count == 0)
        return;
    g_ghost.clock = (float)((double)g_ghost.clock + dt);
    while (g_ghost.at < g_ghost.count && s[g_ghost.at].clock < g_ghost.clock)
        g_ghost.at++;
    if (g_ghost.at >= g_ghost.count)
        return;                 /* the recording has ended: everything stays */
    a = &s[g_ghost.at > 0 ? g_ghost.at - 1 : 0];
    b = &s[g_ghost.at];
    /* (the very first sample against itself: 0 / 0, which the game's test also makes 0) */
    ft = (float)(((double)g_ghost.clock - a->clock) / ((double)b->clock - a->clock));
    if (!(ft >= 0.0f))
        ft = 0.0f;
    if (ft > 1.0f)
        ft = 1.0f;
    t = ft;
    f = 1.0 - t;
    /* (in the game's order the second half is kept in a float but for x) */
    at[0] = (float)(a->at[0] * f + b->at[0] * t);
    for (i = 1; i < 3; i++)
        at[i] = (float)(a->at[i] * f + (float)(b->at[i] * t));
    for (i = 0; i < 3; i++)
        dir[i] = (float)(a->dir[i] * f + (float)(b->dir[i] * t));
    *speed = (float)(b->speed * t + a->speed * f);
    *turbo = ft < 0.5f ? a->turbo : b->turbo;
    for (i = from; i <= g_ghost.at; i++)
        if (s[i].crash) {
            *crash = 1;
            memcpy(v, s[i].v, sizeof s[i].v);
        }
}
