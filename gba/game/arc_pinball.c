/* IVAN · PINBALL DE CONTRATACIÓN (ARC_GAMES[0] del web).
   Mesa de 180x300 (física) dentro de una máquina de 204x368: marquesina con marcador LED arriba,
   luces laterales, tablero con el plano del Eixample y placa FLiiTS abajo.
   La máquina es más alta que la pantalla: la vista (MG.view_y) sigue a la bola. */
#include "arc_pinball.h"

extern bmp_t T;
extern volatile u32 vbl_count;

/* ================================================================ utilidades compartidas */
int arc_maxn;
extern int arcx_cx0, arcx_cx1, arcx_cy0, arcx_cy1;
static u32 ms_last;
void arcx_measure(void) {
    u32 t = (u32)MG.t; int d = (int)(t - ms_last); ms_last = t;
    if (d > 0 && d < 8 && t > 30 && d > arc_maxn) arc_maxn = d;
}
IWRAM_CODE void arcx_bgcopy(const bmp_t *b) {
    volatile u32 *dma = (volatile u32 *)0x040000D4;
    int x = OX, w = b->w;
    if (x < 0 || x + w > SW || !T.vram) return;
    for (int sy = 0; sy < SH; sy++) {
        int r = sy - OY; if (r < 0 || r >= b->h) continue;
        dma[0] = (u32)(b->px + r * w); dma[1] = (u32)(T.px + sy * SW + x); dma[2] = (u32)(w >> 1) | 0x80000000u;
    }
}
void arcx_lut(u8 *lut, u8 col, int k) { lut[0] = 0; for (int i = 1; i < 256; i++) lut[i] = col_mix((u8)i, col, k); }
void arcx_lut_px(int x, int y, const u8 *lut) { u8 c = pget(x, y); if (c) pset(x, y, lut[c]); }
void arcx_halfdisc(int cx, int cy, int r, u8 c) {
    for (int y = -r; y < 0; y++) { int v = r * r - y * y + r, hw = 0; while ((hw + 1) * (hw + 1) <= v) hw++; rect(cx - hw, cy + y, hw * 2 + 1, 1, c); }
}
void arcx_poly(const s16 *p, int n, u8 c) {
    int ymin = 9999, ymax = -9999;
    for (int i = 0; i < n; i++) { ymin = imin(ymin, p[i * 2 + 1]); ymax = imax(ymax, p[i * 2 + 1]); }
    { int sy0 = arcx_cy0 - OY, sy1 = arcx_cy1 - 1 - OY; if (T.vram) { if (ymin < sy0) ymin = sy0; if (ymax > sy1) ymax = sy1; } }
    for (int y = ymin; y <= ymax; y++) {
        int xs[8], k = 0, fy2 = 2 * y + 1;
        for (int i = 0; i < n && k < 8; i++) {
            int j = i + 1 == n ? 0 : i + 1, y0 = p[i * 2 + 1], y1 = p[j * 2 + 1], x0 = p[i * 2], x1 = p[j * 2];
            if ((fy2 >= 2 * y0 && fy2 < 2 * y1) || (fy2 >= 2 * y1 && fy2 < 2 * y0)) {
                int num = (fy2 - 2 * y0) * (x1 - x0), den = 2 * (y1 - y0);
                if (den < 0) { num = -num; den = -den; }
                xs[k++] = x0 + (num >= 0 ? (2 * num + den) / (2 * den) : -((-2 * num + den) / (2 * den)));
            }
        }
        for (int a = 1; a < k; a++) { int v = xs[a], b = a - 1; while (b >= 0 && xs[b] > v) { xs[b + 1] = xs[b]; b--; } xs[b + 1] = v; }
        for (int a = 0; a + 1 < k; a += 2) arcx_hline(xs[a], xs[a + 1], y, c);
    }
}
void arcx_octagon(int x, int y, int w, int ch, u8 c) { /* octógono w x w con chaflán ch */
    for (int r = 0; r < w; r++) { int in = r < ch ? ch - r : r > w - 1 - ch ? r - (w - 1 - ch) : 0; rect(x + in, y + r, w - 2 * in, 1, c); }
}
/* recorte propio para el dibujo rápido (coordenadas de pantalla); arcx_clip() lo toma del lienzo actual */
int arcx_cx0, arcx_cx1 = SW, arcx_cy0, arcx_cy1 = SH;
void arcx_clip(int x, int y, int w, int h) { arcx_cx0 = imax(0, x); arcx_cy0 = imax(0, y); arcx_cx1 = imin(SW, x + w); arcx_cy1 = imin(SH, y + h); }
/* tramo horizontal [x0, x1) en la fila y (con origen); en pantalla (VRAM) o en bitmap */
static inline __attribute__((always_inline)) void span(int x0, int x1, int y, u8 c) {
    x0 += OX; x1 += OX; y += OY;
    if (!T.vram) { if (y < 0 || y >= T.h) return; if (x0 < 0) x0 = 0; if (x1 > T.w) x1 = T.w; u8 *r = T.px + y * T.w; while (x0 < x1) r[x0++] = c; return; }
    if (y < arcx_cy0 || y >= arcx_cy1) return;
    if (x0 < arcx_cx0) x0 = arcx_cx0; if (x1 > arcx_cx1) x1 = arcx_cx1;
    if (x0 >= x1) return;
    u8 *row = T.px + y * SW; u16 c2 = c | (c << 8);
    if (x0 & 1) { volatile u16 *q = (u16 *)(row + x0 - 1); *q = (*q & 0x00FF) | (c << 8); x0++; }
    u16 *q = (u16 *)(row + x0);
    while (x0 + 2 <= x1) { *q++ = c2; x0 += 2; }
    if (x0 < x1) { volatile u16 *v = q; *v = (*v & 0xFF00) | c; }
}
IWRAM_CODE void arcx_hline(int x0, int x1, int y, u8 c) { span(x0, x1, y, c); }
IWRAM_CODE void arcx_rect(int x, int y, int w, int h, u8 c) { for (int j = 0; j < h; j++) span(x, x + w, y + j, c); }
/* blit con transparencia (0) rápido a la pantalla, por parejas de píxeles; recorta con arcx_clip */
IWRAM_CODE void arcx_blit(const bmp_t *b, int x, int y, int flip) {
    if (!b) return;
    if (!T.vram) {   /* a otro bitmap: copia simple con recorte al destino */
        x += OX; y += OY; int w = b->w, h = b->h;
        int i0 = x < 0 ? -x : 0, i1 = x + w > T.w ? T.w - x : w;
        for (int j = 0; j < h; j++) { int ty = y + j; if (ty < 0 || ty >= T.h) continue;
            const u8 *src = b->px + j * w; u8 *dst = T.px + ty * T.w + x;
            if (!flip) { for (int i = i0; i < i1; i++) { u8 c = src[i]; if (c) dst[i] = c; } }
            else { for (int i = i0; i < i1; i++) { u8 c = src[w - 1 - i]; if (c) dst[i] = c; } } }
        return;
    }
    x += OX; y += OY;
    int w = b->w, h = b->h, j0 = 0, j1 = h, i0 = 0, i1 = w;
    if (y < arcx_cy0) j0 = arcx_cy0 - y; if (y + h > arcx_cy1) j1 = arcx_cy1 - y;
    if (x < arcx_cx0) i0 = arcx_cx0 - x; if (x + w > arcx_cx1) i1 = arcx_cx1 - x;
    if (j0 >= j1 || i0 >= i1) return;
    for (int j = j0; j < j1; j++) {
        const u8 *src = b->px + j * w; u8 *row = T.px + (y + j) * SW;
        int i = i0;
        int tx = x + i;
        if (tx & 1) { u8 c = src[flip ? w - 1 - i : i]; if (c) { volatile u16 *q = (u16 *)(row + tx - 1); *q = (*q & 0x00FF) | (c << 8); } i++; tx++; }
        u16 *q = (u16 *)(row + tx);
        if (!flip) {
            for (; i + 1 < i1; i += 2, q++) {
                u32 a = src[i], c = src[i + 1];
                if (a && c) *q = a | (c << 8);
                else if (a) { volatile u16 *v = q; *v = (*v & 0xFF00) | a; }
                else if (c) { volatile u16 *v = q; *v = (*v & 0x00FF) | (c << 8); }
            }
            if (i < i1) { u32 a = src[i]; if (a) { volatile u16 *v = q; *v = (*v & 0xFF00) | a; } }
        } else {
            const u8 *s2 = src + w - 1;
            for (; i + 1 < i1; i += 2, q++) {
                u32 a = s2[-i], c = s2[-i - 1];
                if (a && c) *q = a | (c << 8);
                else if (a) { volatile u16 *v = q; *v = (*v & 0xFF00) | a; }
                else if (c) { volatile u16 *v = q; *v = (*v & 0x00FF) | (c << 8); }
            }
            if (i < i1) { u32 a = s2[-i]; if (a) { volatile u16 *v = q; *v = (*v & 0xFF00) | a; } }
        }
    }
}
/* copia opaca (DMA por filas) de un bitmap a la pantalla; x se redondea a par */
IWRAM_CODE void arcx_blit_opaque(const bmp_t *b, int x, int y) {
    if (!b) return;
    if (!T.vram) { blit(b, x, y, 0); return; }
    volatile u32 *dma = (volatile u32 *)0x040000D4;
    x += OX; y += OY; x &= ~1;
    int w = b->w, h = b->h, i0 = 0, i1 = w;
    if (x < arcx_cx0) i0 = (arcx_cx0 - x + 1) & ~1; if (x + w > arcx_cx1) i1 = (arcx_cx1 - x) & ~1;
    if (i1 <= i0) return;
    for (int j = 0; j < h; j++) {
        int ty = y + j; if (ty < arcx_cy0 || ty >= arcx_cy1) continue;
        const u8 *src = b->px + j * w + i0;
        if ((u32)src & 1) continue; /* (no pasa: anchos pares) */
        dma[0] = (u32)src; dma[1] = (u32)(T.px + ty * SW + x + i0); dma[2] = (u32)((i1 - i0) >> 1) | 0x80000000u;
    }
}
/* disco relleno rápido (enteros, sin raíz): mismo contorno que circle() del motor */
IWRAM_CODE void arcx_disc(int cx, int cy, int r, u8 c) {
    if (r < 0) return;
    int sy = cy + OY; if (T.vram && (sy + r < arcx_cy0 || sy - r >= arcx_cy1)) return;
    int hw = r, v0 = r * r + r;
    for (int y = 0; y <= r; y++) {
        int v = v0 - y * y; while (hw > 0 && hw * hw > v) hw--;
        span(cx - hw, cx + hw + 1, cy + y, c); if (y) span(cx - hw, cx + hw + 1, cy - y, c);
    }
}
/* línea de 1 px (DDA en punto fijo; los píxeles seguidos de una fila salen en un solo tramo) */
IWRAM_CODE void arcx_line(int x0, int y0, int x1, int y1, u8 c) {
    int dx = x1 - x0, dy = y1 - y0, n = imax(iabs(dx), iabs(dy));
    if (!n) { span(x0, x0 + 1, y0, c); return; }
    int sx = (dx << 16) / n, sy = (dy << 16) / n, x = (x0 << 16) + 0x8000, y = (y0 << 16) + 0x8000;
    int ry = y >> 16, ra = x >> 16, rb = ra;
    for (int k = 1; k <= n; k++) {
        x += sx; y += sy; int px = x >> 16, py = y >> 16;
        if (py == ry) { if (px < ra) ra = px; if (px > rb) rb = px; }
        else { span(ra, rb + 1, ry, c); ry = py; ra = rb = px; }
    }
    span(ra, rb + 1, ry, c);
}
/* trazo grueso con extremos redondeados (lineCap round), una fila = un tramo.
   Coordenadas x256. Rectángulo girado (convexo) + discos de radio ri en los extremos. */
