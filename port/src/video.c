/* The window: the 16-bit screen handed to SDL as an RGB565 texture, kept 4:3. */
#include "prof.h"
#include "video.h"
#include "mister.h"
#include <SDL.h>
#ifdef __ARM_NEON
#include <arm_neon.h>
#endif
#include <stdio.h>
#include <string.h>

uint16_t *g_screen;
int g_scr_w = 640, g_scr_h = 480, g_pitch = SCREEN_MAX_W;

#ifdef OMR_ESP32
/* (on cache lines: the board's LCD DMA sends the race's picture from here as it is) */
static uint16_t g_pixels[SCREEN_MAX_W * SCREEN_MAX_H] __attribute__((aligned(64)));
#else
static uint16_t g_pixels[SCREEN_MAX_W * SCREEN_MAX_H];
#endif
static SDL_Window *g_win;
static SDL_Renderer *g_ren;
static SDL_Texture *g_tex;
static int g_fullscreen;
static int g_surf_h = 480;      /* the surface's height: below a 16:9 picture it stays black */
int g_alt_return = 1;
int g_draw_2d = 1;
int g_no_3d;
static int g_no_3d_once;        /* 0x4e8940 */
static int g_kept = -1;         /* 0x4e8954: while another program has the focus */

int video_surface_h(void)
{
    return g_surf_h;
}

