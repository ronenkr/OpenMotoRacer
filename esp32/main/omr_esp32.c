/* OpenMotoRacer on the Waveshare ESP32-S3-Touch-LCD-1.83 (NOTES §4.26).
   Core 0: the game (port/src, unchanged but for OMR_ESP32's few lines). Core 1: the sound,
   the music's reads, the input. The LCD is fed by DMA straight from PSRAM.
   The picture: 4:3 at 284x216 on the sideways LCD, 12 black rows above and below.
   Controls (Danik's/the user's choice, 2026-11-08): tilt steers, BOOT accelerates, the touch
   panel's left half brakes and its right half is turbo; PWR is Escape (pause, back). In the
   menus a touch is the mouse, BOOT is Return. */
#include "omr_esp32.h"
#include "board.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_system.h"
#include "driver/usb_serial_jtag.h"
#include "driver/usb_serial_jtag_vfs.h"
#include "mbedtls/base64.h"

/* the game's */
#include "config.h"
#include "joy.h"
#include "race.h"
#include "dialog.h"
#include "esp32.h"
#include "r3d.h"

static const char *TAG = "omr";

int omr_main(int argc, char **argv);    /* port/src/main.c's main */
void sound_report(void);                /* sound.c: what is playing */

/* ------------------------------------------------------------------ the picture */
#define PIC_W LCD_W                     /* 284 */
#define PIC_H 216                       /* 4:3 near enough, and 8-row strips of 64-byte lines */
#define PIC_Y ((LCD_H - PIC_H) / 2)     /* 12 */
#define STRIP 24                        /* rows a DMA transfer: 13632 bytes */
#define STRIPS (PIC_H / STRIP)

static uint16_t *g_lcd[2];              /* big-endian, PSRAM, sent while the other is made */
static int g_cur;
static int g_col_w = -1, g_col_h = -1;
static uint16_t g_col[PIC_W];           /* the source column of each picture column */
static uint16_t g_row[PIC_H];
static int64_t g_present_us;            /* time spent here, for the readout */
static int64_t g_wait_us;               /* of it, waiting for the LCD */
static int g_presents;
static int64_t g_send_us;               /* of it, handing the picture to the LCD */

static inline uint16_t swap16(uint16_t c)
{
    return c;                           /* (the panel takes the CPU's byte order: board.c) */
}

/* two RGB565 pixels averaged, channel by channel */
static inline uint16_t avg2(uint16_t a, uint16_t b)
{
    return (uint16_t)(((a & 0xf7de) >> 1) + ((b & 0xf7de) >> 1) + (a & b & 0x0821));
}

static volatile int g_shot_wanted, g_shot_busy;

/* the game's own picture over the USB serial, base64 between "SHOT w h" and "END"
   (tools/esp32_shot.py) */
static void shot(const uint16_t *src, int w, int h, int pitch)
{
    static unsigned char out[4 * 640 / 3 * 2 + 8];
    char head[32];
    int y, n;
    n = snprintf(head, sizeof head, "\nSHOT %d %d\n", w, h);
    usb_serial_jtag_write_bytes(head, (size_t)n, portMAX_DELAY);
    for (y = 0; y < h; y++) {
        size_t olen = 0;
        mbedtls_base64_encode(out, sizeof out - 1, &olen, (const unsigned char *)(src + y * pitch), (size_t)w * 2);
        out[olen++] = '\n';
        usb_serial_jtag_write_bytes(out, olen, portMAX_DELAY);
    }
    usb_serial_jtag_write_bytes("END\n", 4, portMAX_DELAY);
    usb_serial_jtag_wait_tx_done(pdMS_TO_TICKS(1000));
}

