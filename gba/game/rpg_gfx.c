/* RPG · CATALUNYA: tonos GB, plantillas de mapa, casillas, edificios y sprites cuantizados (gbQuantize/gbSprite). */
#include "rpg_int.h"
#include "gen.h"

extern bmp_t T;
#define PALRAM ((volatile u16 *)0x05000000)

/* ---------------------------------------------------------------- tonos */
u8 TN[4];
static u8 tn_lut_dark[4];
void rpg_tones_init(void) {
    if (PAL_N <= 252) { for (int i = 0; i < 4; i++) TN[i] = 252 + i; }
    else { TN[0] = C(0xe0f8d0); TN[1] = C(0x88c070); TN[2] = C(0x346856); TN[3] = C(0x081820); }
    (void)tn_lut_dark;
}
static u16 to15(u32 v) { return ((v >> 19) & 31) | (((v >> 11) & 31) << 5) | (((v >> 3) & 31) << 10); }
void rpg_tones_set(const u32 *p) { for (int i = 0; i < 4; i++) PALRAM[TN[i]] = to15(p[i]); }
void rpg_tones_restore(void) { for (int i = 0; i < 4; i++) PALRAM[TN[i]] = PAL15[TN[i]]; }
u8 rpg_tq(u32 c) {
    int r = c >> 16, g = (c >> 8) & 255, b = c & 255, l = 299 * r + 587 * g + 114 * b;
    return TN[l > 183600 ? 0 : l > 114750 ? 1 : l > 51000 ? 2 : 3];
}
static void R(int x, int y, int w, int h, u32 rgb) { rect(x, y, w, h, rpg_tq(rgb)); }
static void F(int x, int y, int w, int h, int i) { rect(x, y, w, h, TN[i]); }

/* ---------------------------------------------------------------- plantillas (TPL) */
static const bld_t B0[] = {{2, 2, 5, 3, 1, "FLiiTS HUB", 0}, {12, 2, 5, 3, 0, 0, "PISOS. Aquí vive medio equipo de reparto."},
    {2, 12, 5, 3, 0, 0, "CAFÉ. Un rider pide un cortado para llevar."}, {12, 12, 6, 3, 0, 0, "MERCAT. ¡Hoy hay muchísimos pedidos!"}};
static const bld_t B1[] = {{2, 2, 3, 3, 0, 0, "FORN. Huele a coca de recapte."}, {8, 2, 5, 4, 0, 0, "AJUNTAMENT. Hoy se tramitan permisos."},
    {14, 2, 4, 3, 0, 0, "CASA RURAL. No hay nadie en casa."}, {9, 12, 4, 3, 0, 0, "TAVERNA. Se oye un partido del Barça."}};
static const bld_t B2[] = {{2, 11, 4, 3, 0, 0, "PESCADOR. Cajas de pescado fresco."}, {9, 11, 3, 3, 0, 0, "GELATS. ¡Un helado de turrón, por favor!"},
    {13, 11, 5, 3, 0, 0, "HOSTAL. Recepción abierta las 24 horas."}};
static const slot_t S0[] = {{6, 9, D_RIGHT}, {12, 6, D_DOWN}, {16, 11, D_LEFT}, {3, 16, D_RIGHT}, {14, 8, D_LEFT}};
static const slot_t S1[] = {{15, 12, D_LEFT}, {13, 6, D_DOWN}, {4, 12, D_RIGHT}, {11, 8, D_LEFT}, {16, 16, D_UP}};
static const slot_t S2[] = {{11, 2, D_DOWN}, {3, 9, D_RIGHT}, {16, 15, D_UP}, {13, 6, D_LEFT}, {6, 16, D_RIGHT}};
static const slot_t S3[] = {{4, 10, D_UP}, {7, 10, D_UP}, {10, 10, D_UP}, {13, 10, D_UP}, {16, 10, D_UP}, {9, 13, D_UP}};
tpl_t TPL[4] = {
    {B0, 4, {9, 12, D_UP}, S0, 5, {17, 9, D_LEFT}, 0, 0},
    {B1, 4, {1, 9, D_RIGHT}, S1, 5, {17, 9, D_LEFT}, 0, 0},
    {B2, 3, {1, 9, D_RIGHT}, S2, 5, {17, 8, D_LEFT}, 0, 0},
    {0, 0, {9, 7, D_DOWN}, S3, 6, {0, 0, 0}, 0, 0}};
