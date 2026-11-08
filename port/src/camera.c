/* The race's camera (NOTES §4.20 §2): the chase 0x46f840 behind the followed bike, its
   four distances (F5), the look back (Tab: the bike-mounted preset 1), the orbit to the
   front once the bike has finished, the on-board camera (F6) and the TV director of the
   replays and demos (0x46e9f0). */
#include "camera.h"
#include "bikes.h"
#include "choices.h"
#include "land.h"
#include "menu.h"
#include "r3d.h"
#include "rand.h"
#include "video.h"
#include <math.h>
#include <string.h>
#include "fastfloat.h"      /* (last: the ESP32's single precision) */

#define ANGLE_K 2607.594587618272       /* 16384 / 2 pi: the cosine table's index */
#define PI 3.141592653589793

static struct {
    float ang[3];               /* +0x38: the camera's angles (the yaw +0x3c) */
    float pos[3];               /* +0x98 */
    float target[3];            /* +0xa4 */
    float lerp[3];              /* +0xc8: the chase's lerped point */
    float c[3][3];              /* +0x68 */
    int preset;                 /* +0x21c: 1 looks back (Tab) */
    int dist;                   /* +0x220: the chase distance, 0..3 (F5) */
    float c0[3][3], pos0[3];    /* (the port's: before the last step, for smooth pictures) */
    int jumped;                 /* (a cut since: no blending) */
    int onboard;                /* 0x4dc008: mode 6, the rider's eyes (F6) */
    int mode;                   /* +8: 7 chase, 6 on-board, 3 looks at a point, 1 placed */
    float orbit;                /* +0x28: the TV's orbit angle */
    float zoom;                 /* 0x6d1f74 */
} g_cam;

static const float g_extra[4] = { 0.0f, 128.0f, 256.0f, 512.0f };       /* +0x224 */
static const float g_preset_off[8][3] = {       /* +0x11c */
    { 0, -160, 1680 }, { 0, 160, -20 }, { -250, 150, 0 }, { 250, 150, 0 },
    { -70, 120, 200 }, { 70, 120, 200 }, { 0, 400, 400 }, { 0, 500, 500 }
};
static const float g_preset_ang[8][3] = {       /* +0x17c */
    { 0, 0, 0 }, { 0, 3.14159265f, 0 }, { 0, 1.57079633f, 0 }, { 0, -1.57079633f, 0 },
    { 0, 3.14159265f, 0 }, { 0, 3.14159265f, 0 },
    { -0.78539816f, 3.14159265f, 0 }, { -0.78539816f, 3.14159265f, 0 }
};
#define CHASE_Y 64.0f           /* +0x1dc */
#define OFF_Y 191.0f            /* +0x54 */
#define OFF_Z -484.0f           /* +0x58 */
#define CAMERA_FOCAL ((float)(g_scr_w * 394 / 640))     /* 0x49a0d0 with the zoom at 1 */

/* 0x490220: the heading of a matrix of rows (its T is 0) */
static double heading(const float *m)
{
    double fx = m[6], fy = m[7], fz = m[8], ux = m[3], uy = m[4];
    double l = sqrt(fx * fx + fy * fy + fz * fz);
    if (l < 1e-4)
        return PI / 2;
    return atan2((ux * fy - fx * uy) / l, fz / l);
}

/* 0x403a80: A then B, for row vectors */
static void mul(float o[3][3], const float a[3][3], const float b[3][3])
{
    float r[3][3];
    int i, j;
    for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++)
            r[i][j] = (float)((double)a[i][0] * b[0][j] + (double)a[i][1] * b[1][j]
                              + (double)a[i][2] * b[2][j]);
    memcpy(o, r, sizeof r);
}

/* v M + P, as the chase sums it: ((y M1 + x M0) + z M2) + P */
static void place(float o[3], const float v[3], const float m[3][3], const float p[3])
{
    int i;
    for (i = 0; i < 3; i++)
        o[i] = (float)((((double)v[1] * m[1][i] + (double)v[0] * m[0][i])
                        + (double)v[2] * m[2][i]) + p[i]);
}

/* W = R(0, psi, 0) R(the camera's angles): the bike's heading only */
static void chase_frame(float w[3][3], const BikePhys *b)
{
    float yaw[3] = { 0.0f, 0.0f, 0.0f }, ry[3][3], rc[3][3];
    yaw[1] = (float)-heading(b->m);
    r3d_rotation(ry, yaw, ANGLE_K);
    r3d_rotation(rc, g_cam.ang, ANGLE_K);
    mul(w, (const float (*)[3])ry, (const float (*)[3])rc);
}

