/* FLiiTS Catalunya World (GBA): portada, intro, modos, selector de jugador / vehículo / repartidor,
   tarjeta de moneda, créditos y guardado. */
#include "modes.h"

#define NIGHT C(0x171213)
#define NIGHT2 C(0x2a1d1f)
#define GOLD C(0xffd23f)
#define RED C(0xe4002b)
#define PANEL C(0xd0102c)
#define PANEL2 C(0x8c0a1e)
#define DIM C(0xffd3d9)
#define MUTE C(0xb0868c)
#define GREEN C(0x62d26f)
#define WHITE C(0xffffff)
#define EDGE C(0x120b0c)

extern int REP_STARS;
__attribute__((weak)) int REP_STARS;

static const int SEL[4] = {CI_IVAN, CI_GERARD, CI_AZLAN, CI_RAUL};
static const int SCREEN_OF[4] = {0, -1, 1, 2};
static const char *const MODE_N[3] = {"PLATAFORMAS", "RPG", "ARCADE"};
static const char *const ROLE[4] = {"Barcelona (Glovo) · Sabadell", "Citas y turnos · jefe final", "Barcelona (Uber) · Gavà · Mataró", "Lleida · Martorell · Sant Cugat · Molins de Rei · Parets"};
typedef struct { const char *name; u8 look; } rider_def;
static const rider_def REP_RIDERS[4] = {{"AAMIR", 0}, {"SAIRA", 4}, {"NIL", 3}, {"AYESHA", 1}};
static int mode, pick, veh, rep_look, just_unlocked;
static u32 base_mark;
static char toast_s[48]; static int toast_t;

static int gold(void) { return PROG[mode][0] + PROG[mode][1] + PROG[mode][2]; }
static int raul_open(void) { return gold() >= 3; }

/* ---------------------------------------------------------------- guardado */
typedef struct { u8 prog[3][3], fin[3], rep_on, rep_done; } save_t;
void game_save(void) { save_t s; memcpy(s.prog, PROG, 9); memcpy(s.fin, FIN, 3); s.rep_on = REP_ON; s.rep_done = REP_DONE; save_write(&s, sizeof s); }
static void game_load(void) { save_t s; if (save_read(&s, sizeof s)) { memcpy(PROG, s.prog, 9); memcpy(FIN, s.fin, 3); REP_ON = s.rep_on; REP_DONE = s.rep_done; } }

/* ---------------------------------------------------------------- dibujo común */
static void menu_reset(void) { pool_reset(base_mark); people_cache_reset(); }
static void bg_night(void) {
    cls(NIGHT);
    for (int y = 0; y < SH; y += 4) if ((y >> 2) % 7 == 3) rect(0, y, SW, 1, NIGHT2);
}
static void draw_toast(void) {
    if (toast_t <= 0) return; toast_t--;
    int w = text_len(toast_s) * 8 + 12;
    ui_box(120 - w / 2, 4, w, 14, PANEL, GOLD); text(120, 7, toast_s, WHITE, 1);
}
static void toast(const char *s) { strncpy(toast_s, s, 47); toast_t = 120; }
static void silhouette(const char *const *spr, int x, int y) {
    for (int r = 0; r < 24; r++) for (int q = 0; spr[r][q]; q++) if (spr[r][q] != '.') pset(x + q, y + r, EDGE);
}
static void stars(int x, int y, int n) { for (int i = 0; i < 3; i++) text_small(x + i * 5, y, "★", i < n ? GOLD : MUTE, 0); }

