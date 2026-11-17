/* RPG · CATALUNYA (portátil clásico 160x144, 4 tonos): definiciones internas compartidas. */
#ifndef RPG_INT_H
#define RPG_INT_H
#include "modes.h"

#define GBW 160
#define GBH 144
#define GBX 40          /* la pantalla GB centrada en 240x160 */
#define GBY 8
#define MW 20
#define MH 18
enum { D_UP, D_DOWN, D_LEFT, D_RIGHT };

/* Los 4 tonos son 4 entradas de la paleta del hardware que se reescriben (paleta GB, fundidos, destello). */
extern u8 TN[4];
#define G0 TN[0]
#define G1 TN[1]
#define G2 TN[2]
#define G3 TN[3]
u8 rpg_tq(u32 rgb);                       /* gbQuantize de un color */
void rpg_tones_init(void);
void rpg_tones_set(const u32 *p4);        /* escribe los 4 colores en la paleta del hardware */
void rpg_tones_restore(void);

/* plantillas de mapa */
typedef struct { s8 x, y, w, h; u8 hub; const char *name; const char *msg; } bld_t;
typedef struct { s8 x, y, d; } slot_t;
typedef struct { const bld_t *bld; u8 nbld; slot_t start; const slot_t *slots; u8 nslots; slot_t boss; s8 sx, sy; } tpl_t;
extern tpl_t TPL[4];
extern char GRID[4][MH][MW + 1];
void rpg_tpl_init(void);
int rpg_solid_ch(char ch);

/* gráficos (todo en tonos) */
extern bmp_t *MAPB;                       /* mapa entero pre-dibujado 320x288 */
void rpg_map_build(int tpl);
void rpg_atlas_init(void);
void rpg_map_anim(int tpl, int wv);       /* agua y flores */
void rpg_tile(char ch, int v, int x, int y);
void rpg_cache_reset(void);
void rpg_cache_frame(void);
bmp_t *spr_crew(int ci, int view, int stride);   /* view: D_UP espalda, D_LEFT espejo, resto de frente */
bmp_t *spr_armor(void);
bmp_t *spr_npc(int id, int view, int stride, int red);
bmp_t *spr_person(int id, int gear);             /* gear: bits casco1 chaqueta2 guantes4 mochila8 */
bmp_t *spr_dog(int f, int flip);
bmp_t *spr_boss(int kind, int good);             /* 0 marti, 1 office, 2 almacen */
bmp_t *spr_scaled(bmp_t *src, u32 key, int w, int h);
void rpg_blit_map(int cx, int cy);
void rpg_blit_win(const bmp_t *src, int cx, int cy);               /* copia la ventana 160x144 del mapa a la pantalla */
extern s16 RC[4];
void rpg_clip(int x, int y, int w, int h);
void rpg_blit(const bmp_t *b, int x, int y);
void px_ellipse(int cx, int cy, int rx, int ry, u8 col);
void draw_bags(int x, int y, int dizzy, int stk);
void draw_contract(int x, int y, int sig18, int stamp, int stampR);
void draw_coin(int x, int y, int t);
void draw_doccard(int x, int y);
void draw_building(const bld_t *b, int ox, int oy);
void draw_contract_icon(int x, int y);           /* sprContract 8x8 a 16x16 */
#endif