IWRAM_CODE static void capsule_fx(int ax, int ay, int bx, int by, int nx, int ny, int ri, u8 c) {
    int P[4][2] = {{ax + nx, ay + ny}, {bx + nx, by + ny}, {bx - nx, by - ny}, {ax - nx, ay - ny}};
    int cax = (ax + 128) >> 8, cay = (ay + 128) >> 8, cbx = (bx + 128) >> 8, cby = (by + 128) >> 8;
    int y0 = imin(cay, cby) - ri, y1 = imax(cay, cby) + ri, hwt[12];
    for (int k = 0; k < 4; k++) { y0 = imin(y0, P[k][1] >> 8); y1 = imax(y1, (P[k][1] >> 8) + 1); }
    { int hw = ri, v0 = ri * ri + ri; for (int y = 0; y <= ri && y < 12; y++) { int v = v0 - y * y; while (hw > 0 && hw * hw > v) hw--; hwt[y] = hw; } }
    int sl[4];
    for (int k = 0; k < 4; k++) { int j = (k + 1) & 3, d = P[j][1] - P[k][1]; sl[k] = d ? ((P[j][0] - P[k][0]) << 16) / d : 0; }
    for (int y = y0; y <= y1; y++) {
        int lo = 99999, hi = -99999, yc = y * 256 + 128;
        for (int k = 0; k < 4; k++) {
            int j = (k + 1) & 3, ya = P[k][1], yb = P[j][1];
            if ((yc >= ya && yc < yb) || (yc >= yb && yc < ya)) {
                int xx = P[k][0] + (int)(((int64_t)(yc - ya) * sl[k]) >> 16);
                int px = (xx - 128 + 255) >> 8; if (px < lo) lo = px;   /* ceil(xx/256-.5) */
                px = (xx - 128) >> 8; if (px > hi) hi = px;
            }
        }
        int d = y - cay; if (d < 0) d = -d; if (d <= ri) { lo = imin(lo, cax - hwt[d]); hi = imax(hi, cax + hwt[d]); }
        d = y - cby; if (d < 0) d = -d; if (d <= ri) { lo = imin(lo, cbx - hwt[d]); hi = imax(hi, cbx + hwt[d]); }
        if (hi >= lo) span(lo, hi + 1, y, c);
    }
}
void arcx_capsule(float x0, float y0, float x1, float y1, float r, u8 c) {
    float dx = x1 - x0, dy = y1 - y0, L = fsqrt(dx * dx + dy * dy);
    int ri = (int)(r - 0.4f); if (ri < 0) ri = 0;
    int nx = 0, ny = 0; if (L > 0.01f) { nx = (int)(-dy / L * r * 256); ny = (int)(dx / L * r * 256); }
    capsule_fx((int)(x0 * 256), (int)(y0 * 256), (int)(x1 * 256), (int)(y1 * 256), nx, ny, ri, c);
}
void arcx_text_outline(int x, int y, const char *s, u8 col, u8 out) {
    for (int dy = -1; dy <= 1; dy++) for (int dx = -1; dx <= 1; dx++) if (dx || dy) text(x + dx, y + dy, s, out, 1);
    text(x, y, s, col, 1);
}

