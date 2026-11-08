/* The game's files: the player's game folder as it is. Windows names come in any case, so
   every part of a path is matched against the folder's entries without regard to case.

   The game reads through its own layer (dsi_fopen 0x42ef80): a file asked for read-only
   is looked for first in a bank, DATA\<first folder>.BKF (the path split at 0x42fb90:
   upper case, "DATA", the bank's name, then the rest joined by '\'), then loose in the
   install folder, then on the CD. A bank (NOTES §3.1): u32 count, records of 0x2c bytes
   (name[0x24], u32 offset, u32 size). */
#include "files.h"
#include <SDL.h>
#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#ifdef _WIN32
#include <windows.h>
#endif

#define MAX_BANKS 32

static char g_data[1024];

typedef struct {
    char name[16];                 /* "MENU" */
    unsigned char *d;              /* its table (u32 count, 0x2c-byte records); 0 if unread */
    unsigned long len;             /* the file's length */
    unsigned count;
    char path[1024];               /* where it is: a file is read from it when asked for */
} Bank;

static Bank g_banks[MAX_BANKS];
static int g_nbanks;

const char *files_data(void) { return g_data; }

/* the program's own folder, ending in its separator; "" if not known. SDL says it in UTF-8;
   Windows' fopen and opendir, which everything here goes through, want the ANSI code page */
static void program_folder(char *out, size_t size)
{
    char *base = SDL_GetBasePath();
    out[0] = 0;
    if (!base)
        return;
#ifdef _WIN32
    {
        WCHAR wide[1024];
        if (!MultiByteToWideChar(CP_UTF8, 0, base, -1, wide, 1024)
            || !WideCharToMultiByte(CP_ACP, 0, wide, -1, out, (int)size, 0, 0))
            out[0] = 0;
    }
#else
    snprintf(out, size, "%s", base);
#endif
    SDL_free(base);
}

int files_init(const char *data)
{
    char dir[768];
    if (data) {
        snprintf(g_data, sizeof g_data, "%s", data);
        return files_find("data", 0, 0);
    }
    /* not told: "Moto Racer" here, or beside the program, or the program's own folder when
       it was put in with game.EXE */
    snprintf(g_data, sizeof g_data, "Moto Racer");
    if (files_find("data", 0, 0))
        return 1;
    program_folder(dir, sizeof dir);
    if (!dir[0])
        return 0;
    snprintf(g_data, sizeof g_data, "%sMoto Racer", dir);
    if (files_find("data", 0, 0))
        return 1;
    snprintf(g_data, sizeof g_data, "%s", dir);
    if (files_find("data", 0, 0))
        return 1;
    snprintf(g_data, sizeof g_data, "%sMoto Racer", dir);
    return 0;
}

/* dir/name, with name's real case in dir; 0 if not there */
static int find_in(const char *dir, const char *name, char *out, size_t size)
{
    DIR *d = opendir(dir);
    struct dirent *e;
    int found = 0;
    if (!d)
        return 0;
    while ((e = readdir(d))) {
        if (strcasecmp(e->d_name, name) == 0) {
            int n = snprintf(out, size, "%s/%s", dir, e->d_name);
            found = n > 0 && (size_t)n < size;
            break;
        }
    }
    closedir(d);
    return found;
}

int files_find(const char *rel, char *out, size_t size)
{
    char part[256], at[1024], next[1024];
    snprintf(at, sizeof at, "%s", g_data);
    while (*rel) {
        size_t n = strcspn(rel, "/\\");
        if (n >= sizeof part)
            return 0;
        memcpy(part, rel, n);
        part[n] = 0;
        rel += n;
        while (*rel == '/' || *rel == '\\')
            rel++;
        if (n == 0)
            continue;
        if (!find_in(at, part, next, sizeof next))
            return 0;
        memcpy(at, next, sizeof at);
    }
    if (out && snprintf(out, size, "%s", at) >= (int)size)
        return 0;
    return 1;
}

int files_count(const char *dir, const char *prefix, const char *suffix)
{
    char path[1100];
    DIR *d;
    struct dirent *e;
    size_t np = strlen(prefix), ns = strlen(suffix);
    int n = 0;
    if (!files_find(dir, path, sizeof path) || !(d = opendir(path)))
        return 0;
    while ((e = readdir(d))) {
        size_t l = strlen(e->d_name);
        char full[1400];
        DIR *sub;
        if (l < np + ns || strncasecmp(e->d_name, prefix, np)
            || strcasecmp(e->d_name + l - ns, suffix))
            continue;
        snprintf(full, sizeof full, "%s/%s", path, e->d_name);
        if ((sub = opendir(full))) {            /* (directories are not counted) */
            closedir(sub);
            continue;
        }
        n++;
    }
    closedir(d);
    return n;
}

static unsigned char *read_path(const char *path, long *len)
{
    FILE *f = fopen(path, "rb");
    unsigned char *d;
    long n;
    if (!f)
        return 0;
    fseek(f, 0, SEEK_END);
    n = ftell(f);
    fseek(f, 0, SEEK_SET);
    d = malloc(n > 0 ? n : 1);
    if (d && fread(d, 1, n, f) != (size_t)n) {
        free(d);
        d = 0;
    }
    fclose(f);
    if (d && len)
        *len = n;
    return d;
}

unsigned char *files_read(const char *rel, long *len)
{
    char path[1024];
    return files_find(rel, path, sizeof path) ? read_path(path, len) : 0;
}

