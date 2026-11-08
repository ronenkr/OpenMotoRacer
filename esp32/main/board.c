/* The Waveshare ESP32-S3-Touch-LCD-1.83 (board.h). Pins: Waveshare's schematic table and its
   BSP (waveshare/esp32_s3_touch_lcd_1_83 2.0.0); NOTES §4.26. */
#include "board.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/i2s_std.h"
#include "driver/spi_master.h"
#include "driver/ledc.h"
#include "driver/sdmmc_host.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_vfs_fat.h"
#include "esp_log.h"
#include "esp_codec_dev.h"
#include "esp_codec_dev_defaults.h"

static const char *TAG = "board";

/* pins */
#define PIN_LCD_MOSI 7
#define PIN_LCD_SCLK 6
#define PIN_LCD_CS   5
#define PIN_LCD_DC   4
#define PIN_LCD_RST  38
#define PIN_LCD_BL   40
#define PIN_I2C_SCL  14
#define PIN_I2C_SDA  15
#define PIN_TP_INT   13
#define PIN_TP_RST   39
#define PIN_SD_CLK   2
#define PIN_SD_CMD   1
#define PIN_SD_D0    3
#define PIN_SD_D3    42
#define PIN_I2S_MCLK 16
#define PIN_I2S_BCLK 9
#define PIN_I2S_WS   45
#define PIN_I2S_DOUT 8
#define PIN_I2S_DIN  10
#define PIN_PA       46
#define PIN_BOOT     0
#define PIN_PWR      41

#define ADDR_TOUCH 0x15     /* CST816D */
#define ADDR_IMU   0x6b     /* QMI8658 */
#define ADDR_PMU   0x34     /* AXP2101 */

#define LCD_CLOCK (80 * 1000 * 1000)
#define LCD_STRIP_MAX (32 * 1024)        /* the driver's chunks of a rectangle */
#define LCD_QUEUE 24

static i2c_master_bus_handle_t g_i2c;
static i2c_master_dev_handle_t g_touch, g_imu, g_pmu;
static esp_lcd_panel_io_handle_t g_io;
static esp_lcd_panel_handle_t g_panel;
static SemaphoreHandle_t g_lcd_done;
static volatile int g_lcd_pending;
static bool g_imu_ok, g_touch_ok;

/* ------------------------------------------------------------------ I2C */
static i2c_master_dev_handle_t i2c_dev(uint8_t addr)
{
    i2c_device_config_t c = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = addr,
        .scl_speed_hz = 400000,
    };
    i2c_master_dev_handle_t d = NULL;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(g_i2c, &c, &d));
    return d;
}

static bool reg_write(i2c_master_dev_handle_t d, uint8_t reg, uint8_t val)
{
    uint8_t b[2] = { reg, val };
    return i2c_master_transmit(d, b, 2, 20) == ESP_OK;
}

static bool reg_read(i2c_master_dev_handle_t d, uint8_t reg, uint8_t *out, size_t n)
{
    return i2c_master_transmit_receive(d, &reg, 1, out, n, 20) == ESP_OK;
}

/* ------------------------------------------------------------------ the power chip */
static void pmu_init(void)
{
    uint8_t v;
    g_pmu = i2c_dev(ADDR_PMU);
    /* 0x27: the PWR key's levels; bits 3:2 the press that powers off, 4/6/8/10 s: 10 s, so
       that holding PWR (Pause/Back) in play never switches the board off */
    if (reg_read(g_pmu, 0x27, &v, 1)) {
        reg_write(g_pmu, 0x27, (uint8_t)(v | 0x0c));
        ESP_LOGI(TAG, "AXP2101: PWR off-press 10 s (0x27 %02x -> %02x)", v, v | 0x0c);
    } else {
        ESP_LOGW(TAG, "AXP2101 not answering");
    }
}

/* ------------------------------------------------------------------ LCD */
static bool lcd_done_cb(esp_lcd_panel_io_handle_t io, esp_lcd_panel_io_event_data_t *e, void *ctx)
{
    BaseType_t woken = pdFALSE;
    __atomic_fetch_sub(&g_lcd_pending, 1, __ATOMIC_RELAXED);
    xSemaphoreGiveFromISR(g_lcd_done, &woken);
    return woken == pdTRUE;
}

