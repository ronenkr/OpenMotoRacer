/* The ESP32 build (OMR_ESP32, esp32/): what the game's code asks of it (esp32/main/omr_esp32.c). */
#ifndef ESP32_H
#define ESP32_H

#ifdef OMR_ESP32
void esp32_default_bindings(void);      /* accelerate, brake, the stick, gears, turbo on the pad */
unsigned long long esp32_us(void);      /* microseconds, for the speed readout */
extern unsigned long long g_esp_step_us, g_esp_draw_us;   /* time in the steps, in the 3D */
extern unsigned g_esp_steps, g_esp_dropped;
#endif

#endif