/* ---------------------------------------------------------------- portada */
static void title_screen(void) {
    menu_reset();
    for (;;) {
        bg_night();
        rect(0, 0, SW, 3, RED);
        text_s(122, 18, "FLiiTS", PANEL2, 1, 3); text_s(120, 16, "FLiiTS", RED, 1, 3);
        text_s(121, 45, "CATALUNYA", EDGE, 1, 2); text_s(120, 44, "CATALUNYA", GOLD, 1, 2);
        text(120, 66, "EL PROGRESO DE LA REGIÓN", WHITE, 1);
        text_small(120, 78, "PRESENTA: RAÜL · NOVIEMBRE 2026", DIM, 1);
        /* suelo y equipo */
        rect(0, 126, SW, 34, C(0x8a5a35)); rect(0, 126, SW, 10, C(0xbdb9ae)); rect(0, 126, SW, 1, C(0x3e7a35));
        draw_stand(0, 30, 102); draw_spr(CREW[2].spr, 24, 104, 102, 0, 0); draw_spr(CREW[3].spr, 24, 136, 102, 0, 0); draw_spr(CREW[1].spr, 24, 180, 102, 1, 0);
        if ((frame_no >> 5) & 1) text(120, 146, "PULSA START", GOLD, 1);
        frame_end();
        if (key_hit & (K_A | K_B | K_START)) { sfx_select(); return; }
    }
}
static void intro_screen(void) {
    int t = 0, stage = 0;
    for (;;) {
        int n = frame_end(); t += n;
        bg_night();
        text(120, 10, "FLiiTS", RED, 1); text(120 + 28, 10, "", GOLD, 0); text(120, 22, "CATALUNYA", GOLD, 1);
        rect(0, 120, SW, 40, C(0x2a1d1f)); rect(0, 120, SW, 1, C(0x5a3a3e));
        int k = imin(t, 90);
        int xs[4] = {-20 + k * 120 / 90, 250 - k * 136 / 90, -40 + k * 128 / 90, 270 - k * 140 / 90};
        if (stage == 2) { int d = imin(t, 40); xs[0] -= d * 4; xs[2] -= d * 5; xs[1] += d * 4; xs[3] += d * 5; }
        int walk = k < 90 && ((t >> 3) & 1);
        draw_spr(CREW[1].spr, 24, 112, 96, 0, 0);
        draw_spr(CREW[0].spr, 24, xs[0], 96, 0, walk); draw_dog(t >> 3, xs[0] - 32, 102, 1);
        draw_spr(CREW[2].spr, 24, xs[1], 96, 1, walk);
        draw_spr(CREW[3].spr, 24, xs[2], 96, 0, walk);
        if (stage == 0 && t > 50 && t < 140) text_sh(120, 66, "¡FIIIIIIII!", GOLD, EDGE, 1);
        if (stage == 0 && t >= 90) {
            ui_box(14, 132, 212, 22, EDGE, GOLD); text(120, 139, "RAÜL: «¡CHICOS! ¡A CURRAR!»", WHITE, 1);
            if ((t >> 4) & 1) text_small(120, 124, "A · CONTINUAR", MUTE, 1);
        }
        if (stage == 2) { ui_box(14, 132, 212, 22, EDGE, GOLD); text_small(120, 140, "¡REUNIÓN TERMINADA! CADA UNO A SU MISIÓN.", WHITE, 1); if (t > 45) { sfx_open(); return; } }
        if (t == 51 || (t > 50 && t < 53)) { if (stage == 0) { slide(1800, 2400, 300); } }
        if (key_hit & (K_A | K_B | K_START)) { if (stage == 0 && t < 90) t = 90; else if (stage == 0) { stage = 2; t = 0; } }
    }
}

/* ---------------------------------------------------------------- elegir modo */
static int mode_screen(void) {
    static const char *const SUB[3] = {"Salta, reparte y contrata", "Misiones por turnos", "Una máquina por jugador"};
    int c = mode;
    menu_reset();
    for (;;) {
        bg_night();
        text(120, 12, "ELIGE TU AVENTURA", GOLD, 1);
        text_small(120, 26, "UNA CATALUNYA, TRES FORMAS DE JUGAR", DIM, 1);
        for (int i = 0; i < 3; i++) {
            int x = 16, y = 38 + i * 34, w = 208, h = 30, on = i == c;
            ui_box(x, y, w, h, on ? PANEL : NIGHT2, on ? GOLD : MUTE);
            if (i == 0) { const rider_t *r = compose_rider(0, 0, on ? frame_no >> 2 : 0); if (r) blit_part(r->b, 0, 6, 68, 24, x + 2, y + 3, 0); }
            if (i == 1) { rect(x + 22, y + 3, 24, 24, C(0x9bbc0f)); frame_rect(x + 21, y + 2, 26, 26, C(0x0f380f));
                for (int r = 0; r < 22; r++) for (int q = 0; q < 16; q++) { char ch = CREW[1].spr[r][q]; if (ch != '.') { u32 v = col_of(P_PAL[(u8)ch]); int L = ((v >> 16) * 3 + ((v >> 8) & 255) * 6 + (v & 255)) / 10; pset(x + 26 + q, y + 4 + r, L < 60 ? C(0x0f380f) : L < 130 ? C(0x306230) : L < 190 ? C(0x8bac0f) : C(0x9bbc0f)); } } }
            if (i == 2) { rect(x + 24, y + 3, 20, 25, C(0x2f6fe0)); rect(x + 22, y + 2, 24, 2, GOLD); rect(x + 27, y + 6, 14, 9, EDGE); rect(x + 28, y + 7, 12, 7, C(0x58c4ff));
                rect(x + 29, y + 19, 2, 5, EDGE); circle(x + 30, y + 18, 2, RED); circle(x + 37, y + 21, 1, GOLD); }
            text(x + 76, y + 7, MODE_N[i], on ? WHITE : DIM, 0);
            text_small(x + 76, y + 19, SUB[i], on ? GOLD : MUTE, 0);
            if (FIN[i]) text(x + w - 14, y + 11, "✓", GREEN, 0);
        }
        text_small(120, 150, "← → ELEGIR · A JUGAR · B VOLVER", MUTE, 1);
        draw_toast();
        frame_end();
        if (key_hit & K_LEFT) { c = (c + 2) % 3; sfx_tick(); }
        if (key_hit & K_RIGHT) { c = (c + 1) % 3; sfx_tick(); }
        if (key_hit & (K_A | K_START)) { sfx_open(); mode = c; return 1; }
        if (key_hit & K_B) { sfx_back(); return 0; }
    }
}

