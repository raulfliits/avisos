/* MODO REPARTIDOR (ARC_GAMES[4] del web): vista trasera en perspectiva de una calle de 4 carriles.
   Pedidos (REP_ORDERS), tráfico, cruces con semáforos, multas, choques y entrega en el carril derecho
   junto a la casa del cliente. Toda la lógica y el dibujo van en punto fijo:
   - mundo (s, x, v…) en Q16 (F(x) = x·65536)
   - escala de proyección y píxeles en Q8
   La calzada se pinta por líneas con tablas por scanline (rep_road, en IWRAM). */
#include "arc_rep.h"

#define F(x) ((s32)((x) * 65536.0))
#define Q8(x) ((s32)((x) * 256.0))

int REP_STARS, REP_NMAX;
/* medidas (para el informe): ciclos de CPU de dibujo por partes [cielo, calzada, objetos, rider, marcador],
   media de dibujo en conducción, y nº de frames por dibujo (REP_NH[n] = veces que n pasos lógicos) */
u32 REP_TK[5], REP_DT, REP_DT_AVG; int REP_NH[4];

/* ---------------------------------------------------------------- datos del web */
static const u8 REP_LOOK[4] = {0, 4, 3, 1};   /* REP_RIDERS: AAMIR, SAIRA, NIL, AYESHA */
typedef struct { const char *rest, *food, *addr; s16 dist, icon, target; } order_t;
static const order_t REP_ORDERS[3] = {
    {"PIZZERIA NAPOLI", "2 PIZZAS", "C/ MALLORCA 140", 520, 0, 55},
    {"SUSHI GRÀCIA", "SUSHI PARA 4", "AV. DIAGONAL 520", 680, 3, 70},
    {"BURGER SANTS", "3 HAMBURGUESAS", "GRAN VIA 610", 840, 1, 85}};
#define RW 2
#define CH 1.4
#define FOC 90
#define HZ 64
#define ZP F(1.6)
static const s32 LANES[4] = {F(-1.5), F(-.5), F(.5), F(1.5)};
#define T_CAR 0
#define T_TAXI 1
#define T_VAN 2
#define T_BUS 3
#define T_MOTO 4
static const s32 VW[5] = {F(.8), F(.8), F(.9), F(1), F(.3)};
static const s32 VLEN[5] = {F(1.8), F(1.8), F(2.2), F(4.5), F(.9)};
static const s16 VW8[5] = {Q8(.8), Q8(.8), Q8(.9), Q8(1), Q8(.3)};
static const s16 VH8[5] = {Q8(.62), Q8(.62), Q8(.95), Q8(1.45), Q8(.8)};
static u8 CAR_COLS[7];
static void init_cols(void) {
    CAR_COLS[0] = C(0x2f6fe0); CAR_COLS[1] = C(0xe8e8e8); CAR_COLS[2] = C(0x62d26f); CAR_COLS[3] = C(0xc45a83);
    CAR_COLS[4] = C(0xff8a5c); CAR_COLS[5] = C(0x8a94aa); CAR_COLS[6] = C(0xffd23f);
}

/* ---------------------------------------------------------------- estado (= G del web) */
typedef struct { s32 s, x, v, base, len; s8 lane, type, mv, braking; s16 blink; u8 col; } car_t;
typedef struct { s32 s, x, dx; u8 col; } cross_t;
typedef struct { s32 s; s16 off; u8 passed; } inter_t;
#define PH_ORDER 0
#define PH_RIDE 1
#define PH_DELIV 2
/* global (no static) para que el bot de pruebas lo lea por nombre */
struct {
    s32 t, order, phase, done, estado, multas, choques, stars, s, v, lane, x, sky, lean;
    s32 inv, brake, horn, dest, time, deliverT, braking, lastStars, lastTime;
    s32 ntr, ncr, nin;
    car_t tr[12]; cross_t cr[16]; inter_t in[4];
} REPG;
#define G REPG

static int light(const inter_t *I, int t) { int k = (t + I->off) % 420; return k < 210 ? 0 : k < 260 ? 1 : 2; } /* 0 G, 1 Y, 2 R */
/* repCurve(s) = sin(s/90)*.55 + sin(s/37)*.2  → Q12 */
static s32 curve(s32 s) {
    s32 s8 = s >> 8;
    return (isin((s8 * 464) >> 16) * 141 + isin((s8 * 1128) >> 16) * 51) >> 8;
}
static int rnd16(void) { return rnd() & 0xFFFF; }
static int sgn(s32 v) { return v > 0 ? 1 : v < 0 ? -1 : 0; }
static s32 approach(s32 v, s32 to, s32 step) { s32 d = to - v; if (d > step) d = step; if (d < -step) d = -step; return v + d; }

static void prog(char *o) {
    char *p = str_cat(o, "PEDIDOS "); p = str_num(p, MG.c1); p = str_cat(p, "/3 · ESTADO ");
    p = str_num(p, imax(0, G.estado)); str_cat(p, "%");
}

/* ---------------------------------------------------------------- tablas por scanline */
static s32 ZT[160];   /* z (Q16) de cada línea de la calzada */
static s32 ST[160];   /* escala (Q8 px por unidad) */
static u8 tab_ok;
static u32 last_fn;
static u32 RECIP[1024] EWRAM_BSS;   /* (90<<24)/m para m en [1024,2048): escala = RECIP >> e */
static bmp_t *PB;       /* 32x64: rider por detrás, origen en (16,62) */
static bmp_t *PCACHE[12];  /* rider ya dibujado por (bote, pierna, freno) */
static bmp_t *HUDB, *CARDB; static int hud_key, card_key;  /* barra de marcador cacheada */
static bmp_t *SKYB;        /* (2W) x 25: filas HZ-24..HZ con el skyline repetido */
static void build_sky(void);
static void tables(void) {
    if (tab_ok) return; tab_ok = 1;
    for (int m = 0; m < 1024; m++) RECIP[m] = (90u << 24) / (u32)(m + 1024) + 0;
    for (int y = HZ + 1; y < 160; y++) {
        ZT[y] = (s32)(CH * FOC / (double)(y - HZ) * 65536.0);
        ST[y] = (s32)((double)(y - HZ) / CH * 256.0);
    }
}

static void init(void) {
    init_cols(); tables(); last_fn = 0;
    if (MG.t == 0) { PB = 0; SKYB = 0; HUDB = 0; CARDB = 0; for (int i = 0; i < 12; i++) PCACHE[i] = 0; }  /* bitmaps del pool: nuevos en cada partida */
    build_sky(); hud_key = -1; card_key = -1;
    memset(&G, 0, sizeof G);
    G.estado = 100; G.lane = 1; G.x = LANES[1]; G.phase = PH_ORDER;
    REP_STARS = 0;
}
static void start_order(void) {
    const order_t *O = &REP_ORDERS[G.order];
    G.phase = PH_RIDE; G.estado = 100; G.s = 0; G.v = 0; G.lane = 1; G.x = LANES[1]; G.ntr = G.ncr = 0;
    G.inv = G.brake = G.horn = 0; G.dest = O->dist << 16; G.time = 0; G.deliverT = 0;
    G.nin = (O->dist - 100) / 150;
    for (int i = 0; i < G.nin; i++) { G.in[i].s = (110 + i * 150) << 16; G.in[i].off = (i * 130 + G.order * 70) % 420; G.in[i].passed = 0; }
}
static void deliver(void) {
    const order_t *O = &REP_ORDERS[G.order];
    G.phase = PH_DELIV; G.done++;
    G.lastStars = G.estado >= 80 && G.time <= O->target * 60 ? 3 : G.estado >= 50 ? 2 : 1;
    G.stars += G.lastStars; REP_STARS = G.stars; G.lastTime = (G.time + 30) / 60;
    sfx_win(); MG.c1 = G.done;
}
static void crash(const char *msg) {
    G.inv = 70; G.v = 0; G.estado -= 20; G.choques++; sfx_hit(); arc_banner(msg, "PEDIDO -20%", 60);
}
static void press(int b) {
    if (G.phase == PH_ORDER) { if (b == B_A) { sfx_open(); start_order(); } return; }
    if (G.phase == PH_DELIV) { if (b == B_A) { G.order++; if (G.order >= 3) arc_win(); else { G.phase = PH_ORDER; sfx_select(); } } return; }
    if (b == B_LEFT && G.lane > 0) { G.lane--; tone(500, 30, 0, W_SQUARE, 60); }
    if (b == B_RIGHT && G.lane < 3) { G.lane++; tone(500, 30, 0, W_SQUARE, 60); }
    if (b == B_B) G.brake = 18;
    if (b == B_A && iabs(G.dest - G.s - ZP) < F(4) && G.lane == 3 && G.v < F(.2)) { G.v = 0; deliver(); return; }
    if (b == B_A) {
        G.horn = 14; tone(440, 120, 0, W_SQUARE, 50); tone(554, 120, 0, W_SQUARE, 50);
        s32 ps = G.s + ZP;
        for (int i = 0; i < G.ntr; i++) {
            car_t *c = &G.tr[i];
            if (c->lane == G.lane && c->s > ps && c->s - ps < F(14) && c->type != T_BUS) {
                if (c->mv < 0) {
                    int to = c->lane + (c->lane < 3 ? 1 : -1), ok = 1;
                    for (int j = 0; j < G.ntr; j++) if (G.tr[j].lane == to && iabs(G.tr[j].s - c->s) < F(5)) ok = 0;
                    if (ok) c->mv = to;
                }
                break;
            }
        }
    }
}

