/* FLiiTS Catalunya World · edición portátil (ROM de Game Boy)
   Portada > selector (Ivan, Gerard, Azlan, Raül) > misiones RPG > jefes > mochilas gigantes > créditos. */
#include <gb/gb.h>
#include <gb/hardware.h>
#include <string.h>
#include "assets.h"

#define WIN_DLG 96
#define WIN_STATUS 128
#define WIN_OFF 144

/* ------------------------------------------------------------------ entrada */
uint8_t cur, prev;
void input(void) { prev = cur; cur = joypad(); }
uint8_t hit(uint8_t b) { return cur & ~prev & b; }
void frame(void) { wait_vbl_done(); input(); }
void frames(uint8_t n) { while (n--) frame(); }

uint8_t rs = 0x5A;
uint8_t rnd(void) { rs ^= rs << 3; rs ^= rs >> 5; rs ^= DIV_REG; return rs; }

/* ------------------------------------------------------------------ sonido */
void snd_init(void) { NR52_REG = 0x80; NR50_REG = 0x77; NR51_REG = 0xFF; }
void tone(uint16_t f, uint8_t env) { NR10_REG = 0; NR11_REG = 0x80; NR12_REG = env; NR13_REG = (uint8_t)f; NR14_REG = 0x80 | ((f >> 8) & 7); }
void sfx_blip(void) { tone(1750, 0x71); }
void sfx_sel(void) { tone(1840, 0xA2); }
void sfx_coin(void) { tone(1899, 0xC2); frames(4); tone(1985, 0xC3); }
void sfx_hit(void) { NR41_REG = 0x3F; NR42_REG = 0xF2; NR43_REG = 0x61; NR44_REG = 0x80; }
void sfx_bad(void) { tone(1200, 0xF3); frames(6); tone(1050, 0xF3); }
const uint16_t WIN_NOTES[] = {1546, 1650, 1714, 1798, 1849, 1899};
void sfx_win(void) { uint8_t i; for (i = 0; i < 6; i++) { tone(WIN_NOTES[i], 0xB2); frames(5); } }

/* ------------------------------------------------------------------ vídeo */
void bg_load(uint8_t first, uint8_t n, const uint8_t *d) {
    uint8_t i; uint16_t t;
    for (i = 0; i < n; i++) {
        t = (uint16_t)first + i;
        set_data((uint8_t *)(t < 128 ? 0x9000 + t * 16 : 0x8800 + (t - 128) * 16), d + (uint16_t)i * 16, 16);
    }
}
const uint8_t FADEP[] = {0xE4, 0x90, 0x40, 0x00};
void fade_out(void) { uint8_t i; for (i = 0; i < 4; i++) { BGP_REG = OBP0_REG = FADEP[i]; frames(3); } }
void fade_in(void) { uint8_t i; for (i = 4; i > 0; i--) { BGP_REG = OBP0_REG = FADEP[i - 1]; frames(3); } }
void flash(uint8_t n) { while (n--) { BGP_REG = 0x1B; frames(3); BGP_REG = 0xE4; frames(3); } }

void hide_all(void) { uint8_t i; for (i = 0; i < 40; i++) move_sprite(i, 0, 0); }
uint8_t box_on;
void (*status_fn)(void);
void load_scene(uint8_t id) {
    DISPLAY_OFF; hide_all();
    bg_load(SCENE0, scenes[id].n, scenes[id].tiles);
    set_bkg_tiles(0, 0, 20, 18, scenes[id].map);
    move_win(7, WIN_OFF); box_on = 0; status_fn = 0;
    DISPLAY_ON;
}
void bg_text(uint8_t x, uint8_t y, const char *s) { while (*s) set_bkg_tile_xy(x++, y, font_lut[(uint8_t)*s++]); }
void bg_ctext(uint8_t y, const char *s) { bg_text((20 - strlen(s)) / 2, y, s); }
void win_text(uint8_t x, uint8_t y, const char *s) { while (*s) set_win_tile_xy(x++, y, font_lut[(uint8_t)*s++]); }

/* sprites: cada personaje 16x32 = 4 sprites de 8x16; "slot" = 8 tiles en VRAM */
void load_char(uint8_t slot, uint8_t id) { set_sprite_data(slot * 8, 8, spr_tiles + (uint16_t)id * 128); }
void put_char(uint8_t e, uint8_t slot, uint8_t x, uint8_t y, uint8_t prop) {
    uint8_t o = e * 4, t = slot * 8, i;
    for (i = 0; i < 4; i++) { set_sprite_tile(o + i, t + i * 2); set_sprite_prop(o + i, prop); }
    if (prop & S_FLIPX) { move_sprite(o, x + 16, y + 16); move_sprite(o + 1, x + 8, y + 16); move_sprite(o + 2, x + 16, y + 32); move_sprite(o + 3, x + 8, y + 32); }
    else { move_sprite(o, x + 8, y + 16); move_sprite(o + 1, x + 16, y + 16); move_sprite(o + 2, x + 8, y + 32); move_sprite(o + 3, x + 16, y + 32); }
}
void put_wide(uint8_t e, uint8_t slot, uint8_t x, uint8_t y) { /* perro 32x16 */
    uint8_t o = e * 4, t = slot * 8, i;
    for (i = 0; i < 4; i++) { set_sprite_tile(o + i, t + i * 2); set_sprite_prop(o + i, 0); move_sprite(o + i, x + 8 + i * 8, y + 16); }
}
void hide_ent(uint8_t e) { uint8_t i; for (i = 0; i < 4; i++) move_sprite(e * 4 + i, 0, 0); }

