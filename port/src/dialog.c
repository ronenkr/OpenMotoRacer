/* The race's dialogs (NOTES §4.19): a dialog is rows of text objects, row 0 the title,
   the others buttons with a normal item (flying in) and a highlighted one (blinking).
   Each button gets a letter of its text as a hotkey, the arrows move between them,
   Return presses; the box behind them grows from its centre and darkens the race. */
#include "mister.h"
#include "dialog.h"
#include "clock.h"
#include "config.h"
#include "exe.h"
#include "font.h"
#include "generic.h"
#include "menu.h"
#include "pointer.h"
#include "race.h"
#include "race_sound.h"
#include "rules.h"
#include "scores.h"
#include "choices.h"
#include "sound.h"
#include "text.h"
#include "video.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ROWS 8
#define CURSOR_ROW 0xe4         /* the rows' pointer (0x433c20) */
#define SCREEN_W g_scr_w
#define SCREEN_H g_scr_h

typedef struct {                /* 0x6c bytes */
    Text *normal, *high;        /* +4, +0xc */
    Text *shown;                /* +0 */
    int is_static;              /* +0x58: the title */
    unsigned char hot[4];       /* +0x68: [0] the letter found each frame */
    void (*action)(void);       /* +0x2c: the hotkey, Return */
    void (*up)(void);           /* +0x20: the left button up */
    void (*down)(void);         /* +0x1c: the left button down */
    int cursor;                 /* +0x34 */
    int (*ticked)(void);        /* +0x60, +0x64: the first letter a tick or a space */
} Row;

typedef struct {                /* 0x78 bytes, 0x4338b0 */
    int kind;                   /* +0x00: 4 results, 5 continue, 6 pause, 7 help */
    int box;                    /* +0x08: the darkened box */
    int keyboard;               /* +0x0c */
    int hold;                   /* +0x10 */
    int hud;                    /* +0x14: the dashboard's other form */
    int race_flags;             /* +0x18: how the race is drawn while held */
    int done;                   /* +0x20 */
    int reserve;                /* +0x1c: the race's messages (0x53ba68) go */
    int box_id;                 /* +0x2c: the box's place in that list */
    int active;                 /* +0x28: each frame's hotkeys and box (0x434960) */
    void (*on_draw)(void);      /* +0x34 */
    int capitals;               /* +0x3c: capitals, the hotkey in small letters */
    int slide;                  /* +0x40, +0x44: the rows slide off when it closes */
    int frames;                 /* +0x60: drawn frames, the box grows over 15 */
    int rect[4];                /* +0x68 */
    Row row[ROWS];              /* +0x58 */
    int nrows;
    int sel;                    /* +0x54: -1 none */
} Dialog;

/* the slots, in the order the window procedure 0x4329a5 asks them: 0x4e8908 the F11 and F12
   columns, 0x4e8910 the message box (the list of screen modes; Direct3D's warnings are not in
   the port), 0x4e890c the help, 0x4e8904 the race's. */
enum { SLOT_SIDE, SLOT_BOX, SLOT_HELP, SLOT_RACE, SLOTS };
static Dialog *g_slot[SLOTS];
static int g_next_side;         /* 0x4e8918's 0x200000, 0x400000: the other column next */
static Dialog *g_dlg;           /* the one at work */
static int g_left_held, g_right_held;   /* 0x4e897c, 0x4e8980 */

int dialog_open(void)
{
    return g_slot[SLOT_RACE] != 0;
}

int dialog_holds(void)
{
    return dialog_flags() != RACE_DRAW_ALL;
}

/* 0x4126a0: the race stands, drawn as the dialog says */
int dialog_flags(void)
{
    int s;
    for (s = 0; s < SLOTS; s++)
        if (g_slot[s] && g_slot[s]->hold)
            return g_slot[s]->race_flags;
    return RACE_DRAW_ALL;
}

static void free_slot(int s)
{
    int i;
    if (!g_slot[s])
        return;
    text_remove(g_slot[s]->box_id);
    for (i = 0; i < g_slot[s]->nrows; i++) {
        text_free(g_slot[s]->row[i].normal);
        text_free(g_slot[s]->row[i].high);
    }
    free(g_slot[s]);
    g_slot[s] = 0;
}

static void layout(void);
static void item_place(int font, int *x, int *y, int *tx, int *ty);

/* 0x4592a0 -> 0x4348f0: every item placed afresh on a new screen, the box after them */
void dialog_screen(void)
{
    int s, i, k;
    Dialog *keep = g_dlg;
    for (s = 0; s < SLOTS; s++) {
        if (!(g_dlg = g_slot[s]))
            continue;
        for (i = 0; i < g_dlg->nrows; i++) {
            Text *t[2] = { g_dlg->row[i].normal, g_dlg->row[i].high };
            for (k = 0; k < 2; k++)
                if (t[k])
                    text_replace(t[k], item_place);
        }
        layout();
    }
    g_dlg = keep;
}

void dialog_free(void)
{
    int s;
    for (s = 0; s < SLOTS; s++)
        free_slot(s);
    g_dlg = 0;
}