/* ================================================================ pinball */
#define PB_OX 12
#define PB_OY 40
#define FL_LEN 24
#define PW 204
#define PH 368
static const float FL_X[2] = {50, 122}, FL_Y[2] = {256, 256};
static const float FL_REST[2] = {.5f, 3.14159265f - .5f}, FL_UP[2] = {-.45f, 3.14159265f + .45f};
static const s16 SEGS[15][4] = {{10,40,40,10},{40,10,140,10},{140,10,172,42},{10,40,10,228},{10,228,50,256},{156,58,156,300},{172,42,172,300},{156,228,122,256},
    {22,198,22,226},{22,226,42,238},{134,198,134,226},{134,226,114,238},{66,16,66,30},{90,16,90,30},{114,16,114,30}};
static const s16 SL_PTS[2][6] = {{22,198,22,226,42,238},{134,198,134,226,114,238}};
static const s16 SL_FACE[2][4] = {{22,198,42,238},{134,198,114,238}};
static const s16 BUMP[3][3] = {{60,84,8},{118,84,8},{89,122,9}};
static const char *const NPC_NAME[6] = {"AAMIR", "BILAL", "HAMZA", "IMRAN", "FARHAN", "SAIRA"};

typedef struct { float x, y, bx; int ph, look, hired, say; } pb_person_t;
typedef struct {
    int balls, score; float plP; int plDir, ready, shot; float flA[2], flW[2]; int flick, flash[3], sfl[2], lanes[3];
    pb_person_t people[3];
    s32 bx, by, vx, vy;    /* bola en punto fijo Q16 (x65536) */
    float view;
} pb_t;
#define FX(v) ((s32)((v) * 65536.0f))
#define FXI(v) ((s32)(v) << 16)
pb_t pb_G;                 /* estado (global para que el bot de pruebas lo lea) */
static bmp_t *pb_bg, *pb_helm[4], *pb_flip[20], *pb_bmp[2][8][2], *pb_scr;
static s8 pb_fdx[20], pb_fdy[20], pb_fdxm[20];
static bmp_t *pb_bub[2], *pb_slw[2];
static u32 pb_bake_end;
static s16 seg_box[15][4];
static s32 seg_inv[15];

static inline __attribute__((always_inline)) s32 fxmul(s32 a, s32 b) { return (s32)(((int64_t)a * b) >> 16); }
IWRAM_CODE static u32 isqrt32(u32 v) {
    u32 r = 0, b = 1u << 30;
    while (b > v) b >>= 2;
    while (b) { if (v >= r + b) { v -= r + b; r = (r >> 1) + b; } else r >>= 1; b >>= 2; }
    return r;
}
static s32 hn_x, hn_y;   /* normal del último choque (Q16) */
/* pinHit del web en Q16. inv = 2^30 / L2 (L2 en px²). R = 3 + rad. e1 = 1 + e. sv = velocidad de la superficie */
IWRAM_CODE static int pin_hit(s32 x1, s32 y1, s32 dx, s32 dy, s32 inv, s32 R, s32 e1, s32 svx, s32 svy) {
    pb_t *G = &pb_G;
    int64_t dot = ((int64_t)(G->bx - x1) * dx + (int64_t)(G->by - y1) * dy) >> 16;
    s32 t = (s32)((dot * inv) >> 30); if (t < 0) t = 0; if (t > 65536) t = 65536;
    s32 cx = x1 + fxmul(dx, t), cy = y1 + fxmul(dy, t), nx = G->bx - cx, ny = G->by - cy;
    if (nx >= R || nx <= -R || ny >= R || ny <= -R) return 0;
    s32 d2 = fxmul(nx, nx) + fxmul(ny, ny);
    if (d2 >= fxmul(R, R) || d2 == 0) return 0;
    s32 d = (s32)isqrt32((u32)d2 << 8); if (d <= 0) return 0;   /* Q12 */
    nx = (nx << 12) / d; ny = (ny << 12) / d;                    /* normal unitaria Q16 */
    G->bx = cx + fxmul(nx, R); G->by = cy + fxmul(ny, R);
    s32 rvx = G->vx - svx, rvy = G->vy - svy, vn = fxmul(rvx, nx) + fxmul(rvy, ny);
    if (vn < 0) { s32 k = fxmul(e1, vn); G->vx = rvx - fxmul(k, nx) + svx; G->vy = rvy - fxmul(k, ny) + svy; hn_x = nx; hn_y = ny; return 1; }
    return 0;
}
static inline int near_box(const s16 *b, int x, int y) { return x >= b[0] && x <= b[2] && y >= b[1] && y <= b[3]; }