char GRID[4][MH][MW + 1];
static s8 SIGN[4][2];
static void PR(int t, int x, int y, int w, int h, char ch) {
    for (int j = y; j < y + h; j++) for (int i = x; i < x + w; i++) if (j >= 0 && j < MH && i >= 0 && i < MW) GRID[t][j][i] = ch;
}
void rpg_tpl_init(void) {
    for (int t = 0; t < 4; t++) {
        for (int y = 0; y < MH; y++) { for (int x = 0; x < MW; x++) GRID[t][y][x] = '.'; GRID[t][y][MW] = 0; }
        PR(t, 0, 0, MW, 1, 'T'); PR(t, 0, MH - 1, MW, 1, 'T'); PR(t, 0, 0, 1, MH, 'T'); PR(t, MW - 1, 0, 1, MH, 'T');
    }
    PR(0, 0, 8, 20, 2, '='); PR(0, 9, 1, 2, 16, '='); PR(0, 4, 5, 1, 3, '='); PR(0, 14, 5, 1, 3, '='); PR(0, 1, 15, 18, 1, '=');
    PR(0, 2, 10, 5, 1, 'F'); PR(0, 13, 10, 3, 1, '~'); PR(0, 16, 5, 2, 2, 'T'); PR(0, 6, 11, 2, 1, 'T'); GRID[0][7][7] = 'S';
    PR(1, 0, 8, 20, 2, '='); PR(1, 3, 5, 1, 3, '='); PR(1, 10, 6, 1, 2, '='); PR(1, 16, 5, 1, 3, '='); PR(1, 8, 10, 1, 5, '='); PR(1, 1, 15, 18, 1, '=');
    PR(1, 2, 11, 5, 3, ','); PR(1, 13, 11, 5, 3, ','); PR(1, 2, 10, 2, 1, 'f'); PR(1, 5, 10, 2, 1, 'f'); PR(1, 1, 16, 18, 1, 'F'); GRID[1][7][6] = 'S';
    PR(2, 0, 0, MW, 4, '~'); PR(2, 0, 4, MW, 2, 's'); PR(2, 0, 4, 1, 2, 'T'); PR(2, 19, 4, 1, 2, 'T'); PR(2, 10, 1, 2, 4, '=');
    PR(2, 0, 8, 20, 2, '='); PR(2, 7, 10, 1, 4, '='); PR(2, 1, 14, 18, 1, '='); PR(2, 1, 15, 18, 2, 's'); PR(2, 3, 6, 1, 1, 'T'); PR(2, 15, 6, 1, 1, 'T'); GRID[2][7][5] = 'S';
    PR(3, 0, 0, MW, MH, '_'); PR(3, 0, 0, MW, 3, 'W'); PR(3, 0, 0, 1, MH, 'W'); PR(3, MW - 1, 0, 1, MH, 'W'); PR(3, 0, MH - 1, MW, 1, 'W');
    PR(3, 7, 5, 6, 1, 'D'); PR(3, 14, 13, 4, 1, 'D'); PR(3, 14, 15, 4, 1, 'D');
    GRID[3][3][1] = 'P'; GRID[3][3][18] = 'P'; GRID[3][16][1] = 'P'; GRID[3][16][18] = 'P'; GRID[3][3][4] = 'S';
    for (int t = 0; t < 4; t++) {
        SIGN[t][0] = -1;
        for (int y = 0; y < MH && SIGN[t][0] < 0; y++) for (int x = 0; x < MW; x++) if (GRID[t][y][x] == 'S') { SIGN[t][0] = x; SIGN[t][1] = y; break; }
        TPL[t].sx = SIGN[t][0]; TPL[t].sy = SIGN[t][1];
    }
}
int rpg_solid_ch(char ch) { return ch == 'T' || ch == '~' || ch == 'f' || ch == 'S' || ch == 'W' || ch == 'D' || ch == 'P'; }