/* 0x4354e0: every row's text slides off to the left, faster and faster, fading */
static void close_dialog(void)
{
    int i, s;
    sound_play(0x28, 0);
    for (i = 0; g_dlg->slide && i < g_dlg->nrows; i++) {
        Text *t = g_dlg->row[i].shown;
        TextArgs a;
        if (!t)
            continue;
        text_args_of(t, &a);
        a.lifetime = 10000;
        a.align = 0x21;
        a.effect = FX_FADE_OUT;
        a.motion = 2;
        a.vx = -1;
        a.s = 1.0;
        a.k = 2.0;
        text_add(&a);
    }
    for (s = 0; s < SLOTS; s++)
        if (g_slot[s] == g_dlg)
            free_slot(s);
    g_dlg = 0;
}

/* ------------------------------------------------------------------ building */

static Dialog *new_dialog(int slot, int kind)
{
    free_slot(slot);            /* (the old one goes without its effects) */
    g_dlg = g_slot[slot] = calloc(1, sizeof *g_dlg);
    if (!g_dlg)
        return 0;
    g_dlg->kind = kind;
    g_dlg->keyboard = g_dlg->hold = 1;
    g_dlg->box = g_dlg->active = g_dlg->capitals = g_dlg->slide = g_dlg->reserve = 1;
    g_dlg->race_flags = RACE_DRAW_DIALOG;
    g_dlg->sel = -1;
    return g_dlg;
}

/* 0x4165c0: a font with a low / high resolution partner has its authored places halved
   (small fonts or not 640x480); at 512x384 one with no partner has them scaled */
static void item_place(int font, int *x, int *y, int *tx, int *ty)
{
    if (font_partnered(font) && (g_small_fonts || g_screen_mode != 2)) {
        *x /= 2, *y /= 2, *tx /= 2, *ty /= 2;
    } else if (g_screen_mode == 1) {
        *x = ((*x << 8) / 640 << 9) >> 8, *y = ((*y << 8) / 480) * 384 >> 8;
        *tx = ((*tx << 8) / 640 << 9) >> 8, *ty = ((*ty << 8) / 480) * 384 >> 8;
    }
}

/* an item as 0x433a30's layout leaves it */
static Text *item(const char *s, int font, int x, int y, int tx, int ty, int align,
                  int effect, int motion, int blink, int vx, int vy, double sp, double k,
                  const unsigned char *fore, const unsigned char *back)
{
    TextArgs a;
    int ax = x, ay = y, atx = tx, aty = ty;
    Text *t;
    memset(&a, 0, sizeof a);
    item_place(font, &x, &y, &tx, &ty);
    a.no_gap = 1;
    a.text = s;
    a.font = font;
    a.x = x;
    a.y = y;
    a.tx = tx;
    a.ty = ty;
    a.align = align;
    a.effect = effect;
    a.motion = motion;
    a.blink = blink;
    a.vx = vx;
    a.vy = vy;
    a.s = sp;
    a.k = k;
    if (fore) {
        a.own_colours = 1;
        a.fore = fore;
        a.back = back;
    }
    t = text_new(&a);
    if (t)
        text_keep_place(t, ax, ay, atx, aty);
    return t;
}

static const unsigned char GREEN[3] = { 19, 188, 5 }, YELLOW[3] = { 255, 255, 0 };

static void title(const char *text, int x, int y, int tx, int ty, int vx, int vy, double k)
{
    Row *r = &g_dlg->row[g_dlg->nrows++];
    r->normal = r->shown = item(text, 9, x, y, tx, ty, 0xc2, FX_TRANSP25, 1, 0,
                                vx, vy, 4.0, k, GREEN, YELLOW);
    r->is_static = 1;
}

/* a button in font 32: flying in (or still) and its blinking twin at the target */
static void button(int text, int x, int y, int tx, int ty, int align, int motion, int blink,
                   int vx, int vy, int escape, void (*action)(void))
{
    Row *r = &g_dlg->row[g_dlg->nrows++];
    const char *s = exe_text(text);
    r->normal = r->shown = item(s, 32, x, y, tx, ty, align, FX_NONE, motion, blink, vx, vy,
                                4.0, 6.0, 0, 0);
    r->high = item(s, 32, tx, ty, tx, ty, align, FX_BLINK, 0, 2, 0, 0, 0, 0, 0, 0);
    if (escape)
        r->hot[1] = VK_ESCAPE;
    r->action = r->up = action;
    r->cursor = CURSOR_ROW;
}

/* 0x436150: a row chosen: its items at their places, the highlighted one shown */
static void enter(int i)
{
    Row *r = &g_dlg->row[i];
    if (r->normal)
        text_snap(r->normal);
    if (r->high)
        text_snap(r->high);
    r->shown = r->high ? r->high : r->normal;
    sound_play(0x29, 0);
}

static void select_row(int i)   /* the old one shows its normal item again (0x4361c0) */
{
    if (g_dlg->sel >= 0 && g_dlg->sel != i)
        g_dlg->row[g_dlg->sel].shown = g_dlg->row[g_dlg->sel].normal;
    g_dlg->sel = i;
}

/* a button standing at its place (the results' and the continue's) */
static void still(int text, int x, int y, int align, int escape, void (*action)(void))
{
    Row *r = &g_dlg->row[g_dlg->nrows++];
    const char *s = exe_text(text);
    r->normal = r->shown = item(s, 32, x, y, x, y, align, FX_NONE, 0, -500, 0, 0, 0, 0, 0, 0);
    r->high = item(s, 32, x, y, x, y, align, FX_BLINK, 0, 2, 0, 0, 0, 0, 0, 0);
    if (escape)
        r->hot[1] = VK_ESCAPE;
    r->action = r->up = action;
    r->cursor = CURSOR_ROW;
}

