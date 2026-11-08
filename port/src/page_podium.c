/* The end of a championship race (NOTES §4.12.14): pages 16 (the Reverse tracks unlocked), 17
   (the Pocket bikes), 18 (the championship won), each a picture and Continue, and page 24, the
   podium (start 0x452240, frame 0x452f80, end 0x452eb0): the first three on their steps, or the
   player in front of them, the camera coming in over 7 s, the crowd.
   The names, times and buttons go on the screen under the 3D, which covers them all but in the
   game's Direct3D mode, as in the original. */
#include "bike3d.h"
#include "bikephys.h"
#include "bikes.h"
#include "choices.h"
#include "clock.h"
#include "config.h"
#include "exe.h"
#include "font.h"
#include "menu.h"
#include "pages.h"
#include "pose.h"
#include "r3d.h"
#include "race.h"
#include "race_sound.h"
#include "rules.h"
#include "sound.h"
#include "video.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define PIC(id) (&g_pics[id])
enum { P_BEST_RACE = 0x14, P_CONTINUE = 0x4d, P_NUMBER = 0x60, P_LINE = 0x68 };
enum { PIC_REVERSE = 199, PIC_POCKET = 198, PIC_WON = 200 };
#define NAMES 3

static char g_names[NAMES][32]; /* 0x53f7e8 */
static int g_times[4];          /* 0x540998 */
static int g_skins[4];          /* 0x540c90 */
static int g_count;             /* 0x4e9120: riders shown */
static int g_place;             /* 0x4e911c: the local player's place + 1, 4 at most */

/* 0x452090, when the race ends: the first three's names, times and liveries; a rider not named
   is "Computer", the liveries nobody wore are drawn at random */
void podium_data(void)
{
    int i, k, found = 0;
    for (i = 0; i < NAMES; i++)
        snprintf(g_names[i], sizeof g_names[i], "%s", exe_text(183));
    g_count = g_riders - 1;
    if (g_count > 3)
        g_count = 3;
    for (i = 0; i < g_count; i++)
        for (k = 1; k < g_riders; k++)
            if (g_racer[k].loaded && g_racer[k].place == i) {
                if (k == race_local() && g_racer[k].human)
                    snprintf(g_names[i], sizeof g_names[i], "%s", g_player_name);
                g_times[i] = g_racer[k].time;
                g_skins[i] = g_rider[k].skin;
                found++;
            }
    g_place = g_racer[race_local()].place + 1;
    if (g_place > 4)
        g_place = 4;
    for (i = found; i < 4; i++)
        g_skins[i] = bikephys_crt_rand() & 7;
}

/* ---------------------------------------------------------------- pages 16, 17, 18 */

static Bitmap g_picture;        /* 0x4e90e8 */
static int g_shown;             /* 0x542380 */

/* 0x44ffd0, 0x450290, 0x450550 */
void page_unlock_start(void)
{
    bitmap_place(PIC(P_CONTINUE), 496, 442, 0);
    menu_clear_buttons();
    menu_add_button(PIC(P_CONTINUE), 0, 0xffffffffu, VK_RETURN, -1, 0, 0, 0, -1, -1, 0);
    g_shown = 0;
}

/* 0x450050 */
void page_unlock_end(void)
{
    bitmap_free(&g_picture);
}

/* 0x44fdb0, 0x450080, 0x450340 */
static void unlock_frame(int id)
{
    if (!g_shown) {
        bitmap_free(&g_picture);
        bitmap_load_id(&g_picture, id);
        g_shown = 1;
    }
    menu_begin_frame();
    if (bitmap_loaded(&g_picture))
        bitmap_draw(&g_picture, BM_OWN, 100, 0);
    menu_draw_buttons();
    if (menu_pressed() == 0) {
        menu_press_done();
        menu_change_page(24);
    }
    menu_transition();
    r3d_render();
    menu_end_frame();
}

void page_reverse_frame(void)
{
    unlock_frame(PIC_REVERSE);
}

void page_pocket_frame(void)
{
    unlock_frame(PIC_POCKET);
}

void page_won_frame(void)
{
    unlock_frame(PIC_WON);
}

/* ---------------------------------------------------------------- page 24 */

static Rider g_rd[4];           /* records 1..4 */
static Model g_stand;           /* 0x540490 */
static float g_from[3], g_to[3], g_look[3];     /* 0x540ad0, 0x53f2a8, 0x53f558 */
static float g_t;               /* 0x53f77c: the camera's way, 7 s */
static float g_bounce[4], g_rate[4];    /* 0x540ca8, 0x53f4d0: the arms, there and back */
static unsigned g_last;         /* 0x53f2b4 */
static int g_ok;

