/* Sound (NOTES §4.10). The effects: SFX.BKF's enciphered WAV files, loaded by id from the
   game's tables, each with 4 more copies (0x4415a0), played, stopped and turned as
   DirectSound buffers (0x441a40..0x441fa0) and mixed here. The queues of delayed sounds
   (0x404b30). The music: the CD's audio tracks as GOG's winmm.dll plays them, NN.wav
   beside game.EXE, else as files of the player's own in music/ (Ogg Vorbis, MP3 or WAV), else
   from the 1997 CD's image; read ahead a piece a picture. */
#include "sound.h"
#include "clock.h"
#include "config.h"
#include "cdaudio.h"
#include "exe.h"
#include "files.h"
#include <SDL.h>
#define STB_VORBIS_HEADER_ONLY
#include "third_party/stb_vorbis.c"
#include "third_party/dr_mp3.h"
#include "third_party/dr_wav.h"
#ifdef MISTER_HYBRID
#include <mister_hybrid.h>
#endif
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#ifdef MISTER_HYBRID
#include <pthread.h>
#include <sched.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif

#ifdef OMR_ESP32
/* the board: one speaker through the ES8311 at 22050 Hz, the music as files at that rate */
#define RATE 22050
#define MUSIC_RING 16384        /* frames: about 0.75 s */
#define MUSIC_AHEAD 8192        /* kept queued: 0.37 s */
#else
#define RATE 44100
#define MUSIC_RING 65536        /* frames: about 1.5 s */
#define MUSIC_AHEAD 22050       /* kept queued: half a second */
#endif
#define SLOTS 1024              /* CfxServer +0x30 */
#define MIX_BLOCK 256           /* frames mixed at a time */

#ifdef OMR_ESP32
/* the board: the game's own 8-bit set of its sounds (its option for slow PCs), kept 8-bit: the
   race's 16-bit ones were 2.3 MB of its 8 (NOTES §4.26) */
typedef int8_t pcm_t;
#define PCM_SCALE 256.0f
#define SFX_SET "8_Bits"
#else
typedef int16_t pcm_t;
#define PCM_SCALE 1.0f
#define SFX_SET "16_Bits"
#endif

typedef struct {
    pcm_t *pcm;                 /* mono */
    unsigned n;                 /* samples */
    int rate;
} Sample;

typedef struct {                /* a DirectSound buffer: a sample object (0x30 bytes) */
    Sample *s;                  /* 0: a free slot */
    int next;                   /* +4: the next copy, -1 none */
    int copy;                   /* +0x28: shares its data with the head */
    int playing, loop;
    uint64_t pos;               /* in samples, 32.32 */
    int freq;                   /* what is heard */
    int hz;                     /* +0x14: as last set (the length's) */
    float gl, gr;               /* volume and pan as gains */
    int vol, pan;
    int paused;                 /* +0x24: stopped by the pause-all, to play again */
} Buffer;

static SDL_AudioDeviceID g_dev;
static SDL_mutex *g_lock;           /* what the mixer reads */
static FILE *g_wav;
static unsigned long g_wav_frames, g_frames;
static int g_ok;
static int g_fx_silent;             /* another program has the focus */
static int g_no_effects;            /* -NoSound (0x4dc1a8): the mixing goes on for the music */

static Buffer g_slot[SLOTS];
static int g_top;               /* +0x1030 */
static int g_handle[256];       /* 0x519ff8 */
static int g_menu_loaded;       /* 0x503554 */
static int g_race_set;          /* the race's set is loaded (0x504874: its voice table) */

/* the switches (NOTES §4.10.6): effects 0x4db664, voices 0x4db668, CD audio 0x4db66c */
static int g_effects_on = 1, g_voices_on = 1, g_cd_on = 1;
static int g_last_voice = -1, g_last_effect = -1;   /* 0x502898, 0x502710 */
static float g_fx_gain = 1.0f, g_mus_gain = 1.0f;   /* the mixer's lines (config.c) */

/* the music */
static FILE *g_mus;
static long g_mus_data, g_mus_len, g_mus_pos;        /* bytes: where the samples start */
/* or a file of music/, decoded: its rate made the mixer's by a line between two frames */
enum { MUS_NONE, MUS_RAW, MUS_OGG, MUS_MP3, MUS_WAV };
static int g_mus_kind;                              /* MUS_RAW: g_mus, 44.1 kHz 16-bit stereo */
static stb_vorbis *g_ogg;
static drmp3 g_mp3;
static drwav g_dwav;
static int g_dec_ch, g_dec_rate;
static int16_t g_dec[2048][2];                      /* decoded, as two sides, at its own rate */
static unsigned g_dec_have, g_dec_at;
static int16_t g_res_a[2], g_res_b[2];              /* the two frames the output lies between */
static double g_res_t, g_res_step;
static int g_mus_track = -1, g_mus_playing;
static int16_t g_ring[MUSIC_RING][2];
static unsigned g_ring_r, g_ring_w;                 /* frames, read and written */
static int g_mus_paused;                            /* the music holds its place (0x4b1660) */
static int g_mus_ended, g_mus_repeat;               /* the CD's: read to the end; +0xc */
static int g_mus_mode;                              /* +0x1c */
static SDL_mutex *g_mus_lock;                       /* the file, while its thread reads it */
static SDL_Thread *g_mus_thread;
static SDL_atomic_t g_mus_quit;
static SDL_sem *g_mus_wake;                         /* a track to open, for the thread */
static int g_mus_open;                              /* that track (under g_mus_lock), 0 none */
static SDL_atomic_t g_mus_failed;                   /* the thread could not open it */

/* (the mixer holds the lock for a short block at a time, so the game never waits long for it) */
static void lock(void)
{
    if (g_lock)
        SDL_LockMutex(g_lock);
}

static void unlock(void)
{
    if (g_lock)
        SDL_UnlockMutex(g_lock);
}