/* ---------------------------------------------------------------- casillas (tileCv) */
static void tuft(int x, int y, int i) { F(x, y, 1, 2, i); F(x + 2, y, 1, 2, i); F(x + 1, y + 1, 1, 1, i); }
static const s8 GV[4][2][2] = {{{3, 4}, {10, 11}}, {{11, 3}, {4, 12}}, {{7, 7}, {-1, -1}}, {{2, 10}, {12, 5}}};
static const s8 RV[4][2][2] = {{{3, 3}, {11, 9}}, {{12, 2}, {5, 12}}, {{8, 6}, {-1, -1}}, {{2, 13}, {13, 6}}};
void rpg_tile(char ch, int v, int x0, int y0) {
    int ox = OX, oy = OY; origin(ox + x0, oy + y0);
    switch (ch) {
    case '.': F(0, 0, 16, 16, 1); for (int k = 0; k < 2; k++) if (GV[v][k][0] >= 0) tuft(GV[v][k][0], GV[v][k][1], 2); break;
    case ',': F(0, 0, 16, 16, 2); for (int y = 0; y < 16; y += 4) for (int x = (y % 8 ? 2 : 0); x < 16; x += 4) { F(x, y + 1, 1, 3, 3); F(x + 2, y + 1, 1, 3, 3); F(x + 1, y, 1, 2, 1); } break;
    case '=': F(0, 0, 16, 16, 0); for (int k = 0; k < 2; k++) if (RV[v][k][0] >= 0) F(RV[v][k][0], RV[v][k][1], 2, 1, 1); break;
    case 's': F(0, 0, 16, 16, 0); for (int y = 1; y < 16; y += 4) for (int x = (y % 8 == 1 ? 1 : 3); x < 16; x += 4) F(x, y, 1, 1, 1); break;
    case '~': { F(0, 0, 16, 16, 2); int o = v * 2;
        F((2 + o) % 16, 4, imin(5, 16 - (2 + o) % 16), 1, 1); F((10 + o) % 16, 4, imin(4, 16 - (10 + o) % 16), 1, 1);
        F((6 + o) % 16, 11, imin(6, 16 - (6 + o) % 16), 1, 1); F((14 + o) % 16, 11, imin(2, 16 - (14 + o) % 16), 1, 1); F((7 + o) % 16, 3, 1, 1, 0); } break;
    case 'F': { static const s8 FP[4][2] = {{3, 4}, {11, 4}, {6, 11}, {13, 12}}; F(0, 0, 16, 16, 1);
        for (int i = 0; i < 4; i++) { int x = FP[i][0], y = FP[i][1], d = (v + i) & 1; F(x + d - 1, y, 3, 1, 0); F(x + d, y - 1, 1, 3, 0); F(x + d, y, 1, 1, 3); F(x, y + 2, 1, 2, 2); } } break;
    case 'T': F(0, 0, 16, 16, 1); F(3, 0, 10, 12, 3); F(1, 1, 14, 10, 3); F(0, 2, 16, 7, 3); F(3, 1, 10, 10, 2); F(1, 2, 14, 8, 2); F(4, 2, 3, 2, 1); F(9, 4, 2, 2, 1);
        F(3, 6, 2, 1, 1); F(7, 7, 2, 1, 1); F(6, 11, 4, 5, 3); F(7, 12, 2, 3, 2); break;
    case 'f': F(0, 0, 16, 16, 1); F(0, 4, 16, 3, 3); F(0, 5, 16, 1, 0); F(0, 10, 16, 3, 3); F(0, 11, 16, 1, 0); F(2, 2, 3, 13, 3); F(3, 3, 1, 11, 0); F(11, 2, 3, 13, 3); F(12, 3, 1, 11, 0); break;
    case 'S': F(0, 0, 16, 16, 1); F(7, 9, 2, 7, 3); F(2, 2, 12, 8, 3); F(3, 3, 10, 6, 0); F(4, 4, 8, 1, 2); F(4, 6, 6, 1, 2); break;
    case '_': F(0, 0, 16, 16, 0); F(0, 15, 16, 1, 1); F(15, 0, 1, 16, 1); if (v == 2) F(6, 6, 2, 2, 1); break;
    case 'W': F(0, 0, 16, 16, 1); F(0, 13, 16, 3, 2); F(0, 15, 16, 1, 3); if (v & 1) { F(3, 2, 10, 8, 3); F(4, 3, 8, 6, 0); F(7, 3, 1, 6, 3); F(4, 6, 8, 1, 3); } break;
    case 'D': F(0, 0, 16, 16, 0); F(0, 2, 16, 9, 3); F(1, 3, 14, 6, 1); F(1, 9, 14, 2, 2); F(1, 11, 2, 5, 3); F(13, 11, 2, 5, 3); break;
    case 'P': F(0, 0, 16, 16, 0); F(4, 10, 8, 6, 3); F(5, 11, 6, 4, 2); F(3, 1, 10, 10, 3); F(4, 2, 8, 8, 2); F(5, 3, 2, 2, 1); F(9, 5, 2, 2, 1); break;
    }
    origin(ox, oy);
}

/* letras FLiiTS (FONT del web) */
static const char *const FN_F[5] = {"111", "100", "110", "100", "100"}, *const FN_L[5] = {"100", "100", "100", "100", "111"},
    *const FN_I[5] = {"1", "0", "1", "1", "1"}, *const FN_T[5] = {"111", "010", "010", "010", "010"}, *const FN_S[5] = {"111", "100", "111", "001", "111"};
