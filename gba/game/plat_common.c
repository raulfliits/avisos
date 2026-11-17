/* AVENTURA DE PLATAFORMAS: fondos (drawBG/PAINT/ground), avioneta (drawPlaneAt), bocadillos (say) y HUD. */
#include "plat.h"
#define REG32(a) (*(volatile u32 *)(a))

int pfr; float pcam; int plat_maxn; int plat_dbg;
plane_t plane = {150, 26};
pfx_t fxs[MAXFX]; int nfx;

s32 plat_nh[4];
void plat_note_n(int n) { if (n > plat_maxn) plat_maxn = n; if (n >= 1 && n <= 3) plat_nh[n]++; }

pfx_t *fx_add(int type) {
    if (nfx >= MAXFX) return 0;
    pfx_t *p = &fxs[nfx++]; memset(p, 0, sizeof *p); p->type = type; return p;
}
void poof(float x, float y, int n) {
    u8 cols[3] = {C(0xffffff), C(0xffd23f), C(0xc9a0ff)};
    for (int i = 0; i < n; i++) {
        pfx_t *p = fx_add(FX_POOF); if (!p) return;
        float a = rndf() * 6.28f, v = .6f + rndf() * 1.8f;
        p->x = x; p->y = y; p->vx = fcos(a) * v; p->vy = fsin(a) * v; p->c = cols[i % 3]; p->life = 28 + (int)(rndf() * 14);
    }
}

void sil(const s16 *pts, int n, u8 col) { poly(pts, n, col); }

/* ---------------------------------------------------------------- texto pequeño rápido (índice directo; ✗ y – añadidos) */
extern const u16 SMALL_CP[]; extern const u8 SMALL_BM[][5];
static s16 SIDX[256]; static s16 SI_CHECK = -1, SI_STAR = -1; static u8 sidx_ok;
static const u8 G_CROSS[5] = {5, 5, 2, 5, 5}, G_DASH[5] = {0, 0, 7, 0, 0};
static int utf8n(const char **ps) {
    const u8 *s = (const u8 *)*ps; int c = *s++;
    if (c >= 0xF0) { c = ((c & 7) << 18) | ((s[0] & 63) << 12) | ((s[1] & 63) << 6) | (s[2] & 63); s += 3; }
    else if (c >= 0xE0) { c = ((c & 15) << 12) | ((s[0] & 63) << 6) | (s[1] & 63); s += 2; }
    else if (c >= 0xC0) { c = ((c & 31) << 6) | (s[0] & 63); s += 1; }
    *ps = (const char *)s; return c;
}
int stext(int x, int y, const char *s, u8 col, int align) {
    if (!sidx_ok) { for (int i = 0; i < 256; i++) SIDX[i] = -1;
        for (int i = 0; SMALL_CP[i] != 10003; i++) if (SMALL_CP[i] < 256) SIDX[SMALL_CP[i]]= i;
        for (int i = 0; i < 200; i++) { if (SMALL_CP[i] == 10003) { SI_CHECK = i; break; } if (SMALL_CP[i] == 9733) SI_STAR = i; }
        sidx_ok = 1; }
    int w = text_len(s) * 4;
    if (align == 1) x -= w / 2; else if (align == 2) x -= w;
    while (*s) { int cp = utf8n(&s); const u8 *g = 0;
        if (cp < 256) { int k = SIDX[cp]; if (k < 0 && cp >= 'a' && cp <= 'z') k = SIDX[cp - 32]; if (k >= 0) g = SMALL_BM[k]; }
        else if (cp == 10003 && SI_CHECK >= 0) g = SMALL_BM[SI_CHECK];
        else if (cp == 9733 && SI_STAR >= 0) g = SMALL_BM[SI_STAR];
        else if (cp == 10007) g = G_CROSS; else if (cp == 8211) g = G_DASH;
        if (g && cp != ' ') bits_draw(x, y, g, 5, 3, col);
        x += 4; }
    return w;
}