static const s32 PMAX[3] = {F(.36), F(.27), F(.31)}, PACC[3] = {F(.006), F(.0045), F(.005)}, PTURN[3] = {F(.08), F(.11), F(.095)};
static void update(void) {
    G.t++; if (G.phase != PH_RIDE) return;
    int vh = MG.veh;
    int braking = (key_held & K_DOWN) || G.brake > 0; if (G.brake > 0) G.brake--; if (G.horn > 0) G.horn--; if (G.inv > 0) G.inv--;
    int gas = key_held & K_UP; G.braking = braking;
    if (braking) G.v = imax(0, G.v - F(.02)); else if (gas) G.v = imin(PMAX[vh], G.v + PACC[vh]); else G.v = imax(0, G.v - F(.004));
    s32 tx = LANES[G.lane]; G.x = approach(G.x, tx, PTURN[vh]); G.lean = tx - G.x;
    s32 prev = G.s + ZP; G.s += G.v; G.time++; s32 ps = G.s + ZP;
    G.sky += ((curve(G.s) * (G.v >> 4)) >> 8) * 6;   /* Q12*Q12>>8 = Q16 */
    /* semáforos */
    for (int i = 0; i < G.nin; i++) {
        inter_t *I = &G.in[i]; int L = light(I, G.t);
        if (!I->passed && prev < I->s && ps >= I->s) { I->passed = 1; if (L == 2) { G.estado -= 15; G.multas++; sfx_hit(); arc_banner("¡SEMÁFORO EN ROJO!", "MULTA · PEDIDO -15%", 70); } }
        if (L == 2 && I->s - G.s < F(70) && I->s - G.s > F(-4) && G.t % 40 == 0 && G.ncr < 16) {
            int fromL = rnd16() < 32768; cross_t *c = &G.cr[G.ncr++];
            c->s = I->s + F(fromL ? 3 : 7); c->x = fromL ? F(-9) : F(9); c->dx = fromL ? F(.16) : F(-.16); c->col = CAR_COLS[(G.t >> 3) % 7];
        }
    }
    {   int k = 0;
        for (int i = 0; i < G.ncr; i++) {
            cross_t *c = &G.cr[i]; c->x += c->dx;
            if (G.inv <= 0 && iabs(c->s - ps) < F(1.1) && iabs(c->x - G.x) < F(1)) crash("¡CUIDADO EN EL CRUCE!");
            if (iabs(c->x) < F(10)) G.cr[k++] = *c;
        }
        G.ncr = k; }
    /* tráfico */
    for (int k = 0; k < 3 && G.ntr < 9 && G.s < G.dest - F(30); k++) {
        int lane = rnd16() >> 14; s32 s = G.s + F(45) + (s32)(((u32)rnd16() * 70u)); int clash = 0;
        for (int j = 0; j < G.ntr; j++) if (G.tr[j].lane == lane && iabs(G.tr[j].s - s) < F(8)) clash = 1;
        if (clash) continue;
        int r = rnd16(), type = r < F(.1) ? T_BUS : r < F(.3) ? T_TAXI : r < F(.45) ? T_VAN : r < F(.62) ? T_MOTO : T_CAR;
        car_t *c = &G.tr[G.ntr++];
        c->lane = lane; c->x = LANES[lane]; c->s = s; c->type = type;
        c->v = type == T_BUS ? F(.1) : type == T_MOTO ? F(.21) + ((rnd16() * F(.06)) >> 16) : F(.12) + ((rnd16() * F(.11)) >> 16);
        c->col = CAR_COLS[rndi(7)]; c->len = VLEN[type]; c->mv = -1; c->blink = 0; c->base = 0; c->braking = 0;
    }
    for (int i = 0; i < G.ntr; i++) {
        car_t *c = &G.tr[i];
        if (!c->base) c->base = c->v;
        s32 want = c->base;
        car_t *ah = 0;
        for (int j = 0; j < G.ntr; j++) { car_t *o = &G.tr[j]; if (o != c && o->lane == c->lane && o->s > c->s && (!ah || o->s < ah->s)) ah = o; }
        if (ah && ah->s - c->s < ah->len / 2 + c->len / 2 + F(1.5)) want = imin(want, ah->v - F(.01));
        for (int j = 0; j < G.nin; j++) {
            s32 d = G.in[j].s - F(.5) - (c->s + c->len / 2);
            if (d > F(-.3) && d < F(12) && light(&G.in[j], G.t) != 0) want = imin(want, imax(0, d / 30));
        }
        c->braking = want < c->v - F(.005);
        c->v = approach(c->v, want, F(.01)); if (c->v < 0) c->v = 0; c->s += c->v;
        if (c->mv < 0 && c->type != T_BUS && rnd16() < 98) {
            int to = c->lane + (rnd16() < 32768 ? -1 : 1);
            if (to >= 0 && to < 4) { int ok = 1; for (int j = 0; j < G.ntr; j++) if (j != i && G.tr[j].lane == to && iabs(G.tr[j].s - c->s) < F(6)) ok = 0; if (ok) c->mv = to; }
        }
        if (c->mv >= 0) { c->blink++; if (c->blink > 40) { s32 t2 = LANES[(int)c->mv]; c->x = approach(c->x, t2, F(.03)); if (iabs(c->x - t2) < F(.01)) { c->lane = c->mv; c->mv = -1; c->blink = 0; } } }
        if (G.inv <= 0 && iabs(c->x - G.x) < VW[(int)c->type] / 2 + F(.25) && iabs(c->s - ps) < c->len / 2 + F(.5)) crash(c->type == T_BUS ? "¡CHOQUE CON EL BUS!" : "¡CHOQUE!");
    }
    {   int k = 0; for (int i = 0; i < G.ntr; i++) { car_t *c = &G.tr[i]; if (c->s > G.s - F(2) && c->s < G.s + F(160)) G.tr[k++] = *c; } G.ntr = k; }
    /* entrega */
    s32 d = G.dest - ps;
    if (iabs(d) < F(3) && G.lane == 3 && G.v < F(.04) && iabs(G.x - LANES[3]) < F(.05)) { if (++G.deliverT > 20) { deliver(); return; } } else G.deliverT = 0;
    if (d < F(-8)) { G.dest = ps + F(70); G.time += 300; arc_banner("¡TE LO HAS PASADO!", "VUELTA A LA MANZANA · +5 s", 80); sfx_back(); }
    if (G.estado <= 0) arc_lose("¡EL PEDIDO HA LLEGADO DESTROZADO!");
}