static void mus_lock(void)
{
    if (g_mus_lock)
        SDL_LockMutex(g_mus_lock);
}

static void mus_unlock(void)
{
    if (g_mus_lock)
        SDL_UnlockMutex(g_mus_lock);
}

#ifdef OMR_ESP32
/* the board's readout: what is playing (loops: the engines, skids) */
void sound_report(void)
{
    int k, n = 0, loops = 0;
    for (k = 0; k < g_top; k++)
        if (g_slot[k].s && g_slot[k].playing && !g_slot[k].paused) {
            n++;
            if (g_slot[k].loop) {
                if (loops++ < 6)
                    printf("  loop %d: %u samples at %d Hz, vol %d pan %d, gains %.3f %.3f\n", k,
                           g_slot[k].s->n, g_slot[k].freq, g_slot[k].vol, g_slot[k].pan,
                           (double)g_slot[k].gl, (double)g_slot[k].gr);
            }
        }
    printf("sound: %d playing, %d loops, volumes %d %d %d\n", n, loops, g_volume[0], g_volume[1], g_volume[2]);
}
#endif

/* ------------------------------------------------------------------ the mixer */

/* one voice into the frames' sums */
static void mix_voice(Buffer *b, float *ls, float *rs, int frames)
{
    const pcm_t *pcm = b->s->pcm;
    unsigned n = b->s->n;
    uint64_t pos = b->pos, step = ((uint64_t)b->freq << 32) / RATE;
    float gl = b->gl, gr = b->gr;
    int i;
    for (i = 0; i < frames; i++) {
        unsigned at = (unsigned)(pos >> 32);
        float a, f;
        if (at >= n) {
            if (!b->loop || !n) {
                b->playing = 0;
                break;
            }
            pos %= (uint64_t)n << 32;
            at = (unsigned)(pos >> 32);
        }
        f = (float)(uint32_t)pos * (1.0f / 4294967296.0f);
        a = pcm[at];
        if (at + 1 < n)
            a += (pcm[at + 1] - a) * f;
        else if (b->loop)
            a += (pcm[0] - a) * f;
        a *= PCM_SCALE;
        ls[i] += a * gl;
        rs[i] += a * gr;
        pos += step;
    }
    b->pos = pos;
}

static void mix(int16_t *out, int frames)
{
    float ls[MIX_BLOCK], rs[MIX_BLOCK];
    float gain = g_fx_silent ? 0.0f : g_fx_gain;
    int i, k;
    while (frames > 0) {
        int n = frames < MIX_BLOCK ? frames : MIX_BLOCK;
        memset(ls, 0, sizeof(float) * (size_t)n);
        memset(rs, 0, sizeof(float) * (size_t)n);
        for (k = 0; k < g_top; k++)
            if (g_slot[k].s && g_slot[k].playing)
                mix_voice(&g_slot[k], ls, rs, n);
        for (i = 0; i < n; i++) {
            float l = ls[i] * gain, r = rs[i] * gain;
            if (!g_mus_paused && g_ring_r != g_ring_w) {
                l += g_ring[g_ring_r % MUSIC_RING][0] * g_mus_gain;
                r += g_ring[g_ring_r % MUSIC_RING][1] * g_mus_gain;
                g_ring_r++;
            }
            out[2 * i] = (int16_t)(l > 32767 ? 32767 : l < -32768 ? -32768 : l);
            out[2 * i + 1] = (int16_t)(r > 32767 ? 32767 : r < -32768 ? -32768 : r);
        }
        out += 2 * n;
        frames -= n;
    }
}

static void callback(void *user, Uint8 *stream, int len)
{
    int16_t *out = (int16_t *)stream;
    int frames = len / 4;
    (void)user;
#ifdef MISTER_HYBRID
    {   /* the second CPU also draws every other row now: the sound goes first (it takes little) */
        static int once;
        if (!once) {
            struct sched_param p;
            once = 1;
            memset(&p, 0, sizeof p);
            p.sched_priority = 1;
            pthread_setschedparam(pthread_self(), SCHED_FIFO, &p);
        }
    }
#endif
    while (frames > 0) {
        int n = frames < MIX_BLOCK ? frames : MIX_BLOCK;
        lock();
        mix(out, n);
        unlock();
        out += 2 * n;
        frames -= n;
    }
}

/* ------------------------------------------------------------------ the buffers */

static void set_gains(Buffer *b)
{
    /* 0x441d50: hundredths of a dB, (v*5 - 0x4fb) * 800 / 256; 0x441e10: pan +-4000,
       the other side turned down by it */
    int att = ((b->vol * 5 - 0x4fb) * 800) / 256, pan = (b->pan * 4000) / 128;
    float g = powf(10.0f, (float)att / 2000.0f);
    b->gl = g * (pan > 0 ? powf(10.0f, (float)-pan / 2000.0f) : 1.0f);
    b->gr = g * (pan < 0 ? powf(10.0f, (float)pan / 2000.0f) : 1.0f);
}

static int new_slot(Sample *s, int copy)
{
    int k;
    for (k = 0; k < SLOTS && g_slot[k].s; k++)
        ;
    if (k == SLOTS)
        return -1;
    memset(&g_slot[k], 0, sizeof g_slot[k]);
    g_slot[k].s = s;
    g_slot[k].next = -1;
    g_slot[k].copy = copy;
    g_slot[k].freq = g_slot[k].hz = s->rate;
    g_slot[k].vol = 255;
    set_gains(&g_slot[k]);
    if (k >= g_top)
        g_top = k + 1;
    return k;
}

static unsigned u32le(const unsigned char *p)
{
    return (unsigned)p[0] | (unsigned)p[1] << 8 | (unsigned)p[2] << 16 | (unsigned)p[3] << 24;
}

/* 0x440d00 on one block read: each byte less 4(i + 8), i from the block's start */
static void decipher(unsigned char *p, unsigned long n)
{
    unsigned long i;
    for (i = 0; i < n; i++)
        p[i] = (unsigned char)(p[i] - 4 * (i + 8));
}

