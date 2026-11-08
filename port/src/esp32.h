/* The ESP32 build (OMR_ESP32, esp32/): what the game's code asks of it (esp32/main/omr_esp32.c). */
#ifndef ESP32_H
#define ESP32_H

#ifdef OMR_ESP32
void esp32_default_bindings(void);      /* accelerate, brake, the stick, gears, turbo on the pad */
unsigned long long esp32_us(void);      /* microseconds, for the speed readout */
/* the last picture is scaled for the LCD on core 1 while core 0 goes on (the race's steps);
   before anything is drawn on the screen again, this waits for it */
void esp32_present_wait(void);
void esp32_present_race(const void *pixels);   /* the race's 284x216 picture to the LCD, now */
void esp32_screen_free(const void *pixels);    /* waits till that picture is not being sent */
void esp32_skip_present(void);                 /* this frame shows nothing new */
extern unsigned long long g_esp_step_us, g_esp_draw_us;   /* time in the steps, in the 3D */
extern unsigned g_esp_steps, g_esp_dropped;
/* the step's parts timed (the readout's "parts"): ESP_T(i) adds the time since the last mark */
extern unsigned long long g_esp_t[16], g_esp_mark;
#define ESP_T(i) do { unsigned long long t_ = esp32_us(); g_esp_t[i] += t_ - g_esp_mark; g_esp_mark = t_; } while (0)
#else
#define ESP_T(i) ((void)0)
#endif

#endif
