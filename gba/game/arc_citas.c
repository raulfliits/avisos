/* RAÜL · ESCÁNER DE CITAS (como el control de un aeropuerto) — ARC_GAMES[1] del web. */
#include "arcade.h"

#define W 192
#define H 160
extern bmp_t T; /* destino actual del motor (eng.c) */

static const char *const SCAN_TODAY = "MIÉ 12/11";
static const char *const BAD[4] = {"JUE 13/11", "LUN 10/11", "MIÉ 19/11", "VIE 14/11"};
static const char *const HOURS[4] = {"10:00", "12:00", "16:00", "19:30"};
static const s8 SPOTS[6][2] = {{8, 13}, {11, 19}, {4, 22}, {3, 12}, {12, 9}, {8, 24}};

/* ---- estado (global para que el bot de pruebas lo lea) ---- */
#define K_OK 0
#define K_MAL 1
#define K_SIN 2
#define PH_ENTER 0
#define PH_SCAN 1
#define PH_LEAVE 2
#define F_NONE 0
#define F_HAPPY 1
#define F_ANGRY 2
#define F_SAD 3
typedef struct { s32 kind, look, hour, spot; const char *date; } ct_person_t;
ct_person_t ct_q[8];
s32 ct_idx, ct_phase, ct_found, ct_beepT, ct_reactT, ct_leaveDir, ct_face, ct_led, ct_reactRaul;
float ct_px, ct_sx, ct_sy;
static const char *ct_react;
s32 ct_pf[6], ct_n[4]; s32 ct_maxn, ct_draws, ct_cyc, ct_maxcyc; static u32 ct_lastf;

/* bitmaps: persona a x3 (48x81) y Raül a x2 (32x48), y filas de fondo */
/* bitmaps en el pool (se piden en el primer init de cada partida: MG.t == 0; los reintentos los reutilizan):
   fondo fijo 192x160 (30 KB), persona a x3 (48x81) y Raül a x2 (32x48) */
static bmp_t *BGB, *P3B, *RA2B, *BARB, *CARDB; static s32 card_key = -1; static s32 p3_key = -1;

static void ct_init(void) {
    s32 kinds[8] = {K_OK, K_MAL, K_SIN, K_OK, K_MAL, K_OK, K_SIN, K_MAL};
    for (int i = 7; i > 0; i--) { int j = rndi(i + 1); s32 t = kinds[i]; kinds[i] = kinds[j]; kinds[j] = t; } /* .sort(()=>Math.random()-.5) */
    for (int i = 0; i < 8; i++) {
        ct_person_t *P = &ct_q[i]; P->kind = kinds[i]; P->look = (i * 5 + 1) % 6;
        P->date = P->kind == K_OK ? SCAN_TODAY : P->kind == K_MAL ? BAD[i % 4] : 0; P->hour = i % 4; P->spot = (i * 5) % 6;
    }
    ct_idx = 0; ct_phase = PH_ENTER; ct_px = -24; ct_sx = 118; ct_sy = 60; ct_found = 0; ct_beepT = 0; ct_react = 0; ct_reactRaul = 0; ct_reactT = 0;
    ct_leaveDir = 1; ct_face = F_NONE; ct_led = 0; p3_key = -1; card_key = -1;
    if (MG.t == 0 || !BGB) {
        BGB = bmp_new(W, H); P3B = bmp_new(48, 81); RA2B = bmp_new(32, 48); BARB = bmp_new(W, 12); CARDB = bmp_new(84, 31); /* fila múltiplo de 4 */
        if (BARB) { tgt_bmp(BARB); cls(C(0x120b0c)); arc_text(W / 2, 2, "A: PASA · B: NO PASA", C(0xffffff), 1, 8); tgt_back(); }
        if (RA2B) { tgt_bmp(RA2B); draw_pal_s(CREW[1].spr, 24, 0, 0, P_PAL, 1, 2); tgt_back(); }
        if (BGB) { /* pared, suelo, puertas, cartel FORM y arco del escáner (lo que no cambia) */
            u8 K = C(0x1b1212);
            tgt_bmp(BGB);
            rect(0, 14, W, 96, C(0xe8dcc6)); for (int x = 0; x < W; x += 24) rect(x, 14, 1, 96, C(0xd4c6ac));
            rect(0, 110, W, 50, C(0x9a9286)); for (int x = 0; x < W; x += 16) rect(x, 110, 1, 50, C(0x857d71)); rect(0, 126, W, 1, C(0x857d71)); rect(0, 142, W, 1, C(0x857d71));
            rect(2, 48, 26, 62, K); rect(4, 50, 22, 60, C(0x6e4527)); rect(22, 80, 2, 3, C(0xffd23f));
            rect(168, 48, 22, 62, K); rect(170, 50, 18, 60, C(0x2f6b3a)); arc_text(179, 38, "FORM", C(0x2f6b3a), 1, 8); rect(173, 70, 12, 10, C(0xffffff));
            rect(58, 24, 6, 88, C(0x555a66)); rect(128, 24, 6, 88, C(0x555a66)); rect(58, 20, 76, 8, C(0x555a66));
            /* barra superior (nada la tapa): "HOY: MIÉ 12/11" fijo; el contador se pinta en cada fotograma */
            rect(0, 0, W, 14, C(0x120b0c)); { char b[24]; str_cat(str_cat(b, "HOY: "), SCAN_TODAY); arc_text(4, 3, b, C(0xffd23f), 0, 8); }
            tgt_back();
        }
    }
    ct_maxn = 0; ct_draws = 0; ct_lastf = frame_no; ct_maxcyc = 0; ct_n[1] = ct_n[2] = ct_n[3] = 0;
}

