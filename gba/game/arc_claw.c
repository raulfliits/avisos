/* GERARD · GRÚA DE MOTOS (ARC_GAMES[2] del web): grúa de peluches con motos, bicis y patinetes.
   Solo cuentan las MOTOS: hay que sacar 3 con 9 fichas.
   La máquina (200x300) es más alta que la pantalla: la vista (MG.view_y) acompaña a la pinza. */
#include "arc_pinball.h"

#define CW 200
#define CH 300
enum { PH_MOVE, PH_DOWN, PH_CLOSE, PH_UP, PH_CARRY, PH_DROP };
enum { T_MOTO, T_BICI, T_PAT };

typedef struct { float x, y; s16 ox, oy; u8 type, look, col, hired, slot; } cl_prize_t;
typedef struct {
    float cx, cy, open; int phase, t, coins, btn, stick, slip;
    int grab, fall, drop; float fall_vy, drop_vy;  /* índices de premio (-1 = ninguno) */
    int ntray, tray[3];
    int npile; s8 pile[40];                        /* G.prizes del web: orden de dibujo */
    int nprize; cl_prize_t pz[40];
    float view; int hold;
} cl_t;
cl_t cl_G;                     /* estado (global para el bot de pruebas) */
static bmp_t *cl_static, *cl_comp;
static u32 cl_bake_end;
static int cl_dirty;
static u8 cl_lut_refl[256];
static bmp_t *cl_pb[40], *cl_pbh[3];   /* peluches pre-dibujados (18x29, ancla 9,17) */

static const u8 *claw_cols(void) {
    static u8 c[7]; c[0] = C(0x2f6fe0); c[1] = C(0x62d26f); c[2] = C(0xff7aa8); c[3] = C(0xffd23f); c[4] = C(0x58c4ff); c[5] = C(0xc45a83); c[6] = C(0xff8a5c);
    return c;
}
/* color + rgba(255,255,255,.45) */
static u8 shine(u8 c) {
    if (c == C(0x2f6fe0)) return C(0x8db0ee); if (c == C(0x62d26f)) return C(0xa9e6b0); if (c == C(0xff7aa8)) return C(0xffb6cf);
    if (c == C(0xffd23f)) return C(0xffe695); if (c == C(0x58c4ff)) return C(0xa3dfff); if (c == C(0xc45a83)) return C(0xdfa4bb);
    if (c == C(0xff8a5c)) return C(0xffbfa5); return C(0xe88992); /* #d62839 */
}
static void halfdisc(int cx, int cy, int r, u8 c) { /* arc(PI,0) relleno: filas de arriba */
    int hw = r, v0 = r * r + r;
    for (int y = 1; y <= r; y++) { int v = v0 - y * y; while (hw > 0 && hw * hw > v) hw--; arcx_hline(cx - hw, cx + hw + 1, cy - y, c); }
}
static void plush_wheel(int cx, int cy, int r) { arcx_disc(cx, cy, r, C(0x1b1212)); if (r >= 3) arcx_disc(cx, cy, 1, C(0x9aa3b5)); else arcx_rect(cx, cy, 1, 1, C(0x9aa3b5)); }