/* ------------------------------------------------------------------ texto */
char sb[160]; uint8_t sbn;
void sb0(void) { sbn = 0; sb[0] = 0; }
void sba(const char *s) { while (*s) sb[sbn++] = *s++; sb[sbn] = 0; }
void sbu(uint8_t v) { if (v >= 100) sb[sbn++] = '0' + v / 100; if (v >= 10) sb[sbn++] = '0' + (v / 10) % 10; sb[sbn++] = '0' + v % 10; sb[sbn] = 0; }

void box_draw(void) {
    uint8_t y;
    set_win_tile_xy(0, 0, BOX0); fill_win_rect(1, 0, 18, 1, BOX0 + 1); set_win_tile_xy(19, 0, BOX0 + 2);
    for (y = 1; y < 5; y++) { set_win_tile_xy(0, y, BOX0 + 3); set_win_tile_xy(19, y, BOX0 + 4); }
    fill_win_rect(1, 1, 18, 4, 0);
    set_win_tile_xy(0, 5, BOX0 + 5); fill_win_rect(1, 5, 18, 1, BOX0 + 6); set_win_tile_xy(19, 5, BOX0 + 7);
}
void box_open(void) { if (!box_on) { box_draw(); move_win(7, WIN_DLG); box_on = 1; } }
void box_clear(void) { fill_win_rect(1, 1, 18, 4, 0); }
void box_close(void) {
    box_on = 0;
    if (status_fn) { status_fn(); move_win(7, WIN_STATUS); } else move_win(7, WIN_OFF);
}
void more_wait(void) {
    uint8_t t = 0;
    for (;;) { frame(); t++; set_win_tile_xy(18, 4, (t & 16) ? 0 : T_MORE); if (hit(J_A | J_B)) break; }
    set_win_tile_xy(18, 4, 0); sfx_blip();
}
/* Escribe un texto con ajuste de línea (18 columnas x 4 filas) y espera A. */
void say(const char *s) {
    uint8_t line = 0, col = 0, len, i;
    box_open(); box_clear();
    while (*s) {
        if (*s == '\n') { line++; col = 0; s++; continue; }
        if (*s == ' ') { if (col) col++; s++; continue; }
        len = 0; while (s[len] && s[len] != ' ' && s[len] != '\n') len++;
        if (col && col + len > 18) { line++; col = 0; }
        for (i = 0; i < len; i++) {
            if (col >= 18) { line++; col = 0; }
            if (line >= 4) { more_wait(); box_clear(); line = 0; col = 0; }
            set_win_tile_xy(1 + col, 1 + line, font_lut[(uint8_t)s[i]]); col++;
            if (!(cur & (J_A | J_B)) || (i & 1)) frame();
        }
        s += len;
    }
    more_wait();
}
void sayb(void) { say(sb); }
uint8_t choose(const char *const *opt, uint8_t n) {
    uint8_t i, c = 0;
    box_open(); box_clear();
    for (i = 0; i < n; i++) win_text(2, 1 + i, opt[i]);
    for (;;) {
        for (i = 0; i < n; i++) set_win_tile_xy(1, 1 + i, i == c ? T_ARROW : 0);
        frame();
        if (hit(J_UP)) { c = c ? c - 1 : n - 1; sfx_blip(); }
        if (hit(J_DOWN)) { c = (c + 1) % n; sfx_blip(); }
        if (hit(J_A)) { sfx_sel(); return c; }
    }
}
void wait_a(void) { do frame(); while (!hit(J_A | J_START)); sfx_sel(); }

/* ------------------------------------------------------------------ datos */
#define CI_IVAN 0
#define CI_RAUL 1
#define CI_GERARD 2
#define CI_AZLAN 3
#define CH_NPC 4
#define CH_NPCNO 10
#define CH_MARTI 16
#define CH_MARTIB 17
#define CH_FRANCO 18
#define CH_DOG 19
const char *const HERO[] = {"IVAN", "RAÜL", "GERARD", "AZLAN"};

typedef struct { uint8_t gx, gy, spr; const char *name; uint8_t a; const char *t1; const char *t2; } npcd_t;
typedef struct {
    uint8_t scene; const uint8_t *solid; const char *zone; const char *title; const char *r1; const char *r2;
    uint8_t sx, sy; const npcd_t *npcs; const char *const *opts; uint8_t nopts;
    const char *lab1; const char *lab2;
} mission_t;

const char *const OPT_IVAN[] = {"CONTRATAR", "RECHAZAR", "DESPEDIR"};
const char *const OPT_GERARD[] = {"APROBAR", "RECHAZAR"};
const char *const OPT_AZLAN[] = {"DAR CASCO", "DAR CHAQUETA", "DAR GUANTES", "DAR MOCHILA"};
const char *const OPT_RAUL[] = {"VALIDAR CITA", "UNA POR PERSONA", "SIN CITA, FUERA", "RELLENAR HUECO"};

