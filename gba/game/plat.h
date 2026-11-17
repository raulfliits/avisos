/* AVENTURA DE PLATAFORMAS: piezas comunes (fondos, avioneta, bocadillos, HUD). */
#ifndef PLAT_H
#define PLAT_H
#include "modes.h"

#define VW 240
#define VH 160
#define GY (VH - 24)
#define KK C(0x1b1212)

extern int pfr;          /* = frame del web (pasos lógicos) */
extern float pcam;       /* = cam */
extern int plat_maxn;    /* n máximo de frame_end() visto (rendimiento) */
extern int plat_dbg;     /* estado para el bot de pruebas */

/* ---- fondo: lands = tipos L_* con su x de mundo */
enum { L_SAGRADA, L_VALLES, L_AQUEDUCT, L_SEU, L_MONASTERY, L_INDUSTRIAL, L_BEACH, L_MATARO, L_PYRENEES };
typedef struct { u8 n; s16 x; } land_t;
void bg_build(const land_t *l, int n);   /* pre-dibuja suelo y paisajes (pool) */
void bg_draw(void);                       /* drawBG + suelo, con pcam */
void bg_sky_only(void);

/* ---- avioneta */
typedef struct { float sx, sy; } plane_t;
extern plane_t plane;
void draw_plane_at(float sx, float sy, int pilot, int final_mode);

/* ---- bocadillos (say/BUB_PRI) */
enum { BC_CAND = 1, BC_ME = 2, BC_OK = 4, BC_NO = 8, BC_BAD = 16, BC_MAIL = 32, BC_PLANE = 64 };
enum { AN_FIX, AN_PL, AN_NPC, AN_PLANE, AN_RAUL, AN_FIN };
typedef struct { char t[72]; u8 an, cls, pri, id, bd; s8 slot; s16 ref, wx, wy, dy, w, h, dx, dy2; int until; } bub_t;
extern bub_t bubs[3]; extern int nbub;
/* x,y en mundo; según an: AN_PL x=pl.x y=headY()+dy; AN_NPC x=npc[ref].x y=wy (si ref|0x100: -3 con casco) */
void say(const char *t, int an, int ref, int wx, int wy, int frames, int cls);
void plane_say(const char *t, int cls, int frames);
void clear_bubbles(void);
extern void (*bub_anchor)(bub_t *b, int *x, int *y); /* lo pone cada modo */
void draw_bubbles(void);

/* ---- utilidades */
void sil(const s16 *pts, int n, u8 col);
void draw_hud_top(int show_ctr, int c1, int c2, const char *l1, const char *l2);
static inline int iround(float v) { return v < 0 ? -(int)(-v + 0.5f) : (int)(v + 0.5f); }
static inline int ffloor(float v) { int i = (int)v; return (v < i) ? i - 1 : i; }
void plat_note_n(int n);
void blit_opq(const bmp_t *b, int sx, int sy, int w, int h, int x, int y, int trans);

/* ---- fx de confeti/poof compartidos */
typedef struct { float x, y, vx, vy; s16 life; u8 type, c, item; s16 t; float tx, ty; s8 rec; } pfx_t;
#define MAXFX 120
extern pfx_t fxs[MAXFX]; extern int nfx;
pfx_t *fx_add(int type);
enum { FX_POOF = 1, FX_CONF, FX_KEY, FX_DOC, FX_DOCFLY, FX_EPI, FX_ITEM };
void poof(float x, float y, int n);

int raul_play(void);

void pers_draw(int look, int x, int y, int flip, int stride, int gear, int nrows);
void pers_reset(void);
void plat_ui_reset(void);
void plat_ui_alloc(void);
typedef struct { const bmp_t *b; u16 *ri, *rs; int cap; } rle_t;
int rle_build(rle_t *r, const bmp_t *b, int cap);
void rle_draw(const rle_t *r, int x, int y);
void blit_t(const bmp_t *b, int x, int y, int flip);
int stext(int x, int y, const char *s, u8 col, int align); /* fuente pequeña rápida */
#endif
