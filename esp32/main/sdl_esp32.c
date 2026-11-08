/* sdl/SDL.h on the ESP32: FreeRTOS threads and locks, esp_timer's clock, the board's sound,
   the picture handed to esp32_present(), events from the board's buttons and touch panel
   (omr_esp32.c), no joysticks (the game's pad is fed by joy_fake_pad). */
#include "SDL.h"
#include "omr_esp32.h"
#include "board.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "esp_log.h"

static const char *TAG = "sdl";

/* ------------------------------------------------------------------ init */
int SDL_Init(Uint32 flags) { (void)flags; return 0; }
int SDL_InitSubSystem(Uint32 flags) { (void)flags; return 0; }
Uint32 SDL_WasInit(Uint32 flags) { return flags & (SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_EVENTS); }
void SDL_Quit(void) {}
const char *SDL_GetError(void) { return "(ESP32)"; }
int SDL_setenv(const char *name, const char *value, int overwrite) { return setenv(name, value, overwrite); }
SDL_bool SDL_SetHint(const char *name, const char *value) { (void)name; (void)value; return SDL_FALSE; }
char *SDL_GetBasePath(void) { return strdup("/sdcard/"); }
void SDL_free(void *p) { free(p); }
int SDL_GetSystemRAM(void) { return 8; }

/* ------------------------------------------------------------------ time */
Uint64 SDL_GetPerformanceCounter(void) { return (Uint64)esp_timer_get_time(); }
Uint64 SDL_GetPerformanceFrequency(void) { return 1000000; }
void SDL_Delay(Uint32 ms) { vTaskDelay(pdMS_TO_TICKS(ms ? ms : 1)); }

/* ------------------------------------------------------------------ threads */
struct SDL_Thread {
    SDL_ThreadFunction fn;
    void *data;
    SemaphoreHandle_t done;
    int status;
};

static void thread_main(void *arg)
{
    SDL_Thread *t = arg;
    t->status = t->fn(t->data);
    xSemaphoreGive(t->done);
    vTaskDeleteWithCaps(NULL);
}

/* the port's only thread is the music's: core 1, beside the sound and the second drawer,
   above the game's priority so that a read never waits behind a picture */
SDL_Thread *SDL_CreateThread(SDL_ThreadFunction fn, const char *name, void *data)
{
    SDL_Thread *t = calloc(1, sizeof *t);
    if (!t)
        return NULL;
    t->fn = fn, t->data = data, t->done = xSemaphoreCreateBinary();
    if (xTaskCreatePinnedToCoreWithCaps(thread_main, name, 16384, t, 5, NULL, 1,
                                        MALLOC_CAP_SPIRAM) != pdPASS) {
        ESP_LOGE(TAG, "thread %s not started", name);
        free(t);
        return NULL;
    }
    return t;
}

void SDL_WaitThread(SDL_Thread *t, int *status)
{
    if (!t)
        return;
    xSemaphoreTake(t->done, portMAX_DELAY);
    if (status)
        *status = t->status;
    vSemaphoreDelete(t->done);
    free(t);
}

SDL_mutex *SDL_CreateMutex(void) { return (SDL_mutex *)xSemaphoreCreateMutex(); }
int SDL_LockMutex(SDL_mutex *m) { return m && xSemaphoreTake((SemaphoreHandle_t)m, portMAX_DELAY) == pdTRUE ? 0 : -1; }
int SDL_UnlockMutex(SDL_mutex *m) { return m && xSemaphoreGive((SemaphoreHandle_t)m) == pdTRUE ? 0 : -1; }
SDL_sem *SDL_CreateSemaphore(Uint32 initial) { return (SDL_sem *)xSemaphoreCreateCounting(1024, initial); }
int SDL_SemPost(SDL_sem *s) { return xSemaphoreGive((SemaphoreHandle_t)s) == pdTRUE ? 0 : -1; }
int SDL_SemWaitTimeout(SDL_sem *s, Uint32 ms)
{
    return xSemaphoreTake((SemaphoreHandle_t)s, pdMS_TO_TICKS(ms)) == pdTRUE ? 0 : 1;
}
int SDL_AtomicSet(SDL_atomic_t *a, int v) { return __atomic_exchange_n(&a->value, v, __ATOMIC_SEQ_CST); }
int SDL_AtomicGet(SDL_atomic_t *a) { return __atomic_load_n(&a->value, __ATOMIC_SEQ_CST); }

/* ------------------------------------------------------------------ video */
struct SDL_Window { int dummy; };
struct SDL_Renderer { int dummy; };
struct SDL_Texture { const void *pixels; int w, h, pitch; };
static struct SDL_Window g_win;
static struct SDL_Renderer g_ren;
static struct SDL_Texture g_tex;