static void lcd_init(void)
{
    spi_bus_config_t bus = {
        .sclk_io_num = PIN_LCD_SCLK, .mosi_io_num = PIN_LCD_MOSI, .miso_io_num = -1,
        .quadwp_io_num = -1, .quadhd_io_num = -1, .max_transfer_sz = LCD_STRIP_MAX,
    };
    esp_lcd_panel_io_spi_config_t io = {
        .dc_gpio_num = PIN_LCD_DC, .cs_gpio_num = PIN_LCD_CS, .pclk_hz = LCD_CLOCK,
        .lcd_cmd_bits = 8, .lcd_param_bits = 8, .spi_mode = 0, .trans_queue_depth = LCD_QUEUE,
        .on_color_trans_done = lcd_done_cb,
        .flags.psram_dma_direct = 1,       /* PSRAM pictures sent by DMA as they are, no copy */
    };
    esp_lcd_panel_dev_config_t pc = {
        .reset_gpio_num = PIN_LCD_RST, .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };
    g_lcd_done = xSemaphoreCreateCounting(64, 0);
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO));
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &io, &g_io));
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(g_io, &pc, &g_panel));
    esp_lcd_panel_reset(g_panel);
    esp_lcd_panel_init(g_panel);
    esp_lcd_panel_invert_color(g_panel, true);
    /* sideways: rows and columns swapped (MV), the 240 axis mirrored (MX), which needs no gap
       in the controller's 240x320 memory */
    esp_lcd_panel_swap_xy(g_panel, true);
    esp_lcd_panel_mirror(g_panel, true, false);
    /* RAMCTRL (0xb0): the second byte's bit 3 takes the pixels little-endian, as the CPU keeps
       them (0xf0 is the default, MSB first): no byte swapping before a DMA */
    esp_lcd_panel_io_tx_param(g_io, 0xb0, (uint8_t[]){ 0x00, 0xf8 }, 2);
    {   /* black, then the light on */
        static uint16_t DMA_ATTR black[LCD_W * 16];
        int y;
        memset(black, 0, sizeof black);
        for (y = 0; y < LCD_H; y += 16)
            board_lcd_draw(0, y, LCD_W, 16, black);
        board_lcd_wait();
    }
    esp_lcd_panel_disp_on_off(g_panel, true);

    ledc_timer_config_t t = { .speed_mode = LEDC_LOW_SPEED_MODE, .duty_resolution = LEDC_TIMER_10_BIT,
                              .timer_num = LEDC_TIMER_1, .freq_hz = 5000, .clk_cfg = LEDC_AUTO_CLK };
    ledc_channel_config_t ch = { .gpio_num = PIN_LCD_BL, .speed_mode = LEDC_LOW_SPEED_MODE,
                                 .channel = LEDC_CHANNEL_1, .timer_sel = LEDC_TIMER_1, .duty = 0 };
    ledc_timer_config(&t);
    ledc_channel_config(&ch);
    board_backlight(100);
}

void board_lcd_draw(int x, int y, int w, int h, const uint16_t *px)
{
    __atomic_fetch_add(&g_lcd_pending, 1, __ATOMIC_RELAXED);
    esp_lcd_panel_draw_bitmap(g_panel, x, y, x + w, y + h, px);
}

void board_lcd_wait_pending(int most)
{
    while (g_lcd_pending > most)
        xSemaphoreTake(g_lcd_done, pdMS_TO_TICKS(100));
}

void board_lcd_wait(void)
{
    board_lcd_wait_pending(0);
    while (xSemaphoreTake(g_lcd_done, 0) == pdTRUE)
        ;
}

int board_lcd_clock(void)
{
    return LCD_CLOCK;
}

void board_backlight(int percent)
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, (uint32_t)(percent * 1023 / 100));
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
}