/* ---------------------------------------------------------------- utilidades de dibujo */
extern bmp_t T;
#define REG_DMA3SAD (*(volatile u32 *)0x040000D4)
#define REG_DMA3DAD (*(volatile u32 *)0x040000D8)
#define REG_DMA3CNT (*(volatile u32 *)0x040000DC)
/* rect rápido para la VRAM (modo 4): recorta a la caja KX0..KX1 x KY0..KY1 (coordenadas del lienzo) */
static s16 KX0, KY0, KX1 = 240, KY1 = 160;
static void kbox(int x0, int y0, int x1, int y1) { KX0 = x0; KY0 = y0; KX1 = x1; KY1 = y1; }
IWRAM_CODE void frect(int x, int y, int w, int h, u8 c) {
    int x1 = x + w, y1 = y + h;
    if (x < KX0) x = KX0; if (y < KY0) y = KY0; if (x1 > KX1) x1 = KX1; if (y1 > KY1) y1 = KY1;
    if (x >= x1 || y >= y1) return;
    if (!T.vram) { rect(x, y, x1 - x, y1 - y, c); return; }
    x += OX; x1 += OX; y += OY; y1 += OY;
    const int tw = T.w; u8 *row = T.px + y * tw;
    const u32 c2 = c | (c << 8), c4 = c2 | (c2 << 16);
    int lo = x & 1, a = x + lo, hi = x1 & 1, b = x1 - hi;
    if (a > b) { /* un solo píxel impar */
        for (; y < y1; y++, row += tw) { volatile u16 *q = (u16 *)(row + x - 1); *q = (*q & 0x00FF) | (c << 8); }
        return;
    }
    int n = (b - a) >> 1, pre = (a & 2) && n ? 1 : 0, n4 = (n - pre) >> 1, post = (n - pre) & 1;
    for (; y < y1; y++, row += tw) {
        if (lo) { volatile u16 *q = (u16 *)(row + x - 1); *q = (*q & 0x00FF) | (c << 8); }
        u16 *p = (u16 *)(row + a);
        if (pre) *p++ = c2;
        u32 *p4 = (u32 *)p; for (int k = n4; k; k--) *p4++ = c4;
        if (post) *(u16 *)p4 = c2;
        if (hi) { volatile u16 *q = (u16 *)(row + b); *q = (*q & 0xFF00) | c; }
    }
}
/* tramo horizontal directo en la VRAM (coordenadas del lienzo, recortado a la caja K) */
IWRAM_CODE static void hspan(u8 *row, int a, int e, u8 c, u32 c4) {
    if (a < KX0) a = KX0; if (e > KX1) e = KX1; if (a >= e) return;
    a += OX; e += OX;
    if (a & 1) { volatile u16 *p = (u16 *)(row + a - 1); *p = (*p & 0x00FF) | (c << 8); a++; }
    if ((a & 2) && a + 2 <= e) { *(u16 *)(row + a) = (u16)c4; a += 2; }
    u32 *p4 = (u32 *)(row + a); for (; a + 4 <= e; a += 4) *p4++ = c4;
    if (a + 2 <= e) { *(u16 *)(row + a) = (u16)c4; a += 2; }
    if (a < e) { volatile u16 *pp = (u16 *)(row + a); *pp = (*pp & 0xFF00) | c; }
}
IWRAM_CODE void rep_circle(int cx, int cy, int r, u8 c) {
    if (r <= 0) { frect(cx, cy, 1, 1, c); return; }
    if (cx + r < KX0 || cx - r >= KX1 || cy + r < KY0 || cy - r >= KY1) return;
    if (!T.vram) { for (int y = -r; y <= r; y++) { int hw = 0; while ((hw + 1) * (hw + 1) <= r * r - y * y + r) hw++; rect(cx - hw, cy + y, 2 * hw + 1, 1, c); } return; }
    const u32 c4 = c * 0x01010101u;
    /* semiancho por fila (mismo criterio que circle() del motor: hw = isqrt(r² - y² + r)), incremental */
    int hw = r, rr = r * r + r;
    for (int y = 0; y <= r; y++) {
        int lim = rr - y * y; while (hw > 0 && hw * hw > lim) hw--;
        int y0 = cy - y, y1 = cy + y;
        if (y0 >= KY0 && y0 < KY1) hspan(T.px + (y0 + OY) * T.w, cx - hw, cx + hw + 1, c, c4);
        if (y && y1 >= KY0 && y1 < KY1) hspan(T.px + (y1 + OY) * T.w, cx - hw, cx + hw + 1, c, c4);
    }
}
/* copia opaca: x debe dar destino par (origen par); filas enteras de b */
IWRAM_CODE void rep_copy(const bmp_t *b, int x, int y) {
    int dx = x + OX, dy = y + OY;
    for (int j = 0; j < b->h; j++) {
        int ty = dy + j; if (ty < 0 || ty >= T.h) continue;
        const u8 *s = b->px + j * b->w; u8 *d = T.px + ty * T.w + dx;
        if (!(((u32)d | (u32)s) & 3) && !(b->w & 3)) { REG_DMA3SAD = (u32)s; REG_DMA3DAD = (u32)d; REG_DMA3CNT = 0x84000000u | (b->w >> 2); }
        else if (!(((u32)d | (u32)s) & 1) && !(b->w & 1)) { REG_DMA3SAD = (u32)s; REG_DMA3DAD = (u32)d; REG_DMA3CNT = 0x80000000u | (b->w >> 1); }
        else blit_part(b, 0, j, b->w, 1, x, y + j, 0);
    }
}
/* drawFood del web con escala s2/2 */
void rep_food(int k, int x, int y, int s2) {
#define FR(a, b, w, h, c) rect(x + (a) * s2 / 2, y + (b) * s2 / 2, ((a) + (w)) * s2 / 2 - (a) * s2 / 2, ((b) + (h)) * s2 / 2 - (b) * s2 / 2, c)
    if (k == 0) { FR(0, 0, 9, 2, C(0xc9971a)); FR(1, 2, 7, 2, C(0xffd23f)); FR(2, 4, 5, 2, C(0xffd23f)); FR(3, 6, 3, 2, C(0xffd23f)); FR(2, 2, 2, 1, C(0xd62839)); FR(5, 4, 1, 1, C(0xd62839)); }
    else if (k == 1) { FR(1, 0, 7, 3, C(0xc9971a)); FR(0, 3, 9, 1, C(0x62d26f)); FR(0, 4, 9, 2, C(0x7d4f2f)); FR(1, 6, 7, 2, C(0xc9971a)); }
    else if (k == 2) { FR(2, 0, 5, 1, C(0xffffff)); FR(1, 1, 7, 7, C(0xd62839)); FR(2, 3, 5, 2, C(0xffffff)); FR(4, -2, 1, 3, C(0xffd23f)); }
    else { FR(0, 2, 9, 5, C(0xffffff)); FR(0, 4, 9, 2, C(0x1b1212)); FR(2, 1, 5, 2, C(0xff8a5c)); }
#undef FR
}

