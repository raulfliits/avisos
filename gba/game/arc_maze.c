/* REPARTO EXPRÉS (ARC_GAMES[5] del web): laberinto de calles visto desde arriba.
   Recoge la comida del pedido y llévala a la casa del cliente; los coches patrullan y van a por ti.
   El semáforo los para un rato. 3 pedidos, 3 cascos. Movimiento casilla a casilla en Q16. */
#include "arc_rep.h"

#define F(x) ((s32)((x) * 65536.0))
int MZ_STARS, MZ_NMAX, MZ_NH[4]; u32 MZ_DT, MZ_BT;

static const char *const MZ[3][15] = {
 {"#####################","#S......#.....F....E#","#.###.#.#.###.#.###.#","#.#F..#...#.....#F#.#","#.#.#####.#.#####.#.#","#.....T.........C...#","###.#.###.###.#.###.#","#F..#...#.F.#.#...#.#","#.###.#.#.#.#.###.#.#","#.....#...#.....#...#","#.###.#####.###.#.#.#","#...#.....C...#...#F#","#.#.#.###.###.#.###.#","#.F.#..F......#..C..#","#####################"},
 {"#####################","#E..F#.......#....F.#","#.##.#.#####.#.###..#","#.#....#.F.....#..#.#","#.#.##.#.###.#.#.##.#","#...#..C...#.#...C..#","#.#.#.####.#.#####.##","#.#F..#..T.#.....#..#","#.###.#.####.###.#.##","#.....#....F.#...#..#","###.#####.##.#.#.##.#","#F..#...C..#...#..F.#","#.#.#.#.##.###.###..#","#.#...#.....F.....#S#","#####################"},
 {"#####################","#....F..#.S.#..F....#","#.####.##...##.####.#","#.#..C.........C..#.#","#.#.###.#####.###.#.#","#F..#.....T.....#..F#","###.#.###.#.###.#.###","#...#...#.#.#...#...#","#.#####.#.#.#.#####.#","#...F...#...#...F...#","#.###.#####.#####.#.#","#.#...#...C...#...#.#","#.#.###.#.#.#.###.#.#","#.F.....#.E.#.....F.#","#####################"}};
static const char *const MZ_FOOD[4][6] = {
 {".bbbb.","bbbbbb","gggggg","mmmmmm","bbbbbb",".bbbb."},{"yyyyyy","yryyry",".yyyy.",".yryy.","..yy..","..y..."},
 {"y.yy.y","yyyyyy","rrrrrr","rwrrwr","rrrrrr",".rrrr."},{".wwww.","rrrrrr","rwwwwr","rrrrrr","rrrrrr",".rrrr."}};
/* direcciones: 0 UP, 1 DOWN, 2 LEFT, 3 RIGHT (orden de Object.keys(MZ_DIR)) */
static const s8 DX[4] = {0, 0, -1, 1}, DY[4] = {-1, 1, 0, 0}, OPP[4] = {1, 0, 3, 2};
typedef struct { s8 x, y, dir, want, face, sx, sy; s32 pr; u8 col; } ent_t;
typedef struct { s8 x, y, k; } food_t;
#define PH_ORDER 0
#define PH_RIDE 1
#define PH_DELIV 2
/* global (no static) para el bot de pruebas */
struct {
    s32 lvl, lives, stars, t, phase, time, hits, frozen, inv, total, wait, warned, doneAt, last, nfood, ncars;
    s8 sx, sy, hx, hy, lx, ly, light, house;
    ent_t p; ent_t cars[8]; food_t food[16];
} MZG;
#define G MZG
static char grid[15][22];
static bmp_t *BG, *FOODB[4];
static u32 last_fn;

static void prog(char *o) { char *p = str_cat(o, "PEDIDOS "); p = str_num(p, MG.c1); p = str_cat(p, "/3 · CASCOS "); str_num(p, G.lives); }

