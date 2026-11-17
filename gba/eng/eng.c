/* Motor GBA: vídeo (modo 4 con doble página), texto, entrada, sonido PSG y guardado. */
#include "eng.h"
#include "gen.h"
#include <math.h>

#define REG16(a) (*(volatile u16 *)(a))
#define REG32(a) (*(volatile u32 *)(a))
#define DISPCNT REG16(0x04000000)
#define DISPSTAT REG16(0x04000004)
#define BLDCNT REG16(0x04000050)
#define BLDY REG16(0x04000054)
#define KEYINPUT REG16(0x04000130)
#define IE REG16(0x04000200)
#define IME REG16(0x04000208)
#define WAITCNT REG16(0x04000204)
#define PALRAM ((volatile u16 *)0x05000000)

volatile u32 vbl_count;
u16 key_held, key_hit;
u32 frame_no;
u8 LUT_DARK[256], LUT_LIGHT[256];
bmp_t T;                         /* destino actual */
s16 OX, OY;                      /* origen (translate) */
static s16 CX0, CY0, CX1 = SW, CY1 = SH; /* recorte en coordenadas del destino */
static bmp_t SCREEN;

/* ---------------------------------------------------------------- colores */
u32 col_of(u8 c) { return PAL24[c]; }
static u8 near_cache_k[64]; static u32 near_cache_v[64];
u8 col_rgb(int r, int g, int b) {
    u32 key = ((u32)iclamp(r, 0, 255) << 16) | ((u32)iclamp(g, 0, 255) << 8) | (u32)iclamp(b, 0, 255);
    int h = (int)((key * 2654435761u) >> 26);
    if (near_cache_v[h] == key && near_cache_k[h]) return near_cache_k[h];
    int best = 1; s32 bd = 0x7FFFFFFF;
    for (int i = 1; i < PAL_N; i++) {
        u32 p = PAL24[i]; int dr = (int)(p >> 16) - (int)(key >> 16), dg = (int)((p >> 8) & 255) - (int)((key >> 8) & 255), db = (int)(p & 255) - (int)(key & 255);
        s32 d = dr * dr * 3 + dg * dg * 4 + db * db * 2;
        if (d < bd) { bd = d; best = i; if (!d) break; }
    }
    near_cache_v[h] = key; near_cache_k[h] = (u8)best;
    return (u8)best;
}
u8 col_hex(u32 x) { return col_rgb(x >> 16, (x >> 8) & 255, x & 255); }
u8 col_shade(u8 c, int k) { u32 p = PAL24[c]; return col_rgb(((p >> 16) * k) >> 8, (((p >> 8) & 255) * k) >> 8, ((p & 255) * k) >> 8); }
u8 col_mix(u8 a, u8 b, int k) {
    u32 p = PAL24[a], q = PAL24[b];
    return col_rgb(((p >> 16) * (256 - k) + (q >> 16) * k) >> 8, (((p >> 8) & 255) * (256 - k) + ((q >> 8) & 255) * k) >> 8, ((p & 255) * (256 - k) + (q & 255) * k) >> 8);
}

/* ---------------------------------------------------------------- pool */
#define POOL_SZ (160 * 1024)
static u8 pool[POOL_SZ] EWRAM_BSS __attribute__((aligned(4)));
static u32 pool_top;
static bmp_t bmp_hdr[256]; static int bmp_n;
u32 pool_mark(void) { return pool_top | ((u32)bmp_n << 24); }
void pool_reset(u32 m) { pool_top = m & 0xFFFFFF; bmp_n = m >> 24; }
bmp_t *bmp_new(int w, int h) {
    u32 sz = ((u32)w * h + 3) & ~3u;
    if (pool_top + sz > POOL_SZ || bmp_n >= 256) return 0;
    bmp_t *b = &bmp_hdr[bmp_n++];
    b->px = pool + pool_top; b->w = w; b->h = h; b->vram = 0;
    memset(b->px, 0, sz); pool_top += sz;
    return b;
}

