/* 3D models: .3DF (the shape) and .MAP (each face's texture), as 0x4216a0 loads them with
   its last argument 1 (NOTES §4.9.3.1, §4.9.4) */
#include "r3d.h"
#include "files.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned u16(const unsigned char *p)
{
    return p[0] | p[1] << 8;
}

static uint32_t u32(const unsigned char *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static float f32(const unsigned char *p)
{
    uint32_t v = u32(p);
    float f;
    memcpy(&f, &v, 4);
    return f;
}

static int load(Model *m, const char *f3df, const char *fmap, const char *base)
{
    unsigned char *d, *mp;
    long len, mlen, p;
    int i, k, nf = 0;
    memset(m, 0, sizeof *m);
    d = dsi_load(f3df, &len);
    mp = dsi_load(fmap, &mlen);
    if (!d || !mp || len < 4 || u16(d) != 1 || mlen < 0x200 || !(mp[1] & 0x80))
        goto bad;
    m->nv = (int)u16(d + 2);
    p = 4 + 12L * m->nv;
    for (;;) {                                          /* count the faces */
        unsigned n;
        if (p + 2 > len)
            goto bad;
        if (!(n = u16(d + p)))
            break;
        p += 4 + 2L * n;
        nf++;
    }
    if (mlen < 0x200 + 0x80L * nf)
        goto bad;
    m->v = malloc(sizeof *m->v * (m->nv ? m->nv : 1));
    m->f = calloc(nf ? nf : 1, sizeof *m->f);
    if (!m->v || !m->f)
        goto bad;
    for (i = 0; i < m->nv; i++)
        for (k = 0; k < 3; k++)
            m->v[i][k] = f32(d + 4 + 12 * i + 4 * k) * 96.0f;     /* 0x4d0690 */
    p = 4 + 12L * m->nv;
    for (i = 0; i < nf; i++) {
        const unsigned char *r = mp + 0x200 + 0x80 * i;
        Face *f = &m->f[m->nf];
        unsigned n = u16(d + p), mkind = u16(r);
        uint32_t fl = u32(r + 0x48);
        int idx[5];
        if (n > 4) {            /* (the game takes up to 5 corners; these files have 3, 4) */
            fprintf(stderr, "openmotoracer: %s: a face of %u corners skipped\n", base, n);
            p += 4 + 2L * n;
            continue;
        }
        for (k = 0; k < (int)n; k++) {
            idx[k] = (int)u16(d + p + 4 + 2 * k);
            if (idx[k] >= m->nv)
                goto bad;
        }
        p += 4 + 2L * n;
        f->n = (int)n;
        f->colour = (int)u16(r + 0x4e);
        f->page = (int)(fl & 0xffff);
        f->flags = 4 | (fl & 0x20000 ? 1 : 0) | (fl & 0x100000 ? 8 : 0);
        if (mkind < 3) {                                /* untextured */
            f->kind = fl & 0x80000 ? 0xc : fl & 0x40000 ? 0xd : fl & 0x10000 ? 2 : 0;
        } else {
            if (mkind != n)
                goto bad;
            f->kind = fl & 0x200000 ? (fl & 0x10000 ? 0x27 : 0x15) : (fl & 0x10000 ? 0x26 : 0x10);
            if (fl & 0x40000)
                f->kind = 0xd;
            if (fl & 0x80000)
                f->kind = 8;
            if (fl & 0x400000)
                f->flags |= 2;
        }
        /* the corners in the reverse order, each with its own (u, v) */
        for (k = 0; k < (int)n; k++) {
            int j = (int)n - 1 - k;
            f->idx[k] = idx[j];
            if (mkind >= 3) {
                f->ut[k] = (float)(int)u32(r + 0x22 + 4 * j);  /* 0x422ed0's floats */
                f->vt[k] = (float)(int)u32(r + 0x32 + 4 * j);

            }
        }
        m->nf++;
    }
    for (i = 0; i < m->nv; i++)                         /* 0x422070: the mirror */
        m->v[i][0] = -m->v[i][0];
    free(d);
    free(mp);
    return 1;
bad:
    fprintf(stderr, "openmotoracer: the model %s could not be read\n", base);
    free(d);
    free(mp);
    model_free(m);
    return 0;
}

int model_load(Model *m, const char *base)
{
    char f3df[256], fmap[256];
    snprintf(f3df, sizeof f3df, "%s.3df", base);
    snprintf(fmap, sizeof fmap, "%s.map", base);
    return load(m, f3df, fmap, base);
}

/* 0x420a50(3df, map, -1, flag): a textured face is 0x10 (0x26 see-through) only when the
   flag is set and its page is not 0, else 1 (2), the kinds 0x49c110 draws through a
   palette; an untextured face's colour is the u16 at MAP +2 (0x423110). The face's page
   must be loaded already. */
int model_load_bike(Model *m, const char *f3df, const char *fmap, int flag)
{
    unsigned char *mp;
    long mlen;
    int i;
    if (!load(m, f3df, fmap, f3df))
        return 0;
    if (!(mp = dsi_load(fmap, &mlen)) || mlen < 0x200 + 0x80L * m->nf) {
        free(mp);
        model_free(m);
        return 0;
    }
    for (i = 0; i < m->nf; i++) {
        const unsigned char *r = mp + 0x200 + 0x80 * i;
        Face *f = &m->f[i];
        const uint8_t *idx = r3d_page_index(f->page);
        const uint16_t *rgb = f->page < R3D_PAGES ? g_texpage[f->page] : 0;
        if (u16(r) < 3) {
            f->colour = (int)u16(r + 2);
            continue;
        }
        if ((f->kind == 0x10 || f->kind == 0x26) && !(flag && f->page != 0))
            f->kind = f->kind == 0x10 ? 1 : 2;
        /* corners 0-2 on one texel: a face of that colour, a palette index on a paletted
           page, else the texel itself */
        if ((idx || rgb) && FACE_U(f, 0) == FACE_U(f, 1) && FACE_U(f, 1) == FACE_U(f, 2)
            && FACE_V(f, 0) == FACE_V(f, 1) && FACE_V(f, 1) == FACE_V(f, 2)) {
            int t = ((FACE_V(f, 0) >> 13) << 8) + (FACE_U(f, 0) >> 13);
            f->kind = 0;
            f->colour = idx ? idx[t & 0xffff] : rgb[t & 0xffff];
        }
    }
    free(mp);
    return 1;
}

void model_centre(Model *m)
{
    float lo[3], hi[3], c[3];
    int i, k;
    if (!m->nv)
        return;
    for (k = 0; k < 3; k++)
        lo[k] = hi[k] = m->v[0][k];
    for (i = 1; i < m->nv; i++)
        for (k = 0; k < 3; k++) {
            if (m->v[i][k] < lo[k])
                lo[k] = m->v[i][k];
            if (m->v[i][k] > hi[k])
                hi[k] = m->v[i][k];
        }
    for (k = 0; k < 3; k++)
        c[k] = (lo[k] + hi[k]) * 0.5f;
    for (i = 0; i < m->nv; i++)
        for (k = 0; k < 3; k++)
            m->v[i][k] -= c[k];
}

void model_scale(Model *m, float sx, float sy, float sz)
{
    int i;
    for (i = 0; i < m->nv; i++) {
        m->v[i][0] *= sx;
        m->v[i][1] *= sy;
        m->v[i][2] *= sz;
    }
}

void model_page_add(Model *m, int add)
{
    int i;
    for (i = 0; i < m->nf; i++)
        m->f[i].page += add;
}

void model_free(Model *m)
{
    free(m->v);
    free(m->f);
    memset(m, 0, sizeof *m);
}

/* 0x422070 (the loader's end) and 0x47ad40: x to -x through a matrix built from the
   cosine table, whose quarter turns are not exactly 0 (terms of 1e-8 survive), and every
   face's corners in the reverse order (0x423170) */
void model_mirror(Model *m)
{
    double c0 = g_cos[0], c1 = g_cos[0x1000], c2 = g_cos[0x2000], c3 = g_cos[0x3000];
    float k0 = (float)(c2 * c0);
    double k18 = (float)(c2 * c3 * c3 + c1 * c0), k1c = (float)((c1 - k0) * c3);
    double k20 = (float)-(c1 * c0), k28 = (float)((c1 * c0 + c2) * c3);
    double k24 = (float)(k0 - c3 * c3 * c1), k30 = (float)-(c3 * c0), k34 = (float)(c0 * c0);
    int i, k;
    for (i = 0; i < m->nv; i++) {
        double x = m->v[i][0], y = -(double)m->v[i][1], z = m->v[i][2];
        m->v[i][0] = (float)((k0 * x + k20 * y) + c3 * z);
        m->v[i][1] = (float)((k24 * y + k18 * x) + k30 * z);
        m->v[i][2] = (float)((k1c * x + k28 * y) + k34 * z);
    }
    for (i = 0; i < m->nf; i++) {
        Face *f = &m->f[i];
        for (k = 0; k < f->n / 2; k++) {
            int j = f->n - 1 - k, t;
            t = f->idx[k], f->idx[k] = f->idx[j], f->idx[j] = t;
            {   /* (the texels go round with the corners) */
                float w = f->ut[k];
                f->ut[k] = f->ut[j], f->ut[j] = w;
                w = f->vt[k], f->vt[k] = f->vt[j], f->vt[j] = w;
            }
        }
    }
}

/* 0x420a50 with its last argument 0, as 0x4794b0 calls it for the 3D font: the faces of
   one colour, the colour at MAP + 2 (0x423110); then the mirror and 0x420250 */
int model_load_menu(Model *m, const char *base)
{
    char path[256];
    unsigned char *d, *mp;
    long len, mlen, p;
    int i, k, nf = 0;
    memset(m, 0, sizeof *m);
    snprintf(path, sizeof path, "%s.3df", base);
    d = dsi_load(path, &len);
    snprintf(path, sizeof path, "%s.map", base);
    mp = dsi_load(path, &mlen);
    if (!d || !mp || len < 4 || u16(d) != 1 || mlen < 0x200 || !(mp[1] & 0x80))
        goto bad;
    m->nv = (int)u16(d + 2);
    p = 4 + 12L * m->nv;
    for (;;) {
        unsigned n;
        if (p + 2 > len)
            goto bad;
        if (!(n = u16(d + p)))
            break;
        p += 4 + 2L * n;
        nf++;
    }
    if (mlen < 0x200 + 0x80L * nf)
        goto bad;
    m->v = malloc(sizeof *m->v * (m->nv ? m->nv : 1));
    m->f = calloc(nf ? nf : 1, sizeof *m->f);
    if (!m->v || !m->f)
        goto bad;
    for (i = 0; i < m->nv; i++)
        for (k = 0; k < 3; k++)
            m->v[i][k] = f32(d + 4 + 12 * i + 4 * k) * 96.0f;
    p = 4 + 12L * m->nv;
    for (i = 0; i < nf; i++) {
        const unsigned char *r = mp + 0x200 + 0x80 * i;
        Face *f = &m->f[m->nf];
        unsigned n = u16(d + p);
        uint32_t fl = u32(r + 0x48);
        if (u16(r) >= 3 || (fl & 0x20000) || n > 4) {
            /* textured faces take other kinds on this path (1, 2, 0x15, 0x27) and the
               0x20000 ones go last: the 3D font has none */
            fprintf(stderr, "openmotoracer: %s: a face this loader does not know yet\n", base);
            goto bad;
        }
        f->n = (int)n;
        for (k = 0; k < (int)n; k++) {
            f->idx[k] = (int)u16(d + p + 4 + 2 * k);
            if (f->idx[k] >= m->nv)
                goto bad;
        }
        p += 4 + 2L * n;
        f->colour = (int)u16(r + 2);
        f->flags = 4 | (fl & 0x100000 ? 8 : 0);
        f->kind = fl & 0x80000 ? 0xc : fl & 0x40000 ? 0xd : fl & 0x10000 ? 2 : 0;
        m->nf++;
    }
    free(d);
    free(mp);
    model_mirror(m);
    model_centre(m);
    return 1;
bad:
    fprintf(stderr, "openmotoracer: the model %s could not be read\n", base);
    free(d);
    free(mp);
    model_free(m);
    return 0;
}

static void bounds(const Model *m, float lo[3], float hi[3])
{
    int i, k;
    for (k = 0; k < 3; k++)
        lo[k] = hi[k] = m->nv ? m->v[0][k] : 0.0f;
    for (i = 1; i < m->nv; i++)
        for (k = 0; k < 3; k++) {
            if (m->v[i][k] < lo[k])
                lo[k] = m->v[i][k];
            if (m->v[i][k] > hi[k])
                hi[k] = m->v[i][k];
        }
}

void model_rescale(Model *m, float s)
{
    float lo[3], hi[3], c[3];
    int i, k;
    bounds(m, lo, hi);
    for (k = 0; k < 3; k++)
        c[k] = (float)(((double)hi[k] + lo[k]) * -0.5);
    for (i = 0; i < m->nv; i++)
        for (k = 0; k < 3; k++)
            m->v[i][k] = (float)(((double)m->v[i][k] + c[k]) * s);
}

float model_fit(Model *m, float size)
{
    float lo[3], hi[3], s = 0;
    int k;
    bounds(m, lo, hi);
    for (k = 0; k < 3; k++) {
        float half = (float)(((double)hi[k] - lo[k]) * 0.5), q;
        if (half == 0.0f)
            half = 1.0f;
        q = (float)(size / half);
        if (k == 0 || q < s)
            s = q;
    }
    model_rescale(m, s);
    return s;
}