static void fliits_word(int x, int y, u8 col) {
    const char *const *gl[6] = {FN_F, FN_L, FN_I, FN_I, FN_T, FN_S};
    for (int k = 0; k < 6; k++) { int w = strlen(gl[k][0]); for (int r = 0; r < 5; r++) for (int q = 0; q < w; q++) if (gl[k][r][q] == '1') pset(x + q, y + r, col); x += w + 1; }
}
void draw_building(const bld_t *b, int ox, int oy) {
    int W = b->w * 16, H = b->h * 16, door = b->w / 2;
#define FB(x, y, w, h, i) F(ox + (x), oy + (y), w, h, i)
    FB(0, 14, W, H - 14, 3); FB(1, 14, W - 2, H - 15, 0); FB(1, H - 3, W - 2, 2, 1);
    FB(-2, -4, W + 4, 20, 3); FB(-1, -3, W + 2, 18, b->hub ? 3 : 2); for (int y = 0; y < 14; y += 4) FB(-1, y, W + 2, 1, b->hub ? 2 : 3); FB(-1, -3, W + 2, 1, b->hub ? 2 : 1);
    for (int r = 1; r < b->h; r++) for (int i = 0; i < b->w; i++) { int tx = i * 16, ty = r * 16;
        if (i == door && r == b->h - 1) { FB(tx + 3, ty + 1, 10, 15, 3); FB(tx + 4, ty + 2, 8, 14, 2); FB(tx + 10, ty + 9, 1, 2, 0); }
        else { FB(tx + 3, ty + 3, 10, 9, 3); FB(tx + 4, ty + 4, 8, 7, 1); FB(tx + 7, ty + 4, 1, 7, 3); FB(tx + 4, ty + 7, 8, 1, 3); FB(tx + 5, ty + 5, 1, 1, 0); } }
    if (b->hub) fliits_word(ox + (W / 2 - 10), oy + 3, G0);
#undef FB
}

/* ---------------------------------------------------------------- mapa pre-dibujado */
bmp_t *MAPB;
static bmp_t *ATL; static const char TCH[] = ".,=s~FTfS_WDP";
void rpg_atlas_init(void) {
    ATL = bmp_new(16, 16 * 13 * 4); if (!ATL) return;
    tgt_bmp(ATL);
    for (int i = 0; i < 13; i++) for (int v = 0; v < 4; v++) { clip(0, (i * 4 + v) * 16, 16, 16); rpg_tile(TCH[i], v, 0, (i * 4 + v) * 16); }
    tgt_back();
}
static void put_tile(char ch, int v, int tx, int ty) {
    const char *p = strchr(TCH, ch); if (!p || !ATL) return;
    const u32 *s = (const u32 *)(ATL->px + ((p - TCH) * 4 + v) * 256); u32 *d = (u32 *)(MAPB->px + ty * 16 * MAPB->w + tx * 16); int st = MAPB->w / 4;
    for (int r = 0; r < 16; r++, s += 4, d += st) { d[0] = s[0]; d[1] = s[1]; d[2] = s[2]; d[3] = s[3]; }
}
void rpg_map_build(int t) {
    for (int ty = 0; ty < MH; ty++) for (int tx = 0; tx < MW; tx++) { char ch = GRID[t][ty][tx];
        put_tile(ch, ch == '~' || ch == 'F' ? (frame_no >> 4) & 3 : (tx * 7 + ty * 13) & 3, tx, ty); }
    tgt_bmp(MAPB);
    for (int i = 0; i < TPL[t].nbld; i++) { const bld_t *b = &TPL[t].bld[i]; draw_building(b, b->x * 16, b->y * 16); }
    tgt_back();
}
void rpg_map_anim(int t, int wv) {
    for (int ty = 0; ty < MH; ty++) for (int tx = 0; tx < MW; tx++) { char ch = GRID[t][ty][tx]; if (ch == '~' || ch == 'F') put_tile(ch, wv, tx, ty); }
}
/* copia una ventana 160x144 de un bitmap (ancho múltiplo de 4) a la pantalla GB, por palabras de 32 bits */
IWRAM_CODE void rpg_blit_win(const bmp_t *src, int cx, int cy) {
    const int sw = src->w, sh = (cx & 3) * 8;
    for (int j = 0; j < GBH; j++) {
        const u32 *s = (const u32 *)(src->px + (cy + j) * sw + (cx & ~3));
        u32 *d = (u32 *)(T.px + (GBY + j) * SW + GBX);
        if (!sh) { for (int i = 0; i < GBW / 4; i += 4) { d[i] = s[i]; d[i + 1] = s[i + 1]; d[i + 2] = s[i + 2]; d[i + 3] = s[i + 3]; } }
        else { const int rs = 32 - sh; u32 a = s[0];
            for (int i = 0; i < GBW / 4; i++) { u32 b = s[i + 1]; d[i] = (a >> sh) | (b << rs); a = b; } }
    }
}
void rpg_blit_map(int cx, int cy) { rpg_blit_win(MAPB, cx, cy); }

/* ---------------------------------------------------------------- caché de sprites */
typedef struct { u32 key; bmp_t *b; } ce_t;
static ce_t CE[96]; static int ce_n; static u32 ce_mark;
void rpg_cache_forget(void) { ce_n = 0; ce_mark = 0; }
void rpg_cache_reset(void) { if (ce_n || ce_mark) pool_reset(ce_mark); ce_mark = pool_mark(); ce_n = 0; }
void rpg_cache_frame(void) {
    u32 m = pool_mark();
    if ((m & 0xFFFFFF) - (ce_mark & 0xFFFFFF) > 32 * 1024 || (m >> 24) - (ce_mark >> 24) > 120 || ce_n > 90) { pool_reset(ce_mark); ce_n = 0; }
}
static bmp_t *ce_get(u32 key) { for (int i = 0; i < ce_n; i++) if (CE[i].key == key) return CE[i].b; return 0; }
static bmp_t *ce_new(u32 key, int w, int h) {
    bmp_t *b = bmp_new(w, h);
    if (!b) { pool_reset(ce_mark); ce_n = 0; b = bmp_new(w, h); if (!b) return 0; }
    if (ce_n < 96) { CE[ce_n].key = key; CE[ce_n].b = b; ce_n++; }
    return b;
}