/* ---------------------------------------------------------------- destino */
static u8 *back_page(void) { return (u8 *)((DISPCNT & 0x10) ? 0x06000000 : 0x0600A000); }
void clip_reset(void) { CX0 = 0; CY0 = 0; CX1 = T.w; CY1 = T.h; }
void clip(int x, int y, int w, int h) { CX0 = imax(0, x); CY0 = imax(0, y); CX1 = imin(T.w, x + w); CY1 = imin(T.h, y + h); }
void origin(int x, int y) { OX = x; OY = y; }
void tgt_screen(void) { SCREEN.px = back_page(); SCREEN.w = SW; SCREEN.h = SH; SCREEN.vram = 1; int same = T.vram; T = SCREEN; if (!same) { clip_reset(); OX = OY = 0; } }
static s16 sv_ox, sv_oy, sv_c[4]; static bmp_t sv_t;
void tgt_bmp(bmp_t *b) { sv_t = T; sv_ox = OX; sv_oy = OY; sv_c[0] = CX0; sv_c[1] = CY0; sv_c[2] = CX1; sv_c[3] = CY1; T = *b; OX = OY = 0; clip_reset(); }
void tgt_back(void) { T = sv_t; OX = sv_ox; OY = sv_oy; CX0 = sv_c[0]; CY0 = sv_c[1]; CX1 = sv_c[2]; CY1 = sv_c[3]; }

IWRAM_CODE void pset(int x, int y, u8 c) {
    x += OX; y += OY;
    if (x < CX0 || x >= CX1 || y < CY0 || y >= CY1) return;
    u8 *p = T.px + y * T.w + x;
    if (!T.vram) { *p = c; return; }
    volatile u16 *q = (u16 *)((u32)p & ~1u);
    *q = (x & 1) ? (u16)((*q & 0x00FF) | (c << 8)) : (u16)((*q & 0xFF00) | c);
}
u8 pget(int x, int y) {
    x += OX; y += OY;
    if ((unsigned)x >= (unsigned)T.w || (unsigned)y >= (unsigned)T.h) return 0;
    const u8 *p = T.px + y * T.w + x;
    if (!T.vram) return *p;
    u16 v = *(u16 *)((u32)p & ~1u); return (x & 1) ? v >> 8 : v & 255;
}
void cls(u8 c) { int ox = OX, oy = OY; OX = OY = 0; rect(0, 0, T.w, T.h, c); OX = ox; OY = oy; }