/* ---------------------------------------------------------------- selector de jugador */
static void draw_rep_stand(int L, int x, int y) { draw_spr(rep_crew(L)->spr, 24, x, y, 0, 0); }
/* devuelve: 0..3 personaje, 4 repartidor, -1 volver */
static int roster_screen(void) {
    int list[5] = {0, 2, 3, 1, 4}, n = REP_ON ? 5 : 4, c = 0;
    for (int i = 0; i < n; i++) if (list[i] == pick) c = i;
    if (mode != 2 && pick != 1 && pick != 4 && SCREEN_OF[pick] >= 0 && PROG[mode][SCREEN_OF[pick]]) {
        int nx = -1; for (int i = 0; i < 3; i++) if (!PROG[mode][SCREEN_OF[SEL[i]]]) { nx = i; break; }
        c = nx >= 0 ? nx : 3;
    }
    menu_reset();
    for (;;) {
        bg_night();
        char hdr[40]; str_cat(str_cat(hdr, "ELIGE JUGADOR · "), MODE_N[mode]);
        text(120, 6, hdr, GOLD, 1);
        int cw = n == 5 ? 46 : 56, x0 = (SW - cw * n) / 2;
        for (int i = 0; i < n; i++) {
            int id = list[i], x = x0 + i * cw, y = 20, on = i == c;
            int locked = id == 1 && !raul_open();
            int done = id == 4 ? REP_DONE : id == 1 ? FIN[mode] : PROG[mode][SCREEN_OF[id]];
            ui_box(x + 1, y, cw - 2, 96, on ? PANEL : NIGHT2, on ? GOLD : (id == 1 ? GOLD : MUTE));
            rect(x + 6, y + 54, cw - 12, 3, on ? PANEL2 : EDGE);
            int sx = x + cw / 2 - 8;
            if (id == 4) draw_rep_stand(0, sx, y + 30);
            else if (locked) silhouette(CREW[1].spr, sx, y + 30);
            else if (id == 0) { draw_spr(CREW[0].spr, 24, x + 3, y + 30, 0, 0); draw_dog(0, x + cw - 30, y + 36, 1); }
            else draw_spr(CREW[id].spr, 24, sx, y + 30, 0, 0);
            const char *nm = id == 4 ? "REPARTO" : locked ? "???" : CREW[id].name;
            text_small(x + cw / 2, y + 62, nm, WHITE, 1);
            if (done) { text_small(x + cw / 2, y + 72, "SUPERADO", GREEN, 1); text(x + cw - 10, y + 3, "✓", GREEN, 0); }
            else if (id == 4) text_small(x + cw / 2, y + 72, "¡NUEVO!", GOLD, 1);
            else if (id == 1) { if (locked) { char b[16]; char *p = str_num(b, gold()); str_cat(p, "/3 ORO"); text_small(x + cw / 2, y + 72, b, GOLD, 1); }
                                else text_small(x + cw / 2, y + 72, "¡LIBRE!", GOLD, 1); }
            if (id == 1 && (!locked || 1)) text_small(x + cw / 2, y + 84, "FINAL", MUTE, 1);
            if (id == 1 && just_unlocked && (frame_no >> 3) & 1) frame_rect(x, y - 1, cw, 98, WHITE);
        }
        int id = list[c];
        /* rol del elegido y nota */
        const char *role = id == 4 ? "Modo libre · reparte por Barcelona" : (id == 1 && !raul_open()) ? "Personaje secreto" : ROLE[id];
        text_small(120, 124, role, DIM, 1);
        char note[96];
        int all = FIN[mode] && (!REP_ON || REP_DONE);
        if (all) str_cat(note, "¡Todo completado! Vuelve a jugar con cualquiera.");
        else if (FIN[mode]) str_cat(note, "¡Juego completado! Te queda el modo repartidor.");
        else if (raul_open()) str_cat(str_cat(note, CREW[1].name), " desbloqueado: su pantalla lleva al jefe final.");
        else { char *p = str_cat(note, "Consigue las 3 monedas de oro: llevas "); p = str_num(p, gold()); str_cat(p, "/3."); }
        rect(0, 136, SW, 1, NIGHT2);
        text_small(120, 140, note, GOLD, 1);
        text_small(120, 151, "← → · A ELEGIR · B MODOS · SELECT+START REPARTIDOR", MUTE, 1);
        draw_toast();
        frame_end();
        if (key_hit & K_LEFT) { c = (c + n - 1) % n; sfx_tick(); }
        if (key_hit & K_RIGHT) { c = (c + 1) % n; sfx_tick(); }
        if ((key_held & (K_SEL | K_START)) == (K_SEL | K_START) && (key_hit & (K_SEL | K_START))) { sfx_select(); just_unlocked = 0; pick = 4; return 4; }
        if (key_hit & K_A || (key_hit & K_START && !(key_held & K_SEL))) {
            if (id == 1 && !raul_open()) { toast("BLOQUEADO"); sfx_back(); continue; }
            just_unlocked = 0; pick = id; sfx_select(); return id;
        }
        if (key_hit & K_B) { sfx_back(); just_unlocked = 0; return -1; }
    }
}

