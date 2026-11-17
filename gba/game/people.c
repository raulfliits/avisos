#include "people.h"

u8 P_PAL[128], P_DOG[128], P_FPAL[128], P_MPAL[128], P_MPAL_I[128], P_MPAL_A[128], P_EVIL[128], P_ARMOR[128], P_IPAL[128];
void people_init(void) {
    pal_load(P_PAL, PAL_CH, PAL_COL); pal_load(P_DOG, DOGPAL_CH, DOGPAL_COL);
    pal_load(P_FPAL, FPAL_CH, FPAL_COL); pal_load(P_MPAL, MPAL_CH, MPAL_COL); pal_load(P_MPAL_I, MPAL_I_CH, MPAL_I_COL);
    pal_load(P_MPAL_A, MPAL_A_CH, MPAL_A_COL); pal_load(P_EVIL, EVIL_CH, EVIL_COL); pal_load(P_ARMOR, ARMOR_CH, ARMOR_COL);
    pal_load(P_IPAL, IPAL_CH, IPAL_COL);
}
void draw_spr(const char *const *spr, int n, int x, int y, int flip, int stride) {
    for (int r = 0; r < n; r++) {
        const char *row = spr[r]; int w = strlen(row);
        for (int q = 0; q < w; q++) {
            char ch = row[q]; if (ch == '.') continue; u8 col = P_PAL[(u8)ch & 127]; if (!col) continue;
            int qx = flip ? w - 1 - q : q;
            if (stride && r >= n - 5) qx += (q < w / 2 ? -stride : stride);
            pset(x + qx, y + r, col);
        }
    }
}
void draw_pal(const char *const *spr, int n, int x, int y, const u8 *pal, int flip) { rows(spr, n, x, y, pal, flip); }
void draw_pal_s(const char *const *spr, int n, int x, int y, const u8 *pal, int flip, int sc) {
    if (sc == 1) { rows(spr, n, x, y, pal, flip); return; }
    for (int r = 0; r < n; r++) { const char *row = spr[r]; int w = strlen(row);
        for (int q = 0; q < w; q++) { char ch = row[q]; if (ch == '.') continue; u8 col = pal[(u8)ch & 127]; if (!col) continue;
            rect(x + (flip ? w - 1 - q : q) * sc, y + r * sc, sc, sc, col); } }
}
void draw_evil(const char *const *spr, int n, int x, int y, const u8 *pal, int flip, int sc) {
    u8 aura = (frame_no >> 2) & 1 ? C(0x9b30ff) : C(0x5a1a8a);
    int w = strlen(spr[0]);
    for (int r = 0; r < n; r++) for (int q = 0; q < w; q++) if (spr[r][q] != '.') rect(x + (flip ? w - 1 - q : q) * sc - 2, y + r * sc - 2, sc + 4, sc + 4, aura);
    u8 p[128]; memcpy(p, pal, 128); for (int i = 0; i < 128; i++) if (P_EVIL[i]) p[i] = P_EVIL[i];
    draw_pal_s(spr, n, x, y, p, flip, sc);
    for (int r = 0; r < n; r++) for (int q = 0; q < w; q++) if (spr[r][q] == 'E') {
        int qq = flip ? w - 1 - q : q, left = qq < w / 2;
        for (int k = 0; k < 3; k++) rect(x + (qq - 1 + k) * sc, y + (r - 1) * sc + (left ? k : 2 - k) * sc / 3 - sc / 3, sc, imax(1, sc / 2), C(0x120b0c));
    }
}
void draw_dog(int f, int x, int y, int flip) { rows(f & 1 ? DOG1 : DOG0, 18, x, y, P_DOG, flip); }

