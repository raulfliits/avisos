/* FINAL: las mochilas gigantes (startFinal/finalStep/drawBag/drawFinal del web).
   En 240 de ancho las mochilas van 10 px más a la izquierda (196/230 → 186/220) y Raül anda un poco menos. */
#include "plat.h"

typedef struct { s16 x; u8 col, dk, lt; u8 st[30][2]; int n, hit; bmp_t *bb; int bn, bd; } bag_t;
static bag_t BAGS[2]; static rle_t CRB[3], RAB[2];
static int abs3(float v) { if (v < 0) v = -v; return iround(v * 3); }
static struct { int t, flash, done; float rx; u8 armor; struct { float x, y; u8 on, gone; } coins[3]; } fin;
s32 final_dbg[4];

static void fin_anchor(bub_t *b, int *x, int *y) { if (b->an == AN_FIN) { *x = iround(fin.rx); *y = b->wy; } }

static void add_sticker(void) {
    bag_t *b = 0; for (int i = 0; i < 2; i++) if (BAGS[i].n < 30) { b = &BAGS[i]; break; }
    if (!b) return;
    b->st[b->n][0] = 1 + rndi(19); b->st[b->n][1] = 1 + rndi(14); b->n++; b->hit = 3; tone(700 + rndi(300), 40, 0, W_SQUARE, 40);
}
static int final_step(void) {
    int t = ++fin.t; pfr++;
    if (t < 100) { plane.sx = -30 + t * 1.5f; plane.sy = 26 + (t / 100.0f) * (GY - 46); }
    if (t == 100) { say("¡Para ti, Raül!", AN_FIX, 0, 44, 64, 80, BC_ME); sfx_select(); }
    if (t >= 130 && t < 200) for (int i = 0; i < 3; i++) { if (!fin.coins[i].on) continue;
        float k = (t - 130) / 70.0f, a = k * 8 + i * 2.1f, r = (1 - k) * 30;
        fin.coins[i].x = fin.rx + fcos(a) * r; fin.coins[i].y = GY - 40 + fsin(a) * r * .6f - (1 - k) * 20; }
    if (t == 200) { for (int i = 0; i < 3; i++) fin.coins[i].gone = 1; fin.armor = 1; fin.flash = 20; sfx_power(); say("¡¡PODER FLiiTS!!", AN_FIN, 0, 0, GY - 36, 110, BC_BAD); }
    if (t == 218) sfx_win();
    if (t >= 250 && t < 330) fin.rx += .97f;
    if (t == 300) say("¡¡NO HAY MÁS ZONAS!!", AN_FIX, 0, 203, GY - 30, 90, BC_BAD);
    if (t >= 340 && !fin.done) { if (t % 6 == 0) add_sticker(); if (BAGS[0].n >= 30 && BAGS[1].n >= 30) fin.done = t; }
    if (fin.done && t == fin.done + 20) { for (int i = 0; i < 2; i++) say("¡¡ESTÁ BIEN!!", AN_FIX, 0, BAGS[i].x + (i ? 8 : -8), GY - 34, 170, BC_BAD); sfx_win(); }
    if (fin.done && t == fin.done + 190) return 1;
    return 0;
}
static void bag_body(bag_t *b, int dizzy, int x, int y) {
    u8 W = C(0xffffff);
    rect(x, y, 36, 28, KK);
    rect(x + 1, y + 1, 34, 26, b->col); rect(x + 1, y + 1, 34, 4, b->lt); rect(x + 1, y + 21, 34, 5, b->dk); rect(x + 1, y + 5, 3, 16, b->dk);
    if (!dizzy) { rect(x + 7, y + 8, 7, 7, W); rect(x + 22, y + 8, 7, 7, W); rect(x + 10, y + 10, 3, 3, KK); rect(x + 24, y + 10, 3, 3, KK);
        line(x + 5, y + 5, x + 14, y + 8, KK, 2); line(x + 32, y + 5, x + 23, y + 8, KK, 2); rect(x + 8, y + 18, 20, 5, KK);
        for (int i = 0; i < 9; i++) rect(x + 10 + i * 2, y + (i % 2 ? 21 : 18), 1, 3, W); }
    else { int E[2][2] = {{x + 7, y + 8}, {x + 22, y + 8}}; for (int e = 0; e < 2; e++) { int ex = E[e][0], ey = E[e][1]; line(ex, ey, ex + 6, ey + 6, KK, 2); line(ex + 6, ey, ex, ey + 6, KK, 2); }
        for (int i = 0; i < 16; i++) pset(x + 10 + i, y + 20 + ((i >> 1) % 2), KK); }
    for (int i = 0; i < b->n; i++) { int px = x + iround(b->st[i][0] * 1.5f), py = y + iround(b->st[i][1] * 1.5f);
        rect(px - 1, py - 1, 9, 8, KK); rect(px, py, 7, 6, C(0xd62839)); rect(px + 1, py + 1, 2, 3, W); rect(px + 1, py + 1, 4, 1, W); rect(px + 1, py + 3, 3, 1, W); }
}
static void draw_bag(bag_t *b, int dizzy) {
    int bounce = dizzy ? 0 : abs3(fsin((pfr + b->x) / 7.0f)), x = b->x - 18, y = GY - 31 - bounce;
    rect(x + 5, y + 27, 7, 4 + bounce, KK); rect(x + 24, y + 27, 7, 4 + bounce, KK);
    if (!b->bb) b->bb = bmp_new(36, 28);
    if (b->bb && (b->bn != b->n || b->bd != dizzy)) { b->bn = b->n; b->bd = dizzy; tgt_bmp(b->bb); bag_body(b, dizzy, 0, 0); tgt_back(); }
    if (b->bb) blit_opq(b->bb, 0, 0, 36, 28, x, y, 0); else bag_body(b, dizzy, x, y);
    if (b->hit > 0) { b->hit--; rect_light(x + 1, y + 1, 34, 26); }
    if (dizzy) for (int j = 0; j < 3; j++) { float a = pfr / 10.0f + j * 2.09f; int sx = iround(x + 18 + fcos(a) * 17), sy = iround(y - 8 + fsin(a) * 4);
        rect(sx - 2, sy - 1, 5, 3, KK); rect(sx - 1, sy - 2, 3, 5, KK); rect(sx - 1, sy, 3, 1, C(0xffd23f)); rect(sx, sy - 1, 1, 3, C(0xffd23f)); }
}
static void final_draw(void) {
    bg_draw();
    if (fin.t >= 100) draw_plane_at(120, GY - 14, 0, 1);
    static const s8 CR[3][2] = {{20, 0}, {38, 2}, {56, 3}};
    for (int i = 0; i < 3; i++) { if (!CRB[i].b) { memset(&CRB[i], 0, sizeof CRB[i]); rle_build(&CRB[i], rows_bmp(CREW[CR[i][1]].spr, 24, P_PAL, 0), 24 * 8); }
        if (CRB[i].b) rle_draw(&CRB[i], CR[i][0] - 8, GY - 24); else draw_spr(CREW[CR[i][1]].spr, 24, CR[i][0] - 8, GY - 24, 0, 0); }
    for (int i = 0; i < 5; i++) pers_draw(i % 6, 68 + i * 17, GY - 24, 0, (pfr >> 3) & 1, 15, 24);
    for (int i = 0; i < 2; i++) draw_bag(&BAGS[i], !!fin.done);
    if (fin.t >= 100) { int x = iround(fin.rx - 8);
        if (fin.armor) { if ((pfr >> 2) & 1) rect_dither(x - 2, GY - 26, 20, 26, C(0xffd23f)); else rect_light(x - 2, GY - 26, 20, 26); }
        int v = fin.armor ? 1 : 0;
        if (!RAB[v].b) { memset(&RAB[v], 0, sizeof RAB[v]); rle_build(&RAB[v], rows_bmp(CREW[1].spr, 24, v ? P_ARMOR : P_PAL, 0), 24 * 8); }
        if (RAB[v].b) rle_draw(&RAB[v], x, GY - 24); else draw_pal(CREW[1].spr, 24, x, GY - 24, v ? P_ARMOR : P_PAL, 0);
        if (fin.armor) { rect(x + 2, GY - 27, 12, 2, KK); u8 Y = C(0xffd23f); rect(x + 3, GY - 29, 2, 2, Y); rect(x + 7, GY - 30, 2, 3, Y); rect(x + 11, GY - 29, 2, 2, Y); } }
    if (fin.flash > 0) { fin.flash--; if (fin.flash > 12) rect(0, 0, VW, VH, C(0xffffff)); else if (fin.flash > 4) rect_light(0, 0, VW, VH); }
    for (int i = 0; i < 3; i++) if (fin.coins[i].on && !fin.coins[i].gone) big_coin(iround(fin.coins[i].x), iround(fin.coins[i].y), pfr);
    if (fin.t < 100) draw_plane_at(plane.sx, plane.sy, 1, 1);
    draw_hud_top(0, 0, 0, 0, 0);
    draw_bubbles();
}
int final_play(void) {
    u32 mark = pool_mark(); people_cache_reset(); pers_reset(); plat_ui_reset(); plat_ui_alloc();
    static const land_t PY[1] = {{L_PYRENEES, 128}};
    pcam = 0; bg_build(PY, 1);
    memset(&fin, 0, sizeof fin); fin.rx = 92;
    BAGS[0].x = 186; BAGS[0].col = C(0xffc244); BAGS[0].dk = C(0xd99a1c); BAGS[0].lt = C(0xffe08a);
    BAGS[1].x = 220; BAGS[1].col = C(0x2bb673); BAGS[1].dk = C(0x1d8a55); BAGS[1].lt = C(0x7fe0b0);
    BAGS[0].n = BAGS[1].n = BAGS[0].hit = BAGS[1].hit = 0; BAGS[0].bb = BAGS[1].bb = 0; BAGS[0].bn = BAGS[1].bn = -1;
    memset(CRB, 0, sizeof CRB); memset(RAB, 0, sizeof RAB);
    int any = PROG[0][0] || PROG[0][1] || PROG[0][2];
    for (int i = 0; i < 3; i++) { fin.coins[i].on = any ? PROG[0][i] : 1; fin.coins[i].x = 20 + i * 18; fin.coins[i].y = GY - 46; }
    clear_bubbles(); nfx = 0; bub_anchor = fin_anchor; plat_maxn = 0; plane.sx = -30; plane.sy = 26;
    int result = 0;
    final_draw(); frame_end();
    for (;;) {
        int n = frame_end(); plat_note_n(n);
        if (key_hit & K_START) { if (pause_menu()) { result = 0; break; } frame_end(); continue; }
        if ((key_hit & K_A) && fin.t >= 340 && !fin.done) for (int i = 0; i < 4; i++) add_sticker();
        int done = 0;
        for (int i = 0; i < n && !done; i++) done = final_step();
        final_dbg[0] = fin.t; final_dbg[1] = plat_maxn; final_dbg[2] = fin.done; final_dbg[3] = BAGS[0].n + BAGS[1].n;
        if (done) { result = 1; break; }
        final_draw();
    }
    clear_bubbles(); bub_anchor = 0;
    pool_reset(mark); people_cache_reset(); pers_reset(); plat_ui_reset();
    return result;
}