static void ct_press(int b) {
    if (ct_phase != PH_SCAN || (b != B_A && b != B_B)) return;
    ct_person_t *P = &ct_q[ct_idx]; int pass = b == B_A, valid = P->kind == K_OK, ok = pass == valid;
    if (ok) { MG.c1++; MG.hires = MG.c1; sfx_coin(); ct_face = F_HAPPY; ct_reactRaul = 1; ct_react = pass ? "¡A FORMACIÓN!" : P->kind == K_SIN ? "SIN CITA NO SE ENTRA" : "¡HOY NO ES TU DÍA!"; }
    else { MG.c2++; sfx_hit();
        if (pass) { ct_face = F_ANGRY; ct_reactRaul = 1; ct_react = P->kind == K_SIN ? "¡¡NO TENÍA CITA!!" : "¡¡NO ERA SU DÍA!!"; }
        else { ct_face = F_SAD; ct_reactRaul = 0; ct_react = "¡¡TENGO CITA HOY!!"; } }
    ct_reactT = 90; ct_leaveDir = pass ? 1 : -1; ct_phase = PH_LEAVE;
}

static void ct_update(void) {
    ct_person_t *P = &ct_q[ct_idx < 8 ? ct_idx : 7];
    if (ct_phase == PH_ENTER) { ct_px += 1.3f; if (ct_px >= 72) { ct_px = 72; ct_phase = PH_SCAN; ct_found = 0; } }
    else if (ct_phase == PH_SCAN) {
        if (key_held & K_LEFT) ct_sx -= 1.6f; if (key_held & K_RIGHT) ct_sx += 1.6f; if (key_held & K_UP) ct_sy -= 1.6f; if (key_held & K_DOWN) ct_sy += 1.6f;
        ct_sx = fclamp(ct_sx, 56, 140); ct_sy = fclamp(ct_sy, 26, 112);
        if (P->date) {
            float tx = 72 + SPOTS[P->spot][0] * 3, ty = 30 + SPOTS[P->spot][1] * 3, d = fsqrt((ct_sx - tx) * (ct_sx - tx) + (ct_sy - ty) * (ct_sy - ty));
            if (d < 7 && !ct_found) { ct_found = 1; tone(1400, 80, 0, W_SQUARE, 60); tone(1800, 120, 80, W_SQUARE, 60); }
            if (d < 46 && --ct_beepT <= 0) { int r = (int)(d / 1.6f + .5f); ct_beepT = r > 4 ? r : 4; tone(900 + (int)((46 - d) * 12), 30, 0, W_SQUARE, 35); ct_led = 4; }
        }
        if (ct_led > 0) ct_led--;
    } else if (ct_phase == PH_LEAVE) {
        if (ct_reactT > 40) ct_reactT--;
        else { ct_px += ct_leaveDir * 1.6f; ct_reactT--;
            if (ct_px > 200 || ct_px < -30) { ct_idx++; ct_face = F_NONE; ct_react = 0;
                if (MG.c2 >= 3) { arc_lose("¡3 FALLOS EN LA PUERTA!"); return; }
                if (ct_idx >= 8) { arc_win(); return; }
                ct_phase = PH_ENTER; ct_px = -24; ct_sx = 118; ct_sy = 60; } }
    }
}