/* drawPlush del web */
static void draw_plush(const cl_prize_t *p, float fx, float fy) {
    int x = (int)(fx + 1000.5f) - 1000, y = (int)(fy + 1000.5f) - 1000;
    const look_t *L = &LOOKS[p->look % 6]; const u8 K = C(0x1b1212); int hired = p->hired; u8 col = claw_cols()[p->col];
    if (p->type == T_MOTO) {
        plush_wheel(x - 6, y + 7, 3); plush_wheel(x + 6, y + 7, 3);
        arcx_rect(x - 8, y - 1, 16, 7, K); u8 bc = hired ? C(0xd62839) : col; arcx_rect(x - 7, y, 14, 5, bc); arcx_rect(x - 6, y, 10, 1, shine(bc));
        arcx_rect(x + 6, y + 1, 2, 2, C(0xffd23f)); arcx_rect(x + 5, y - 4, 1, 4, K);
    } else if (p->type == T_BICI) {
        ring(x - 6, y + 7, 3, K); ring(x + 6, y + 7, 3, K);
        arcx_line(x - 6, y + 7, x - 1, y + 1, col); arcx_line(x - 1, y + 1, x + 6, y + 7, col);
        arcx_line(x - 3, y + 1, x + 4, y + 1, col); arcx_line(x + 4, y + 1, x + 5, y - 3, col);
    } else {
        plush_wheel(x - 7, y + 9, 2); plush_wheel(x + 7, y + 9, 2);
        arcx_rect(x - 8, y + 6, 16, 3, K); arcx_rect(x - 7, y + 6, 14, 1, col); arcx_rect(x + 5, y - 5, 2, 12, K); arcx_rect(x + 2, y - 5, 7, 2, K);
    }
    /* muñeco cabezón */
    arcx_rect(x - 5, y - 6, 10, 8, K); arcx_rect(x - 4, y - 5, 8, 6, hired ? C(0xd62839) : L->C); if (hired) arcx_rect(x - 1, y - 5, 2, 6, C(0xffffff));
    arcx_disc(x, y - 11, 5, K); arcx_disc(x, y - 11, 4, L->S);
    if (hired) { halfdisc(x, y - 13, 5, C(0xd62839)); arcx_rect(x - 5, y - 13, 10, 1, C(0x1b1212)); arcx_rect(x - 3, y - 17, 2, 1, C(0xff8a92)); }
    else halfdisc(x, y - 13, 4, L->H);
    arcx_rect(x - 2, y - 11, 1, 2, K); arcx_rect(x + 2, y - 11, 1, 2, K); arcx_rect(x - 4, y - 9, 1, 1, C(0xff8a92)); arcx_rect(x + 4, y - 9, 1, 1, C(0xff8a92));
    if (hired) { arcx_rect(x - 9, y - 6, 4, 6, C(0xd62839)); arcx_rect(x - 8, y - 4, 2, 1, C(0xffffff)); }
}