/* ---------------------------------------------------------------- calzada por líneas */
#define W 224
#define H 160
static u8 LB[W] __attribute__((aligned(4)));
__attribute__((optimize("no-tree-loop-distribute-patterns"))) static inline void hf(int x, int w, u8 c) {
    if (x < 0) { w += x; x = 0; } if (x + w > W) w = W - x; if (w <= 0) return;
    u8 *p = LB + x, *e = p + w;
    while (p < e && ((u32)p & 3)) *p++ = c;
    u32 c4 = c * 0x01010101u; while (p + 4 <= e) { *(u32 *)p = c4; p += 4; }
    while (p < e) *p++ = c;
}
static inline int rnd8(s32 v) { return (v + 128) >> 8; }
static inline int wq(s32 v) { int r = rnd8(v); return r < 1 ? 1 : r; }
__attribute__((optimize("no-tree-loop-distribute-patterns"))) IWRAM_CODE static void rep_road(s32 cam8, s32 cv) {
    u8 cIn = C(0x4a4a52), cG0 = C(0xbdb39f), cG1 = C(0xc9c0ae), cR0 = C(0x41414a), cR1 = C(0x45454e), cRed = C(0xd62839), cW = C(0xffffff);
    u8 cLane = C(0xe8e8e8), cZeb = C(0xf2f2f2), cDest = C(0x52845d);
    int showDest = G.phase != PH_ORDER;
    for (int y = HZ + 1; y < H; y++) {
        s32 z = ZT[y], sc = ST[y], sr = G.s + z;
        s32 shift8 = (((cv * (z >> 8)) >> 12) * 410) >> 8;
        s32 cx = (W / 2) * 256 - ((cam8 * sc) >> 8) + shift8;   /* Q8 */
        s32 rw = RW * sc;
        int seg = (sr >> 17) & 1, inX = 0, zebra = 0;
        for (int i = 0; i < G.nin; i++) { s32 is = G.in[i].s; if (sr >= is && sr < is + F(10)) inX = 1; if (sr >= is - F(2) && sr < is) zebra = 1; }
        { static volatile u32 fill; fill = (inX ? cIn : seg ? cG1 : cG0) * 0x01010101u;
          REG_DMA3SAD = (u32)&fill; REG_DMA3DAD = (u32)LB; REG_DMA3CNT = 0x85000000u | (W / 4); }
        int rl = rnd8(cx - rw);
        hf(rl, rnd8(rw * 2), inX ? cIn : seg ? cR1 : cR0);
        if (!inX) {
            int ew = wq((sc * 31) >> 8);
            hf(rnd8(cx - rw - ((sc * 31) >> 8)), ew, seg ? cRed : cW); hf(rnd8(cx + rw), ew, seg ? cRed : cW);
            if (seg && !zebra) { int lw = wq((sc * 20) >> 8); s32 o = (sc * 10) >> 8; hf(rnd8(cx - sc - o), lw, cLane); hf(rnd8(cx - o), lw, cLane); hf(rnd8(cx + sc - o), lw, cLane); }
        }
        if (zebra) { int zw = wq((sc * 51) >> 8); for (int k = -2 * 256 + 26; k < 2 * 256; k += 92) hf(rnd8(cx + ((k * sc) >> 8)), zw, cZeb); }
        if (showDest && sr >= G.dest - F(1) && sr < G.dest + F(1)) hf(rnd8(cx + sc), rnd8(sc), cDest);
        /* a la pantalla (origin del lienzo) */
        u8 *d = T.px + (y + OY) * T.w + OX;
        if (!((u32)d & 3)) { REG_DMA3SAD = (u32)LB; REG_DMA3DAD = (u32)d; REG_DMA3CNT = 0x84000000u | (W / 4); }
        else { u16 *d2 = (u16 *)d; const u16 *s2 = (const u16 *)LB; for (int i = 0; i < W / 2; i++) d2[i] = s2[i]; }
    }
}

/* ---------------------------------------------------------------- objetos de la calle */
static s32 SC;   /* escala actual Q8 */
#define M(k) ((Q8(k) * SC) >> 8)
IWRAM_CODE static void R8(s32 x, s32 y, s32 w, s32 h, u8 c) { frect(rnd8(x), rnd8(y), wq(w), wq(h), c); }
static void D8(s32 x, s32 y, s32 w, s32 h) { rect_dark(rnd8(x), rnd8(y), wq(w), wq(h)); }
static void L8(s32 x, s32 y, s32 w, s32 h) { rect_light(rnd8(x), rnd8(y), wq(w), wq(h)); }
static void C8(s32 x, s32 y, s32 r, u8 c) { rep_circle(rnd8(x), rnd8(y), rnd8(r), c); }
static s32 mulq(s32 a8, int k256) { return (a8 * k256) >> 8; }

static void rep_rear(s32 x, s32 y, int type, u8 col, int brk, int blink, int t) {
    u8 K = C(0x1b1212);
    s32 w = (VW8[type] * SC) >> 8, h = (VH8[type] * SC) >> 8;
    D8(x - w / 2, y - M(.06), w, M(.08));
    if (type == T_MOTO) {
        R8(x - M(.06), y - M(.35), M(.12), M(.35), K); R8(x - M(.14), y - M(.75), M(.28), M(.42), col);
        R8(x - M(.1), y - M(.95), M(.2), M(.22), C(0x333333)); R8(x - M(.05), y - M(.4), M(.1), M(.06), brk ? C(0xff2a2a) : C(0xa01010)); return;
    }
    u8 bc = type == T_TAXI ? C(0x1b1b1b) : type == T_BUS ? C(0xd0102c) : type == T_VAN ? C(0xf0f0f0) : col;
    R8(x - w / 2 + M(.05), y - M(.18), M(.16), M(.18), K); R8(x + w / 2 - M(.21), y - M(.18), M(.16), M(.18), K);
    R8(x - w / 2, y - h, w, h - M(.12), bc);
    if (type == T_TAXI) {
        R8(x - w / 2, y - mulq(h, 141), mulq(w, 46), mulq(h, 102), C(0xffd23f)); R8(x + w / 2 - mulq(w, 46), y - mulq(h, 141), mulq(w, 46), mulq(h, 102), C(0xffd23f));
        R8(x - M(.12), y - h - M(.08), M(.24), M(.08), C(0x62d26f));
    }
    R8(x - w / 2 + M(.08), y - h + M(.06), w - M(.16), mulq(h, type == T_BUS ? 64 : 90), type == T_BUS ? C(0x2a2a33) : C(0x3a4a5c));
    L8(x - w / 2 + M(.1), y - h + M(.08), mulq(w, 77), M(.04));
    if (type == T_BUS) R8(x - w / 2, y - mulq(h, 141), w, M(.05), C(0xffd23f));
    u8 lc = brk ? C(0xff2a2a) : C(0xa01010);
    R8(x - w / 2 + M(.04), y - M(.38), M(.16), M(.1), lc); R8(x + w / 2 - M(.2), y - M(.38), M(.16), M(.1), lc);
    if (brk && SC > 8 * 256) {
        rect_dither(rnd8(x - w / 2 - M(.05)), rnd8(y - M(.45)), rnd8(M(.3)), rnd8(M(.24)), C(0xff3c3c));
        rect_dither(rnd8(x + w / 2 - M(.25)), rnd8(y - M(.45)), rnd8(M(.3)), rnd8(M(.24)), C(0xff3c3c));
    }
    if (blink && (t >> 3) & 1) R8(blink > 0 ? x + w / 2 - M(.12) : x - w / 2 + M(.02), y - M(.5), M(.1), M(.08), C(0xffb000));
    R8(x - M(.14), y - M(.3), M(.28), M(.09), C(0xf2f2f2));
}
static void rep_side(s32 x, s32 y, u8 col, int right) {
    s32 w = M(1.9), h = M(.62);
    R8(x - w / 2, y - h, w, mulq(h, 192), col); R8(x - mulq(w, 77), y - h - M(.25), mulq(w, 141), M(.28), col);
    R8(x - mulq(w, 67), y - h - M(.2), mulq(w, 56), M(.18), C(0x9fd8ff)); R8(x, y - h - M(.2), mulq(w, 51), M(.18), C(0x9fd8ff));
    C8(x - mulq(w, 82), y - M(.12), M(.14), C(0x1b1212)); C8(x + mulq(w, 82), y - M(.12), M(.14), C(0x1b1212));
    R8(right ? x + w / 2 - M(.08) : x - w / 2, y - mulq(h, 154), M(.08), M(.08), C(0xfff6b0));
}
static void sblit(const bmp_t *b, int x, int y, int w, int h) {   /* drawImage escalado (recorta al lienzo) */
    if (!b || w <= 0 || h <= 0) return;
    for (int j = 0; j < h; j++) {
        int yy = y + j; if (yy < 0 || yy >= H) continue;
        const u8 *src = b->px + (j * b->h / h) * b->w;
        int i0 = x < 0 ? -x : 0, i1 = x + w > W ? W - x : w;
        for (int i = i0; i < i1; i++) { u8 c = src[i * b->w / w]; if (c) { int a = i, n = 1; while (a + n < i1 && src[(a + n) * b->w / w] == c) n++; rect(x + a, yy, n, 1, c); i += n - 1; } }
    }
}
static void rep_house(s32 x, s32 y) {
    s32 w = M(3), h = M(2.6), hx = x - w / 2, hy = y - h;
    R8(hx, hy, w, h, C(0x62d26f)); R8(hx, hy, w, M(.25), C(0x2f7d3a));
    R8(hx + M(.3), hy + M(.6), M(.7), M(.6), C(0x9fd8ff)); R8(hx + M(1.9), hy + M(.6), M(.7), M(.6), C(0x9fd8ff)); R8(hx + M(1.1), hy + M(1.4), M(.7), M(1.2), C(0x7a4a2a));
    if (SC > 2 * 256) sblit(person_bmp(2, 0, 0), rnd8(x - M(1.6)), rnd8(y - M(1.6)), rnd8(M(.95)), rnd8(M(1.6)));
    /* marcador rojo: semicírculo + punta */
    int cx = rnd8(x), cy = rnd8(hy - M(.9)), r = rnd8(M(.45)), tip = rnd8(hy - M(.1));
    for (int dy = 1; dy <= r; dy++) { int hw = 0; while ((hw + 1) * (hw + 1) + dy * dy <= r * r) hw++; rect(cx - hw, cy - dy, hw * 2 + 1, 1, C(0xd62839)); }
    for (int yy = cy; yy < tip; yy++) { int hw = r * (tip - yy) / imax(1, tip - cy); rect(cx - hw, yy, hw * 2 + 1, 1, C(0xd62839)); }
    C8(x, hy - M(.95), M(.15), C(0xffffff));
    if (SC > 4 * 256) arc_text(rnd8(x), rnd8(hy - M(2)), "CLIENTE", C(0x62d26f), 1, 8);
}
static const u8 *BLD_COLS(int i) {
    static u8 c[5]; c[0] = C(0xe8d2a8); c[1] = C(0xd9b98c); c[2] = C(0xf0e0c0); c[3] = C(0xd0a878); c[4] = C(0xe0c8b0); return &c[i];
}
/* edificio; (ox0, ox1, oy0) = rectángulo del siguiente edificio más cercano del mismo lado (lo tapa hasta abajo):
   la fachada se pinta en L (sin lo tapado) y las ventanas totalmente tapadas no se pintan */
