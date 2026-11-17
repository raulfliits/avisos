/* AZLAN · EPI SHOOTER (primera persona, laberinto del almacén) — ARC_GAMES[3] del web.
   Raycaster en punto fijo (DDA entero, 16.16) con z-buffer; dibuja en un búfer de 192x144 en EWRAM
   (bytes) y lo copia a la pantalla con palabras de 32 bits. Las manos y textos van encima, directo a pantalla. */
#include "arcade.h"

#define W 192
#define H 144
#define MAPN 12
extern bmp_t T; /* destino actual del motor (eng.c) */
typedef int64_t s64; typedef uint64_t u64;

static const char *const DMAP[MAPN] = {"############", "#....#.....#", "#.##.#.###.#", "#.#........#", "#.#.####.#.#", "#...#..#.#.#",
                                       "###.#..#.#.#", "#......#...#", "#.####.###.#", "#.#........#", "#...#..#...#", "############"};
static const char *const EPI_UP[3] = {"CASCO", "CHAQUETA", "GUANTES"};
static const char *const EPI_NAME[3] = {"EL CASCO", "LA CHAQUETA", "LOS GUANTES"};
static const char *const NPC_NAME[6] = {"AAMIR", "BILAL", "HAMZA", "IMRAN", "FARHAN", "SAIRA"};

/* colores de pared con niebla (6 niveles: alfa 0 · .15 · .3 · .45 · .6 · .75 de negro) */
static const u8 FOGC[6][6] = {
    {C(0xd62839), C(0xb62230), C(0x961c28), C(0x76161f), C(0x561017), C(0x360a0e)}, /* cartel rojo */
    {C(0xffffff), C(0xd9d9d9), C(0xb2b2b2), C(0x8c8c8c), C(0x666666), C(0x404040)}, /* letras del cartel */
    {C(0x8c3a2a), C(0x773124), C(0x62291d), C(0x4d2017), C(0x381711), C(0x230e0a)}, /* pared lado x */
    {C(0x6e2d21), C(0x5e261c), C(0x4d1f17), C(0x3d1912), C(0x2c120d), C(0x1c0b08)}, /* pared lado y */
    {C(0x502218), C(0x441d14), C(0x381811), C(0x2c130d), C(0x200e0a), C(0x140806)}, /* juntas lado x (rgba(30,15,10,.55)) */
    {C(0x421c14), C(0x381811), C(0x2e140e), C(0x240f0b), C(0x1a0b08), C(0x100705)}, /* juntas lado y */
};
static const u8 CEIL[6] = {C(0x16161f), C(0x1d1d28), C(0x242430), C(0x2c2c39), C(0x333341), C(0x3a3a4a)};
static const u8 FLOOR[6] = {C(0x2b2622), C(0x342e28), C(0x3e362e), C(0x473d34), C(0x51453a), C(0x5a4d40)};

/* ---- estado (global para que el bot de pruebas lo lea) ---- */
typedef struct { float x, y; s32 gear, look, hired, t, alive; } sh_person_t;
typedef struct { float x, y, dx, dy; s32 item, life; } sh_proj_t;
float sh_px, sh_py, sh_pa, sh_bob;
s32 sh_item, sh_hand, sh_msgT, sh_near, sh_nproj;
sh_person_t sh_people[5];
sh_proj_t sh_proj[16];
static char sh_msg[64];
s32 sh_mxinfo[4], sh_last[8]; static u32 sh_tspr; s32 sh_cp[3], sh_msgcyc; s32 sh_sum[4]; s32 sh_upd, sh_updmax; s32 sh_n3, sh_n2, sh_n1; s32 sh_maxn, sh_cyc, sh_maxcyc, sh_draws, sh_ccast;
static u32 sh_lastf;

static s32 ZB[W];                       /* z-buffer: distancia perpendicular 16.16 */
static u8 MAPW[MAPN * MAPN], POSTER[MAPN * MAPN];
static u32 BG4[H];                      /* techo/suelo: pareja de píxeles (trama a cuadros) */
static s16 CAM[W];                      /* cámara: 2x/W - 1 con 14 bits de fracción */
#include "arc_shooter_rcp.h"           /* RCP[m] = 2^42 / m, m = 2048..4095 (ROM) */
static s16 CY0[W], CY1[W]; static u8 CMAIN[W], CDARK[W], CPOST[W], CSTR[W]; static u32 CH8F[W]; static s16 CTOP[W];
/* bitmaps ya pintados/escalados, en el pool (se piden en el primer init de cada partida, MG.t == 0; los reintentos
   los reutilizan). Ancho de fila múltiplo de 4 (rect() del motor lo necesita en bitmaps); RW = ancho útil. */