/* ---------------------------------------------------------------- copias rápidas a la pantalla (modo 4) */
extern bmp_t T;
IWRAM_CODE void row_copy(u8 *row, int a, const u8 *s, int n, int trans) {
    int e = a + n;
    if (a & 1) { u8 c = *s++; if (c || !trans) { u16 *q = (u16 *)(row + a - 1); *q = (*q & 0xFF) | (c << 8); } a++; }
    while (a + 2 <= e) {
        u8 c0 = s[0], c1 = s[1]; u16 *q = (u16 *)(row + a);
        if (!trans || (c0 && c1)) *q = c0 | (c1 << 8);
        else if (c0) *q = (*q & 0xFF00) | c0; else if (c1) *q = (*q & 0xFF) | (c1 << 8);
        s += 2; a += 2;
    }
    if (a < e) { u8 c = *s; if (c || !trans) { u16 *q = (u16 *)(row + a); *q = (*q & 0xFF00) | c; } }
}
/* copia n píxeles opacos a una fila de VRAM (dst cualquier paridad) */
IWRAM_CODE void copy_span(u8 *row, int a, const u8 *src, int n) {
    if (n <= 0) return;
    if (a & 1) { volatile u16 *q = (u16 *)(row + a - 1); *q = (*q & 0xFF) | (*src << 8); a++; src++; n--; }
    int pairs = n >> 1; u16 *d = (u16 *)(row + a);
    if (pairs) {
        if (!((u32)src & 1)) {
            if (pairs >= 8) { REG32(0x040000D4) = (u32)src; REG32(0x040000D8) = (u32)d; REG32(0x040000DC) = 0x80000000u | pairs; }
            else { const u16 *s16 = (const u16 *)src; for (int i = 0; i < pairs; i++) d[i] = s16[i]; }
        } else {
            const u16 *s16 = (const u16 *)(src - 1); u32 prev = *s16++ >> 8;
            for (int i = 0; i < pairs; i++) { u32 nx = *s16++; d[i] = prev | ((nx & 0xFF) << 8); prev = nx >> 8; }
        }
    }
    if (n & 1) { volatile u16 *q = (u16 *)(row + a + pairs * 2); *q = (*q & 0xFF00) | src[pairs * 2]; }
}
IWRAM_CODE void far_rows(const u8 *H, u8 fc) {
    u16 cc = fc | (fc << 8); u8 *base = T.px + GY * VW;
    for (int a = 0; a < VW; a += 2) { int h0 = H[a], h1 = H[a + 1], lo = h0 < h1 ? h0 : h1, hi = h0 < h1 ? h1 : h0;
        u16 *q = (u16 *)(base + a) - (VW / 2);
        for (int k = 0; k < lo; k++, q -= VW / 2) *q = cc;
        if (h0 > h1) for (int k = lo; k < hi; k++, q -= VW / 2) *q = (*q & 0xFF00) | fc;
        else for (int k = lo; k < hi; k++, q -= VW / 2) *q = (*q & 0xFF) | (fc << 8); }
}
/* blit con transparencia rápido (de dos en dos píxeles), con espejo; sin origen */
IWRAM_CODE void blit_t(const bmp_t *b, int x, int y, int flip) {
    if (!b) return;
    int w = b->w, h = b->h, i0 = 0, i1 = w, j0 = 0, j1 = h;
    if (x < 0) i0 = -x; if (x + w > VW) i1 = VW - x; if (y < 0) j0 = -y; if (y + h > VH) j1 = VH - y;
    if (i0 >= i1 || j0 >= j1) return;
    for (int j = j0; j < j1; j++) {
        u8 *row = T.px + (y + j) * VW; const u8 *src = b->px + j * w;
        int a = x + i0, e = x + i1, i = i0;
        if (!flip) { row_copy(row, a, src + i0, i1 - i0, 1); continue; }
        /* espejo: píxel de pantalla x+i ← src[w-1-i] */
        if (a & 1) { u8 c = src[w - 1 - i]; if (c) { u16 *q = (u16 *)(row + a - 1); *q = (*q & 0xFF) | (c << 8); } a++; i++; }
        for (; a + 2 <= e; a += 2, i += 2) { u8 c0 = src[w - 1 - i], c1 = src[w - 2 - i]; u16 *q = (u16 *)(row + a);
            if (c0 && c1) *q = c0 | (c1 << 8); else if (c0) *q = (*q & 0xFF00) | c0; else if (c1) *q = (*q & 0xFF) | (c1 << 8); }
        if (a < e) { u8 c = src[w - 1 - i]; if (c) { u16 *q = (u16 *)(row + a); *q = (*q & 0xFF00) | c; } }
    }
}
/* sprite con tramos opacos precalculados (para copiar con copy_span/DMA) */
int rle_build(rle_t *r, const bmp_t *b, int cap) {
    r->b = b; if (!b) return 0;
    if (!cap) { for (int y = 0; y < b->h; y++) { const u8 *p = b->px + y * b->w; for (int x = 0; x < b->w; x++) if (p[x] && (x == 0 || !p[x - 1])) cap++; } if (!cap) cap = 1; r->ri = 0; }
    if (!r->ri || r->cap < cap) { bmp_t *m = bmp_new(cap * 4 + (b->h + 1) * 2 + 4, 1); if (!m) { r->b = 0; return 0; } r->ri = (u16 *)m->px; r->rs = r->ri + b->h + 1; r->cap = cap; }
    int n = 0, W = b->w;
    for (int y = 0; y < b->h; y++) { r->ri[y] = n; const u8 *p = b->px + y * W; int x = 0;
        while (x < W) { while (x < W && !p[x]) x++; if (x >= W) break; int st = x; while (x < W && p[x]) x++;
            if (n < r->cap) { r->rs[n * 2] = st; r->rs[n * 2 + 1] = x; n++; } } }
    r->ri[b->h] = n; return 1;
}
IWRAM_CODE void rle_draw(const rle_t *r, int x, int y) {
    const bmp_t *b = r->b; if (!b) return;
    int j0 = y < 0 ? -y : 0, j1 = y + b->h > VH ? VH - y : b->h;
    for (int j = j0; j < j1; j++) { u8 *row = T.px + (y + j) * VW; const u8 *src = b->px + j * b->w;
        for (int k = r->ri[j]; k < r->ri[j + 1]; k++) { int a = r->rs[k * 2], e = r->rs[k * 2 + 1];
            if (x + a < 0) a = -x; if (x + e > VW) e = VW - x; if (e > a) copy_span(row, x + a, src + a, e - a); } }
}
/* bitmap → pantalla sin transparencia (o con ella si trans), recortado a 240x160; sin origen */
void blit_opq(const bmp_t *b, int sx, int sy, int w, int h, int x, int y, int trans) {
    if (x < 0) { sx -= x; w += x; x = 0; } if (y < 0) { sy -= y; h += y; y = 0; }
    if (x + w > VW) w = VW - x; if (y + h > VH) h = VH - y;
    if (w <= 0 || h <= 0) return;
    for (int j = 0; j < h; j++) { if (trans) row_copy(T.px + (y + j) * VW, x, b->px + (sy + j) * b->w + sx, w, 1); else copy_span(T.px + (y + j) * VW, x, b->px + (sy + j) * b->w + sx, w); }
}