/* a rider off his bike, which waits out of sight (the drawer's crashed branch) */
static void stand(Rider *r, int skin, const float body[3][3], float x, float y, float z)
{
    static const float far_[3] = { 0.0f, 0.0f, 10000.0f };
    static const float ident[3][3] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };
    memset(r, 0, sizeof *r);
    r->bike = g_bike;
    r->skin = skin;
    memcpy(r->m, ident, sizeof r->m);
    r->crashed = 1;
    memcpy(r->body, body ? body : ident, sizeof r->body);
    r->body_pt[0] = x, r->body_pt[1] = y, r->body_pt[2] = z;
    bike3d_place(r, far_);
}

void page_podium_start(void)
{
    float base, ground;
    int i;
    g_alt_return = behaviour_cd();      /* 0x4e8934 = -1; the 1997 CD does not ask */
    if (g_bike < 0)
        g_bike = 0;
    if (getenv("OMR_PODIUM")) {         /* the test's way in: the player's place, three riders */
        g_place = atoi(getenv("OMR_PODIUM"));
        g_count = 3;
        for (i = 0; i < 4; i++) {
            g_skins[i] = i;
            snprintf(g_names[i < NAMES ? i : 0], sizeof g_names[0], "%s", exe_text(183));
        }
    }
    if (g_count < g_place)
        g_count = g_place;
    g_ok = bikes_load(g_track) && bike3d_load(g_bikes[0].perso == 1);
    if (!g_ok)
        fprintf(stderr, "openmotoracer: the podium's bikes could not be read\n");
    base = g_bikes[0].perso ? 30.0f : 0.0f;     /* 0x53f4cc */
    ground = g_bikes[0].perso ? -178.0f : -258.0f;      /* 0x54032c */
    if (g_ok) {
        stand(&g_rd[0], g_skins[0], 0, 0.0f, base + 50.0f, 0.0f);
        if (g_count > 1)
            stand(&g_rd[1], g_skins[1], 0, 100.0f, base + 11.0f, 0.0f);
        if (g_count > 2)
            stand(&g_rd[2], g_skins[2], 0, -100.0f, base - 2.0f, 0.0f);
        if (g_count > 3) {      /* the player, turned 135 degrees, on the ground before them */
            float e = g_cos[0x1800], f = g_cos[0x800];
            const float turn[3][3] = { { e, 0.0f, -f }, { 0.0f, 1.0f, 0.0f }, { f, 0.0f, e } };
            stand(&g_rd[3], g_skins[3], turn, -50.0f, ground, 300.0f);
        }
    }
    g_from[0] = 1800.0f, g_from[1] = 500.0f, g_from[2] = 200.0f;
    g_to[0] = 0.0f;
    g_to[1] = -50.0f;
    g_to[2] = g_place < 4 ? 1500.0f : 2120.0f;
    g_look[0] = 0.0f, g_look[1] = g_place < 4 ? 120.0f : 100.0f, g_look[2] = 0.0f;
    g_t = 0.0f;
    if (bikephys_crt_rand() & 1) {
        g_from[0] = -g_from[0];
        g_to[0] = -g_to[0];
    }
    bitmap_place(PIC(P_CONTINUE), 496, 429, 0);
    bitmap_place(PIC(P_BEST_RACE), 430, 145, 0);
    menu_clear_buttons();
    /* 0x452bee: Escape leaves; 0x476b17: on the 1997 CD no key does */
    menu_add_button(PIC(P_CONTINUE), 0, 0xffffffffu, behaviour_cd() ? -1 : VK_ESCAPE, -1, 0, 0, 0,
                    -1, -1, 0);
    for (i = 0; i < 4; i++) {
        g_rate[i] = 6.0f;
        g_bounce[i] = (float)(i * 0.2);
    }
    /* the two banks after the family's, and the stand on them */
    r3d_load_page(3, "data\\podium\\Bnk00_01.raw");
    r3d_load_page(4, "data\\podium\\Bnk00_02.raw");
    model_load(&g_stand, "data\\podium\\podium");
    model_page_add(&g_stand, 2);
    model_scale(&g_stand, 0.04f, 0.04f, 0.04f);
    /* the player among the first three: the winners' music; else another, and a word */
    for (i = 0; i < NAMES; i++)
        if (strcasecmp(g_player_name, g_names[i]) == 0)
            break;
    if (i < NAMES)
        music_play(12, 0);
    else {
        music_play(11, 0);
        sound_play(0x5b, 0);
    }
    g_last = g_now;
    {
        static const unsigned char white[3] = { 255, 255, 255 };
        font_set_colors(font_get(12), white, white);
    }
    sound_podium_load();
    podium_sound_start();
}