static bmp_t *LBLB[3], *MSGBB, *ITSB[3][3], *HLB, *HRB, *FBXB, *ITBB[3]; static char msg_key[64]; static int msg_h;
static const u8 ITH[3] = {5, 6, 6};
static void item_at(int it, int x, int y, int sc) { if (sc >= 1 && sc <= 3 && ITSB[it][sc - 1]) blit_part(ITSB[it][sc - 1], 0, 0, 7 * sc, ITH[it] * sc, x, y, 0); else draw_item(it, x, y, sc); }
static bmp_t *bmp4(int w, int h) { return bmp_new((w + 3) & ~3, h); }
/* escalado al vecino más próximo de src (sw x sh, fila sstride) a d (dw x dh) */
static void scale_copy(bmp_t *d, int dw, int dh, const u8 *src, int sw, int sh, int sstride) {
    if (!d) return;
    for (int j = 0; j < dh; j++) for (int i = 0; i < dw; i++) d->px[j * d->w + i] = src[(j * sh / dh) * sstride + i * sw / dw];
}

static int dwall(float x, float y) {
    if (x < 0 || y < 0) return 1;
    int r = (int)y, c = (int)x; if (r >= MAPN || c >= MAPN) return 1;
    return DMAP[r][c] != '.';
}
static float fsgn(float v) { return v > 0 ? 1.f : v < 0 ? -1.f : 0.f; }
static int ffloor(float v) { int i = (int)v; return (v < i) ? i - 1 : i; }
static int iround(float v) { return ffloor(v + 0.5f); } /* Math.round */
static int fceil(float v) { int i = (int)v; return (v > i) ? i + 1 : i; }

static void sh_init(void) {
    static const float P[5][2] = {{6.5f, 1.5f}, {10.5f, 3.5f}, {3.5f, 7.5f}, {9.5f, 9.5f}, {5.5f, 10.5f}};
    static const u8 GEAR[5] = {0, G_GUANT, G_CASCO, 0, G_CHAQ};
    sh_px = 1.5f; sh_py = 1.5f; sh_pa = 0; sh_item = 0; sh_hand = 0; sh_bob = 0; sh_msgT = 0; sh_msg[0] = 0; sh_near = -1; sh_nproj = 0;
    for (int i = 0; i < 5; i++) { sh_person_t *q = &sh_people[i]; q->x = P[i][0]; q->y = P[i][1]; q->gear = GEAR[i]; q->look = i; q->hired = 0; q->t = 0; q->alive = 1; }
    for (int r = 0; r < MAPN; r++) for (int c = 0; c < MAPN; c++) { MAPW[r * MAPN + c] = DMAP[r][c] != '.'; POSTER[r * MAPN + c] = (c * 7 + r * 3) % 6 == 0; }
    /* degradados del techo y del suelo (6 tonos con trama a cuadros) */
    for (int y = 0; y < H; y++) {
        const u8 *g = y < H / 2 ? CEIL : FLOOR; int yy = y < H / 2 ? y : y - H / 2;
        int v = yy * 10 / (H / 2 - 1); /* 0..10 */
        { u32 e = g[(v + 1) / 2 > 5 ? 5 : (v + 1) / 2], o = g[v / 2]; BG4[y] = (e | (o << 8)) * 0x10001u; }
    }
    for (int x = 0; x < W; x++) CAM[x] = (s16)(((2 * x - W) << 14) / W);
    if (MG.t == 0 || !FBXB) {
        u8 skin = C(0x9c6640), K = C(0x1b1212), sk2 = C(0x7d4f2f);
        /* mochila FLiiTS x2 y EPIs x2,4 de la mano ya escalados (sin escalar en cada fotograma) */
        bmp_t *tmp = bmp4(21, 13); FBXB = bmp4(42, 26);
        if (tmp) { tgt_bmp(tmp); fbox(0, 0); tgt_back(); scale_copy(FBXB, 42, 26, tmp->px, 21, 13, tmp->w); }
        for (int i = 0; i < 3; i++) for (int k = 0; k < 3; k++) { bmp_t *b = ITSB[i][k] = bmp4(7 * (k + 1), ITH[i] * (k + 1)); if (b) { tgt_bmp(b); draw_item(i, 0, 0, k + 1); tgt_back(); } }
        for (int i = 0; i < 3; i++) { ITBB[i] = bmp4(17, i ? 14 : 12); if (ITSB[i][0]) scale_copy(ITBB[i], 17, i ? 14 : 12, ITSB[i][0]->px, 7, ITH[i], ITSB[i][0]->w); }
        for (int i = 0; i < 3; i++) { LBLB[i] = bmp4(68, 12); if (LBLB[i]) { tgt_bmp(LBLB[i]); cls(C(0x120b0c)); arc_text(34, 2, EPI_UP[i], C(0xffd23f), 1, 8); tgt_back(); } }
        MSGBB = bmp4(188, 49);
        HLB = bmp4(18, 8); if (HLB) { tgt_bmp(HLB); rect(0, 0, 18, 8, K); rect(1, 1, 16, 6, skin); rect(5, 1, 1, 6, sk2); rect(10, 1, 1, 6, sk2); tgt_back(); }
        HRB = bmp4(22, 24); if (HRB) { tgt_bmp(HRB); rect(0, 0, 22, 24, K); rect(1, 1, 20, 23, skin); rect(6, 1, 1, 8, sk2); rect(11, 1, 1, 8, sk2); rect(16, 1, 1, 8, sk2); tgt_back(); }
    }
    msg_key[0] = 0;
    sh_maxn = 0; sh_lastf = frame_no; sh_maxcyc = 0; sh_draws = 0; sh_n1 = sh_n2 = sh_n3 = 0;
}

