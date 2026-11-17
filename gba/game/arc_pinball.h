/* Utilidades compartidas por arc_pinball.c y arc_claw.c */
#ifndef ARC_PINBALL_H
#define ARC_PINBALL_H
#include "arcade.h"
/* Copia opaca y rápida (DMA) de un fondo W x H pre-dibujado a la pantalla, respetando origin() y MG.view_y.
   Requiere OX par y ancho par. */
void arcx_bgcopy(const bmp_t *b);
/* LUT de mezcla con un color (k256 = opacidad del color encima) */
void arcx_lut(u8 *lut, u8 col, int k256);
/* aplica una LUT a un píxel / a un rectángulo (en el destino actual) */
void arcx_lut_px(int x, int y, const u8 *lut);
/* medio disco superior (filas -r..-1), como arc(...,PI,0) relleno */
void arcx_halfdisc(int cx, int cy, int r, u8 c);
/* trazo grueso con extremos redondeados (lineCap round): r = medio grosor */
void arcx_capsule(float x0, float y0, float x1, float y1, float r, u8 c);
void arcx_poly(const s16 *p, int n, u8 c);   /* polígono relleno en enteros (rápido) */
void arcx_octagon(int x, int y, int w, int ch, u8 c);
IWRAM_CODE void arcx_disc(int cx, int cy, int r, u8 c);  /* = circle() rápido */
IWRAM_CODE void arcx_line(int x0, int y0, int x1, int y1, u8 c);
void arcx_clip(int x, int y, int w, int h);
IWRAM_CODE void arcx_hline(int x0, int x1, int y, u8 c);
IWRAM_CODE void arcx_rect(int x, int y, int w, int h, u8 c);
IWRAM_CODE void arcx_blit(const bmp_t *b, int x, int y, int flip); /* blit rápido (transparencia 0) */
/* texto con contorno negro (strokeText + fillText) */
void arcx_text_outline(int x, int y, const char *s, u8 col, u8 out);
/* frames lógicos entre dos dibujos (n de frame_end) y su máximo */
void arcx_measure(void);
extern int arc_maxn;
#endif