static void build_bg(void) {
    if (!BG) BG = bmp_new(168, 120);
    tgt_bmp(BG);
    for (int y = 0; y < 15; y++) for (int x = 0; x < 21; x++) {
        int X = x * 8, Y = y * 8;
        if (grid[y][x] == '#') {
            int edge = y == 0 || y == 14 || x == 0 || x == 20;
            rect(X, Y, 8, 8, edge ? C(0x8c7a5e) : C(0xd9c9a8));
            if (!edge) { rect(X, Y + 7, 8, 1, C(0xb5a283)); rect(X + 1, Y + 1, 2, 2, C(0x7fa6c9)); rect(X + 5, Y + 1, 2, 2, C(0x7fa6c9)); rect(X + 1, Y + 4, 2, 2, C(0x7fa6c9)); rect(X + 5, Y + 4, 2, 2, C(0x7fa6c9)); }
        } else { rect(X, Y, 8, 8, C(0x3a3a46)); if ((x + y) % 2 == 0) rect(X + 3, Y + 3, 2, 2, C(0x4a4a58)); }
    }
    tgt_back();
}
static void reset(void) {
    G.p.x = G.sx; G.p.y = G.sy; G.p.dir = -1; G.p.pr = 0; G.p.want = -1; G.p.face = 3;
    for (int i = 0; i < G.ncars; i++) { ent_t *c = &G.cars[i]; c->x = c->sx; c->y = c->sy; c->dir = -1; c->pr = 0; c->want = -1; c->face = 2; }
    static u8 cc[7]; cc[0] = C(0x2f6fe0); cc[1] = C(0xe8e8e8); cc[2] = C(0x62d26f); cc[3] = C(0xc45a83); cc[4] = C(0xff8a5c); cc[5] = C(0x8a94aa); cc[6] = C(0xffd23f);
    for (int i = 0; i < G.ncars; i++) G.cars[i].col = cc[(i * 2 + G.lvl) % 7];
    G.wait = 90;
}
static void load(void) {
    int k = 0; G.nfood = G.ncars = 0; G.light = 0; G.house = 0;
    for (int y = 0; y < 15; y++) for (int x = 0; x < 21; x++) {
        char ch = MZ[G.lvl][y][x]; grid[y][x] = ch;
        if (ch == 'S') { G.sx = x; G.sy = y; }
        else if (ch == 'E') { G.hx = x; G.hy = y; G.house = 1; }
        else if (ch == 'F') { food_t *f = &G.food[G.nfood++]; f->x = x; f->y = y; f->k = k++ % 4; }
        else if (ch == 'C') { ent_t *c = &G.cars[G.ncars++]; c->sx = x; c->sy = y; }
        else if (ch == 'T') { G.lx = x; G.ly = y; G.light = 1; }
    }
    G.phase = PH_ORDER; G.time = 0; G.hits = 0; G.frozen = 0; G.inv = 0; G.total = G.nfood;
    build_bg(); reset();
}
static void init(void) {
    if (MG.t == 0) { BG = 0; for (int i = 0; i < 4; i++) FOODB[i] = 0; }
    if (!FOODB[0]) {
        u8 pal[128]; memset(pal, 0, 128);
        pal['b'] = C(0xe09a3c); pal['g'] = C(0x62d26f); pal['m'] = C(0x6b3a1f); pal['y'] = C(0xffd23f); pal['r'] = C(0xd0102c); pal['w'] = C(0xffffff);
        for (int i = 0; i < 4; i++) FOODB[i] = rows_bmp(MZ_FOOD[i], 6, pal, 0);
    }
    memset(&G, 0, sizeof G); G.lives = 3; MZ_STARS = 0; last_fn = 0;
    load();
}
static int open_(int x, int y, int car) {
    if (y < 0 || y >= 15 || x < 0 || x >= 21) return 0;
    char ch = grid[y][x]; if (ch == '#') return 0; if (car && ch == 'E') return 0; return 1;
}
static void pos(const ent_t *e, s32 *x, s32 *y) {
    int d = e->dir; *x = (e->x << 16) + (d >= 0 ? DX[d] * e->pr : 0); *y = (e->y << 16) + (d >= 0 ? DY[d] * e->pr : 0);
}
static int choose_car(ent_t *e) {
    int opts[4], n = 0;
    for (int d = 0; d < 4; d++) if ((e->dir < 0 || d != OPP[(int)e->dir]) && open_(e->x + DX[d], e->y + DY[d], 1)) opts[n++] = d;
    if (!n) { if (e->dir >= 0) { opts[0] = OPP[(int)e->dir]; n = 1; } else return -1; }
    int chase = F(.35) + G.lvl * F(.15);
    if ((s32)(rnd() & 0xFFFF) < chase) {
        int best = opts[0], bd = 9999;
        for (int i = 0; i < n; i++) { int d = opts[i], dd = iabs(e->x + DX[d] - G.p.x) + iabs(e->y + DY[d] - G.p.y); if (dd < bd) { bd = dd; best = d; } }
        return best;
    }
    return opts[rndi(n)];
}
static void step(ent_t *e, s32 sp, int car) {
    if (e->dir >= 0 && e->want >= 0 && e->want == OPP[(int)e->dir] && e->pr > 0) { e->x += DX[(int)e->dir]; e->y += DY[(int)e->dir]; e->pr = 65536 - e->pr; e->dir = e->want; }
    if (e->dir < 0 || e->pr == 0) {
        int d;
        if (car) d = choose_car(e);
        else d = (e->want >= 0 && open_(e->x + DX[(int)e->want], e->y + DY[(int)e->want], 0)) ? e->want : e->dir;
        e->dir = (d >= 0 && open_(e->x + DX[d], e->y + DY[d], car)) ? d : -1;
        if (e->dir >= 0) e->face = e->dir;
    }
    if (e->dir < 0) return;
    e->pr += sp;
    if (e->pr >= 65536) { e->x += DX[(int)e->dir]; e->y += DY[(int)e->dir]; e->pr = 0; }
}
static void deliver(void) {
    int tgt = 50 + G.lvl * 5;
    G.phase = PH_DELIV; G.doneAt = G.t;
    G.last = G.hits == 0 && G.time <= tgt * 60 ? 3 : G.hits <= 1 ? 2 : 1; G.stars += G.last; MZ_STARS = G.stars; MG.c1 = G.lvl + 1; sfx_win();
}
static void next(void) { G.lvl++; if (G.lvl >= 3) { arc_win(); return; } load(); sfx_select(); }
static void press(int b) {
    if (G.phase == PH_ORDER) { if (b == B_A) { G.phase = PH_RIDE; sfx_open(); } return; }
    if (G.phase == PH_DELIV) { if (b == B_A && G.t > G.doneAt + 30) next(); return; }
    if (b == B_UP) G.p.want = 0; else if (b == B_DOWN) G.p.want = 1; else if (b == B_LEFT) G.p.want = 2; else if (b == B_RIGHT) G.p.want = 3;
}
static const s32 PSP[3] = {F(.115), F(.09), F(.1)}, CSP[3] = {F(.062), F(.07), F(.078)};
static void update(void) {
    G.t++; if (G.phase != PH_RIDE) return; G.time++;
    int held = key_held & K_UP ? 0 : key_held & K_DOWN ? 1 : key_held & K_LEFT ? 2 : key_held & K_RIGHT ? 3 : -1; if (held >= 0) G.p.want = held;
    ent_t *P = &G.p;
    step(P, PSP[MG.veh % 3], 0);
    for (int i = 0; i < G.nfood; i++) if (G.food[i].x == P->x && G.food[i].y == P->y && P->pr < 32768) {
        for (int j = i; j < G.nfood - 1; j++) G.food[j] = G.food[j + 1];
        G.nfood--; sfx_coin();
        if (!G.nfood) { arc_banner("¡PEDIDO LISTO!", "LLÉVALO A LA CASA DEL CLIENTE", 90); sfx_power(); }
        break;
    }
    if (G.light && P->x == G.lx && P->y == G.ly) { G.light = 0; G.frozen = 300; sfx_power(); arc_banner("¡SEMÁFORO EN ROJO!", "LOS COCHES SE PARAN", 70); }
    if (G.house && P->x == G.hx && P->y == G.hy && P->pr == 0) {
        if (G.nfood) { if (!G.warned) { G.warned = 1; char b[40], *p = str_cat(b, "QUEDAN "); p = str_num(p, G.nfood); str_cat(p, " PRODUCTOS"); arc_banner("¡FALTA COMIDA!", b, 70); sfx_back(); } }
        else { deliver(); return; }
    } else G.warned = 0;
    if (G.wait > 0) G.wait--; else if (G.frozen > 0) G.frozen--; else
        for (int i = 0; i < G.ncars; i++) step(&G.cars[i], CSP[G.lvl], 1);
    if (G.inv > 0) G.inv--; else {
        s32 px, py; pos(P, &px, &py);
        for (int i = 0; i < G.ncars; i++) {
            s32 cx, cy; pos(&G.cars[i], &cx, &cy);
            if (iabs(cx - px) + iabs(cy - py) < F(.7)) {
                G.lives--; G.hits++; G.inv = 100; sfx_hit();
                if (G.lives <= 0) { arc_lose("¡TE HAN ATROPELLADO!"); return; }
                arc_banner("¡CUIDADO!", "PIERDES UN CASCO", 70); reset(); break;
            }
        }
    }
}

