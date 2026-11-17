/* Motor mínimo de FLiiTS Catalunya World para Game Boy Advance.
   Modo 4 (240x160, 8 bits por píxel, paleta de 256) con doble página.

   COLORES: en el código se escribe C(0xRRGGBB). build.py recoge todos los C(...) de todos los .c,
   construye la paleta (si hay más de 255 colores, fusiona los más parecidos) y sustituye cada C(...)
   por su índice (u8). El índice 0 es "transparente" (en pantalla se ve negro).
   Para colores calculados en tiempo de ejecución: col_rgb(r,g,b) busca el más parecido de la paleta. */
#ifndef ENG_H
#define ENG_H
#include <stdint.h>
#include <string.h>

typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32;
typedef int8_t s8; typedef int16_t s16; typedef int32_t s32;

#define SW 240
#define SH 160
#define IWRAM_CODE __attribute__((section(".iwram"), target("arm"), long_call, noinline))
#define EWRAM_BSS __attribute__((section(".ewram")))

#ifndef C
#define C(x) ((u8)col_hex(x)) /* solo si no lo sustituye build.py (no debería pasar) */
#endif

/* ---------------------------------------------------------------- colores */
u8 col_rgb(int r, int g, int b);        /* el más parecido de la paleta */
u8 col_hex(u32 rgb);
u32 col_of(u8 c);                       /* 0xRRGGBB de un índice */
u8 col_shade(u8 c, int k256);           /* multiplica por k/256 (oscurecer) */
u8 col_mix(u8 a, u8 b, int k256);       /* a*(256-k)+b*k */
extern u8 LUT_DARK[256];                /* ≈ rgba(0,0,0,.5) encima */
extern u8 LUT_LIGHT[256];               /* ≈ rgba(255,255,255,.4) encima */

/* ---------------------------------------------------------------- destino de dibujo */
typedef struct { u8 *px; s16 w, h; u8 vram; } bmp_t;
void tgt_screen(void);                  /* dibujar en la página oculta de la pantalla */
void tgt_bmp(bmp_t *b);                 /* dibujar en un bitmap (guarda el destino anterior; origen 0,0) */
void tgt_back(void);                    /* vuelve al destino anterior a tgt_bmp */
void origin(int x, int y);              /* desplaza todo lo que se dibuja (como ctx.translate) */
void clip(int x, int y, int w, int h);  /* recorte en coordenadas de pantalla (sin origen) */
void clip_reset(void);
extern s16 OX, OY;
bmp_t *bmp_new(int w, int h);           /* del pool de EWRAM (160 KB), limpio (transparente) */
u32 pool_mark(void);
void pool_reset(u32 mark);              /* libera todo lo reservado después de la marca */

/* ---------------------------------------------------------------- primitivas (recortan al destino) */
void cls(u8 c);
IWRAM_CODE void rect(int x, int y, int w, int h, u8 c);
IWRAM_CODE void pset(int x, int y, u8 c);
IWRAM_CODE void bits_draw(int x, int y, const u8 *bm, int nrows, int w, u8 c); /* filas de bits, MSB izq. */
u8 pget(int x, int y);
void line(int x0, int y0, int x1, int y1, u8 c, int t);   /* como line() del JS: cuadrados t x t */
IWRAM_CODE void rect_lut(int x, int y, int w, int h, const u8 *lut); /* cambia cada píxel por lut[p] (sombras) */
#define rect_dark(x, y, w, h) rect_lut(x, y, w, h, LUT_DARK)
#define rect_light(x, y, w, h) rect_lut(x, y, w, h, LUT_LIGHT)
void rect_dither(int x, int y, int w, int h, u8 c);       /* 50 % a cuadros */
void frame_rect(int x, int y, int w, int h, u8 c);        /* solo el borde de 1 px */
void circle(int cx, int cy, int r, u8 c);                 /* círculo relleno */
void ring(int cx, int cy, int r, u8 c);                   /* circunferencia */
void ellipse(int cx, int cy, int rx, int ry, u8 c);       /* elipse rellena */
void poly(const s16 *pts, int n, u8 c);                   /* polígono relleno: pares x,y */
void blit(const bmp_t *b, int x, int y, int flip);        /* 0 = transparente */
IWRAM_CODE void blit_part(const bmp_t *b, int sx, int sy, int w, int h, int x, int y, int flip);
void blit_scaled(const bmp_t *b, int x, int y, int w, int h, int flip);