/* ---------------------------------------------------------------- fondo */
static bmp_t *ground, *ground2; static rle_t LR[6];
static u8 FARH[900]; static int far_ok;
static land_t LANDS[6]; static int nland;

static void sea_live(int x, int w) {
    rect(x - w / 2, GY - 12, w, 12, C(0x3d86d6));
    for (int i = 0; i < w; i += 9) rect(x - w / 2 + i + ((pfr >> 4) % 3), GY - 12 + ((i >> 3) % 3) * 3, 4, 1, C(0xbfe0ff));
}
#define S2(...) do { static const s16 _p[] = {__VA_ARGS__}; s16 q[sizeof _p / sizeof _p[0]]; for (unsigned _i = 0; _i < sizeof _p / sizeof _p[0]; _i += 2) { q[_i] = _p[_i] + x; q[_i + 1] = _p[_i + 1] + GY; } poly(q, sizeof _p / sizeof _p[0] / 2, col); } while (0)
static void paint(int n, int x) {
    u8 col;
    switch (n) {
    case L_SAGRADA: {
        u8 st = C(0xb88d63), sd = C(0x94704c);
        static const s8 T[4][2] = {{0, 46}, {9, 54}, {18, 54}, {27, 46}};
        for (int i = 0; i < 4; i++) { int dx = T[i][0], h = T[i][1];
            rect(x + dx, GY - h, 6, h, st); rect(x + dx + 4, GY - h, 2, h, sd); rect(x + dx + 1, GY - h - 6, 4, 6, st); rect(x + dx + 2, GY - h - 10, 2, 4, st);
            rect(x + dx + 2, GY - h - 12, 2, 2, C(0xe2c341)); for (int k = GY - h + 4; k < GY - 6; k += 6) rect(x + dx + 2, k, 2, 2, C(0x6f5034)); }
        rect(x - 6, GY - 22, 46, 22, st);
        int tx = x + 60; rect(tx, GY - 48, 12, 48, C(0x2f5f9e)); rect(tx + 1, GY - 52, 10, 4, C(0x2f5f9e)); rect(tx + 3, GY - 55, 6, 3, C(0x2f5f9e));
        for (int k = 0; k < 20; k++) rect(tx + 1 + (k * 5) % 10, GY - 46 + ((k * 7) % 44), 1, 1, k % 3 ? C(0xd8433f) : C(0x7fc7ff));
    } break;
    case L_VALLES:
        col = C(0x5b9b55); S2(-90, 0, -40, -26, 0, -14, 40, -30, 100, 0);
        for (int i = 0; i < 5; i++) { rect(x - 40 + i * 20, GY - 20 - (i % 2) * 8, 8, 14, C(0x9e9a92)); rect(x - 41 + i * 20, GY - 22 - (i % 2) * 8, 10, 3, C(0xc84d3c)); }
        rect(x + 60, GY - 40, 4, 40, C(0x7c6a58)); rect(x + 59, GY - 42, 6, 3, C(0x9a8670));
        break;
    case L_AQUEDUCT:
        col = C(0x3f7f4a); S2(-100, 0, -70, -12, -60, 0, 60, 0, 80, -14, 110, 0);
        col = C(0xc9a46a); S2(-30, 0, -26, -30, -12, -44, 12, -44, 26, -30, 30, 0);
        col = C(0x5f9fd8); S2(-18, 0, -16, -24, -8, -32, 8, -32, 16, -24, 18, 0);
        rect(x - 30, GY - 46, 60, 3, C(0xa88550)); rect(x - 4, GY - 56, 8, 10, C(0xa88550)); rect(x - 6, GY - 58, 12, 2, C(0xa88550));
        break;
    case L_SEU:
        col = C(0xa18a5a); S2(-80, 0, -30, -34, 30, -34, 80, 0);
        rect(x - 22, GY - 52, 36, 18, C(0xd8c18f));
        col = C(0xb8a070); S2(-24, -52, -4, -62, 16, -52);
        rect(x + 14, GY - 70, 10, 36, C(0xd8c18f)); rect(x + 15, GY - 75, 8, 5, C(0xd8c18f)); rect(x + 17, GY - 79, 4, 4, C(0xd8c18f));
        rect(x + 17, GY - 64, 4, 6, C(0x5d4b30)); rect(x - 14, GY - 46, 4, 6, C(0x5d4b30)); rect(x - 2, GY - 46, 4, 6, C(0x5d4b30));
        break;
    case L_MONASTERY:
        rect(x - 40, GY - 30, 70, 30, C(0xc7b28a)); for (int i = 0; i < 6; i++) rect(x - 36 + i * 11, GY - 18, 6, 18, C(0xa8936c));
        rect(x + 22, GY - 58, 14, 58, C(0xc7b28a)); rect(x + 26, GY - 52, 6, 8, C(0xa8936c));
        col = C(0x9a8460); S2(20, -58, 29, -68, 38, -58);
        rect(x - 8, GY - 27, 8, 8, C(0x7f6b4a)); rect(x - 40, GY - 32, 62, 2, C(0xc84d3c));
        break;
    case L_INDUSTRIAL:
        for (int i = 0; i < 4; i++) { int a = x - 60 + i * 28;
            rect(a, GY - 26, 26, 26, C(0x9aa3b5));
            s16 q[6] = {a, GY - 26, a + 20, GY - 36, a + 20, GY - 26}; poly(q, 3, C(0x7d879c));
            rect(x - 56 + i * 28, GY - 18, 6, 5, C(0x5f6880)); rect(x - 46 + i * 28, GY - 18, 6, 5, C(0x5f6880)); }
        rect(x + 60, GY - 60, 7, 60, C(0x7d879c)); rect(x + 60, GY - 56, 7, 3, C(0xd8433f));
        break;
    case L_BEACH: {
        rect(x - 130, GY - 3, 260, 3, C(0xecd58f));
        static const s8 DX[3] = {-70, -10, 60}; static const s8 FR[5][2] = {{-9, 3}, {-5, -2}, {0, -4}, {5, -2}, {9, 3}};
        for (int j = 0; j < 3; j++) { int dx = DX[j];
            for (int k = 0; k < 28; k++) rect(x + dx + iround(fsin(k / 9.0f) * 3), GY - 3 - k, 2, 1, C(0x7a5530));
            int tx = x + dx + iround(fsin(27 / 9.0f) * 3), ty = GY - 31;
            for (int f = 0; f < 5; f++) line(tx, ty, tx + FR[f][0], ty + FR[f][1] + 3, C(0x2f8f3a), 2); }
    } break;
    case L_MATARO:
        for (int i = 0; i < 6; i++) { int ax = x - 60 + i * 20;
            s16 q[20] = {ax, GY - 12, ax + 2, GY - 30, ax + 9, GY - 40, ax + 16, GY - 30, ax + 18, GY - 12, ax + 15, GY - 12, ax + 13, GY - 28, ax + 9, GY - 35, ax + 5, GY - 28, ax + 3, GY - 12};
            poly(q, 10, C(0xf2efe6)); }
        rect(x - 64, GY - 14, 128, 2, C(0xc9a46a));
        break;
    case L_PYRENEES: {
        static const s16 M[4][2] = {{-70, 70}, {-10, 86}, {50, 74}, {100, 60}};
        for (int i = 0; i < 4; i++) { int dx = M[i][0], h = M[i][1];
            s16 a[6] = {x + dx - 60, GY, x + dx, GY - h, x + dx + 60, GY}; poly(a, 3, C(0x6c7b95));
            s16 b[12] = {x + dx - 14, GY - h + 16, x + dx, GY - h, x + dx + 14, GY - h + 16, x + dx + 6, GY - h + 12, x + dx, GY - h + 17, x + dx - 6, GY - h + 12};
            poly(b, 6, C(0xf4f6ff)); }
    } break;
    }
}
static const u8 TOPH[9] = {67, 43, 59, 80, 69, 61, 32, 41, 87};
static const s16 EXT[9][2] = {{-6, 72}, {-90, 100}, {-100, 110}, {-80, 80}, {-40, 38}, {-60, 67}, {-130, 130}, {-64, 64}, {-130, 160}};