void camera_reset(const BikePhys *b)
{
    float w[3][3], e[3];
    memset(g_cam.ang, 0, sizeof g_cam.ang);     /* 0x46da40 */
    g_cam.preset = 0;
    /* 0x47bf70: behind the bike on its own height, raised by +0x54, and the chase brings it
       down into place, 0.8 a frame: about 0.4 s at the start of every race */
    chase_frame(w, b);
    e[0] = 0.0f, e[1] = CHASE_Y, e[2] = OFF_Z + g_extra[g_cam.dist];
    place(g_cam.target, g_preset_off[0], (const float (*)[3])w, b->pos);
    e[1] = 0.0f;
    place(g_cam.lerp, e, (const float (*)[3])w, b->pos);
    g_cam.lerp[1] = (float)(g_cam.lerp[1] + OFF_Y);
    memcpy(g_cam.pos, g_cam.lerp, sizeof g_cam.pos);
    camera_step(b, 1.6f, 1, 0, 0);
    g_cam.jumped = 1;
}

/* 0x46f840 */
static void chase(const BikePhys *b, float dt, int finished)
{
    float w[3][3], e0[3], t0[3], v[3];
    double d, u[3], l;
    int i;
    if (b->crashed) {           /* between the bike and the rider's body, from where it is */
        for (i = 0; i < 3; i++)
            g_cam.target[i] = (float)(((double)b->pos[i] + b->tumble_pt[i]) * 0.5);
        r3d_camera(g_cam.pos, g_cam.target);
        r3d_get_camera(g_cam.c, v);
        return;
    }
    if (g_cam.preset) {         /* R(angles) M inverted: the transpose, T = 0 */
        float r[3][3], m[3][3], rm[3][3];
        memcpy(m, b->frame, sizeof m);
        r3d_rotation(r, g_preset_ang[g_cam.preset], ANGLE_K);
        mul(rm, (const float (*)[3])r, (const float (*)[3])m);
        for (i = 0; i < 3; i++) {
            int j;
            for (j = 0; j < 3; j++)
                g_cam.c[i][j] = rm[j][i];
        }
        memcpy(m, b->m, sizeof m);
        place(g_cam.pos, g_preset_off[g_cam.preset], (const float (*)[3])m, b->pos);
        return;
    }
    d = (double)g_extra[g_cam.dist] + OFF_Z;
    if (finished) {             /* the orbit to the front: -3 pi / 4, 1.5 times as far */
        if (g_cam.ang[1] > -2.356194490192345)
            g_cam.ang[1] = (float)(dt * -0.010471975511965976 + g_cam.ang[1]);
        d -= (g_cam.ang[1] * -0.4244131815783876) * (OFF_Z * -0.5);
    }
    chase_frame(w, b);
    v[0] = g_preset_off[0][0], v[1] = CHASE_Y, v[2] = (float)d;
    place(e0, v, (const float (*)[3])w, b->pos);
    for (i = 0; i < 3; i++)     /* 0.2 a picture, whatever dt */
        g_cam.lerp[i] = (float)((e0[i] - (double)g_cam.lerp[i]) * 0.2f + g_cam.lerp[i]);
    if (finished) {
        static const float up[3] = { 0.0f, CHASE_Y, 0.0f };
        place(t0, up, (const float (*)[3])w, b->pos);
    } else
        place(t0, g_preset_off[0], (const float (*)[3])w, b->pos);
    g_cam.target[0] = (float)((t0[0] - (double)g_cam.target[0]) * 0.5f + g_cam.target[0]);
    g_cam.target[1] = (float)((t0[1] - (double)g_cam.target[1]) * 0.3333333432674408
                              + g_cam.target[1]);
    g_cam.target[2] = (float)((t0[2] - (double)g_cam.target[2]) * 0.5f + g_cam.target[2]);
    r3d_camera(g_cam.lerp, g_cam.target);
    r3d_get_camera(g_cam.c, v);
    /* the eye pushed to exactly |d| from the bike along the lerped direction, 191 up */
    for (i = 0; i < 3; i++)
        u[i] = (double)g_cam.lerp[i] - b->pos[i];
    l = sqrt((u[1] * u[1] + u[2] * u[2]) + u[0] * u[0]);
    if (l != 0.0)
        for (i = 0; i < 3; i++)
            u[i] = (float)(u[i] / l);
    for (i = 0; i < 3; i++)
        g_cam.pos[i] = (float)(b->pos[i] - u[i] * d);
    g_cam.pos[1] = (float)(g_cam.pos[1] + OFF_Y);
    if (finished)
        g_cam.pos[1] = (float)(g_cam.pos[1]
                               - (OFF_Y * 0.6666666865348816) * (g_cam.ang[1] * -0.4244131815783876));
}