/* ------------------------------------------------------------------ touch, IMU, buttons */
static void touch_init(void)
{
    uint8_t id = 0;
    gpio_config_t o = { .pin_bit_mask = 1ull << PIN_TP_RST, .mode = GPIO_MODE_OUTPUT };
    gpio_config_t i = { .pin_bit_mask = 1ull << PIN_TP_INT, .mode = GPIO_MODE_INPUT,
                        .pull_up_en = GPIO_PULLUP_ENABLE };
    gpio_config(&o);
    gpio_config(&i);
    gpio_set_level(PIN_TP_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(PIN_TP_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(60));
    g_touch = i2c_dev(ADDR_TOUCH);
    g_touch_ok = reg_read(g_touch, 0xa7, &id, 1);       /* chip id */
    reg_write(g_touch, 0xfe, 0x01);                     /* never sleep: always answers */
    ESP_LOGI(TAG, "touch CST816D: %s, id %02x", g_touch_ok ? "ok" : "MISSING", id);
}

bool board_touch(int *x, int *y)
{
    uint8_t b[6];
    int px, py;
    if (!g_touch_ok || !reg_read(g_touch, 0x01, b, 6) || (b[1] & 0x0f) == 0)
        return false;
    px = (b[2] & 0x0f) << 8 | b[3];                     /* the panel's own: 0..239 across */
    py = (b[4] & 0x0f) << 8 | b[5];                     /* 0..283 down */
    /* to the sideways picture (MV + MX) */
    *x = py;
    *y = 239 - px;
    return true;
}

static void imu_init(void)
{
    uint8_t id = 0;
    g_imu = i2c_dev(ADDR_IMU);
    g_imu_ok = reg_read(g_imu, 0x00, &id, 1) && id == 0x05;
    if (g_imu_ok) {
        reg_write(g_imu, 0x60, 0xb0);                   /* soft reset */
        vTaskDelay(pdMS_TO_TICKS(20));
        reg_write(g_imu, 0x02, 0x40);                   /* CTRL1: address auto-increment */
        reg_write(g_imu, 0x03, 0x15);                   /* CTRL2: accel +-4 g, 250 Hz */
        reg_write(g_imu, 0x06, 0x01);                   /* CTRL5: accel low-pass on */
        reg_write(g_imu, 0x08, 0x01);                   /* CTRL7: accel on */
    }
    ESP_LOGI(TAG, "IMU QMI8658: %s (id %02x)", g_imu_ok ? "ok" : "MISSING", id);
}

bool board_accel(float g[3])
{
    uint8_t b[6];
    int i;
    if (!g_imu_ok || !reg_read(g_imu, 0x35, b, 6))
        return false;
    for (i = 0; i < 3; i++)
        g[i] = (int16_t)(b[2 * i] | b[2 * i + 1] << 8) * (4.0f / 32768.0f);
    return true;
}

bool board_boot_down(void)
{
    return gpio_get_level(PIN_BOOT) == 0;
}

bool board_pwr_down(void)
{
    return gpio_get_level(PIN_PWR) == 1;      /* high while pressed (measured) */
}

/* ------------------------------------------------------------------ SD card */
bool board_sd_mount(void)
{
    esp_vfs_fat_sdmmc_mount_config_t mc = { .format_if_mount_failed = false, .max_files = 8,
                                            .allocation_unit_size = 16 * 1024 };
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot = SDMMC_SLOT_CONFIG_DEFAULT();
    sdmmc_card_t *card;
    esp_err_t e;
    gpio_config_t d3 = { .pin_bit_mask = 1ull << PIN_SD_D3, .mode = GPIO_MODE_OUTPUT };
    gpio_config(&d3);
    gpio_set_level(PIN_SD_D3, 1);                       /* D3/CS held high: SD mode, 1 bit */
    host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;
    slot.width = 1;
    slot.clk = PIN_SD_CLK;
    slot.cmd = PIN_SD_CMD;
    slot.d0 = PIN_SD_D0;
    slot.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;
    /* the first start after flashing finds the card not ready now and then: a few tries */
    for (int tries = 0; tries < 5; tries++) {
        e = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot, &mc, &card);
        if (e == ESP_OK)
            break;
        ESP_LOGW(TAG, "SD card: %s, again", esp_err_to_name(e));
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    if (e != ESP_OK) {
        ESP_LOGE(TAG, "SD card: %s", esp_err_to_name(e));
        return false;
    }
    ESP_LOGI(TAG, "SD card: %s, %llu MB, %d kHz", card->cid.name,
             (unsigned long long)card->csd.capacity * card->csd.sector_size >> 20,
             card->real_freq_khz);
    return true;
}

/* ------------------------------------------------------------------ sound */
static i2s_chan_handle_t g_tx;
static esp_codec_dev_handle_t g_codec;
static board_audio_fill g_fill;
#define AUDIO_FRAMES 256

static void audio_task(void *arg)
{
    static int16_t mono[AUDIO_FRAMES];
    static int16_t DMA_ATTR st[AUDIO_FRAMES * 2];
    for (;;) {
        size_t done;
        int i;
        g_fill(mono, AUDIO_FRAMES);
        for (i = 0; i < AUDIO_FRAMES; i++)
            st[2 * i] = st[2 * i + 1] = mono[i];
        i2s_channel_write(g_tx, st, sizeof st, &done, portMAX_DELAY);
    }
}

bool board_audio_start(int rate, board_audio_fill fill)
{
    i2s_chan_config_t cc = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_1, I2S_ROLE_MASTER);
    i2s_std_config_t sc = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(rate),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = { .mclk = PIN_I2S_MCLK, .bclk = PIN_I2S_BCLK, .ws = PIN_I2S_WS,
                      .dout = PIN_I2S_DOUT, .din = I2S_GPIO_UNUSED },
    };
    cc.auto_clear = true;
    cc.dma_desc_num = 4;
    cc.dma_frame_num = AUDIO_FRAMES;
    sc.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_256;
    if (i2s_new_channel(&cc, &g_tx, NULL) != ESP_OK || i2s_channel_init_std_mode(g_tx, &sc) != ESP_OK
        || i2s_channel_enable(g_tx) != ESP_OK) {
        ESP_LOGE(TAG, "I2S failed");
        return false;
    }
    {
        audio_codec_i2c_cfg_t ic = { .port = I2C_NUM_1, .addr = ES8311_CODEC_DEFAULT_ADDR, .bus_handle = g_i2c };
        audio_codec_i2s_cfg_t sd = { .port = I2S_NUM_1, .tx_handle = g_tx };
        const audio_codec_ctrl_if_t *ctrl = audio_codec_new_i2c_ctrl(&ic);
        const audio_codec_data_if_t *data = audio_codec_new_i2s_data(&sd);
        const audio_codec_gpio_if_t *gp = audio_codec_new_gpio();
        es8311_codec_cfg_t ec = {
            .ctrl_if = ctrl, .gpio_if = gp, .codec_mode = ESP_CODEC_DEV_WORK_MODE_DAC,
            .master_mode = false, .use_mclk = true, .pa_pin = PIN_PA, .pa_reverted = false,
            .hw_gain = { .pa_voltage = 5.0, .codec_dac_voltage = 3.3 },
            .mclk_div = I2S_MCLK_MULTIPLE_256,
        };
        const audio_codec_if_t *es = es8311_codec_new(&ec);
        esp_codec_dev_cfg_t dc = { .dev_type = ESP_CODEC_DEV_TYPE_OUT, .codec_if = es, .data_if = data };
        esp_codec_dev_sample_info_t si = { .bits_per_sample = 16, .channel = 2, .channel_mask = 3,
                                           .sample_rate = (uint32_t)rate };
        g_codec = esp_codec_dev_new(&dc);
        if (!g_codec || esp_codec_dev_open(g_codec, &si) != ESP_CODEC_DEV_OK) {
            ESP_LOGE(TAG, "ES8311 failed");
            return false;
        }
        esp_codec_dev_set_out_vol(g_codec, 75);
    }
    g_fill = fill;
    xTaskCreatePinnedToCore(audio_task, "audio", 4096, NULL, configMAX_PRIORITIES - 2, NULL, 1);
    ESP_LOGI(TAG, "sound: %d Hz mono through the ES8311", rate);
    return true;
}

void board_audio_volume(int percent)
{
    if (g_codec)
        esp_codec_dev_set_out_vol(g_codec, percent);
}

/* ------------------------------------------------------------------ all */
void board_init(void)
{
    i2c_master_bus_config_t bc = {
        .i2c_port = I2C_NUM_1, .sda_io_num = PIN_I2C_SDA, .scl_io_num = PIN_I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT, .glitch_ignore_cnt = 7,
    };
    gpio_config_t keys = { .pin_bit_mask = 1ull << PIN_BOOT | 1ull << PIN_PWR,
                           .mode = GPIO_MODE_INPUT, .pull_up_en = GPIO_PULLUP_ENABLE };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bc, &g_i2c));
    gpio_config(&keys);
    pmu_init();
    lcd_init();
    touch_init();
    imu_init();
}