void bg_build(const land_t *l, int n) {
    if (!far_ok) { for (int x = 0; x < 900; x++) FARH[x] = (u8)iround(18 + fsin(x / 37.0f) * 6 + fsin(x / 13.0f) * 3); far_ok = 1; }
    /* suelo: baldosa 32x24 repetida en 272 */
    bmp_t *tile = bmp_new(32, 24);
    tgt_bmp(tile);
    rect(0, 0, 32, 10, C(0xbdb9ae));
    for (int t = 0; t < 2; t++) { int o = t * 16; u8 c = C(0x8f8a7e);
        rect(o, 0, 1, 10, c); rect(o, 9, 16, 1, c);
        rect(o + 4, 3, 2, 2, c); rect(o + 11, 3, 2, 2, c); rect(o + 4, 6, 2, 2, c); rect(o + 11, 6, 2, 2, c); rect(o + 7, 4, 3, 3, c); }
    rect(0, 0, 32, 1, C(0x3e7a35));
    rect(0, 10, 32, 14, C(0x8a5a35));
    for (int i = 0; i < 8; i++) rect((i * 11) % 32, 12 + (i * 5) % 12, 3, 2, C(0x6e4527));
    for (int i = 0; i < 6; i++) rect((i * 7 + 3) % 32, 14 + (i * 3) % 10, 2, 1, C(0xa5714a));
    tgt_back();
    ground = bmp_new(272, 24);
    tgt_bmp(ground); for (int x = 0; x < 272; x += 32) blit(tile, x, 0, 0); tgt_back();
    ground2 = bmp_new(272, 24); /* copia desplazada 1 px para copiar siempre alineado (DMA) */
    tgt_bmp(ground2); for (int x = -1; x < 272; x += 32) blit(tile, x, 0, 0); tgt_back();
    /* paisajes en una tira (coordenadas de parallax) */
    nland = n; memcpy(LANDS, l, n * sizeof *l);
    for (int i = 0; i < n; i++) {   /* un bitmap ajustado por paisaje (menos memoria que una tira) */
        int t = l[i].n, w = EXT[t][1] - EXT[t][0] + 2, h = TOPH[t]; LR[i].b = 0; LR[i].ri = 0;
        bmp_t *bm = bmp_new(w, h); if (!bm) continue;
        tgt_bmp(bm); origin(-(l[i].x + EXT[t][0]), -(GY - h)); paint(t, l[i].x); origin(0, 0); tgt_back();
        rle_build(&LR[i], bm, 0);
    }
}
void bg_sky_only(void) {
    u8 sk[8] = {C(0x66b8f3), C(0x73bef4), C(0x80c6f6), C(0x8dccf8), C(0x9bd4f9), C(0xa8dafb), C(0xb5e2fd), C(0xc2e8fe)};
    for (int i = 0; i < 8; i++) rect(0, i * 17, VW, 17, sk[i]);
}
void bg_draw(void) {

    bg_sky_only();
    int cam4 = (int)(pcam * 4);
    for (int i = 0; i < 7; i++) {
        int v = i * 157 * 20 - cam4 - pfr, m = v % 6000; if (m < 0) m += 6000;
        int cx = (m + 10) / 20 - 30, cy = 12 + (i * 23) % 40; u8 w = C(0xffffff);
        rect(cx, cy, 26, 6, w); rect(cx + 5, cy - 4, 14, 4, w); rect(cx + 3, cy + 6, 20, 2, w);
    }

    int fo = ffloor(pcam * .25f); u8 fc = C(0xa7c8b4);
    if (fo < 0) fo = 0; if (fo > 900 - VW) fo = 900 - VW;
    far_rows(FARH + fo, fc);

    for (int i = 0; i < nland; i++) { int x = iround(LANDS[i].x - pcam * .5f); if (x < -170 || x > VW + 170) continue;
        if (LANDS[i].n == L_SAGRADA) sea_live(x + 10, 200); else if (LANDS[i].n == L_BEACH) sea_live(x, 260); else if (LANDS[i].n == L_MATARO) sea_live(x, 240); }

    { int ox = iround(-pcam * .5f);
      for (int i = 0; i < nland; i++) if (LR[i].b) { int t = LANDS[i].n, X = LANDS[i].x + EXT[t][0] + ox;
          if (X < VW && X + LR[i].b->w > 0) rle_draw(&LR[i], X, GY - LR[i].b->h); } }
    for (int i = 0; i < nland; i++) if (LANDS[i].n == L_INDUSTRIAL) { int x = iround(LANDS[i].x - pcam * .5f);
        if (x > -170 && x < VW + 170) rect(x + 62 + ((pfr >> 4) % 3), GY - 66 - ((pfr >> 3) % 6), 4, 3, C(0xe8eef8)); }
    int c = iround(pcam);
    int gs = ((c % 32) + 32) % 32;
    if (gs & 1) blit_opq(ground2, gs - 1, 0, VW, 24, 0, GY, 0); else blit_opq(ground, gs, 0, VW, 24, 0, GY, 0);
}