const npcd_t NPC_IVAN[] = {
    {2, 1, CH_NPCNO + 0, "AAMIR", 0, "Busco trabajo de rider. ¡Hago Glovo y Uber!", 0},
    {7, 1, CH_NPCNO + 1, "BILAL", 1, "Busco trabajo de rider. Solo hago Glovo.", 0},
    {2, 3, CH_NPC + 2, "HAMZA", 2, "Ya reparto con FLiiTS, pero... solo hago Glovo.", 0},
    {7, 3, CH_NPCNO + 3, "IMRAN", 0, "Busco trabajo de rider. ¡Hago Glovo y Uber!", 0},
    {4, 5, CH_NPC + 4, "FARHAN", 2, "Ya reparto con FLiiTS, pero... mañana no trabajo.", 0},
    {7, 5, CH_NPCNO + 5, "SAIRA", 1, "Busco trabajo de rider. Solo hago Glovo.", 0},
};
const npcd_t NPC_GERARD[] = {
    {2, 1, CH_NPCNO + 0, "AYESHA", 1, "NIE", "Aquí tienes mi NIE."},
    {7, 1, CH_NPCNO + 1, "ZAIN", 0, "carnet de moto", "Tengo carnet extranjero."},
    {2, 3, CH_NPCNO + 2, "NIL", 1, "pasaporte", "¡Pasaporte en regla!"},
    {7, 3, CH_NPCNO + 3, "LUCAS", 0, "permiso de trabajo", "Me caducó en 2025..."},
    {2, 5, CH_NPCNO + 4, "AAMIR", 1, "carnet de moto", "Carnet español, ¡toma!"},
    {5, 5, CH_NPCNO + 5, "BILAL", 0, "NIE", "NIE caducado..."},
};
/* necesidades de EPIs: bit0 casco · bit1 chaqueta · bit2 guantes · bit3 mochila */
const npcd_t NPC_AZLAN[] = {
    {2, 1, CH_NPCNO + 0, "HAMZA", 1, 0, 0},
    {7, 1, CH_NPCNO + 1, "IMRAN", 2 | 4, 0, 0},
    {2, 3, CH_NPCNO + 2, "FARHAN", 8, 0, 0},
    {7, 3, CH_NPCNO + 3, "SAIRA", 1 | 4, 0, 0},
    {2, 5, CH_NPCNO + 4, "AYESHA", 2, 0, 0},
    {5, 5, CH_NPCNO + 5, "ZAIN", 8 | 1, 0, 0},
};
const npcd_t NPC_RAUL[] = {
    {2, 1, CH_NPCNO + 0, "NIL", 0, "¡Hola! Tengo cita. Mi código es 7KQ2.", "✓ Código 7KQ2 validado. Turno: LUN 12-16h. ¡Pasa a formación!"},
    {7, 1, CH_NPCNO + 1, "LUCAS", 1, "Ya tengo cita... ¿puedo coger otra para mañana?", "✓ Solo una cita por persona. ¡Nadie acapara huecos!"},
    {2, 3, CH_NPCNO + 2, "AAMIR", 2, "¡Vengo sin cita! ¿Me dejas pasar?", "✓ Sin cita no se entra. Pide tu cita con el QR."},
    {7, 3, CH_NPCNO + 3, "BILAL", 0, "Buenos días. Código de cita: M4TR.", "✓ Código M4TR validado. Turno: MAR 19-23h. ¡Pasa a formación!"},
    {2, 5, CH_NPCNO + 4, "HAMZA", 3, "No puedo venir el martes. Cancelo mi cita de las 12h.", "✓ Hueco rellenado: adelantamos a quien tenía la cita lejos."},
    {7, 5, CH_NPCNO + 5, "IMRAN", 0, "Me llegó el mail con el código X9PL.", "✓ Código X9PL validado. Turno: VIE 19-23h. ¡Pasa a formación!"},
};
const mission_t MISSIONS[4] = {
    {SC_IVAN, solid_ivan, "BARCELONA", "CONTRATA A QUIEN HAGA GLOVO + UBER",
     "Habla con cada rider. Si hace GLOVO + UBER, ofrécele un CONTRATO.",
     "Si solo hace Glovo, recházalo. Y al que ya es de FLiiTS y falla... ¡despídelo!",
     5, 4, NPC_IVAN, OPT_IVAN, 3, "CONTRATADOS", "DESPEDIDOS"},
    {SC_RAUL, solid_raul, "OFICINA FLiiTS", "ORDENA LA OFICINA",
     "La oficina es un caos: sin citas y sin turnos.",
     "VALIDA la cita, UNA cita por persona, SIN cita no se entra y RELLENA los huecos.",
     5, 2, NPC_RAUL, OPT_RAUL, 4, "CITAS", "TURNOS"},
    {SC_GERARD, solid_gerard, "GAVÀ · MATARÓ", "PIDE LOS PAPELES A TODOS",
     "Cada aspirante te enseña un documento.",
     "APRUEBA los que están en regla y RECHAZA los que no. Los aprobados van a tu mochila.",
     4, 4, NPC_GERARD, OPT_GERARD, 2, "APROBADOS", "RECHAZADOS"},
    {SC_AZLAN, solid_azlan, "MARTORELL", "¡NADIE SE VA SIN SU EQUIPO!",
     "Cada rider necesita casco, chaqueta, guantes y mochila.",
     "Dale el EPI que le falta. ¡Si te equivocas, pierdes un corazón!",
     4, 4, NPC_AZLAN, OPT_AZLAN, 4, "EPIS", "COMPLETOS"},
};
const char *const EPI_NAME[] = {"el casco", "la chaqueta", "los guantes", "la mochila"};
const char *const EPI_UP[] = {"¡CASCO", "¡CHAQUETA", "¡GUANTES", "¡MOCHILA"};

