/* The board's bring-up test (stage 0): a picture that shows the orientation, touches drawn as
   dots, buttons and tilt in the log, the LCD's speed measured, music from the card. */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <dirent.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "board.h"

static const char *TAG = "test";

#define BE(c) (uint16_t)((c) << 8 | (c) >> 8)
#define RGB(r, g, b) BE((uint16_t)(((r) >> 3) << 11 | ((g) >> 2) << 5 | (b) >> 3))

static uint16_t *g_fb;          /* the whole screen, big-endian, internal DMA memory */

static void fill(int x0, int y0, int w, int h, uint16_t c)
{
    int x, y;
    for (y = y0; y < y0 + h; y++)
        for (x = x0; x < x0 + w; x++)
            if (x >= 0 && y >= 0 && x < LCD_W && y < LCD_H)
                g_fb[y * LCD_W + x] = c;
}

static void pattern(int phase)
{
    static const uint16_t bars[8][3] = { {255,255,255}, {255,255,0}, {0,255,255}, {0,255,0},
                                         {255,0,255}, {255,0,0}, {0,0,255}, {0,0,0} };
    int i, y;
    for (i = 0; i < 8; i++)
        fill(i * LCD_W / 8, 40, LCD_W / 8 + 1, 160, RGB(bars[i][0], bars[i][1], bars[i][2]));
    fill(0, 0, LCD_W, 40, RGB(40, 40, 40));
    fill(0, 200, LCD_W, 40, RGB(40, 40, 40));
    for (y = 0; y < 30; y++)                            /* an arrow pointing up, top middle */
        fill(LCD_W / 2 - y / 2, 5 + y, y + 1, 1, RGB(255, 0, 0));
    fill(2, 2, 12, 12, RGB(0, 255, 0));                 /* green: top left */
    fill(LCD_W - 14, LCD_H - 14, 12, 12, RGB(0, 0, 255)); /* blue: bottom right */
    fill(0, 0, LCD_W, 1, 0xffff);                       /* a white frame on the edge pixels */
    fill(0, LCD_H - 1, LCD_W, 1, 0xffff);
    fill(0, 0, 1, LCD_H, 0xffff);
    fill(LCD_W - 1, 0, 1, LCD_H, 0xffff);
    fill((phase * 3) % LCD_W, 205, 6, 30, RGB(255, 255, 255));  /* moving: tearing/speed */
}

static void push(void)
{
    int y;
    for (y = 0; y < LCD_H; y += 48)
        board_lcd_draw(0, y, LCD_W, LCD_H - y < 48 ? LCD_H - y : 48, g_fb + y * LCD_W);
    board_lcd_wait();
}

/* sound: a 1 s tone, then track 02 from the card */
static FILE *g_music;
static int g_tone = 22050;
static float g_ph;

static void audio(int16_t *out, int n)
{
    int i;
    if (g_tone > 0) {
        for (i = 0; i < n; i++) {
            out[i] = (int16_t)(sinf(g_ph) * 8000);
            g_ph += 2 * 3.14159265f * 440 / 22050;
            if (g_ph > 6.2831853f)
                g_ph -= 6.2831853f;
        }
        g_tone -= n;
        return;
    }
    if (!g_music || fread(out, 2, (size_t)n, g_music) != (size_t)n)
        memset(out, 0, (size_t)n * 2);
}

static void listdir(const char *path)
{
    DIR *d = opendir(path);
    struct dirent *e;
    int n = 0;
    if (!d) {
        ESP_LOGE(TAG, "%s: cannot open", path);
        return;
    }
    while ((e = readdir(d)) && n++ < 12)
        ESP_LOGI(TAG, "  %s/%s", path, e->d_name);
    closedir(d);
}

void boardtest(void)
{
    int frame = 0, i;
    int64_t t0;
    board_init();
    g_fb = heap_caps_malloc(LCD_W * LCD_H * 2, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    ESP_LOGI(TAG, "internal free %u, PSRAM free %u", (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    if (board_sd_mount()) {
        listdir("/sdcard");
        listdir("/sdcard/MOTO");
        if ((g_music = fopen("/sdcard/MOTO/music/track02.wav", "rb"))) {
            fseek(g_music, 44, SEEK_SET);
            ESP_LOGI(TAG, "music: track02.wav open");
        }
    }
    board_audio_start(22050, audio);

    pattern(0);
    t0 = esp_timer_get_time();
    for (i = 0; i < 60; i++)
        push();
    ESP_LOGI(TAG, "LCD: %d full frames a second at %d MHz",
             (int)(60 * 1000000LL / (esp_timer_get_time() - t0)), board_lcd_clock() / 1000000);

    for (;;) {
        int tx, ty;
        float g[3];
        bool touched = board_touch(&tx, &ty);
        pattern(frame);
        if (touched)
            fill(tx - 4, ty - 4, 9, 9, RGB(255, 128, 0));
        push();
        if (frame % 15 == 0) {
            bool a = board_accel(g);
            ESP_LOGI(TAG, "BOOT %d PWR %d touch %s %d,%d accel %s %+.2f %+.2f %+.2f", board_boot_down(),
                     board_pwr_down(), touched ? "yes" : "no", touched ? tx : -1, touched ? ty : -1,
                     a ? "ok" : "--", a ? g[0] : 0, a ? g[1] : 0, a ? g[2] : 0);
        }
        frame++;
        vTaskDelay(pdMS_TO_TICKS(16));
    }
}