/* ---------------------------------------------------------------- repartidor y vehículo */
static int rider_screen(void) {
    int c = rep_look;
    for (;;) {
        bg_night();
        text(120, 10, "ELIGE REPARTIDOR", GOLD, 1);
        text_small(120, 24, "CON EL UNIFORME DE FLiiTS", DIM, 1);
        for (int i = 0; i < 4; i++) {
            int x = 12 + i * 56, y = 38, on = i == c;
            ui_box(x, y, 50, 80, on ? PANEL : NIGHT2, on ? GOLD : MUTE);
            draw_rep_stand(REP_RIDERS[i].look, x + 17, y + 14);
            rect(x + 8, y + 38, 34, 3, on ? PANEL2 : EDGE);
            text_small(x + 25, y + 50, REP_RIDERS[i].name, WHITE, 1);
        }
        text_small(120, 150, "← → ELEGIR · A SEGUIR · B VOLVER", MUTE, 1);
        frame_end();
        if (key_hit & K_LEFT) { c = (c + 3) & 3; sfx_tick(); }
        if (key_hit & K_RIGHT) { c = (c + 1) & 3; sfx_tick(); }
        if (key_hit & (K_A | K_START)) { rep_look = c; sfx_select(); return 1; }
        if (key_hit & K_B) { sfx_back(); return 0; }
    }
}
static int veh_screen(int who_rep) {
    static const char *const ST[3] = {"VELOCIDAD", "CARGA", "ECO"};
    int c = veh;
    for (;;) {
        bg_night();
        text(120, 8, "ELIGE VEHÍCULO", GOLD, 1);
        text_small(120, 22, who_rep ? REP_RIDERS[rep_look].name : CREW[pick].name, DIM, 1);
        for (int i = 0; i < 3; i++) {
            int x = 6 + i * 77, y = 32, on = i == c;
            ui_box(x, y, 74, 104, on ? PANEL : NIGHT2, on ? GOLD : MUTE);
            const rider_t *r = who_rep ? compose_rider_c(rep_crew(REP_RIDERS[rep_look].look), 10 + REP_RIDERS[rep_look].look, i, on ? frame_no >> 2 : 0) : compose_rider(pick, i, on ? frame_no >> 2 : 0);
            if (r) blit(r->b, x + 3, y + 4, 0);
            text_small(x + 37, y + 54, i == 2 ? "PATINETE" : VEH[i].n, WHITE, 1);
            for (int k = 0; k < 3; k++) { text_small(x + 6, y + 66 + k * 10, ST[k], DIM, 0); stars(x + 52, y + 66 + k * 10, VEH[i].st[k]); }
        }
        text_small(120, 150, "← → ELEGIR · A ¡A REPARTIR! · B VOLVER", MUTE, 1);
        frame_end();
        if (key_hit & K_LEFT) { c = (c + 2) % 3; sfx_tick(); }
        if (key_hit & K_RIGHT) { c = (c + 1) % 3; sfx_tick(); }
        if (key_hit & (K_A | K_START)) { veh = c; sfx_open(); return 1; }
        if (key_hit & K_B) { sfx_back(); return 0; }
    }
}

