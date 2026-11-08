/* The front end's machinery (menu\menu.cpp, menuTools.cpp; NOTES §4.4): the menu object,
   its pages, the page change, the buttons and how the keyboard and mouse work them. */
#include "menu.h"
#include "font.h"
#include "race.h"
#include "sound.h"
#include "clock.h"
#include "pages.h"
#include "files.h"
#include "r3d.h"
#include "rand.h"
#include "text.h"
#include "video.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Menu g_menu;
Bitmap g_pics[MENU_PICS];
unsigned g_mouse;
int g_mx, g_my;
int g_pocket_unlocked, g_pocket_unlocked2, g_reverse_unlocked, g_reverse_unlocked2;
int g_pocket, g_reverse;
int g_island_unlocked;
int g_quit;

static int g_mods;              /* 0x4dbfc8: 1 Alt, 2 Shift, 4 Ctrl */
static int g_navigated;         /* 0x504e78: Tab or the arrows were used on this page */
static int g_changing;          /* 0x504e74: a page change is under way */
static int g_capture_pending;   /* 0x4dbd00: do the glass transition at the next start */
static int g_blink;             /* 0x4dbd08 */

/* the page table 0x4dbb38: frame, start and end routines (0 = none); pages the port has
   not written yet have none and end the program when reached */
static const struct {
    void (*frame)(void), (*start)(void), (*end)(void);
    const char *what;
} pages[MENU_PAGES] = {
    [1] = { page_load_frame, 0, 0, "loading" },
    [3] = { page_main_frame, page_main_start, 0, "main menu" },
    [4] = { page_quit_frame, 0, 0, "quit" },
    [5] = { page_mode_frame, page_mode_start, 0, "select mode" },
    [6] = { page_track_frame, page_track_start, page_track_end, "select race" },
    [9] = { page_top5_frame, page_top5_start, page_top5_end, "TOP 5" },
    [10] = { page_bike_frame, page_bike_start, page_bike_end, "choose bike" },
    [11] = { page_options_frame, page_options_start, 0, "options" },
    [12] = { page_settings_frame, page_settings_start, 0, "settings" },
    [13] = { page_diag_frame, page_diag_start, 0, "diagnostic" },
    [14] = { page_title_frame, page_title_start, page_title_end, "title" },
    [15] = { page_name_frame, page_name_start, page_name_end, "name entry" },
    [16] = { page_reverse_frame, page_unlock_start, page_unlock_end, "Reverse unlocked" },
    [17] = { page_pocket_frame, page_unlock_start, page_unlock_end, "Pocket unlocked" },
    [18] = { page_won_frame, page_unlock_start, page_unlock_end, "championship won" },
    [20] = { page_load_champ_frame, page_load_champ_start, 0, "championship load" },
    [21] = { page_save_champ_frame, page_save_champ_start, 0, "championship save" },
    [23] = { page_datadisk_frame, page_datadisk_start, page_datadisk_end, "data disk" },
    [24] = { page_podium_frame, page_podium_start, page_podium_end, "podium" },
    [25] = { page_logo_frame, page_logo_start, page_logo_end, "Delphine logo" },
    [26] = { page_mode_frame, page_mode_start, 0, "select mode (26)" },
};

/* the game has routines for every page but 0 */
static const char *const page_names[MENU_PAGES] = {
    "nothing", "loading", "network", "main menu", "quit", "solo", "track choice", "7",
    "8", "times", "10", "options", "12", "3D card", "title", "name entry", "16", "17", "18",
    "19", "championship saves", "championship saves", "22", "23", "podium",
    "Delphine logo", "solo (26)"
};

/* the pane of glass (0x504c70, NOTES §4.4.2.4): vitre.3df's 99 triangles, each turned
   into a piece of its own about its centre (0x479630) */
typedef struct {
    float v[3][3];              /* the corners, from the centre */
    float c[3];                 /* +0x8c: the centre */
    float spin[3];              /* +0x90: radians over the whole transition */
    Face f[2];                  /* both windings */
} Shard;