/* the scaling, on core 1 (present_task) */
static void scale(const uint16_t *src, int w, int h, int pitch)
{
    uint16_t *out;
    int64_t t0 = esp_timer_get_time();
    int x, y;
    if (w != g_col_w || h != g_col_h) {
        for (x = 0; x < PIC_W; x++)
            g_col[x] = (uint16_t)((x * w + w / 2) / PIC_W);
        for (y = 0; y < PIC_H; y++)
            g_row[y] = (uint16_t)((y * h + h / 2) / PIC_H);
        g_col_w = w, g_col_h = h;
    }
    /* the buffer sent two pictures ago must be gone: at most the last picture left to send
       (each picture is one transfer: esp_lcd waits for the queue to empty before a rectangle's
       commands, so strips would have made this wait for every one of them) */
    board_lcd_wait_pending(1);
    g_wait_us += esp_timer_get_time() - t0;
    g_presents++;
    out = g_lcd[g_cur];
    if (w > PIC_W * 2 - PIC_W / 2) {
        /* 640x480 menus: each picture pixel the mean of the 2x2 under it (the texts stay readable) */
        for (y = 0; y < PIC_H; y++) {
            const uint16_t *a = src + g_row[y] * pitch, *b = g_row[y] + 1 < h ? a + pitch : a;
            uint16_t *o = out + y * PIC_W;
            for (x = 0; x < PIC_W; x++) {
                int c = g_col[x], c1 = c + 1 < w ? c + 1 : c;
                o[x] = swap16(avg2(avg2(a[c], a[c1]), avg2(b[c], b[c1])));
            }
        }
    } else {
        for (y = 0; y < PIC_H; y++) {   /* the race at 320x240: the nearest pixel */
            const uint16_t *a = src + g_row[y] * pitch;
            uint16_t *o = out + y * PIC_W;
            for (x = 0; x < PIC_W; x++)
                o[x] = swap16(a[g_col[x]]);
        }
    }
    {
        int64_t t1 = esp_timer_get_time();
        board_lcd_draw(0, PIC_Y, PIC_W, PIC_H, out);
        g_send_us += esp_timer_get_time() - t1;
    }
    g_cur ^= 1;
    g_present_us += esp_timer_get_time() - t0;
}

static SemaphoreHandle_t g_scale_go, g_scale_done;
static volatile int g_scaling;          /* a picture handed over, not yet scaled */
static int g_direct;                    /* the race's picture being sent from the game's screen */
static int g_skip_present;              /* the race showed its picture itself this frame */
static const uint16_t *g_sc_src;
static int g_sc_w, g_sc_h, g_sc_pitch;

static void present_task(void *arg)
{
    (void)arg;
    for (;;) {
        xSemaphoreTake(g_scale_go, portMAX_DELAY);
        scale(g_sc_src, g_sc_w, g_sc_h, g_sc_pitch);
        xSemaphoreGive(g_scale_done);
    }
}

/* the race's pipeline (race.c): its picture sent now, from the game's screen; the main loop's
   present this frame passes */
void esp32_present_race(const void *pixels)
{
    if (g_shot_wanted) {
        g_shot_wanted = 0;
        g_shot_busy = 1;
        vTaskDelay(pdMS_TO_TICKS(20));
        shot(pixels, PIC_W, PIC_H, PIC_W);
        g_shot_busy = 0;
    }
    g_presents++;
    board_lcd_wait_pending(1);
    board_lcd_draw(0, PIC_Y, PIC_W, PIC_H, pixels);
    g_skip_present = 1;
}

void esp32_skip_present(void)
{
    g_skip_present = 1;
}

void esp32_screen_free(const void *pixels)
{
    int64_t t0 = esp_timer_get_time();
    (void)pixels;
    board_lcd_wait_pending(1);          /* (the one just queued may still go: not that picture) */
    g_wait_us += esp_timer_get_time() - t0;
}

void esp32_present_wait(void)
{
    r3d_finish();                       /* (a race's picture still being drawn on core 1) */
    if (g_scaling) {
        xSemaphoreTake(g_scale_done, portMAX_DELAY);
        g_scaling = 0;
    }
    if (g_direct) {                     /* the screen sent: the game may draw on it again */
        int64_t t0 = esp_timer_get_time();
        board_lcd_wait_pending(0);
        g_wait_us += esp_timer_get_time() - t0;
        g_direct = 0;
    }
}

void esp32_present(const void *pixels, int w, int h, int pitch)
{
    if (g_skip_present) {               /* (the race has shown it: race.c's pipeline) */
        g_skip_present = 0;
        return;
    }
    esp32_present_wait();
    if (g_shot_wanted) {
        g_shot_wanted = 0;
        g_shot_busy = 1;                /* (no readout in the middle of it) */
        vTaskDelay(pdMS_TO_TICKS(20));
        shot(pixels, w, h, pitch);
        g_shot_busy = 0;
    }
    if (w == PIC_W && h == PIC_H && pitch == PIC_W && !((uintptr_t)pixels & 63)) {
        /* the race at the LCD's size, its rows one after another: sent from the game's screen
           by DMA as it is, while core 0 goes on to the next steps (no copy, no scaling) */
        g_presents++;
        board_lcd_wait_pending(0);
        board_lcd_draw(0, PIC_Y, PIC_W, PIC_H, pixels);
        g_direct = 1;
        return;
    }
    if (!g_scale_go) {                  /* (not started yet: here) */
        scale(pixels, w, h, pitch);
        return;
    }
    g_sc_src = pixels, g_sc_w = w, g_sc_h = h, g_sc_pitch = pitch;
    g_scaling = 1;
    xSemaphoreGive(g_scale_go);
}

