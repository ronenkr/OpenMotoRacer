/* The Waveshare ESP32-S3-Touch-LCD-1.83: its LCD, SD card, touch panel, IMU, buttons, codec
   and power chip (pins from Waveshare's schematic and BSP, NOTES §4.26). */
#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>
#include <stdbool.h>

/* the LCD held sideways: 284 wide, 240 tall */
#define LCD_W 284
#define LCD_H 240

void board_init(void);                 /* I2C, the power chip, buttons, LCD (black), touch, IMU */
bool board_sd_mount(void);             /* the card at /sdcard */

/* LCD: a rectangle of big-endian RGB565 pixels from DMA-capable memory, sent in the
   background; board_lcd_wait() waits for every rectangle sent so far */
void board_lcd_draw(int x, int y, int w, int h, const uint16_t *px);
void board_lcd_wait(void);
void board_lcd_wait_pending(int most);  /* until at most `most` rectangles are still queued */
int board_lcd_clock(void);             /* the SPI clock in Hz */
void board_backlight(int percent);

/* input */
bool board_boot_down(void);            /* BOOT (GPIO0) */
bool board_pwr_down(void);             /* PWR (through the AXP2101, sensed on GPIO41, high pressed) */
bool board_touch(int *x, int *y);      /* one finger, in the LCD's sideways pixels */
bool board_accel(float g[3]);          /* the IMU's accelerometer, in g, the board's axes */

/* sound: 16-bit mono at `rate` through the ES8311; `fill` is called from the audio task on
   core 1 for `frames` samples at a time */
typedef void (*board_audio_fill)(int16_t *out, int frames);
bool board_audio_start(int rate, board_audio_fill fill);
void board_audio_volume(int percent);

#endif
