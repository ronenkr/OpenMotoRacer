/* The board's side of the port: the picture, the input, messages (omr_esp32.c). */
#ifndef OMR_ESP32_BOARD_H
#define OMR_ESP32_BOARD_H

#include "SDL.h"

void esp32_present(const void *pixels, int w, int h, int pitch);   /* pitch in pixels */
int esp32_event(SDL_Event *e);
void esp32_message(const char *title, const char *msg);

#endif