typedef struct { const char *name; uint8_t hp; uint8_t scene; uint8_t spr1, spr2; const char *const *atk; } boss_t;
const char *const ATK_MARTI[] = {"Forecast negativo", "Hoja de cálculo", "Reunión urgente", "Recorte de plantilla"};
const char *const ATK_VALID[] = {"¡Rechazado!", "Falta una firma", "Vuelva mañana", "Sello caducado"};
const char *const ATK_ALMACEN[] = {"Inventario cerrado", "Escáner sin batería", "Caja equivocada", "Pedido bloqueado"};
const char *const ATK_BAGS[] = {"No hay más zonas", "Pedido gigante", "Lluvia de pedidos", "Aplastón"};
const boss_t BOSS_MARTI = {"MARTÍ", 95, SC_BATTLE, CH_MARTI, 255, ATK_MARTI};
const boss_t BOSS_VALID = {"VALIDACIÓN", 100, SC_BATTLE_DOC, 255, 255, ATK_VALID};
const boss_t BOSS_ALMACEN = {"ALMACÉN", 110, SC_BATTLE, CH_MARTIB, CH_FRANCO, ATK_ALMACEN};
const boss_t BOSS_BAGS = {"MOCHILAS", 160, SC_BATTLE_BAGS, 255, 255, ATK_BAGS};

const char *const MOVES[4][4] = {
    {"OFERTA GLOVO", "OFERTA UBER", "MOTO EMPRESA", "SUELDO FIJO"},
    {"PEGATINA FLiiTS", "RUTA OPTIMIZADA", "TURNO ASIGNADO", "PODER FLiiTS"},
    {"REVISAR PAPELES", "VALIDAR PERMISO", "PASAPORTE + NIE", "APROBACIÓN RTO"},
    {"CASCO", "CHAQUETA", "MOCHILA TÉRMICA", "KIT COMPLETO"},
};
const uint8_t POWER[4] = {10, 18, 24, 36};

/* ------------------------------------------------------------------ estado */
uint8_t coins[4];
uint8_t ci;
const mission_t *M;
uint8_t alive[6], need[6], done_[6];
uint8_t pgx, pgy, px, py, pdir, pflip, hearts, nleft, cnt1, cnt2, docs;
const int8_t DX[4] = {0, 0, -1, 1}, DY[4] = {-1, 1, 0, 0};

/* ------------------------------------------------------------------ combate */
uint8_t ehp, emax, hhp, hmax;
char mv[4][20];
void bar(uint8_t x, uint8_t y, uint8_t v, uint8_t max) {
    uint8_t p = (uint16_t)v * 48 / max, i, f;
    if (v && !p) p = 1;
    set_bkg_tile_xy(x, y, BAR0 + 9);
    for (i = 0; i < 6; i++) { f = p >= 8 ? 8 : p; set_bkg_tile_xy(x + 1 + i, y, BAR0 + f); p -= f; }
    set_bkg_tile_xy(x + 7, y, BAR0 + 10);
}
void hero_num(void) { sb0(); sba("PS "); sbu(hhp); sba("  "); bg_text(12, 11, sb); }
void drain_enemy(uint8_t to) { while (ehp > to) { ehp--; bar(1, 2, ehp, emax); frame(); } }
void drain_hero(uint8_t to) { while (hhp > to) { hhp--; bar(11, 10, hhp, hmax); hero_num(); frame(); } }
void fill_hero(void) { while (hhp < hmax) { hhp++; bar(11, 10, hhp, hmax); hero_num(); frame(); } }
const boss_t *B;
void draw_enemy(uint8_t show) {
    if (!show) { hide_ent(1); hide_ent(2); return; }
    if (B->spr1 != 255) {
        if (B->spr2 != 255) { put_char(1, 1, 100, 32, 0); put_char(2, 2, 128, 32, 0); }
        else put_char(1, 1, 116, 32, 0);
    }
}
void battle_screen(void) {
    load_scene(B->scene);
    BGP_REG = OBP0_REG = 0;
    load_char(0, ci);
    if (B->spr1 != 255) load_char(1, B->spr1);
    if (B->spr2 != 255) load_char(2, B->spr2);
    draw_enemy(1); put_char(0, 0, 28, 60, 0);
    bg_text(1, 1, B->name); bar(1, 2, ehp, emax);
    bg_text(11, 9, HERO[ci]); bar(11, 10, hhp, hmax); hero_num();
    fade_in();
}
void enemy_hit(void) {
    uint8_t i;
    sfx_hit();
    for (i = 0; i < 3; i++) { draw_enemy(0); if (B->spr1 == 255) BGP_REG = 0x1B; frames(4); draw_enemy(1); BGP_REG = 0xE4; frames(4); }
}
void hero_hit(void) {
    uint8_t i;
    sfx_hit();
    for (i = 0; i < 4; i++) { put_char(0, 0, 25, 60, 0); frames(3); put_char(0, 0, 31, 60, 0); frames(3); }
    put_char(0, 0, 28, 60, 0);
}
/* Combate por turnos: devuelve cuando el jefe cae. */
void battle(void) {
    uint8_t c, uses = 3, dmg, a, i, power3 = POWER[3];
    const char *opts[4];
    for (i = 0; i < 4; i++) { strcpy(mv[i], MOVES[ci][i]); opts[i] = mv[i]; }
    if (ci == CI_GERARD && docs) { strcpy(mv[3], "SUPER ENVÍO"); power3 = 14 + 6 * docs; uses = 2; }
    sb0(); sba("¡"); sba(B->name); sba(" te desafía!"); sayb();
    for (;;) {
        sb0(); sba(ci == CI_GERARD && docs ? "SUPER ENVÍO" : MOVES[ci][3]); sba(" ×"); sbu(uses); strcpy(mv[3], sb);
        c = choose(opts, 4);
        if (c == 3 && !uses) { say("¡No quedan usos! Elige otro movimiento."); continue; }
        if (c == 3) uses--;
        sb0(); sba("¡"); sba(HERO[ci]); sba(" usa "); sba(c == 3 && ci == CI_GERARD && docs ? "SUPER ENVÍO" : MOVES[ci][c]); sba("!"); sayb();
        dmg = (c == 3 ? power3 : POWER[c]) + (rnd() & 7);
        enemy_hit();
        drain_enemy(ehp > dmg ? ehp - dmg : 0);
        if (c == 3) say(ci == CI_GERARD && docs ? "¡¡DOCUMENTOS EN REGLA!!" : "¡Golpe crítico!");
        else if (ci == CI_IVAN && c == 1) say("¡La oferta FLiiTS convence!");
        if (!ehp) return;
        a = rnd() & 3;
        sb0(); sba("¡"); sba(B->name); sba(" usa "); sba(B->atk[a]); sba("!"); sayb();
        dmg = 7 + (rnd() & 7);
        hero_hit();
        drain_hero(hhp > dmg ? hhp - dmg : 0);
        if (!hhp) {
            sb0(); sba("¡"); sba(HERO[ci]); sba(" está agotado!"); sayb();
            say("¡Pero el equipo no se rinde! ¡Otra vez!");
            fill_hero();
        }
    }
}