int video_init(int headless, char *err, int errsize)
{
    g_screen = g_pixels;
    if (headless)
        return 1;
#ifdef MISTER_HYBRID
    if (mister_on()) {
        /* the window is the core's screen, the surface its next picture */
        SDL_SetHint("SDL_MISTER_DIRECT", "1");
        g_win = SDL_CreateWindow("OpenMotoRacer", 0, 0, 640, 480, 0);
        if (!g_win) {
            snprintf(err, errsize, "No window: %s", SDL_GetError());
            return 0;
        }
        return 1;
    }
#endif
    g_win = SDL_CreateWindow("OpenMotoRacer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                             g_scr_w * 2, g_scr_h * 2, SDL_WINDOW_RESIZABLE);
    if (g_win)
        g_ren = SDL_CreateRenderer(g_win, -1, SDL_RENDERER_PRESENTVSYNC);
    if (g_ren) {
        SDL_RenderSetLogicalSize(g_ren, 640, 480);
        g_tex = SDL_CreateTexture(g_ren, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING,
                                  g_scr_w, g_scr_h);
    }
    if (!g_tex) {
        snprintf(err, errsize, "No window: %s", SDL_GetError());
        return 0;
    }
    return 1;
}

void video_set_size(int w, int h, int view_h)
{
    if (w == g_scr_w && h == g_surf_h && view_h == g_scr_h)
        return;
    g_scr_w = w;
    g_scr_h = view_h;
    g_surf_h = h;
#ifdef OMR_ESP32
    /* the board's race picture is its rows one after another, so the LCD's DMA can send it as
       it is; the menus keep the 640 pixels' pitch */
    g_pitch = w < SCREEN_MAX_W ? w : SCREEN_MAX_W;
#endif
    memset(g_pixels, 0, sizeof g_pixels);
}

void video_mouse(int *x, int *y)
{
    *x = *x * g_scr_w / SCREEN_MAX_W;
    *y = *y * g_surf_h / SCREEN_MAX_H;
}

#ifdef MISTER_HYBRID
static uint16_t g_small[320 * 240];
/* over a race's 3D at 640x240, the 2D's rows are taken from it only as the 2D comes to them, and
   only those go back: VIDEO_ROWS in whatever draws there */
int g_half_lazy;
static unsigned char g_half_taken[240];
#ifdef OMR_PROF
static int g_rows_check = -1;
#endif
/* where 640x240's rows are: each g_half_step pixels from g_half_to */
static uint16_t *g_half_to = g_pixels;
static int g_half_step = 2 * SCREEN_MAX_W;
/* a race's picture is drawn right in the core's next buffer, without a copy: g_direct while it is.
   The last one stays good for a frame that draws nothing, and for what comes after the race. */
static SDL_Surface *g_direct;
static int g_direct_used, g_shown_w, g_shown_h;
static const uint16_t *g_last;
static int g_last_step;

void video_set_half(int on)
{
    g_half_rows = on;
    g_screen = on ? g_small : g_pixels;
    g_pitch = on ? 320 : SCREEN_MAX_W;
}

static void shown_size(int w, int h)
{
    if (w != g_shown_w || h != g_shown_h) {
        SDL_SetWindowSize(g_win, w, h);
        g_shown_w = w, g_shown_h = h;
    }
}

void video_direct_begin(void)
{
    SDL_Surface *s;
    if (!g_win || g_direct || !mister_on())
        return;
    shown_size(g_half_rows ? 640 : g_scr_w, g_half_rows ? 240 : g_surf_h);
    s = SDL_GetWindowSurface(g_win);
    if (!s)
        return;
    g_direct = s, g_direct_used = 1;
    if (g_half_rows) {
        g_half_to = s->pixels, g_half_step = s->pitch / 2;
    } else {
        g_screen = s->pixels, g_pitch = s->pitch / 2;
    }
}

void video_direct_end(void)
{
    int y;
    if (g_direct_used && g_last && !g_half_rows)        /* (a glass transition takes it from here) */
        for (y = 0; y < g_surf_h; y++)
            memcpy(g_pixels + y * SCREEN_MAX_W, g_last + y * g_last_step, (size_t)g_scr_w * 2);
    g_direct_used = 0;
    g_last = 0;
}

static void half_take(int y)
{
    const uint16_t *from = g_half_to + y * g_half_step;
    uint16_t *to = g_small + y * 320;
    int x;
#ifdef __ARM_NEON
    for (x = 0; x < 320; x += 8)
        vst1q_u16(to + x, vld2q_u16(from + 2 * x).val[0]);
#else
    for (x = 0; x < 320; x++)
        to[x] = from[2 * x];
#endif
}

void video_half_rows(int y0, int y1)
{
    int y;
    if (y0 < 0)
        y0 = 0;
    if (y1 > 239)
        y1 = 239;
    for (y = y0; y <= y1; y++)
        if (!g_half_taken[y]) {
            g_half_taken[y] = 1;
            half_take(y);
        }
}

void video_half_3d(int on)
{
    if (on) {
        g_screen = g_half_to, g_pitch = g_half_step / 2, g_scr_w = 640, g_scr_h = 480;
        return;
    }
    memset(g_half_taken, 0, sizeof g_half_taken);
    g_half_lazy = 1;
#ifdef OMR_PROF
    if (g_rows_check < 0)
        g_rows_check = getenv("OMR_ROWS_CHECK") != 0;
    if (g_rows_check) {                 /* every row at hand, to see what draws without saying */
        int y;
        for (y = 0; y < 240; y++)
            half_take(y);
    }
#endif
    g_screen = g_small, g_pitch = 320, g_scr_w = 320, g_scr_h = 240;
}

/* what the 2D changed goes over the 3D, each pixel twice as wide */
static void half_merge_row(int y)
{
    const uint16_t *a = g_small + y * 320;
    uint16_t *o = g_half_to + y * g_half_step;
    int x;
#ifdef __ARM_NEON
    for (x = 0; x < 320; x += 8) {
        uint16x8_t d = veorq_u16(vld1q_u16(a + x), vld2q_u16(o + 2 * x).val[0]);
        uint64x2_t q = vreinterpretq_u64_u16(d);
        int i;
        if (!(vgetq_lane_u64(q, 0) | vgetq_lane_u64(q, 1)))
            continue;
        for (i = x; i < x + 8; i++)
            o[2 * i] = o[2 * i + 1] = a[i];
    }
#else
    for (x = 0; x < 320; x++)
        if (a[x] != o[2 * x])
            o[2 * x] = o[2 * x + 1] = a[x];
#endif
}

static void half_merge(void)
{
    int y;
    for (y = 0; y < 240; y++) {
#ifdef OMR_PROF
        if (g_half_lazy && !g_half_taken[y] && g_rows_check > 0) {
            const uint16_t *a = g_small + y * 320, *o = g_half_to + y * g_half_step;
            int x;
            for (x = 0; x < 320; x++)
                if (a[x] != o[2 * x]) {
                    fprintf(stderr, "rows: row %d drawn on without saying\n", y);
                    break;
                }
        }
#endif
        if (!g_half_lazy || g_half_taken[y])
            half_merge_row(y);
    }
    g_half_lazy = 0;
}

/* the picture to the core: drawn in its buffer already, or its rows copied there. The copy only
   for the menus, and for a race's frame that drew nothing (the last picture again). */
static void present_mister(void)
{
    const uint16_t *from = g_direct_used && g_last ? g_last : g_screen;
    int w = g_scr_w, h = g_surf_h, step = g_direct_used && g_last ? g_last_step : g_pitch, y;
    SDL_Surface *s;
    if (g_direct) {
        if (g_half_rows)
            half_merge();
        g_last = g_half_rows ? g_half_to : g_screen;
        g_last_step = g_half_rows ? g_half_step : g_pitch;
        SDL_UpdateWindowSurface(g_win);
        g_direct = 0;
        g_half_to = g_pixels, g_half_step = 2 * SCREEN_MAX_W;
        if (!g_half_rows)
            g_screen = g_pixels, g_pitch = SCREEN_MAX_W;
        prof_mark(MARK_PRESENT);
        mister_wait();
        return;
    }
    if (g_half_rows) {
        if (!(g_direct_used && g_last))
            half_merge();
        w = 640, h = 240;
        if (!(g_direct_used && g_last))
            from = g_pixels, step = 2 * SCREEN_MAX_W;
    }
    shown_size(w, h);
    s = SDL_GetWindowSurface(g_win);
    if (s) {
        for (y = 0; y < h; y++)
            memcpy((char *)s->pixels + y * s->pitch, from + y * step, (size_t)w * 2);
        SDL_UpdateWindowSurface(g_win);
    }
    prof_mark(MARK_PRESENT);
    mister_wait();
}
#endif

void video_present(void)
{
#ifdef MISTER_HYBRID
    if (g_win && mister_on()) {
        present_mister();
        return;
    }
#endif
    if (!g_tex)
        return;
    SDL_Rect r = { 0, 0, g_scr_w, g_surf_h };
    SDL_UpdateTexture(g_tex, &r, g_screen, g_pitch * 2);
    SDL_RenderClear(g_ren);
    SDL_RenderCopy(g_ren, g_tex, &r, 0);
    SDL_RenderPresent(g_ren);
}

/* what a player must see: a program started from an icon has no stderr to read */
void video_error(const char *msg)
{
    fprintf(stderr, "openmotoracer: %s\n", msg);
    if (g_win)
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "OpenMotoRacer", msg, g_win);
}