/* v M + T + P: a point of the bike */
static void on_bike(float o[3], const float v[3], const BikePhys *b)
{
    int i;
    for (i = 0; i < 3; i++)
        o[i] = (float)((((double)v[0] * b->m[i] + (double)v[1] * b->m[3 + i])
                        + (double)v[2] * b->m[6 + i] + b->m[9 + i]) + b->pos[i]);
}

/* 0x4700e0's mode 6: from the rider's head to 40 ahead, lower as the front wheel lifts;
   the height lerped (dirt bikes 1/2, road bikes 10/13 a step), then rolled by a quarter of
   the lean */
static void onboard(const BikePhys *b)
{
    int dirt = g_bikes[b->bike].perso != 0, i;
    float k = dirt ? 10.0f : 5.0f, ve[3], vt[3], e[3], t[3], rr[3][3], ang[3] = { 0, 0, 0 };
    double f = dirt ? 0.5f : 0.7692307692307692;
    vt[0] = 0.0f, vt[1] = (float)(b->wheelie * k + 170.0f), vt[2] = 40.0f;
    ve[0] = 0.0f, ve[1] = (float)(b->wheelie * -k + 170.0f), ve[2] = 0.0f;
    on_bike(t, vt, b);
    on_bike(e, ve, b);
    for (i = 0; i < 3; i += 2) {
        g_cam.pos[i] = e[i];
        g_cam.target[i] = t[i];
    }
    g_cam.pos[1] = (float)((e[1] - (double)g_cam.pos[1]) * f + g_cam.pos[1]);
    g_cam.target[1] = (float)((t[1] - (double)g_cam.target[1]) * f + g_cam.target[1]);
    r3d_camera(g_cam.pos, g_cam.target);
    r3d_get_camera(g_cam.c, e);
    ang[2] = (float)(b->lean * 0.25f);
    r3d_rotation(rr, ang, ANGLE_K);
    mul(g_cam.c, (const float (*)[3])g_cam.c, (const float (*)[3])rr);
}

/* ------------------------------------------------------------------ the TV director */

/* the shot (0x4ea5a8, kept from race to race, the image's 1) and its time left (0x4dc13c,
   the image's 600): the units are dt's */
enum { TV_TRACKSIDE, TV_CHASE, TV_ORBIT, TV_FRONT, TV_ZOOM, TV_ONBIKE };
static int g_shot = TV_CHASE, g_shot_left = 600;

/* 0x40d4f0: the trackside camera nearest the point by x and z, in the 23 sections ahead
   of sec (behind when the track runs backwards); 0 when none */
static int nearest_camera(float o[3], int sec, const float p[3])
{
    double best = 1e13;
    int i, j, found = 0, from = g_reverse ? sec - 23 : sec;
    for (i = from; i < from + 23; i++) {
        const Section *s = &g_land.sec[(i + g_land.nsec) % g_land.nsec];
        for (j = 0; j < s->ndecor; j++) {
            const Decor *d;
            double dx, dz, q;
            if (s->decor[j] >= g_land.ndecor)
                continue;
            d = &g_land.decor[s->decor[j]];
            if (!d->camera)
                continue;
            dx = (float)(d->pos[0] - p[0]);
            dz = (float)(d->pos[2] - p[2]);
            q = (float)(dz * dz + dx * dx);
            if (q < best) {
                best = q;
                memcpy(o, d->pos, sizeof d->pos);
                found = 1;
            }
        }
    }
    return found;
}

/* a new shot out of nowhere to look from: one in twelve each of the orbit, the front and
   the bike-mounted ones, otherwise the chase */
static void random_shot(void)
{
    int i = (int)((rand_next(0) & 0xffff) * (double)1.5259021893143654e-05f * 12.0);
    if (i == 0) {
        g_shot = TV_ORBIT;
        g_cam.orbit = rand_next(0) & 1 ? (float)PI : (float)-PI;
    } else
        g_shot = i == 1 ? TV_FRONT : i == 2 ? TV_ONBIKE : TV_CHASE;
}

/* V times the matrix, plus its T, plus the bike's place (the +0x98 block, b->m) */
static void from_bike(float o[3], const double v[3], const BikePhys *b)
{
    int i;
    for (i = 0; i < 3; i++)
        o[i] = (float)(v[0] * b->m[i] + v[1] * b->m[3 + i] + v[2] * b->m[6 + i]
                       + b->m[9 + i] + b->pos[i]);
}