static void sh_press(int b) {
    if (b == B_B) { sh_item = (sh_item + 1) % 3; tone(500, 40, 0, W_SQUARE, 60); }
    if (b == B_A && sh_hand <= 0 && sh_nproj < 16) {
        float c = fcos(sh_pa), s = fsin(sh_pa);
        sh_proj_t *p = &sh_proj[sh_nproj++];
        p->x = sh_px + c * .3f; p->y = sh_py + s * .3f; p->dx = c * .16f; p->dy = s * .16f; p->item = sh_item; p->life = 70;
        sh_hand = 14; slide(500, 900, 120);
    }
}

static int seen(const sh_person_t *q) { /* igual que el JS (11 muestras en la recta), en punto fijo 16.16 */
    s32 px = (s32)(sh_px * 65536), py = (s32)(sh_py * 65536), ex = (s32)(q->x * 65536) - px, ey = (s32)(q->y * 65536) - py;
    for (int k = 1; k < 12; k++) { s32 x = px + ex * k / 12, y = py + ey * k / 12;
        if (x < 0 || y < 0 || (x >> 16) >= MAPN || (y >> 16) >= MAPN || MAPW[(y >> 16) * MAPN + (x >> 16)]) return 0; }
    return 1;
}

static void sh_update_(void);
static void sh_update(void) { u32 t0 = ticks(); sh_update_(); sh_upd = ticks() - t0; if (sh_upd > sh_updmax) sh_updmax = sh_upd; }
static void sh_update_(void) {
    int fw = (key_held & K_UP) ? 1 : (key_held & K_DOWN) ? -1 : 0, tr = (key_held & K_LEFT) ? -1 : (key_held & K_RIGHT) ? 1 : 0;
    sh_pa += tr * .05f; if (sh_hand > 0) sh_hand--;
    if (fw) {
        float sp = .05f * fw, nx = sh_px + fcos(sh_pa) * sp, ny = sh_py + fsin(sh_pa) * sp, m = .22f;
        if (!dwall(nx + fsgn(nx - sh_px) * m, sh_py)) sh_px = nx;
        if (!dwall(sh_px, ny + fsgn(ny - sh_py) * m)) sh_py = ny;
        sh_bob += .25f;
    }
    int k = 0;
    for (int i = 0; i < sh_nproj; i++) {
        sh_proj_t *p = &sh_proj[i]; int keep = 1;
        p->x += p->dx; p->y += p->dy;
        if (--p->life <= 0 || dwall(p->x, p->y)) keep = 0;
        else {
            sh_person_t *h = 0;
            for (int j = 0; j < 5; j++) { sh_person_t *q = &sh_people[j]; float ax = q->x - p->x, ay = q->y - p->y;
                if (q->alive && !q->hired && ax * ax + ay * ay < .42f * .42f) { h = q; break; } }
            if (h) {
                const char *name = NPC_NAME[h->look]; char *o = sh_msg; int bit = 1 << p->item;
                keep = 0;
                if (h->gear & bit) { o = str_cat(o, "¡"); o = str_cat(o, name); o = str_cat(o, " YA TIENE "); o = str_cat(o, EPI_NAME[p->item]); str_cat(o, "!"); sh_msgT = 90; sfx_back(); }
                else {
                    h->gear |= bit; MG.c1++; sfx_coin();
                    if ((h->gear & 7) == 7) { h->hired = 1; h->t = 80; MG.c2++; o = str_cat(o, name); str_cat(o, ": ¡GRACIAS!"); sh_msgT = 80; arc_hire(name); }
                    else { o = str_cat(o, "¡"); o = str_cat(o, EPI_NAME[p->item]); str_cat(o, " PUESTO!"); sh_msgT = 60; }
                }
            }
        }
        if (keep) sh_proj[k++] = *p;
    }
    sh_nproj = k;
    for (int j = 0; j < 5; j++) { sh_person_t *q = &sh_people[j]; if (q->alive && q->hired && --q->t <= 0) q->alive = 0; }
    if (sh_msgT > 0) sh_msgT--;
    else {
        int best = -1; float bd = 1e9f;
        for (int j = 0; j < 5; j++) { sh_person_t *q = &sh_people[j]; if (!q->alive || q->hired || !seen(q)) continue;
            float ax = q->x - sh_px, ay = q->y - sh_py, d = ax * ax + ay * ay; if (d < bd) { bd = d; best = j; } }
        sh_near = best >= 0 && bd < 3.2f * 3.2f ? best : -1;
    }
}