/* ------------------------------------------------------------------ tarjeta de moneda */
void coin_card(void) {
    fade_out(); load_scene(SC_COIN); BGP_REG = OBP0_REG = 0;
    bg_ctext(9, "¡MONEDA DE ORO!");
    bg_ctext(11, HERO[ci]); bg_ctext(12, "MISIÓN SUPERADA");
    sb0(); sba(M->lab1); sba(": "); sbu(cnt1); bg_ctext(14, sb);
    sb0(); sba(M->lab2); sba(": "); sbu(cnt2); bg_ctext(15, sb);
    bg_ctext(17, "PULSA A");
    fade_in(); sfx_win(); wait_a();
}

/* ------------------------------------------------------------------ mapa */
void status(void) {
    uint8_t i;
    fill_win_rect(0, 0, 20, 2, 0);
    for (i = 0; i < 3; i++) set_win_tile_xy(1 + i, 0, i < hearts ? T_HEART : T_HEART0);
    win_text(5, 0, M->zone);
    sb0(); sba("QUEDAN "); sbu(nleft); win_text(1, 1, sb); win_text(11, 1, "A: HABLAR");
}
void draw_npcs(void) {
    uint8_t k;
    for (k = 0; k < 6; k++) if (alive[k]) put_char(1 + k, 1 + k, M->npcs[k].gx * 16, M->npcs[k].gy * 16 - 16, 0); else hide_ent(1 + k);
}
void draw_player(uint8_t bob) { put_char(0, 0, px, py - 16 - bob, pflip ? S_FLIPX : 0); }
uint8_t blocked(uint8_t gx, uint8_t gy) {
    uint8_t k;
    if (gx > 9 || gy < 1 || gy > 5) return 1;
    if (M->solid[gy * 10 + gx]) return 1;
    for (k = 0; k < 6; k++) if (alive[k] && M->npcs[k].gx == gx && M->npcs[k].gy == gy) return 1;
    return 0;
}
void map_screen(void) {
    uint8_t k;
    load_scene(M->scene); BGP_REG = OBP0_REG = 0;
    load_char(0, ci);
    for (k = 0; k < 6; k++) load_char(1 + k, done_[k] && ci == CI_AZLAN ? CH_NPC + k : M->npcs[k].spr);
    draw_npcs(); draw_player(0);
    status_fn = status; status(); move_win(7, WIN_STATUS);
    fade_in();
}
void lose_heart(const char *hint) {
    sfx_bad(); hearts--;
    sb0(); sba(hint); sba(" (♥ -1)"); sayb();
}
/* Devuelve 1 si se pierden todos los corazones. */
uint8_t talk(uint8_t k) {
    const npcd_t *n = &M->npcs[k];
    uint8_t c, a = n->a, i;
    if (done_[k]) { sb0(); sba(n->name); sba(": ¡Ya voy completo! ¡Gracias!"); sayb(); return 0; }
    if (ci == CI_IVAN || ci == CI_RAUL) { sb0(); sba(n->name); sba(": "); sba(n->t1); sayb(); }
    else if (ci == CI_GERARD) {
        sb0(); sba(HERO[ci]); sba(": ¿Me enseñas tu "); sba(n->t1); sba("?"); sayb();
        sb0(); sba(n->name); sba(": "); sba(n->t2); sayb();
    }
    for (;;) {
        if (ci == CI_AZLAN) {
            for (i = 0; i < 4; i++) if (need[k] & (1 << i)) break;
            sb0(); sba(n->name); sba(": ¡Me falta "); sba(EPI_NAME[i]); sba("!"); sayb();
        }
        c = choose(M->opts, M->nopts);
        if (ci == CI_IVAN) {
            if (c != a) { lose_heart(a == 0 ? "¡Ojo! Si hace GLOVO + UBER, se le ofrece un CONTRATO." : a == 1 ? "¡Ojo! Solo Glovo no puede ser." : "¡Ojo! Ya es de FLiiTS... y falla."); }
            else {
                sfx_coin(); sb0();
                if (a == 0) { sba("¡CONTRATO FIRMADO! ¡"); sba(n->name); sba(" CONTRATADO!"); cnt1++; }
                else if (a == 1) { sba("¡"); sba(n->name); sba(" RECHAZADO! Solo Glovo no puede ser."); }
                else { sba("¡"); sba(n->name); sba(" DESPEDIDO!"); cnt2++; }
                sayb(); break;
            }
        } else if (ci == CI_GERARD) {
            if ((c == 0) != a) {
                sb0(); if (a) { sba("¡Ojo! El "); sba(n->t1); sba(" de "); sba(n->name); sba(" estaba en regla."); }
                else { sba("¡Ojo! Ese "); sba(n->t1); sba(" no es válido."); }
                lose_heart(sb);
            } else if (a) {
                sfx_coin(); cnt1++; docs++;
                sb0(); sba("¡APROBADO! El "); sba(n->t1); sba(" va a la mochila. Llevas "); sbu(docs); sba("."); sayb(); break;
            } else {
                sfx_bad(); cnt2++;
                sb0(); sba("¡RECHAZADO! "); sba(n->name); sba(": Volveré con los papeles en regla."); sayb(); break;
            }
        } else if (ci == CI_AZLAN) {
            if (!(need[k] & (1 << c))) { sb0(); sba("¡"); sba(n->name); sba(" ya tiene "); sba(EPI_NAME[c]); sba("!"); lose_heart(sb); }
            else {
                need[k] &= ~(1 << c); cnt1++; sfx_coin();
                sb0(); sba(EPI_UP[c]); sba(" ENTREGADO!"); sayb();
                if (!need[k]) {
                    cnt2++; done_[k] = 1; load_char(1 + k, CH_NPC + k);
                    sb0(); sba(n->name); sba(": ¡Completo! ¡Gracias!"); sayb(); break;
                }
                continue;
            }
        } else {
            if (c != a) lose_heart("Así no... ¡piensa en el nuevo sistema de citas!");
            else { sfx_coin(); if (a == 0) { cnt1++; cnt2++; } if (a == 3) cnt2++; say(n->t2); break; }
        }
        if (!hearts) return 1;
    }
    nleft--;
    if (ci != CI_AZLAN) { alive[k] = 0; draw_npcs(); }
    return 0;
}
/* 0 = misión completada · 1 = sin corazones */
uint8_t map_loop(void) {
    uint8_t d, tx, ty, k, s;
    for (;;) {
        frame();
        d = 255;
        if (cur & J_UP) d = 0; else if (cur & J_DOWN) d = 1; else if (cur & J_LEFT) d = 2; else if (cur & J_RIGHT) d = 3;
        if (d != 255) {
            pdir = d; if (d == 2) pflip = 1; else if (d == 3) pflip = 0;
            tx = pgx + DX[d]; ty = pgy + DY[d];
            if (!blocked(tx, ty)) {
                for (s = 0; s < 8; s++) { px += DX[d] * 2; py += DY[d] * 2; draw_player((s >> 2) & 1); frame(); }
                pgx = tx; pgy = ty;
            } else draw_player(0);
        }
        if (hit(J_A)) {
            tx = pgx + DX[pdir]; ty = pgy + DY[pdir];
            for (k = 0; k < 6; k++) if (alive[k] && M->npcs[k].gx == tx && M->npcs[k].gy == ty) break;
            if (k < 6) {
                if (talk(k)) return 1;
                if (!nleft) return 0;
                box_close();
            } else if (tx < 10 && ty < 9 && M->solid[ty * 10 + tx] == 2) {
                sb0(); sba("Cartel: "); sba(M->zone); sayb(); box_close();
            }
        }
    }
}
void mission(void) {
    uint8_t k;
    M = &MISSIONS[ci];
restart:
    for (k = 0; k < 6; k++) { alive[k] = 1; need[k] = M->npcs[k].a; done_[k] = 0; }
    pgx = M->sx; pgy = M->sy; px = pgx * 16; py = pgy * 16; pdir = 1; pflip = 0;
    hearts = 3; nleft = 6; cnt1 = cnt2 = docs = 0;
    fade_out(); map_screen();
    sb0(); sba("MISIÓN: "); sba(M->title); sayb();
    say(M->r1); say(M->r2); box_close();
    if (map_loop()) {
        sb0(); sba("¡"); sba(HERO[ci]); sba(" está agotado! Vuelves al inicio de la misión..."); sayb();
        goto restart;
    }
    if (ci == CI_RAUL) { sfx_win(); say("¡Oficina en orden ✓! Todos tienen su cita y su turno."); return; }
    B = ci == CI_IVAN ? &BOSS_MARTI : ci == CI_GERARD ? &BOSS_VALID : &BOSS_ALMACEN;
    sb0(); sba("¡Todos atendidos! Ahora, a por "); sba(B->name); sba("."); sayb();
    fade_out(); ehp = emax = B->hp; hhp = hmax = 100; battle_screen();
    if (ci == CI_IVAN) say("MARTÍ: ¡¡TU FORECAST NO CUADRA!!");
    else if (ci == CI_GERARD) { say("VALIDACIÓN: ¡¡RECHAZADO!! ¡¡RECHAZADO!!"); say("Usa tus documentos: ¡SUPER ENVÍO!"); }
    else say("MARTÍ Y FRANCO: ¡¡AQUÍ NO SE ENTREGA NADA!!");
    battle();
    sfx_win(); flash(2);
    if (ci == CI_IVAN) { say("MARTÍ: ¿Y estas contrataciones...?"); say("MARTÍ: ¡¡FORECAST PERFECTO!!"); }
    else if (ci == CI_GERARD) { say("¡¡ALL APPROVED!!"); say("VALIDACIÓN: ¡Gracias!"); }
    else { say("FRANCO: ¡Bienvenido al almacén!"); say("¡EPIs 100% entregados en Catalunya!"); }
    coins[ci] = 1;
    coin_card();
}