static s16 OC0, OC1, OCY, OCS;   /* oclusor actual; OCS = lado (-1 izq, 1 dcha, 0 ninguno) */
IWRAM_CODE static void RB(s32 x, s32 y, s32 w, s32 h, u8 c) {
    int a = rnd8(x), b = rnd8(y), ww = wq(w), hh = wq(h);
    if (OCS && a >= OC0 && a + ww <= OC1 && b >= OCY) return;
    frect(a, b, ww, hh, c);
}
/* 3 rectángulos iguales separados dx (Q8): una fila de ventanas de una sola pasada */
IWRAM_CODE static void RB3(s32 x, s32 dx, s32 y, s32 w, s32 h, u8 c) {
    int b = rnd8(y), hh = wq(h), ww = wq(w), xs[3], n = 0;
    for (int q = 0; q < 3; q++) { int a = rnd8(x + q * dx); if (OCS && a >= OC0 && a + ww <= OC1 && b >= OCY) continue; xs[n++] = a; }
    if (!n) return;
    int y0 = imax(b, KY0), y1 = imin(b + hh, KY1); if (y0 >= y1) return;
    if (!T.vram) { for (int q = 0; q < n; q++) frect(xs[q], b, ww, hh, c); return; }
    int x0s[3], x1s[3], m = 0;
    for (int q = 0; q < n; q++) { int a = imax(xs[q], KX0), e = imin(xs[q] + ww, KX1); if (a < e) { x0s[m] = a + OX; x1s[m] = e + OX; m++; } }
    const u16 c2 = c | (c << 8); const u32 c4 = c2 | ((u32)c2 << 16);
    u8 *row = T.px + (y0 + OY) * T.w;
    for (int yy = y0; yy < y1; yy++, row += T.w)
        for (int q = 0; q < m; q++) {
            int a = x0s[q], e = x1s[q];
            if (a & 1) { volatile u16 *p = (u16 *)(row + a - 1); *p = (*p & 0x00FF) | (c << 8); a++; }
            if ((a & 2) && a + 2 <= e) { *(u16 *)(row + a) = c2; a += 2; }
            u32 *p4 = (u32 *)(row + a); for (; a + 4 <= e; a += 4) *p4++ = c4;
            if (a + 2 <= e) { *(u16 *)(row + a) = c2; a += 2; }
            if (a < e) { volatile u16 *pp = (u16 *)(row + a); *pp = (*pp & 0xFF00) | c; }
        }
}
IWRAM_CODE static void rep_bld(s32 x, s32 y, int idx) {
    s32 w = M(4.2), h = M(3.6), bx = x - w / 2, by = y - h;
    u8 bc = *BLD_COLS(idx % 5);
    if (!OCS) { R8(bx, by, w, h, bc); }
    else {
        int a = rnd8(bx), b = rnd8(by), ww = wq(w), hh = wq(h);
        if (OCS < 0) { kbox(OC1, 0, W, H); frect(a, b, ww, hh, bc); kbox(0, 0, OC1, OCY); frect(a, b, ww, hh, bc); }
        else { kbox(0, 0, OC0, H); frect(a, b, ww, hh, bc); kbox(OC0, 0, W, OCY); frect(a, b, ww, hh, bc); }
        kbox(0, 0, W, H);
    }
    RB(bx, by, w, M(.18), C(0xa8865c));
    if (SC > 3 * 256) {
        s32 m45 = M(.45), m125 = M(1.25), m8 = M(.8), m55 = M(.55), m35 = M(.35), m9 = M(.9), m75 = M(.75), m06 = M(.06);
        for (int r = 0; r < 4; r++) {
            RB3(bx + m45, m125, by + m45 + r * m8, m55, m45, C(0x5a4030));
            RB3(bx + m35, m125, by + m9 + r * m8, m75, m06, C(0x2a2a2a));
        }
        RB(bx + M(1.7), y - M(.9), M(.8), M(.9), C(0x7a4a2a));
    }
}
static void rep_tree(s32 x, s32 y) {
    R8(x - M(.05), y - M(1.1), M(.1), M(1.1), C(0x5a3a24));
    C8(x, y - M(1.4), M(.55), C(0x2f7d3a)); C8(x - M(.15), y - M(1.55), M(.22), C(0x62b85c));
}
static void rep_light(s32 x, s32 y, int L) {
    R8(x - M(.04), y - M(2.4), M(.08), M(2.4), C(0x333333)); R8(x - M(.18), y - M(3), M(.36), M(.9), C(0x1b1212));
    static const s32 off[3] = {Q8(2.85), Q8(2.55), Q8(2.25)};
    for (int i = 0; i < 3; i++) {
        int on = (i == 0 && L == 2) || (i == 1 && L == 1) || (i == 2 && L == 0);
        u8 c = on ? (i == 0 ? C(0xff2a2a) : i == 1 ? C(0xffd23f) : C(0x62d26f)) : C(0x3a3a3a);
        C8(x, y - ((off[i] * SC) >> 8), M(.11), c);
        if (on && SC > 6 * 256) L8(x - M(.04), y - (((off[i] + Q8(.05)) * SC) >> 8), M(.05), M(.05));
    }
}