/* ------------------------------------------------------------------ the pause menu */

static void act_restart(void)   /* 0x437610 */
{
    g_dlg->done = 1;
    sound_pause_all(0);
    sound_stop_all();
    if (g_race_kind == 1)
        rules_restore_results();
    if (g_solo)
        scores_write_race();
    race_event(EV_NORMAL | EV_RESTART);
}

static void act_quit(void)      /* 0x437890 */
{
    g_dlg->done = 1;
    sound_pause_all(0);         /* 0x424090(0), then everything stopped (0x424020) */
    sound_stop_all();
    if (g_race_kind == 1)
        rules_restore_results();
    if (g_solo)
        scores_write_race();
    if (g_mode == MODE_CHAMPIONSHIP)
        g_racer[race_local()].state = RS_OUT;
    race_event(EV_NORMAL | EV_UNLOAD | EV_TOP5);
}

static void act_next_demo(void) /* 0x437680 */
{
    g_dlg->done = 1;
    sound_pause_all(0);
    race_event(EV_DEMO_NEXT | EV_DEMO_LOAD);
}

static void act_quit_demo(void) /* 0x4376d0 */
{
    g_dlg->done = 1;
    sound_pause_all(0);
    sound_stop_all();
    race_event(EV_UNLOAD | EV_CLOSE_DEMOS | EV_TITLE);
}

/* 0x437760: a championship race's replay left, on to what the race itself led to */
static void act_quit_replay(void)
{
    int me = race_local();
    g_dlg->done = 1;
    sound_pause_all(0);
    rules_restore_results();
    if (g_solo)
        scores_write_race();
    if (g_racer[me].state == RS_FINISHED && g_racer[me].place <= 2)
        race_event(EV_NORMAL | EV_UNLOAD | EV_TOP5);
    else
        race_event(EV_NORMAL | EV_CONTINUE);
}

static void act_continue(void)  /* 0x437ae0: the keys let go */
{
    g_dlg->done = 1;
    race_release_keys();
    g_ctrl_held = g_shift_held = 0;
    sound_pause_all(0);         /* 0x424090(0): they go on */
}

void dialog_pause(int arg)
{
    if (!new_dialog(SLOT_RACE, 6))
        return;
    sound_pause_all(1);         /* 0x424090(1): the looping sounds paused */
    sound_play(3, 0);
    g_dlg->hud = arg != 1;
    title("MOTO RACER", 0, 200, 0, 5, 0, -1, 5.0);      /* (0x4e8aec, not a word number) */
    if (g_playback) {
        button(107, 300, 0, 0, 0, 0x62, 1, -400, -1, 0, 0, act_next_demo);
        button(106, -300, 22, 0, 22, 0x62, 1, -500, 1, 0, 0, act_quit_demo);
    } else {
        button(104, 300, 0, 0, 0, 0x62, 1, -400, -1, 0, 0, act_restart);
        button(105, -300, 22, 0, 22, 0x62, 1, -500, 1, 0, 0, act_quit);
    }
    if (arg == 1)
        button(g_race_kind == 0 ? 110 : g_playback ? 108 : 109, 0, 200, 0, 44, 0x62, 1, -600,
               0, 1, 1, act_continue);
    if (arg == 1 && g_race_kind == 1 && !g_playback && g_mode == MODE_CHAMPIONSHIP)
        button(111, 0, -200, 0, 66, 0x62, 1, -700, 0, -1, 0, act_quit_replay);
    g_dlg->sel = 1;
    enter(1);
}

/* ------------------------------------------------------------------ the race's end */

/* 0x438690: no replay; the table written; a championship's top three go on, the others
   may try again; after a single race the pause menu without CONTINUE */
static void act_results_no(void)
{
    int me = race_local(), place;
    g_dlg->done = 1;
    rules_restore_results();
    if (g_solo)
        scores_write_race();
    if (g_mode == MODE_CHAMPIONSHIP) {
        place = g_racer[me].state == RS_FINISHED ? g_racer[me].place : rules_place_of(me);
        race_event(EV_NORMAL | (place > 2 ? EV_CONTINUE : EV_UNLOAD | EV_TOP5));
    } else
        race_event(EV_NORMAL | EV_PAUSE_AFTER);
}

static void act_results_yes(void)       /* 0x438650: the race again from its recording */
{
    g_dlg->done = 1;
    race_event(EV_WATCH | EV_RESTART);
}

/* 0x437b70 */
void dialog_results(void)
{
    if (!new_dialog(SLOT_RACE, 4))
        return;
    sound_stop_all();
    sound_play(3, 0);
    if (g_race_kind != 1)
        rules_save_results();
    g_dlg->hud = 1;
    title(exe_text(100), -300, 5, 0, 5, 1, 0, 2.0);    /* [REPLAY] */
    still(101, 10, 0, 0x6a, 0, act_results_yes);
    still(102, 10, 0, 0x63, 1, act_results_no);
    /* ([SAVE] 114 only with the original's -saverecord switch, and its file dialog: not
       written, the game itself has no way of playing such a file back) */
    g_dlg->sel = 1;
    enter(1);
}

static void act_again(void)     /* 0x436210: the same race again */
{
    g_dlg->done = 1;
    race_event(EV_NORMAL | EV_RESTART);
}

static void act_give_up(void)   /* 0x436250 */
{
    g_dlg->done = 1;
    race_event(EV_NORMAL | EV_UNLOAD | (g_mode == MODE_SINGLE ? EV_PAGE6 : EV_TOP5));
}