/* capa estática: mueble, cartel (sin texto), vitrina, rampa, pórtico, cristal, panel */
static void cl_bake_static(void) {
    const int W = CW, H = CH;
    cl_static = bmp_new(W, H); cl_comp = bmp_new(W, H);
    tgt_bmp(cl_static);
    rect(0, 0, W, H, C(0x7a0a1c)); rect(4, 0, W - 8, H, C(0xa8122b));
    rect(0, 0, W, 36, C(0xd0102c)); rect(0, 34, W, 3, C(0xffd23f));
    arcx_text_outline(W / 2, 13, "FLiiTS GRÚA", C(0x1b1212), C(0x1b1212));
    /* degradado de la vitrina (#ffb3d1 → #c58bd8 → #6b4aa8) en 14 bandas, y la rampa de acrílico encima */
    const u8 BAND[14] = {C(0xfbb0d1),C(0xf4abd2),C(0xeca6d3),C(0xe5a1d4),C(0xdd9cd5),C(0xd696d6),C(0xce91d7),C(0xc78cd8),C(0xba83d2),C(0xab78ca),C(0x9d6ec3),C(0x8f64bb),C(0x805ab3),C(0x724fac)};
    const u8 RAMP[14] = {C(0xfcbed9),C(0xf6bada),C(0xefb6db),C(0xeab2dc),C(0xe3aedd),C(0xdda9dd),C(0xd7a5de),C(0xd1a1df),C(0xc699da),C(0xba90d4),C(0xaf88ce),C(0xa380c7),C(0x9778c1),C(0x8b6fbb)};
    for (int i = 0; i < 14; i++) { int y0 = 40 + (i * 200 + 7) / 14, y1 = 40 + ((i + 1) * 200 + 7) / 14; rect(12, y0, 176, y1 - y0, BAND[i]); }
    static const s16 SKY[11][4] = {{20,150,10,90},{34,160,14,80},{52,128,4,112},{58,120,4,120},{64,124,4,116},{70,130,4,110},{86,150,18,90},{110,140,8,100},{124,108,12,132},{150,150,16,90},{172,136,10,104}};
    for (int i = 0; i < 11; i++) rect(SKY[i][0], SKY[i][1], SKY[i][2], SKY[i][3], C(0x5a3a8f));
    for (int y = 98; y < 108; y++) { int dy = 108 - y; int hw = (int)(6 * fsqrt(1.0f - (dy * dy) / 100.0f) + .5f); rect(130 - hw, y, hw * 2, 1, C(0x5a3a8f)); }
    static const s16 SP[4] = {54, 60, 66, 72};
    for (int i = 0; i < 4; i++) { s16 t[6] = {SP[i] - 2, 124, SP[i], 112, SP[i] + 2, 124}; arcx_poly(t, 3, C(0x5a3a8f)); }
    /* rampa de premio: rgba(255,255,255,.18) */
    for (int y = 150, band = 0; y < 240; y++) {
        while (band < 13 && 40 + ((band + 1) * 200 + 7) / 14 <= y) band++;
        u8 *q = cl_static->px + y * W, rc = RAMP[band];
        for (int x = 14; x < 60; x++) q[x] = q[x] == C(0x5a3a8f) ? C(0x785da3) : rc;
    }
    rect(14, 150, 46, 3, C(0xffd23f)); rect(57, 150, 3, 90, C(0xffd23f)); rect(17, 153, 38, 5, C(0x2a0b10));
    rect(60, 234, 128, 6, C(0xe8b9d8));
    /* pórtico */
    rect(12, 42, 176, 4, C(0xcfd3db)); rect(12, 46, 176, 1, C(0x8a8f9c));
    /* perfil cromado (strokeRect 11,39,178,202 grosor 3) */
    frame_rect(10, 38, 180, 204, C(0xcfd3db)); frame_rect(11, 39, 178, 202, C(0xcfd3db)); frame_rect(12, 40, 176, 200, C(0xcfd3db));
    /* panel de mandos */
    rect(0, 244, W, 56, C(0x2a0b10)); rect(6, 248, W - 12, 48, C(0x120b0c));
    rect(14, 254, 26, 16, C(0x555a66)); rect(25, 257, 4, 10, C(0x000000)); arc_text(27, 284, "FICHAS", C(0xc9b48c), 1, 8);
    rect(54, 270, 14, 4, C(0x333333));
    rect(102, 252, 90, 40, C(0x1d1d22)); frame_rect(102, 252, 90, 40, C(0xffd23f));
    tgt_back();
    arcx_lut(cl_lut_refl, C(0xffffff), 56);   /* rgba(255,255,255,.22) */
}
static void blit_plush(const cl_t *G, int i, float x, float y) {
    const cl_prize_t *p = &G->pz[i]; bmp_t *b = p->hired ? cl_pbh[p->slot] : cl_pb[i];
    (void)G; arcx_blit(b, (int)(x + 1000.5f) - 1000 - 9, (int)(y + 1000.5f) - 1000 - 17, 0);
}
/* capa con el montón de peluches y la bandeja: se rehace solo cuando cambian */
static void cl_compose(int full) {   /* full = 0: solo desde la fila 145 (lo de arriba no cambia) */
    cl_t *G = &cl_G; const int W = CW, y0 = full ? 0 : 145;
    { volatile u32 *dma = (volatile u32 *)0x040000D4; dma[0] = (u32)(cl_static->px + y0 * W); dma[1] = (u32)(cl_comp->px + y0 * W); dma[2] = (u32)(W * (CH - y0) / 4) | 0x84000000u; }
    tgt_bmp(cl_comp);
    for (int k = 0; k < G->npile; k++) { int i = G->pile[k]; blit_plush(G, i, G->pz[i].x, G->pz[i].y); }
    /* reflejos del cristal (encima del montón) */
    for (int y = imax(40, y0); y < 240; y++) {
        int xl = 150 - (y - 40) * 46 / 200, xr = xl + 16; u8 *row = cl_comp->px + y * W;
        for (int x = xl; x < xr; x++) row[x] = cl_lut_refl[row[x]];
        if (y >= 44 && y < 144) { row[18] = cl_lut_refl[row[18]]; row[19] = cl_lut_refl[row[19]]; }
    }
    char b[16], *q = str_cat(b, "MOTOS "); q = str_num(q, G->ntray); str_cat(q, "/3");
    arc_text(147, 255, b, C(0xffffff), 1, 8);
    for (int i = 0; i < G->ntray; i++) blit_plush(G, G->tray[i], 120 + i * 26, 280);
    tgt_back();
    cl_dirty = 0;
}

