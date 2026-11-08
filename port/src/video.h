#ifndef VIDEO_H
#define VIDEO_H
#include <stdint.h>

/* The game's screen: 16-bit RGB565 (NOTES §4.3). The menus are 640x480. */
#define SCREEN_MAX_W 640
#define SCREEN_MAX_H 480

extern uint16_t *g_screen;     /* the back surface the game draws on (0x624f5c) */
extern int g_scr_w, g_scr_h;   /* the picture's size (0x655d10, 0x624b18): the 16:9 height in 16:9 */
extern int g_pitch;            /* pixels a row (0x62ebec) */

int video_init(int headless, char *err, int errsize);
void video_set_size(int w, int h, int view_h);  /* 0x4592a0's screen: w x h, the picture view_h high */
int video_surface_h(void);       /* 0x4dbffc: the screen's height, also under a 16:9 picture */
void video_mouse(int *x, int *y);       /* the window's 640x480 to the screen's own places */
void video_present(void);
void video_error(const char *msg);     /* a message window as well, when there is a window */
int video_shot(const char *file);   /* the screen as a BMP */
void video_quit(void);

void video_toggle_fullscreen(void);     /* the display menu's FS (0x43a3a0) */
int video_fullscreen(void);             /* 0x4dbff0 */
void video_set_fullscreen(int on);
int video_has_window(void);
void video_focus(int gained);           /* WM_ACTIVATEAPP's part for the screen */
void video_frame_end(void);             /* 0x49b1c0's part for it */
int video_no_3d(void);                  /* 0x4e893c, 0x4e8940: the 3D is not drawn */
extern int g_draw_2d;                   /* 0x4f1824: texts and pictures are drawn */
int video_fullscreen_kept(void);        /* what is saved as FullScreen */
extern int g_alt_return;                /* 0x4e8934: Alt+Return works (not on the podium) */

#ifdef MISTER_HYBRID
/* 640x240: the game takes itself for 320x240 (its texts, dials' digits, dialogs, each of their
   pixels shown twice as wide) and only the 3D is drawn at 640x480, its even rows */
extern int g_half_rows;
void video_set_half(int on);
void video_half_3d(int on);             /* the 3D's part of a picture begins, ends */
void video_half_rows(int y0, int y1);   /* the 2D is about to draw in these rows */
extern int g_half_lazy;
#define VIDEO_ROWS(y0, y1) do { if (g_half_lazy) video_half_rows(y0, y1); } while (0)
void video_direct_begin(void);          /* a race's picture begins: in the core's buffer */
void video_direct_end(void);            /* the race ends */
extern int g_hud_top, g_hud_bottom;     /* the OSD's HUD margins, in this screen's rows */
#else
#define g_half_rows 0
#define VIDEO_ROWS(y0, y1) ((void)0)
#define g_hud_top 0
#define g_hud_bottom 0
#endif

#ifdef OMR_ESP32
uint16_t *video_race_buffer(int k);     /* the board: the race's three pictures, 0..2 */
void video_use(uint16_t *screen);       /* drawing on that one */
#endif

#endif