/* rowsPal con paleta de 24 bits y cuantización directa a tonos */
static void rows_t(const char *const *spr, int n, int ox, int oy, const u32 *pal, int flip, int stride) {
    int w = strlen(spr[0]); u8 lut[128]; memset(lut, 0, sizeof lut);
    for (int r = 0; r < n; r++) for (int q = 0; q < w; q++) { u8 ch = (u8)spr[r][q] & 127; if (!lut[ch] && pal[ch] && ch != '.') lut[ch] = rpg_tq(pal[ch] & 0xFFFFFF); }
    for (int r = 0; r < n; r++) { int y = oy + r; if (y < 0 || y >= T.h) continue; u8 *row = T.px + y * T.w;
        for (int q = 0; q < w; q++) {
            u8 c = lut[(u8)spr[r][q] & 127]; if (!c) continue;
            int x = flip ? w - 1 - q : q;
            if (stride && r >= n - 5) x += (q < w / 2 ? -stride : stride);
            x += ox; if (x >= 0 && x < T.w) row[x] = c;
        } }
}
#define HAS 0x1000000u /* marca de entrada presente (permite el negro) */
static void pal32(u32 *d, const u8 *p) { for (int i = 0; i < 128; i++) d[i] = p[i] ? (col_of(p[i]) | HAS) : 0; }
static void pal32_over(u32 *d, const u8 *p) { for (int i = 0; i < 128; i++) if (p[i]) d[i] = col_of(p[i]) | HAS; }

/* espalda (backRows) */
static char BACKR[5][24][17];
static const char *BACKP[5][24];
static u8 back_ok[5];
static const char *const *back_rows(int which) { /* 0..3 crew (con mochila), 4 candidato */
    if (back_ok[which]) return BACKP[which];
    const char *const *src = which < 4 ? CREW[which].spr : CAND_T; int pack = which < 4;
    int cnt[128] = {0}, ord[128], no = 0;
    for (int r = 0; r < 5; r++) for (const char *p = src[r]; *p; p++) if (*p != '.' && *p != 'K') { if (!cnt[(u8)*p]) ord[no++] = *p; cnt[(u8)*p]++; }
    char hair = 'H'; int best = 0; for (int i = 0; i < no; i++) if (cnt[ord[i]] > best) { best = cnt[ord[i]]; hair = ord[i]; }
    for (int r = 0; r < 24; r++) { for (int q = 0; q < 16; q++) { char ch = src[r][q]; BACKR[which][r][q] = (r >= 3 && r <= 9 && ch != '.' && ch != 'K') ? hair : ch; } BACKR[which][r][16] = 0; }
    if (pack) { for (int y = 12; y <= 18; y++) for (int x = 4; x <= 11; x++) BACKR[which][y][x] = (y == 12 || y == 18 || x == 4 || x == 11) ? 'K' : (x == 5 || x == 10) ? 'r' : 'R';
        BACKR[which][14][7] = BACKR[which][15][7] = BACKR[which][16][7] = 'W'; }
    for (int r = 0; r < 24; r++) BACKP[which][r] = BACKR[which][r];
    back_ok[which] = 1; return BACKP[which];
}
bmp_t *spr_crew(int ci, int view, int stride) {
    u32 key = 0x01000000u | (ci << 8) | (view << 4) | (stride + 1); bmp_t *b = ce_get(key); if (b) return b;
    b = ce_new(key, 16, 24); if (!b) return 0;
    static u32 p[128]; pal32(p, P_PAL);
    tgt_bmp(b); rows_t(view == D_UP ? back_rows(ci) : CREW[ci].spr, 24, 0, 0, p, view == D_LEFT, stride); tgt_back();
    return b;
}
bmp_t *spr_armor(void) {
    u32 key = 0x06000000u; bmp_t *b = ce_get(key); if (b) return b;
    b = ce_new(key, 16, 24); if (!b) return 0;
    static u32 p[128]; pal32(p, P_PAL); pal32_over(p, P_ARMOR);
    tgt_bmp(b); rows_t(back_rows(1), 24, 0, 0, p, 0, 0); tgt_back();
    return b;
}
typedef struct { u32 shirt, hair, skin; } npcc_t;
static const npcc_t NPCC[10] = {{0x2f6b8f, 0x241a18, 0xb97950}, {0xd57b35, 0x302018, 0xc98b63}, {0x347b63, 0x211915, 0x9f6748}, {0x6654a0, 0x201a19, 0xbd8058},
    {0xa34b43, 0x30221c, 0xad714f}, {0xc45a83, 0x30201d, 0xc48b69}, {0x4b82b8, 0x2b1d19, 0xd09a76}, {0x8b763d, 0x211916, 0xa96d4b},
    {0x7b8b9b, 0x76513a, 0xefc49b}, {0x547d58, 0x583a25, 0xd7a27b}};