SDL_Window *SDL_CreateWindow(const char *t, int x, int y, int w, int h, Uint32 f)
{
    (void)t; (void)x; (void)y; (void)w; (void)h; (void)f;
    return &g_win;
}
void SDL_DestroyWindow(SDL_Window *w) { (void)w; }
void SDL_SetWindowSize(SDL_Window *w, int width, int height) { (void)w; (void)width; (void)height; }
int SDL_SetWindowFullscreen(SDL_Window *w, Uint32 f) { (void)w; (void)f; return 0; }
SDL_Surface *SDL_GetWindowSurface(SDL_Window *w) { (void)w; return NULL; }
int SDL_UpdateWindowSurface(SDL_Window *w) { (void)w; return 0; }
SDL_Renderer *SDL_CreateRenderer(SDL_Window *w, int i, Uint32 f) { (void)w; (void)i; (void)f; return &g_ren; }
void SDL_DestroyRenderer(SDL_Renderer *r) { (void)r; }
int SDL_RenderSetLogicalSize(SDL_Renderer *r, int w, int h) { (void)r; (void)w; (void)h; return 0; }
SDL_Texture *SDL_CreateTexture(SDL_Renderer *r, Uint32 f, int a, int w, int h)
{
    (void)r; (void)f; (void)a; (void)w; (void)h;
    return &g_tex;
}
void SDL_DestroyTexture(SDL_Texture *t) { (void)t; }
int SDL_UpdateTexture(SDL_Texture *t, const SDL_Rect *r, const void *pixels, int pitch)
{
    t->pixels = pixels;
    t->w = r ? r->w : 640;
    t->h = r ? r->h : 480;
    t->pitch = pitch;
    return 0;
}
int SDL_RenderClear(SDL_Renderer *r) { (void)r; return 0; }
int SDL_RenderCopy(SDL_Renderer *r, SDL_Texture *t, const SDL_Rect *s, const SDL_Rect *d)
{
    (void)r; (void)t; (void)s; (void)d;
    return 0;
}
void SDL_RenderPresent(SDL_Renderer *r)
{
    (void)r;
    if (g_tex.pixels)
        esp32_present(g_tex.pixels, g_tex.w, g_tex.h, g_tex.pitch / 2);
}
int SDL_ShowCursor(int toggle) { (void)toggle; return 0; }
int SDL_ShowSimpleMessageBox(Uint32 f, const char *title, const char *msg, SDL_Window *w)
{
    (void)f; (void)w;
    esp32_message(title, msg);
    return 0;
}

SDL_Surface *SDL_CreateRGBSurfaceWithFormatFrom(void *pixels, int w, int h, int depth, int pitch, Uint32 f)
{
    SDL_Surface *s = calloc(1, sizeof *s);
    (void)depth; (void)f;
    if (s)
        s->pixels = pixels, s->w = w, s->h = h, s->pitch = pitch;
    return s;
}
void SDL_FreeSurface(SDL_Surface *s) { free(s); }

/* a 24-bit BMP of an RGB565 surface (--shot on the board, to the card) */
int SDL_SaveBMP(SDL_Surface *s, const char *file)
{
    FILE *f;
    int y, x, row = (s->w * 3 + 3) & ~3;
    unsigned char h[54] = { 'B', 'M' }, *line;
    unsigned size = 54u + (unsigned)row * (unsigned)s->h;
    if (!(f = fopen(file, "wb")) || !(line = calloc(1, (size_t)row))) {
        if (f)
            fclose(f);
        return -1;
    }
    memcpy(h + 2, &size, 4);
    h[10] = 54, h[14] = 40;
    memcpy(h + 18, &s->w, 4);
    memcpy(h + 22, &s->h, 4);
    h[26] = 1, h[28] = 24;
    fwrite(h, 1, 54, f);
    for (y = s->h - 1; y >= 0; y--) {
        const uint16_t *p = (const uint16_t *)((const char *)s->pixels + y * s->pitch);
        for (x = 0; x < s->w; x++) {
            unsigned c = p[x];
            line[3 * x] = (unsigned char)((c & 31) * 255 / 31);
            line[3 * x + 1] = (unsigned char)((c >> 5 & 63) * 255 / 63);
            line[3 * x + 2] = (unsigned char)((c >> 11) * 255 / 31);
        }
        fwrite(line, 1, (size_t)row, f);
    }
    free(line);
    fclose(f);
    return 0;
}

/* ------------------------------------------------------------------ sound */
static SDL_AudioCallback g_cb;
static void *g_cb_user;
static volatile int g_paused = 1;

/* the console's W: 3 s of what goes to the codec, kept for sending (tools/esp32_shot.py --wav) */
int16_t *g_cap;
volatile int g_cap_n, g_cap_want;