/* ---------------------------------------------------------------- avioneta */
static const char *const PF_F[5] = {"111", "100", "110", "100", "100"}, *const PF_L[5] = {"100", "100", "100", "100", "111"},
    *const PF_I[5] = {"1", "0", "1", "1", "1"}, *const PF_T[5] = {"111", "010", "010", "010", "010"}, *const PF_S[5] = {"111", "100", "111", "001", "111"};
static void plane_body(int x, int y, int pilot, int banner) {
    u8 R = C(0xd62839), W = C(0xffffff);
    if (banner) { line(x - 4, y + 3, x - 1, y + 3, KK, 1); rect(x - 27, y - 2, 23, 11, KK); rect(x - 26, y - 1, 21, 9, W); }
    rect(x - 1, y - 7, 7, 9, KK); rect(x, y - 6, 5, 7, R);
    rect(x + 7, y - 13, 24, 3, KK); rect(x + 8, y - 12, 22, 1, C(0xf2f2f2));
    rect(x + 10, y - 11, 1, 20, KK); rect(x + 27, y - 11, 1, 20, KK);
    if (pilot) {
        draw_spr(CREW[1].spr, 10, x + 8, y - 10, 0, 0);
        rect(x + 10, y - 11, 12, 5, KK); rect(x + 11, y - 10, 10, 3, C(0x7a4a2a)); rect(x + 10, y - 6, 12, 3, KK);
        rect(x + 11, y - 5, 4, 1, C(0x9fd8ff)); rect(x + 17, y - 5, 4, 1, C(0x9fd8ff));
    }
    rect(x, y, 35, 9, KK); rect(x + 1, y + 1, 33, 7, R); rect(x + 1, y + 4, 33, 1, W);
    rect(x + 6, y + 9, 24, 3, KK); rect(x + 7, y + 10, 22, 1, C(0xf2f2f2));
    rect(x + 12, y + 12, 1, 3, KK); rect(x + 24, y + 12, 1, 3, KK); rect(x + 11, y + 14, 3, 2, KK); rect(x + 23, y + 14, 3, 2, KK);
    rect(x + 35, y + 2, 2, 5, KK);
}
/* avioneta pre-dibujada (sin hélice ni letras, que se mueven): [0] con piloto y pancarta, [1] sola */
static bmp_t *PLB[2]; static rle_t PLR[2];
void draw_plane_at(float sx, float sy, int pilot, int fin) {
    int x = iround(sx) - 17, y = iround(sy); u8 R = C(0xd62839);
    int banner = pilot || !fin, v = pilot ? 0 : 1;
    if (!PLB[v]) PLB[v] = bmp_new(66, 30);
    if (PLB[v] && !PLR[v].b) { tgt_bmp(PLB[v]); plane_body(27, 13, pilot, banner); tgt_back(); rle_build(&PLR[v], PLB[v], 30 * 8); }
    if (PLR[v].b) rle_draw(&PLR[v], x - 27, y - 13); else plane_body(x, y, pilot, banner);
    if (banner) {
        const char *const *gl[6] = {PF_F, PF_L, PF_I, PF_I, PF_T, PF_S}; int bx = x - 25;
        for (int k = 0; k < 6; k++) { int w = strlen(gl[k][0]), wv = ((bx + pfr) >> 3) & 1;
            for (int r = 0; r < 5; r++) for (int q = 0; q < w; q++) if (gl[k][r][q] == '1') pset(bx + q, y + 1 + r + wv, R);
            bx += w + 1; }
    }
    int pf = (pfr >> 1) & 1;
    if (banner) rect(x + 37, pf ? y - 3 : y + 4, 1, 7, C(0xcfd6e2)); else rect(x + 37, y, 1, 7, C(0xcfd6e2));
}