u32 cl_bake_ticks, cl_comp_ticks;
static void cl_init(void) {
    cl_t *G = &cl_G; memset(G, 0, sizeof *G);
    u32 t0 = ticks();
    int cl_new_bake = 0;
    if (!cl_static || (pool_mark() & 0xFFFFFF) < cl_bake_end) { cl_bake_static(); cl_new_bake = 1; }
    cl_bake_ticks = ticks() - t0;
    static const s16 ROWS[6][2] = {{230,7},{218,7},{206,6},{194,6},{182,5},{170,4}};
    int n = 0;
    for (int r = 0; r < 6; r++) for (int i = 0; i < ROWS[r][1]; i++) {
        cl_prize_t *p = &G->pz[n];
        p->ox = 70 + (r % 2 ? 9 : 0) + i * 17 + (r > 3 ? 18 : r > 1 ? 8 : 0) + ((i * 7 + r * 3) % 5) - 2;
        p->oy = ROWS[r][0] + ((i * 5 + r) % 3) - 1;
        p->x = p->ox; p->y = p->oy; p->look = n % 6; p->col = n % 7; p->hired = 0;
        n++;
    }
    G->nprize = n;
    u8 types[40]; for (int i = 0; i < n; i++) types[i] = i % 4 == 0 ? T_MOTO : i % 2 ? T_BICI : T_PAT;
    for (int i = n - 1; i > 0; i--) { int j = rndi(i + 1); u8 t = types[i]; types[i] = types[j]; types[j] = t; }
    for (int i = 0; i < n; i++) { G->pz[i].type = types[i]; G->pile[i] = i; }
    /* bitmaps de los peluches: se reservan una vez y se redibujan en cada partida (el tipo cambia) */
    if (!cl_pb[0] || cl_new_bake) { for (int i = 0; i < 40; i++) cl_pb[i] = bmp_new(18, 29); for (int i = 0; i < 3; i++) cl_pbh[i] = bmp_new(18, 29); }
    for (int i = 0; i < n; i++) { memset(cl_pb[i]->px, 0, 18 * 29 + 2); tgt_bmp(cl_pb[i]); draw_plush(&G->pz[i], 9, 17); tgt_back(); }
    G->npile = n;
    if (cl_new_bake) cl_bake_end = pool_mark() & 0xFFFFFF;
    G->cx = 120; G->cy = 50; G->phase = PH_MOVE; G->coins = 9; G->open = 1; G->grab = G->fall = G->drop = -1;
    G->view = 0; MG.view_y = 0;
    t0 = ticks(); cl_compose(1); cl_comp_ticks = ticks() - t0;
}

static void cl_press(int b) {
    cl_t *G = &cl_G;
    if (b == B_A && G->phase == PH_MOVE && G->coins > 0 && G->drop < 0) { G->coins--; G->phase = PH_DOWN; G->open = 1; G->btn = 12; slide(500, 250, 300); }
}
static int under(void) {
    cl_t *G = &cl_G; int best = -1;
    for (int k = 0; k < G->npile; k++) { cl_prize_t *p = &G->pz[(int)G->pile[k]]; float d = p->x - G->cx;
        if (d < 11 && d > -11 && (best < 0 || p->y < G->pz[best].y)) best = G->pile[k]; }
    return best;
}
static void pile_remove(int idx) { cl_t *G = &cl_G; int j = 0; for (int k = 0; k < G->npile; k++) if (G->pile[k] != idx) G->pile[j++] = G->pile[k]; G->npile = j; cl_dirty = 1; }
static void pile_add(int idx) {   /* push + sort((a,b)=>b.oy-a.oy) estable */
    cl_t *G = &cl_G; G->pile[G->npile++] = idx;
    for (int a = 1; a < G->npile; a++) { s8 v = G->pile[a]; int b = a - 1; while (b >= 0 && G->pz[(int)G->pile[b]].oy < G->pz[(int)v].oy) { G->pile[b + 1] = G->pile[b]; b--; } G->pile[b + 1] = v; }
    cl_dirty = 1;
}

