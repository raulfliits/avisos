/* Personajes y piezas de dibujo compartidas (portado de la parte PIXEL ART / PEOPLE del juego web). */
#ifndef PEOPLE_H
#define PEOPLE_H
#include "eng.h"
typedef struct { u8 H, S, s, beard, C, c, P; } look_t;
typedef struct { const char *const *spr; char pants, sleeve, skin; u8 helm, dog; const char *name; } crew_t;
#include "people_data.h"
extern const look_t LOOKS[6];
extern const crew_t CREW[4];          /* 0 IVAN · 1 RAÜL · 2 GERARD · 3 AZLAN (mismo orden que el web) */
/* Paletas letra→color (128 entradas) listas tras people_init() */
extern u8 P_PAL[128], P_DOG[128], P_FPAL[128], P_MPAL[128], P_MPAL_I[128], P_MPAL_A[128], P_EVIL[128], P_ARMOR[128], P_IPAL[128];
void people_init(void);
/* drawSpr(g,spr,x,y,flip,stride) con PAL; n = filas */
void draw_spr(const char *const *spr, int n, int x, int y, int flip, int stride);
/* drawPal / drawPalS (escala entera sc) */
void draw_pal(const char *const *spr, int n, int x, int y, const u8 *pal, int flip);
void draw_pal_s(const char *const *spr, int n, int x, int y, const u8 *pal, int flip, int sc);
void draw_evil(const char *const *spr, int n, int x, int y, const u8 *pal, int flip, int sc);
void draw_dog(int f, int x, int y, int flip);   /* DOG[f&1] con DOGPAL; el web lo pinta con flip=true al ir a la derecha */
void wheel(int cx, int cy, int r, int f);
void fbox(int x, int y);                         /* mochila roja FLiiTS 21x13 */
int helmet(int sx, int sy, const char *const *spr, int helm); /* devuelve y0 */
/* composeRider: bitmap 68x44 cacheado. vi: 0 moto, 1 bici, 2 patinete. f: fotograma de rueda (0..7) */
typedef struct { bmp_t *b; s16 ax, h, W, H, bxc, byt; } rider_t;
const rider_t *compose_rider(int ci, int vi, int f);                 /* ci 0..3 = CREW */
const rider_t *compose_rider_c(const crew_t *c, int key, int vi, int f); /* key único (p.ej. 10+look) */
const crew_t *rep_crew(int look);                                    /* repSpr(): rider FLiiTS con la cara de LOOKS[look] */
/* drawPerson: gear = bits G_* ; nrows = filas a pintar (24 = todas) */
#define G_CASCO 1
#define G_CHAQ 2
#define G_GUANT 4
#define G_MOCH 8
void draw_person(int look, int x, int y, int flip, int stride, int gear, int nrows);
bmp_t *person_bmp(int look, int gear, int flip); /* personCv: 16x27 cacheado (sin mochila) */
void draw_worker(int look, int x, int y, int evil, int sc);
void draw_item(int it, int x, int y, int sc);    /* 0 casco 1 chaqueta 2 guantes 3 mochila */
void big_coin(int x, int y, int t);
void draw_stand(int ci, int x, int y);           /* standCanvas: personaje + perro de Ivan */
void people_cache_reset(void);                   /* olvida cachés (tras pool_reset) */
#endif