/* ---------------------------------------------------------------- rider visto por detrás (repPlayer) */
static void dome(int r2, int hy, u8 c) {   /* r2 = radio*2 (17, 15, 13): cúpula + 4 px rectos debajo */
    int n = (r2 + 1) / 2;
    rect(16 - n, 62 + hy, 2 * n, 4, c);
    for (int yy = hy - 1; yy >= hy - n; yy--) {
        int dy2 = 2 * (hy - yy) - 1, rr = r2 * r2 - dy2 * dy2; if (rr <= 0) break;
        int hw2 = 0; while ((hw2 + 1) * (hw2 + 1) <= rr) hw2++;
        int m = (hw2 + 1) / 2; if (m > 0) rect(16 - m, 62 + yy, 2 * m, 1, c);
    }
}
static void build_player(int bob, int leg, int brk) {
    tgt_bmp(PB);
    u8 K = C(0x1b1212), LT = C(0xe8edf5);
    const look_t *Lk = &LOOKS[REP_LOOK[MG.look & 3]];
#define R(a, b, w, h, c) rect(16 + (a), 62 + (b), (w), (h), (c))
    int vh = MG.veh;
    if (vh == 0) {
        R(-6, -16, 12, 16, LT); R(-4, -15, 8, 15, K); R(-2, -14, 4, 12, C(0x555a66)); R(-10, -25, 20, 11, LT); R(-9, -24, 18, 9, C(0xd62839)); R(-9, -24, 18, 2, C(0xff8a92));
        R(-4, -22, 8, 3, brk ? C(0xffe0e0) : C(0x7a0010)); R(-3, -17, 6, 2, C(0xffffff)); R(-11, -36, 22, 3, LT); R(-10, -35, 20, 1, K); R(-12, -39, 4, 4, LT); R(8, -39, 4, 4, LT);
    } else if (vh == 1) {
        R(-3, -17, 6, 17, LT); R(-1, -16, 2, 16, K); R(-6, -21, 12, 4, LT); R(-5, -20, 10, 2, C(0x1f9e8f)); R(-2, -26, 4, 6, C(0x7fe0d0)); R(-11, -36, 22, 3, LT); R(-10, -35, 20, 1, K);
    } else {
        R(-4, -9, 8, 9, LT); R(-2, -8, 4, 8, K); R(-8, -5, 16, 5, LT); R(-7, -4, 14, 3, C(0x58c4ff)); R(-2, -36, 4, 28, LT); R(-1, -35, 2, 27, C(0x555a66)); R(-10, -37, 20, 3, LT); R(-9, -36, 18, 1, K);
    }
    R(-9, -26 - bob, 18, 6, LT); R(-8, -25 - bob, 16, 4, C(0x4a6fa5)); R(-3, -22 - bob, 6, 9, K); R(-2, -22 - bob, 4, 8, C(0x333333));
    R(-9, -24 - bob + leg, 7, 14, LT); R(-8, -24 - bob + leg, 5, 13, K); R(-7, -23 - bob + leg, 3, 11, C(0x4a6fa5));
    R(2, -24 - bob - leg, 7, 14, LT); R(3, -24 - bob - leg, 5, 13, K); R(4, -23 - bob - leg, 3, 11, C(0x4a6fa5));
    u8 RED = C(0xd62839), RED2 = C(0xff5a66), RED3 = C(0xa51d2b);
    R(-10, -39 - bob, 20, 17, LT); R(-9, -38 - bob, 18, 15, K); R(-8, -37 - bob, 16, 13, RED); R(-11, -35 - bob, 3, 8, RED); R(8, -35 - bob, 3, 8, RED);
    R(-11, -28 - bob, 3, 2, Lk->S); R(8, -28 - bob, 3, 2, Lk->S);
    int hy = -53 - bob;
    dome(17, hy, LT); dome(15, hy, K); dome(13, hy, RED);
    R(-1, hy - 6, 2, 10, C(0xffffff)); R(-6, hy - 4, 2, 2, RED2); R(-7, hy + 2, 14, 2, K);
    R(-9, -50 - bob, 18, 16, LT); R(-8, -49 - bob, 16, 14, K); R(-7, -48 - bob, 14, 12, RED); R(-7, -48 - bob, 14, 2, RED2); R(-7, -37 - bob, 14, 1, RED3);
    static const s8 px[8][2] = {{-4, -46}, {-4, -45}, {-4, -44}, {-4, -43}, {-4, -42}, {-3, -46}, {-2, -46}, {-3, -44}};
    for (int i = 0; i < 8; i++) R(px[i][0], px[i][1] - bob, 1, 1, C(0xffffff));
#undef R
    tgt_back();
}
static u8 PSP[12][64][2] EWRAM_BSS;   /* tramo opaco de cada fila del rider */
/* blit del rider con cizalla horizontal (≈ rotate) y solo el tramo opaco de cada fila */
IWRAM_CODE static void pblit(const bmp_t *b, u8 (*sp)[2], int x, int y, s32 a) {
    for (int j = 0; j < 64; j++) {
        int a0 = sp[j][0], a1 = sp[j][1]; if (a0 >= a1) continue;
        int ty = y + j; if (ty < 0 || ty >= H) continue;
        int off = (int)((-(s32)(j - 62) * a + 32768) >> 16), bx = x + off;
        const u8 *src = b->px + j * 32;
        int i0 = imax(a0, -bx), i1 = imin(a1, W - bx);
        u8 *row = T.px + (ty + OY) * T.w + OX + bx;
        for (int i = i0; i < i1; i++) {
            u8 c = src[i]; if (!c) continue;
            u8 *pp = row + i; volatile u16 *q = (u16 *)((u32)pp & ~1u);
            if ((u32)pp & 1) *q = (*q & 0x00FF) | (c << 8);
            else if (i + 1 < i1 && src[i + 1]) { *q = c | (src[i + 1] << 8); i++; }
            else *q = (*q & 0xFF00) | c;
        }
    }
}
static void rep_player(int x, int y, s32 lean, int brk, int t, s32 v) {
    int bob = v > F(.05) ? ((t >> 2) & 1) : 0;
    int leg = MG.veh == 1 && v > F(.05) ? ((t >> 3) & 1 ? 2 : -2) : 0;
    int key = bob * 6 + (leg + 2) / 2 * 2 + (brk ? 1 : 0);
    if (!PCACHE[key]) {
        PB = PCACHE[key] = bmp_new(32, 64); build_player(bob, leg, brk);
        for (int j = 0; j < 64; j++) { const u8 *r = PB->px + j * 32; int a0 = 0, a1 = 32; while (a0 < 32 && !r[a0]) a0++; while (a1 > a0 && !r[a1 - 1]) a1--; PSP[key][j][0] = a0; PSP[key][j][1] = a1; }
    }
    PB = PCACHE[key];
    rect_dark(x - 12, y - 2, 24, 3);
    /* g.rotate(-lean*.25) ≈ cizalla horizontal: x' = x - y·sin(a) */
    s32 a = -(lean >> 2);   /* Q16 rad */
    pblit(PB, PSP[key], x - 16, y - 62, a);
}

/* ---------------------------------------------------------------- tarjeta de pedido (repCard) */
static void rep_card(const char *title, const char *l0, const char *l1, const char *l2, const char *btn, int icon, int stars) {
    int x = 16, y = 22, w = W - 32, h = 118;
    int key = G.phase * 64 + G.order * 4 + stars + (((MG.t >> 4) & 1) << 10);
    if (!CARDB) { CARDB = bmp_new(w + 4, h + 4); card_key = -1; }
    if (key == card_key) { rep_copy(CARDB, x - 2, y - 2); return; }
    card_key = key; tgt_bmp(CARDB); origin(-(x - 2), -(y - 2));
    rect(x - 2, y - 2, w + 4, h + 4, C(0x120b0c)); rect(x, y, w, h, C(0xffffff)); rect(x, y, w, 16, C(0xd0102c));
    arc_text(W / 2, y + 4, title, C(0xffffff), 1, 8);
    rep_food(icon, x + 6, y + 22, 4);
    text(x + 30, y + 24, l0, C(0x1b1212), 0); text(x + 6, y + 24 + 15 + 6, l1, C(0x1b1212), 0); text(x + 6, y + 24 + 30 + 6, l2, C(0x1b1212), 0);
    if (stars) {
        for (int i = 0; i < 3; i++) {
            int cx = W / 2 - 22 + i * 22, cy = y + 82; s16 pts[20];
            for (int k = 0; k < 10; k++) { int r2 = k & 1 ? 7 : 16; int a = -256 + k * 1024 / 10; pts[k * 2] = cx + ((icos(a) * r2 + 4096) >> 13); pts[k * 2 + 1] = cy + ((isin(a) * r2 + 4096) >> 13); }
            poly(pts, 10, i < stars ? C(0xffd23f) : C(0xcccccc));
            for (int k = 0; k < 10; k++) { int n = (k + 1) % 10; line(pts[k * 2], pts[k * 2 + 1], pts[n * 2], pts[n * 2 + 1], C(0xc9971a), 1); }
        }
    }
    rect(W / 2 - 50, y + h - 18, 100, 14, (MG.t >> 4) & 1 ? C(0xd0102c) : C(0xa51d2b));
    arc_text(W / 2, y + h - 15, btn, C(0xffffff), 1, 8);
    tgt_back(); rep_copy(CARDB, x - 2, y - 2);
}

