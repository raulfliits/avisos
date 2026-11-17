/* Estado global de partida y utilidades de interfaz comunes a todos los modos. */
#include "modes.h"

const veh_t VEH[3] = {{"MOTO", 2.5f, {3, 3, 1}}, {"BICI", 1.8f, {2, 1, 3}}, {"PATINETE ELÉCTRICO", 2.1f, {2, 2, 2}}};
u8 PROG[3][3], FIN[3], REP_ON, REP_DONE;
s32 SCORE, COINS;
s16 RES_C1, RES_C2; const char *RES_L1, *RES_L2;
int UI_W = SW;

/* Modos aún no portados: definiciones "weak" que se sustituyen por las de verdad. */
static int stub_screen(const char *what) {
    for (;;) {
        cls(C(0x120b0c)); text(120, 60, what, C(0xffd23f), 1); text(120, 80, "PRÓXIMAMENTE", C(0xffffff), 1);
        text(120, 130, "B: VOLVER", C(0x62d26f), 1);
        frame_end(); if (key_hit & (K_B | K_A | K_START)) return 0;
    }
}
__attribute__((weak)) int platform_play(int ci, int veh) { (void)ci; (void)veh; return stub_screen("PLATAFORMA"); }
__attribute__((weak)) int rpg_play(int ci) { (void)ci; return stub_screen("RPG"); }
__attribute__((weak)) int final_play(void) { return stub_screen("FINAL"); }

void arc_text(int x, int y, const char *t, u8 col, int align, int size) {
    if (size <= 6) { text_small(x + 1, y + 1, t, C(0x000000), align); text_small(x, y, t, col, align); }
    else { text(x + 1, y + 1, t, C(0x000000), align); text(x, y, t, col, align); }
}
void arc_bubble(int x, int y, const char *t, u8 col, u8 bg) {
    int w = text_len(t) * 8 + 8, bx = iclamp(x - w / 2, 2, UI_W - w - 2);
    rect(bx, y, w, 13, bg); frame_rect(bx, y, w, 13, col); text(bx + 4, y + 3, t, col, 0);
}
void ui_box(int x, int y, int w, int h, u8 bg, u8 border) { rect(x, y, w, h, bg); frame_rect(x, y, w, h, border); }
void ui_wait_a(void) { do frame_end(); while (!(key_hit & (K_A | K_START))); }

/* Menú de pausa sobre la imagen actual (se copia la página visible a la oculta). */
extern void game_save(void);
__attribute__((weak)) void game_save(void) {}
int pause_menu(void) {
    static const char *const OPT[3] = {"CONTINUAR", "GUARDAR PARTIDA", "SALIR AL SELECTOR"};
    volatile u16 *dc = (volatile u16 *)0x04000000;
    u8 *front = (u8 *)((*dc & 0x10) ? 0x0600A000 : 0x06000000);
    static u8 snap[SW * SH] EWRAM_BSS;
    memcpy(snap, front, SW * SH);
    { bmp_t sb = {snap, SW, SH, 0}; tgt_bmp(&sb); rect_dark(0, 0, SW, SH); rect_dark(40, 30, 160, 100); tgt_back(); }
    int c = 0; const char *msg = 0; int mt = 0;
    sfx_select();
    for (;;) {
        extern bmp_t T; volatile u32 *dma = (volatile u32 *)0x040000D4; dma[0] = (u32)snap; dma[1] = (u32)T.px; dma[2] = 0x84000000u | (SW * SH / 4);
        ui_box(40, 30, 160, 100, C(0x120b0c), C(0xffd23f));
        text(120, 40, "PARTIDA EN PAUSA", C(0xffd23f), 1);
        for (int i = 0; i < 3; i++) { if (i == c) text(52, 64 + i * 16, "▶", C(0xffd23f), 0); text(64, 64 + i * 16, OPT[i], i == c ? C(0xffffff) : C(0x9aa3b5), 0); }
        if (msg && mt > 0) { text(120, 116, msg, C(0x62d26f), 1); mt--; }
        frame_end();
        if (key_hit & K_UP) { c = (c + 2) % 3; sfx_tick(); }
        if (key_hit & K_DOWN) { c = (c + 1) % 3; sfx_tick(); }
        if (key_hit & (K_START | K_B)) { sfx_back(); return 0; }
        if (key_hit & K_A) {
            if (c == 0) { sfx_select(); return 0; }
            if (c == 1) { game_save(); sfx_coin(); msg = "PARTIDA GUARDADA"; mt = 90; }
            if (c == 2) { sfx_back(); return 1; }
        }
    }
}