/* 0x440d30: the header (0x24 bytes), the data chunk's (8) and the samples, each block
   deciphered when the file starts with 'r' */
static Sample *read_wav(const char *path)
{
    long len;
    unsigned char *d = dsi_load(path, &len), h[0x24], c[8];
    unsigned fsize, size, bits, ch, rate, i;
    long p;
    Sample *s = 0;
    if (!d || len < 0x24)
        goto out;
    memcpy(h, d, 0x24);
    if (d[0] == 'r')
        decipher(h, 0x24);
    fsize = u32le(h + 0x10);
    ch = h[0x16] | h[0x17] << 8;
    rate = u32le(h + 0x18);
    bits = h[0x22] | h[0x23] << 8;
    p = 0x24 + (long)fsize - 16;
    if (memcmp(h, "RIFF", 4) || memcmp(h + 8, "WAVEfmt ", 8) || fsize < 16 || p + 8 > len)
        goto out;
    memcpy(c, d + p, 8);
    if (d[0] == 'r')
        decipher(c, 8);
    size = u32le(c + 4);
    p += 8;
    if (memcmp(c, "data", 4) || size > (unsigned long)(len - p) || ch < 1 ||
        (bits != 8 && bits != 16) || rate < 1000)
        goto out;
    if (d[0] == 'r')
        decipher(d + p, size);
    s = calloc(1, sizeof *s);
    if (!s)
        goto out;
    s->rate = (int)rate;
    s->n = size / (bits / 8 * ch);
    s->pcm = malloc(sizeof *s->pcm * (s->n ? s->n : 1));
    if (!s->pcm) {
        free(s);
        s = 0;
        goto out;
    }
    for (i = 0; i < s->n; i++) {        /* the first channel (all the files are mono) */
        const unsigned char *q = d + p + (unsigned long)i * (bits / 8 * ch);
#ifdef OMR_ESP32
        s->pcm[i] = (pcm_t)(bits == 16 ? (int8_t)q[1] : (int8_t)(q[0] - 128));
#else
        s->pcm[i] = bits == 16 ? (int16_t)(q[0] | q[1] << 8) : (int16_t)((q[0] - 128) << 8);
#endif
    }
out:
    free(d);
    return s;
}

/* 0x4415a0: a sample and 4 copies of it, chained. The file is read with the mixer free to go on:
   a race's set takes 30 ms, which it could not wait for */
static int load(const char *path)
{
    Sample *s = read_wav(path);
    int h, k, prev;
    if (!s) {
        fprintf(stderr, "openmotoracer: the sound %s could not be read\n", path);
        return -1;
    }
    lock();
    h = prev = new_slot(s, 0);
    for (k = 0; k < 4 && prev >= 0; k++) {
        int c = new_slot(s, 1);
        g_slot[prev].next = c;
        prev = c;
    }
    unlock();
    return h;
}

static void free_chain(int h)
{
    Sample *s;
    if (h < 0 || !g_slot[h].s)
        return;
    s = g_slot[h].s;
    while (h >= 0) {
        int next = g_slot[h].next;
        memset(&g_slot[h], 0, sizeof g_slot[h]);
        h = next;
    }
    free(s->pcm);
    free(s);
}

/* 0x441a40 + 0x440ec0 */
static int play_buffer(int h, int flags)
{
    int c = h;
    if (h < 0 || !g_slot[h].s)
        return -1;
    if (flags & 2) {
        int last = h;
        for (c = h; c >= 0 && g_slot[c].playing; c = g_slot[c].next)
            last = c;
        if (c < 0) {
            c = new_slot(g_slot[h].s, 1);
            if (c < 0)
                return -1;
            g_slot[last].next = c;
        }
    }
    g_slot[c].pos = 0;
    g_slot[c].loop = flags & 1;
    g_slot[c].playing = 1;
    return c;
}

static void set_volume(int h, int v)
{
    if (h < 0 || !g_slot[h].s)
        return;
    g_slot[h].vol = v < 0 ? 0 : v > 255 ? 255 : v;
    set_gains(&g_slot[h]);
}

/* the sound tables: {char *name, int id} (NOTES §4.10.2) */
static void load_table(int which, int count, const char *folder)
{
    unsigned a = exe_addr(which);
    int k;
    for (k = 0; k < count; k++) {
        char path[256];
        unsigned id = exe_u32(a + 8 * k + 4);
        int h;
        if (id > 255)
            continue;
        snprintf(path, sizeof path, "data\\sfx\\" SFX_SET "\\%s%s.wav", folder,
                 exe_str(exe_u32(a + 8 * k)));
        h = load(path);
        g_handle[id] = h;
        lock();
        set_volume(h, 0xff);    /* 0x424030, 0x424070: full, in the middle */
        unlock();
    }
}

/* ------------------------------------------------------------------ the game's calls */

static void music_fill(unsigned want);
static int music_open(int track);

/* The music is read on its own thread: a read from the MiSTer's SD card can take several ms,
   and a file of music/ is decoded there. On the MiSTer CPU1 is shared with the second CPU's rows
   of the picture (r3d.c), at the game's nice -20, and Main_MiSTer's polling at nice 0: the music
   thread goes to nice -10, so that the rows come first and Main_MiSTer after it (the half second
   it keeps ahead covers the waits). It opens the tracks too (music_play): stb_vorbis builds its
   decoder there, and the half second it fills first took the game's picture at a race's end to
   41 ms on the MiSTer. It fills 4096 frames at a time, so that music_play() never waits long for
   the lock. */