/* 0x4358b0: practice's end, the time over, a championship race not won */
void dialog_continue(void)
{
    if (!new_dialog(SLOT_RACE, 5))
        return;
    sound_stop_all();
    sound_play(0x44, 0);        /* TRYAGAIN */
    g_dlg->hud = 1;
    title(exe_text(99), -300, 5, 0, 5, 1, 0, 4.0);     /* [CONTINUE] */
    still(101, 10, 0, 0x6a, 0, act_again);
    still(102, 10, 0, 0x63, 0, act_give_up);
    g_dlg->sel = 1;
    enter(1);
}

/* ------------------------------------------------------------------ Shift+F12 */

enum { BOX_MODES = 0xd };       /* +0x00 */

static void act_box_close(void) /* 0x43cf60's button -1 */
{
    g_dlg->done = 1;
}

/* 0x43cf60 -> 0x47ec50: a mode sharing the width or the height with the present one is refused */
#ifdef MISTER_HYBRID
#define MODES MR_COUNT
#else
#define MODES 3
#endif
static void act_mode(void)
{
#ifndef MISTER_HYBRID
    static const int w[3] = { 320, 512, 640 }, h[3] = { 240, 384, 480 };
#endif
    int m = g_dlg->sel - 2;
    g_dlg->done = 1;
#ifdef MISTER_HYBRID
    if (m >= 0 && m < MODES)
        race_ask_screen(m);
#else
    if (m >= 0 && m < 3 && w[m] != g_scr_w && h[m] != g_scr_h)
        race_ask_screen(m);
#endif
}

/* 0x43c2b0 through 0x43c570: the game's three sizes in place of Windows' list; each a letter
   from A up that no control has taken */
void dialog_modes(void)
{
    static const unsigned char TITLE_FORE[3] = { 255, 127, 10 }, TITLE_BACK[3] = { 200, 100, 0 };
#ifdef MISTER_HYBRID
    static const int w[MODES] = { 320, 640 }, h[MODES] = { 240, 240 };
#else
    static const int w[3] = { 320, 512, 640 }, h[3] = { 240, 384, 480 };
#endif
    static char text[MODES][32];
    int i, a, hh, ww, step, y = 0, letter = 'A';
    Font *f = font_get(32);
    if (g_slot[SLOT_BOX]) {
        g_dlg = g_slot[SLOT_BOX];
        close_dialog();
        return;
    }
    if (!f || !new_dialog(SLOT_BOX, BOX_MODES))
        return;
    g_dlg->keyboard = g_dlg->hold = g_dlg->box = g_solo;
    g_dlg->capitals = g_dlg->reserve = 0;
    font_measure(f, "[ ]******[   X    Y  ]", &ww, &hh, 0);
    step = g_small_fonts || g_screen_mode != 2 ? 2 * (hh + 10) : hh + 10;
    {
        Row *r = &g_dlg->row[g_dlg->nrows++];
        r->normal = r->shown = item(exe_text(43), 12, 0, hh + 5, 0, hh + 5, 0xc2, FX_BLINK, 0,
                                    -333, 0, 0, 0, 0, TITLE_FORE, TITLE_BACK);
        r->is_static = 1;
        r = &g_dlg->row[g_dlg->nrows++];
        r->normal = r->shown = item("[ ]******[   X    Y  ]", 32, 0, 0, 0, 0, 0xc2, FX_NONE, 0, 0,
                                    0, 0, 0, 0, 0, 0);
        r->is_static = 1;
    }
    for (i = 0; i <= MODES; i++, y += step) {
        Row *r = &g_dlg->row[g_dlg->nrows++];
        const char *s = exe_text(103);  /* [CANCEL] */
        if (i < MODES) {
            int taken;
            do {
                taken = 0;
                for (a = 0; a < ACTIONS; a++)
                    taken |= toupper(g_binding[a][0] & 0xff) == letter;
                if (taken)
                    letter++;
            } while (taken);
            snprintf(text[i], sizeof text[i], "[%c]******[%4d X %-4d]", letter < 'X' ? letter : ' ',
                     w[i], h[i]);
            letter++;
            s = text[i];
        } else
            r->hot[1] = VK_ESCAPE;
        r->normal = r->shown = item(s, 32, 0, y, 0, y, 0x62, FX_NONE, 0, 0, 0, 0, 0, 0, 0, 0);
        r->high = item(s, 32, 0, y, 0, y, 0x62, FX_NONE, 0, 0, 0, 0, 0, 0, 0, 0);
        r->action = r->up = i < MODES ? act_mode : act_box_close;
        r->cursor = CURSOR_ROW;
    }
    g_dlg->sel = -1;
    g_dlg = 0;
}

/* ------------------------------------------------------------------ the help (F1) */

static void act_help_next(void) /* 0x43df70 */
{
    if (help_at() < help_pages() - 1)
        help_page(help_at() + 1);
}

static void act_help_previous(void)     /* 0x43df40 */
{
    if (help_at() > 0)
        help_page(help_at() - 1);
}

static void act_help_first(void)        /* 0x43def0 */
{
    help_page(0);
}

static void act_help_last(void) /* 0x43df10 */
{
    help_page(help_pages() - 1);
}

static void act_help_down_five(void)    /* 0x43dff0 */
{
    if (help_at() < help_pages() - 1) {
        int p = help_at() + 5;
        help_page(p >= help_pages() ? help_pages() - 1 : p);
    }
}