/* 0x46e9f0: which shot, and for the fixed ones where the eye is */
static void director(const BikePhys *b, float dt)
{
    int was = g_shot;
    float cam[3];
    if (g_shot_left < 0) {
        int found = nearest_camera(cam, b->sec, b->pos);
        switch (g_shot) {
        case TV_TRACKSIDE:
        case TV_ZOOM:           /* a look from the next one, one time in eight elsewhere */
            if (!found || (rand_next(0) & 7) == 0)
                random_shot();
            else
                memcpy(g_cam.pos, cam, sizeof cam);
            break;
        case TV_ORBIT:          /* from a camera, always zoomed; else the old look on */
            if (found) {
                rand_next(0);
                g_shot = TV_ZOOM;
                memcpy(g_cam.pos, cam, sizeof cam);
            } else
                g_shot = TV_TRACKSIDE;
            break;
        default:
            if (found) {
                g_shot = rand_next(0) & 1 ? TV_TRACKSIDE : TV_ZOOM;
                memcpy(g_cam.pos, cam, sizeof cam);
            } else
                random_shot();
        }
        switch (g_shot) {
        case TV_TRACKSIDE: g_shot_left = 250; break;
        case TV_CHASE: g_shot_left = 600; break;
        case TV_ORBIT: g_shot_left = 300; break;
        case TV_FRONT: g_shot_left = 400; break;
        case TV_ZOOM: g_shot_left = 260; break;
        default:
            g_shot_left = 400;
            do
                g_cam.preset = rand_next(0) & 7;
            while (g_cam.preset <= 1);
            if (g_cam.preset == 2 || g_cam.preset == 3)
                g_shot_left -= 100;
        }
        g_cam.jumped = 1;
    }
    if (g_bikeworld.snap) {     /* 0x4ea510: a bike got up: the chase, at once */
        g_shot = TV_CHASE;
        g_shot_left = 600;
        g_cam.preset = 0;
        g_bikeworld.snap = 0;
        g_cam.jumped |= was != TV_CHASE;
    }
    if (g_shot != TV_ONBIKE)
        g_cam.preset = 0;
    switch (g_shot) {
    case TV_CHASE:
        g_cam.orbit = 0.0f;
        g_cam.ang[1] = 0.0f;
        g_cam.mode = 7;
        break;
    case TV_ORBIT: {            /* round the bike, never lower than 260 above the road */
        int i = (int)(g_cam.orbit * ANGLE_K);
        double c = g_cos[i & 0x3fff], s = g_cos[(i - 0x1000) & 0x3fff], v[3];
        float y;
        v[0] = (float)(s * OFF_Z);
        v[1] = (float)(-(g_cos[0x3000] * c) * OFF_Z);
        v[2] = (float)(c * g_cos[0] * OFF_Z);
        from_bike(g_cam.pos, v, b);
        g_cam.orbit = (float)(dt * -0.015707963267948967 + g_cam.orbit);
        y = (float)(bikephys_road_y(b->sec, g_cam.pos) + 260.0f);
        if (y > g_cam.pos[1])
            g_cam.pos[1] = y;
        memcpy(g_cam.target, b->pos, sizeof g_cam.target);
        g_cam.mode = 3;
        break;
    }
    case TV_FRONT: {            /* ahead of it looking back, drawn up towards 560 above the road */
        double v[3];
        float y;
        g_cam.orbit = (float)PI;
        v[0] = (float)((double)OFF_Z * g_cos[0x1000]);
        v[1] = (float)(-((double)g_cos[0x2000] * g_cos[0x3000]) * OFF_Z);
        v[2] = (float)((double)g_cos[0x2000] * g_cos[0] * OFF_Z);
        from_bike(g_cam.pos, v, b);
        y = (float)(bikephys_road_y(b->sec, g_cam.pos) + 560.0f);
        if (y > g_cam.pos[1])
            g_cam.pos[1] = (float)((y - (double)g_cam.pos[1]) * 0.25f + g_cam.pos[1]);
        memcpy(g_cam.target, b->pos, sizeof g_cam.target);
        g_cam.mode = 3;
        break;
    }
    case TV_ZOOM: {             /* the lens closes so that the bike keeps its size */
        double dx = (double)g_cam.pos[0] - b->pos[0], dy = (double)g_cam.pos[1] - b->pos[1];
        double dz = (double)g_cam.pos[2] - b->pos[2];
        float d = (float)sqrt(dx * dx + dy * dy + dz * dz);
        g_cam.zoom = (float)((g_scr_h / 2) / ((double)CAMERA_FOCAL * 256.0f / d));
        if (g_cam.zoom < 1.0f)
            g_cam.zoom = 1.0f;
        if (g_cam.zoom > 3.0f)
            g_cam.zoom = 3.0f;
    }   /* fall through */
    case TV_TRACKSIDE:
        memcpy(g_cam.target, b->pos, sizeof g_cam.target);
        g_cam.mode = 3;
        break;
    case TV_ONBIKE: {           /* fixed to the bike, leaning with it */
        float m[3][3], r[3][3], rm[3][3], v[3];
        int i, j;
        g_cam.orbit = 0.0f;
        memcpy(m, b->frame, sizeof m);
        for (i = 0; i < 3; i++) {
            const float *off = g_preset_off[g_cam.preset];
            v[i] = (float)(((double)off[0] * m[0][i] + (double)off[1] * m[1][i]
                            + (double)off[2] * m[2][i]) + b->frame[9 + i]);
            g_cam.pos[i] = (float)((double)b->pos[i] + v[i]);
        }
        r3d_rotation(r, g_preset_ang[g_cam.preset], ANGLE_K);
        mul(rm, (const float (*)[3])r, (const float (*)[3])m);
        for (i = 0; i < 3; i++)
            for (j = 0; j < 3; j++)
                g_cam.c[i][j] = rm[j][i];
        break;                  /* (stays mode 1: 0x458620 leaves it) */
    }
    }
    g_shot_left = (int)((double)g_shot_left - dt);
}