static int music_thread(void *user)
{
    (void)user;
#ifdef MISTER_HYBRID
    MH_PinThreadToCPU1();
    {
        id_t tid = (id_t)syscall(SYS_gettid);
        if (getpriority(PRIO_PROCESS, tid) < -10)
            setpriority(PRIO_PROCESS, tid, -10);
    }
#endif
    while (!SDL_AtomicGet(&g_mus_quit)) {
        unsigned have, target;
        int more;
        mus_lock();
        if (g_mus_open) {
            if (!music_open(g_mus_open))
                SDL_AtomicSet(&g_mus_failed, 1);
            g_mus_open = 0;
        }
        lock();
        have = g_ring_w - g_ring_r;
        unlock();
        target = have + 4096 < MUSIC_AHEAD ? have + 4096 : MUSIC_AHEAD;
        music_fill(target);
        more = g_mus_kind && target < MUSIC_AHEAD;
        mus_unlock();
        if (!more)
            SDL_SemWaitTimeout(g_mus_wake, 50);
    }
    return 0;
}

int sound_init(const char *wav, int no_effects)
{
    int k;
    for (k = 0; k < 256; k++)
        g_handle[k] = -1;
    g_no_effects = no_effects;
    g_ok = 1;
    if (wav) {
        static const unsigned char head[44] = {
            'R','I','F','F', 0,0,0,0, 'W','A','V','E','f','m','t',' ', 16,0,0,0, 1,0, 2,0,
            0x44,0xac,0,0, 0x10,0xb1,2,0, 4,0, 16,0, 'd','a','t','a', 0,0,0,0
        };
        g_wav = fopen(wav, "wb");
        if (!g_wav || fwrite(head, 1, 44, g_wav) != 44) {
            fprintf(stderr, "openmotoracer: %s could not be written\n", wav);
            g_ok = 0;
        }
        return g_ok;
    }
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) == 0) {
        SDL_AudioSpec want, got;
        memset(&want, 0, sizeof want);
        want.freq = RATE;
        want.format = AUDIO_S16SYS;
        want.channels = 2;
        want.samples = 1024;
        want.callback = callback;
        g_lock = SDL_CreateMutex();
        g_dev = SDL_OpenAudioDevice(0, 0, &want, &got, 0);
        if (g_dev) {
            SDL_PauseAudioDevice(g_dev, 0);
            g_mus_lock = SDL_CreateMutex();
            g_mus_wake = SDL_CreateSemaphore(0);
            g_mus_thread = SDL_CreateThread(music_thread, "music", 0);
        }
    }
    if (!g_dev)
        fprintf(stderr, "openmotoracer: no sound (%s)\n", SDL_GetError());
    return 1;
}

static void wav_close(void)
{
    unsigned char n[4];
    unsigned long bytes = g_wav_frames * 4;
    int k;
    for (k = 0; k < 4; k++)
        n[k] = (unsigned char)((bytes + 36) >> 8 * k);
    fseek(g_wav, 4, SEEK_SET);
    fwrite(n, 1, 4, g_wav);
    for (k = 0; k < 4; k++)
        n[k] = (unsigned char)(bytes >> 8 * k);
    fseek(g_wav, 40, SEEK_SET);
    fwrite(n, 1, 4, g_wav);
    fclose(g_wav);
    g_wav = 0;
}

void sound_quit(void)
{
    int k;
    if (g_mus_thread) {
        SDL_AtomicSet(&g_mus_quit, 1);
        SDL_SemPost(g_mus_wake);
        SDL_WaitThread(g_mus_thread, 0);
        g_mus_thread = 0;
    }
    if (g_dev)
        SDL_CloseAudioDevice(g_dev);
    g_dev = 0;
    if (g_wav)
        wav_close();
    music_stop();
    for (k = 0; k < g_top; k++)
        if (g_slot[k].s && !g_slot[k].copy)
            free_chain(k);
}

void sound_menu_load(void)
{
    if (!sound_running())
        return;
    if (g_handle[0x29] < 0)     /* (the game frees them before a race) */
        load_table(X_SFX_MENU, 7, "Menu\\");
    if (g_voices_on && !g_menu_loaded) {
        load_table(X_SFX_MENU_VOICES, 11, "Voices\\Menu\\");
        g_menu_loaded = 1;
    }
}

/* every sound of a set freed and its ids forgotten (0x4042c0 for the menu's, the race's
   at its end); 0x4044c0 starts by forgetting every id */
static void free_all(void)
{
    int k;
    sound_stop_all();
    lock();
    for (k = 0; k < g_top; k++)
        if (g_slot[k].s && !g_slot[k].copy)
            free_chain(k);
    for (k = 0; k < 256; k++)
        g_handle[k] = -1;
    g_menu_loaded = 0;
    unlock();
}

/* 0x4044c0 (from 0x410d70 at the race's load): the bikes' engines (KTM for a dirt family,
   else Ninja), the effects, (the track's ambience from Sound.Spt: not yet, 0x4059a0) and
   the voices if they are on */
void sound_race_load(int ktm)
{
    if (!sound_running())
        return;
    free_all();
    if (ktm)
        load_table(X_SFX_KTM, 16, "Ktm\\");
    else
        load_table(X_SFX_NINJA, 11, "Ninja\\");
    load_table(X_SFX_DIVERS, 18, "Divers\\");
    if (g_voices_on)
        load_table(X_SFX_VOICES, 30, "Voices\\");
    g_race_set = 1;
}

/* the race's end: its set freed; the menu loads its own as it comes back */
void sound_podium_load(void)
{
    if (!sound_running())
        return;
    if (g_handle[4] < 0)
        load_table(X_SFX_PODIUM, 2, "Menu\\");
}

void sound_podium_free(void)
{
    int i;
    if (!sound_running())
        return;
    lock();
    for (i = 4; i <= 5; i++)
        if (g_handle[i] >= 0) {
            free_chain(g_handle[i]);
            g_handle[i] = -1;
        }
    unlock();
}

void sound_race_free(void)
{
    if (!sound_running())
        return;
    free_all();
    g_race_set = 0;
}

int sound_running(void)
{
    return g_ok && !g_no_effects;
}