/* Sprites de cadenas como en el JS: cada carácter se busca en pal[128] (0 = no pinta). '.' nunca pinta. */
IWRAM_CODE void rows(const char *const *spr, int n, int x, int y, const u8 *pal, int flip);
bmp_t *rows_bmp(const char *const *spr, int n, const u8 *pal, int flip);
void pal_load(u8 *pal, const char *chars, const u8 *cols); /* pal[chars[i]] = cols[i] */

/* ---------------------------------------------------------------- texto (UTF-8) */
/* Fuente Press Start 2P 8x8 (con acentos, ¡¿·×✓✗→←↑↓★☆▶…). align: 0 izq, 1 centro, 2 dcha.
   scale: 1 = 8 px, 2 = 16 px. Devuelve el ancho dibujado. */
int text(int x, int y, const char *s, u8 col, int align);
int text_s(int x, int y, const char *s, u8 col, int align, int scale);
int text_sh(int x, int y, const char *s, u8 col, u8 shadow, int align); /* con sombra 1 px */
int text_small(int x, int y, const char *s, u8 col, int align);         /* fuente 4x6 (para size 6 del JS) */
int text_len(const char *s);            /* nº de caracteres (UTF-8) */
/* Texto con ajuste de línea a 'cols' caracteres; devuelve nº de líneas. lh = alto de línea. */
int text_wrap(int x, int y, const char *s, u8 col, int cols, int lh, int align);
/* Utilidades de cadenas */
char *str_num(char *dst, s32 v);        /* escribe el número, devuelve el final */
char *str_cat(char *dst, const char *s);/* copia s, devuelve el final */

/* ---------------------------------------------------------------- bucle, entrada, azar */
#define K_A 0x001
#define K_B 0x002
#define K_SEL 0x004
#define K_START 0x008
#define K_RIGHT 0x010
#define K_LEFT 0x020
#define K_UP 0x040
#define K_DOWN 0x080
#define K_R 0x100
#define K_L 0x200
extern u16 key_held, key_hit;
extern u32 frame_no;                    /* frames lógicos (60 Hz) */
/* Muestra la página dibujada (espera al VBlank) y lee los botones.
   Devuelve cuántos 1/60 s han pasado desde la llamada anterior (1..3): ejecuta la lógica tantas veces. */
int frame_end(void);
void wait_frames(int n);                /* espera n frames mostrando lo que haya */
u32 rnd(void);
int rndi(int n);                        /* 0..n-1 */
float rndf(void);                       /* 0..1 */
void fade_to_black(int frames);         /* oscurece la pantalla actual */
void fade_level(int l);                 /* 0 = normal … 16 = negro (afecta a todo) */

/* ---------------------------------------------------------------- sonido (como sfx del JS) */
#define W_SQUARE 0
#define W_SAW 1
#define W_TRI 2
void tone(int hz, int dur_ms, int delay_ms, int wave, int vol100); /* vol100 ~ v*1000 del JS (0.06 → 60) */
void slide(int hz0, int hz1, int dur_ms);
void noise(int dur_ms, int vol100);
void sfx_jump(void); void sfx_bump(void); void sfx_select(void); void sfx_open(void); void sfx_coin(void);
void sfx_tick(void); void sfx_clear(void); void sfx_power(void); void sfx_hit(void); void sfx_win(void); void sfx_back(void);

/* ---------------------------------------------------------------- matemáticas en punto fijo (opcional) */
s32 isin(int a1024);                    /* seno, ángulo 0..1023 = 0..2π, resultado ×4096 */
s32 icos(int a1024);
float fsin(float a); float fcos(float a); float fsqrt(float x); float fatan2(float y, float x);
static inline int iabs(int v) { return v < 0 ? -v : v; }
static inline int imin(int a, int b) { return a < b ? a : b; }
static inline int imax(int a, int b) { return a > b ? a : b; }
static inline int iclamp(int v, int a, int b) { return v < a ? a : v > b ? b : v; }
static inline float fclamp(float v, float a, float b) { return v < a ? a : v > b ? b : v; }

/* ---------------------------------------------------------------- guardado (SRAM) */
void save_write(const void *d, int n);
int save_read(void *d, int n);          /* 1 si había datos válidos */

u32 ticks(void);                        /* ciclos de CPU (16,78 MHz) para medir */
void eng_init(void);
#endif