static unsigned u32(const unsigned char *p)
{
    return (unsigned)p[0] | (unsigned)p[1] << 8 | (unsigned)p[2] << 16 | (unsigned)p[3] << 24;
}

/* the bank DATA\<name>.BKF: its table read the first time it is asked for, a file out of it
   read from the disk each time (the banks were read whole once: 5-6 MB a track, more than the
   ESP32 has, NOTES §4.26) */
static FILE *g_bank_file;          /* the last bank read from, kept open */
static const Bank *g_bank_open;

static Bank *bank(const char *name)
{
    char rel[64];
    unsigned char h[4];
    Bank *b;
    FILE *f;
    long n;
    int i;
    for (i = 0; i < g_nbanks; i++)
        if (strcasecmp(g_banks[i].name, name) == 0)
            return g_banks[i].d ? &g_banks[i] : 0;
    if (g_nbanks == MAX_BANKS || strlen(name) >= sizeof g_banks[0].name)
        return 0;
    b = &g_banks[g_nbanks++];
    snprintf(b->name, sizeof b->name, "%s", name);
    snprintf(rel, sizeof rel, "data/%s.BKF", name);
    if (!files_find(rel, b->path, sizeof b->path) || !(f = fopen(b->path, "rb")))
        return 0;
    if (fseek(f, 0, SEEK_END) == 0 && (n = ftell(f)) >= 4 && fseek(f, 0, SEEK_SET) == 0
        && fread(h, 1, 4, f) == 4 && 4 + 0x2cul * u32(h) <= (unsigned long)n
        && (b->d = malloc(4 + 0x2cul * u32(h)))) {
        memcpy(b->d, h, 4);
        if (fread(b->d + 4, 0x2c, u32(h), f) != u32(h)) {
            free(b->d);
            b->d = 0;
        }
        b->len = (unsigned long)n;
    }
    fclose(f);
    if (!b->d)
        return 0;
    b->count = u32(b->d);
    return b;
}

static unsigned char *bank_piece(const Bank *b, unsigned off, unsigned size)
{
    unsigned char *d = malloc(size ? size : 1);
    if (!d)
        return 0;
    if (g_bank_open != b) {
        if (g_bank_file)
            fclose(g_bank_file);
        g_bank_file = fopen(b->path, "rb");
        g_bank_open = g_bank_file ? b : 0;
    }
    if (!g_bank_file || fseek(g_bank_file, (long)off, SEEK_SET) != 0
        || fread(d, 1, size, g_bank_file) != size) {
        free(d);
        return 0;
    }
    return d;
}

FILE *dsi_fopen(const char *path, long *len)
{
    char up[256], *rest, found[1024];
    size_t n = strlen(path), i;
    FILE *f;
    if (n < sizeof up) {
        for (i = 0; i <= n; i++)
            up[i] = (char)toupper((unsigned char)path[i]);
        for (i = 0; i < n; i++)
            if (up[i] == '/')
                up[i] = '\\';
        if (strncmp(up, "DATA\\", 5) == 0 && (rest = strchr(up + 5, '\\'))) {
            Bank *b;
            *rest++ = 0;
            if ((b = bank(up + 5)))
                for (i = 0; i < b->count; i++) {
                    const unsigned char *r = b->d + 4 + 0x2c * i;
                    char name[0x25];
                    unsigned off = u32(r + 0x24), size = u32(r + 0x28);
                    memcpy(name, r, 0x24);
                    name[0x24] = 0;
                    if (strcasecmp(name, rest) == 0 && off <= b->len && size <= b->len - off) {
                        if (!(f = fopen(b->path, "rb")) || fseek(f, (long)off, SEEK_SET) != 0) {
                            if (f)
                                fclose(f);
                            return 0;
                        }
                        *len = (long)size;
                        return f;
                    }
                }
        }
    }
    if (!files_find(path, found, sizeof found) || !(f = fopen(found, "rb")))
        return 0;
    if (fseek(f, 0, SEEK_END) != 0 || (*len = ftell(f)) < 0 || fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return 0;
    }
    return f;
}

unsigned char *dsi_load(const char *path, long *len)
{
    char up[256], *bk, *rest;
    size_t n = strlen(path), i;
    if (n < sizeof up) {
        for (i = 0; i <= n; i++)
            up[i] = (char)toupper((unsigned char)path[i]);
        for (i = 0; i < n; i++)
            if (up[i] == '/')
                up[i] = '\\';
        /* "DATA\<bank>\<name in the bank>" */
        if (strncmp(up, "DATA\\", 5) == 0 && (rest = strchr(up + 5, '\\'))) {
            Bank *b;
            *rest++ = 0;
            bk = up + 5;
            if ((b = bank(bk))) {
                for (i = 0; i < b->count; i++) {
                    const unsigned char *r = b->d + 4 + 0x2c * i;
                    char name[0x25];
                    unsigned off = u32(r + 0x24), size = u32(r + 0x28);
                    memcpy(name, r, 0x24);
                    name[0x24] = 0;
                    if (strcasecmp(name, rest) == 0 && off <= b->len && size <= b->len - off) {
                        unsigned char *d = bank_piece(b, off, size);
                        if (!d)
                            return 0;
                        if (len)
                            *len = size;
                        return d;
                    }
                }
            }
        }
    }
    return files_read(path, len);
}