/* ------------------------------------------------------------------ final */
void final_battle(void) {
    uint8_t i, t;
    fade_out(); B = &BOSS_BAGS; ci = CI_RAUL; ehp = emax = B->hp; hhp = hmax = 140; battle_screen();
    say("¡Las MOCHILAS GIGANTES no dejan repartir en Catalunya!");
    say("MOCHILAS: ¡¡NO HAY MÁS ZONAS!!");
    say("¡Ivan, Gerard y Azlan te dan sus monedas de oro!");
    /* tres monedas suben hacia Raül */
    for (t = 0; t < 48; t++) {
        for (i = 0; i < 3; i++) {
            uint8_t x = 8 + i * 20, y = 130 - t;
            set_sprite_tile(28 + i * 2, 120 + ((t >> 2) & 1) * 4); set_sprite_tile(29 + i * 2, 122 + ((t >> 2) & 1) * 4);
            set_sprite_prop(28 + i * 2, 0); set_sprite_prop(29 + i * 2, 0);
            move_sprite(28 + i * 2, x + 8, y + 16); move_sprite(29 + i * 2, x + 16, y + 16);
        }
        if (!(t & 7)) sfx_blip();
        frame();
    }
    for (i = 28; i < 34; i++) move_sprite(i, 0, 0);
    sfx_win(); flash(4);
    say("¡¡PODER FLiiTS!!");
    battle();
    sfx_win(); flash(3);
    say("MOCHILAS: ¡¡ESTÁ BIEN!! ¡¡ESTÁ BIEN!!");
    say("¡¡NUEVAS ZONAS PARA REPARTIR!!");
}