static void cl_update(void) {
    cl_t *G = &cl_G;
    int L = key_held & K_LEFT, R = key_held & K_RIGHT; G->stick = L ? -1 : R ? 1 : 0; if (G->btn > 0) G->btn--;
    switch (G->phase) {
    case PH_MOVE: if (L) G->cx -= 1.4f; if (R) G->cx += 1.4f; G->cx = fclamp(G->cx, 70, 182); break;
    case PH_DOWN: { G->cy += 1.5f; int top = under(); float floor = top >= 0 ? G->pz[top].y - 26 : 212; if (G->cy >= floor) { G->phase = PH_CLOSE; G->t = 0; tone(150, 80, 0, W_SQUARE, 60); } break; }
    case PH_CLOSE:
        G->t++; G->open = 1 - G->t / 14.0f; if (G->open < 0) G->open = 0;
        if (G->t == 16) { int top = under();
            if (top >= 0) { float dx = G->pz[top].x - G->cx; if (dx < 0) dx = -dx;
                if (rndf() < (dx < 6 ? .96f : .75f)) { G->grab = top; G->slip = rndf() < (dx < 6 ? .05f : .3f); pile_remove(top); tone(400, 60, 0, W_SQUARE, 60); } }
            G->phase = PH_UP; }
        break;
    case PH_UP:
        G->cy -= 1.3f;
        if (G->grab >= 0 && G->slip && G->cy < 130) { G->fall = G->grab; G->fall_vy = 0; G->grab = -1; sfx_back(); arc_banner("¡SE ESCAPA!", 0, 50); }
        if (G->cy <= 50) { G->cy = 50; G->phase = G->grab >= 0 ? PH_CARRY : PH_MOVE; }
        break;
    case PH_CARRY: G->cx -= 1.5f; if (G->cx <= 36) { G->cx = 36; G->phase = PH_DROP; G->t = 0; } break;
    case PH_DROP:
        G->t++; G->open = G->t / 10.0f; if (G->open > 1) G->open = 1;
        if (G->t == 10) { G->drop = G->grab; G->drop_vy = 0; G->grab = -1; }
        if (G->t > 30) G->phase = PH_MOVE;
        break;
    }
    if (G->grab >= 0) { G->pz[G->grab].x = G->cx; G->pz[G->grab].y = G->cy + 30; }
    if (G->fall >= 0) { cl_prize_t *p = &G->pz[G->fall]; G->fall_vy += .25f; p->x += (p->ox - p->x) * .1f; p->y += G->fall_vy;
        if (p->y >= p->oy) { p->y = p->oy; p->x = p->ox; pile_add(G->fall); G->fall = -1; tone(120, 60, 0, W_SQUARE, 60); } }
    if (G->drop >= 0) { cl_prize_t *p = &G->pz[G->drop]; G->drop_vy += .25f; p->y += G->drop_vy;
        if (p->y >= 226) { int i = G->drop; G->drop = -1; G->hold = 70;
            if (p->type == T_MOTO) { p->hired = 1; p->slot = G->ntray; G->tray[G->ntray++] = i; memset(cl_pbh[p->slot]->px, 0, 18 * 29 + 2); tgt_bmp(cl_pbh[p->slot]); draw_plush(p, 9, 17); tgt_back(); cl_dirty = 1; MG.c1++; arc_hire("MOTO"); }
            else { MG.c2++; sfx_back(); arc_banner(p->type == T_BICI ? "UNA BICI..." : "UN PATINETE...", "¡SOLO MOTOS!", 80); } } }
    if (MG.state == ST_PLAY && G->coins <= 0 && G->phase == PH_MOVE && G->drop < 0 && G->fall < 0 && G->grab < 0) arc_lose("¡SIN FICHAS!");
    /* vista: pinza y montón; al soltar, la rampa y la bandeja */
    { float tgt;
      if (G->hold > 0) G->hold--;
      if (G->phase == PH_DROP || G->drop >= 0 || G->hold > 0) tgt = CH - SH;
      else if (G->phase == PH_DOWN || G->phase == PH_CLOSE || G->phase == PH_UP) tgt = fclamp(G->cy - 50, 50, CH - SH);
      else tgt = 50;
      G->view += (tgt - G->view) * (MG.t < 50 ? .04f : .12f); MG.view_y = (int)(G->view + .5f); }
}