static void drawHelmet(int x, int y, u8 col) {
    rect(x + 1, y, 10, 1, C(0x1b1212)); rect(x, y + 1, 12, 7, C(0x1b1212)); rect(x + 1, y + 1, 10, 6, col); rect(x + 2, y + 2, 3, 1, C(0xffffff));
    rect(x + 6, y + 5, 7, 2, C(0x1b1212)); rect(x + 7, y + 5, 5, 1, C(0x9fd8ff));
}
static void drawFood(int k, int x, int y) {
#define F(a, b, w, h, c) rect(x + (a), y + (b), w, h, c)
    if (k == 0) { F(0,0,9,2,C(0xc9971a)); F(1,2,7,2,C(0xffd23f)); F(2,4,5,2,C(0xffd23f)); F(3,6,3,2,C(0xffd23f)); F(2,2,2,1,C(0xd62839)); F(5,4,1,1,C(0xd62839)); }
    else if (k == 1) { F(1,0,7,3,C(0xc9971a)); F(0,3,9,1,C(0x62d26f)); F(0,4,9,2,C(0x7d4f2f)); F(1,6,7,2,C(0xc9971a)); }
    else if (k == 2) { F(2,0,5,1,C(0xffffff)); F(1,1,7,7,C(0xd62839)); F(2,3,5,2,C(0xffffff)); F(4,-2,1,3,C(0xffd23f)); }
    else { F(0,2,9,5,C(0xffffff)); F(0,4,9,2,C(0x1b1212)); F(2,1,5,2,C(0xff8a5c)); }
#undef F
}
static void seg7(int n, int x, int y) {
    static const u8 S[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F}; /* bits a..g */
    static const s8 R[7][4] = {{1,0,5,1},{6,1,1,5},{6,7,1,5},{1,12,5,1},{0,7,1,5},{0,1,1,5},{1,6,5,1}};
    int d[6]; for (int i = 5; i >= 0; i--) { d[i] = n % 10; n /= 10; }
    for (int i = 0; i < 6; i++) { int X = x + i * 10;
        for (int k = 0; k < 7; k++) rect(X + R[k][0], y + R[k][1], R[k][2], R[k][3], (S[d[i]] >> k) & 1 ? C(0xff2a2a) : C(0x3a0b0b)); }
}
static void arcx_ellipse93(int cx, int cy, u8 c) { /* ellipse(cx,cy,9,3) */
    static const u8 HW[4] = {9, 8, 6, 0}; /* filas 0..3 */
    for (int y = -3; y <= 3; y++) { int hw = HW[iabs(y)]; if (iabs(y) == 3) hw = 0; arcx_hline(cx - hw, cx + hw + 1, cy + y, c); }
}
static void sling_draw(int i, u8 fill) {
    arcx_poly(SL_PTS[i], 3, fill);
    for (int k = 0; k < 3; k++) { int j = (k + 1) % 3; line(SL_PTS[i][k * 2], SL_PTS[i][k * 2 + 1], SL_PTS[i][j * 2], SL_PTS[i][j * 2 + 1], C(0xffd1e3), 1); }
}

/* fondo estático de la máquina (todo lo que no se mueve) */