/* ---------------------------------------------------------------- bocadillos */
bub_t bubs[3]; int nbub; static u8 bub_ids;
void (*bub_anchor)(bub_t *b, int *x, int *y);
static int pri_of(int cls) {
    int p = -1;
    if (cls & BC_BAD) p = imax(p, 4); if (cls & (BC_PLANE | BC_NO)) p = imax(p, 3);
    if (cls & (BC_OK | BC_MAIL)) p = imax(p, 2); if (cls & BC_ME) p = imax(p, 1); if (cls & BC_CAND) p = imax(p, 0);
    return p < 0 ? 1 : p;
}
void clear_bubbles(void) { nbub = 0; }
void say(const char *t, int an, int ref, int wx, int wy, int frames, int cls) {
    int pri = pri_of(cls), mx = 2;
    frames = imin(frames, pri >= 4 ? 120 : pri >= 2 ? 85 : 60);
    for (int i = 0; i < nbub; i++) if (!strcmp(bubs[i].t, t)) { if (pfr + frames > bubs[i].until) bubs[i].until = pfr + frames; return; }
    if (nbub >= mx) {
        int lo = 0; for (int i = 1; i < nbub; i++) if (bubs[i].pri < bubs[lo].pri) lo = i;
        if (bubs[lo].pri > pri) return;
        for (int i = lo; i < nbub - 1; i++) bubs[i] = bubs[i + 1];
        nbub--;
    }
    bub_t *b = &bubs[nbub++];
    strncpy(b->t, t, 71); b->t[71] = 0; b->an = an; b->ref = ref; b->wx = wx; b->wy = wy; b->dy = wy; b->until = pfr + frames; b->pri = pri; b->cls = cls; b->slot = -1; b->id = ++bub_ids; if (!b->id) b->id = ++bub_ids;
}
void plane_say(const char *t, int cls, int frames) { say(t, AN_PLANE, 0, 0, 0, frames, BC_PLANE | cls); }