static void act_help_up_five(void)      /* 0x43dfa0 */
{
    if (help_at() > 0) {
        int p = help_at() - 5;
        help_page(p < 0 ? 0 : p);
    }
}

static void act_help_close(void)        /* 0x43e040: the race goes on */
{
    g_dlg->done = 1;
    help_close();
    music_pause(0);             /* 0x4096e0(0) */
    sound_pause_all(0);         /* 0x409700(0) */
    race_release_keys();
}

/* a key cap at the screen's right edge: one character in KeyboardHelp, still, and its blinking
   twin when the pointer is on it */
static void help_key(int ch, int y, int align, void (*action)(void), int k0, int k1, int k2)
{
    Row *r = &g_dlg->row[g_dlg->nrows++];
    char s[2] = { (char)ch, 0 };
    r->normal = r->shown = item(s, 30, 5, y, 0, 0, align, FX_NONE, 0, -500, 0, 0, 0, 0, 0, 0);
    r->high = item(s, 30, 5, y, 0, 0, align, FX_BLINK, 0, 2, 0, 0, 0, 0, 0, 0);
    r->hot[0] = (unsigned char)k0;
    r->hot[1] = (unsigned char)k1;
    r->hot[2] = (unsigned char)k2;
    r->action = r->up = action;
    r->cursor = CURSOR_ROW;
}

/* a key with no picture and no place on the screen (the rows of item 6) */
static void help_key_only(int vk, void (*action)(void))
{
    Row *r = &g_dlg->row[g_dlg->nrows++];
    r->hot[0] = (unsigned char)vk;
    r->action = action;
}

/* 0x43cf90: Help.gen, the race held, nothing but the road under it */
int dialog_help(void)
{
    if (!help_open())
        return 0;
    if (!new_dialog(SLOT_HELP, 7)) {
        help_close();
        return 0;
    }
    g_dlg->hud = 1;
    g_dlg->race_flags = RACE_DRAW_HELP;
    g_dlg->on_draw = help_draw;
    g_dlg->active = g_dlg->box = g_dlg->slide = g_dlg->capitals = 0;
    /* (no sounds of its own, no row chosen at first) */
    help_key(';', 2, 0x68, act_help_next, VK_DOWN, VK_RIGHT, 0);
    help_key('9', 2, 0xc8, act_help_previous, VK_UP, VK_LEFT, 0);
    help_key(0x83, 5, 0x88, act_help_close, 0x70, VK_ESCAPE, VK_RETURN);
    help_key_only(VK_HOME, act_help_first);
    help_key_only(VK_END, act_help_last);
    help_key('5', 28, 0x68, act_help_down_five, VK_NEXT, 0, 0);
    help_key('4', 28, 0xc8, act_help_up_five, VK_PRIOR, 0, 0);
    music_pause(1);             /* 0x4096e0(1) */
    sound_pause_all(1);         /* 0x409700(1) */
    return 1;
}

/* ------------------------------------------------------------------ F11 and F12 */

enum { SIDE_DISPLAY = 1, SIDE_SOUND = 3 };      /* +0x00 */

static void act_side_hide(void) /* 0x43a2b0 */
{
    g_dlg->done = 1;
}

static void act_side_sound(void)        /* 0x43a3d0: 0x4e8918 = 0x400000 */
{
    g_dlg->done = 1;
    g_next_side = SIDE_SOUND;
}

static void act_side_display(void)      /* 0x43b6d0: 0x4e8918 = 0x200000 */
{
    g_dlg->done = 1;
    g_next_side = SIDE_DISPLAY;
}

/* the screen's size (0x43a2d0, 0x43a300, 0x43a320) and the 16:9 band (0x43a370) */
static void act_side_low(void)
{
    race_ask_screen(0);
}

static void act_side_medium(void)
{
    race_ask_screen(1);
}

static void act_side_high(void)
{
    race_ask_screen(2);
}

/* a row of a column: the short tag flies in from the left edge, the long text shows while the
   pointer is on it. Pressed as the button goes down; no row is chosen at first. */
static void side_row(int i, int y, int tag, int text, void (*action)(void), int (*ticked)(void))
{
    Row *r = &g_dlg->row[i];
    r->normal = r->shown = item(exe_text(tag), 32, -60, y, 0, y, 0x40, FX_NONE, 1, 0, 1, 0,
                                4.0, 7.0, 0, 0);
    r->high = item(exe_text(text), 32, 0, y, 0, y, 0x40, FX_NONE, 0, -500, 0, 0, 0, 0, 0, 0);
    r->action = r->down = action;
    r->cursor = CURSOR_ROW;
    r->ticked = ticked;
}

static void side_begin(int kind)
{
    if (!new_dialog(SLOT_SIDE, kind))
        return;
    g_dlg->hold = g_dlg->keyboard = g_dlg->box = g_dlg->reserve = 0;
    g_dlg->nrows = kind == SIDE_DISPLAY ? 8 : 7;
    /* (the rows left out stay static and empty) */
    {
        int i;
        for (i = 0; i < g_dlg->nrows; i++)
            g_dlg->row[i].is_static = 1;
    }
}

static void side_row_on(int i)
{
    g_dlg->row[i].is_static = 0;
}