static void dash_prep(void);
static void pb_bake(void) {
    pb_scr = 0;
    pb_bg = bmp_new(PW, PH); tgt_bmp(pb_bg);
    const int W = PW, H = PH;
    rect(0, 0, W, H, C(0xd0102c)); rect(0, 0, W, 3, C(0xffd23f)); rect(0, H - 3, W, 3, C(0xffd23f));
    rect(8, 4, W - 16, 34, C(0x120b0c)); rect(10, 6, W - 20, 30, C(0x2a0b10));
    rect(14, 9, 66, 15, C(0x000000));
    text_small(140 + 1, 27 + 1, "FLiiTS PINBALL", C(0x000000), 1); text_small(140, 27, "FLiiTS PINBALL", C(0xffd23f), 1);
    /* tablero */
    origin(PB_OX, PB_OY);
    rect(0, 0, 180, 300, C(0x141a3a));
    for (int y = -6; y < 300; y += 26) for (int x = -6; x < 170; x += 26) {
        arcx_octagon(x, y, 23, 5, C(0x2c357a)); arcx_octagon(x + 1, y + 1, 21, 4, C(0x1d2452));
    }
    { const char *s = "BARCELONA"; text(84, 140, s, C(0x3e3b3e), 1); }
    rect(157, 56, 15, 250, C(0x0e1230));
    /* recorte del contorno de la mesa */
    for (int y = 0; y < 300; y++) {
        int xl = y < 10 ? 999 : y < 40 ? 50 - y : 10, xr = y < 10 ? 999 : y < 42 ? 130 + y : 172;
        if (y < 10) { rect(0, y, 180, 1, C(0xd0102c)); continue; }
        rect(0, y, xl, 1, C(0xd0102c)); rect(xr, y, 180 - xr, 1, C(0xd0102c));
    }
    /* guías de plástico */
    for (int i = 0; i < 15; i++) line(SEGS[i][0] - 2, SEGS[i][1] - 2, SEGS[i][2] - 2, SEGS[i][3] - 2, C(0x535e7f), 4);
    for (int i = 0; i < 15; i++) line(SEGS[i][0] - 1, SEGS[i][1] - 1, SEGS[i][2] - 1, SEGS[i][3] - 1, C(0xe8f0ff), 2);
    sling_draw(0, C(0xff7aa8)); sling_draw(1, C(0xff7aa8));
    origin(0, 0);
    /* placa inferior */
    rect(8, H - 34, W - 16, 30, C(0xffd23f)); rect(8, H - 6, W - 16, 2, C(0xc9971a));
    arcx_text_outline(W / 2, H - 31, "FLiiTS", C(0xd62839), C(0x1b1212));
    tgt_back();
    bmp_t *fd = bmp_new(9, 10);
    for (int k = 0; k < 4; k++) {
        memset(fd->px, 0, 92); tgt_bmp(fd); drawFood(k, 0, 2); tgt_back();
        tgt_bmp(pb_bg); blit_scaled(fd, 18 + k * 48, H - 19 - 4, 16, 18, 0); tgt_back();
    }
    /* cascos de la marquesina (escala 1,6) */
    bmp_t *hb = bmp_new(13, 8);
    for (int i = 0; i < 4; i++) {
        u8 col = i == 0 ? C(0xd62839) : i == 1 ? C(0xffd23f) : i == 2 ? C(0x58c4ff) : C(0x62d26f);
        memset(hb->px, 0, 104); tgt_bmp(hb); drawHelmet(0, 0, col); tgt_back();
        pb_helm[i] = bmp_new(21, 13); tgt_bmp(pb_helm[i]); blit_scaled(hb, 0, 0, 21, 13, 0); tgt_back();
    }
    /* flippers pre-dibujados: 20 ángulos del izquierdo (el derecho es su espejo); pivote en (5,15) */
    for (int k = 0; k < 20; k++) {
        float a = -.45f + .95f * k / 19, tx = 5 + fcos(a) * FL_LEN, ty = 15 + fsin(a) * FL_LEN;
        if (!pb_scr) pb_scr = bmp_new(34, 32);
        memset(pb_scr->px, 0, 34 * 32); tgt_bmp(pb_scr);
        arcx_capsule(5, 15, tx, ty, 4.5f, C(0x0b2a6a)); arcx_capsule(5, 15, tx, ty, 3, C(0x2f6fe0));
        arcx_line(5, 13, (int)(tx + .5f), (int)(ty + .5f) - 2, C(0x9fd0ff));
        arcx_disc(5, 15, 2, C(0xffd23f)); tgt_back();
        int x0 = 34, x1 = -1, y0 = 32, y1 = -1;
        for (int y = 0; y < 32; y++) for (int x = 0; x < 34; x++) if (pb_scr->px[y * 34 + x]) { x0 = imin(x0, x); x1 = imax(x1, x); y0 = imin(y0, y); y1 = imax(y1, y); }
        bmp_t *b = pb_flip[k] = bmp_new(x1 - x0 + 1, y1 - y0 + 1);
        for (int y = y0; y <= y1; y++) memcpy(b->px + (y - y0) * b->w, pb_scr->px + y * 34 + x0, b->w);
        pb_fdx[k] = x0 - 5; pb_fdy[k] = y0 - 15; pb_fdxm[k] = 5 - x1; /* espejo: pivote → borde izquierdo */
    }
    /* bumpers pre-dibujados: radio 8/9 · 8 giros de los radios · encendido */
    for (int q = 0; q < 2; q++) for (int k = 0; k < 8; k++) for (int f = 0; f < 2; f++) {
        int r = 8 + q, w = 2 * r + 7, h = 2 * r + 9, x = r + 3, y = r + 3;
        bmp_t *b = pb_bmp[q][k][f] = bmp_new(w, h); tgt_bmp(b);
        arcx_disc(x, y + 2, r + 3, C(0x1b1212)); arcx_disc(x, y, r + 2, f ? C(0xffffff) : C(0xd62839)); arcx_disc(x, y, r - 1, C(0x2f6fe0));
        for (int j = 0; j < 6; j++) { int a = (j * 1024 / 6 + k * 1024 / 48) & 1023; arcx_line(x, y, x + (icos(a) * (r - 2) + 2048) / 4096, y + (isin(a) * (r - 2) + 2048) / 4096, C(0x9fd0ff)); }
        arcx_disc(x, y, 1, C(0xffffff)); tgt_back();
    }
    dash_prep();
    /* rebotadores encendidos (blancos) pre-dibujados */
    for (int i = 0; i < 2; i++) {
        int x0 = imin(SL_PTS[i][0], SL_PTS[i][4]);
        pb_slw[i] = bmp_new(22, 42); tgt_bmp(pb_slw[i]); origin(-x0, -197); sling_draw(i, C(0xffffff)); tgt_back();
    }
    /* bocadillos (arcBubble) pre-dibujados, ancho par */
    for (int k = 0; k < 2; k++) {
        const char *tx = k ? "¡CONTRATADO!" : "¡BUSCO CURRO!"; u8 col = k ? C(0x62d26f) : C(0xffffff);
        int w = (text_len(tx) * 8 + 8 + 1) & ~1;
        pb_bub[k] = bmp_new(w, 13); tgt_bmp(pb_bub[k]);
        rect(0, 0, w, 13, C(0x120b0c)); frame_rect(0, 0, w, 13, col); text(4, 3, tx, col, 0); tgt_back();
    }
    /* caras de los aspirantes (personCv) ya cacheadas para no parar el juego al contratar */
    for (int i = 0; i < 3; i++) { person_bmp(i * 2 + 1, 0, 0); person_bmp(i * 2 + 1, G_CASCO | G_CHAQ | G_GUANT, 0); }
    /* cajas de colisión de las guías (con margen) */
    for (int i = 0; i < 15; i++) {
        seg_box[i][0] = imin(SEGS[i][0], SEGS[i][2]) - 6; seg_box[i][1] = imin(SEGS[i][1], SEGS[i][3]) - 6;
        seg_box[i][2] = imax(SEGS[i][0], SEGS[i][2]) + 6; seg_box[i][3] = imax(SEGS[i][1], SEGS[i][3]) + 6;
        int dx = SEGS[i][2] - SEGS[i][0], dy = SEGS[i][3] - SEGS[i][1]; seg_inv[i] = (s32)((1u << 30) / (u32)(dx * dx + dy * dy));
    }
}

static void pb_init(void) {
    pb_t *G = &pb_G; memset(G, 0, sizeof *G);
    if (!pb_bg || (pool_mark() & 0xFFFFFF) < pb_bake_end) { pb_bake(); pb_bake_end = pool_mark() & 0xFFFFFF; }
    G->balls = 5; G->plDir = 1; G->flA[0] = FL_REST[0]; G->flA[1] = FL_REST[1];
    static const s16 PP[3][2] = {{40,170},{90,186},{134,168}};
    for (int i = 0; i < 3; i++) { pb_person_t *p = &G->people[i]; p->x = p->bx = PP[i][0]; p->y = PP[i][1]; p->ph = i; p->look = (i * 2 + 1) % 6; }
    G->bx = FXI(164); G->by = FXI(282); G->view = PH - SH; MG.view_y = PH - SH;
}