static u32 shade(u32 h, int f100) {
    u32 r = ((h >> 16) * f100 + 50) / 100, g = (((h >> 8) & 255) * f100 + 50) / 100, b = ((h & 255) * f100 + 50) / 100;
    return (r << 16) | (g << 8) | b;
}
static void npc_pal(u32 *p, int id, int beard, int red, int gloves_unused) {
    (void)gloves_unused; const npcc_t *d = &NPCC[id]; u32 Cc = red ? 0xd62839 : d->shirt;
    memset(p, 0, 128 * 4);
    p['K'] = 0x1b1212 | HAS; p['H'] = d->hair | HAS; p['S'] = d->skin | HAS; p['s'] = shade(d->skin, 80) | HAS; p['B'] = d->hair | HAS;
    p['C'] = Cc | HAS; p['c'] = shade(Cc, 75) | HAS; p['P'] = 0x2d3a5c | HAS; p['G'] = 0x3a2a1e | HAS; p['b'] = (beard ? d->hair : d->skin) | HAS;
}
bmp_t *spr_npc(int id, int view, int stride, int red) {
    u32 key = 0x02000000u | (id << 12) | (view << 8) | ((stride + 1) << 4) | red; bmp_t *b = ce_get(key); if (b) return b;
    b = ce_new(key, 16, 24); if (!b) return 0;
    static u32 p[128]; npc_pal(p, id, id % 2, red, 0);
    tgt_bmp(b); rows_t(view == D_UP ? back_rows(4) : CAND_T, 24, 0, 0, p, view == D_LEFT, stride); tgt_back();
    return b;
}
/* drawPerson con lookOf(id) en un lienzo 40x28 */
static void helmet_t(int sx, int sy) {
    int mn = 99, mx = -1, top = 99;
    for (int r = 0; r < 5; r++) { const char *row = CAND_T[r]; for (int q = 0; row[q]; q++) if (row[q] != '.') { if (q < mn) mn = q; if (q > mx) mx = q; if (r < top) top = r; } }
    int x0 = sx + mn, x1 = sx + mx, y0 = sy + top - 2, y1 = sy + 3;
    R(x0 + 1, y0, x1 - x0 - 1, 1, 0x1b1212); R(x0, y0 + 1, x1 - x0 + 1, y1 - y0, 0x1b1212);
    R(x0 + 1, y0 + 1, x1 - x0 - 1, y1 - y0 - 1, 0xe63946); R(x0 + 2, y0 + 1, 3, 1, 0xff8a92); R(x1 - 2, y1, 5, 1, 0x1b1212);
}
static void fbox_t(int x, int y) {
    R(x, y, 21, 13, 0x1b1212); R(x + 1, y + 1, 19, 11, 0xd62839); R(x + 1, y + 1, 19, 1, 0xff5a66); R(x + 1, y + 10, 19, 2, 0xa51d2b);
    fliits_word(x + 1, y + 4, rpg_tq(0xffffff));
}
bmp_t *spr_person(int id, int gear) {
    u32 key = 0x03000000u | (id << 8) | gear; bmp_t *b = ce_get(key); if (b) return b;
    b = ce_new(key, 40, 28); if (!b) return 0;
    tgt_bmp(b);
    const npcc_t *d = &NPCC[id]; int x = 20, y = 3, beard = id % 2;
    u32 pal[128]; memset(pal, 0, sizeof pal);
    pal['K'] = 0x1b1212 | HAS; pal['H'] = d->hair | HAS; pal['S'] = d->skin | HAS; pal['s'] = shade(d->skin, 80) | HAS; pal['B'] = d->hair | HAS;
    pal['C'] = ((gear & G_CHAQ) ? 0xd62839 : d->shirt) | HAS; pal['c'] = ((gear & G_CHAQ) ? 0xa51d2b : shade(d->shirt, 75)) | HAS; pal['P'] = 0x2d3a5c | HAS; pal['G'] = 0x3a2a1e | HAS;
    if (gear & G_MOCH) fbox_t(x - 20, y + 5);
    for (int r = 0; r < 24; r++) for (int q = 0; q < 16; q++) {
        char ch = CAND_T[r][q]; if (ch == 'b') ch = beard ? 'B' : 'S'; u32 col = pal[(u8)ch & 127]; if (!col) continue;
        if ((gear & G_GUANT) && ch == 'S' && r >= 14 && r <= 15) col = 0x2a2a2a;
        pset(x + q, y + r, rpg_tq(col & 0xFFFFFF));
    }
    if (gear & G_CASCO) helmet_t(x, y);
    tgt_back();
    return b;
}
bmp_t *spr_dog(int f, int flip) {
    u32 key = 0x04000000u | (f << 4) | flip; bmp_t *b = ce_get(key); if (b) return b;
    b = ce_new(key, 28, 18); if (!b) return 0;
    static u32 p[128]; pal32(p, P_DOG);
    tgt_bmp(b); rows_t(f ? DOG1 : DOG0, 18, 0, 0, p, flip, 0); tgt_back();
    return b;
}
bmp_t *spr_boss(int kind, int good) {
    u32 key = 0x05000000u | (kind << 4) | good; bmp_t *b = ce_get(key); if (b) return b;
    b = ce_new(key, kind == 2 ? 36 : 18, 28); if (!b) return 0;
    static u32 p[128];
    tgt_bmp(b);
    if (kind == 0) { pal32(p, P_MPAL_I); if (!good) pal32_over(p, P_EVIL); rows_t(MARTI, 28, 0, 0, p, 1, 0); }
    else if (kind == 1) {
        memset(p, 0, sizeof p);
        if (good) { p['K'] = 0x1b1212; p['H'] = 0x2b1b14; p['S'] = 0xe0ac69; p['s'] = 0xc08a50; p['B'] = 0x2b1b14; p['C'] = 0x2f4f8f; p['c'] = 0x243d70; p['P'] = 0x56657e; p['G'] = 0x3a2a1e; p['b'] = 0x2b1b14; }
        else { p['K'] = 0x1b1212; p['H'] = 0x120c0a; p['S'] = 0xc79a86; p['s'] = 0xa87c6a; p['B'] = 0x120c0a; p['C'] = 0x3b1f4f; p['c'] = 0x2a1538; p['P'] = 0x241a2e; p['G'] = 0x120b0c; p['b'] = 0xc79a86; }
        for (int i = 0; i < 128; i++) if (p[i]) p[i] |= HAS;
        rows_t(CAND_T, 24, 1, 4, p, 0, 0);
    } else {
        pal32(p, P_MPAL_A); if (!good) pal32_over(p, P_EVIL); rows_t(MARTI_B, 28, 0, 0, p, 1, 0);
        pal32(p, P_FPAL); if (!good) pal32_over(p, P_EVIL); rows_t(FRANCO, 24, 20, 4, p, 1, 0);
    }
    tgt_back();
    return b;
}
bmp_t *spr_scaled(bmp_t *src, u32 key, int w, int h) {
    if (!src) return 0;
    key |= 0x80000000u; bmp_t *b = ce_get(key); if (b) return b;
    /* la fuente podría desaparecer si la caché se vacía: se reserva primero y se comprueba */
    u32 m = pool_mark();
    if ((m & 0xFFFFFF) + ((w * h + 3) & ~3) + 64 > 160 * 1024) return 0;
    b = ce_new(key, w, h); if (!b) return 0;
    u32 fx = ((u32)src->w << 16) / w, fy = ((u32)src->h << 16) / h, sy = 0;
    for (int j = 0; j < h; j++, sy += fy) { const u8 *s = src->px + (sy >> 16) * src->w; u8 *d = b->px + j * w; u32 sx = 0;
        for (int i = 0; i < w; i++, sx += fx) d[i] = s[sx >> 16]; }
    return b;
}