/* 0x438f70 */
static void dialog_display_menu(void)
{
    side_begin(SIDE_DISPLAY);
    if (!g_dlg)
        return;
    side_row(0, -20, 138, 141, act_side_low, 0);
    side_row(1, -40, 137, 140, act_side_medium, 0);
    side_row(2, -60, 136, 139, act_side_high, 0);
    side_row(3, 10, 142, 143, race_ask_169, race_is_169);
    side_row(4, 50, 147, 148, video_toggle_fullscreen, video_fullscreen);
    side_row(6, 80, 149, 150, act_side_sound, 0);
    side_row(7, 100, 151, 152, act_side_hide, 0);
    side_row_on(0), side_row_on(1), side_row_on(2), side_row_on(3);
    side_row_on(4), side_row_on(6), side_row_on(7);
    g_dlg = 0;
}

/* 0x43a400 */
static void dialog_sound_menu(void)
{
    int i;
    side_begin(SIDE_SOUND);
    if (!g_dlg)
        return;
    side_row(0, -50, 153, 154, race_sound_toggle_music, race_sound_music_on);
    side_row(1, 0, 157, 158, race_sound_toggle_engine, race_sound_engine_on);
    side_row(2, -20, 155, 156, race_sound_toggle_effects, sound_effects_on);
    side_row(3, 50, 161, 162, race_sound_toggle_voices, sound_voices_on);
    side_row(4, 80, 163, 164, act_side_display, 0);
    side_row(5, 100, 151, 152, act_side_hide, 0);
    side_row(6, 20, 159, 160, race_sound_toggle_ambience, race_sound_ambience_on);
    for (i = 0; i < 7; i++)
        side_row_on(i);
    g_dlg = 0;
}

/* 0x432200..: the same key again closes the column, the other key changes it */
void dialog_side(int kind)
{
    if (g_slot[SLOT_SIDE]) {
        int same = g_slot[SLOT_SIDE]->kind == kind;
        g_dlg = g_slot[SLOT_SIDE];
        close_dialog();
        if (same)
            return;
    }
    if (kind == SIDE_SOUND)
        dialog_sound_menu();
    else
        dialog_display_menu();
}

/* ------------------------------------------------------------------ each frame */

/* 0x434860: the box round the rows' shown items (where the flying ones are going) */
static void layout(void)
{
    int i, *b = g_dlg->rect;
    b[0] = b[1] = 0xffff;
    b[2] = b[3] = 0;
    for (i = 0; i < g_dlg->nrows; i++) {
        int r[4];
        Text *t = g_dlg->row[i].shown;
        if (!t || !text_alive(t))
            continue;
        text_rect(t, r);
        if (r[0] < b[0]) b[0] = r[0];
        if (r[1] < b[1]) b[1] = r[1];
        if (r[2] > b[2]) b[2] = r[2];
        if (r[3] > b[3]) b[3] = r[3];
    }
}

/* 0x43e0f0 + 0x434960: each button's hotkey, the first letter of its text that is not
   a control key's (nor one an earlier row took) */
void dialog_frame(void)
{
    unsigned char used[64];
    int n = 0, a, i, s;
    static const int order[SLOTS] = { SLOT_BOX, SLOT_HELP, SLOT_RACE, SLOT_SIDE };
    if (g_next_side) {          /* 0x4126a0 -> 0x4587e0 */
        int kind = g_next_side;
        g_next_side = 0;
        dialog_side(kind);
    }
    for (a = 0; a < ACTIONS; a++)
        used[n++] = (unsigned char)toupper(g_binding[a][0] & 0xff);
    for (i = 0x70; i <= 0x77; i++)
        used[n++] = (unsigned char)i;
    used[n++] = 9;
    for (s = 0; s < SLOTS; s++) {
      g_dlg = g_slot[order[s]];
      if (!g_dlg || !g_dlg->active)
        continue;
      for (i = 0; i < g_dlg->nrows; i++) {
        Row *r = &g_dlg->row[i];
        const unsigned char *s;
        if (r->is_static || !r->shown || !r->normal)
            continue;
        r->hot[0] = 0;
        for (s = (const unsigned char *)text_string(r->normal); *s; s++) {
            int c = toupper(*s), k;
            if (!isalpha(*s))
                continue;
            for (k = 0; k < n && used[k] != c; k++)
                ;
            if (k == n) {
                r->hot[0] = (unsigned char)c;
                if (n < 64)
                    used[n++] = (unsigned char)c;
                break;
            }
        }
      }
      layout();
    }
    g_dlg = 0;
}

/* ------------------------------------------------------------------ keys */

/* 0x435090: the nearest row that way (no wrapping round) */
static int nearest(int vk)
{
    int from[4], i, best = -1;
    unsigned bestkey = 0;
    if (g_dlg->sel >= 0 && text_alive(g_dlg->row[g_dlg->sel].shown))
        text_now(g_dlg->row[g_dlg->sel].shown, from);
    else {
        from[0] = SCREEN_W - 1, from[1] = SCREEN_H - 1;
        from[2] = from[3] = 0;
    }
    for (i = 0; i < g_dlg->nrows; i++) {
        Row *r = &g_dlg->row[i];
        int b[4];
        unsigned key;
        if (r->is_static || !r->shown || !text_alive(r->shown))
            continue;
        text_now(r->shown, b);
        switch (vk) {
        case VK_LEFT:
            if (b[2] >= from[0])
                continue;
            key = (unsigned)b[2] << 16 | (unsigned)abs((from[1] + from[3]) / 2 - (b[1] + b[3]) / 2) << 8;
            break;
        case VK_RIGHT:
            if (b[0] <= from[2])
                continue;
            key = (unsigned)b[0] << 16 | (unsigned)abs((from[1] + from[3]) / 2 - (b[1] + b[3]) / 2) << 8;
            break;
        case VK_UP:
            if (b[3] >= from[1])
                continue;
            key = (unsigned)b[3] << 16 | (unsigned)abs((from[0] + from[2]) / 2 - (b[0] + b[2]) / 2) << 8;
            break;
        default:
            if (b[1] <= from[3])
                continue;
            key = (unsigned)b[1] << 16 | (unsigned)abs((from[0] + from[2]) / 2 - (b[0] + b[2]) / 2) << 8;
            break;
        }
        key |= (unsigned)i;
        /* left and up: the largest first (0x4353d0); right and down: the smallest */
        if (best < 0 || (vk == VK_LEFT || vk == VK_UP ? key > bestkey : key < bestkey))
            best = i, bestkey = key;
    }
    return best;
}