/* ---------------------------------------------------------------- cielo */
static const u8 *SKYC(void) {
    static u8 c[8]; c[0] = C(0x4282d8); c[1] = C(0x538fdd); c[2] = C(0x649ce2); c[3] = C(0x74a8e7); c[4] = C(0x85b6ed); c[5] = C(0x95c3f2); c[6] = C(0xa6cff7); c[7] = C(0xb7dcfc); return c;
}
static void build_sky(void) {
    if (SKYB) return;
    SKYB = bmp_new(2 * W, 25); tgt_bmp(SKYB); origin(0, -(HZ - 24));
    const u8 *c = SKYC();
    for (int y = HZ - 24; y <= HZ; y++) rect(0, y, 2 * W, 1, c[imin(7, y >> 3)]);
    for (int q = 0; q < 2; q++) {
        int off = q * W;
        s16 pts[12] = {off, HZ, off + 30, HZ - 14, off + 70, HZ - 8, off + 110, HZ - 16, off + 160, HZ - 6, off + W, HZ};
        poly(pts, 6, C(0x7d8fb5));
        static const u8 tw[7][2] = {{18, 10}, {60, 22}, {64, 30}, {68, 28}, {72, 22}, {130, 18}, {180, 12}};
        for (int i = 0; i < 7; i++) rect(off + tw[i][0], HZ - tw[i][1], 3, tw[i][1], C(0x6a7ca3));
        static const u8 el[5] = {3, 3, 3, 2, 1};   /* media elipse rx3 ry5 */
        for (int i = 0; i < 5; i++) rect(off + 131 - el[i], HZ - 18 - 1 - i, el[i] * 2, 1, C(0x6a7ca3));
    }
    tgt_back();
}