static volatile u32 dma_fill;
IWRAM_CODE void rect(int x, int y, int w, int h, u8 c) {
    x += OX; y += OY;
    const int cx0 = CX0, cy0 = CY0, cx1 = CX1, cy1 = CY1, tw = T.w, vram = T.vram;
    u8 *const px = T.px;
    if (x < cx0) { w -= cx0 - x; x = cx0; } if (y < cy0) { h -= cy0 - y; y = cy0; }
    if (x + w > cx1) w = cx1 - x; if (y + h > cy1) h = cy1 - y;
    if (w <= 0 || h <= 0) return;
    const u32 c4 = c * 0x01010101u; const u16 c2 = (u16)c4;
    u8 *row = px + y * tw;
    for (int j = 0; j < h; j++, row += tw) {
        int a = x; const int b = x + w;
        if (!vram) { while (a < b && (a & 3)) row[a++] = c; int nw = (b - a) >> 2;
            if (nw >= 8) { dma_fill = c4; REG32(0x040000D4) = (u32)&dma_fill; REG32(0x040000D8) = (u32)(row + a); REG32(0x040000DC) = 0x85000000u | nw; a += nw * 4; }
            while (a + 4 <= b) { *(u32 *)(row + a) = c4; a += 4; } while (a < b) row[a++] = c; continue; }
        if (a & 1) { volatile u16 *q = (u16 *)(row + a - 1); *q = (*q & 0x00FF) | (c << 8); a++; }
        if ((a & 2) && a + 2 <= b) { *(u16 *)(row + a) = c2; a += 2; }
        u32 *w4 = (u32 *)(row + a); int nw = (b - a) >> 2;
        if (nw >= 8) { dma_fill = c4; REG32(0x040000D4) = (u32)&dma_fill; REG32(0x040000D8) = (u32)w4; REG32(0x040000DC) = 0x85000000u | nw; w4 += nw; nw = 0; }
        while (nw >= 4) { w4[0] = c4; w4[1] = c4; w4[2] = c4; w4[3] = c4; w4 += 4; nw -= 4; }
        while (nw--) *w4++ = c4;
        a = (u8 *)w4 - row;
        if (a + 2 <= b) { *(u16 *)(row + a) = c2; a += 2; }
        if (a < b) { volatile u16 *q = (u16 *)(row + a); *q = (*q & 0xFF00) | c; }
    }
}
/* dibuja nrows filas de bits (MSB = izquierda, ancho w) en color c: texto y glifos */
IWRAM_CODE void bits_draw(int x, int y, const u8 *bm, int nrows, int w, u8 c) {
    x += OX; y += OY;
    const int cx0 = CX0, cy0 = CY0, cx1 = CX1, cy1 = CY1, tw = T.w, vram = T.vram; u8 *const px = T.px;
    if (x >= cx1 || x + w <= cx0 || y >= cy1 || y + nrows <= cy0) return;
    if (vram && x >= cx0 && x + w + 1 <= cx1 && y >= cy0 && y + nrows <= cy1) {
        /* camino rápido: dentro del recorte, de dos en dos píxeles */
        const u32 cc = c | (c << 8);
        for (int r = 0; r < nrows; r++) {
            u32 bits = (u32)bm[r] << (8 - w) << 1; /* bit 8 = primer píxel */
            if (!bits) continue;
            int xx = x & ~1; if (x & 1) bits >>= 1; /* alinear: el bit 8 corresponde a xx */
            volatile u16 *h = (u16 *)(px + (y + r) * tw + xx);
            for (; bits & 0x1FF; bits = (bits << 2) & 0x1FF, h++) {
                u32 pr = (bits >> 7) & 3; /* bit1 = píxel izq, bit0 = píxel dcho */
                if (pr == 3) *h = cc; else if (pr == 2) *h = (*h & 0xFF00) | c; else if (pr == 1) *h = (*h & 0x00FF) | (c << 8);
            }
        }
        return;
    }
    for (int r = 0; r < nrows; r++) {
        int yy = y + r; u32 bits = bm[r]; if (!bits || yy < cy0 || yy >= cy1) continue;
        u8 *row = px + yy * tw;
        for (int q = 0; q < w; q++) if (bits & (0x80u >> (q + 8 - w))) {
            int xx = x + q; if (xx < cx0 || xx >= cx1) continue;
            if (!vram) { row[xx] = c; continue; }
            volatile u16 *h = (u16 *)(row + (xx & ~1));
            *h = (xx & 1) ? (u16)((*h & 0x00FF) | (c << 8)) : (u16)((*h & 0xFF00) | c);
        }
    }
}
IWRAM_CODE void rect_lut(int x, int y, int w, int h, const u8 *lut) {
    x += OX; y += OY;
    if (x < CX0) { w -= CX0 - x; x = CX0; } if (y < CY0) { h -= CY0 - y; y = CY0; }
    if (x + w > CX1) w = CX1 - x; if (y + h > CY1) h = CY1 - y;
    if (w <= 0 || h <= 0) return;
    for (int j = 0; j < h; j++) {
        u8 *row = T.px + (y + j) * T.w;
        if (!T.vram) { for (int i = x; i < x + w; i++) row[i] = lut[row[i]]; continue; }
        int a = x & ~1, b = x + w;
        for (; a < b; a += 2) {
            u16 *q = (u16 *)(row + a); u16 v = *q;
            u8 lo = v & 255, hi = v >> 8;
            if (a >= x) lo = lut[lo];
            if (a + 1 < b) hi = lut[hi];
            *q = lo | (hi << 8);
        }
    }
}
void rect_dither(int x, int y, int w, int h, u8 c) {
    for (int j = 0; j < h; j++) for (int i = (x + y + j) & 1; i < w; i += 2) pset(x + i, y + j, c);
}
void frame_rect(int x, int y, int w, int h, u8 c) { rect(x, y, w, 1, c); rect(x, y + h - 1, w, 1, c); rect(x, y, 1, h, c); rect(x + w - 1, y, 1, h, c); }
void line(int x0, int y0, int x1, int y1, u8 c, int t) {
    int dx = iabs(x1 - x0), dy = iabs(y1 - y0), n = dx > dy ? dx : dy;
    if (!n) n = 1;
    for (int k = 0; k <= n; k++) {
        int x = x0 + ((x1 - x0) * k * 2 + (x1 >= x0 ? n : -n)) / (2 * n);
        int y = y0 + ((y1 - y0) * k * 2 + (y1 >= y0 ? n : -n)) / (2 * n);
        if (t == 1) pset(x, y, c); else rect(x, y, t, t, c);
    }
}
static int isqrt(int v) { if (v <= 0) return 0; int r = (int)sqrtf((float)v); while (r * r > v) r--; while ((r + 1) * (r + 1) <= v) r++; return r; }
void circle(int cx, int cy, int r, u8 c) {
    for (int y = -r; y <= r; y++) { int hw = isqrt(r * r - y * y + r); rect(cx - hw, cy + y, hw * 2 + 1, 1, c); }
}
void ring(int cx, int cy, int r, u8 c) {
    int x = r, y = 0, e = 1 - r;
    while (x >= y) {
        pset(cx + x, cy + y, c); pset(cx - x, cy + y, c); pset(cx + x, cy - y, c); pset(cx - x, cy - y, c);
        pset(cx + y, cy + x, c); pset(cx - y, cy + x, c); pset(cx + y, cy - x, c); pset(cx - y, cy - x, c);
        y++; if (e < 0) e += 2 * y + 1; else { x--; e += 2 * (y - x) + 1; }
    }
}
void ellipse(int cx, int cy, int rx, int ry, u8 c) {
    if (ry <= 0 || rx <= 0) return;
    for (int y = -ry; y <= ry; y++) { int hw = (int)(rx * sqrtf(1.0f - (float)(y * y) / (float)(ry * ry)) + 0.5f); rect(cx - hw, cy + y, hw * 2 + 1, 1, c); }
}
void poly(const s16 *p, int n, u8 c) {
    int ymin = 9999, ymax = -9999;
    for (int i = 0; i < n; i++) { if (p[i * 2 + 1] < ymin) ymin = p[i * 2 + 1]; if (p[i * 2 + 1] > ymax) ymax = p[i * 2 + 1]; }
    if (ymin < CY0 - OY) ymin = CY0 - OY; if (ymax > CY1 - 1 - OY) ymax = CY1 - 1 - OY;
    int xs[16];
    for (int y = ymin; y <= ymax; y++) {
        int k = 0; float fy = y + 0.5f;
        for (int i = 0; i < n && k < 16; i++) {
            int j = (i + 1) % n; float y0 = p[i * 2 + 1], y1 = p[j * 2 + 1], x0 = p[i * 2], x1 = p[j * 2];
            if ((fy >= y0 && fy < y1) || (fy >= y1 && fy < y0)) xs[k++] = (int)(x0 + (fy - y0) * (x1 - x0) / (y1 - y0) + 0.5f);
        }
        for (int a = 1; a < k; a++) { int v = xs[a], b = a - 1; while (b >= 0 && xs[b] > v) { xs[b + 1] = xs[b]; b--; } xs[b + 1] = v; }
        for (int a = 0; a + 1 < k; a += 2) rect(xs[a], y, xs[a + 1] - xs[a], 1, c);
    }
}
IWRAM_CODE void blit_part(const bmp_t *b, int sx, int sy, int w, int h, int x, int y, int flip) {
    x += OX; y += OY;
    const int cx0 = CX0, cy0 = CY0, cx1 = CX1, cy1 = CY1, tw = T.w, vram = T.vram, bw = b->w; u8 *const px = T.px;
    int i0 = 0, i1 = w;
    if (x < cx0) i0 = cx0 - x; if (x + w > cx1) i1 = cx1 - x;
    if (i0 >= i1) return;
    for (int j = 0; j < h; j++) {
        int ty = y + j; if (ty < cy0 || ty >= cy1) continue;
        const u8 *src = b->px + (sy + j) * bw;
        u8 *row = px + ty * tw;
        for (int i = i0; i < i1; i++) {
            u8 c = src[flip ? sx + w - 1 - i : sx + i]; if (!c) continue;
            int tx = x + i;
            if (!vram) { row[tx] = c; continue; }
            volatile u16 *q = (u16 *)(row + (tx & ~1));
            *q = (tx & 1) ? (u16)((*q & 0x00FF) | (c << 8)) : (u16)((*q & 0xFF00) | c);
        }
    }
}
void blit(const bmp_t *b, int x, int y, int flip) { if (b) blit_part(b, 0, 0, b->w, b->h, x, y, flip); }
void blit_scaled(const bmp_t *b, int x, int y, int w, int h, int flip) {
    if (!b || w <= 0 || h <= 0) return;
    for (int j = 0; j < h; j++) {
        int sy = j * b->h / h;
        for (int i = 0; i < w; i++) { int sx = i * b->w / w; u8 c = b->px[sy * b->w + (flip ? b->w - 1 - sx : sx)]; if (c) pset(x + i, y + j, c); }
    }
}
void pal_load(u8 *pal, const char *chars, const u8 *cols) { for (int i = 0; chars[i]; i++) pal[(u8)chars[i] & 127] = cols[i]; }
IWRAM_CODE void rows(const char *const *spr, int n, int x, int y, const u8 *pal, int flip) {
    for (int r = 0; r < n; r++) {
        const char *s = spr[r]; int w = strlen(s);
        for (int q = 0; q < w; q++) { char ch = s[q]; if (ch == '.') continue; u8 c = pal[(u8)ch & 127]; if (c) pset(x + (flip ? w - 1 - q : q), y + r, c); }
    }
}
bmp_t *rows_bmp(const char *const *spr, int n, const u8 *pal, int flip) {
    int w = 0; for (int r = 0; r < n; r++) { int l = strlen(spr[r]); if (l > w) w = l; }
    bmp_t *b = bmp_new(w, n); if (!b) return 0;
    tgt_bmp(b); rows(spr, n, 0, 0, pal, flip); tgt_back();
    return b;
}

