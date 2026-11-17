/* Oficina de Raül: citas y turnos (startRaul/raulStep/drawRaul del web). Escena guionizada.
   En 240 de ancho las gradas y los sitios de la derecha se desplazan 16 px a la izquierda (el web dibuja a 256). */
#include "plat.h"

#define RSH 16 /* desplazamiento de la parte derecha */
static const char *const SHIFTS[11] = {"LUN 12–16h", "LUN 19–23h", "MAR 12–16h", "MAR 19–23h", "MIÉ 12–16h", "JUE 19–23h", "VIE 19–23h", "SÁB 12–16h", "SÁB 19–23h", "DOM 12–16h", "DOM 19–23h"};
typedef struct { float x, y, bx; u8 look, kind, flip, moving, ok, seated; s16 sx, sy; char code[5]; const char *shift; } rp_t;
typedef struct { float x, y; int t; s8 p; } env_t;
static struct { int t, lid, next, shown, ordered; float rx; u8 typing, seated; rp_t ppl[11]; env_t envs[12]; int nenv; } rs;
static int rc1, rc2;
static bmp_t *rbg;

static void raul_anchor(bub_t *b, int *x, int *y) {
    if (b->an == AN_RAUL) { *x = iround(rs.rx) + b->ref; *y = b->wy; }
}
static void mk_code(char *o) { static const char A[] = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789"; for (int i = 0; i < 4; i++) o[i] = A[rndi(32)]; o[4] = 0; }

static void raul_start(void) {
    memset(&rs, 0, sizeof rs); clear_bubbles(); nfx = 0; pcam = 0;
    static const u8 QL[5] = {0, 1, 2, 3, 5}, SL[3] = {5, 0, 2};
    for (int i = 0; i < 8; i++) { rp_t *p = &rs.ppl[i]; p->x = p->bx = 98 + i * 10; p->y = GY; p->look = QL[i % 5]; p->kind = 0; p->flip = 1; }
    static const s16 SP[3][2] = {{206 - RSH, GY - 10}, {226 - RSH, GY - 22}, {244 - RSH, GY - 34}};
    for (int i = 0; i < 3; i++) { rp_t *p = &rs.ppl[8 + i]; p->x = p->bx = SP[i][0]; p->y = SP[i][1]; p->look = SL[i]; p->kind = 1; p->flip = 0; }
    int k = 0;
    for (int r = 2; r >= 0; r--) for (int c = 0; c < 4; c++) { if (k < 11) { rs.ppl[k].sx = 202 - RSH + c * 14; rs.ppl[k].sy = GY - 10 - r * 12; } k++; }
    for (int i = 0; i < 11; i++) { mk_code(rs.ppl[i].code); rs.ppl[i].shift = SHIFTS[i % 11]; }
    rs.rx = -12; rc1 = rc2 = 0;
}
static int raul_step(void) {
    int t = ++rs.t; pfr++;
    if (t < 70) { rs.rx += 1; if (rs.rx > 56) rs.rx = 56; }
    if (!rs.shown) for (int i = 0; i < 11; i++) { rp_t *p = &rs.ppl[i]; if (p->ok) continue;
        if (!p->kind) p->x = p->bx + fsin((pfr + i * 40) / 6.0f) * 1.2f;
        else { float a = (pfr + i * 50) / 15.0f; p->x = p->bx + fsin(a) * 6; p->flip = fcos(a) < 0; p->moving = 1; } }
    if (t % 26 == 0 && t < 135) { static const char *const L[5] = {"¿Hay cita?", "¡Yo primero!", "¿Cuándo empiezo?", "¡Shhh!", "¿Dónde firmo?"};
        int k = t / 26; rp_t *p = &rs.ppl[(k * 3) % 11]; say(L[k % 5], AN_FIX, 0, iround(p->x), iround(p->y - 26), 40, BC_CAND); }
    if (t == 72) { say("¡¿Pero esto qué es?!", AN_RAUL, 0, 0, GY - 28, 70, BC_BAD); sfx_hit(); }
    if (t == 110) { rs.seated = 1; sfx_select(); }
    if (t >= 110 && t <= 118) rs.lid = t - 110;
    if (t == 122) { say("¡A TECLEAR!", AN_RAUL, 0, 0, GY - 34, 60, BC_OK); rs.typing = 1; }
    if (rs.typing) { if (t % 3 == 0) tone(900 + rndi(700), 20, 0, W_SQUARE, 25);
        if (t % 2 == 0) { pfx_t *p = fx_add(FX_KEY); if (p) { p->x = 74 + rndf() * 10; p->y = GY - 26; p->vy = -.5f - rndf() * .7f; p->c = rndf() < .5f ? C(0x62d26f) : C(0xffffff); p->life = 28; } } }
    if (t >= 140 && rs.next < 11 && (t - 140) % 20 == 0) { env_t *e = &rs.envs[rs.nenv++]; e->x = 78; e->y = GY - 26; e->p = rs.next++; e->t = 0; }
    int j = 0;
    for (int i = 0; i < rs.nenv; i++) { env_t *e = &rs.envs[i]; rp_t *p = &rs.ppl[(int)e->p]; e->t++; e->x += (p->x - e->x) * .16f; e->y += ((p->y - 34) - e->y) * .16f;
        if (e->t >= 20) { p->ok = 1; rc1++; sfx_coin(); char b[24]; char *q = str_cat(b, "Tu código: "); str_cat(q, p->code); say(b, AN_FIX, 0, iround(p->x), iround(p->y - 30), 70, BC_MAIL); continue; }
        rs.envs[j++] = *e; }
    rs.nenv = j;
    if (t == 175) { rp_t *p = &rs.ppl[6]; say("¿Puedo coger otra cita?", AN_FIX, 0, iround(p->x), iround(p->y - 30), 80, BC_CAND); }
    if (t == 205) { say("✗ Solo una cita por persona", AN_RAUL, 0, 0, GY - 34, 90, BC_NO); sfx_back(); }
    if (t == 245) { rp_t *p = &rs.ppl[9]; say("¡Vengo sin cita!", AN_FIX, 0, iround(p->x), iround(p->y - 30), 80, BC_CAND); }
    if (t == 275) { say("✗ Sin cita no se entra", AN_RAUL, 0, 0, GY - 34, 90, BC_NO); sfx_back(); }
    if (t == 315) say("Cancelación MAR 12h", AN_FIX, 0, 150, GY - 62, 80, BC_CAND);
    if (t == 340) { say("✓ Hueco rellenado automáticamente", AN_RAUL, 40, 0, GY - 34, 100, BC_OK); sfx_power(); }
    if (rs.next >= 11 && !rs.nenv && !rs.shown && t > 360) { rs.shown = t; rs.typing = 0; sfx_win(); }
    if (rs.shown && rc2 < 11 && (t - rs.shown) % 7 == 0) { rc2++; sfx_tick(); }
    if (rs.shown && t > rs.shown + 60) {
        int all = 1;
        for (int i = 0; i < 11; i++) { rp_t *p = &rs.ppl[i]; float dx = p->sx - p->x, dy = p->sy - p->y;
            if (dx > .5f || dx < -.5f || dy > .5f || dy < -.5f) { all = 0; p->seated = 0;
                float ax = dx < 0 ? -dx : dx, ay = dy < 0 ? -dy : dy;
                p->x += (dx > 0 ? 1 : -1) * (ax < 1.3f ? ax : 1.3f) * (ax > 0); p->y += (dy > 0 ? 1 : -1) * (ay < .7f ? ay : .7f) * (ay > 0); p->flip = dx < 0; p->moving = 1; }
            else { p->x = p->sx; p->y = p->sy; p->seated = 1; p->moving = 0; p->flip = 1; } }
        if (all && !rs.ordered) { rs.ordered = t; say("Oficina en orden ✓", AN_RAUL, 0, 0, GY - 34, 130, BC_OK); sfx_clear();
            u8 cc[4] = {C(0xe4002b), C(0xffd23f), C(0x62d26f), C(0xffffff)};
            for (int i = 0; i < 40; i++) { pfx_t *p = fx_add(FX_CONF); if (p) { p->x = rndf() * VW; p->y = -rndf() * 20; p->vx = (rndf() - .5f) * .6f; p->vy = .6f + rndf() * .7f; p->c = cc[i % 4]; p->life = 170; } } }
    }
    /* fx (en el web se actualizan al dibujar) */
    j = 0;
    for (int i = 0; i < nfx; i++) { pfx_t *p = &fxs[i]; p->life--; int keep;
        if (p->type == FX_KEY) { p->y += p->vy; keep = p->life > 0; } else { p->x += p->vx; p->y += p->vy; keep = p->life > 0 && p->y < GY; }
        if (keep) fxs[j++] = *p; }
    nfx = j;
    return rs.ordered && t == rs.ordered + 140;
}
static void build_bg(void) {
    rbg = bmp_new(VW, VH); tgt_bmp(rbg);
    rect(0, 0, VW, GY, C(0xefe6d8)); rect(0, GY - 16, VW, 16, C(0xddd2c0)); rect(0, GY - 17, VW, 1, C(0xc9bca6));
    for (int i = 0; i < 2; i++) { int wx = 12 + i * 36; rect(wx, 12, 28, 30, KK); rect(wx + 1, 13, 26, 28, C(0x8fd3ff)); rect(wx + 4, 20, 8, 3, C(0xffffff)); rect(wx + 13, 13, 2, 28, KK); rect(wx + 1, 26, 26, 2, KK); }
    fbox(26, 52);
    rect(90, 8, 100, 50, KK); rect(138, 58, 4, 6, KK); rect(130, 63, 20, 2, KK);
    rect(0, GY, VW, VH - GY, C(0xb9b2a5)); for (int x = 0; x < VW; x += 16) rect(x, GY, 1, VH - GY, C(0xa39b8c)); rect(0, GY + 12, VW, 1, C(0xa39b8c));
    for (int b = 2; b >= 0; b--) { int y = GY - 10 - b * 12, x = 194 - RSH; rect(x, y, 62, 4, KK); rect(x + 1, y + 1, 61, 2, C(0xd0102c)); rect(x, y + 4, 62, GY - y - 4, C(0x8c0a1e)); rect(x, y + 4, 62, 1, KK); }
    rect(44, GY - 18, 48, 5, KK); rect(45, GY - 17, 46, 3, C(0x8a5a35)); rect(46, GY - 13, 3, 13, KK); rect(87, GY - 13, 3, 13, KK);
    tgt_back();
}
static void small_dash(int x, int y, const char *s, u8 col, int align) {
    char b[40]; int n = 0; /* la fuente pequeña no tiene la raya: se cambia por un guion */
    while (*s && n < 38) { if ((u8)s[0] == 0xE2 && (u8)s[1] == 0x80 && (u8)s[2] == 0x93) { b[n++] = '-'; s += 3; } else b[n++] = *s++; }
    b[n] = 0; stext(x, y, b, col, align);
}
static int cmp_y(const void *a, const void *b) { float d = (*(rp_t *const *)a)->y - (*(rp_t *const *)b)->y; return d < 0 ? -1 : d > 0; }
static void raul_draw(void) {
    blit_opq(rbg, 0, 0, VW, VH, 0, 0, 0);
    rect(92, 10, 96, 46, rs.shown ? C(0x0f2a1a) : C(0x2a0d12));
    /* pantalla (turnEl) */
    if (!rs.shown) { if ((pfr / 30) & 1) stext(140, 24, "SIN CITAS · SIN TURNOS", C(0xff5a66), 1); stext(140, 36, "11 personas esperando", C(0x62d26f), 1); }
    else { stext(140, 13, "TURNOS ASIGNADOS", C(0xffffff), 1);
        int el = rs.t - rs.shown, vis = imin(11, el / 7 + 1), top = 0;
        if (vis > 6) top = imin(vis - 6, 5); if (vis >= 11) top = ((el - 77) / 40) % 6; if (top > 5) top = 5; if (top < 0) top = 0;
        for (int i = top; i < vis && i < top + 6; i++) { char b[32]; char *p = str_cat(b, rs.ppl[i].code); p = str_cat(p, " · "); str_cat(p, rs.ppl[i].shift);
            small_dash(96, 21 + (i - top) * 6, b, C(0x62d26f), 0); } }
    /* Raül y escritorio */
    if (!rs.seated) draw_spr(CREW[1].spr, 24, iround(rs.rx) - 8, GY - 24, 0, 0);
    else { draw_spr(CREW[1].spr, 16, iround(rs.rx) - 8, GY - 31, 0, 0);
        if (rs.typing) { u8 sk = P_PAL['S']; rect(68 + ((pfr >> 1) % 3), GY - 20, 2, 2, sk); rect(74 + ((pfr >> 1) + 1) % 3, GY - 20, 2, 2, sk); } }
    rect(44, GY - 18, 48, 5, KK); rect(45, GY - 17, 46, 3, C(0x8a5a35));
    rect(66, GY - 20, 18, 2, KK);
    if (rs.lid > 0) { rect(81, GY - 20 - rs.lid, 3, rs.lid, KK); rect(82, GY - 19 - rs.lid, 1, rs.lid - 1, rs.typing && (pfr >> 2) & 1 ? C(0x62d26f) : C(0x9aa3b5)); }
    /* rótulo SALA FORMACIÓN */
    { int x = 225 - RSH, w = 9 * 4 + 10; rect(x - w / 2, 62 - 17, w, 17, C(0xffffff)); rect(x - w / 2 + 1, 62 - 16, w - 2, 15, C(0x120b0c));
      stext(x, 62 - 14, "SALA", C(0xffd23f), 1); stext(x, 62 - 7, "FORMACIÓN", C(0xffffff), 1); }
    /* gente (filas de atrás primero) */
    rp_t *ord[11]; for (int i = 0; i < 11; i++) ord[i] = &rs.ppl[i];
    for (int a = 1; a < 11; a++) { rp_t *v = ord[a]; int b = a - 1; while (b >= 0 && cmp_y(&ord[b], &v) > 0) { ord[b + 1] = ord[b]; b--; } ord[b + 1] = v; }
    for (int i = 0; i < 11; i++) { rp_t *p = ord[i]; int x = iround(p->x - 8);
        if (p->seated) pers_draw(p->look, x, iround(p->y - 17), 1, 0, 0, 17);
        else pers_draw(p->look, x, iround(p->y - 24), p->flip, p->moving ? (((pfr >> 3) & 1) ? 1 : -1) : 0, 0, 24);
        if (p->ok) { int y = iround(p->y - (p->seated ? 24 : 31)); rect(x + 5, y, 7, 6, KK); rect(x + 6, y + 1, 5, 4, C(0x62d26f)); u8 W = C(0xffffff); pset(x + 7, y + 3, W); pset(x + 8, y + 4, W); rect(x + 9, y + 2, 1, 2, W); } }
    for (int i = 0; i < rs.nenv; i++) { env_t *e = &rs.envs[i]; int x = iround(e->x - 4), y = iround(e->y);
        rect(x, y, 9, 7, KK); rect(x + 1, y + 1, 7, 5, C(0xffffff)); u8 R = C(0xd0102c); pset(x + 2, y + 2, R); rect(x + 3, y + 3, 3, 1, R); pset(x + 6, y + 2, R); }
    for (int i = 0; i < nfx; i++) { pfx_t *p = &fxs[i]; if (p->type == FX_KEY) rect(iround(p->x), iround(p->y), 2, 1, p->c); else rect(iround(p->x), iround(p->y), 2, 2, p->c); }
    draw_hud_top(1, rc1, rc2, "Citas", "Turnos");
    draw_bubbles();
}
s32 raul_dbg[4];
int raul_play(void) {
    u32 mark = pool_mark(); people_cache_reset(); pers_reset(); plat_ui_reset(); plat_ui_alloc();
    build_bg(); raul_start(); bub_anchor = raul_anchor; plat_maxn = 0;
    RES_L1 = "Citas"; RES_L2 = "Turnos";
    int result = 0;
    raul_draw(); frame_end();
    for (;;) {
        int n = frame_end(); plat_note_n(n);
        if (key_hit & K_START) { if (pause_menu()) { result = 0; break; } frame_end(); continue; }
        int done = 0;
        for (int i = 0; i < n && !done; i++) done = raul_step();
        RES_C1 = rc1; RES_C2 = rc2;
        raul_dbg[0] = rs.t; raul_dbg[1] = plat_maxn; raul_dbg[2] = rs.ordered;
        if (done) { result = 1; break; }
        raul_draw();
    }
    clear_bubbles(); bub_anchor = 0;
    pool_reset(mark); people_cache_reset(); pers_reset(); plat_ui_reset();
    return result;
}