int sound_mixing(void)
{
    return g_ok;
}

void sound_set_switches(int effects, int voices, int music)
{
    g_effects_on = effects != 0;
    g_voices_on = voices != 0;
    g_cd_on = music != 0;
}

void sound_get_switches(int *effects, int *voices, int *music)
{
    *effects = g_effects_on;
    *voices = g_voices_on;
    *music = g_cd_on;
}

/* 0x408b30: off stops every sound */
void sound_toggle_effects(void)
{
    g_effects_on ^= 1;
    if (!g_effects_on)
        sound_stop_all();
}

/* 0x408f30: on loads the voices of the set in use if they were left out */
void sound_toggle_voices(void)
{
    g_voices_on ^= 1;
    if (!g_voices_on)
        return;
    if (!g_race_set)
        sound_menu_load();
    else if (sound_running() && g_handle[0x30] < 0)
        load_table(X_SFX_VOICES, 30, "Voices\\");
}

/* 0x409410: off stops the music, on plays the track again */
void music_toggle(void)
{
    g_cd_on ^= 1;
    if (!g_cd_on)
        music_stop();
    else if (g_mus_track >= 0)
        music_play(g_mus_track, g_mus_mode);
}

/* the mixer's lines as gains: master x wave for the effects, master x CD for the music
   (the original set Windows' own volumes) */
void sound_set_volumes(const int v[3])
{
    float m = v[0] / 32.0f;
    lock();
    g_fx_gain = m * v[2] / 32.0f;
    g_mus_gain = m * v[1] / 32.0f;
    unlock();
}

int sound_handle(int id)
{
    return id >= 0 && id < 256 ? g_handle[id] : -1;
}

/* 0x4049f0: 0x2f..0x5c are voices, the rest effects, each class with its switch, its
   volume (0x4db654 = 0xff, 0x4db658 = 0xee) and the last one played */
static int play(int id, int flags, int not_twice)
{
    int h;
    if (id >= 0x2f && id <= 0x5c) {
        if (!g_voices_on)
            return -1;
        if (not_twice && (flags & 4) && g_last_voice == id)
            return -1;
        g_last_voice = id;
        lock();
        h = play_buffer(g_handle[id], flags);
        set_volume(h, 0xff);
        unlock();
        return h;
    }
    if (!g_effects_on)
        return -1;
    if (not_twice && (flags & 4) && g_last_effect == id)
        return -1;
    if (id < 0 || id >= 256)
        return -1;
    g_last_effect = id;
    lock();
    h = play_buffer(g_handle[id], flags);
    set_volume(h, 0xee);
    unlock();
    return h;
}

int sound_play(int id, int flags)
{
    return play(id, flags, 1);
}

void sound_stop(int h)
{
    lock();
    if (h >= 0 && h < SLOTS && g_slot[h].s)
        g_slot[h].playing = g_slot[h].paused = 0;
    unlock();
}

void sound_stop_all(void)
{
    int k;
    lock();
    for (k = 0; k < g_top; k++)
        g_slot[k].playing = g_slot[k].paused = 0;
    unlock();
}

/* ------------------------------------------------------------------ the server's own calls */

/* 0x423fd0 on the id's sample (0x519ff8[id]): no switch, no volume set, no "last" kept;
   the copy playing, -1 none */
int sound_server_play(int id, int flags)
{
    int h;
    if (id < 0 || id > 255)
        return -1;
    lock();
    h = play_buffer(g_handle[id], flags);
    unlock();
    return h;
}

void sound_volume(int h, int v)         /* 0x424030 */
{
    lock();
    set_volume(h, v);
    unlock();
}

void sound_pan(int h, int p)            /* 0x424070 → 0x441e10 */
{
    if (h < 0 || h >= SLOTS || !g_slot[h].s)
        return;
    lock();
    g_slot[h].pan = p < -128 ? -128 : p > 128 ? 128 : p;
    set_gains(&g_slot[h]);
    unlock();
}

/* 0x424050 → 0x441dc0: kept always, heard only between 1000 and 44100 Hz */
void sound_frequency(int h, int hz)
{
    if (h < 0 || h >= SLOTS || !g_slot[h].s)
        return;
    lock();
    if (hz > 1000 && hz < 44100)
        g_slot[h].freq = hz;
    g_slot[h].hz = hz;
    unlock();
}

/* 0x424090 → 0x441e80: 1 stops every looping copy that plays and marks it, 0 plays the
   marked ones again from their start */
void sound_pause_all(int on)
{
    int k;
    lock();
    for (k = 0; k < g_top; k++) {
        Buffer *b = &g_slot[k];
        if (!b->s)
            continue;
        if (on && b->playing && b->loop) {
            b->playing = 0;
            b->paused = 1;
        } else if (!on && b->paused) {
            b->paused = 0;
            b->pos = 0;
            b->playing = 1;
        }
    }
    unlock();
}

int sound_last_voice(void)              /* 0x502898 */
{
    return g_last_voice;
}

void sound_set_last_voice(int id)
{
    g_last_voice = id;
}

int sound_last_effect(void)             /* 0x502710 */
{
    return g_last_effect;
}

void sound_set_last_effect(int id)
{
    g_last_effect = id;
}

/* 0x441fa0: in whole samples, then ms at the buffer's frequency */
unsigned sound_length(int h)
{
    if (h < 0 || h >= SLOTS || !g_slot[h].s || g_slot[h].hz <= 0)
        return 0;
    return g_slot[h].s->n * 1000u / (unsigned)g_slot[h].hz;
}

/* ------------------------------------------------------------------ the queues */

typedef struct {                /* 0x64 bytes */
    int mode, idle, state, i, count, h;
    unsigned start, played, len;
    int ids[8], delays[8];
} Queue;

static Queue g_queue[8];        /* 0x504240 */
static Queue g_vqueue = { .mode = 1, .idle = 1 };   /* 0x5028c8: one after the other */