/* ---------------------------------------------------------------- texto */
static int utf8(const char **ps) {
    const u8 *s = (const u8 *)*ps; int c = *s++;
    if (c >= 0xF0) { c = ((c & 7) << 18) | ((s[0] & 63) << 12) | ((s[1] & 63) << 6) | (s[2] & 63); s += 3; }
    else if (c >= 0xE0) { c = ((c & 15) << 12) | ((s[0] & 63) << 6) | (s[1] & 63); s += 2; }
    else if (c >= 0xC0) { c = ((c & 31) << 6) | (s[0] & 63); s += 1; }
    *ps = (const char *)s; return c;
}
static int glyph(int cp) {
    int a = 0, b = FONT_N - 1;
    while (a <= b) { int m = (a + b) >> 1; if (FONT_CP[m] == cp) return m; if (FONT_CP[m] < cp) a = m + 1; else b = m - 1; }
    return FONT_Q; /* '?' */
}
int text_len(const char *s) { int n = 0; while (*s) { utf8(&s); n++; } return n; }
int text_s(int x, int y, const char *s, u8 col, int align, int sc) {
    int w = text_len(s) * 8 * sc;
    if (align == 1) x -= w / 2; else if (align == 2) x -= w;
    while (*s) {
        int cp = utf8(&s);
        if (cp != ' ') {
            const u8 *g = FONT_BM[glyph(cp)];
            if (sc == 1) bits_draw(x, y, g, 8, 8, col);
            else for (int r = 0; r < 8; r++) { u8 bits = g[r]; if (!bits) continue;
                for (int q = 0; q < 8; q++) if (bits & (0x80 >> q)) rect(x + q * sc, y + r * sc, sc, sc, col); }
        }
        x += 8 * sc;
    }
    return w;
}
int text(int x, int y, const char *s, u8 col, int align) { return text_s(x, y, s, col, align, 1); }
int text_sh(int x, int y, const char *s, u8 col, u8 sh, int align) { text_s(x + 1, y + 1, s, sh, align, 1); return text_s(x, y, s, col, align, 1); }
int text_small(int x, int y, const char *s, u8 col, int align) {
    int w = text_len(s) * 4;
    if (align == 1) x -= w / 2; else if (align == 2) x -= w;
    while (*s) {
        int cp = utf8(&s), k = -1;
        for (int i = 0; i < SMALL_N; i++) if (SMALL_CP[i] == cp) { k = i; break; }
        if (k >= 0) bits_draw(x, y, SMALL_BM[k], 5, 3, col);
        x += 4;
    }
    return w;
}
int text_wrap(int x, int y, const char *s, u8 col, int cols, int lh, int align) {
    char buf[64]; int lines = 0;
    while (*s) {
        while (*s == ' ') s++;
        if (!*s) break;
        const char *ls = s, *brk = 0, *p = s; int n = 0;
        while (*p && *p != '\n') { const char *q = p; utf8(&q); n++; if (n > cols) break; if (*p == ' ') brk = p; p = q; }
        const char *end;
        if (!*p || *p == '\n') end = p; else end = brk && brk > ls ? brk : p;
        int len = end - ls; if (len > 63) len = 63;
        memcpy(buf, ls, len); buf[len] = 0;
        int bx = align == 1 ? x : align == 2 ? x : x;
        text(bx, y + lines * lh, buf, col, align); lines++;
        s = end; if (*s == '\n') s++;
    }
    return lines;
}
char *str_cat(char *d, const char *s) { while (*s) *d++ = *s++; *d = 0; return d; }
char *str_num(char *d, s32 v) {
    char t[12]; int n = 0; if (v < 0) { *d++ = '-'; v = -v; }
    do { t[n++] = '0' + v % 10; v /= 10; } while (v);
    while (n) *d++ = t[--n];
    *d = 0; return d;
}