/* ------------------------------------------------------------------ créditos */
void credits(void) {
    fade_out(); load_scene(SC_BLANK); BGP_REG = OBP0_REG = 0;
    bg_ctext(1, "FLiiTS CATALUNYA"); bg_ctext(3, "EL EQUIPO");
    load_char(0, CI_IVAN); load_char(1, CI_GERARD); load_char(2, CI_AZLAN); load_char(3, CI_RAUL); load_char(4, CH_DOG);
    put_char(0, 0, 12, 40, 0); put_char(1, 1, 52, 40, 0); put_char(2, 2, 92, 40, 0); put_char(3, 3, 132, 40, 0);
    bg_text(1, 10, "IVAN"); bg_text(5, 11, "GERARD"); bg_text(10, 10, "AZLAN"); bg_text(16, 11, "RAÜL");
    put_wide(4, 4, 64, 100); bg_ctext(15, "OREO");
    bg_ctext(17, "PULSA A");
    fade_in(); wait_a();
    fade_out(); load_scene(SC_BLANK); BGP_REG = OBP0_REG = 0;
    bg_ctext(2, "COLABORACIÓN"); bg_ctext(3, "ESPECIAL");
    load_char(0, CH_MARTI); load_char(1, CH_MARTIB); load_char(2, CH_FRANCO);
    put_char(0, 0, 24, 56, 0); put_char(1, 1, 72, 56, 0); put_char(2, 2, 120, 56, 0);
    bg_text(1, 12, "MARTÍ"); bg_text(1, 13, "PUJOL"); bg_text(7, 12, "MARTÍ"); bg_text(7, 13, "ESPRIU"); bg_text(13, 12, "FRANCO");
    bg_ctext(16, "PULSA A");
    fade_in(); wait_a();
    fade_out(); load_scene(SC_BLANK); BGP_REG = OBP0_REG = 0;
    bg_ctext(5, "¡GRACIAS POR JUGAR!");
    bg_ctext(8, "FLiiTS"); bg_ctext(9, "CATALUNYA WORLD");
    bg_ctext(11, "18 · 11 · 2026");
    bg_ctext(15, "PULSA START");
    fade_in(); sfx_win(); wait_a();
}