/* corta el texto en líneas de como mucho 'cols' caracteres (UTF-8) */
static int wrap_lines(const char *s, int cols, char out[][48], int maxl) {
    int n = 0, cur = 0; out[0][0] = 0;
    while (*s) {
        while (*s == ' ') s++;
        if (!*s) break;
        const char *e = s; int wl = 0;
        while (*e && *e != ' ') { u8 c = (u8)*e; e += c >= 0xF0 ? 4 : c >= 0xE0 ? 3 : c >= 0xC0 ? 2 : 1; wl++; }
        if (cur && cur + 1 + wl > cols) { if (n + 1 >= maxl) break; n++; out[n][0] = 0; cur = 0; }
        int bl = strlen(out[n]); if (cur) { out[n][bl++] = ' '; cur++; }
        int len = e - s; if (bl + len > 47) len = 47 - bl;
        memcpy(out[n] + bl, s, len); out[n][bl + len] = 0; cur += wl; s = e;
    }
    return n + 1;
}
s32 bub_renders;
#define NBB 2
static bmp_t *BB[NBB]; static s8 bb_owner[NBB];
static int dx_direct = -9999, dy_direct;
static void bub_render(bub_t *b, bmp_t *dst) {
    int cls = b->cls; u8 bg = C(0xffffff), bd = C(0x120b0c), fg = C(0x120b0c);
    if (cls & BC_ME) bg = C(0xfff0f2);
    if (cls & BC_OK) bg = C(0x62d26f);
    if (cls & BC_BAD) bg = C(0xffd23f);
    if (cls & BC_NO) { bg = C(0xe4002b); fg = C(0xffffff); }
    if (cls & BC_MAIL) bd = C(0x1f9e4a);
    int len = text_len(b->t), big = len <= 12, w, h, nl = 1; char L[4][48];
    if (big) { w = len * 8 + 6; h = 12; }
    else { nl = wrap_lines(b->t, 26, L, 4); int m = 0; for (int k = 0; k < nl; k++) m = imax(m, text_len(L[k])); w = m * 4 + 6; h = nl * 7 + 4; }
    if (w > 112) w = 112; if (h > 32) h = 32;
    b->w = w; b->h = h; b->bd = bd; bub_renders++;
    int ox = 0, oy = 0;
    if (dst) { memset(dst->px, 0, dst->w * dst->h); tgt_bmp(dst); } else { if (dx_direct < -999) return; ox = dx_direct; oy = dy_direct; }
    rect(ox, oy, w, h, bd); rect(ox + 1, oy + 1, w - 2, h - 2, bg);
    if (big) text(ox + 3, oy + 2, b->t, fg, 0);
    else for (int k = 0; k < nl; k++) stext(ox + w / 2, oy + 3 + k * 7, L[k], fg, 1);
    if (dst) tgt_back();
}
void draw_bubbles(void) {
    int j = 0;
    for (int i = 0; i < nbub; i++) if (pfr <= bubs[i].until) bubs[j++] = bubs[i];
    nbub = j;
    for (int i = 0; i < nbub; i++) {
        bub_t *b = &bubs[i];
        if (b->slot < 0 || !BB[(int)b->slot] || bb_owner[(int)b->slot] != b->id) {
            int sl = -1;
            for (int q = 0; q < NBB && sl < 0; q++) { int used = 0; for (int k = 0; k < nbub; k++) if (k != i && bubs[k].slot == q && bb_owner[q] == bubs[k].id) used = 1; if (!used) sl = q; }
            if (sl >= 0 && !BB[sl]) sl = -1;
            b->slot = sl; if (sl >= 0) bb_owner[sl] = b->id;
            bub_render(b, sl >= 0 ? BB[sl] : 0);
        }
        int x = b->wx, y = b->wy;
        if (bub_anchor) bub_anchor(b, &x, &y);
        int sx = x - iround(pcam), w = b->w, h = b->h;
        int bx = sx - w / 2;
        if (sx > -20 && sx < VW + 20) bx = iclamp(bx, 2, VW - 2 - w);
        int by = y - 5 - h; if (by < 2) by = 2;
        /* que no se tapen entre ellos: si choca con uno anterior, se sube */
        for (int k = 0; k < i; k++) { bub_t *o = &bubs[k]; if (bx < o->dx + o->w && o->dx < bx + w && by < o->dy2 + o->h + 3 && o->dy2 < by + h + 3) { int ny = o->dy2 - h - 4; if (ny >= 2) by = ny; else by = o->dy2 + o->h + 4; } }
        b->dx = bx; b->dy2 = by;
        if (b->slot >= 0 && BB[(int)b->slot]) blit_opq(BB[(int)b->slot], 0, 0, w, h, bx, by, 0); else { dx_direct = bx; dy_direct = by; bub_render(b, 0); dx_direct = -9999; }
        int ax = iclamp(sx, bx + 4, bx + w - 5);
        if (y - 5 >= by + h) { u8 bd = b->bd; rect(ax - 2, by + h, 5, 1, bd); rect(ax - 1, by + h + 1, 3, 1, bd); pset(ax, by + h + 2, bd); }
    }
}