int video_shot(const char *file)
{
    void *from = g_screen;
    int w = g_scr_w, h = g_surf_h, pitch = g_pitch * 2;
    SDL_Surface *s;
    int ok;
#ifdef MISTER_HYBRID
    if (g_half_rows) {                  /* the picture as the core would have it */
        half_merge();
        from = g_half_to, w = 640, h = 240, pitch = g_half_step * 2;
    }
#endif
    s = SDL_CreateRGBSurfaceWithFormatFrom(from, w, h, 16, pitch, SDL_PIXELFORMAT_RGB565);
    ok = s && SDL_SaveBMP(s, file) == 0;
    SDL_FreeSurface(s);
    return ok;
}

void video_toggle_fullscreen(void)
{
#ifdef MISTER_HYBRID
    return;                     /* the core's screen is the only one */
#endif
    g_fullscreen ^= 1;
    if (g_win)
        SDL_SetWindowFullscreen(g_win, g_fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
}

/* the start; a run without a window stays as it is, so that the pictures do not depend on
   the settings */
void video_set_fullscreen(int on)
{
    if (g_win && !g_fullscreen != !on)
        video_toggle_fullscreen();
}

int video_fullscreen(void)
{
    return g_fullscreen;
}

int video_has_window(void)
{
    return g_win != 0;
}

/* WM_ACTIVATEAPP (0x4333bf, 0x433425): the full screen gives way to a window while another
   program has the focus, and comes back with it; what is saved is what it was (0x432fda) */
void video_focus(int gained)
{
    if (!gained) {
        g_no_3d_once = 0;
        g_draw_2d = 0;
        if (g_kept < 0)
            g_kept = g_fullscreen;
        if (g_fullscreen) {
            video_set_fullscreen(0);
            g_no_3d = 1;
        }
    } else {
        g_no_3d = 0;
        g_no_3d_once = 1;
        if (g_kept >= 0)
            video_set_fullscreen(g_kept);
        g_kept = -1;
    }
}

/* 0x49ad90's test */
int video_no_3d(void)
{
    return g_no_3d || g_no_3d_once;
}

/* 0x49b1c0, each picture shown: back from another program, one without 3D, then all */
void video_frame_end(void)
{
    if (g_no_3d_once) {
        g_no_3d_once = 0;
        g_draw_2d = 1;
    }
}

int video_fullscreen_kept(void)
{
    return g_kept >= 0 ? g_kept : g_fullscreen;
}

void video_quit(void)
{
    if (g_tex)
        SDL_DestroyTexture(g_tex);
    if (g_ren)
        SDL_DestroyRenderer(g_ren);
    if (g_win)
        SDL_DestroyWindow(g_win);
}