/* ---------------------------------------------------------------- raycaster (ARM en IWRAM) */
/* ---------------------------------------------------------------- raycaster (ARM en IWRAM) */
/* 2^K / v aproximado (error < 0,05 %) sin dividir: v normalizado a 12 bits y tabla de inversos */
static inline __attribute__((always_inline)) u32 rcp_n(u32 v, int *sh) {
    int n = 0; u32 t = v;
    if (t >> 16) { t >>= 16; n += 16; } if (t >> 8) { t >>= 8; n += 8; } if (t >> 4) { t >>= 4; n += 4; } if (t >> 2) { t >>= 2; n += 2; } if (t >> 1) n += 1;
    u32 m = n >= 11 ? v >> (n - 11) : v << (11 - n);
    *sh = 31 + n; return RCP[m - 2048]; /* 2^K/v = R >> (31 + n - K) */
}
IWRAM_CODE static void cast_cols(s32 px, s32 py, s32 dx, s32 dy, s32 plx, s32 ply, u8 *scr) {
    const s32 BIG = 1 << 26;
    /* 1) rayos */
    for (int x = 0; x < W; x++) {
        s32 cam = CAM[x];
        s32 rx = dx + ((plx * cam) >> 14), ry = dy + ((ply * cam) >> 14); /* 14 bits de fracción */
        int mx = px >> 16, my = py >> 16, sh;
        u32 ax = rx < 0 ? -rx : rx, ay = ry < 0 ? -ry : ry;
        s32 ddx = BIG, ddy = BIG;
        if (ax) { u32 r = rcp_n(ax, &sh); sh -= 30; ddx = sh >= 32 ? 0 : (s32)(r >> sh); if (ddx > BIG || !ddx) ddx = BIG; }
        if (ay) { u32 r = rcp_n(ay, &sh); sh -= 30; ddy = sh >= 32 ? 0 : (s32)(r >> sh); if (ddy > BIG || !ddy) ddy = BIG; }
        int sx, sy; s32 sdx, sdy;
        if (rx < 0) { sx = -1; sdx = (s32)(((s64)(px & 0xffff) * ddx) >> 16); } else { sx = 1; sdx = (s32)(((s64)(0x10000 - (px & 0xffff)) * ddx) >> 16); }
        if (ry < 0) { sy = -1; sdy = (s32)(((s64)(py & 0xffff) * ddy) >> 16); } else { sy = 1; sdy = (s32)(((s64)(0x10000 - (py & 0xffff)) * ddy) >> 16); }
        int side = 0, cell = my * MAPN + mx;
        for (int k = 0; k < 40; k++) {
            if (sdx < sdy) { sdx += ddx; cell += sx; side = 0; } else { sdy += ddy; cell += sy * MAPN; side = 1; }
            if (MAPW[cell]) break;
        }
        s32 dist = side ? sdy - ddy : sdx - ddx;
        if (dist < 256) dist = 256;
        u32 r = rcp_n((u32)dist, &sh);
        u32 h8 = (u32)(((u64)r * 144u) >> (sh - 24)); if (h8 > 432u * 256) h8 = 432u * 256; /* H/dist ×256 */
        int top = (72 * 256 - (s32)(h8 >> 1) + 128) >> 8, hc = (h8 + 255) >> 8;
        u32 wx = (u32)(side ? px + (s32)(((s64)dist * rx) >> 14) : py + (s32)(((s64)dist * ry) >> 14)) & 0xffff;
        int poster = POSTER[cell] && wx > 13107 && wx < 52429;
        int f = 0; /* niebla: round(min(.75,(dist-2)/9) / .15) */
        if (dist > 0x2AD00) { f = 1; if (dist > 0x4C99A) f = 2; if (dist > 0x6E666) f = 3; if (dist > 0x90333) f = 4; if (dist > 0xB2000) f = 5; }
        u8 main = poster ? FOGC[0][f] : FOGC[2 + side][f], dark = FOGC[4 + side][f];
        if (!poster && ((wx * 4) & 0xffff) < 5243) main = dark;
        CY0[x] = top < 0 ? 0 : top; CY1[x] = top + hc > H ? H : top + hc; CMAIN[x] = main; CDARK[x] = dark; CTOP[x] = top; CH8F[x] = h8;
        CPOST[x] = poster ? (((wx * 20) >> 16) % 3 == 0 ? 2 : 1) : 0; CSTR[x] = FOGC[1][f];
        ZB[x] = dist;
    }
    /* 2) relleno de 4 columnas a la vez (escrituras de 32 bits directas a VRAM): techo/suelo con máscara + pared */
    if (!((u32)scr & 3)) {
        for (int x = 0; x < W; x += 4) {
            u32 *p = (u32 *)(scr + x);
            int y = 0;
            while (y < H) {
                u32 mask = 0xffffffffu, wall = 0; int ny = H;
                for (int k = 0; k < 4; k++) {
                    int a0 = CY0[x + k], a1 = CY1[x + k];
                    if (y < a0) { if (a0 < ny) ny = a0; }
                    else if (y < a1) { if (a1 < ny) ny = a1; mask &= ~(0xffu << (8 * k)); wall |= (u32)CMAIN[x + k] << (8 * k); }
                }
                if (mask == 0) { for (; y < ny; y++, p += SW / 4) *p = wall; }
                else if (mask == 0xffffffffu) { for (; y < ny; y++, p += SW / 4) *p = BG4[y]; }
                else { for (; y < ny; y++, p += SW / 4) *p = (BG4[y] & mask) | wall; }
            }
        }
    } else
    for (int x = 0; x < W; x += 2) {
        int a0 = CY0[x], a1 = CY1[x], b0 = CY0[x + 1], b1 = CY1[x + 1];
        u16 ma = CMAIN[x], mb = CMAIN[x + 1] << 8, mab = ma | mb;
        u16 *p = (u16 *)(scr + x);
        int y = 0;
        while (y < H) {
            int wa = y >= a0 && y < a1, wb = y >= b0 && y < b1;
            int na = y < a0 ? a0 : y < a1 ? a1 : H, nb = y < b0 ? b0 : y < b1 ? b1 : H, ny = na < nb ? na : nb;
            if (wa && wb) { for (; y < ny; y++, p += SW / 2) *p = mab; }
            else if (wa) { for (; y < ny; y++, p += SW / 2) *p = ma | (BG4[y] & 0xff00); }
            else if (wb) { for (; y < ny; y++, p += SW / 2) *p = (BG4[y] & 0xff) | mb; }
            else { for (; y < ny; y++, p += SW / 2) *p = (u16)BG4[y]; }
        }
    }
    /* 3) detalles: juntas horizontales (4 filas de tablones) o letras del cartel */
    for (int x = 0; x < W; x++) {
        u32 h8 = CH8F[x]; int top = CTOP[x]; u8 c; int ya, yb;
        if (CPOST[x] == 1) continue;
        if (CPOST[x] == 2) { ya = (72 * 256 - (s32)((h8 * 31) >> 8) + 128) >> 8; int sh = (s32)((h8 * 61 / 256 + 128) >> 8); if (sh < 1) sh = 1; yb = ya + sh; c = CSTR[x];
            if (ya < 0) ya = 0; if (yb > H) yb = H;
            for (int y = ya; y < yb; y++) { u16 *q = (u16 *)(scr + y * SW + (x & ~1)); *q = (x & 1) ? (u16)((*q & 0xff) | (c << 8)) : (u16)((*q & 0xff00) | c); } }
        else { c = CDARK[x];
            for (int r = 1; r < 4; r++) { int y = (top * 256 + (s32)(h8 * r / 4) + 128) >> 8; if (y < 0 || y >= H) continue;
                u16 *q = (u16 *)(scr + y * SW + (x & ~1)); *q = (x & 1) ? (u16)((*q & 0xff) | (c << 8)) : (u16)((*q & 0xff00) | c); } }
    }
}
/* columna a columna de un sprite 16x27 escalado (como drawImage(img,sx,0,1,27,c,top,1,ceil(h))) */
IWRAM_CODE static void sprite_cols(u8 *scr, const u8 *src, int x0, s32 w8, int top, int hh, s32 ty) {
    if (hh <= 0) return;
    int c0 = x0 < 0 ? 0 : x0, wfl = (x0 * 256 + w8 + 255) >> 8, c1 = wfl > W ? W : wfl; /* c < x0 + w */
    s32 ystep = (27 << 16) / hh, xstep = (16 << 24) / w8; /* sx = floor((c-x0)/w*16) en 16.16 */
    int ya = top < 0 ? 0 : top, yb = top + hh > H ? H : top + hh;
    s32 sy0 = (ya - top) * ystep;
    for (int c = c0; c < c1; c++) {
        if (ty >= ZB[c]) continue;
        int sx = ((c - x0) * xstep) >> 16; if (sx > 15) sx = 15;
        const u8 *s = src + sx;
        u16 *d = (u16 *)(scr + ya * SW + (c & ~1)); s32 sy = sy0;
        /* pareja de columnas con el mismo texel: una escritura de 16 bits sin leer */
        if (!(c & 1) && c + 1 < c1 && ty < ZB[c + 1] && ((((c + 1 - x0) * xstep) >> 16) == sx || sx == 15)) {
            for (int y = ya; y < yb; y++, d += SW / 2, sy += ystep) { u32 v = s[(sy >> 16) * 16]; if (v) *d = v | (v << 8); }
            c++; continue;
        }
        if (c & 1) { for (int y = ya; y < yb; y++, d += SW / 2, sy += ystep) { u32 v = s[(sy >> 16) * 16]; if (v) *d = (*d & 0xff) | (v << 8); } }
        else { for (int y = ya; y < yb; y++, d += SW / 2, sy += ystep) { u32 v = s[(sy >> 16) * 16]; if (v) *d = (*d & 0xff00) | v; } }
    }
}