static void pb_press(int b) {
    pb_t *G = &pb_G;
    if (b == B_A && G->ready) { G->vy = -FX(7.6f + G->plP * 3.4f); G->ready = 0; G->shot = 12; slide(200, 700, 250); }
    else if (b == B_A || b == B_B) { G->flick = 9; tone(300, 40, 0, W_SQUARE, 40); }
}

/* 6 subpasos de la física (en IWRAM, punto fijo) */
IWRAM_CODE static void pb_physics(s32 fl[2][4], s32 fw[2]) {
    pb_t *G = &pb_G;
    for (int s = 0; s < 6; s++) {
        G->vy += 1201; G->bx += G->vx / 6; G->by += G->vy / 6;   /* gravedad .11/6 */
        int ix = G->bx >> 16, iy = G->by >> 16;
        for (int i = 0; i < 15; i++) if (near_box(seg_box[i], ix, iy))
            pin_hit(FXI(SEGS[i][0]), FXI(SEGS[i][1]), FXI(SEGS[i][2] - SEGS[i][0]), FXI(SEGS[i][3] - SEGS[i][1]), seg_inv[i], FXI(4), FX(1.45f), 0, 0);
        if (iy >= 190 && iy <= 246) for (int i = 0; i < 2; i++) {
            int dx = SL_FACE[i][2] - SL_FACE[i][0], dy = SL_FACE[i][3] - SL_FACE[i][1];
            if (pin_hit(FXI(SL_FACE[i][0]), FXI(SL_FACE[i][1]), FXI(dx), FXI(dy), (s32)((1u << 30) / (u32)(dx * dx + dy * dy)), FXI(4), FX(1.6f), 0, 0)) {
                G->vx += fxmul(hn_x, FX(2.4f)); G->vy += fxmul(hn_y, FX(2.4f));
                if (!G->sfl[i]) { tone(520, 40, 0, W_SQUARE, 50); G->score += 50; }
                G->sfl[i] = 8;
            }
        }
        if (G->by >= FXI(282) && G->bx > FXI(157)) { G->by = FXI(282); G->vy = 0; G->vx = 0; }
        for (int i = 0; i < 3; i++) {
            s32 x = FXI(BUMP[i][0]), y = FXI(BUMP[i][1]), R = FXI(BUMP[i][2] + 3), nx = G->bx - x, ny = G->by - y;
            if (nx >= R || nx <= -R || ny >= R || ny <= -R) continue;
            s32 d2 = fxmul(nx, nx) + fxmul(ny, ny);
            if (d2 < fxmul(R, R) && d2 > 0) {
                s32 d = (s32)isqrt32((u32)d2 << 8); if (d <= 0) continue;
                nx = (nx << 12) / d; ny = (ny << 12) / d; G->bx = x + fxmul(nx, R); G->by = y + fxmul(ny, R);
                s32 vn = fxmul(G->vx, nx) + fxmul(G->vy, ny); if (vn < 0) { G->vx -= 2 * fxmul(vn, nx); G->vy -= 2 * fxmul(vn, ny); }
                G->vx += fxmul(nx, FX(1.9f)); G->vy += fxmul(ny, FX(1.9f));
                if (!G->flash[i]) { tone(660 + i * 120, 50, 0, W_SQUARE, 50); G->score += 100; }
                G->flash[i] = 8;
            }
        }
        for (int i = 0; i < 2; i++) {
            s32 fx = fl[i][0], fy = fl[i][1], dx = fl[i][2], dy = fl[i][3], tx = fx + dx, ty = fy + dy;
            s32 mnx = (fx < tx ? fx : tx) - FXI(8), mxx = (fx > tx ? fx : tx) + FXI(8), mny = (fy < ty ? fy : ty) - FXI(8), mxy = (fy > ty ? fy : ty) + FXI(8);
            if (G->bx < mnx || G->bx > mxx || G->by < mny || G->by > mxy) continue;
            int64_t dot = ((int64_t)(G->bx - fx) * dx + (int64_t)(G->by - fy) * dy) >> 16;
            s32 t = (s32)((dot * ((1 << 30) / (FL_LEN * FL_LEN))) >> 30); if (t < 0) t = 0; if (t > 65536) t = 65536;
            s32 cx = fx + fxmul(dx, t), cy = fy + fxmul(dy, t), w = fw[i];
            pin_hit(fx, fy, dx, dy, (1 << 30) / (FL_LEN * FL_LEN), FXI(6), FX(1.25f), -fxmul(w, cy - fy), fxmul(w, cx - fx));
        }
        for (int i = 0; i < 3; i++) {
            pb_person_t *p = &G->people[i]; if (p->hired) continue;
            s32 px = FX(p->x), py = FXI((int)p->y);
            s32 cx = G->bx < px - FXI(6) ? px - FXI(6) : G->bx > px + FXI(6) ? px + FXI(6) : G->bx;
            s32 cy = G->by < py - FXI(11) ? py - FXI(11) : G->by > py + FXI(11) ? py + FXI(11) : G->by;
            s32 nx = G->bx - cx, ny = G->by - cy;
            if (nx >= FXI(3) || nx <= -FXI(3) || ny >= FXI(3) || ny <= -FXI(3)) continue;
            s32 d2 = fxmul(nx, nx) + fxmul(ny, ny);
            if (d2 < FXI(9)) {
                if (d2 > 0) { s32 d = (s32)isqrt32((u32)d2 << 8); if (d > 0) { nx = (nx << 12) / d; ny = (ny << 12) / d; } else { nx = 0; ny = 65536; } } else { nx = 0; ny = 65536; }
                G->bx = cx + nx * 3; G->by = cy + ny * 3;
                s32 vn = fxmul(G->vx, nx) + fxmul(G->vy, ny); if (vn < 0) { G->vx -= fxmul(fxmul(FX(1.7f), vn), nx); G->vy -= fxmul(fxmul(FX(1.7f), vn), ny); }
                p->hired = 1; p->say = 90; MG.c1++; G->score += 1000; arc_hire(NPC_NAME[p->look]);
            }
        }
    }
    static const s16 LX[3] = {54, 78, 102};
    for (int i = 0; i < 3; i++) {
        s32 dx = G->bx - FXI(LX[i]);
        if (G->by > FXI(14) && G->by < FXI(32) && dx < FXI(9) && dx > -FXI(9) && !G->lanes[i]) {
            G->lanes[i] = 1; G->score += 200; tone(990, 50, 0, W_SQUARE, 60);
            if (G->lanes[0] && G->lanes[1] && G->lanes[2]) { G->score += 2000; G->lanes[0] = G->lanes[1] = G->lanes[2] = 0; arc_banner("¡RUTA COMPLETA!", "+2000", 50); }
        }
    }
    s32 sp2 = fxmul(G->vx, G->vx) + fxmul(G->vy, G->vy);
    if (sp2 > FXI(64)) { u32 sp = isqrt32((u32)sp2 << 8); if (sp) { s32 k = (s32)((8u << 28) / sp); G->vx = fxmul(G->vx, k); G->vy = fxmul(G->vy, k); } }
}
u32 pb_upd_ticks, pb_upd_max;
static void pb_update_(void);
static void pb_update(void) { u32 t0 = ticks(); pb_update_(); pb_upd_ticks = ticks() - t0; if (pb_upd_ticks > pb_upd_max) pb_upd_max = pb_upd_ticks; }
static void pb_update_(void) {
    pb_t *G = &pb_G;
    int L = (key_held & (K_LEFT | K_L)) || G->flick > 0, R = (key_held & (K_RIGHT | K_R)) || G->flick > 0;
    if (G->flick > 0) G->flick--; if (G->shot > 0) G->shot--;
    for (int i = 0; i < 2; i++) {
        float tgt = (i ? R : L) ? FL_UP[i] : FL_REST[i], d = tgt - G->flA[i];
        float st = d > 0 ? (d < .32f ? d : .32f) : (d > -.32f ? d : -.32f);
        G->flW[i] = st; G->flA[i] += st;
    }
    G->ready = G->bx > FXI(157) && G->by >= FXI(279) && G->vy < FX(.8f) && G->vy > -FX(.8f);
    if (G->ready) {
        G->by = FXI(282); G->vx = G->vy = 0; G->plP += .016f * G->plDir;
        if (G->plP >= 1 || G->plP <= 0) { G->plDir = -G->plDir; G->plP = fclamp(G->plP, 0, 1); }
    } else {
        s32 fl[2][4];   /* x, y, dx, dy del flipper (Q16) */
        s32 fw[2];
        for (int i = 0; i < 2; i++) { fl[i][0] = FXI((int)FL_X[i]); fl[i][1] = FXI((int)FL_Y[i]); fl[i][2] = FX(fcos(G->flA[i]) * FL_LEN); fl[i][3] = FX(fsin(G->flA[i]) * FL_LEN); fw[i] = FX(G->flW[i]); }
        pb_physics(fl, fw);
    }
    for (int i = 0; i < 3; i++) if (G->flash[i]) G->flash[i]--;
    for (int i = 0; i < 2; i++) if (G->sfl[i]) G->sfl[i]--;
    for (int i = 0; i < 3; i++) { pb_person_t *p = &G->people[i]; if (!p->hired) p->x = p->bx + fsin((MG.t + p->ph * 60) / 45.0f) * 10; if (p->say > 0) p->say--; }
    /* vista que sigue a la bola */
    { float tgt = (G->by >> 16) + PB_OY - 92; tgt = fclamp(tgt, 0, PH - SH); G->view += (tgt - G->view) * .22f; MG.view_y = (int)(G->view + .5f); }
    if (G->by > FXI(306)) {
        G->balls--; sfx_back();
        if (G->balls <= 0) { arc_lose("¡TE HAS QUEDADO SIN BOLAS!"); return; }
        char b[16], *q = str_cat(b, "QUEDAN "); str_num(q, G->balls);
        arc_banner("BOLA PERDIDA", b, 60); G->bx = FXI(164); G->by = FXI(282); G->vx = G->vy = 0;
    }
}