/* ---------------------------------------------------------------- azar y trigonometría */
static u32 rs = 0x12345678;
u32 rnd(void) { rs ^= rs << 13; rs ^= rs >> 17; rs ^= rs << 5; return rs; }
int rndi(int n) { return n > 0 ? (int)((rnd() >> 8) % (u32)n) : 0; }
float rndf(void) { return (float)(rnd() >> 8) / 16777216.0f; }
s32 isin(int a) { return SIN_T[a & 1023]; }
s32 icos(int a) { return SIN_T[(a + 256) & 1023]; }
float fsin(float a) { int i = (int)(a * 162.974661f); if (a < 0) i--; return SIN_T[i & 1023] / 4096.0f; }
float fcos(float a) { return fsin(a + 1.5707963f); }
float fsqrt(float x) { return x > 0 ? sqrtf(x) : 0; }
float fatan2(float y, float x) { return atan2f(y, x); }

/* ---------------------------------------------------------------- sonido */
#define SND1SWEEP REG16(0x04000060)
#define SND1CNT REG16(0x04000062)
#define SND1FREQ REG16(0x04000064)
#define SND2CNT REG16(0x04000068)
#define SND2FREQ REG16(0x0400006C)
#define SND4CNT REG16(0x04000078)
#define SND4FREQ REG16(0x0400007C)
#define SNDCNT_L REG16(0x04000080)
#define SNDCNT_H REG16(0x04000082)
#define SNDCNT_X REG16(0x04000084)
typedef struct { s16 start, end; u16 hz, hz1; u8 ch, wave, vol, on; } snd_ev;
static snd_ev EV[24]; static u8 nextch; static s32 snd_t;
static int ms2f(int ms) { return (ms * 60 + 500) / 1000; }
static void ev_add(int hz, int hz1, int dur, int del, int ch, int wave, int vol) {
    for (int i = 0; i < 24; i++) if (!EV[i].on) {
        EV[i].start = ms2f(del); EV[i].end = EV[i].start + imax(1, ms2f(dur)); EV[i].hz = hz; EV[i].hz1 = hz1;
        EV[i].ch = ch; EV[i].wave = wave; EV[i].vol = vol; EV[i].on = 1; return;
    }
}
void tone(int hz, int dur, int del, int wave, int vol) { if (hz > 0) ev_add(hz, hz, dur, del, 255, wave, vol); }
void slide(int h0, int h1, int dur) { ev_add(h0, h1, dur, 0, 0, 0, 50); }
void noise(int dur, int vol) { ev_add(1, 1, dur, 0, 3, 0, vol); }
static u16 frq(int hz) { int n = 2048 - 131072 / imax(hz, 64); return (u16)iclamp(n, 0, 2047); }
static u8 chbusy[4];
static void snd_update(int n) {
    for (int i = 0; i < 24; i++) if (EV[i].on) {
        snd_ev *e = &EV[i];
        if (e->start > 0) { e->start -= n; e->end -= n; if (e->start > 0) continue;
            /* empieza */
            if (e->ch == 255) { e->ch = chbusy[0] <= chbusy[1] ? 0 : 1; nextch ^= 1; }
            int v = iclamp(e->vol * 15 / 80, 4, 15);
            u16 duty = e->wave == W_SQUARE ? 2 : e->wave == W_SAW ? 1 : 0;
            if (e->ch == 0) { SND1SWEEP = 0x0008; SND1CNT = (v << 12) | (duty << 6); SND1FREQ = 0x8000 | frq(e->hz); }
            else if (e->ch == 1) { SND2CNT = (v << 12) | (duty << 6); SND2FREQ = 0x8000 | frq(e->hz); }
            else { SND4CNT = (v << 12) | (1 << 8) | 1; SND4FREQ = 0x8000 | 0x0053; }
            chbusy[e->ch] = 1;
        } else { e->end -= n; }
        if (e->start <= 0 && e->ch == 0 && e->hz1 != e->hz) {
            /* glissando lineal hacia hz1 */
            int left = e->end; int cur = e->hz + (e->hz1 - e->hz) / (left > 0 ? left + 1 : 1);
            e->hz = cur; SND1FREQ = frq(cur);
        }
        if (e->end <= 0) {
            e->on = 0; chbusy[e->ch] = 0;
            if (e->ch == 0) { SND1CNT = 0; SND1FREQ = 0x8000; } else if (e->ch == 1) { SND2CNT = 0; SND2FREQ = 0x8000; } else { SND4CNT = 0; SND4FREQ = 0x8000; }
        }
    }
    snd_t += n;
}
void sfx_jump(void) { slide(280, 620, 160); }
void sfx_bump(void) { tone(150, 80, 0, W_SQUARE, 80); }
void sfx_select(void) { tone(660, 50, 0, W_SQUARE, 60); tone(880, 70, 50, W_SQUARE, 60); }
void sfx_open(void) { tone(392, 90, 0, 0, 60); tone(523, 90, 60, 0, 60); tone(659, 90, 120, 0, 60); tone(784, 90, 180, 0, 60); }
void sfx_coin(void) { tone(988, 60, 0, 0, 50); tone(1319, 220, 60, 0, 50); }
void sfx_tick(void) { tone(1200, 30, 0, 0, 25); }
void sfx_clear(void) { static const u16 f[] = {523, 659, 784, 1047, 784, 1047}; for (int i = 0; i < 6; i++) tone(f[i], 140, i * 110, 0, 60); }
void sfx_power(void) { for (int i = 0; i < 12; i++) tone(300 + i * 80, 60, i * 40, 0, 50); }
void sfx_hit(void) { noise(150, 100); tone(80, 200, 50, W_SQUARE, 80); }
void sfx_win(void) { static const u16 f[] = {659, 659, 0, 659, 0, 523, 659, 0, 784}; for (int i = 0; i < 9; i++) tone(f[i], 130, i * 120, 0, 60); tone(392, 400, 1100, 0, 60); }
void sfx_back(void) { tone(392, 60, 0, 0, 60); tone(262, 100, 60, 0, 60); }