void wheel(int cx, int cy, int r, int f) {
    for (int y = -r; y <= r; y++) for (int x = -r; x <= r; x++) {
        float d = fsqrt((float)(x * x + y * y)); if (d > r + .4f) continue;
        pset(cx + x, cy + y, d > r - 1.4f ? C(0x1b1212) : d > r - 2.4f ? C(0x3a3a44) : C(0xcfd6e2));
    }
    if (r > 3) {
        int a = (f & 7) * 128; s32 ca = icos(a), sa = isin(a);
        for (int k = 1; k < r - 2; k++) {
            int dx = (ca * k + (ca >= 0 ? 2048 : -2048)) / 4096, dy = (sa * k + (sa >= 0 ? 2048 : -2048)) / 4096;
            pset(cx + dx, cy + dy, C(0x6a7082)); pset(cx - dx, cy - dy, C(0x6a7082));
        }
    }
    pset(cx, cy, C(0x1b1212));
}
static const char *const FB_F[5] = {"111", "100", "110", "100", "100"}, *const FB_L[5] = {"100", "100", "100", "100", "111"},
    *const FB_I[5] = {"1", "0", "1", "1", "1"}, *const FB_T[5] = {"111", "010", "010", "010", "010"}, *const FB_S[5] = {"111", "100", "111", "001", "111"};
void fbox(int x, int y) {
    rect(x, y, 21, 13, C(0x1b1212)); rect(x + 1, y + 1, 19, 11, C(0xd62839));
    rect(x + 1, y + 1, 19, 1, C(0xff5a66)); rect(x + 1, y + 10, 19, 2, C(0xa51d2b));
    const char *const *gl[6] = {FB_F, FB_L, FB_I, FB_I, FB_T, FB_S};
    int tx = x + 1;
    for (int k = 0; k < 6; k++) { int w = strlen(gl[k][0]); for (int r = 0; r < 5; r++) for (int q = 0; q < w; q++) if (gl[k][r][q] == '1') pset(tx + q, y + 4 + r, C(0xffffff)); tx += w + 1; }
}
int helmet(int sx, int sy, const char *const *spr, int helm) {
    int mn = 99, mx = -1, top = 99;
    for (int r = 0; r < 5; r++) { const char *row = spr[r]; int w = strlen(row); for (int q = 0; q < w; q++) if (row[q] != '.') { if (q < mn) mn = q; if (q > mx) mx = q; if (r < top) top = r; } }
    int x0 = sx + mn, x1 = sx + mx, y0 = sy + top - 2, y1 = sy + helm;
    rect(x0 + 1, y0, x1 - x0 - 1, 1, C(0x1b1212)); rect(x0, y0 + 1, x1 - x0 + 1, y1 - y0, C(0x1b1212));
    rect(x0 + 1, y0 + 1, x1 - x0 - 1, y1 - y0 - 1, C(0xe63946));
    rect(x0 + 2, y0 + 1, 3, 1, C(0xff8a92));
    rect(x1 - 2, y1, 5, 1, C(0x1b1212));
    return y0;
}
static void draw_rows_upto(const char *const *spr, int x, int y, int r1) {
    for (int r = 0; r <= r1; r++) { const char *row = spr[r]; for (int q = 0; row[q]; q++) { char ch = row[q]; if (ch == '.') continue; u8 c = P_PAL[(u8)ch & 127]; if (c) pset(x + q, y + r, c); } }
}