/* ---------------------------------------------------------------- tarjeta de moneda (showWin / showRepWin) */
static void win_card(int who, int screen, int rep) {
    menu_reset();
    char kick[48], msg[80], *p;
    if (rep) { str_cat(str_cat(kick, "MODO REPARTIDOR · "), REP_RIDERS[rep_look].name); p = str_cat(msg, "¡Repartidor del mes! 3 pedidos · "); p = str_num(p, REP_STARS); str_cat(p, "/9 ★"); }
    else {
        p = str_cat(kick, MODE_N[mode]); p = str_cat(p, " · MUNDO "); p = str_num(p, screen + 1); p = str_cat(p, " · "); str_cat(p, CREW[who].name);
        if (gold() == 3) str_cat(msg, "¡3 monedas! Se desbloquea el personaje secreto.");
        else { p = str_cat(msg, "Moneda de oro conseguida · llevas "); p = str_num(p, gold()); str_cat(p, "/3"); }
    }
    int t = 0;
    sfx_clear();
    for (;;) {
        int n = frame_end(); t += n;
        if (t > 30 && t - n <= 30) sfx_coin();
        bg_night();
        ui_box(16, 12, 208, 136, PANEL, GOLD);
        text_small(120, 20, kick, DIM, 1);
        text_s(121, 31, rep ? "¡REPARTIDOR DEL MES!" : "¡PANTALLA SUPERADA!", EDGE, 1, 1); text(120, 30, rep ? "¡REPARTIDOR DEL MES!" : "¡PANTALLA SUPERADA!", GOLD, 1);
        rect(96, 74, 48, 3, PANEL2);
        if (rep) draw_rep_stand(REP_RIDERS[rep_look].look, 112, 50);
        else if (who == 0) { draw_spr(CREW[0].spr, 24, 96, 50, 0, 0); draw_dog(0, 114, 56, 1); }
        else draw_spr(CREW[who].spr, 24, 112, 50, 0, 0);
        for (int k = 0; k < 3; k++) {
            int on = rep ? 1 : PROG[mode][k], nw = rep ? k == 2 : k == screen, cx = 90 + k * 30;
            if (on && (!nw || t > 30)) big_coin(cx, 84, nw ? t : 0); else { rect(cx - 9, 84, 18, 22, PANEL2); frame_rect(cx - 9, 84, 18, 22, EDGE); }
        }
        text_wrap(120, 114, msg, WHITE, 25, 9, 1);
        if (RES_L1 && !rep) { char b[48]; p = str_cat(b, RES_L1); p = str_cat(p, " "); p = str_num(p, RES_C1); if (RES_L2) { p = str_cat(p, " · "); p = str_cat(p, RES_L2); p = str_cat(p, " "); str_num(p, RES_C2); } text_small(120, 106, b, GOLD, 1); }
        if ((t >> 4) & 1) text_small(120, 138, "A · CONTINUAR", WHITE, 1);
        if (key_hit & (K_A | K_START) && t > 20) { sfx_select(); return; }
        if (t > 300) return;
    }
}

