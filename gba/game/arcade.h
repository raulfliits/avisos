/* Máquina de arcade común (portado de arcStart/arcTick/arcPress/arcDrawBanner del web).
   Cada juego define un arc_def_t: ARC_DEF0 (pinball, Ivan), ARC_DEF1 (citas, Raül), ARC_DEF2 (grúa, Gerard),
   ARC_DEF3 (EPI shooter, Azlan), ARC_DEF4 (modo repartidor), ARC_DEF5 (reparto exprés).
   El juego dibuja en su lienzo W x H: arcade.c pone origin() centrado en la pantalla de 240x160 y recorta a ese
   rectángulo. Si H > 160 el juego puede desplazar la vista con MG.view_y (0 = arriba). */
#ifndef ARCADE_H
#define ARCADE_H
#include "modes.h"
/* botones para press() (como los "A","B","UP"... del web) */
#define B_A 1
#define B_B 2
#define B_UP 3
#define B_DOWN 4
#define B_LEFT 5
#define B_RIGHT 6
typedef struct {
    const char *title; s16 w, h; const char *help;
    void (*init)(void);        /* = ARC_GAMES[gk].init() → el estado vive en variables estáticas del juego */
    void (*update)(void);      /* 60 veces por segundo mientras MG.state == ST_PLAY (lee key_held para teclas mantenidas) */
    void (*draw)(void);        /* dibuja el lienzo W x H */
    void (*press)(int b);      /* pulsación (opcional) */
    void (*progress)(char *out); /* texto de arcProgress() (opcional; si no, "CONTRATADOS n/3") */
} arc_def_t;
#define ST_PLAY 0
#define ST_WON 1
#define ST_LOST 2
typedef struct { int ci, gk, veh, look, hires, c1, c2, t, state, endAt, view_y; const char *ban_t, *ban_s; int ban_n, ban_t0; char ban_buf[40]; } mg_t;
extern mg_t MG;
void arc_banner(const char *t, const char *sub, int n);
void arc_hire(const char *name);
void arc_win(void);
void arc_lose(const char *why);
extern const arc_def_t ARC_DEF0, ARC_DEF1, ARC_DEF2, ARC_DEF3, ARC_DEF4, ARC_DEF5;
#endif
