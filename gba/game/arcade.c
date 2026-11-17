/* Máquina de arcade común: portada de instrucciones, bucle, cartel, ganar/perder. */
#include "arcade.h"

mg_t MG;

/* ---- juegos de relleno (se sustituyen por los de verdad: definiciones "weak") ---- */
static void st_init(void) {}
static void st_update(void) {}
static void st_draw(void) { cls(C(0x120b0c)); text(MG.gk == 0 ? 102 : 96, 60, "PRÓXIMAMENTE", C(0xffd23f), 1); }
#define STUB(n, t, w, h, hp) __attribute__((weak)) const arc_def_t ARC_DEF##n = {t, w, h, hp, st_init, st_update, st_draw, 0, 0};
STUB(0, "PINBALL DE CONTRATACIÓN", 204, 368, "A: LANZAR LA BOLA · ← →: FLIPPERS · B: GOLPE DOBLE. ¡Dale a los aspirantes para contratarlos!")
STUB(1, "CONTROL DE CITAS", 192, 160, "FLECHAS: MOVER EL ESCÁNER · A: PASA A FORMACIÓN · B: NO PASA. Solo pasa quien tiene cita HOY.")
STUB(2, "GRÚA DE MOTOS", 200, 300, "← →: MOVER LA GRÚA · A: BAJAR. Solo cuentan las MOTOS: ¡saca 3!")
STUB(3, "EPI SHOOTER", 192, 144, "↑ ↓: ANDAR · ← →: GIRAR · A: LANZAR EPI · B: CAMBIAR EPI. ¡Viste a 3 riders de FLiiTS!")
STUB(4, "MODO REPARTIDOR", 224, 160, "MANTÉN ↑ PARA ACELERAR · ← →: CAMBIAR DE CARRIL · ↓/B: FRENAR · A: CLAXON / ENTREGAR. Para en los semáforos en rojo y entrega en el carril derecho, junto a la casa del cliente.")
STUB(5, "REPARTO EXPRÉS", 168, 134, "FLECHAS: MOVERTE. Recoge toda la comida del pedido y llévala a la casa del cliente. ¡Esquiva los coches! El semáforo los para un rato.")

static const arc_def_t *def(int gk) {
    switch (gk) { case 0: return &ARC_DEF0; case 1: return &ARC_DEF1; case 2: return &ARC_DEF2; case 3: return &ARC_DEF3; case 4: return &ARC_DEF4; default: return &ARC_DEF5; }
}

void arc_banner(const char *t, const char *sub, int n) {
    MG.ban_t = t; MG.ban_s = sub; MG.ban_n = n; MG.ban_t0 = MG.t;
    if (sub && sub != MG.ban_buf) { strncpy(MG.ban_buf, sub, 39); MG.ban_buf[39] = 0; MG.ban_s = MG.ban_buf; }
}
void arc_hire(const char *name) {
    MG.hires++; sfx_power();
    if (MG.hires >= 3) { arc_win(); return; }
    char b[40], *p = b; *p = 0;
    if (name) { p = str_cat(p, name); p = str_cat(p, " · "); }
    p = str_num(p, MG.hires); str_cat(p, "/3");
    arc_banner("¡CONTRATADO!", b, 80);
}
void arc_win(void) {
    MG.state = ST_WON;
    arc_banner(MG.gk == 4 ? "¡REPARTIDOR DEL MES!" : MG.ci == 1 ? "¡OFICINA EN ORDEN!" : "¡3 CONTRATADOS!", MG.gk == 4 ? "3 PEDIDOS ENTREGADOS" : "¡MONEDA DE ORO!", 180);
    sfx_win(); sfx_coin();
    MG.endAt = MG.t + 180;
}
void arc_lose(const char *why) { MG.state = ST_LOST; arc_banner("GAME OVER", why, 1000000); sfx_hit(); }