/* 0x433ec0's keys, for the dialog at work */
static int key_one(int vk, int down)
{
    Dialog *d = g_dlg;
    int i, k, taken = 0;
    if (!down) {
        /* (Ctrl and Shift let go go on to the race's handler 0x432700, which clears them) */
        if (vk == VK_CONTROL || vk == VK_SHIFT)
            return 0;
        return d->keyboard;
    }
    if (vk == VK_CONTROL || vk == VK_SHIFT) {
        *(vk == VK_CONTROL ? &g_ctrl_held : &g_shift_held) = 1;
        return 1;
    }
    if (g_ctrl_held)
        return vk == 'F';
    if (g_shift_held || (vk >= 0x71 && vk <= 0x73) || vk == 0x6a || vk == 0x6f)
        return 0;
    for (i = 0; i < d->nrows && !taken; i++)
        for (k = 0; k < 4; k++)
            if (d->row[i].hot[k] && d->row[i].hot[k] == vk) {
                select_row(i);
                if (d->row[i].action)
                    d->row[i].action();
                taken = 1;
                break;
            }
    if (!taken && d->keyboard) {
        if (vk == VK_LEFT || vk == VK_RIGHT || vk == VK_UP || vk == VK_DOWN) {
            int r = nearest(vk);
            if (r >= 0) {
                select_row(r);
                enter(r);
            }
        } else if (vk == VK_RETURN && d->sel >= 0 && d->row[d->sel].action)
            d->row[d->sel].action();
    }
    return taken || d->keyboard;
}

/* 0x4329a5: asked in turn until one takes it; closed if that one is done */
int dialog_key(int vk, int down)
{
    int s;
    for (s = 0; s < SLOTS; s++) {
        if (!(g_dlg = g_slot[s]) || !key_one(vk, down))
            continue;
        if (g_dlg->done)
            close_dialog();
        g_dlg = 0;
        return 1;
    }
    g_dlg = 0;
    return 0;
}

/* ------------------------------------------------------------------ the mouse */

/* 0x435430: the first row whose shown item, where it is now, has the point */
static int row_at(int x, int y)
{
    int i, b[4];
#ifdef OMR_ESP32
    int best = -1, bd = 0, reach = g_scr_w / 7;     /* (40 of the LCD's pixels) */
#endif
    for (i = 0; i < g_dlg->nrows; i++) {
        const Row *r = &g_dlg->row[i];
        if (r->is_static || !r->shown || !text_alive(r->shown))
            continue;
        text_now(r->shown, b);
        if (b[0] <= x && x <= b[2] && b[1] <= y && y <= b[3])
            return i;
#ifdef OMR_ESP32
        {   /* the board: off every row, the nearest within reach (menu.c's menu_hit) */
            int dx = x < b[0] ? b[0] - x : x > b[2] ? x - b[2] : 0;
            int dy = y < b[1] ? b[1] - y : y > b[3] ? y - b[3] : 0, d = dx * dx + dy * dy;
            if (d < reach * reach && (best < 0 || d < bd))
                best = i, bd = d;
        }
#endif
    }
#ifdef OMR_ESP32
    return best;
#else
    return -1;
#endif
}

/* 0x433ec0, 0x200: hovering chooses a row, and off the rows none is chosen. Not while a
   button is held. 1 if it was taken. */
static int mouse_move(int x, int y)
{
    int r;
    if (g_left_held || g_right_held)
        return 1;
    r = row_at(x, y);
    if (r == g_dlg->sel)
        return r >= 0;
    select_row(r);
    if (r >= 0)
        enter(r);
    return r >= 0;
}

/* 0x433ec0, 0x201 and 0x202: a press belongs to the row under the button going down and
   happens as it comes up, wherever the pointer is by then */
static int mouse_left(int x, int y, int down)
{
    if (down) {
        int r = row_at(x, y);
        g_left_held = 1;
        select_row(r);
        if (r >= 0 && g_dlg->row[r].down)
            g_dlg->row[r].down();
        return r >= 0;
    }
    g_left_held = 0;
    if (g_dlg->sel < 0 || !g_dlg->row[g_dlg->sel].up)
        return 0;
    g_dlg->row[g_dlg->sel].up();
    return 1;
}

void dialog_mouse_move(int x, int y)
{
    int s;
    pointer_touched(x, y);
    for (s = 0; s < SLOTS; s++) {
        if (!(g_dlg = g_slot[s]) || !mouse_move(x, y))
            continue;
        break;
    }
    g_dlg = 0;
}