void esp32_message(const char *title, const char *msg)
{
    FILE *f;
    ESP_LOGE(TAG, "%s: %s", title, msg);
    ESP_LOGE(TAG, "internal free %u (largest %u), PSRAM free %u (largest %u)",
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
             (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL),
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
             (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
    if ((f = fopen("/sdcard/OMR/message.txt", "w"))) {
        fprintf(f, "%s\n", msg);
        fclose(f);
    }
}

/* ------------------------------------------------------------------ input (core 1, 200 Hz) */
static volatile int g_touch_down, g_touch_x, g_touch_y;
static volatile int g_boot, g_pwr;
static volatile float g_tilt;           /* the lateral gravity, in g, smoothed */
static volatile int g_show_accel;       /* the console's I: the accelerometer 4 times a second */

/* the accelerometer's axis along the LCD's long side (X), held sideways with the USB connector
   on top, and its sign: the left end down reads below 0 and steers left (measured on the board,
   2026-11-09: left -0.6 g, right +0.7 g, Y unmoved). The same axis held flat or upright. */
#define TILT_AXIS 0
#define TILT_SIGN (+1.0f)
#define TILT_FULL 0.45f                 /* g at full lock: about 27 degrees */
#define TILT_DEAD 0.03f

static void input_task(void *arg)
{
    float g[3], f = 0;
    int first = 1;
    (void)arg;
    for (;;) {
        int x, y;
        if (board_touch(&x, &y))
            g_touch_x = x, g_touch_y = y, g_touch_down = 1;
        else
            g_touch_down = 0;
        g_boot = board_boot_down();
        g_pwr = board_pwr_down();
        if (board_accel(g)) {
            float v = TILT_SIGN * g[TILT_AXIS];
            f = first ? v : f + (v - f) * 0.35f;  /* (a few ms of smoothing at 200 Hz) */
            first = 0;
            g_tilt = f;
            {
                static int n;
                if (g_show_accel && ++n % 50 == 0)
                    printf("ACCEL %+.2f %+.2f %+.2f\n", g[0], g[1], g[2]);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

/* ------------------------------------------------------------------ events and the pad */
#define QUEUE 16
static SDL_Event g_q[QUEUE];
static int g_qn, g_qat;
static float g_tilt_zero;
static volatile int g_serial_key;       /* a key typed on the serial console, for tests */
static volatile int g_serial_hold;      /* bit 0 BOOT held, bit 1 brake, bit 2 turbo */
static volatile int g_click_x = -1, g_click_y;   /* a click typed on the console (game pixels) */
/* test settings typed on the console ("X OMR_PAGE=1 OMR_CHOICES=1,0,4,0,0\n"): kept over the
   restart that follows, in the RTC's memory, and set before the game starts */
#define ENV_MAGIC 0x4f4d5245u
static RTC_NOINIT_ATTR uint32_t g_env_magic;
static RTC_NOINIT_ATTR char g_env[256];
static int g_was_driving = -1;
static volatile int g_sent_x = 0x8000;  /* the stick last given to the game (readout) */

static void push_key(SDL_Keycode k, int down)
{
    SDL_Event *e = &g_q[g_qn++];
    memset(e, 0, sizeof *e);
    e->type = down ? SDL_KEYDOWN : SDL_KEYUP;
    e->key.keysym.sym = k;
}

static void push_mouse(int type, int x, int y)
{
    SDL_Event *e = &g_q[g_qn++];
    memset(e, 0, sizeof *e);
    e->type = (Uint32)type;
    if (type == SDL_MOUSEMOTION)
        e->motion.x = x, e->motion.y = y;
    else
        e->button.button = SDL_BUTTON_LEFT, e->button.x = x, e->button.y = y;
}

void esp32_default_bindings(void)
{
    /* accelerate, brake, the stick, gear up, gear down, turbo (as the MiSTer's) */
    static const int btn[ACTIONS] = { PAD_A, PAD_B, -2, -3, PAD_RB, PAD_LB, PAD_X };
    int i;
    for (i = 0; i < ACTIONS; i++)
        g_binding[i][1] = btn[i];
}

/* once a picture, before its events: the board's state into the game's */
static void sample(void)
{
    static int boot, pwr, touch;
    int bootnow = g_boot;
    int driving = g_racing && !dialog_holds();
    int t = g_touch_down, tx = g_touch_x, ty = g_touch_y;
    unsigned buttons = 0;
    int x = 0x8000;
    g_qn = g_qat = 0;
    {   /* the serial console's key: down now, up the next picture */
        static SDL_Keycode up;
        if (up)
            push_key(up, 0), up = 0;
        if (g_serial_key) {
            up = g_serial_key;
            g_serial_key = 0;
            push_key(up, 1);
        }
    }
    if (g_serial_hold & 1)
        bootnow = 1;
    {   /* the console's click: a touch for two pictures, at the LCD place of that game pixel */
        static int frames;
        if (g_click_x >= 0 && !frames)
            frames = 4;                 /* moved, down, held, up */
        if (frames) {
            if (--frames > 0) {
                t = 1;
                tx = g_click_x * PIC_W / 640;
                ty = PIC_Y + g_click_y * PIC_H / 480;
            } else
                g_click_x = -1;
        }
    }
    if (g_serial_hold & 6)
        t = 1, tx = g_serial_hold & 2 ? 10 : LCD_W - 10, ty = LCD_H / 2;
    if (driving && g_was_driving != 1)
        g_tilt_zero = g_tilt;           /* the way it is held when the riding starts is straight */
    g_was_driving = driving;
    if (driving) {
        float s = g_tilt - g_tilt_zero;
        s = fabsf(s) < TILT_DEAD ? 0 : (s - copysignf(TILT_DEAD, s)) / (TILT_FULL - TILT_DEAD);
        if (s > 1)
            s = 1;
        if (s < -1)
            s = -1;
        x = 0x8000 + (int)(s * 0x7fff);
        if (bootnow)
            buttons |= 1u << PAD_A;
        if (t)
            buttons |= 1u << (tx < LCD_W / 2 ? PAD_B : PAD_X);
        if (touch && !t)                /* a touch let go in a race: no click for the menus */
            push_mouse(SDL_MOUSEBUTTONUP, 0, 0);
    } else {
        int gx = tx * 640 / PIC_W, gy = (ty - PIC_Y) * 480 / PIC_H;
        gy = gy < 0 ? 0 : gy > 479 ? 479 : gy;
        /* the pointer goes there first, the button goes down the picture after (a page sees
           what is under the pointer before the press), and up when the finger lifts */
        if (t)
            push_mouse(SDL_MOUSEMOTION, gx, gy);
        if (t && touch == 1)
            push_mouse(SDL_MOUSEBUTTONDOWN, gx, gy);
        if (!t && touch == 2)
            push_mouse(SDL_MOUSEBUTTONUP, gx, gy);
        if (bootnow != boot)
            push_key(SDLK_RETURN, bootnow);
    }
    touch = !t ? 0 : touch ? 2 : 1;
    boot = bootnow;
    if (g_pwr != pwr)
        push_key(SDLK_ESCAPE, g_pwr);
    pwr = g_pwr;
    g_sent_x = x;
    joy_fake_pad(x, 0x8000, buttons, 0, 0, 0);
}

int esp32_event(SDL_Event *e)
{
    static int fresh = 1;
    if (fresh) {
        if (!g_racing)
            esp32_present_wait();       /* (a menu draws at once; a race after its steps) */
        sample();
        fresh = 0;
    }
    if (g_qat < g_qn) {
        *e = g_q[g_qat++];
        return 1;
    }
    fresh = 1;                          /* the next call is the next picture's */
    return 0;
}

/* ------------------------------------------------------------------ the serial console */
/* S: a picture; n Return, e Escape, u d l r the arrows, 1..9 F1..F9, a/b/t hold accelerate,
   brake, turbo (toggle) */
static void console_task(void *arg)
{
    (void)arg;
    for (;;) {
        unsigned char c;
        if (usb_serial_jtag_read_bytes(&c, 1, portMAX_DELAY) != 1)
            continue;
        switch (c) {
        case 'S': g_shot_wanted = 1; break;
        case 'I': g_show_accel ^= 1; break;
        case 'W': {                     /* 3 s of the sound as sent to the codec, base64 */
            extern int16_t *g_cap;
            extern volatile int g_cap_n, g_cap_want;
            static unsigned char out[700];
            int at;
            if (!g_cap)
                g_cap = heap_caps_malloc(22050 * 3 * 2, MALLOC_CAP_SPIRAM);
            if (!g_cap)
                break;
            g_cap_n = 0;
            g_cap_want = 22050 * 3;
            while (g_cap_n < g_cap_want)
                vTaskDelay(pdMS_TO_TICKS(50));
            g_shot_busy = 1;
            usb_serial_jtag_write_bytes("\nWAV 22050\n", 11, portMAX_DELAY);
            for (at = 0; at < g_cap_want; at += 256) {
                size_t olen = 0;
                mbedtls_base64_encode(out, sizeof out - 1, &olen, (const unsigned char *)(g_cap + at), 512);
                out[olen++] = '\n';
                usb_serial_jtag_write_bytes(out, olen, portMAX_DELAY);
            }
            usb_serial_jtag_write_bytes("END\n", 4, portMAX_DELAY);
            g_shot_busy = 0;
            break;
        }
        case 'X': {                     /* X settings... \n: restart with them (X\n: none) */
            char line[sizeof g_env];
            int k = 0;
            unsigned char ch;
            while (k < (int)sizeof line - 1 && usb_serial_jtag_read_bytes(&ch, 1, pdMS_TO_TICKS(2000)) == 1
                   && ch != '\n')
                line[k++] = (char)ch;
            line[k] = 0;
            memcpy(g_env, line, sizeof line);
            g_env_magic = ENV_MAGIC;
            esp_restart();
            break;
        }
        case 'c': {                     /* cXXXYYY */
            char d[6];
            int k;
            for (k = 0; k < 6; k++)
                if (usb_serial_jtag_read_bytes(&d[k], 1, pdMS_TO_TICKS(500)) != 1)
                    break;
            if (k == 6) {
                g_click_y = (d[3] - '0') * 100 + (d[4] - '0') * 10 + (d[5] - '0');
                g_click_x = (d[0] - '0') * 100 + (d[1] - '0') * 10 + (d[2] - '0');
            }
            break;
        }
        case 'n': g_serial_key = SDLK_RETURN; break;
        case 'e': g_serial_key = SDLK_ESCAPE; break;
        case 'u': g_serial_key = SDLK_UP; break;
        case 'd': g_serial_key = SDLK_DOWN; break;
        case 'l': g_serial_key = SDLK_LEFT; break;
        case 'r': g_serial_key = SDLK_RIGHT; break;
        case 'a': g_serial_hold ^= 1; break;
        case 'b': g_serial_hold ^= 2; break;
        case 't': g_serial_hold ^= 4; break;
        default:
            if (c >= '1' && c <= '9')
                g_serial_key = SDLK_F1 + (c - '1');
        }
    }
}

unsigned long long g_esp_step_us, g_esp_draw_us;
unsigned g_esp_steps, g_esp_dropped;
unsigned long long g_esp_t[16], g_esp_mark;

unsigned long long esp32_us(void)
{
    return (unsigned long long)esp_timer_get_time();
}

/* ------------------------------------------------------------------ the start */
static void perf_task(void *arg)
{
    (void)arg;
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        while (g_shot_busy)
            vTaskDelay(pdMS_TO_TICKS(50));
        ESP_LOGI(TAG, "%d pictures shown, present %lld ms/5s (waiting %lld, sending %lld), internal free %u (min %u), PSRAM free %u (min %u)",
                 g_presents, g_present_us / 1000, g_wait_us / 1000, g_send_us / 1000,
                 (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                 (unsigned)heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL),
                 (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
                 (unsigned)heap_caps_get_minimum_free_size(MALLOC_CAP_SPIRAM));
        ESP_LOGI(TAG, "race: %u steps (%u dropped), steps %llu ms, 3D %llu ms /5s",
                 g_esp_steps, g_esp_dropped, g_esp_step_us / 1000, g_esp_draw_us / 1000);
        ESP_LOGI(TAG, "parts ms: in %llu riders %llu collide %llu fx %llu laps %llu cam %llu signs %llu snd %llu bodies %llu",
                 g_esp_t[0] / 1000, g_esp_t[1] / 1000, g_esp_t[2] / 1000, g_esp_t[3] / 1000,
                 g_esp_t[4] / 1000, g_esp_t[5] / 1000, g_esp_t[6] / 1000, g_esp_t[7] / 1000,
                 g_esp_t[8] / 1000);
        ESP_LOGI(TAG, "frame ms: blend/cam %llu horizon %llu track %llu bikes %llu fx/hud3d %llu hud2d %llu",
                 g_esp_t[9] / 1000, g_esp_t[10] / 1000, g_esp_t[11] / 1000, g_esp_t[12] / 1000,
                 g_esp_t[13] / 1000, g_esp_t[14] / 1000);
        memset(g_esp_t, 0, sizeof g_esp_t);
        {
            extern unsigned long long g_esp_r0_us, g_esp_r1_us, g_esp_rwait_us;
            extern int r3d_band_split(void);
            ESP_LOGI(TAG, "3D rows: core 0 %llu ms, core 1 %llu ms, waited %llu ms, split at %d", g_esp_r0_us / 1000,
                     g_esp_r1_us / 1000, g_esp_rwait_us / 1000, r3d_band_split());
            g_esp_r0_us = g_esp_r1_us = g_esp_rwait_us = 0;

        }
        {
            float g[3] = { 0, 0, 0 };
            board_accel(g);
            ESP_LOGI(TAG, "input: accel %+.2f %+.2f %+.2f tilt %+.2f zero %+.2f stick %04x device %d analog %d "
                     "bind %d %d %d %d %d %d %d racing %d", g[0], g[1], g[2], g_tilt, g_tilt_zero, g_sent_x,
                     g_input_device, g_analog, g_binding[0][1], g_binding[1][1], g_binding[2][1],
                     g_binding[3][1], g_binding[4][1], g_binding[5][1], g_binding[6][1], g_racing);
        }
        g_esp_steps = g_esp_dropped = 0;
        g_esp_step_us = g_esp_draw_us = 0;
        g_present_us = g_wait_us = g_send_us = 0;
        g_presents = 0;
    }
}

static void game_task(void *arg)
{
    static char *argv[] = { "openmotoracer", "-data", "/sdcard/MOTO", "-home", "/sdcard/OMR", NULL };
    int code;
    (void)arg;
    setenv("OMR_PERF", "1", 0);
    setenv("OMR_R3D_TWO", "1", 0);      /* both cores draw (r3d.c) */
    if (g_env_magic == ENV_MAGIC) {     /* the console's test settings */
        char *save, *tok;
        g_env[sizeof g_env - 1] = 0;
        for (tok = strtok_r(g_env, " ", &save); tok; tok = strtok_r(NULL, " ", &save)) {
            char *eq = strchr(tok, '=');
            if (eq) {
                *eq = 0;
                setenv(tok, eq + 1, 1);
                ESP_LOGI(TAG, "test setting %s=%s", tok, eq + 1);
            }
        }
        g_env_magic = 0;                /* (once: the next start is a plain one) */
    }
    code = omr_main(5, argv);
    ESP_LOGW(TAG, "the game ended (%d); restarting", code);
    vTaskDelay(pdMS_TO_TICKS(500));
    esp_restart();
}

void app_main(void)
{
    int i;
    usb_serial_jtag_driver_config_t uc = { .tx_buffer_size = 4096, .rx_buffer_size = 256 };
    usb_serial_jtag_driver_install(&uc);
    usb_serial_jtag_vfs_use_driver();
    board_init();
    for (i = 0; i < 2; i++) {
        g_lcd[i] = heap_caps_aligned_calloc(64, PIC_W * PIC_H, 2, MALLOC_CAP_SPIRAM);
        if (!g_lcd[i])
            ESP_LOGE(TAG, "no picture buffer");
    }
    if (!board_sd_mount()) {
        static uint16_t DMA_ATTR red[LCD_W * 8];
        for (i = 0; i < LCD_W * 8; i++)
            red[i] = 0xf800;            /* red */
        for (i = 0; i < LCD_H; i += 8)
            board_lcd_draw(0, i, LCD_W, 8, red);
        board_lcd_wait();
        ESP_LOGE(TAG, "no SD card: put the card with MOTO/ on it in, then press reset");
        return;
    }
    g_scale_go = xSemaphoreCreateBinary();
    g_scale_done = xSemaphoreCreateBinary();
    xTaskCreatePinnedToCore(present_task, "present", 4096, NULL, 7, NULL, 1);
    xTaskCreatePinnedToCore(input_task, "input", 4096, NULL, 6, NULL, 1);
    xTaskCreatePinnedToCore(perf_task, "perf", 4096, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(console_task, "console", 3072, NULL, 2, NULL, 1);
    /* the game: a big stack (the menus and the race go deep), internal RAM */
    xTaskCreatePinnedToCore(game_task, "game", 48 * 1024, NULL, 4, NULL, 0);
}