static void draw(void) {
    if (last_fn) { int n = frame_no - last_fn; if (n > MZ_NMAX && n < 10) MZ_NMAX = n; if (G.phase == PH_RIDE) MZ_NH[imin(n, 3)]++; }
    u32 t0 = ticks();
    last_fn = frame_no;
    const int W = 168, OY = 14; u8 K = C(0x1b1212);
    rep_copy(BG, 0, OY); MZ_BT = ticks() - t0;
    if (G.house) {
        int X = G.hx * 8, Y = OY + G.hy * 8;
        rect(X, Y + 3, 8, 5, C(0xffe08a)); rect(X + 1, Y, 6, 3, C(0xd0102c)); rect(X + 3, Y + 5, 2, 3, K);
        if (!G.nfood && (G.t >> 4) & 1) frame_rect(X - 1, Y - 1, 10, 10, C(0x62d26f));
    }
    if (G.light) { int X = G.lx * 8 + 2, Y = OY + G.ly * 8; rect(X, Y, 4, 8, K); rect(X + 1, Y + 1, 2, 2, C(0xff3a3a)); rect(X + 1, Y + 5, 2, 2, C(0x62d26f)); }
    for (int i = 0; i < G.nfood; i++) { food_t *f = &G.food[i]; blit(FOODB[(int)f->k], f->x * 8 + 1, OY + f->y * 8 + 1 + ((G.t >> 3) + f->x) % 2, 0); }
    for (int i = 0; i < G.ncars; i++) {
        ent_t *c = &G.cars[i]; s32 x, y; pos(c, &x, &y);
        int X = (x * 8 + 32768) >> 16, Y = OY + ((y * 8 + 32768) >> 16), f = c->face, h = f == 2 || f == 3;
        int w = h ? 8 : 6, l = h ? 6 : 8, ox = X + (8 - w) / 2, oy = Y + (8 - l) / 2;
        rect(ox, oy, w, l, K); rect(ox + 1, oy + 1, w - 2, l - 2, G.frozen > 0 && (G.t >> 3) & 1 ? C(0x8a94aa) : c->col);
        rect(f == 2 ? ox + 1 : f == 3 ? ox + w - 3 : ox + 1, f == 0 ? oy + 1 : f == 1 ? oy + l - 3 : oy + 1, h ? 2 : w - 2, h ? l - 2 : 2, C(0x2a3550));
        u8 hl = C(0xffe08a);
        if (f == 3) { rect(ox + w - 1, oy + 1, 1, 1, hl); rect(ox + w - 1, oy + l - 2, 1, 1, hl); }
        if (f == 2) { rect(ox, oy + 1, 1, 1, hl); rect(ox, oy + l - 2, 1, 1, hl); }
        if (f == 0) { rect(ox + 1, oy, 1, 1, hl); rect(ox + w - 2, oy, 1, 1, hl); }
        if (f == 1) { rect(ox + 1, oy + l - 1, 1, 1, hl); rect(ox + w - 2, oy + l - 1, 1, 1, hl); }
    }
    {   ent_t *P = &G.p; s32 x, y; pos(P, &x, &y);
        int X = (x * 8 + 32768) >> 16, Y = OY + ((y * 8 + 32768) >> 16), f = P->face, h = f == 2 || f == 3;
        if (!(G.inv > 0 && (G.t >> 2) & 1)) {
            if (h) rect(X, Y + 3, 8, 2, K); else rect(X + 3, Y, 2, 8, K);
            rect(X + 2, Y + 2, 4, 4, K); rect(X + 2, Y + 2, 4, 3, C(0xd0102c)); rect(X + 3, Y + 3, 2, 1, C(0xffffff));
            int hx = f == 2 ? X + 1 : f == 3 ? X + 5 : X + 3, hy = f == 0 ? Y + 1 : f == 1 ? Y + 5 : Y + 3; rect(hx, hy, 2, 2, C(0xffffff));
        }
    }
    /* marcador */
    char b[40], *p;
    rect(0, 0, W, OY, C(0x120b0c));
    p = str_cat(b, "PEDIDO "); p = str_num(p, imin(G.lvl + 1, 3)); str_cat(p, "/3"); arc_text(3, 3, b, C(0xffd23f), 0, 6);
    blit(FOODB[0], 66, 3, 0);
    p = str_num(b, G.total - G.nfood); *p++ = '/'; str_num(p, G.total); arc_text(74, 3, b, C(0xffffff), 0, 6);
    for (int i = 0; i < 3; i++) { u8 c = i < G.lives ? C(0xd0102c) : C(0x4a3a3e); int X = 106 + i * 9;
        rect(X + 1, 3, 5, 1, c); rect(X, 4, 7, 4, c); rect(X + 4, 5, 2, 2, i < G.lives ? C(0xffffff) : C(0x5a4a4e)); rect(X, 8, 7, 1, K); }
    { int s = G.time / 60; p = b; if (s < 10) *p++ = '0'; p = str_num(p, s); str_cat(p, "s"); arc_text(W - 3, 3, b, C(0xffffff), 2, 6); }
    if (G.phase == PH_ORDER || G.phase == PH_DELIV) {
        rect(10, 34, W - 20, 70, C(0x120b0c)); frame_rect(10, 34, W - 20, 70, C(0xffd23f)); frame_rect(11, 35, W - 22, 68, C(0xffd23f));
        if (G.phase == PH_ORDER) {
            p = str_cat(b, "PEDIDO "); p = str_num(p, G.lvl + 1); str_cat(p, "/3"); arc_text(W / 2, 42, b, C(0xffd23f), 1, 8);
            arc_text(W / 2, 58, "RECOGE TODA LA COMIDA", C(0xffffff), 1, 6); arc_text(W / 2, 68, "Y LLÉVALA AL CLIENTE", C(0xffffff), 1, 6);
            arc_text(W / 2, 80, "¡OJO CON LOS COCHES!", C(0xff8a92), 1, 6);
            if ((G.t >> 4) & 1) arc_text(W / 2, 92, "A: EMPEZAR", C(0x62d26f), 1, 6);
        } else {
            arc_text(W / 2, 42, "¡ENTREGADO!", C(0x62d26f), 1, 8);
            p = b; for (int i = 0; i < 3; i++) p = str_cat(p, i < G.last ? "★" : "☆"); arc_text(W / 2, 58, b, C(0xffd23f), 1, 8);
            p = str_num(b, G.time / 60); p = str_cat(p, " s · "); p = str_num(p, G.hits); str_cat(p, G.hits == 1 ? " golpe" : " golpes");
            arc_text(W / 2, 74, b, C(0xffffff), 1, 6);
            if (G.t > G.doneAt + 30 && (G.t >> 4) & 1) arc_text(W / 2, 90, G.lvl < 2 ? "A: SIGUIENTE PEDIDO" : "A: TERMINAR", C(0x62d26f), 1, 6);
        }
    }
    MZ_DT = ticks() - t0;
}

const arc_def_t ARC_DEF5 = {"REPARTO EXPRÉS", 168, 134,
    "FLECHAS: MOVERTE. Recoge toda la comida del pedido y llévala a la casa del cliente. ¡Esquiva los coches! El semáforo los para un rato.",
    init, update, draw, press, prog};