/* raya roja discontinua animada (ruta del rider) */
/* raya roja discontinua animada (ruta del rider): 364 posiciones precalculadas con la máscara de los
   4 píxeles (2x2) que caen sobre el plano (bajo las guías no se pinta, como en el web) */
typedef struct { s16 x, y; u8 m; } dash_pt;
static dash_pt DASH[364] EWRAM_BSS;
static int dash_n;
static void dash_prep(void) {
    static const s16 P[5][2] = {{20,280},{20,150},{72,150},{72,46},{150,46}};
    dash_n = 0;
    for (int k = 0; k < 4; k++) {
        int x0 = P[k][0], y0 = P[k][1], x1 = P[k + 1][0], y1 = P[k + 1][1];
        int sx = x1 > x0 ? 1 : x1 < x0 ? -1 : 0, sy = y1 > y0 ? 1 : y1 < y0 ? -1 : 0, n = iabs(x1 - x0) + iabs(y1 - y0);
        for (int i = 0; i < n && dash_n < 364; i++) {
            int x = x0 + sx * i, y = y0 + sy * i, m = 0;
            for (int q = 0; q < 4; q++) {
                int px = x - 1 + (q & 1), py = y - 1 + (q >> 1);
                u8 c = pb_bg->px[(py + PB_OY) * PW + px + PB_OX];
                if (c == C(0x141a3a) || c == C(0x1d2452) || c == C(0x2c357a)) m |= 1 << q;
            }
            DASH[dash_n].x = x; DASH[dash_n].y = y; DASH[dash_n].m = m; dash_n++;
        }
    }
}
IWRAM_CODE static void dash_path(int t) {
    int ph = (((-(t / 4)) % 7) + 7) % 7, vy0 = MG.view_y - PB_OY - 2, vy1 = vy0 + SH + 4;
    for (int i = 0; i < dash_n; i++, ph = ph == 6 ? 0 : ph + 1) {
        if (ph >= 4) continue;
        const dash_pt *d = &DASH[i]; int y = d->y, x = d->x, m = d->m;
        if (y < vy0 || y > vy1 || !m) continue;
        if ((m & 3) == 3) span(x - 1, x + 1, y - 1, C(0x832644)); else if (m & 1) span(x - 1, x, y - 1, C(0x832644)); else if (m & 2) span(x, x + 1, y - 1, C(0x832644));
        if ((m & 12) == 12) span(x - 1, x + 1, y, C(0x832644)); else if (m & 4) span(x - 1, x, y, C(0x832644)); else if (m & 8) span(x, x + 1, y, C(0x832644));
    }
}
IWRAM_CODE static void pb_lights(int t, int vy) {
    static const u8 LC[4] = {C(0x62d26f), C(0xffd23f), C(0xff4a5c), C(0x58c4ff)}, LL[4] = {C(0xb0e8b7), C(0xffe89f), C(0xffa4ae), C(0xace2ff)};
    for (int i = 0; i < 22; i++) {
        int y = 44 + i * 14; if (y + 9 < vy || y > vy + SH) continue;
        int k = (i + (t >> 3)) & 3; u8 c = LC[k], l = LL[k];
        for (int j = 0; j < 9; j++) { span(2, 9, y + j, c); span(PW - 9, PW - 2, y + j, c); }
        for (int j = 1; j < 3; j++) { span(3, 5, y + j, l); span(PW - 8, PW - 6, y + j, l); }
    }
}
u32 pb_draw_ticks, pb_draw_max;   /* ciclos de CPU del último dibujo y el máximo (para medir) */
static void pb_draw(void) {
    pb_t *G = &pb_G; const int W = PW, H = PH; int t = MG.t, vy = MG.view_y;
    u32 t0 = ticks();
    arcx_measure(); arcx_clip(OX, 0, W, SH);
    arcx_bgcopy(pb_bg);
    /* luces laterales */
    pb_lights(t, vy);
    if (vy < 40) {
        for (int i = 0; i < 4; i++) arcx_blit(pb_helm[i], 96 + i * 24, (((t >> 4) + i) & 1) + 6, 0);
        seg7(G->score, 16, 10);
        for (int i = 0; i < G->balls; i++) arcx_disc(19 + i * 9, 31, 2, C(0xcfd6e2));
    }
    origin(OX + PB_OX, OY + PB_OY);
    dash_path(t);
    static const s16 LX[3] = {54, 78, 102};
    for (int i = 0; i < 3; i++) arcx_disc(LX[i], 26, 2, G->lanes[i] ? C(0xffd23f) : C(0x4a3a10));
    for (int i = 0; i < 2; i++) if (G->sfl[i]) arcx_blit(pb_slw[i], imin(SL_PTS[i][0], SL_PTS[i][4]), 197, 0);
    for (int i = 0; i < 3; i++) {
        int x = BUMP[i][0], y = BUMP[i][1], r = BUMP[i][2];
        if (y + PB_OY + r + 6 < vy || y + PB_OY - r - 4 > vy + SH) continue;
        arcx_blit(pb_bmp[r - 8][(t * 391 >> 10) & 7][G->flash[i] ? 1 : 0], x - r - 3, y - r - 3, 0);
    }
    int ouw = UI_W; UI_W = W;
    for (int i = 0; i < 3; i++) {
        pb_person_t *p = &G->people[i]; int px = (int)(p->x + 100.5f) - 100, py = (int)p->y;
        if (py + PB_OY + 16 < vy || py + PB_OY - 16 > vy + SH) continue;
        arcx_ellipse93(px, py + 11, p->hired ? C(0x62d26f) : C(0xffd23f));
        arcx_blit(person_bmp(p->look, p->hired ? (G_CASCO | G_CHAQ | G_GUANT) : 0, 0), px - 8, py - 15, 0);
    }
    for (int i = 0; i < 3; i++) {
        pb_person_t *p = &G->people[i]; int px = (int)(p->x + .5f), py = (int)p->y;
        int k = p->say > 0 ? 1 : (!p->hired && (t + p->ph * 90) % 300 < 60) ? 0 : -1;
        if (k >= 0) { int w = pb_bub[k]->w, bx = iclamp(px - w / 2, 2, W - w - 2); arcx_blit_opaque(pb_bub[k], bx, py - 30); }
    }
    UI_W = ouw;
    /* flippers */
    for (int i = 0; i < 2; i++) {
        float a = i ? 3.14159265f - G->flA[i] : G->flA[i];
        int k = (int)((a + .45f) * (19 / .95f) + .5f); k = iclamp(k, 0, 19);
        if (i == 0) arcx_blit(pb_flip[k], (int)FL_X[0] + pb_fdx[k], (int)FL_Y[0] + pb_fdy[k], 0);
        else arcx_blit(pb_flip[k], (int)FL_X[1] + pb_fdxm[k], (int)FL_Y[1] + pb_fdy[k], 1);
    }
    /* lanzador */
    int pull = G->ready ? (int)(G->plP * 12 + .5f) : 0, kick = G->shot > 6 ? -8 : 0, py = 286 + pull + kick;
    { int lx = -1, ly = 0, k = 0; for (int y = py + 4; y < 299; y += 2, k++) { int x = k & 1 ? 161 : 167; if (lx >= 0) arcx_line(lx, ly, x, y, C(0xcfd6e2)); lx = x; ly = y; } }
    rect(159, py, 10, 4, C(0xd62839)); rect(159, py, 10, 1, C(0xffffff));
    { int bx = (G->bx + 32768) >> 16, by = (G->by + 32768) >> 16;
      arcx_disc(bx, by, 3, C(0x7f8796)); arcx_disc(bx - 1, by - 1, 2, C(0xb9bfca)); rect(bx - 2, by - 2, 2, 2, C(0xffffff)); }
    origin(OX - PB_OX, OY - PB_OY);
    /* medidor de fuerza */
    if (G->ready) {
        rect(W - 11, 250, 8, 60, C(0x120b0c)); int h = (int)(G->plP * 56 + .5f);
        rect(W - 10, 309 - h, 6, h, G->plP > .75f ? C(0x62d26f) : G->plP > .4f ? C(0xffd23f) : C(0xff4a5c));
        if ((t >> 4) & 1) arc_text(W - 7, 238, "A", C(0xffd23f), 1, 8);
    }
    /* marcador compacto cuando la marquesina no se ve */
    if (vy >= 34) {
        char b[24], *q = str_num(b, G->score); *q = 0;
        rect(12, vy + 1, 40 + 4 * 6, 8, C(0x000000)); text_small(14, vy + 2, b, C(0xff2a2a), 0);
        for (int i = 0; i < G->balls; i++) arcx_disc(48 + i * 6, vy + 4, 1, C(0xcfd6e2));
    }
    (void)H;
    pb_draw_ticks = ticks() - t0; if (pb_draw_ticks > pb_draw_max && MG.t > 30) pb_draw_max = pb_draw_ticks;
}

const arc_def_t ARC_DEF0 = {"PINBALL DE CONTRATACIÓN", PW, PH,
    "A: LANZAR LA BOLA · ← →: FLIPPERS · B: GOLPE DOBLE. ¡Dale a los aspirantes para contratarlos!",
    pb_init, pb_update, pb_draw, pb_press, 0};