/* ------------------------------------------------------------------ portada y selector */
void title(void) {
    uint8_t t = 0;
    fade_out(); load_scene(SC_TITLE); BGP_REG = OBP0_REG = 0;
    bg_ctext(6, "CATALUNYA WORLD"); bg_ctext(8, "EDICIÓN PORTÁTIL");
    load_char(0, CI_IVAN); load_char(1, CI_GERARD); load_char(2, CI_AZLAN); load_char(3, CI_RAUL); load_char(4, CH_DOG);
    put_char(0, 0, 16, 72, 0); put_char(1, 1, 52, 72, 0); put_char(2, 2, 92, 72, 0); put_char(3, 3, 128, 72, S_FLIPX);
    put_wide(4, 4, 64, 106);
    fade_in();
    for (;;) {
        frame(); t++;
        if (t == 1 || t == 33) bg_ctext(16, t == 1 ? "PULSA START" : "           ");
        if (t >= 64) t = 0;
        if (hit(J_START | J_A)) break;
    }
    sfx_sel();
    say("¡Las MOCHILAS GIGANTES bloquean Catalunya!");
    say("Consigue las 3 monedas de oro con Ivan, Gerard y Azlan...");
    say("...y Raül tendrá el PODER FLiiTS.");
}
const uint8_t ORDER[4] = {CI_IVAN, CI_GERARD, CI_AZLAN, CI_RAUL};
const char *const DESC1[4] = {"CONTRATA A QUIEN", "PIDE LOS PAPELES", "¡NADIE SE VA SIN", "ORDENA LA OFICINA"};
const char *const DESC2[4] = {"HAGA GLOVO + UBER", "A TODOS", "SU EQUIPO!", "Y SALVA CATALUNYA"};
uint8_t unlocked_shown;
uint8_t select_hero(void) {
    uint8_t i, c = 0, n, locked;
    n = coins[0] + coins[2] + coins[3];
    locked = n < 3;
    fade_out(); load_scene(SC_BLANK); BGP_REG = OBP0_REG = 0;
    OBP1_REG = 0xFF;
    bg_ctext(1, "ELIGE PERSONAJE");
    sb0(); sba("MONEDAS: "); sbu(n); sba("/3"); bg_ctext(2, sb);
    for (i = 0; i < 4; i++) {
        load_char(i, ORDER[i]);
        put_char(i, i, 12 + i * 40, 44, (i == 3 && locked) ? S_PALETTE : 0);
        if (i < 3 && coins[ORDER[i]]) set_bkg_tile_xy(2 + i * 5, 12, T_COIN);
    }
    bg_text(1, 10, "IVAN"); bg_text(5, 11, "GERARD"); bg_text(10, 10, "AZLAN"); bg_text(16, 11, locked ? "???" : "RAÜL");
    bg_ctext(17, "A: JUGAR");
    fade_in();
    if (!locked && !unlocked_shown) { unlocked_shown = 1; sfx_win(); say("¡RAÜL DESBLOQUEADO! Ya tienes las 3 monedas de oro."); box_close(); c = 3; }
    for (;;) {
        for (i = 0; i < 4; i++) set_bkg_tile_xy(2 + i * 5, 5, i == c ? T_MORE : 0);
        fill_bkg_rect(0, 14, 20, 2, 0);
        if (c == 3 && locked) { bg_ctext(14, "CONSIGUE LAS 3"); bg_ctext(15, "MONEDAS DE ORO"); }
        else { bg_ctext(14, DESC1[c]); bg_ctext(15, DESC2[c]); }
        do {
            frame();
            if (hit(J_LEFT)) { c = c ? c - 1 : 3; sfx_blip(); break; }
            if (hit(J_RIGHT)) { c = (c + 1) & 3; sfx_blip(); break; }
            if (hit(J_A | J_START)) {
                if (c == 3 && locked) { sfx_bad(); say("Consigue antes las monedas de Ivan, Gerard y Azlan."); box_close(); break; }
                sfx_sel(); return ORDER[c];
            }
        } while (1);
    }
}

void main(void) {
    DISPLAY_OFF;
    LCDC_REG = LCDCF_OFF | LCDCF_WIN9C00 | LCDCF_WINON | LCDCF_BG8800 | LCDCF_BG9800 | LCDCF_OBJ16 | LCDCF_OBJON | LCDCF_BGON;
    bg_load(0, COMMON_N, common_tiles);
    set_sprite_data(112, 2, spr_tiles + SPR_BANG * 16);
    set_sprite_data(120, 8, spr_tiles + SPR_COIN * 16);
    fill_win_rect(0, 0, 20, 18, 0);
    BGP_REG = OBP0_REG = 0xE4; OBP1_REG = 0xFF;
    SHOW_BKG; SHOW_WIN; SHOW_SPRITES; SPRITES_8x16;
    DISPLAY_ON;
    snd_init();
    for (;;) {
        coins[0] = coins[1] = coins[2] = coins[3] = 0; unlocked_shown = 0;
        title();
        for (;;) {
            ci = select_hero();
            mission();
            if (ci == CI_RAUL) break;
        }
        final_battle();
        credits();
    }
}