void sound_queue_reset(void)
{
    int k;
    for (k = 0; k <= 8; k++) {
        Queue *q = k < 8 ? &g_queue[k] : &g_vqueue;
        q->idle = 1;
        q->state = q->i = q->count = 0;
    }
}

void sound_queue(int n, ...)
{
    va_list ap;
    int k;
    Queue *q = 0;
    for (k = 0; k < 8 && !q; k++)
        if (g_queue[k].idle)
            q = &g_queue[k];
    if (!q)
        return;
    memset(q, 0, sizeof *q);
    va_start(ap, n);
    for (k = 0; k < n && k < 8; k++) {
        q->delays[k] = va_arg(ap, int);
        q->ids[k] = va_arg(ap, int);
    }
    va_end(ap);
    q->count = k;
}

void sound_queue_voice(int id, int delay)
{
    Queue *q = &g_vqueue;
    if (!q->idle && ((q->count + 1) & 7) == q->i)
        return;                 /* full */
    q->ids[q->count] = id;
    q->delays[q->count] = delay;
    q->count = (q->count + 1) & 7;
    q->idle = 0;
}

/* 0x404b30: one step of a queue a call */
static void queue_step(Queue *q)
{
    if (q->idle)
        return;
    switch (q->state) {
    case 0:
        q->start = g_now - sound_length(sound_handle(q->ids[q->i]));
        q->state = 1;
        break;
    case 1:
        if ((int)(g_now - q->start) >= q->delays[q->i] || q->mode) {
            q->h = play(q->ids[q->i], 0, 0);
            q->played = g_now;
            q->len = q->h >= 0 ? sound_length(q->h) : 0;
            q->state = 2;
        }
        break;
    case 2:
        if (q->mode && (int)(g_now - q->played) < q->delays[q->i] + (int)q->len)
            break;
        q->i = (q->i + 1) & 7;
        if (q->i == q->count)
            q->idle = 1;
        q->state = 0;           /* (also when idle: the next voice queued starts afresh) */
        break;
    }
}

void sound_update(void)
{
    int k;
    for (k = 0; k < 8; k++)
        queue_step(&g_queue[k]);
    queue_step(&g_vqueue);
}

/* ------------------------------------------------------------------ the music */

static void music_close(void)
{
    if (g_mus)
        fclose(g_mus);
    g_mus = 0;
    if (g_mus_kind == MUS_OGG)
        stb_vorbis_close(g_ogg);
    else if (g_mus_kind == MUS_MP3)
        drmp3_uninit(&g_mp3);
    else if (g_mus_kind == MUS_WAV)
        drwav_uninit(&g_dwav);
    g_ogg = 0;
    g_mus_kind = MUS_NONE;
}

/* The CD's track n as a file of the player's own, made from the disc: music/trackNN, track_NN
   or NN (any case), .ogg, .mp3 or .wav, in the game's folder's music/, else in the game's
   folder itself (where GOG's NN.wav are found first). */
static int music_find_file(int track, char *path, size_t size)
{
    static const char *const forms[] = { "track%02d.%s", "track_%02d.%s", "track %02d.%s", "%02d.%s" };
    static const char *const kinds[] = { "ogg", "mp3", "wav" };
    char name[64], rel[96];
    int d, f, k;
    for (d = 0; d < 2; d++)
        for (f = 0; f < 4; f++)
            for (k = 0; k < 3; k++) {
                snprintf(name, sizeof name, forms[f], track, kinds[k]);
                snprintf(rel, sizeof rel, "%s%s", d ? "" : "music/", name);
                if (files_find(rel, path, size))
                    return 1;
            }
    return 0;
}

static int music_open_file(const char *path)
{
    const char *dot = strrchr(path, '.');
    int err = 0;
    if (!dot)
        return 0;
    if (!strcasecmp(dot, ".ogg")) {
        stb_vorbis_info in;
        if (!(g_ogg = stb_vorbis_open_filename(path, &err, NULL)))
            return 0;
        in = stb_vorbis_get_info(g_ogg);
        g_mus_kind = MUS_OGG;
        g_dec_ch = in.channels;
        g_dec_rate = (int)in.sample_rate;
    } else if (!strcasecmp(dot, ".mp3")) {
        if (!drmp3_init_file(&g_mp3, path, NULL))
            return 0;
        g_mus_kind = MUS_MP3;
        g_dec_ch = (int)g_mp3.channels;
        g_dec_rate = (int)g_mp3.sampleRate;
    } else {
        if (!drwav_init_file(&g_dwav, path, NULL))
            return 0;
        g_mus_kind = MUS_WAV;
        g_dec_ch = (int)g_dwav.channels;
        g_dec_rate = (int)g_dwav.sampleRate;
    }
    if (g_dec_ch < 1 || g_dec_rate < 8000 || g_dec_rate > 192000) {
        fprintf(stderr, "openmotoracer: %s: %d channels at %d Hz cannot be played\n", path,
                g_dec_ch, g_dec_rate);
        music_close();
        return 0;
    }
    g_dec_have = g_dec_at = 0;
    g_res_step = (double)g_dec_rate / RATE;
    g_res_t = 2.0;              /* (the first two frames are read before the first is given) */
    return 1;
}

/* GOG's winmm.dll: MCI_PLAY of track n is PlaySound("0n.wav" or "n.wav", SND_LOOP), so
   every track goes round and round (NOTES §4.10.5); a copy without them has the music in
   the CD's image, or as files the player made from it (music/). Where the music comes from
   is what the player has; how it is played, the Game Behaviour setting. */