u32 cl_draw_ticks, cl_draw_max;
static void cl_draw(void) {
    cl_t *G = &cl_G; const int W = CW; int t = MG.t, vy = MG.view_y; const u8 K = C(0x1b1212);
    u32 t0 = ticks();
    arcx_measure(); arcx_clip(OX, 0, W, SH);
    int was_dirty = cl_dirty;
    if (cl_dirty) { u32 c0 = ticks(); cl_compose(0); cl_comp_ticks = ticks() - c0; }
    arcx_bgcopy(cl_comp);
    if (vy < 37) {   /* cartel luminoso */
        for (int i = 0; i < 20; i++) { u8 c = ((t >> 3) + i) % 3 == 0 ? C(0xfff6b0) : C(0xc9971a); arcx_disc(6 + i * 10, 4, 2, c); arcx_disc(6 + i * 10, 30, 2, c); }
        text(W / 2, 13, "FLiiTS GRÚA", (t >> 4) & 1 ? C(0xffd23f) : C(0xffffff), 1);
    }
    if (G->fall >= 0) blit_plush(G, G->fall, G->pz[G->fall].x, G->pz[G->fall].y);
    /* carro y cable */
    int cx = (int)(G->cx + .5f), cy = (int)(G->cy + .5f); float o = G->open;
    if (vy < 50) { arcx_rect(cx - 8, 41, 16, 7, C(0x555a66)); arcx_rect(cx - 7, 41, 14, 2, C(0x9aa3b5)); }
    arcx_rect(cx, 48, 1, cy - 48, C(0x2a2a33));
    if (G->grab >= 0) blit_plush(G, G->grab, G->pz[G->grab].x, G->pz[G->grab].y);
    /* pinza de tres dedos */
    arcx_rect(cx - 6, cy, 12, 7, C(0x9aa3b5)); arcx_rect(cx - 5, cy + 1, 10, 2, C(0xcfd6e2)); arcx_rect(cx - 6, cy + 7, 12, 1, K);
    { int a1 = (int)(6 * o + .5f), a2 = (int)(4 * o + .5f), m = (int)(2 * (1 - o) + .5f);
      static const s8 OFF[6][2] = {{0,0},{-1,0},{1,0},{0,-1},{0,1},{0,0}};
      for (int k = 0; k < 6; k++) {
        int ex = OFF[k][0], ey = OFF[k][1]; u8 c = k == 5 ? C(0x7f8796) : C(0xe8edf5);
        arcx_line(cx - 5 + ex, cy + 8 + ey, cx - 6 - a1 + ex, cy + 15 + ey, c); arcx_line(cx - 6 - a1 + ex, cy + 15 + ey, cx - 3 - a2 + ex, cy + 22 + ey, c);
        arcx_line(cx + 5 + ex, cy + 8 + ey, cx + 6 + a1 + ex, cy + 15 + ey, c); arcx_line(cx + 6 + a1 + ex, cy + 15 + ey, cx + 3 + a2 + ex, cy + 22 + ey, c);
        arcx_line(cx + ex, cy + 8 + ey, cx + ex, cy + 16 + m + ey, c);
      } }
    if (G->drop >= 0) blit_plush(G, G->drop, G->pz[G->drop].x, G->pz[G->drop].y);
    if (vy < 41) arcx_rect(10, 38, 180, 3, C(0xcfd3db));   /* el perfil cromado tapa el carro */
    /* panel: fichas, joystick y botón */
    if (vy + SH > 250) {
        char b[8]; str_num(b, G->coins); arc_text(27, 274, b, C(0xffd23f), 1, 8);
        int sx = 60 + G->stick * 4;
        arcx_line(61, 272, sx, 258, C(0xcfd6e2)); arcx_line(62, 272, sx + 1, 258, C(0xcfd6e2));
        arcx_disc(sx, 257, 4, C(0xd62839));
        arcx_disc(86, 266, 6, G->btn ? C(0xffd23f) : C(0xc9971a)); arcx_rect(83, 262, 3, 2, C(0xfff6b0));
    } else {   /* fichas y motos visibles aunque el panel no se vea */
        char b[24], *q = str_cat(b, "FICHAS "); q = str_num(q, G->coins); q = str_cat(q, " · MOTOS "); q = str_num(q, G->ntray); str_cat(q, "/3");
        int w = text_len(b) * 4 + 4; arcx_rect(W - w - 4, vy + SH - 9, w + 2, 8, C(0x120b0c)); text_small(W - 3, vy + SH - 8, b, C(0xffd23f), 2);
    }
    cl_draw_ticks = ticks() - t0; if (cl_draw_ticks > cl_draw_max && MG.t > 30 && !was_dirty) cl_draw_max = cl_draw_ticks;
}

const arc_def_t ARC_DEF2 = {"GRÚA DE MOTOS", CW, CH,
    "← →: MOVER LA GRÚA · A: BAJAR. Solo cuentan las MOTOS: ¡saca 3!",
    cl_init, cl_update, cl_draw, cl_press, 0};