typedef struct { int key; u8 vi, f; rider_t r; } rcache_t;
static rcache_t RC[48]; static int rc_n;
void people_cache_reset(void) { rc_n = 0; extern void person_cache_reset(void); person_cache_reset(); }
const rider_t *compose_rider_c(const crew_t *c, int key, int vi, int f) {
    f &= 7;
    for (int i = 0; i < rc_n; i++) if (RC[i].key == key && RC[i].vi == vi && RC[i].f == f) return &RC[i].r;
    const int W = 68, H = 44;
    bmp_t *b = bmp_new(W, H); if (!b) return 0;
    tgt_bmp(b);
    const int ox = 4; /* g.translate(4,0) */
    u8 pants = P_PAL[(u8)c->pants], sleeve = P_PAL[(u8)c->sleeve], skin = P_PAL[(u8)c->skin], K = C(0x1b1212);
    int sx, sy, ax, hx, hy;
#define R(a, b, w, h, col) rect(ox + (a), (b), (w), (h), (col))
#define L(a, b, c2, d, col, t) line(ox + (a), (b), ox + (c2), (d), (col), (t))
    if (vi == 0) {
        sx = 17; sy = 10; ax = 36; hx = 38; hy = 17;
        fbox(ox + sx - 20, sy + 5);
        wheel(ox + 24, 39, 4, f);
        R(16, 26, 19, 11, K); R(17, 27, 17, 9, C(0xeef0f4)); R(17, 31, 17, 2, C(0xe63946));
        R(17, 25, 13, 2, C(0x2a2a33));
        R(28, 34, 14, 3, K); R(29, 35, 12, 1, C(0x555a66));
        draw_rows_upto(c->spr, ox + sx, sy, 16);
        R(sx + 5, 22, 12, 5, K); R(sx + 13, 24, 5, 10, K); R(sx + 6, 23, 10, 3, pants); R(sx + 14, 25, 3, 8, pants);
        R(sx + 13, 32, 5, 2, C(0x2a2018));
        R(39, 21, 8, 16, K); R(40, 22, 6, 14, C(0xeef0f4)); R(40, 28, 6, 2, C(0xe63946)); R(45, 23, 2, 2, C(0xffd23f));
        L(42, 21, 41, 16, K, 2); R(36, 15, 9, 2, K);
        wheel(ox + 48, 39, 4, f);
    } else if (vi == 1) {
        sx = 19; sy = 7; ax = 33; hx = 38; hy = 19;
        fbox(ox + sx - 20, sy + 5);
        wheel(ox + 20, 37, 6, f); wheel(ox + 46, 37, 6, f);
        u8 fr = C(0x1f9e8f);
        L(20, 36, 31, 36, fr, 2); L(31, 36, 27, 24, fr, 2); L(27, 25, 40, 24, fr, 2); L(31, 36, 41, 28, fr, 2); L(20, 36, 27, 25, fr, 1); L(41, 24, 46, 37, fr, 2);
        L(41, 24, 40, 19, K, 2); R(36, 18, 7, 2, K); R(23, 22, 8, 2, K);
        draw_rows_upto(c->spr, ox + sx, sy, 16);
        L(sx + 8, 22, sx + 16, 26, K, 4); L(sx + 16, 26, sx + 13, 34, K, 3);
        L(sx + 9, 23, sx + 16, 27, pants, 2); L(sx + 16, 27, sx + 14, 34, pants, 2);
        R(sx + 12, 34, 4, 2, C(0x2a2018)); L(31, 36, sx + 14, 35, K, 1);
    } else {
        sx = 20; sy = 14; ax = 31; hx = 40; hy = 18;
        fbox(ox + sx - 20, sy + 6);
        L(45, 38, 43, 16, K, 2); R(38, 15, 8, 2, K);
        R(13, 37, 35, 3, K); R(14, 38, 33, 1, C(0x3a3a44)); R(16, 37, 4, 1, C(0x58c4ff));
        wheel(ox + 16, 41, 2, f); wheel(ox + 46, 41, 2, f);
        draw_rows_upto(c->spr, ox + sx, sy, 23);
    }
    int shx = sx + 12, shy = sy + 11;
    L(shx, shy - 1, hx, hy - 1, K, 4); L(shx, shy, hx, hy, sleeve, 2);
    R(hx, hy - 1, 3, 3, skin);
    int topY = helmet(ox + sx, sy, c->spr, c->helm);
#undef R
#undef L
    tgt_back();
    rcache_t *e = rc_n < 48 ? &RC[rc_n++] : &RC[47];
    e->key = key; e->vi = vi; e->f = f;
    e->r.b = b; e->r.ax = ax + 4; e->r.h = H - 1 - topY; e->r.W = W; e->r.H = H; e->r.bxc = sx - 6; e->r.byt = sy + 5;
    return &e->r;
}
const rider_t *compose_rider(int ci, int vi, int f) { return compose_rider_c(&CREW[ci], ci, vi, f); }