static int music_open(int track)
{
    char name[16], path[1024];
    unsigned char h[12], c[8];
    music_close();
    snprintf(name, sizeof name, track < 10 ? "0%d.wav" : "%d.wav", track);
    if (!files_find(name, path, sizeof path)) {
        if (music_find_file(track, path, sizeof path))
            return music_open_file(path);
        if (!cdaudio_track(track, path, sizeof path, &g_mus_data, &g_mus_len))
            return 0;
        if (!(g_mus = fopen(path, "rb")) || fseek(g_mus, g_mus_data, SEEK_SET)) {
            music_close();
            return 0;
        }
        g_mus_kind = MUS_RAW;
        g_mus_len &= ~3L;
        g_mus_pos = 0;
        return 1;
    }
    if (!(g_mus = fopen(path, "rb")))
        return 0;
    g_mus_kind = MUS_RAW;
    if (fread(h, 1, 12, g_mus) != 12 || memcmp(h, "RIFF", 4) || memcmp(h + 8, "WAVE", 4))
        goto bad;
    for (;;) {                  /* the chunks, to "data"; "fmt " is 44.1 kHz 16-bit stereo */
        unsigned n;
        if (fread(c, 1, 8, g_mus) != 8)
            goto bad;
        n = u32le(c + 4);
        if (!memcmp(c, "fmt ", 4)) {
            unsigned char f[16];
            if (n < 16 || fread(f, 1, 16, g_mus) != 16)
                goto bad;
            if ((f[0] | f[1] << 8) != 1 || (f[2] | f[3] << 8) != 2 || u32le(f + 4) != RATE ||
                (f[14] | f[15] << 8) != 16) {
                fprintf(stderr, "openmotoracer: %s is not 44.1 kHz 16-bit stereo\n", name);
                goto bad;
            }
            n -= 16;
        } else if (!memcmp(c, "data", 4)) {
            g_mus_data = ftell(g_mus);
            g_mus_len = (long)(n & ~3u);
            g_mus_pos = 0;
            return 1;
        }
        if (fseek(g_mus, (long)(n + (n & 1)), SEEK_CUR))
            goto bad;
    }
bad:
    music_close();
    return 0;
}

/* up to n frames of the file at its own rate, as two sides; 0 at its end */
static unsigned music_decode(int16_t (*out)[2], unsigned n)
{
    static int16_t raw[2048 * 8];
    unsigned got = 0, i, ch = g_dec_ch > 8 ? 8 : (unsigned)g_dec_ch;
    if (n > 2048)
        n = 2048;
    if (g_mus_kind == MUS_OGG) {    /* (stb_vorbis gives the sides it is asked for) */
        int k = ch >= 2 ? 2 : 1;
        int r = stb_vorbis_get_samples_short_interleaved(g_ogg, k, raw, (int)n * k);
        got = r > 0 ? (unsigned)r : 0;
        for (i = 0; i < got; i++) {
            out[i][0] = raw[k * i];
            out[i][1] = raw[k * i + k - 1];
        }
        return got;
    }
    got = g_mus_kind == MUS_MP3 ? (unsigned)drmp3_read_pcm_frames_s16(&g_mp3, n, raw)
                                : (unsigned)drwav_read_pcm_frames_s16(&g_dwav, n, raw);
    for (i = 0; i < got; i++) {
        out[i][0] = raw[i * ch];
        out[i][1] = raw[i * ch + (ch >= 2 ? 1 : 0)];
    }
    return got;
}

/* up to n frames at the mixer's rate, as two sides; 0 at the track's end */
static unsigned music_read(int16_t (*out)[2], unsigned n)
{
    unsigned i;
    if (g_mus_kind == MUS_RAW) {
        unsigned char buf[4096];
        size_t got;
        if (g_mus_pos >= g_mus_len)
            return 0;
        if (n > sizeof buf / 4)
            n = sizeof buf / 4;
        if ((long)n * 4 > g_mus_len - g_mus_pos)
            n = (unsigned)((g_mus_len - g_mus_pos) / 4);
        got = fread(buf, 4, n, g_mus);
        g_mus_pos += (long)got * 4;
        if (!got)
            g_mus_pos = g_mus_len;
        for (i = 0; i < got; i++) {
            out[i][0] = (int16_t)(buf[4 * i] | buf[4 * i + 1] << 8);
            out[i][1] = (int16_t)(buf[4 * i + 2] | buf[4 * i + 3] << 8);
        }
        return (unsigned)got;
    }
    if (g_dec_rate == RATE)
        return music_decode(out, n);
    for (i = 0; i < n; i++) {
        while (g_res_t >= 1.0) {
            if (g_dec_at >= g_dec_have) {
                g_dec_have = music_decode(g_dec, 2048);
                g_dec_at = 0;
                if (!g_dec_have)
                    return i;
            }
            g_res_a[0] = g_res_b[0];
            g_res_a[1] = g_res_b[1];
            g_res_b[0] = g_dec[g_dec_at][0];
            g_res_b[1] = g_dec[g_dec_at][1];
            g_dec_at++;
            g_res_t -= 1.0;
        }
        out[i][0] = (int16_t)(g_res_a[0] + (g_res_b[0] - g_res_a[0]) * g_res_t);
        out[i][1] = (int16_t)(g_res_a[1] + (g_res_b[1] - g_res_a[1]) * g_res_t);
        g_res_t += g_res_step;
    }
    return n;
}

/* back to the track's start (GOG's SND_LOOP); 0 if it can't be */
static int music_rewind(void)
{
    switch (g_mus_kind) {
    case MUS_RAW:
        g_mus_pos = 0;
        return fseek(g_mus, g_mus_data, SEEK_SET) == 0;
    case MUS_OGG:
        if (!stb_vorbis_seek_start(g_ogg))
            return 0;
        break;
    case MUS_MP3:
        if (!drmp3_seek_to_pcm_frame(&g_mp3, 0))
            return 0;
        break;
    case MUS_WAV:
        if (!drwav_seek_to_pcm_frame(&g_dwav, 0))
            return 0;
        break;
    default:
        return 0;
    }
    g_dec_have = g_dec_at = 0;
    g_res_t = 2.0;
    return 1;
}

