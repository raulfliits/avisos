/* Utilidades compartidas por arc_rep.c (MODO REPARTIDOR) y arc_maze.c (REPARTO EXPRÉS). */
#ifndef ARC_REP_H
#define ARC_REP_H
#include "arcade.h"
extern int REP_STARS;            /* estrellas ganadas en el MODO REPARTIDOR (0..9) */
extern int MZ_STARS;             /* estrellas del REPARTO EXPRÉS (0..9) */
extern int REP_NMAX, MZ_NMAX;    /* máx. de pasos lógicos por dibujo (rendimiento) */
/* drawFood(g,k,x,y) del web con escala s2/2 (s2 = 2 → 1x, 3 → 1.5x, 4 → 2x) */
void rep_food(int k, int x, int y, int s2);
/* círculo relleno rápido (solo enteros) */
IWRAM_CODE void rep_circle(int cx, int cy, int r, u8 c);
/* copia opaca rápida de un bitmap al destino actual (respeta origin, recorta a 0..cw x 0..ch del lienzo) */
void rep_copy(const bmp_t *b, int x, int y);
#endif