static char rep_rows[6][24][17]; static const char *rep_ptr[6][24]; static crew_t rep_c[6]; static u8 rep_ok[6];
const crew_t *rep_crew(int L) {
    if (rep_ok[L]) return &rep_c[L];
    u32 S = 0; /* LOOKS[L].S como color: miramos el rojo del color de la paleta */
    S = col_of(LOOKS[L].S) >> 16;
    int dark = S < 0xb0; char sk = dark ? 'D' : 'S', sk2 = dark ? 'd' : 's';
    for (int r = 0; r < 24; r++) {
        for (int q = 0; q < 16; q++) {
            char ch = CAND_T[r][q];
            switch (ch) { case 'S': ch = sk; break; case 's': ch = sk2; break; case 'C': ch = 'R'; break; case 'c': ch = 'r'; break;
                case 'P': ch = 'N'; break; case 'b': ch = LOOKS[L].beard ? 'H' : sk; break; }
            rep_rows[L][r][q] = ch;
        }
        rep_rows[L][r][16] = 0; rep_ptr[L][r] = rep_rows[L][r];
    }
    rep_c[L].spr = rep_ptr[L]; rep_c[L].pants = 'N'; rep_c[L].sleeve = 'R'; rep_c[L].skin = sk; rep_c[L].helm = 3; rep_c[L].dog = 0; rep_c[L].name = "";
    rep_ok[L] = 1; return &rep_c[L];
}