static void audio_fill(int16_t *out, int frames)
{
    static int16_t st[2 * 512];
    int i;
    if (g_paused || !g_cb || frames > 512) {
        memset(out, 0, (size_t)frames * 2);
        return;
    }
    g_cb(g_cb_user, (Uint8 *)st, frames * 4);
    /* the board has one small speaker: mono; then what it cannot play taken away (a 200 Hz
       high-pass: 96% of a race's sound was below it, engines and the music's bass, measured
       2026-11-09), and the rest made louder in the room that leaves (x 1.6, clipped) */
    {
        static float x1, x2, y1, y2;
        static float b0, b1, b2, a1, a2;
        if (b0 == 0.0f) {
            float w0 = 2.0f * 3.14159265f * 200.0f / 22050.0f, c = cosf(w0), al = sinf(w0) / (2.0f * 0.7071f);
            float n = 1.0f / (1.0f + al);
            b0 = (1.0f + c) * 0.5f * n, b1 = -(1.0f + c) * n, b2 = b0;
            a1 = -2.0f * c * n, a2 = (1.0f - al) * n;
        }
        for (i = 0; i < frames; i++) {
            float x = (float)(st[2 * i] + st[2 * i + 1]) * 0.5f;
            float y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            x2 = x1, x1 = x, y2 = y1, y1 = y;
            y *= 1.6f;
            out[i] = (int16_t)(y > 32767.0f ? 32767 : y < -32768.0f ? -32768 : (int)y);
        }
    }
    if (g_cap && g_cap_n < g_cap_want)
        for (i = 0; i < frames && g_cap_n < g_cap_want; i++)
            g_cap[g_cap_n++] = out[i];
}

SDL_AudioDeviceID SDL_OpenAudioDevice(const char *dev, int capture, const SDL_AudioSpec *want,
                                      SDL_AudioSpec *got, int changes)
{
    (void)dev; (void)capture; (void)changes;
    if (g_cb || !want->callback)
        return 0;
    if (got)
        *got = *want;
    g_cb = want->callback;
    g_cb_user = want->userdata;
    if (!board_audio_start(want->freq, audio_fill)) {
        g_cb = NULL;
        return 0;
    }
    return 1;
}
void SDL_PauseAudioDevice(SDL_AudioDeviceID d, int pause) { (void)d; g_paused = pause; }
void SDL_CloseAudioDevice(SDL_AudioDeviceID d) { (void)d; g_paused = 1; }

/* ------------------------------------------------------------------ events */
int SDL_PollEvent(SDL_Event *e)
{
    return esp32_event(e);
}

/* ------------------------------------------------------------------ joysticks: none */
int SDL_NumJoysticks(void) { return 0; }
SDL_bool SDL_IsGameController(int i) { (void)i; return SDL_FALSE; }
SDL_GameController *SDL_GameControllerOpen(int i) { (void)i; return NULL; }
void SDL_GameControllerClose(SDL_GameController *c) { (void)c; }
SDL_Joystick *SDL_GameControllerGetJoystick(SDL_GameController *c) { (void)c; return NULL; }
void SDL_GameControllerUpdate(void) {}
Sint16 SDL_GameControllerGetAxis(SDL_GameController *c, SDL_GameControllerAxis a) { (void)c; (void)a; return 0; }
Uint8 SDL_GameControllerGetButton(SDL_GameController *c, SDL_GameControllerButton b) { (void)c; (void)b; return 0; }
SDL_Joystick *SDL_JoystickOpen(int i) { (void)i; return NULL; }
void SDL_JoystickClose(SDL_Joystick *j) { (void)j; }
SDL_bool SDL_JoystickGetAttached(SDL_Joystick *j) { (void)j; return SDL_FALSE; }
void SDL_JoystickUpdate(void) {}
int SDL_JoystickNumAxes(SDL_Joystick *j) { (void)j; return 0; }
int SDL_JoystickNumButtons(SDL_Joystick *j) { (void)j; return 0; }
int SDL_JoystickNumHats(SDL_Joystick *j) { (void)j; return 0; }
Sint16 SDL_JoystickGetAxis(SDL_Joystick *j, int a) { (void)j; (void)a; return 0; }
Uint8 SDL_JoystickGetButton(SDL_Joystick *j, int b) { (void)j; (void)b; return 0; }
Uint8 SDL_JoystickGetHat(SDL_Joystick *j, int h) { (void)j; (void)h; return 0; }
int SDL_JoystickRumble(SDL_Joystick *j, Uint16 lo, Uint16 hi, Uint32 ms) { (void)j; (void)lo; (void)hi; (void)ms; return -1; }