/* 0x452eb0 */
void page_podium_end(void)
{
    g_alt_return = 1;
    podium_sound_stop();
    sound_podium_free();
    model_free(&g_stand);
    r3d_free_page(3);
    r3d_free_page(4);
    bike3d_free();
}

/* 0x45e390: the two poses a rider goes between, by the place the player took */
static void pose_of(Pose *o, int i, float t)
{
    static const signed char pair[4][4][2] = {
        /* place 1 or 4 */ { { 17, 18 }, { 19, 20 }, { 21, 22 }, { 23, 24 } },
        /* place 2 */      { { 27, 28 }, { 17, 18 }, { 21, 22 }, { -1, -1 } },
        /* place 3 */      { { 25, 26 }, { 19, 20 }, { 17, 18 }, { -1, -1 } },
    };
    const signed char *p = pair[g_place == 2 ? 1 : g_place == 3 ? 2 : 0][i];
    if (p[0] >= 0)
        pose_mix(o, pose_rot(p[0]), pose_rot(p[1]), t);
}

void page_podium_frame(void)
{
    float ms = (float)(g_now - g_last), eye[3];
    int i, y, rows;
    g_last = g_now;
    menu_begin_frame();
    for (y = 0; y < g_scr_h; y++)
        memset(g_screen + y * g_pitch, 0, sizeof(uint16_t) * (size_t)g_scr_w);
    g_t = ms * 0.00014285714f + g_t;
    if (g_t > 1.0f)
        g_t = 1.0f;
    for (i = 0; i < 3; i++)
        eye[i] = (g_to[i] - g_from[i]) * g_t + g_from[i];
    r3d_camera(eye, g_look);
    for (i = 0; g_ok && i < g_count; i++) {
        g_bounce[i] = g_rate[i] * ms * 0.001f + g_bounce[i];
        if (g_bounce[i] > 1.0f) {
            g_bounce[i] = 1.0f;
            g_rate[i] = -g_rate[i];
        } else if (g_bounce[i] < 0.0f) {
            g_bounce[i] = 0.0f;
            g_rate[i] = -g_rate[i];
        }
        pose_of(&g_rd[i].pose, i, g_bounce[i]);
        bike3d_draw_rider(&g_rd[i], 1);
    }
    {
        static const float ang[3] = { 0.0f, 1.5707964f, 0.0f }, at[3] = { 0.0f, 72.0f, -80.0f };
        r3d_object(&g_stand, ang, at);
    }
    if (behaviour_cd())         /* 0x47710b: the stand first, the results over it */
        r3d_render();
    rows = g_t < (float)0.6 ? 0 : g_t < 0.7 ? 1 : g_t < 0.8 ? 2 : 3;
    if (rows) {
        bitmap_step(PIC(P_BEST_RACE), 0, 0);
        bitmap_draw(PIC(P_BEST_RACE), BM_OWN, 100, 0);
    }
    for (i = 0, y = 0xc0; i < rows; i++, y += 0x2c)
        if ((!g_solo || i == g_place - 1) && i < g_count) {
            char s[32];
            int t = g_times[i] / 1000;
            bitmap_place(PIC(P_LINE), 30, y - 12, 0);
            bitmap_draw(PIC(P_LINE), BM_OWN, 100, 0);
            bitmap_place(PIC(P_NUMBER + i), 28, y - 14, 0);
            bitmap_draw(PIC(P_NUMBER + i), BM_OWN, 100, 0);
            font_draw(font_get(12), g_names[i], 110, y, 0, 0);
            snprintf(s, sizeof s, "%02d'%02d''%02d", (t / 60) % 60, t % 60,
                     g_times[i] % 1000 / 10);
            font_draw(font_get(12), s, 480, y, 0, 0);
        }
    menu_transition();
    menu_draw_buttons();
    r3d_render();
    menu_end_frame();
    podium_sound_frame();
    if (menu_pressed() == 0) {
        menu_press_done();
        if (behaviour_cd())     /* 0x47733d: on the 1997 CD the glass breaks on the way */
            menu_capture();
        menu_change_page(9);
    }
}