/* copia opaca de un bitmap (x y ancho pares) con escrituras de 16 bits, recortada al lienzo */
IWRAM_CODE static void blit16(u8 *scr, const bmp_t *b, int x, int y, int w, int h) {
    if (!b) return;
    for (int j = 0; j < h; j++) { int yy = y + j; if (yy < 0 || yy >= H) continue;
        u16 *d = (u16 *)(scr + yy * SW + x); const u8 *s = b->px + j * b->w;
        const u16 *s2 = (const u16 *)s; for (int i = 0; i < w / 2; i++) d[i] = s2[i]; }
}

typedef struct { s32 x, y, d; s8 q, p; } spr_t;

static void sh_draw(void) {
    u32 t0 = ticks();
    sh_draws++;
    { u32 d = frame_no - sh_lastf; sh_lastf = frame_no; if (sh_draws > 30 && (s32)d > sh_maxn && d < 10) sh_maxn = d; if (sh_draws > 30) { if (d == 1) sh_n1++; else if (d == 2) sh_n2++; else sh_n3++; } }
    float dx = fcos(sh_pa), dy = fsin(sh_pa), plx = -dy * .66f, ply = dx * .66f;
    u8 *scr = T.px + OY * SW + OX; /* el lienzo 192x144 en la página oculta (OX es par) */
    cast_cols((s32)(sh_px * 65536), (s32)(sh_py * 65536), (s32)(dx * 16384), (s32)(dy * 16384), (s32)(plx * 16384), (s32)(ply * 16384), scr);
    sh_ccast = ticks() - t0;
    /* sprites (personas y EPIs lanzados), del más lejano al más cercano — transformación en punto fijo:
       con plano = 0,66·perp(dir): ty = dir·r, tx = (dx·ry - dy·rx) / 0,66 */
    s32 pxf = (s32)(sh_px * 65536), pyf = (s32)(sh_py * 65536), dxi = (s32)(dx * 16384), dyi = (s32)(dy * 16384);
    spr_t sp[24]; int ns = 0;
    for (int j = 0; j < 5; j++) if (sh_people[j].alive) { sp[ns].x = (s32)(sh_people[j].x * 65536); sp[ns].y = (s32)(sh_people[j].y * 65536); sp[ns].q = j; sp[ns].p = -1; ns++; }
    for (int j = 0; j < sh_nproj; j++) { sp[ns].x = (s32)(sh_proj[j].x * 65536); sp[ns].y = (s32)(sh_proj[j].y * 65536); sp[ns].q = -1; sp[ns].p = j; ns++; }
    for (int i = 0; i < ns; i++) { s32 ax = (sp[i].x - pxf) >> 8, ay = (sp[i].y - pyf) >> 8; sp[i].d = ax * ax + ay * ay; }
    for (int i = 1; i < ns; i++) { spr_t v = sp[i]; int j = i; while (j > 0 && sp[j - 1].d < v.d) { sp[j] = sp[j - 1]; j--; } sp[j] = v; }
    for (int i = 0; i < ns; i++) {
        s32 rx = sp[i].x - pxf, ry = sp[i].y - pyf;
        s32 ty = (s32)(((s64)dxi * rx + (s64)dyi * ry) >> 14);
        s32 tx = (s32)((((s64)dxi * ry - (s64)dyi * rx) * 24824) >> 28); /* inv = -1/.66 */
        if (ty <= 9830) continue; /* ty <= .15 */
        s32 num = 192 * tx + ty, den = 2 * ty; /* Math.round(W/2*(1+tx/ty)) */
        int scx = 96 + (num >= 0 ? num / den : -((-num + den - 1) / den));
        u32 size8 = (144u << 24) / (u32)ty; /* size·256 */
        int zc = iclamp(scx, 0, W - 1);
        if (sp[i].q >= 0) {
            sh_person_t *q = &sh_people[sp[i].q];
            if (q->hired && ((q->t >> 2) & 1)) continue;
            bmp_t *img = person_bmp(q->look, q->gear, 0);
            s32 h8 = (s32)((size8 * 471u) >> 9), w8 = h8 * 16 / 27; /* h = size·.92, w = h·16/27 */
            int top = (72 * 256 + (s32)(size8 >> 1) - h8 + 128) >> 8, x0 = (scx * 256 - w8 / 2 + 128) >> 8, hh = (h8 + 255) >> 8;
            if (hh > 4 * H) hh = 4 * H;
            if (img && w8 > 128 && x0 < W && x0 * 256 + w8 > 0) sprite_cols(scr, img->px, x0, w8, top, hh, ty);
            if (!q->hired && ty < ZB[zc]) {
                int need[3], nn = 0; for (int k = 0; k < 3; k++) if (!(q->gear & (1 << k))) need[nn++] = k;
                int sc = iclamp((s32)((size8 + 20 * 256) / (40 * 256)), 1, 3);
                for (int k = 0; k < nn; k++) item_at(need[k], scx - nn * 4 * sc + k * 8 * sc, top - 8 * sc, sc);
            }
            if (q->hired && ty < ZB[zc]) arc_text(scx, imax(2, top - 12), "¡FLiiTS!", C(0x62d26f), 1, 8);
        } else {
            sh_proj_t *p = &sh_proj[sp[i].p]; int sc = imax(1, (s32)((size8 + 11 * 256) / (22 * 256)));
            if (ty < ZB[zc]) item_at(p->item, scx - 3 * sc, (72 * 256 + (s32)((size8 * 13) >> 8) + 128) >> 8, sc);
        }
    }
    sh_tspr = ticks() - t0;
    /* manos: la izquierda lleva la mochila FLiiTS; la derecha, el EPI que vas a lanzar */
    int bob = iround(fsin(sh_bob) * 2), th = sh_hand > 0 ? iround(fsin(sh_hand / 14.f * 3.14159265f) * 14) : 0;
    blit16(scr, FBXB, 6, H - 34 + bob, 42, 26);
    blit16(scr, HLB, 14, H - 40 + bob, 18, 8);
    int hx = 144 - th, hy = H - 28 - bob - th;
    if (!(hx & 1)) blit16(scr, HRB, hx - 2, hy + 8, 22, 24); else if (HRB) blit_part(HRB, 0, 0, 22, 24, hx - 2, hy + 8, 0);
    if (sh_hand <= 4 && ITBB[sh_item]) blit_part(ITBB[sh_item], 0, 0, 17, sh_item ? 14 : 12, hx + 1, hy - 6, 0); /* drawItem(...,2.4) */
    sh_last[4] = ticks() - t0;
    { u8 wc = C(0xffffff); u16 w2 = wc | (wc << 8); /* mira */
      u16 *r = (u16 *)(scr + (H / 2) * SW + W / 2 - 4); r[0] = (r[0] & 0xff) | (wc << 8); r[1] = w2; r[2] = w2; r[3] = w2; /* x 93..99 */
      for (int y = H / 2 - 3; y < H / 2 + 4; y++) { u16 *q = (u16 *)(scr + y * SW + W / 2); *q = (*q & 0xff00) | wc; } }
    blit16(scr, LBLB[sh_item], W / 2 - 34, H - 13, 68, 12);
    sh_last[5] = ticks() - t0;
    const char *txt = 0; char nb[64];
    if (sh_msgT > 0) txt = sh_msg;
    else if (sh_near >= 0) {
        sh_person_t *q = &sh_people[sh_near]; int k = 0; while (k < 3 && (q->gear & (1 << k))) k++;
        char *o = str_cat(nb, NPC_NAME[q->look]); o = str_cat(o, ": ¡ME FALTA "); o = str_cat(o, EPI_NAME[k < 3 ? k : 0]); str_cat(o, "!"); txt = nb;
    }
    if (txt && *txt && strcmp(txt, msg_key)) {
        strcpy(msg_key, txt); u32 tm0 = ticks();
        /* ajuste de línea por palabras como measureText: 8 px por carácter, ancho máximo W-12 */
        char lines[4][64]; int nl = 1; lines[0][0] = 0;
        const char *s = txt;
        while (*s) {
            while (*s == ' ') s++; if (!*s) break;
            const char *e = s; while (*e && *e != ' ') e++;
            char word[64]; int wl = e - s; if (wl > 63) wl = 63; memcpy(word, s, wl); word[wl] = 0; s = e;
            char t[130]; char *o = t; *o = 0; if (lines[nl - 1][0]) { o = str_cat(o, lines[nl - 1]); o = str_cat(o, " "); } str_cat(o, word);
            if (text_len(t) * 8 > W - 12 && nl < 4) strcpy(lines[nl++], word);
            else if (strlen(t) < 64) strcpy(lines[nl - 1], t);
        }
        msg_h = nl * 11 + 5; if (msg_h > 49) msg_h = 49;
        if (MSGBB) { tgt_bmp(MSGBB); cls(C(0x120b0c));
        for (int i = 0; i < nl; i++) arc_text(W / 2 - 2, 3 + i * 11, lines[i], C(0xffffff), 1, 8);
        tgt_back(); } sh_msgcyc = ticks() - tm0;
    }
    if (txt && *txt) blit16(scr, MSGBB, 2, 2, 188, msg_h);
    sh_cyc = ticks() - t0; sh_last[0] = sh_ccast; sh_last[1] = sh_tspr; sh_last[2] = sh_cyc; sh_sum[0] += sh_ccast >> 4; sh_sum[1] += (sh_tspr - sh_ccast) >> 4; sh_sum[2] += (sh_cyc - sh_tspr) >> 4; sh_sum[3]++; if (sh_draws > 30 && sh_cyc > sh_maxcyc) { sh_maxcyc = sh_cyc; sh_mxinfo[0] = sh_ccast; sh_mxinfo[1] = sh_tspr; sh_mxinfo[2] = ns; sh_mxinfo[3] = sh_msgT; }
}

const arc_def_t ARC_DEF3 = {"EPI SHOOTER", W, H, "↑ ↓: ANDAR · ← →: GIRAR · A: LANZAR EPI · B: CAMBIAR EPI. ¡Viste a 3 riders de FLiiTS!",
                            sh_init, sh_update, sh_draw, sh_press, 0};
