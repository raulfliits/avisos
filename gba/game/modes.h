/* Interfaz entre el menú principal (main.c) y los modos de juego. */
#ifndef MODES_H
#define MODES_H
#include "eng.h"
#include "people.h"

/* Personajes (índices de CREW): */
#define CI_IVAN 0
#define CI_RAUL 1
#define CI_GERARD 2
#define CI_AZLAN 3
/* Vehículos: 0 MOTO, 1 BICI, 2 PATINETE (VEH del web) */
typedef struct { const char *n; float max; u8 st[3]; } veh_t;
extern const veh_t VEH[3];

/* Progreso: modo 0 plataforma, 1 RPG, 2 arcade · pantalla 0 Ivan, 1 Gerard, 2 Azlan (SCREEN_OF del web) */
extern u8 PROG[3][3], FIN[3], REP_ON, REP_DONE;
extern s32 SCORE, COINS;                /* marcador y monedas normales (plataforma) */
/* Lo que el modo quiera mostrar en la tarjeta de moneda: dos contadores con su etiqueta (opcional) */
extern s16 RES_C1, RES_C2; extern const char *RES_L1, *RES_L2;

/* Cada modo devuelve 1 si se supera (moneda de oro / final), 0 si se sale o se abandona. */
int platform_play(int ci, int veh);     /* ci 0/2/3: su pantalla con su jefe · ci 1: oficina de Raül + final de las mochilas */
int rpg_play(int ci);                   /* ci 0/2/3: su misión · ci 1: oficina de Raül + jefe final */
int arcade_play(int gk, int ci, int veh, int look);
int final_play(void);                   /* startFinal(): jefe final de las mochilas gigantes (plataforma). 1 = superado */ /* gk 0..5 (ARC_INFO del web); 4 repartidor (veh, look de REP_RIDERS), 5 reparto exprés */

/* Utilidades comunes de interfaz (main.c) */
int pause_menu(void);                   /* menú de pausa (START): 1 = salir al selector */
void arc_text(int x, int y, const char *t, u8 col, int align, int size); /* arcText del web: sombra negra; size 8 o 6 */
void arc_bubble(int x, int y, const char *t, u8 col, u8 bg);             /* arcBubble del web (W = ancho del lienzo del modo) */
extern int UI_W;                        /* ancho del lienzo para arc_bubble (por defecto 240) */
/* Bocadillo de diálogo estilo plataforma/RPG: caja con texto ajustado (cols caracteres) */
void ui_box(int x, int y, int w, int h, u8 bg, u8 border);
void ui_wait_a(void);                   /* espera a que se pulse A/START (sin dibujar) */
#endif
