#ifndef R3D_H
#define R3D_H
#include <stdint.h>
#include "ftol.h"

/* The software 3D renderer (NOTES §4.9.3): texture pages, models, the float32 transform
   and projection (0x48c030, 0x49b2c0), the depth buckets (0x49b7f0), and the drawing
   nearest first into per-row lists of the pixels still free (0x49ad90, an s-buffer). */

#define R3D_PAGES 64

typedef struct {                /* 0x40 bytes in the game; the narrow fields hold what the
                                   game's do (a track has thousands of faces: NOTES §4.26) */
    int16_t kind;               /* +0x00: the draw kind (16, 21, 38, 39 textured) */
    uint16_t colour;            /* +0x02: RGB565, for the untextured kinds */
    int16_t drawn;              /* +0x3a: the kind as last queued: copies of a model share it */
    uint16_t page;              /* +0x36 */
    uint16_t flags;             /* +0x38: bit 1 = sorted 1024 farther */
    uint8_t n;                  /* 3 or 4 corners */
    uint16_t idx[4];            /* +0x2c */
    float ut[4], vt[4];         /* +0x04: the corners in texels, as the race's sub 6 takes them
                                   (u/256 x 256.0, 0x4a0f90); the affine kinds' texels x 8192
                                   (u/256 x 2^21, 0x49e510) are FACE_U: computed where drawn, not
                                   kept (a track's faces, NOTES §4.26) */
} Face;
/* the corner's u and v x 8192 as the game keeps them: ftol(u x 8192.0), exact in float */
#define FACE_U(f, k) ftolf((f)->ut[k] * 8192.0f)
#define FACE_V(f, k) ftolf((f)->vt[k] * 8192.0f)

typedef struct {
    int nv, nf;
    float (*v)[3];
    Face *f;
} Model;

extern float g_cos[16384];      /* 0x5fcb80: cos(k x 2pi / 16384) */
extern uint16_t *g_texpage[R3D_PAGES];   /* page objects 0x4f92f8 + n x 0x338: 256x256 */

void r3d_init(void);
uint16_t *r3d_page(int n);                      /* a page's texels, made if missing */
int r3d_load_page(int n, const char *path);     /* 0x401160 */
#ifdef OMR_ESP32
void r3d_page_to8(int n);                       /* the board: made 8-bit, 256 colours (r3d.c) */
#endif
void r3d_free_page(int n);
/* 0x401160 with flag 0 on a paletted RAW (the bikes' bank 0): the 8-bit texels kept and
   the liveries' palettes, the RAW's own then "<name>.1" .. ".7" (8 at most) */
int r3d_load_page_pal(int n, const char *path);
const uint8_t *r3d_page_index(int n);           /* its 8-bit texels; 0: not paletted */
const uint16_t *r3d_page_palette(int n, int k); /* its palette k (RGB565); 0: none */

/* 0x4216a0 with the last argument 1: x96, the mirror and the corner order of §4.9.3.1 */
int model_load(Model *m, const char *base);
/* 0x420a50 as the bikes' wheels are loaded (flag: the family's rider is the dirt one) */
int model_load_bike(Model *m, const char *f3df, const char *fmap, int flag);
void model_centre(Model *m);                    /* 0x420250 */
void model_scale(Model *m, float sx, float sy, float sz);   /* 0x4204b0 */
void model_page_add(Model *m, int add);         /* 0x422250 */
void model_free(Model *m);
/* 0x420a50 with the last argument 0 (the menu's 3D font), then 0x420250 */
int model_load_menu(Model *m, const char *base);
void model_mirror(Model *m);                    /* 0x422070, 0x47ad40 */
float model_fit(Model *m, float size);          /* 0x47a960: largest half extent = size */
void model_rescale(Model *m, float s);          /* 0x47abd0: centred again, times s */

