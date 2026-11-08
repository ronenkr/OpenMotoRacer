/* Page 1, the loading screen (frame 0x40b3f0, no start or end): the track's own picture
   and "Loading...", drawn once while the menu is still up; the original then takes the
   menu down and loads the race, and each step of the load draws the screen again
   (0x4470d0) with the Moto Racer logo on every other step and the Pocket and Reverse
   badges (NOTES §4.12.10). Then the race takes over (race.c); if it cannot be loaded,
   the main menu comes back, as in the original. */
#include "video.h"
#include "choices.h"
#include "clock.h"
#include "menu.h"
#include "pages.h"
#include "race.h"
#include <stdio.h>

enum { P_LOGO = 0xaf, P_LOADING = 0xb0, P_REVERSE = 0xb9, P_POCKET = 0xba };

static Bitmap g_load;           /* 0x4e90e8 */
static int g_logo = 1;          /* 0x4e90fc: the logo is shown on this step */
static int g_step;

/* 0x4470d0, a step of the race's load */
void loading_progress(void)
{
    if (!bitmap_loaded(&g_load))
        return;
    bitmap_draw(&g_load, BM_OWN, 100, 0);
    bitmap_draw(&g_pics[P_LOADING], BM_OWN, 100, 0);
    if (g_reverse)
        bitmap_draw(&g_pics[P_REVERSE], BM_OWN, 100, 0);
    if (g_pocket)
        bitmap_draw(&g_pics[P_POCKET], BM_OWN, 100, 0);
    if (g_logo && !g_playback)
        bitmap_draw(&g_pics[P_LOGO], BM_OWN, 100, 0);
    g_logo ^= 1;
}

void page_load_frame(void)
{
    if (g_step == 0) {
        char path[300];
        if (race_demo_picture())        /* 0x542891 */
            snprintf(path, sizeof path, "%s", race_demo_picture());
        else
            snprintf(path, sizeof path, "%sload.lez", g_tracks[g_track].folder);
        bitmap_load_lez(&g_load, path, BM_OPAQUE);
        bitmap_place(&g_pics[P_LOGO], 498, 381, 0);
        bitmap_place(&g_pics[P_LOADING], 0, 408, 0);
        if (g_pocket)
            bitmap_place(&g_pics[P_POCKET], 4, 460, 0);
        if (g_reverse)
            bitmap_place(&g_pics[P_REVERSE], 4, 470, 0);
        /* (the badges are tested here too, but the menu is still up and they are not
           drawn until the load's steps) */
        if (bitmap_loaded(&g_load))
            bitmap_draw(&g_load, BM_OWN, 100, 0);
        bitmap_draw(&g_pics[P_LOADING], BM_OWN, 100, 0);
        g_step = 1;
        return;
    }
    if (g_step == 1) {
        char err[256];
        /* the menu goes, the race's load begins */
        loading_progress();
        g_step = 0;
        if (!race_load(err, sizeof err)) {
            video_error(err);
            bitmap_free(&g_load);
            menu_change_page(3);
            return;
        }
        bitmap_free(&g_load);
        menu_leave();
    }
}