float camera_zoom(void)
{
    return g_cam.zoom;
}

void camera_step(const BikePhys *b, float dt, int countdown, int finished, int tv)
{
    float *yaw = &g_cam.ang[1];
    float v[3];
    g_cam.zoom = 1.0f;          /* 0x4583e0 */
    g_cam.mode = g_cam.onboard ? 6 : 7;
    if (tv && g_land.ncam > 0) {
        g_cam.mode = 1;
        director(b, dt);
        if (g_cam.mode == 1)
            return;
    }
    if (countdown) {            /* 0x458620: the yaw comes back to 0, pi / 128 a 10 ms */
        if (*yaw > 0.0f) {
            *yaw = (float)(dt * -0.02454369260617026 + *yaw);
            if (*yaw < 0.0f)
                *yaw = 0.0f;
        } else if (*yaw < 0.0f) {
            *yaw = (float)(dt * 0.02454369260617026 + *yaw);
            if (*yaw > 0.0f)
                *yaw = 0.0f;
        }
    }
    if (g_cam.mode == 3) {      /* 0x4700e0: from where it is at the point */
        r3d_camera(g_cam.pos, g_cam.target);
        r3d_get_camera(g_cam.c, v);
    } else if (g_cam.mode == 6 && !g_cam.preset)
        onboard(b);
    else
        chase(b, dt, finished);
}

void camera_onboard(int on)
{
    if (on != g_cam.onboard)
        g_cam.jumped = 1;
    g_cam.onboard = on;
}

int camera_preset(void)
{
    return g_cam.preset;
}

void camera_key(int vk, int down)
{
    if (vk == VK_TAB) {         /* 0x431250 / 0x4328c7 */
        if (g_cam.preset != (down ? 1 : 0))
            g_cam.jumped = 1;
        g_cam.ang[1] = down ? (float)PI : 0.0f;
        g_cam.preset = down ? 1 : 0;
    } else if (vk == 0x74 && down)              /* F5 */
        g_cam.dist = (g_cam.dist + 1) & 3;
}

int camera_cut(void)
{
    return g_cam.jumped;
}

void camera_keep(void)
{
    memcpy(g_cam.c0, g_cam.c, sizeof g_cam.c0);
    memcpy(g_cam.pos0, g_cam.pos, sizeof g_cam.pos0);
    g_cam.jumped = 0;
}

void camera_apply(float a)
{
    float c[3][3], p[3];
    int i, j;
    if (a >= 1.0f || g_cam.jumped) {
        r3d_set_camera((const float (*)[3])g_cam.c, g_cam.pos);
        return;
    }
    for (i = 0; i < 3; i++) {
        p[i] = g_cam.pos0[i] + (g_cam.pos[i] - g_cam.pos0[i]) * a;
        for (j = 0; j < 3; j++)
            c[i][j] = g_cam.c0[i][j] + (g_cam.c[i][j] - g_cam.c0[i][j]) * a;
    }
    r3d_set_camera((const float (*)[3])c, p);
}

const float *camera_eye(void)
{
    return g_cam.pos;
}

const float *camera_target(void)
{
    return g_cam.target;
}