void r3d_camera(const float pos[3], const float target[3]);  /* 0x46e140 */
void r3d_camera_pos(const float pos[3]);
void r3d_get_camera(float c[3][3], float pos[3]);       /* the camera block's +0x68, +0x98 */        /* the block's +0x98 alone, not looked again */
void r3d_begin(void);                           /* 0x49a160: empties the lists */
/* 0x48c030: a model at an angle (a, b, c: radians) and a place, through the camera */
void r3d_object(const Model *m, const float ang[3], const float pos[3]);
/* 0x49b2c0 + 0x49b7f0 on points already in the view's frame: v x N + T */
void r3d_view(int nv, const float (*v)[3], int nf, const Face *f, const float n[3][3],
              const float t[3]);
/* the two halves of r3d_view: 0x49b2c0 (the points; their first index is returned, -1 when
   there is no room) and the faces on them, through 0x49b7f0 (skin < 0) or 0x49c110 (the
   bikes': no 1024 farther, and faces on a paletted page drawn through the palette skin) */
int r3d_points(int nv, const float (*v)[3], const float n[3][3], const float t[3]);
void r3d_faces(int base, int nf, const Face *f, int skin);
/* 0x461f70's way: 0x49b7f0 with the detail 0x4dc098 forced for these faces */
void r3d_faces_detail(int base, int nf, const Face *f, int detail);
/* what 0x49b2c0's light does: dark = the points' word is below 0x3e000 (paletted faces then
   halved); swap = 0x4221d0 (a wheel's kinds 0x10 -> 0x15, 0x26 -> 0x27). For the faces queued
   from now on */
void r3d_dark(int dark, int swap);
/* 0x49bca0, the on-board cockpit's faces: the farthest corner's depth x 0.2 - near as the
   key, the kinds but 0 drawn as 0x10 (from 1 or 0x10) or 0x2c */
void r3d_faces_cockpit(int base, int nf, const Face *f);
/* the same with a rotation of row vectors, M x C (0x403880's decor, the track's blocks) */
void r3d_object_m(const Model *m, const float r[3][3], const float pos[3]);
/* a placement (rotation of row vectors, position) seen through the camera: N = R C,
   T = (pos - camera) C */
void r3d_place(float n[3][3], float t[3], const float r[3][3], const float pos[3]);
void r3d_set_camera(const float c[3][3], const float pos[3]);   /* the block's +0x68, +0x98 */
/* the dashboard's dial and needles: points already on the screen at depth z, and faces
   on them at one sort key, their kinds kept (0x4f1a18 = 1), none culled */
int r3d_screen_points(int n, const int (*xy)[2], float z);
void r3d_screen_faces(int base, int nf, const Face *f, float key);
/* the same with the kinds through a detail row (0x4f1a18 = 0; detail < 0: the race's own) */
void r3d_key_faces(int base, int nf, const Face *f, float key, int detail);
void r3d_render(void);                          /* 0x49ad90 */
#ifdef OMR_ESP32
void r3d_render_async(void);                    /* the board: drawn by core 1 while core 0 goes on */
void r3d_finish(void);                          /* and waited for (nothing pending: returns) */
#endif
#ifdef R3D_TWO
void r3d_two_cpus(int (*pin)(void));            /* a second thread draws every other row; pin puts it on its CPU */
#endif
/* 0x410e10: the race's near plane (64) and its kinds for a detail row (0..4); -1 the
   menus' (near 256, kinds as they are) */
void r3d_race(int detail);
void r3d_focal(float f);                        /* 0x49a0e0 + 0x49a100 */
void r3d_screen(void);                          /* 0x499ba0: the centre of the screen's size */
#ifdef MISTER_HYBRID
void r3d_screen_down(int y);                    /* what is drawn next, y rows lower: until r3d_screen */
#endif

/* the menu's 3D objects (0x479eb0) and their camera (0x47af50, the object 0x504bc8) */
void r3d_menu_matrix(float m[3][3], const float ang[3]);
void r3d_menu_camera(const float ang[3], const float pos[3]);
void r3d_menu_object(const Model *m, const float ang[3], const float pos[3]);
int r3d_menu_project(const float v[3], int *sx, int *sy);      /* 0: behind */

/* 0x48c030's rotation for row vectors, M = Rz(c) Ry(b) Rx(a), from the cosine table at
   index = trunc(angle x k) */
void r3d_rotation(float m[3][3], const float ang[3], double k);

#endif