/* ---------------------------------------------------------------- piezas del combate */
static u32 isqrt32(u32 v) { u32 r = 0, b = 1u << 30; while (b > v) b >>= 2; while (b) { if (v >= r + b) { v -= r + b; r = (r >> 1) + b; } else r >>= 1; b >>= 2; } return r; }
void px_ellipse(int cx, int cy, int rx, int ry, u8 col) {
    for (int dy = -ry; dy <= ry; dy++) {
        u32 v = (u32)(rx * rx) * (u32)(ry * ry - dy * dy); int h = (int)((isqrt32(v * 4) + ry) / (2 * ry));
        rect(cx - h, cy + dy, 2 * h, 1, col);
    }
}
/* blit con transparencia (0) recortado a RC (coordenadas del destino) */
s16 RC[4] = {GBX, GBY, GBX + GBW, GBY + GBH};
void rpg_clip(int x, int y, int w, int h) { clip(x, y, w, h); RC[0] = imax(0, x); RC[1] = imax(0, y); RC[2] = imin(T.w, x + w); RC[3] = imin(T.h, y + h); }
IWRAM_CODE void rpg_blit(const bmp_t *b, int x, int y) {
    if (!b) return;
    x += OX; y += OY;
    int x0 = imax(x, RC[0]), x1 = imin(x + b->w, RC[2]), y0 = imax(y, RC[1]), y1 = imin(y + b->h, RC[3]);
    if (x0 >= x1) return;
    for (int j = y0; j < y1; j++) {
        const u8 *s = b->px + (j - y) * b->w + (x0 - x); u8 *row = T.px + j * T.w; int a = x0;
        if (!T.vram) { for (; a < x1; a++, s++) if (*s) row[a] = *s; continue; }
        if (a & 1) { if (*s) { u16 *q = (u16 *)(row + a - 1); *q = (*q & 0xff) | (*s << 8); } a++; s++; }
        for (; a + 1 < x1; a += 2, s += 2) { u32 p0 = s[0], p1 = s[1];
            if (p0 && p1) *(u16 *)(row + a) = p0 | (p1 << 8);
            else if (p0 | p1) { u16 *q = (u16 *)(row + a); u32 v = *q; if (p0) v = (v & 0xff00) | p0; if (p1) v = (v & 0xff) | (p1 << 8); *q = v; } }
        if (a < x1 && *s) { u16 *q = (u16 *)(row + a); *q = (*q & 0xff00) | *s; }
    }
}
void draw_bags(int x0, int y0, int dizzy, int stk) {
    static const u32 BC[2][2] = {{0xffc244, 0xd99a1c}, {0x2bb673, 0x1d8a55}};
    for (int i = 0; i < 2; i++) { int x = x0 + i * 40, y = y0; u32 col = BC[i][0], dk = BC[i][1], K = 0x1b1212;
#define f(a, b, w, h, c) R(x + (a), y + (b), w, h, c)
        f(0, 0, 36, 28, K); f(1, 1, 34, 26, col); f(1, 21, 34, 5, dk); f(5, 27, 7, 6, K); f(24, 27, 7, 6, K);
        if (!dizzy) { f(7, 8, 7, 7, 0xffffff); f(22, 8, 7, 7, 0xffffff); f(10, 10, 3, 3, K); f(24, 10, 3, 3, K); f(5, 5, 9, 2, K); f(22, 5, 9, 2, K); f(8, 18, 20, 4, K);
            f(10, 19, 1, 2, 0xffffff); f(14, 19, 1, 2, 0xffffff); f(18, 19, 1, 2, 0xffffff); f(22, 19, 1, 2, 0xffffff); }
        else { for (int k = 0; k < 6; k++) { f(7 + k, 8 + k, 1, 1, K); f(12 - k, 8 + k, 1, 1, K); f(22 + k, 8 + k, 1, 1, K); f(27 - k, 8 + k, 1, 1, K); } f(10, 20, 16, 1, K); }
        for (int s = 0; s < stk; s++) { int sx = 2 + ((s * 7 + i * 3) % 27), sy = 2 + ((s * 5 + i * 2) % 17); f(sx, sy, 6, 5, K); f(sx + 1, sy + 1, 4, 3, 0xd62839); f(sx + 2, sy + 1, 1, 3, 0xffffff); }
#undef f
    }
}
void draw_contract(int x, int y, int n, int stamp, int stampR) {
    rect(x, y, 28, 36, G3); rect(x + 1, y + 1, 26, 34, G0);
    fliits_word(x + 4, y + 4, G3);
    for (int r = 0; r < 4; r++) rect(x + 4, y + 12 + r * 4, r == 3 ? 12 : 20, 1, G2);
    for (int i = 0; i < n; i++) { float s = fsin(i / 1.6f) * 2; rect(x + 5 + i, y + 29 + (int)(s < 0 ? s - .5f : s + .5f), 1, 1, G3); }
    if (stamp) { px_ellipse(x + 21, y + 27, stampR, stampR, G2); rect(x + 20, y + 24, 1, 6, G0); rect(x + 20, y + 24, 3, 1, G0); rect(x + 20, y + 27, 2, 1, G0); }
}
void draw_coin(int x, int y, int t) {
    float c = fcos(t / 6.f); if (c < 0) c = -c; int w = imax(1, (int)(c * 7 + .5f));
    px_ellipse(x, y, w + 1, 8, G3); if (w > 1) { px_ellipse(x, y, w, 7, G1); rect(x - 1, y - 4, 1, 6, G0); }
}
void draw_doccard(int x, int y) {
    rect(x, y, 22, 16, G3); rect(x + 1, y + 1, 20, 14, G0); rect(x + 3, y + 3, 6, 8, G1);
    rect(x + 11, y + 4, 8, 1, G2); rect(x + 11, y + 7, 7, 1, G2); rect(x + 11, y + 10, 5, 1, G2);
}
void draw_contract_icon(int x, int y) {
    static const char *const CT[8] = {"KKKKKKKK", "KWWWWWWK", "KWKKKKWK", "KWWWWWWK", "KWKKKWWK", "KWWWWRRK", "KWWWWRRK", "KKKKKKKK"};
    for (int r = 0; r < 8; r++) for (int q = 0; q < 8; q++) { char ch = CT[r][q];
        rect(x + q * 2, y + r * 2, 2, 2, rpg_tq(ch == 'K' ? 0x111111 : ch == 'W' ? 0xffffff : 0xd62839)); }
}