/* ---------------------------------------------------------------- guardado SRAM */
const char save_tag[] __attribute__((aligned(4), used)) = "SRAM_V113";
void save_write(const void *d, int n) {
    volatile u8 *s = (volatile u8 *)0x0E000000; const u8 *p = d;
    s[0] = 'F'; s[1] = 'C'; s[2] = 'W'; s[3] = (u8)n;
    for (int i = 0; i < n; i++) s[4 + i] = p[i];
}
int save_read(void *d, int n) {
    volatile u8 *s = (volatile u8 *)0x0E000000; u8 *p = d;
    if (s[0] != 'F' || s[1] != 'C' || s[2] != 'W' || s[3] != (u8)n) return 0;
    for (int i = 0; i < n; i++) p[i] = s[4 + i];
    return 1;
}

/* ---------------------------------------------------------------- bucle */
static u32 last_vbl;
static void vblank_wait(void) { __asm__ volatile("swi 0x05" ::: "r0", "r1", "r2", "r3", "memory"); }
int frame_end(void) {
    vblank_wait();
    DISPCNT ^= 0x10;
    tgt_screen();
    u16 k = (~KEYINPUT) & 0x3FF;
    key_hit = k & ~key_held; key_held = k;
    int n = (int)(vbl_count - last_vbl); last_vbl = vbl_count;
    if (n < 1) n = 1; if (n > 3) n = 3;
    frame_no += n;
    snd_update(n);
    rnd();
    return n;
}
void wait_frames(int n) { while (n > 0) { /* repite la última imagen en ambas páginas */ n -= frame_end(); } }
void fade_level(int l) { BLDCNT = l ? 0x00C4 : 0; BLDY = iclamp(l, 0, 16); }
void fade_to_black(int frames) {
    for (int i = 1; i <= frames; i++) { fade_level(i * 16 / frames); vblank_wait(); }
}

