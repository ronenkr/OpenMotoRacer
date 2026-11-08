#ifndef FILES_H
#define FILES_H
#include <stddef.h>

/* the player's game folder (GOG's "Moto Racer": game.EXE and data/) */
int files_init(const char *data);
const char *files_data(void);

/* rel inside the game folder, any case, '\\' or '/' between parts; 0 if not there */
int files_find(const char *rel, char *out, size_t size);
/* how many files of the folder dir (as files_find takes it) are named prefix*suffix, in
   any case (0x443000's scan of Data\List*.bpt) */
int files_count(const char *dir, const char *prefix, const char *suffix);
/* a whole loose file, malloc'd; 0 if missing */
unsigned char *files_read(const char *rel, long *len);

/* a file as the game asks for it ("data\\menu\\bitmap\\title.lez"): from its bank
   (DATA\MENU.BKF, name BITMAP\TITLE.LEZ) first, then loose (0x42ef80) */
/* the same file opened, at its start (in its bank or alone), *len its length: for reading a big
   one bit by bit (the ESP32 has no room for a 1.3 MB track whole) */
#include <stdio.h>
FILE *dsi_fopen(const char *path, long *len);
unsigned char *dsi_load(const char *path, long *len);

#endif