void draw_person(int li, int x, int y, int flip, int stride, int gear, int nrows) {
    const look_t *lk = &LOOKS[li % 6];
    u8 pal[128]; memset(pal, 0, 128);
    pal['K'] = C(0x1b1212); pal['H'] = lk->H; pal['S'] = lk->S; pal['s'] = lk->s; pal['B'] = lk->H;
    pal['C'] = (gear & G_CHAQ) ? C(0xd62839) : lk->C; pal['c'] = (gear & G_CHAQ) ? C(0xa51d2b) : lk->c; pal['P'] = lk->P; pal['G'] = C(0x3a2a1e);
    if (gear & G_MOCH) fbox(flip ? x + 15 : x - 20, y + 5);
    int R = nrows ? nrows : 24;
    for (int r = 0; r < R; r++) { const char *row = CAND_T[r];
        for (int q = 0; q < 16; q++) { char ch = row[q]; if (ch == 'b') ch = lk->beard ? 'B' : 'S'; u8 col = pal[(u8)ch & 127]; if (!col) continue;
            if ((gear & G_GUANT) && ch == 'S' && r >= 14 && r <= 15) col = C(0x2a2a2a);
            int qx = flip ? 15 - q : q; if (stride && r >= 19) qx += (q < 8 ? -stride : stride);
            pset(x + qx, y + r, col); } }
    if (gear & G_CASCO) helmet(x, y, CAND_T, 3);
}
typedef struct { u8 look, gear, flip; bmp_t *b; } pcache_t;
static pcache_t PC[40]; static int pc_n;
void person_cache_reset(void) { pc_n = 0; }
bmp_t *person_bmp(int look, int gear, int flip) {
    gear &= ~G_MOCH; look %= 6;
    for (int i = 0; i < pc_n; i++) if (PC[i].look == look && PC[i].gear == gear && PC[i].flip == flip) return PC[i].b;
    bmp_t *b = bmp_new(16, 27); if (!b) return 0;
    tgt_bmp(b); draw_person(look, 0, 3, flip, 0, gear, 24); tgt_back();
    if (pc_n < 40) { PC[pc_n].look = look; PC[pc_n].gear = gear; PC[pc_n].flip = flip; PC[pc_n].b = b; pc_n++; }
    return b;
}
void draw_worker(int li, int x, int y, int evil, int sc) {
    const look_t *lk = &LOOKS[li % 6];
    u8 pal[128]; memset(pal, 0, 128);
    pal['K'] = C(0x1b1212); pal['H'] = lk->H; pal['S'] = lk->S; pal['s'] = lk->s; pal['B'] = lk->H; pal['C'] = lk->C; pal['c'] = lk->c; pal['P'] = lk->P; pal['G'] = C(0x3a2a1e);
    char rws[15][17]; const char *rp[15];
    for (int r = 0; r < 15; r++) { for (int q = 0; q < 16; q++) { char ch = CAND_T[r][q]; rws[r][q] = ch == 'b' ? (lk->beard ? 'B' : 'S') : ch; } rws[r][16] = 0; rp[r] = rws[r]; }
    if (evil) {
        u8 aura = (frame_no >> 2) & 1 ? C(0x9b30ff) : C(0x5a1a8a);
        for (int r = 0; r < 15; r++) for (int q = 0; q < 16; q++) if (rp[r][q] != '.') rect(x + q * sc - 2, y + r * sc - 2, sc + 4, sc + 4, aura);
        pal['C'] = C(0x3b1f4f); pal['c'] = C(0x2a1538);
        draw_pal_s(rp, 15, x, y, pal, 0, sc);
        rect(x + 6 * sc, y + 5 * sc, sc, sc, C(0xff2a2a)); rect(x + 9 * sc, y + 5 * sc, sc, sc, C(0xff2a2a));
        rect(x + 5 * sc, y + 4 * sc, 2 * sc, imax(1, sc / 2), C(0x120b0c)); rect(x + 9 * sc, y + 4 * sc, 2 * sc, imax(1, sc / 2), C(0x120b0c));
    } else {
        draw_pal_s(rp, 15, x, y, pal, 0, sc);
        rect(x + 7 * sc, y + 8 * sc, 2 * sc, sc, lk->S); rect(x + 6 * sc, y + 8 * sc, sc, sc, C(0x1b1212)); rect(x + 9 * sc, y + 8 * sc, sc, sc, C(0x1b1212)); rect(x + 7 * sc, y + 9 * sc, 2 * sc, sc, C(0x1b1212));
    }
}
void draw_item(int it, int x, int y, int sc) {
    static const char *const *const IT[4] = {ITEM_CASCO, ITEM_CHAQUETA, ITEM_GUANTES, ITEM_MOCHILA};
    static const u8 IH[4] = {ITEM_CASCO_H, ITEM_CHAQUETA_H, ITEM_GUANTES_H, ITEM_MOCHILA_H};
    draw_pal_s(IT[it & 3], IH[it & 3], x, y, P_IPAL, 0, sc);
}
void big_coin(int x, int y, int t) {
    int w = (int)(fcos(t / 10.0f) * 18.0f + (fcos(t / 10.0f) < 0 ? -0.5f : 0.5f)); w = imax(2, iabs(w));
    int h = 22, l = x - (w + 1) / 2;
    rect(l - 1, y - 1, w + 2, h + 2, C(0x1b1212)); rect(l, y, w, h, C(0xc9971a));
    rect(l + 1, y + 1, imax(0, w - 2), h - 2, C(0xffd23f));
    if (w > 12) { rect(l + 2, y + 2, 2, h - 6, C(0xfff3b0)); rect(x - 3, y + 5, 7, 2, C(0xc9971a)); rect(x - 3, y + 5, 2, 12, C(0xc9971a)); rect(x - 3, y + 10, 5, 2, C(0xc9971a)); }
    if ((t >> 3) & 1) { rect(l + w + 3, y - 4, 1, 5, C(0xffffff)); rect(l + w + 1, y - 2, 5, 1, C(0xffffff)); }
}
void draw_stand(int ci, int x, int y) {
    draw_spr(CREW[ci].spr, 24, x, y, 0, 0);
    if (CREW[ci].dog) draw_dog(0, x + 18, y + 6, 1);
}