u32 ticks(void) { u16 lo = REG16(0x04000108), hi = REG16(0x0400010C), lo2 = REG16(0x04000108); if (lo2 < lo) hi = REG16(0x0400010C); return ((u32)hi << 16) | lo2; }
void eng_init(void) {
    REG16(0x0400010A) = 0; REG16(0x0400010E) = 0; REG16(0x0400010C) = 0; REG16(0x04000108) = 0;
    REG16(0x0400010E) = 0x0084; REG16(0x0400010A) = 0x0080; /* TM2 a 16,78 MHz, TM3 en cascada */
    WAITCNT = 0x4317; /* ROM más rápida (3/1) y prefetch */
    DISPCNT = 0x0404; /* modo 4, BG2 */
    for (int i = 0; i < 256; i++) PALRAM[i] = PAL15[i];
    memset((void *)0x06000000, 0, 0x14000);
    DISPSTAT = 0x0008; IE = 0x0001; IME = 1;
    SNDCNT_X = 0x80; SNDCNT_L = 0xFF77; SNDCNT_H = 0x0002;
    for (int i = 0; i < 256; i++) { LUT_DARK[i] = i ? col_shade(i, 128) : 0; LUT_LIGHT[i] = i ? col_mix(i, col_rgb(255, 255, 255), 102) : 0; }
    last_vbl = vbl_count;
    tgt_screen();
}