/* copia opaca del fondo (ancho W) a todo el lienzo: ldm/stm de 8 palabras */
IWRAM_CODE static void copy_bg(u8 *scr, const u8 *src) {
    for (int y = 0; y < H; y++) {
        const u32 *s = (const u32 *)(src + y * W); u32 *d = (u32 *)(scr + y * SW);
        if (((u32)d & 3) == 0) {
            for (int k = 0; k < W / 32; k++)
                __asm__ volatile("ldmia %0!, {r3-r10}\n\tstmia %1!, {r3-r10}" : "+r"(s), "+r"(d) : : "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "memory");
        } else { u16 *d2 = (u16 *)d; const u16 *s2 = (const u16 *)s; for (int i = 0; i < W / 2; i++) d2[i] = s2[i]; }
    }
}
/* blit con transparencia (0) directo a VRAM, de dos en dos píxeles; recorta al lienzo W x H */
IWRAM_CODE static void tblit(u8 *scr, const bmp_t *b, int x, int y) {
    int i0 = x < 0 ? -x : 0, i1 = x + b->w > W ? W - x : b->w; /* columnas a 0 = transparentes */
    if (i0 >= i1) return;
    for (int j = 0; j < b->h; j++) {
        int yy = y + j; if (yy < 0 || yy >= H) continue;
        const u8 *s = b->px + j * b->w; u8 *row = scr + yy * SW;
        int i = i0;
        if ((x + i) & 1) { u8 c = s[i]; if (c) { u16 *q = (u16 *)(row + x + i - 1); *q = (*q & 0xff) | (c << 8); } i++; }
        for (; i + 1 < i1; i += 2) {
            u32 a = s[i], c = s[i + 1]; if (!(a | c)) continue;
            u16 *q = (u16 *)(row + x + i);
            if (a && c) *q = a | (c << 8); else if (a) *q = (*q & 0xff00) | a; else *q = (*q & 0xff) | (c << 8);
        }
        if (i < i1) { u8 c = s[i]; if (c) { u16 *q = (u16 *)(row + x + i); *q = (*q & 0xff00) | c; } }
    }
}

static void ct_draw(void) {
    u32 t0 = ticks(); ct_draws++;
    { u32 d = frame_no - ct_lastf; ct_lastf = frame_no; if (ct_draws > 30 && (s32)d > ct_maxn && d < 10) ct_maxn = d; if (ct_draws > 30) ct_n[d > 3 ? 3 : d]++; }
    u8 K = C(0x1b1212); int sv_uiw = UI_W; UI_W = W;
    ct_person_t *P = &ct_q[ct_idx < 8 ? ct_idx : 7];
    u8 *scr = T.px + OY * SW + OX;
    if (BGB) copy_bg(scr, BGB->px); else rect(0, 0, W, H, C(0xe8dcc6));
    ct_pf[1] = ticks() - t0;
    rect(92, 22, 8, 3, ct_phase == PH_SCAN && ct_led > 0 ? C(0x62d26f) : C(0x9aa3b5));
    if (ct_idx < 8) {
        int walk = ct_phase != PH_SCAN ? ((MG.t >> 3) & 1 ? 1 : -1) : 0, flip = ct_leaveDir < 0 && ct_phase == PH_LEAVE;
        s32 key = P->look * 2 + flip;
        if (key != p3_key) { /* drawImage(img,0,0,16,27,x,30,48,81): escala x3 entera */
            bmp_t *img = person_bmp(P->look, 0, flip); p3_key = key;
            if (P3B) for (int r = 0; r < 27; r++) for (int q = 0; q < 16; q++) { u8 c = img ? img->px[r * 16 + q] : 0;
                for (int j = 0; j < 3; j++) { u8 *d = P3B->px + (r * 3 + j) * 48 + q * 3; d[0] = d[1] = d[2] = c; } }
        }
        float fr = ct_px + .5f; int px = (int)fr; if (fr < px) px--; /* Math.round */
        ct_pf[2] = ticks() - t0;
        if (P3B) tblit(scr, P3B, px, 30);
        ct_pf[3] = ticks() - t0;
        if (walk) rect(px + (walk > 0 ? 12 : 28), 108, 8, 3, C(0x9a9286));
        if (ct_found && P->date) {
            int tx = 72 + SPOTS[P->spot][0] * 3, ty = 30 + SPOTS[P->spot][1] * 3; rect(tx - 3, ty - 2, 6, 4, C(0xffd23f));
            if (card_key != ct_idx && CARDB) { card_key = ct_idx; tgt_bmp(CARDB);
                rect(0, 0, 82, 31, K); rect(1, 1, 80, 29, C(0xffffff)); arc_text(41, 3, "CITA", C(0xd62839), 1, 8);
                text(41, 13, P->date, K, 1); text(41, 22, HOURS[P->hour], K, 1); tgt_back(); }
            if (CARDB) tblit(scr, CARDB, 1, 16);
        }
        if (ct_reactT > 0 && ct_react && !ct_reactRaul) arc_bubble(px + 24, 18, ct_react, C(0xff8a92), C(0x120b0c));
    }
    /* escáner de mano */
    if (ct_phase == PH_SCAN) {
        int x = (int)(ct_sx + .5f), y = (int)(ct_sy + .5f);
        rect(x - 7, y - 4, 14, 8, K); rect(x - 6, y - 3, 12, 6, C(0x333333)); rect(x - 2, y - 2, 4, 2, ct_led > 0 ? C(0x62d26f) : C(0xd62839));
        rect(x + 6, y - 1, 16, 3, K); rect(x + 16, y - 1, 4, 3, C(0xffd23f));
    }
    /* Raül vigila la puerta */
    ct_pf[4] = ticks() - t0;
    if (RA2B) tblit(scr, RA2B, 136, 104);
    ct_pf[5] = ticks() - t0;
    if (ct_face == F_ANGRY) { rect(142, 110, 8, 2, C(0xd62839)); rect(154, 110, 8, 2, C(0xd62839)); arc_text(152, 90, "!!", C(0xff4a5c), 1, 8); }
    if (ct_face == F_HAPPY) arc_text(152, 92, "OK", C(0x62d26f), 1, 8);
    if (ct_reactT > 0 && ct_react && ct_reactRaul) arc_bubble(140, 92, ct_react, ct_face == F_ANGRY ? C(0xff8a92) : C(0x62d26f), C(0x120b0c));
    { char b[8], *o = str_num(b, imin(ct_idx + 1, 8)); o = str_cat(o, "/"); str_num(o, 8); arc_text(W - 4, 3, b, C(0xffffff), 2, 8); }
    if (ct_phase == PH_SCAN) { if (BARB) tblit(scr, BARB, 0, H - 12); else { rect(0, H - 12, W, 12, C(0x120b0c)); arc_text(W / 2, H - 10, "A: PASA · B: NO PASA", C(0xffffff), 1, 8); } }
    UI_W = sv_uiw;
    ct_cyc = ticks() - t0; if (ct_draws > 30 && ct_cyc > ct_maxcyc) ct_maxcyc = ct_cyc;
}

static void ct_progress(char *out) {
    char *o = str_cat(out, "ACIERTOS "); o = str_num(o, MG.c1); o = str_cat(o, " · FALLOS "); o = str_num(o, MG.c2); str_cat(o, "/3");
}

const arc_def_t ARC_DEF1 = {"CONTROL DE CITAS", W, H, "FLECHAS: MOVER EL ESCÁNER · A: PASA A FORMACIÓN · B: NO PASA. Solo pasa quien tiene cita HOY.",
                            ct_init, ct_update, ct_draw, ct_press, ct_progress};