void dialog_mouse_button(int left, int down)
{
    int s;
    for (s = 0; s < SLOTS; s++) {
        if (!(g_dlg = g_slot[s]))
            continue;
        if (!left) {
            g_right_held = down;        /* (no row answers it: +0x28 is 0) */
            continue;
        }
        if (mouse_left(g_mx, g_my, down)) {     /* (the place the last move left it) */
            if (g_dlg->done)
                close_dialog();
            break;
        }
    }
    g_dlg = 0;
}

/* the pointer's shape after a message (0x4329a5, 0x4e8960): the last dialog asked has its say,
   the row's or "no" off the rows; a dialog that does not take the keyboard none */
static void set_pointer(void)
{
    int s, shape = CUR_ARROW;
    for (s = 0; s < SLOTS; s++) {
        const Dialog *d = g_slot[s];
        if (!d)
            continue;
        if (d->sel >= 0)
            shape = d->row[d->sel].cursor;
        else if (d->keyboard)
            shape = CUR_NO;
    }
    pointer_shape(shape);
}

/* ------------------------------------------------------------------ drawing */

/* 0x49a5a0: the inside at half brightness, a 5-pixel bevel from 230/256 to 128/256 */
static uint16_t scale(uint16_t p, int s)
{
    return (uint16_t)((((p & 0xf800u) * (unsigned)s >> 8) & 0xf800u)
                      | (((p & 0x07e0u) * (unsigned)s >> 8) & 0x07e0u)
                      | (((p & 0x001fu) * (unsigned)s >> 8) & 0x001fu));
}

void dialog_shade(int x0, int y0, int x1, int y1)
{
    const int E = 5;
    int x, y;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > g_scr_w - 1) x1 = g_scr_w - 1;
    if (y1 > g_scr_h - 1) y1 = g_scr_h - 1;
    if (x1 <= x0 || y1 <= y0)
        return;
    VIDEO_ROWS(y0, y1);
    for (y = y0; y <= y1; y++) {
        uint16_t *p = g_screen + y * g_pitch;
        int t = -1;
        if (y < y0 + E - 1)
            t = y0 - y - 1 + E;
        else if (y > y1 - E + 1)
            t = y - y1 - 1 + E;
        for (x = x0; x <= x1; x++) {
            int s;
            if (x < x0 + E && (t < 0 || x < x0 + E - t))
                s = ((x0 - x + E - 1) * 256 / E + 256) >> 1;
            else if (x > x1 - E && (t < 0 || x > x1 - E + t))
                s = ((x - x1 + E - 1) * 256 / E + 256) >> 1;
            else if (t >= 0)
                s = (t * 256 / E + 256) >> 1;
            else {
                p[x] = (uint16_t)((p[x] >> 1) & ~0x0410u);
                continue;
            }
            p[x] = scale(p[x], s);
        }
    }
}

/* 0x434ae0 */
static void draw_one(void)
{
    Dialog *d = g_dlg;
    int i, b[4];
    d->frames += g_ticks;
    if (d->on_draw)
        d->on_draw();
    /* +0x1c: the race's message list cleared once, and the box's place taken in it, for new
       messages to keep out of (0x434ae0) */
    if (d->reserve) {
        d->reserve = 0;
        layout();
        text_clear_list(1);
        text_reserve(1, d->rect[0], d->rect[1], d->rect[2] - d->rect[0] + 1,
                 d->rect[3] - d->rect[1] + 1, 0, &d->box_id);
    }
    layout();
    memcpy(b, d->rect, sizeof b);
    if (d->frames < 15) {
        double f = d->frames * (1.0 / 15);
        int hw = (b[2] - b[0]) / 2, hh = (b[3] - b[1]) / 2;
        int cx = (b[0] + b[2]) / 2, cy = (b[1] + b[3]) / 2;
        b[0] = (int)(cx - hw * f);
        b[2] = (int)(cx + hw * f);
        b[1] = (int)(cy - hh * f);
        b[3] = (int)(cy + hh * f);
    }
    if (d->box)
        dialog_shade(b[0] - 5, b[1] - 5, b[2] + 5, b[3] + 5);
    for (i = 0; i < d->nrows; i++) {
        Row *r = &d->row[i];
        char *s;
        if (!r->shown)
            continue;
        if (r->ticked)          /* the fonts 2, 6, 32 and 33 have it, as '^' */
            text_string(r->shown)[0] = r->ticked() ? '^' : ' ';
        if (!r->is_static && d->capitals) {     /* the hotkey in the font's small letters */
            int lowered = 0;
            for (s = text_string(r->shown); *s; s++) {
                if (*s >= 'a' && *s <= 'z')
                    *s = (char)(*s - 'a' + 'A');
                if (!lowered && *s >= 'A' && *s <= 'Z'
                    && memchr(r->hot, (unsigned char)*s, 4)) {
                    *s = (char)(*s - 'A' + 'a');
                    lowered = 1;
                }
            }
        }
        text_draw_one(r->shown);
    }
}

/* 0x47ff90 step 19 */
void dialog_draw(void)
{
    static const int order[SLOTS] = { SLOT_SIDE, SLOT_RACE, SLOT_HELP, SLOT_BOX };
    int s;
    set_pointer();
    for (s = 0; s < SLOTS; s++)
        if ((g_dlg = g_slot[order[s]]) != 0)
            draw_one();
    g_dlg = 0;
}