static Shard *g_glass;
static int g_nglass;

/* 0x458cd0: the menu's random numbers */
static int rand_stream1(void)
{
    return rand_next(1);
}

static float le_float(const unsigned char *p)
{
    uint32_t u = (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16
                 | (uint32_t)p[3] << 24;
    float f;
    memcpy(&f, &u, 4);
    return f;
}

static void load_glass(void)
{
    long len, p;
    unsigned char *d = dsi_load("data\\menu\\meshes\\vitre.3df", &len);
    int nv, n = 0, i, k;
    if (!d || len < 4 || (d[0] | d[1] << 8) != 1)
        goto bad;
    nv = d[2] | d[3] << 8;
    for (p = 4 + 12L * nv; p + 2 <= len && (d[p] | d[p + 1] << 8); p += 4 + 2L * (d[p] | d[p + 1] << 8))
        n++;
    for (i = 0; i < n; i++)    /* the loader (0x420590) draws a colour for each face */
        rand_stream1();
    free(g_glass);
    if (!(g_glass = calloc(n ? n : 1, sizeof *g_glass)))
        goto bad;
    g_nglass = 0;
    for (p = 4 + 12L * nv; p + 2 <= len && (d[p] | d[p + 1] << 8); p += 4 + 2L * (d[p] | d[p + 1] << 8)) {
        Shard *s = &g_glass[g_nglass];
        float sx = 0, sy = 0;
        int u[3], v[3];
        if ((d[p] | d[p + 1] << 8) != 3 || p + 10 > len)
            continue;
        for (k = 0; k < 3; k++) {
            int ix = d[p + 4 + 2 * k] | d[p + 5 + 2 * k] << 8;
            if (ix >= nv)
                goto bad;
            for (i = 0; i < 3; i++)                     /* (no x96 in this loader) */
                s->v[k][i] = le_float(d + 4 + 12 * ix + 4 * i);
            sx += s->v[k][0];
            sy += s->v[k][1];
        }
        s->c[0] = sx * 0.333333343f;
        s->c[1] = sy * 0.333333343f;
        s->c[2] = 0.0f;
        /* the spins: (r - 32767) / 32767 times 1.8pi, 1.5pi, 1.7pi */
        s->spin[2] = (float)((rand_stream1() - 32767.0f) * 3.05185094e-05f * 5.340707511102648);
        s->spin[1] = (float)((rand_stream1() - 32767.0f) * 3.05185094e-05f * 4.71238898038469);
        s->spin[0] = (float)((rand_stream1() - 32767.0f) * 3.05185094e-05f * 5.654866776461628);
        for (k = 0; k < 3; k++) {       /* the old screen's 256x192 picture across it */
            u[k] = (int)((s->v[k][0] + 320.0f) * 256.0f * 0.0015625f);
            v[k] = (int)((s->v[k][1] + 240.0f) * 192.0f * -0.00208333344f + 192.0f);
            s->v[k][0] -= s->c[0];
            s->v[k][1] -= s->c[1];
            s->v[k][2] = 0.0f;
        }
        for (i = 0; i < 2; i++) {
            static const int order[2][3] = { { 0, 1, 2 }, { 0, 2, 1 } };
            Face *f = &s->f[i];
            f->kind = 0x10;
            f->page = 2;
            f->flags = 4;
            f->n = 3;
            for (k = 0; k < 3; k++) {
                f->idx[k] = order[i][k];
                f->ut[k] = (float)u[order[i][k]];   /* (x 8192: the affine texels) */
                f->vt[k] = (float)v[order[i][k]];
            }
        }
        g_nglass++;
    }
    free(d);
    return;
bad:
    fprintf(stderr, "openmotoracer: the glass (vitre.3df) could not be read\n");
    free(d);
    g_nglass = 0;
}

/* 0x479c10: each piece tumbles about its centre and comes at the viewer, z 1911 to 61 */
static void queue_glass(float t)
{
    int i;
    for (i = 0; i < g_nglass; i++) {
        const Shard *s = &g_glass[i];
        float ang[3], m[3][3], pos[3];
        ang[0] = t * s->spin[0];
        ang[1] = t * s->spin[1];
        ang[2] = t * s->spin[2];
        r3d_rotation(m, ang, 2607.594587618272);        /* 0x4d1878 */
        pos[0] = s->c[0];
        pos[1] = s->c[1];
        pos[2] = (float)((double)t * -1850.0 + 1911.0 + s->c[2]);
        r3d_view(3, (const float (*)[3])s->v, 2, s->f, (const float (*)[3])m, pos);
    }
}

/* 0x40a9a0: the menu's resources, loaded at the first entry */
static int load_resources(char *err, int errsize)
{
    int id;
    load_glass();
    sound_menu_load();          /* 0x403f90 */
    for (id = 0; id < MENU_PICS; id++)
#ifdef OMR_ESP32
        if (!g_pics[id].pix && !g_pics[id].id && !bitmap_load_id_lazy(&g_pics[id], id)) {
#else
        if (!g_pics[id].pix && !bitmap_load_id(&g_pics[id], id)) {
#endif
            snprintf(err, errsize, "The menu picture %d could not be read.", id);
            return 0;
        }
    return 1;
}

/* 0x409f70 */
static void set_page(int page)
{
    if (!g_menu.active) {
        /* the first entry (or back from a race): 640x480, focal length 1911 (for the 3D
           pages, not yet), the resources */
        g_menu.active = 1;
        g_menu.sel = g_menu.prev_sel = g_menu.pressed = -1;
        g_menu.mouse_mode = 0;
        sound_menu_load();      /* 0x40a9a0's 0x403f90 (after a race: its set is gone) */
        sound_queue_reset();    /* 0x404d50 */
    }
    g_menu.last_input = g_now;
    g_menu.started[page] = 0;
    g_menu.page = page;
    g_menu.key = 0;
    g_mouse = 0;
    g_menu.prev_mouse = 0;
}

void menu_change_page(int page)
{
    /* (0x409eb0 also frees unpacked pictures) */
    text_clear();
    g_menu.sel = -1;
    g_navigated = 0;
    g_menu.mouse_mode = 1;
    g_changing = 1;
    if (g_menu.started[g_menu.page]) {
        if (pages[g_menu.page].end)
            pages[g_menu.page].end();
        g_menu.started[g_menu.page] = 0;
    }
    set_page(page);
}

/* page 1's teardown (an inline 0x409eb0, SetPage(0), every started page's end, 0x40b1f0):
   the race takes over. (The port keeps the menu's pictures loaded.) */
void menu_leave(void)
{
    int i;
    text_clear();
    g_menu.sel = -1;
    g_menu.mouse_mode = 1;
    for (i = 0; i < MENU_PAGES; i++)
        if (g_menu.started[i]) {
            if (pages[i].end)
                pages[i].end();
            g_menu.started[i] = 0;
        }
    g_menu.page = 0;
    g_menu.active = 0;
}

int menu_start(int page, char *err, int errsize)
{
    memset(&g_menu, 0, sizeof g_menu);
    g_menu.sel = -1;
    if (!load_resources(err, errsize))
        return 0;
    menu_change_page(page);
    return 1;
}

/* 0x479260: the back buffer into a 256x192 texture, keeping columns and rows 0, 2 of every
   5 (0, 2, 5, 7, 10, ...) */
static void capture_screen(void)
{
    uint16_t *tex = r3d_page(2);
    int x, y;
    if (!tex)
        return;
    for (y = 0; y < 192; y++) {
        const uint16_t *row = g_screen + ((y >> 1) * 5 + (y & 1) * 2) * g_pitch;
        for (x = 0; x < 256; x++)
            tex[y * 256 + x] = row[(x >> 1) * 5 + (x & 1) * 2];
    }
}

void menu_capture(void)
{
    g_capture_pending = 1;
    capture_screen();
}

void menu_frame(void)
{
    int page = g_menu.page;
    text_clock(g_now);          /* 0x409c70 */
    if (!pages[page].frame && !pages[page].start && page != 0) {
        char msg[100];
        snprintf(msg, sizeof msg, "The page %d (%s) is not written yet.", page, page_names[page]);
        video_error(msg);
        g_quit = 1;
        return;
    }
    /* the page's start */
    if (!g_menu.started[page]) {
        int i;
        g_menu.nbuttons = 0;
        for (i = 0; i < MENU_PICS; i++)
            g_pics[i].sliding = 0;
        if (pages[page].start) {
            pages[page].start();
            g_changing = 0;
        }
        g_menu.started[page] = 1;
        if (g_capture_pending) {
            /* the glass: the old page's picture breaks and flies at the viewer for 1.2 s
               (the pieces are not drawn yet: they need the 3D renderer) */
            g_menu.tr_running = 1;
            g_menu.tr_start = g_now;
            g_menu.tr_end = g_now + 1200;
            sound_play(0x2a, 0);
            g_capture_pending = 0;
        }
    }
    if (pages[page].frame)
        pages[page].frame();
    sound_update();             /* 0x404eb0 */
    /* 0x409a30: nobody has touched anything for 30 s on the title page */
    if (page == 14 && g_menu.active && race_demos_exist() && g_now - g_menu.last_input > 30000
        && !race_demo_start())
        race_demos_off();
    /* a held left button presses a repeating button again: every frame in the game, which
       no one can stop on the number wanted; the port's a click is one step, and only after
       a third of a second held it goes on, ten a second */
    if ((g_mouse & 2) && (g_menu.prev_mouse & 2)) {
        g_menu.last_input = g_now;
        if (g_menu.mouse_mode && g_menu.sel != -1 && (g_menu.b[g_menu.sel].flags & 1)
            && (int)(g_now - g_menu.repeat_at) >= 0) {
            g_menu.pressed = g_menu.sel;
            g_menu.repeat_at = g_now + 100;
        }
    } else
        g_menu.repeat_at = g_now + 333;
    g_menu.prev_mouse = g_mouse;
}

void menu_clear_buttons(void)
{
    g_menu.nbuttons = 0;
}

void menu_add_button(Bitmap *pic, Bitmap *hl, unsigned nb, int hotkey, int group,
                     int value, int *var, int blink, int x, int y, int flags)
{
    Button *b;
    if (g_menu.nbuttons >= MENU_BUTTONS) {
        fprintf(stderr, "openmotoracer: MAX BUTTON reached\n");
        return;
    }
    b = &g_menu.b[g_menu.nbuttons++];
    memset(b, 0, sizeof *b);
    b->kind = 2;
    b->pic = pic;
    b->hl = hl;
    b->nb = nb;
    b->hotkey = hotkey;
    b->group = group;
    b->value = value;
    b->var = var;
    b->blink = blink;
    b->px = x;
    b->py = y;
    b->flags = flags;
}

void menu_add_text_button(int x, int y, const char *text, int font, unsigned nb, int style,
                          int hotkey, int flags)
{
    Button *b;
    Font *f = font_get(font);
    int w = 0, h = 0, px = x, py = y;
    if (g_menu.nbuttons >= MENU_BUTTONS) {
        fprintf(stderr, "openmotoracer: MAX BUTTON reached\n");
        return;
    }
    b = &g_menu.b[g_menu.nbuttons++];
    memset(b, 0, sizeof *b);
    b->kind = 1;
    snprintf(b->text, sizeof b->text, "%s", text);
    if (f)
        font_place(f, text, &px, &py, style, &w, &h);     /* 0x492a90 */
    b->rect[0] = px;
    b->rect[1] = py;
    b->rect[2] = px + w - 1;
    b->rect[3] = py + h - 1;
    b->tx = x;
    b->ty = y;
    b->font = font;
    b->style = style;
    b->hotkey = hotkey;
    b->group = -1;
    b->nb = nb;
    b->px = b->py = -1;
    b->flags = flags;
}

int menu_hit(void)
{
    int i;
    if (!g_menu.mouse_mode)
        return g_menu.sel;
    for (i = 0; i < g_menu.nbuttons; i++) {
        Button *b = &g_menu.b[i];
        if (b->kind == 2) {
            Bitmap *p = b->pic;
            if (b->px != -1 && b->py != -1)
                bitmap_place(p, b->px, b->py, 0);
            if (p->x <= g_mx && g_mx <= p->x + p->w && p->y <= g_my && g_my <= p->y + p->h)
                return i;
        }
        else if (b->rect[0] <= g_mx && g_mx <= b->rect[2] && b->rect[1] <= g_my
                 && g_my <= b->rect[3])
            return i;
    }
    return -1;
}

void menu_draw_buttons(void)
{
    int i, g = -1;
    for (i = 0; i < g_menu.nbuttons; i++)
        if (g_menu.b[i].kind == 2)
            bitmap_step(g_menu.b[i].pic, 0, 0);
    if (g_menu.sel != -1)
        g = g_menu.b[g_menu.sel].group;
    g_blink ^= g_ticks & 1;     /* (each frame in the game: each 60th here) */
    for (i = 0; i < g_menu.nbuttons; i++) {
        Button *b = &g_menu.b[i];
        int state = 0;
        if (b->kind == 1) {
            /* white, or yellow while selected (fore and back alike), drawn at the place
               given with the style; the yellow is stored again afterwards, unbuilt */
            static const unsigned char white[3] = { 255, 255, 255 }, yellow[3] = { 255, 255, 0 };
            Font *f = font_get(b->font);
            if (!f)
                continue;
            if (g_menu.sel == i) {
                font_set_rgb(f, yellow, yellow);
                font_draw(f, b->text, b->tx, b->ty, b->style, 0);
                font_store_rgb(f, yellow, yellow);
            } else {
                font_set_rgb(f, white, white);
                font_draw(f, b->text, b->tx, b->ty, b->style, 0);
            }
            continue;
        }
        if (b->kind != 2)
            continue;
        if (b->px != -1 && b->py != -1)
            bitmap_place(b->pic, b->px, b->py, 0);
        if (g != -1 && b->group == g)
            state = 4;
        if (g_menu.sel == i)
            state |= 2;
        if (b->var && *b->var == b->value)
            state |= 1;
        if (b->blink && g_blink)
            state = 0;
        /* the table 0x40c474 */
        switch (state) {
        case 0:
            bitmap_draw(b->pic, b->hl ? BM_OWN : BM_HALF_KEYED, 100, 0);
            break;
        case 2:
            if (!b->hl) {
                bitmap_draw(b->pic, BM_OWN, 100, 0);
                break;
            }
            /* fall through */
        case 1: case 6: case 7:
            bitmap_place(b->hl, b->pic->x, b->pic->y, 0);
            bitmap_draw(b->hl, BM_OWN, 100, 0);
            break;
        case 4:
            bitmap_draw(b->pic, BM_HALF, 100, 0);
            break;
        case 5:
            bitmap_place(b->hl, b->pic->x, b->pic->y, 0);
            bitmap_draw(b->hl, BM_HALF, 100, 0);
            break;
        default:                /* 3: selected and on: nothing (as the game has it) */
            break;
        }
    }
}

void menu_transition(void)
{
    if (!g_menu.tr_running)
        return;
    if (g_now < g_menu.tr_end) {
        g_menu.tr_t = (float)((double)(g_now - g_menu.tr_start) * 0.000833333354f);
        queue_glass(g_menu.tr_t);
    } else
        g_menu.tr_running = 0;
}

int menu_pressed(void)
{
    return g_menu.tr_running ? -1 : g_menu.pressed;
}

void menu_press_done(void)
{
    g_menu.key = 0;
    g_menu.pressed = -1;
}

/* 0x49a160's part for the menu: 0x4099a0, the button under the pointer */
void menu_begin_frame(void)
{
    r3d_begin();
    int sel = menu_hit();
    g_menu.sel = sel;
    if (sel != g_menu.prev_sel) {
        g_menu.prev_sel = sel;
        if (sel != -1)
            sound_play(0x29, 0);
    }
}

/* the text objects (0x418790, 0x417840, 0x418860), then the game's own mouse pointer
   (Pointeur.lez, 0x55), its corner on the hot spot */
void menu_end_frame(void)
{
    text_update();
    if (!text_empty())
        text_draw();
    Bitmap *p = &g_pics[0x55];
    bitmap_place(p, g_mx, g_my, 0);
    bitmap_draw(p, BM_OWN, 100, 0);
}

/* 0x40a0f0 */
int menu_alt(void)
{
    return g_mods & 1;
}

void menu_key(int vk)
{
    int i;
    if (!g_menu.active)
        return;
    g_menu.last_input = g_now;
    g_menu.key = vk;
    if (vk != VK_SHIFT && vk != VK_CONTROL)
        g_menu.mouse_mode = 0;
    /* hotkeys: the last button with this key wins; Return after Tab or the arrows
       goes to the selection instead */
    if (!g_menu.tr_running && g_menu.pressed == -1)
        for (i = 0; i < g_menu.nbuttons; i++) {
            if (g_menu.b[i].hotkey != vk)
                continue;
            if (g_navigated && vk == VK_RETURN && !(g_mods & 1))
                break;
            g_menu.sel = g_menu.pressed = i;
            g_menu.key = 0;
            g_navigated = 0;
        }
    switch (g_menu.key) {
    case VK_TAB:
        if (g_mods & 2) {
            if (--g_menu.sel < 0)
                g_menu.sel = g_menu.nbuttons - 1;
        } else if (++g_menu.sel == g_menu.nbuttons)
            g_menu.sel = 0;
        g_navigated = 1;
        break;
    case VK_RETURN:
    case VK_SPACE:
        if (!g_menu.tr_running && !(g_mods & 1) && g_menu.sel != -1)
            g_menu.pressed = g_menu.sel;
        break;
    case VK_SHIFT:
        g_mods |= 2;
        break;
    case VK_CONTROL:
        g_mods |= 4;
        break;
    case VK_MENU:
        g_mods |= 1;
        break;
    case VK_LEFT:
    case VK_RIGHT:
    case VK_DOWN:
    case VK_UP:
        if (g_menu.sel == -1)
            g_menu.sel = g_menu.nbuttons == 0;
        else {
            int shift = g_menu.key == VK_LEFT ? 0 : g_menu.key == VK_RIGHT ? 8
                      : g_menu.key == VK_DOWN ? 16 : 24;
            unsigned n = g_menu.b[g_menu.sel].nb >> shift & 0xff;
            if (n != 0xff)
                g_menu.sel = (int)n;
        }
        g_navigated = 1;
        break;
    }
}

int g_menu_char;

void menu_key_up(int vk)
{
    if (vk == VK_SHIFT)
        g_mods &= ~2;
    else if (vk == VK_CONTROL)
        g_mods &= ~4;
    else if (vk == VK_MENU)
        g_mods &= ~1;
}

void menu_mouse_move(int x, int y)
{
    g_mx = x;
    g_my = y;
    if (g_menu.active) {
        g_menu.mouse_mode = 1;
        g_menu.last_input = g_now;
    }
}

void menu_mouse_button(int left, int down)
{
    unsigned bit = left ? 2 : 1;
    if (!down) {
        g_mouse &= ~bit;
        return;
    }
    g_mouse |= bit;
    /* 0x40a460: a button is pressed as the left button goes down */
    if (left && g_menu.active) {
        g_menu.last_input = g_now;
        if (g_menu.mouse_mode && g_menu.sel != -1) {
            g_menu.pressed = g_menu.sel;
            sound_play(3, 0);
        }
    }
}