/* ---------------------------------------------------------------- dibujo */
typedef struct { s32 z, sc, x, y; s16 k, i; s8 side, hid; } obj_t;
#define K_BLD 0
#define K_TREE 1
#define K_LIGHT 2
#define K_HOUSE 3
#define K_CAR 4
#define K_CROSS 5
static obj_t OBJ[96] EWRAM_BSS; static obj_t *OP[96];
static s32 CAM, CV;
static inline int proj(obj_t *o, s32 s, s32 x) {
    s32 z = s - G.s; if (z < F(.35) || z >= F(115)) return 0;
    u32 m = (u32)z; int e = 0; while (m >= 2048) { m >>= 1; e++; }   /* z = m·2^e, m en [1024, 2048) */
    s32 sc = (s32)(RECIP[m - 1024] >> e);
    s32 shift8 = (((CV * (z >> 8)) >> 12) * 410) >> 8;
    o->z = z; o->sc = sc; o->x = (W / 2) * 256 + ((((x - CAM) >> 8) * sc) >> 8) + shift8; o->y = HZ * 256 + ((sc * 358) >> 8);
    return 1;
}
IWRAM_CODE static int gather(void) {
    /* objetos */
    int n = 0;
    s32 i6 = (G.s >> 16) / 6;
    for (s32 s = i6 * 6; (s << 16) < G.s + F(110) && n < 80; s += 6, i6++) {
        int skip = 0; for (int i = 0; i < G.nin; i++) { s32 is = G.in[i].s >> 16; if (s >= is - 3 && s < is + 11) skip = 1; }
        if (skip) continue;
        if (proj(&OBJ[n], s << 16, F(-4.95))) { OBJ[n].k = K_BLD; OBJ[n].i = i6; OBJ[n].side = -1; n++; }
        if (proj(&OBJ[n], s << 16, F(4.95))) { OBJ[n].k = K_BLD; OBJ[n].i = i6 + 7; OBJ[n].side = 1; n++; }
        if (!(i6 & 1)) {
            if (proj(&OBJ[n], (s + 3) << 16, F(-2.45))) { OBJ[n].k = K_TREE; n++; }
            if (proj(&OBJ[n], (s + 3) << 16, F(2.45))) { OBJ[n].k = K_TREE; n++; }
        }
    }
    for (int i = 0; i < G.nin; i++) {
        if (proj(&OBJ[n], G.in[i].s - F(.6), F(2.35))) { OBJ[n].k = K_LIGHT; OBJ[n].i = i; n++; }
        if (proj(&OBJ[n], G.in[i].s - F(.6), F(-2.35))) { OBJ[n].k = K_LIGHT; OBJ[n].i = i; n++; }
    }
    if (G.phase != PH_ORDER && proj(&OBJ[n], G.dest, F(3.2))) { OBJ[n].k = K_HOUSE; n++; }
    for (int i = 0; i < G.ntr; i++) if (proj(&OBJ[n], G.tr[i].s, G.tr[i].x)) { OBJ[n].k = K_CAR; OBJ[n].i = i; n++; }
    for (int i = 0; i < G.ncr; i++) if (proj(&OBJ[n], G.cr[i].s, G.cr[i].x)) { OBJ[n].k = K_CROSS; OBJ[n].i = i; n++; }
    {   static s32 ZS[96]; static u8 ORD[96];   /* orden de lejos a cerca (casi ordenado al recorrerlo al revés) */
        for (int m = 0; m < n; m++) { int i = n - 1 - m; s32 z = OBJ[i].z; int j = m; while (j > 0 && ZS[j - 1] < z) { ZS[j] = ZS[j - 1]; ORD[j] = ORD[j - 1]; j--; } ZS[j] = z; ORD[j] = i; }
        for (int m = 0; m < n; m++) OP[m] = &OBJ[ORD[m]];
    }
    /* oclusión: un objeto lejano que queda entero dentro de un edificio más cercano no se dibuja */
    static s16 occ[40][4]; int nocc = 0;
    for (int i = n - 1; i >= 0; i--) {
        obj_t *o = OP[i]; SC = o->sc; o->hid = 0;
        s32 hw = (o->k == K_BLD ? M(2.1) : o->k == K_TREE ? M(.6) : o->k == K_LIGHT ? M(.2) : M(1.6));
        s32 ht = (o->k == K_BLD ? M(3.6) : o->k == K_TREE ? M(2) : o->k == K_LIGHT ? M(3) : M(4));
        int x0 = rnd8(o->x - hw), x1 = rnd8(o->x + hw), y0 = rnd8(o->y - ht), y1 = rnd8(o->y);
        if (o->k > K_LIGHT) continue;
        int hid = 0;
        for (int q = 0; q < nocc && !hid; q++) if (x0 >= occ[q][0] && x1 <= occ[q][1] && y0 >= occ[q][2] && y1 <= occ[q][3]) hid = 1;
        o->hid = hid; if (hid) continue;
        if (o->k == K_BLD && nocc < 12) { occ[nocc][0] = x0; occ[nocc][1] = x1; occ[nocc][2] = y0; occ[nocc][3] = y1; nocc++; }
    }
    return n;
}
/* edificio con recorte en L: no se pinta lo que tapará el siguiente edificio más cercano del mismo lado */
static void bld_clipped(int i, int n) {
    obj_t *o = OP[i], *nr = 0;
    for (int j = i + 1; j < n; j++) if (OP[j]->k == K_BLD && OP[j]->side == o->side) { nr = OP[j]; break; }
    OCS = 0;
    if (nr) { SC = nr->sc; OC0 = rnd8(nr->x - M(2.1)); OC1 = OC0 + wq(M(4.2)); OCY = rnd8(nr->y - M(3.6)); OCS = o->side; }
    SC = o->sc; rep_bld(o->x, o->y, iabs(o->i)); OCS = 0;
}
static void draw_(void);
static void draw(void) { u32 t0 = ticks(); draw_(); REP_DT = ticks() - t0; if (G.phase == PH_RIDE) REP_DT_AVG = (REP_DT_AVG * 15 + REP_DT) >> 4; }
static void draw_(void) {
    kbox(0, 0, W, H);
    if (last_fn) { int n = frame_no - last_fn; if (n > REP_NMAX && n < 10) REP_NMAX = n; if (G.phase == PH_RIDE) REP_NH[imin(n, 3)]++; }
    last_fn = frame_no;
    int t = G.t; u32 t0 = ticks();
    CAM = (G.x >> 8) * 218; /* G.x*.85 en Q16 */
    CV = curve(G.s);
    /* cielo: degradado (bandas), nubes y skyline pre-dibujado (SKYB, 2 copias seguidas) */
    static const u8 *skyc; skyc = SKYC();
    for (int i = 0; i < 5; i++) rect(0, i * 8, W, 8, skyc[i]);
    s32 sky16 = G.sky;
    {   int sk = (int)(((-(sky16 >> 8) * 154) >> 16) % W); sk = ((sk + W) % W) & ~1;
        const u8 *src0 = SKYB->px + (W - sk); u8 *d0 = T.px + (HZ - 24 + OY) * T.w + OX;
        for (int j = 0; j < 25; j++) { REG_DMA3SAD = (u32)(src0 + j * (2 * W)); REG_DMA3DAD = (u32)(d0 + j * T.w); REG_DMA3CNT = 0x80000000u | (W / 2); }
    }
    for (int i = 0; i < 4; i++) {
        int x = (int)(((i * 71 * 65536 - (sky16 >> 8) * 77) >> 16) % (W + 60)); x = ((x + W + 60) % (W + 60)) - 30;
        rect(x, 10 + i * 7, 22, 4, C(0xffffff)); rect(x + 4, 7 + i * 7, 12, 3, C(0xffffff));
    }
    REP_TK[0] = ticks() - t0; t0 = ticks();
    rep_road(CAM >> 8, CV);
    REP_TK[1] = ticks() - t0; t0 = ticks();
    int n = gather();
    for (int i = 0; i < n; i++) {
        obj_t *o = OP[i]; SC = o->sc;
        if (o->hid) continue;
        switch (o->k) {
            case K_BLD: bld_clipped(i, n); break;
            case K_TREE: rep_tree(o->x, o->y); break;
            case K_LIGHT: rep_light(o->x, o->y, light(&G.in[o->i], t)); break;
            case K_HOUSE: rep_house(o->x, o->y); break;
            case K_CAR: { car_t *c = &G.tr[o->i]; rep_rear(o->x, o->y, c->type, c->col, c->braking, c->mv >= 0 && c->blink <= 40 ? (c->mv > c->lane ? 1 : -1) : 0, t); } break;
            case K_CROSS: rep_side(o->x, o->y, G.cr[o->i].col, G.cr[o->i].dx > 0); break;
        }
    }
    REP_TK[2] = ticks() - t0; t0 = ticks();
    if (G.phase != PH_ORDER && !(G.inv > 0 && (t >> 2) & 1)) {
        s32 px = (W / 2) * 256 + ((((G.x - (CAM)) >> 8) * 14400) >> 8); /* FOC/ZP = 56.25 → Q8 14400 */
        rep_player(rnd8(px), H - 4, G.lean, G.braking, t, G.v);
    }
    REP_TK[3] = ticks() - t0; t0 = ticks();
    if (G.horn > 0) arc_text(W / 2, H - 60, "¡PIII!", C(0xffd23f), 1, 8);
    /* marcador */
    const order_t *O = &REP_ORDERS[imin(G.order, 2)]; char b[40], *p;
    {   int sec = G.time / 60, key = (G.order << 24) ^ (sec << 8) ^ (G.estado & 255);
        if (!HUDB) { HUDB = bmp_new(W, 20); hud_key = -1; }
        if (key != hud_key) {
            hud_key = key; tgt_bmp(HUDB);
            rect(0, 0, W, 20, C(0x120b0c));
            rep_food(O->icon, 4, 4, 3);
            p = str_cat(b, "PEDIDO "); p = str_num(p, imin(G.order + 1, 3)); str_cat(p, "/3"); arc_text(22, 2, b, C(0xffd23f), 0, 8);
            { int mm = sec / 60, ss = sec % 60; p = b; if (mm < 10) *p++ = '0'; p = str_num(p, mm); *p++ = ':'; if (ss < 10) *p++ = '0'; str_num(p, ss); arc_text(W - 4, 2, b, C(0xffffff), 2, 8); }
            rect(22, 12, 70, 5, C(0x333333)); if (G.estado > 0) rect(22, 12, 70 * G.estado / 100, 5, G.estado > 60 ? C(0x62d26f) : G.estado > 30 ? C(0xffd23f) : C(0xff4a5c));
            tgt_back();
        }
        rep_copy(HUDB, 0, 0);
    }
    if (G.phase == PH_RIDE) {
        s32 k = (G.s + ZP) / (G.dest >> 16); /* Q16 */ k = iclamp(k, 0, 65536);
        rect(100, 12, 80, 5, C(0x333333)); rect(100, 12, (80 * k) >> 16, 5, C(0xd62839)); rect(178, 10, 4, 9, C(0x62d26f));
        /* (en la GBA el cuadro de velocidad sube 8 px para no tapar el texto de progreso) */
        rect(2, H - 22, 60, 12, C(0x120b0c));
        p = str_num(b, (G.v * 140 + 32768) >> 16); str_cat(p, " KM/H"); arc_text(32, H - 20, b, C(0xffffff), 1, 8);
        if (G.v < F(.02) && G.time < 240) arc_text(W / 2, H - 38, "MANTÉN ↑ PARA ACELERAR", (G.t >> 4) & 1 ? C(0xffd23f) : C(0xffffff), 1, 8);
        for (int i = 0; i < G.nin; i++) {
            inter_t *I = &G.in[i];
            if (I->s > G.s + ZP && I->s - G.s < F(45)) {
                int L = light(I, t); if (L) arc_text(W / 2, 24, L == 2 ? "¡ROJO! FRENA" : "ÁMBAR", (t >> 3) & 1 ? C(0xff4a5c) : C(0xffd23f), 1, 8);
                break;
            }
        }
        if (iabs(G.dest - G.s - ZP) < F(30)) arc_text(W / 2, 36, G.lane == 3 ? "¡FRENA Y ENTREGA (A)!" : "CARRIL DERECHO »", C(0x62d26f), 1, 8);
    }
    if (G.phase == PH_ORDER) {
        char d[40]; str_cat(str_cat(d, "DIR: "), O->addr);
        rep_card("NUEVO PEDIDO", O->rest, O->food, d, "A: ACEPTAR", O->icon, 0);
    }
    if (G.phase == PH_DELIV) {
        char l[40]; p = str_cat(l, "TIEMPO "); p = str_num(p, G.lastTime); p = str_cat(p, " s · "); p = str_num(p, imax(0, G.estado)); str_cat(p, "%");
        rep_card("¡ENTREGADO!", O->food, l, "CLIENTE: ¡GRACIAS!", G.order < 2 ? "A: SIGUIENTE" : "A: TERMINAR", O->icon, G.lastStars);
    }
    REP_TK[4] = ticks() - t0;
}

const arc_def_t ARC_DEF4 = {"MODO REPARTIDOR", 224, 160,
    "MANTÉN ↑ PARA ACELERAR · ← →: CAMBIAR DE CARRIL · ↓/B: FRENAR · A: CLAXON / ENTREGAR. Para en los semáforos en rojo y entrega en el carril derecho, junto a la casa del cliente.",
    init, update, draw, press, prog};