/* ---------------------------------------------------------------- créditos */
static void credits(int new_rep) {
    static const int WHO[4] = {0, 2, 3, 1};
    menu_reset();
    sfx_win();
    int y0 = 160, t = 0, total = new_rep ? 270 : 230;
    for (;;) {
        int n = frame_end(); t += n;
        static int acc; acc += n; while (acc >= 2) { acc -= 2; if (y0 > 160 - total) y0--; }
        if (key_held & K_DOWN) y0 -= 2; if (key_held & K_UP && y0 < 160) y0 += 2;
        bg_night();
        int y = y0;
        text_s(121, y + 1, "¡GRACIAS", EDGE, 1, 2); text_s(120, y, "¡GRACIAS", GOLD, 1, 2);
        text_s(121, y + 19, "POR JUGAR!", EDGE, 1, 2); text_s(120, y + 18, "POR JUGAR!", GOLD, 1, 2);
        text(120, y + 46, "EQUIPO CATALUNYA", RED, 1);
        for (int i = 0; i < 4; i++) {
            int id = WHO[i], x = 22 + i * 56;
            draw_spr(CREW[id].spr, 24, x, y + 60, 0, 0);
            text_small(x + 8, y + 88, CREW[id].name, WHITE, 1);
            if (id == 0) { draw_dog(t >> 3, x + 18, y + 66, 1); text_small(x + 32, y + 96, "OREO", WHITE, 1); }
        }
        text(120, y + 112, "CON LA COLABORACIÓN", RED, 1); text(120, y + 122, "ESPECIAL DE", RED, 1);
        draw_pal(MARTI, MARTI_H, 40, y + 138, P_MPAL_I, 0); text_small(49, y + 170, "MARTÍ PUJOL", WHITE, 1);
        draw_pal(MARTI_B, MARTI_B_H, 111, y + 138, P_MPAL_A, 0); text_small(120, y + 170, "MARTÍ ESPRIU", WHITE, 1);
        draw_pal(FRANCO, FRANCO_H, 183, y + 142, P_FPAL, 0); text_small(191, y + 170, "FRANCO", WHITE, 1);
        text(120, y + 190, "Y A TODOS NUESTROS RIDERS", RED, 1);
        char b[64], *p = str_cat(b, "PUNTUACIÓN "); char sc[12]; str_num(sc, SCORE); for (int k = text_len(sc); k < 6; k++) p = str_cat(p, "0");
        p = str_cat(p, sc); p = str_cat(p, " · MONEDAS "); p = str_num(p, COINS); p = str_cat(p, " · ORO "); p = str_num(p, gold()); str_cat(p, "/3");
        text_small(120, y + 206, b, DIM, 1);
        if (new_rep) { text(120, y + 222, "¡MODO REPARTIDOR", GOLD, 1); text(120, y + 232, "DESBLOQUEADO!", GOLD, 1); text_small(120, y + 246, "YA APARECE EN EL SELECTOR DE JUGADORES", DIM, 1); }
        if ((t >> 4) & 1) text_small(120, 152, "B · VOLVER", MUTE, 1);
        if (key_hit & (K_B | K_START) || (key_hit & K_A && t > 120)) { sfx_select(); return; }
    }
}

/* ---------------------------------------------------------------- partida */
static void do_credits(void) {
    FIN[mode] = 1;
    int nr = !REP_ON; REP_ON = 1;
    credits(nr);
}
static void play_pick(int id) {
    if (id == 4) {
        if (!rider_screen()) return;
        if (!veh_screen(1)) return;
        int gk = mode == 2 ? 5 : 4;
        if (arcade_play(gk, 4, veh, REP_RIDERS[rep_look].look)) { REP_DONE = 1; win_card(0, 0, 1); }
        return;
    }
    RES_L1 = RES_L2 = 0;
    int won = 0;
    if (id == 1) {
        if (mode == 0) won = platform_play(1, 0) && final_play();
        else if (mode == 1) won = rpg_play(1);
        else won = arcade_play(1, 1, 0, 0) && final_play();
        if (won) do_credits();
        return;
    }
    if (mode == 0) { if (!veh_screen(0)) return; won = platform_play(id, veh); }
    else if (mode == 1) won = rpg_play(id);
    else won = arcade_play(id, id, 0, 0);
    if (won) {
        int s = SCREEN_OF[id];
        int before = gold(); PROG[mode][s] = 1;
        win_card(id, s, 0);
        if (before < 3 && gold() == 3) { pick = 1; just_unlocked = 1; toast("¡NUEVO PERSONAJE DESBLOQUEADO!"); sfx_power(); }
    }
}

int main(void) {
    eng_init(); people_init();
    base_mark = pool_mark();
    game_load();
#ifdef TEST_CARDS
    PROG[0][0] = 1; RES_L1 = "CONTRATADOS"; RES_C1 = 3; RES_L2 = "DESPEDIDOS"; RES_C2 = 2; win_card(0, 0, 0); credits(1);
#endif
    for (;;) {
        title_screen();
        intro_screen();
        while (mode_screen()) {
            for (;;) {
                int id = roster_screen();
                if (id < 0) break;
                play_pick(id);
            }
        }
    }
}