static void music_fill(unsigned want)
{
    int16_t buf[1024][2];
    while (g_mus_kind && g_ring_w - g_ring_r < want) {
        unsigned n = want - (g_ring_w - g_ring_r), i, got;
        if (n > 1024)
            n = 1024;
        got = music_read(buf, n);
        if (!got) {
            if (behaviour_cd()) {               /* a real disc comes to the track's end */
                music_close();
                g_mus_ended = 1;
                break;
            }
            if (!music_rewind() || !(got = music_read(buf, n))) {   /* GOG's SND_LOOP */
                music_close();
                break;
            }
        }
        lock();
        for (i = 0; i < got; i++) {
            g_ring[g_ring_w % MUSIC_RING][0] = buf[i][0];
            g_ring[g_ring_w % MUSIC_RING][1] = buf[i][1];
            g_ring_w++;
        }
        unlock();
    }
}

/* 0x404990 → 0x4b18e0 (choose) and 0x4b19f0 (play with -1): the track asked for is chosen
   whatever follows, and modes 1 and 3 leave going whatever plays, so the podium's runs on into the
   menus. (Track 0, a random one, is asked for by no level.) */
void music_play(int track, int mode)
{
    if (track < 2 || track > 13)
        return;
    g_mus_track = track;
    g_mus_mode = mode;          /* +0x1c */
    if (!g_cd_on || ((mode == 1 || mode == 3) && g_mus_playing))
        return;
    mus_lock();
    lock();
    g_ring_r = g_ring_w = 0;
    g_mus_paused = 0;
    unlock();
    g_mus_ended = 0;
    g_mus_repeat = mode == 2 || mode == 3;      /* +0xc */
    if (g_mus_thread) {         /* opened and filled there; a track it cannot open: music_timer */
        music_close();
        g_mus_open = track;
        SDL_AtomicSet(&g_mus_failed, 0);
        g_mus_playing = 1;
        mus_unlock();
        SDL_SemPost(g_mus_wake);
        return;
    }
    g_mus_playing = music_open(track);
    if (!g_mus_playing)
        g_mus_repeat = 0;
    music_fill(MUSIC_AHEAD);
    mus_unlock();
}

/* The CD's 1 s timer 0x4b12e0, and what MCI says of the drive: a track at its end has stopped;
   one to repeat plays again with mode 0, so twice in all, and stays stopped until a page asks
   for it. */
static void music_timer(void)
{
    static unsigned next;
    if (SDL_AtomicGet(&g_mus_failed)) {         /* (as music_play() without the thread) */
        mus_lock();
        if (SDL_AtomicGet(&g_mus_failed) && !g_mus_open) {
            SDL_AtomicSet(&g_mus_failed, 0);
            g_mus_playing = g_mus_repeat = 0;
        }
        mus_unlock();
    }
    if (!behaviour_cd())
        return;
    if (g_mus_ended) {
        unsigned left;
        mus_lock();
        lock();
        left = g_ring_w - g_ring_r;
        unlock();
        if (!left) {
            g_mus_ended = 0;
            g_mus_playing = 0;
        }
        mus_unlock();
    }
    if (g_now < next)
        return;
    next = g_now + 1000;
    if (g_mus_repeat && !g_mus_playing)
        music_play(g_mus_track, 0);
}

void music_stop(void)
{
    mus_lock();
    music_close();
    g_mus_open = 0;
    g_mus_playing = 0;
    g_mus_ended = g_mus_repeat = 0;
    lock();
    g_ring_r = g_ring_w = 0;
    g_mus_paused = 0;
    unlock();
    mus_unlock();
}

static void music_hold(int on)
{
    if (!g_cd_on)
        return;
    lock();
    g_mus_paused = on;
    unlock();
}

/* 0x4096e0 -> 0x4b1660: a disc holds, and goes on from there. GOG's winmm.dll never hears
   the request, so on its program the music runs straight through a pause (NOTES §4.10.5.3) */
void music_pause(int on)
{
    if (behaviour_cd())
        music_hold(on);
}

/* 0x409670, on WM_ACTIVATEAPP: the music that plays holds while another program has the focus
   (0x4db678). A pause of its own stays as it was. */
void music_focus(int gained)
{
    static int held;
    lock();
    g_fx_silent = !gained;      /* DirectSound: buffers with no GLOBALFOCUS are not heard */
    unlock();
    if (!gained && g_mus_playing && !g_mus_paused) {
        music_hold(1);
        held = 1;
    } else if (gained && held) {
        music_hold(0);
        held = 0;
    }
}

void sound_frame(void)
{
    if (!g_ok)
        return;
    if (!g_mus_thread)
        music_fill(g_wav ? RATE / 60 + 2 : MUSIC_AHEAD);
    music_timer();
    if (g_wav) {                /* exactly 44100 frames every 60 pictures */
        int16_t out[2 * (RATE / 60 + 2)];
        unsigned long n;
        g_frames++;
        n = g_frames * RATE / 60 - g_wav_frames;
        mix(out, (int)n);
        fwrite(out, 4, n, g_wav);
        g_wav_frames += n;
    }
}

int sound_effects_on(void)
{
    return g_effects_on;
}

int sound_voices_on(void)
{
    return g_voices_on;
}

/* 0x4059a0's load of one of the track's ambient samples (DIVERS\name.wav) as id, if that
   id has none yet: volume full, pan in the middle */
void sound_load_named(int id, const char *name)
{
    char path[256];
    if (!sound_running() || id < 0 || id > 255 || g_handle[id] >= 0)
        return;
    snprintf(path, sizeof path, "data\\sfx\\" SFX_SET "\\Divers\\%s.wav", name);
    g_handle[id] = load(path);
    lock();
    set_volume(g_handle[id], 0xff);
    unlock();
}

int sound_rate(int id)                  /* 0x442010: the file's own rate, 0 none */
{
    int h = sound_handle(id);
    return h >= 0 && g_slot[h].s ? g_slot[h].s->rate : 0;
}