/* HUD (cabecera del web: puntos, monedas, contadores) arriba a la derecha */
static bmp_t *HUDB; static rle_t HUDR; static s32 hud_key[5]; s32 hud_renders;
void draw_hud_top(int show_ctr, int c1, int c2, const char *l1, const char *l2) {
    s32 key[5] = {SCORE, COINS, show_ctr ? c1 : -1, show_ctr ? c2 : -1, (s32)l1};
    if (!HUDB) { HUDB = bmp_new(100, 26); hud_key[0] = -1; hud_key[1] = -1; }
    if (!HUDB || memcmp(key, hud_key, sizeof key)) {
        hud_renders++; memcpy(hud_key, key, sizeof key); if (HUDB) { memset(HUDB->px, 0, 100 * 26); tgt_bmp(HUDB); origin(100 - VW, 0); }
        char b[48], *p; u8 W = C(0xffffff), K = C(0x120b0c);
        p = b; *p = 0; { s32 s = SCORE; char d[12]; str_num(d, s); for (int k = strlen(d); k < 6; k++) *p++ = '0'; *p = 0; p = str_cat(p, d); }
        p = str_cat(p, "  x"); if (COINS < 10) p = str_cat(p, "0"); str_num(p, COINS);
        int w = text_len(b) * 4; int x = VW - 3 - w;
        stext(x + 1, 4, b, K, 0); stext(x, 3, b, W, 0);
        int cx = x + 6 * 4 + 4; rect(cx - 1, 2, 5, 7, K); rect(cx, 3, 3, 5, C(0xffd23f));
        if (show_ctr) for (int r = 0; r < 2; r++) {
            int v = r ? c2 : c1; p = str_cat(b, r ? l2 : l1); p = str_cat(p, " "); if (v < 10) p = str_cat(p, "0"); str_num(p, v);
            stext(VW - 3 + 1, 11 + r * 7 + 1, b, K, 2); stext(VW - 3, 11 + r * 7, b, r ? C(0xff8a92) : C(0x62d26f), 2);
        }
        if (HUDB) { origin(0, 0); tgt_back(); rle_build(&HUDR, HUDB, 26 * 24); }
    }
    if (HUDB) rle_draw(&HUDR, VW - 100, 0);
}
void plat_ui_alloc(void) {
    HUDB = bmp_new(100, 26); hud_key[0] = hud_key[1] = -1;
    for (int i = 0; i < NBB; i++) BB[i] = bmp_new(112, 32);
    PLB[0] = bmp_new(66, 30); PLB[1] = bmp_new(66, 30);
}
void plat_ui_reset(void) { HUDB = 0; memset(&HUDR, 0, sizeof HUDR); memset(PLR, 0, sizeof PLR); PLB[0] = PLB[1] = 0; for (int i = 0; i < NBB; i++) { BB[i] = 0; bb_owner[i] = 0; } nbub = 0; }

/* drawPerson cacheado: 18x27 por (look, gear sin mochila, flip, stride, filas); la mochila se pinta aparte */
#define PCN 20
typedef struct { u8 look, gear, flip, rows; s8 stride; u8 ok; bmp_t *b; rle_t r; } pslot_t;
static pslot_t PS[PCN]; static int ps_next, ps_init; int pers_miss;
void pers_reset(void) { ps_init = 0; }
void pers_draw(int look, int x, int y, int flip, int stride, int gear, int nrows) {
    if (!ps_init) { for (int i = 0; i < PCN; i++) { PS[i].ok = 0; PS[i].b = bmp_new(18, 27); memset(&PS[i].r, 0, sizeof PS[i].r); } ps_next = 0; ps_init = 1; }
    if (gear & G_MOCH) fbox(flip ? x + 15 : x - 20, y + 5);
    int g = gear & ~G_MOCH; look %= 6; if (!nrows) nrows = 24;
    pslot_t *s = 0;
    for (int i = 0; i < PCN; i++) if (PS[i].ok && PS[i].look == look && PS[i].gear == g && PS[i].flip == flip && PS[i].stride == stride && PS[i].rows == nrows) { s = &PS[i]; break; }
    if (!s) {
        pers_miss++; s = &PS[ps_next]; ps_next = (ps_next + 1) % PCN;
        if (!s->b) { draw_person(look, x, y, flip, stride, g, nrows); return; }
        memset(s->b->px, 0, 18 * 27);
        tgt_bmp(s->b); draw_person(look, 1, 3, flip, stride, g, nrows); tgt_back();
        s->look = look; s->gear = g; s->flip = flip; s->stride = stride; s->rows = nrows; s->ok = 1; rle_build(&s->r, s->b, 27 * 3);
    }
    if (s->r.b) rle_draw(&s->r, x - 1 + OX, y - 3 + OY); else blit(s->b, x - 1, y - 3, 0);
}