static void draw_banner(int W, int H) {
    if (!MG.ban_t) return;
    if (MG.t - MG.ban_t0 > MG.ban_n) { MG.ban_t = 0; return; }
    int lost = MG.state == ST_LOST, y = H * 36 / 100, hh = MG.ban_s || lost ? 42 : 24;
    rect(6, y - 6, W - 12, hh, C(0x120b0c));
    frame_rect(7, y - 5, W - 14, hh - 2, C(0xffd23f)); frame_rect(8, y - 4, W - 16, hh - 4, C(0xffd23f));
    arc_text(W / 2, y, MG.ban_t, C(0xffd23f), 1, 8);
    if (MG.ban_s) arc_text(W / 2, y + 14, MG.ban_s, C(0xffffff), 1, MG.ban_s && text_len(MG.ban_s) * 8 > W - 16 ? 6 : 8);
    if (lost) arc_text(W / 2, y + 26, "A: OTRA · B: SALIR", (MG.t >> 4) & 1 ? C(0xffffff) : C(0xff8a92), 1, 8);
}

static void intro_card(const arc_def_t *A) {
    for (;;) {
        cls(C(0x120b0c));
        text(120, 10, MG.gk == 4 ? "MODO REPARTIDOR" : "MINI ARCADE", C(0xff8a92), 1);
        text(120, 24, A->title, C(0xffd23f), 1);
        rect(20, 36, 200, 1, C(0xffd23f));
        text_wrap(120, 46, A->help, C(0xffffff), 28, 11, 1);
        if ((frame_no >> 4) & 1) text(120, 144, "A: EMPEZAR · B: VOLVER", C(0x62d26f), 1);
        frame_end();
        if (key_hit & (K_A | K_START)) { sfx_select(); return; }
        if (key_hit & K_B) { MG.state = -1; sfx_back(); return; }
    }
}

int arcade_play(int gk, int ci, int veh, int look) {
    const arc_def_t *A = def(gk);
    memset(&MG, 0, sizeof MG);
    MG.gk = gk; MG.ci = ci; MG.veh = veh; MG.look = look;
    u32 mark = pool_mark();
    intro_card(A);
    if (MG.state == -1) { pool_reset(mark); people_cache_reset(); return 0; }
    MG.state = ST_PLAY;
    A->init();
    int W = A->w, H = A->h, vh = H > SH ? SH : H;
    int ox = (SW - W) / 2, oy = (SH - vh) / 2;
    int result = 0;
    for (;;) {
        int n = frame_end();
        for (int i = 0; i < n; i++) {
            MG.t++;
            if (MG.state == ST_PLAY) A->update();
        }
        /* entradas */
        if (key_hit & K_START) { if (pause_menu()) { result = 0; break; } continue; }
        if (MG.state == ST_LOST) {
            if (key_hit & K_A) { u32 t = MG.t; int g = MG.gk, c = MG.ci, v = MG.veh, l = MG.look; memset(&MG, 0, sizeof MG); MG.gk = g; MG.ci = c; MG.veh = v; MG.look = l; MG.t = t; A->init(); sfx_select(); }
            else if (key_hit & K_B) { result = 0; break; }
        } else if (MG.state == ST_PLAY && A->press) {
            if (key_hit & K_A) A->press(B_A);
            if (key_hit & K_B) A->press(B_B);
            if (key_hit & K_UP) A->press(B_UP);
            if (key_hit & K_DOWN) A->press(B_DOWN);
            if (key_hit & K_LEFT) A->press(B_LEFT);
            if (key_hit & K_RIGHT) A->press(B_RIGHT);
        }
        if (MG.state == ST_WON && MG.t >= MG.endAt) { result = 1; break; }
        /* dibujo */
        cls(C(0x120b0c));
        clip(ox, oy, W, vh); origin(ox, oy - MG.view_y);
        A->draw();
        origin(ox, oy); UI_W = W;
        draw_banner(W, vh);
        clip_reset(); origin(0, 0); UI_W = SW;
        char pb[48];
        if (A->progress) A->progress(pb); else { char *p = str_cat(pb, "CONTRATADOS "); p = str_num(p, MG.hires); str_cat(p, "/3"); }
        rect(0, SH - 7, text_len(pb) * 4 + 4, 7, C(0x120b0c)); text_small(2, SH - 6, pb, C(0xffd23f), 0);
    }
    RES_C1 = MG.c1; RES_C2 = MG.c2;
    pool_reset(mark); people_cache_reset();
    return result;
}
