/* AVENTURA DE PLATAFORMAS: las 3 pantallas (Ivan/contratar, Gerard/papeles, Azlan/EPIs) con sus jefes.
   Portado de SCREENS, genLevel, physics, updNpcs, npcHire/npcDocs/npcEpis, shoot, bossStep, drawPlay… del web. */
#include "plat.h"

enum { T_HIRE, T_DOCS, T_EPIS };
enum { IT_CASCO, IT_CHAQ, IT_GUANT, IT_MOCH, IT_MONSTER, IT_CONTRATO, IT_TICKET, IT_FAVOR };
static const char *const ITEM_TXT[8] = {"+3 CASCOS", "+3 CHAQUETAS", "+3 GUANTES", "+3 MOCHILAS", "¡MONSTER! +1 VIDA", "+1 CONTRATO", "¡TICKET DE UBER!", "¡NEREA TE DEBE UN FAVOR!"};
static const char *const INAME[4] = {"el casco", "la chaqueta", "los guantes", "la mochila"};
static const char *const ENAME[4] = {"casco", "chaqueta", "guantes", "mochila"};
static const char *const EPLUR[4] = {"cascos", "chaquetas", "guantes", "mochilas"};
static const char *const EUP[4] = {"CASCO", "CHAQUETA", "GUANTES", "MOCHILA"};

typedef struct { s16 len0, len; u8 type, who; const char *c1, *c2, *boss, *hint; land_t land[5]; u8 nland; s16 px[5]; const char *pn[5]; u8 np; } scr_t;
static const scr_t SCR[3] = {
    {1320, 2000, T_HIRE, 0, "Contratados", "Despedidos", "JEFE: MARTÍ", "¡Uber + Glovo se contrata, el resto fuera!",
     {{L_SAGRADA, 175}, {L_VALLES, 560}}, 2, {150, 900}, {"Barcelona", "Sabadell"}, 2},
    {1320, 2000, T_DOCS, 2, "Aprobados", "Rechazados", "JEFE: VALIDACIÓN", "¡Pide los papeles a todos!",
     {{L_SAGRADA, 175}, {L_BEACH, 410}, {L_MATARO, 600}}, 3, {150, 620, 1000}, {"Barcelona", "Gavà", "Mataró"}, 3},
    {1460, 2100, T_EPIS, 3, "EPIs entregados", "Riders completos", "ALMACÉN EPIs", "¡Nadie se va sin su equipo!",
     {{L_SEU, 170}, {L_AQUEDUCT, 320}, {L_MONASTERY, 470}, {L_INDUSTRIAL, 595}, {L_VALLES, 690}}, 5,
     {140, 440, 740, 990, 1180}, {"Lleida", "Martorell", "Sant Cugat", "Molins de Rei", "Parets del Vallès"}, 5},
};
/* aspirantes */
static const struct { u8 robe; const char *say; u8 act; } HIRE_N[8] = {
    {0, "¡Glovo y Uber!", 0}, {0, "Solo Glovo", 1}, {1, "Solo Glovo", 2}, {0, "¡Glovo y Uber!", 0},
    {1, "Mañana no trabajo", 2}, {0, "Solo Glovo", 1}, {0, "¡Glovo y Uber!", 0}, {1, "¡Este pedido está muy lejos!", 2}};
#define ACT_HIRE 0
#define ACT_REJECT 1
#define ACT_FIRE 2
static const struct { const char *doc; u8 ok; const char *ans; } DOCS_N[8] = {
    {"NIE", 1, "Aquí tienes mi NIE"}, {"Carnet de moto", 0, "Tengo carnet extranjero"}, {"Pasaporte", 1, "¡Pasaporte en regla!"},
    {"Permiso de trabajo", 0, "Me caducó en 2025..."}, {"Carnet de moto", 1, "Carnet español, ¡toma!"}, {"NIE", 0, "NIE caducado..."},
    {"Permiso de trabajo", 1, "Permiso vigente"}, {"Pasaporte", 0, "Pasaporte caducado"}};
static const s8 EPIS_N[7][4] = {{0, -1}, {1, 2, -1}, {3, -1}, {0, 2, -1}, {1, -1}, {3, 0, -1}, {2, 1, 3, -1}};

/* ---------------------------------------------------------------- estado */
static const scr_t *S; static int si, CI, VEHI, LEN, RHh, STARTT;
static s16 PLX[5]; static land_t LND[5];
static struct { float x, y, vx, vy; u8 on, flip; int step; } pl;
static struct { float x; u8 flip; int step; } dog;
static int c1, c2, docs, hireN, life, inv, gameOverAt, bagDocs, lastShot, st_t;
static int it_contracts, it_tickets, it_favor, it_epi[4];
typedef struct { s16 x, y; u8 item, used; s8 bump; } box_t;
typedef struct { s16 x0, x1; } hole_t;
typedef struct { s16 x; u8 type; } obst_t;
typedef struct { s16 x, y; u8 got; } coin_t;
typedef struct { s16 x, y; u8 item, got; } loose_t;
static box_t boxes[12]; static int nbox;
static hole_t holes[8]; static int nhole;
static obst_t obst[8]; static int nobst;
static coin_t coinsA[64]; static int ncoin;
static loose_t loose[8]; static int nloose;
static s16 plats[8][3]; static int nplat;
static s16 tops[24][3]; static int ntop;
enum { NS_WALK, NS_ACT, NS_HOLD, NS_LEAVE, NS_GONE, NS_WAIT };
typedef struct { s32 x; s16 home; u8 look, state, flip, moving, flash, gear, spoke, missed, act, ok; s8 dir; int t, step, cue, hold;
    const char *say, *doc, *ans; s8 need[3]; u8 nneed; s8 lastNeed, warned; } npc_t;
static npc_t npcs[8]; static int nnpc;
typedef struct { float x, y0, y; int t; s8 k; u8 item; } proj_t;
static proj_t proj[8]; static int nproj;
static struct { u8 on, good, ko, stamped, charge, cheer, got, epiTip, hascoin; int t, end; float cx, cy; int ct;
    struct { float x; u8 look, gear; } rec[3];
    u8 del_on, ditem, dr; int di, dt; float dwho; } B;
static bmp_t *wh_bmp; static u32 mark_g; static rle_t RR[4], DR[4];
s32 plat_pool; s32 plat_dbgv[16];

static s32 PLX256;
static int stopX(void) { return LEN - 190; }
static int headY(void) { return iround(pl.y) - RHh - 2; }
static int bxW(void) { return LEN - 110; }

/* ---------------------------------------------------------------- bocadillos: anclas */
static void play_anchor(bub_t *b, int *x, int *y) {
    switch (b->an) {
    case AN_PL: *x = iround(pl.x); *y = headY() + b->dy; break;
    case AN_NPC: { npc_t *k = &npcs[b->ref & 0xff]; *x = (k->x + 128) >> 8; *y = b->wy - ((b->ref & 0x100) && (k->gear & G_CASCO) ? 3 : 0); } break;
    case AN_PLANE: *x = iround(pcam + plane.sx + 4); *y = iround(plane.sy - 14); break;
    default: break;
    }
}
#define SAY_PL(t, dy, fr, cls) say(t, AN_PL, 0, 0, dy, fr, cls)
#define SAY_NPC(t, i, y, fr, cls) say(t, AN_NPC, i, 0, y, fr, cls)

/* ---------------------------------------------------------------- nivel */
static void box_add(int x, int item, int by) { box_t *b = &boxes[nbox++]; b->x = x; b->y = by; b->item = item; b->used = 0; b->bump = 0; }
static int nNpcDef(void) { return S->type == T_EPIS ? 7 : 8; }
static void gen_level(void) {
    nbox = nhole = nobst = ncoin = nloose = nplat = 0;
    int n = nNpcDef(), A = 250, Bx = LEN - 330; s16 xs[8];
    for (int i = 0; i < n; i++) xs[i] = iround(A + (float)(Bx - A) * i / (n - 1));
    int by = iround(GY - RHh - 20);
    for (int i = 0; i < n - 1; i++) { int x = xs[i] + 50;
        if (i % 2) { holes[nhole].x0 = x - 18; holes[nhole].x1 = x + 18; nhole++;
            int y = GY - (i % 4 == 1 ? 40 : 44); plats[nplat][0] = x - 24; plats[nplat][1] = y; plats[nplat][2] = 3; nplat++;
            for (int k = 0; k < 3; k++) { coinsA[ncoin].x = x - 16 + k * 16; coinsA[ncoin].y = y - 10; coinsA[ncoin].got = 0; ncoin++; }
            if (S->type == T_EPIS) { loose[nloose].x = x; loose[nloose].y = y - 12; loose[nloose].item = (i >> 1) % 4; loose[nloose].got = 0; nloose++; } }
        else { obst[nobst].x = x; obst[nobst].type = (i >> 1) % 3; nobst++; } }
    if (S->type == T_HIRE) { for (int j = 0; j < n; j++) if (HIRE_N[j].act == ACT_HIRE) box_add(xs[j] - 60, IT_CONTRATO, by);
        box_add(xs[2] - 60, IT_MONSTER, by); box_add(xs[5] - 60, IT_MONSTER, by); }
    else if (S->type == T_DOCS) { int first = 1; for (int j = 0; j < n; j++) if (!DOCS_N[j].ok) { box_add(xs[j] - 60, first ? IT_FAVOR : IT_TICKET, by); first = 0; }
        box_add(xs[2] - 60, IT_MONSTER, by); box_add(xs[6] - 60, IT_MONSTER, by); }
    else { box_add(xs[0] - 140, IT_CASCO, by); box_add(xs[0] - 70, IT_CHAQ, by); box_add(xs[1] - 60, IT_GUANT, by); box_add(xs[2] - 60, IT_MOCH, by);
        box_add(xs[4] - 60, IT_MONSTER, by); box_add(xs[6] - 60, IT_MONSTER, by); }
    for (int i = 0; i < nbox; i++) for (int k = 0; k < 3; k++) { coinsA[ncoin].x = boxes[i].x - 16 + k * 16; coinsA[ncoin].y = GY - 8; coinsA[ncoin].got = 0; ncoin++; }
    ntop = 0;
    for (int i = 0; i < nplat; i++) { tops[ntop][0] = plats[i][0]; tops[ntop][1] = plats[i][0] + plats[i][2] * 16; tops[ntop][2] = plats[i][1]; ntop++; }
    for (int i = 0; i < nbox; i++) { tops[ntop][0] = boxes[i].x - 8; tops[ntop][1] = boxes[i].x + 8; tops[ntop][2] = boxes[i].y; ntop++; }
    /* aspirantes */
    nnpc = n;
    for (int i = 0; i < n; i++) { npc_t *k = &npcs[i]; memset(k, 0, sizeof *k);
        k->x = iround(A + (float)(Bx - A) * i / (n - 1)) << 8; k->home = i * 57; k->look = i % 6; k->state = NS_WALK; k->flip = 1; k->dir = -1; k->lastNeed = -1; k->warned = -1;
        if (S->type == T_HIRE) { k->gear = HIRE_N[i].robe ? 15 : 0; k->say = HIRE_N[i].say; k->act = HIRE_N[i].act; }
        else if (S->type == T_DOCS) { k->doc = DOCS_N[i].doc; k->ok = DOCS_N[i].ok; k->ans = DOCS_N[i].ans; }
        else { k->state = NS_WAIT; k->gear = 15; k->nneed = 0; for (int j = 0; j < 3 && EPIS_N[i][j] >= 0; j++) { k->need[k->nneed++] = EPIS_N[i][j]; k->gear &= ~(1 << EPIS_N[i][j]); } }
    }
}
static int overHole(float x) { for (int i = 0; i < nhole; i++) if (x > holes[i].x0 + 3 && x < holes[i].x1 - 3) return 1; return 0; }

static void start_screen(void) {
    c1 = c2 = docs = hireN = 0; memset(&B, 0, sizeof B); nfx = 0; nproj = 0; clear_bubbles();
    pl.x = 40; pl.y = GY; pl.vx = pl.vy = 0; pl.on = 1; pl.flip = 0; pl.step = 0; pcam = 0; dog.x = 0; dog.step = 0; dog.flip = 0;
    life = 3; inv = 0; gameOverAt = 0; it_contracts = it_tickets = it_favor = 0; for (int i = 0; i < 4; i++) it_epi[i] = 0;
    bagDocs = 0; lastShot = -100; st_t = 0; plane.sx = 150; plane.sy = 26;
    gen_level();
}

/* ---------------------------------------------------------------- objetos */
static void give(int it, int hasxy, int x, int y) {
    if (it == IT_MONSTER) { if (life < 3) life++; else SCORE += 500; }
    else if (it == IT_CONTRATO) it_contracts++;
    else if (it == IT_TICKET) it_tickets++;
    else if (it == IT_FAVOR) it_favor++;
    else it_epi[it] += hasxy ? 3 : 1;
    SCORE += 200; sfx_coin();
    if (hasxy) { pfx_t *p = fx_add(FX_ITEM); if (p) { p->item = it; p->x = x; p->y = y; p->life = 46; }
        say(ITEM_TXT[it], AN_FIX, 0, x, y - 12, 70, it == IT_MONSTER ? BC_OK : BC_ME); }
}
static void hit_box(box_t *b) { if (b->used) return; b->used = 1; b->bump = 10; sfx_power(); give(b->item, 1, b->x, b->y - 4); }
static void lose_life(const char *msg) {
    if (inv > 0 || gameOverAt) return;
    life--; inv = 90; sfx_hit(); SAY_PL(msg, 0, 70, BC_NO);
    if (life <= 0) { gameOverAt = pfr + 110; SAY_PL("¡SIN VIDA! VUELVE A EMPEZAR", -16, 110, BC_BAD); }
}

/* ---------------------------------------------------------------- aspirantes */
static float walk_toward(npc_t *k) {
    s32 dx = PLX256 - k->x, ox = k->x;
    if (dx < 150 * 256 && dx > -150 * 256) { k->x += (dx > 0 ? 115 : dx < 0 ? -115 : 0); k->flip = dx < 0; k->moving = 1; }
    else { s32 v = isin(((pfr + k->home) * 1854) >> 10) * 77 / 4096; k->x += v; k->flip = v < 0; k->moving = v > 26 || v < -26; }
    for (int i = 0; i < nhole; i++) if (k->x > (holes[i].x0 - 6) * 256 && k->x < (holes[i].x1 + 6) * 256) { k->x = ox; k->moving = 0; break; }
    return dx / 256.0f;
}
static void leave(npc_t *k, int dir, int wait) { k->state = NS_HOLD; k->hold = k->t + wait; k->dir = dir; }
static int touching(npc_t *k) { s32 d = PLX256 - k->x; return d < 12 * 256 && d > -12 * 256 && pl.y > GY - 14; }
static void cue_for(npc_t *k) {
    char b[64];
    if (S->type == T_HIRE) {
        if (k->act == ACT_HIRE) { plane_say("¡CONTRATA!", BC_OK, 80); sfx_select(); }
        else if (k->act == ACT_FIRE) { plane_say("¡DESPIDE!", BC_NO, 80); sfx_back(); }
        else { plane_say("¡ESE NO!", BC_NO, 80); sfx_back(); }
    } else if (S->type == T_DOCS) {
        SAY_NPC(k->ans, k - npcs, GY - 28, 100, BC_CAND);
        if ((k->home / 57) % 2) plane_say("¡Mira que no esté caducado!", BC_OK, 90);
        else { const char *w = !strcmp(k->doc, "NIE") ? "el NIE" : !strcmp(k->doc, "Pasaporte") ? "el pasaporte" : !strcmp(k->doc, "Carnet de moto") ? "el carnet" : "el permiso";
            char *p = str_cat(b, "¡Revisa "); p = str_cat(p, w); str_cat(p, "!"); plane_say(b, BC_OK, 90); }
    }
}
static void npc_hire(npc_t *k) {
    int i = k - npcs;
    if (k->state == NS_WALK) { float dx = walk_toward(k);
        if (dx < 110 && dx > -110 && !k->spoke) { k->spoke = 1; SAY_NPC(k->say, i | 0x100, GY - 28, 110, BC_CAND); k->cue = pfr + 30; }
        if (touching(k)) { if (!k->spoke) { k->spoke = 1; SAY_NPC(k->say, i, GY - 28, 90, BC_CAND); } k->state = NS_ACT; k->t = 0; sfx_bump(); } }
    else if (k->state == NS_ACT && k->t == 18) {
        if (k->act == ACT_HIRE && it_contracts <= 0) { k->missed = 1; sfx_back(); SAY_PL("¡SIN CONTRATOS! Te has saltado una caja", 0, 110, BC_NO); SAY_NPC("Pues me voy...", i, GY - 40, 80, BC_CAND); leave(k, -1, 30); }
        else if (k->act == ACT_HIRE) { it_contracts--; k->gear = 15; k->flash = 24; c1++; hireN++; SCORE += 500; sfx_power(); SAY_NPC("¡CONTRATADO!", i, GY - 46, 80, BC_OK); SAY_PL(hireN % 2 ? "¡Tú a Sabadell!" : "¡Tú a Barcelona!", 0, 90, BC_ME); leave(k, 1, 48); }
        else if (k->act == ACT_FIRE) { k->gear = 0; k->flash = 24; c2++; SCORE += 300; sfx_hit(); SAY_NPC("¡DESPEDIDO!", i, GY - 40, 80, BC_NO); leave(k, -1, 48); }
        else { sfx_back(); SAY_PL("Solo Glovo no puede ser", 0, 90, BC_ME); leave(k, -1, 30); }
    }
}
static void npc_docs(npc_t *k) {
    int i = k - npcs; char b[64];
    if (k->state == NS_WALK) { float dx = walk_toward(k);
        if (dx < 115 && dx > -115 && !k->spoke) { k->spoke = 1; char *p = str_cat(b, "¿"); p = str_cat(p, k->doc); str_cat(p, "?"); SAY_PL(b, 0, 80, BC_ME); k->cue = pfr + 35; }
        if (touching(k)) { if (!k->spoke) { k->spoke = 1; k->cue = pfr + 5; } k->state = NS_ACT; k->t = 0; sfx_bump(); } }
    else if (k->state == NS_ACT && k->t == 24) {
        const char *plus = 0;
        if (!k->ok && it_favor > 0) { it_favor--; plus = "¡NEREA TE DEBE UN FAVOR!"; } else if (!k->ok && it_tickets > 0) { it_tickets--; plus = "¡TICKET DE UBER!"; }
        if (plus) { char *p = str_cat(b, plus); str_cat(p, " ¡APROBADO!"); SAY_PL(b, 0, 100, BC_ME); }
        if (k->ok || plus) { k->gear = 15; k->flash = 24; c1++; docs++; SCORE += 500; sfx_power(); SAY_NPC("¡APROBADO!", i, GY - 46, 80, BC_OK);
            pfx_t *p = fx_add(FX_DOCFLY); if (p) { p->x = k->x / 256.0f + (k->flip ? -6 : 6); p->y = GY - 26; p->life = 99; }
            if (docs == 1) SAY_PL("¡A la mochila!", 0, 60, BC_ME); leave(k, 1, 48); }
        else { c2++; SCORE += 100; sfx_hit(); SAY_NPC("¡RECHAZADO!", i, GY - 40, 80, BC_NO); leave(k, -1, 48); }
    }
}
static void npc_epis(npc_t *k) {
    if (k->state != NS_WAIT) return;
    k->flip = PLX256 < k->x;
    s32 dx = k->x - PLX256; char b[64];
    if (k->nneed && dx < 110 * 256 && dx > -20 * 256 && k->lastNeed != k->need[0]) {
        if (!k->spoke) { char *p = str_cat(b, "¡Ahí hay uno sin "); p = str_cat(p, ENAME[(int)k->need[0]]); str_cat(p, "!"); plane_say(b, BC_NO, 90); }
        k->lastNeed = k->need[0];
        char *p = str_cat(b, "¡Me falta "); p = str_cat(p, INAME[(int)k->need[0]]); str_cat(p, "!"); SAY_NPC(b, (k - npcs) | 0x100, GY - 28, 90, BC_CAND);
        k->spoke = 1;
    }
}
static void upd_npcs(void) {
    for (int i = 0; i < nnpc; i++) { npc_t *k = &npcs[i];
        if (k->state == NS_GONE) continue; k->t++; k->moving = 0;
        if (k->cue && pfr >= k->cue) { k->cue = 0; cue_for(k); }
        if (k->state == NS_HOLD) { if (k->t >= k->hold) { k->state = NS_LEAVE; k->t = 0; } }
        else if (k->state == NS_LEAVE) { k->x += k->dir * (k->dir > 0 ? 666 : 230); k->flip = k->dir < 0; k->moving = 1;
            if (k->t > 260 || (S->type == T_EPIS && k->x > (LEN - 185) * 256)) k->state = NS_GONE; }
        else if (S->type == T_HIRE) npc_hire(k);
        else if (S->type == T_DOCS) npc_docs(k);
        else npc_epis(k);
        if (k->moving) k->step++;
    }
}
static npc_t *epi_target(void) {
    for (int i = 0; i < nnpc; i++) { npc_t *k = &npcs[i]; if (k->state == NS_WAIT && k->nneed && k->x > PLX256 - 1024 && k->x - PLX256 < 85 * 256) return k; }
    return 0;
}
static void shoot(void) {
    npc_t *k = epi_target(); if (!k) return;
    for (int i = 0; i < nproj; i++) if (proj[i].k == k - npcs) return;
    int item = k->need[0]; char b[72];
    if (it_epi[item] <= 0) {
        if (k->warned == item) { it_epi[item] = 1; k->warned = -1; char *p = str_cat(b, "¡Toma, te lanzo "); p = str_cat(p, INAME[item]); str_cat(p, "!"); plane_say(b, BC_OK, 90); sfx_power(); }
        else { k->warned = item; sfx_back(); char *p = str_cat(b, "¡No te quedan "); p = str_cat(p, EPLUR[item]); str_cat(p, "! Rompe cajas o dispara otra vez"); SAY_PL(b, 0, 110, BC_NO); return; }
    }
    it_epi[item]--;
    if (nproj < 8) { proj_t *p = &proj[nproj++]; p->x = pl.x + 12; p->y0 = p->y = pl.y - 18; p->t = 0; p->k = k - npcs; p->item = item; }
    lastShot = pfr; slide(500, 900, 120);
    if (!B.epiTip) { B.epiTip = 1; plane_say("¡DISPARA EPIs!", BC_OK, 90); }
}
static void upd_proj(void) {
    int j = 0;
    for (int i = 0; i < nproj; i++) { proj_t *p = &proj[i]; p->t++; p->x += 4; p->y = p->y0 - fsin(p->t / 6.0f) * 6;
        npc_t *k = &npcs[(int)p->k];
        if ((s32)(p->x * 256) >= k->x - 1536) {
            int m = 0; for (int q = 0; q < k->nneed; q++) if (k->need[q] != p->item) k->need[m++] = k->need[q]; k->nneed = m;
            k->gear |= 1 << p->item; k->flash = 6; c1++; SCORE += 100; sfx_coin();
            if (!k->nneed) { c2++; k->state = NS_HOLD; k->hold = k->t + 50; k->dir = 1; SAY_NPC("¡Completo! ¡Gracias!", k - npcs, GY - 32, 90, BC_OK); sfx_power(); }
            continue; }
        proj[j++] = *p; }
    nproj = j;
}

/* ---------------------------------------------------------------- física */
static void follow_cam(float t) { pcam += (t - pcam) * .12f; if (pcam > LEN - VW) pcam = LEN - VW; if (pcam < 0) pcam = 0; }
static void start_boss(void);
static int jump_req, shoot_req;
static void physics(void) {

    PLX256 = (s32)(pl.x * 256);
    if (gameOverAt) { if (pfr >= gameOverAt) { gameOverAt = 0; start_screen(); } return; }
    if (jump_req && pl.on) { pl.vy = -5.6f; pl.on = 0; sfx_jump(); }
    if (shoot_req && S->type == T_EPIS) shoot();
    jump_req = shoot_req = 0;
    int dir = 0; if (key_held & K_LEFT) dir = -1; if (key_held & K_RIGHT) dir = 1;
    float mx = VEH[VEHI].max;
    if (dir) { pl.vx += dir * .25f; pl.vx = fclamp(pl.vx, -mx, mx); pl.flip = dir < 0; } else { pl.vx *= .75f; if (pl.vx < .05f && pl.vx > -.05f) pl.vx = 0; }
    PLX256 = (s32)(pl.x * 256);
    float maxX = stopX();
    if (S->type == T_EPIS) for (int i = 0; i < nnpc; i++) { npc_t *k = &npcs[i]; if (k->state == NS_WAIT && k->nneed && k->x > PLX256 - 1024 && k->x / 256.0f - 26 < maxX) maxX = k->x / 256.0f - 26; }
    pl.x = pl.x + pl.vx; if (pl.x > maxX) pl.x = maxX; if (pl.x < 8) pl.x = 8;
    if (pl.x >= maxX - .01f && pl.vx > 0) pl.vx = 0;
    float prevY = pl.y; int rh = RHh; pl.vy += .32f; if (pl.vy > 6) pl.vy = 6; pl.y += pl.vy;
    if (pl.vy < 0) for (int i = 0; i < nbox; i++) { box_t *b = &boxes[i]; float d = pl.x - b->x;
        if (d < 10 && d > -10 && prevY - rh >= b->y + 14 && pl.y - rh < b->y + 14) { pl.y = b->y + 14 + rh; pl.vy = 1; hit_box(b); break; } }
    for (int i = 0; i < nbox; i++) if (boxes[i].bump > 0) boxes[i].bump--;
    pl.on = 0;
    if (pl.vy >= 0) for (int i = 0; i < ntop; i++) { int a = tops[i][0], b = tops[i][1], y = tops[i][2];
        if (pl.x > a - 3 && pl.x < b + 3 && prevY <= y && pl.y >= y) { pl.y = y; pl.vy = 0; pl.on = 1; } }
    if (pl.y >= GY && !overHole(pl.x) && prevY <= GY + 2) { pl.y = GY; pl.vy = 0; pl.on = 1; }
    if (pl.y > VH + 20) { int hx0 = iround(pl.x); for (int i = 0; i < nhole; i++) if (pl.x > holes[i].x0 - 6 && pl.x < holes[i].x1 + 6) { hx0 = holes[i].x0; break; }
        lose_life("¡AL AGUJERO!"); pl.x = hx0 - 22; pl.y = GY; pl.vx = pl.vy = 0; pcam = pl.x - 100; if (pcam < 0) pcam = 0; }
    if (inv > 0) inv--;
    for (int i = 0; i < nobst; i++) { float d = pl.x - obst[i].x; if (d < 9 && d > -9 && pl.y > GY - 10 && inv <= 0) {
        lose_life("¡CUIDADO!"); float sg = obst[i].x - pl.x; pl.vx = -2.5f * (sg > 0 ? 1 : sg < 0 ? -1 : 1); pl.vy = -3; pl.on = 0; } }
    if (pl.vx > .2f || pl.vx < -.2f) pl.step++;
    int ipx = iround(pl.x), ipy = iround(pl.y);
    for (int i = 0; i < ncoin; i++) { coin_t *c = &coinsA[i]; int d = c->x - ipx;
        if (!c->got && d < 9 && d > -9 && c->y > ipy - rh - 2 && c->y < ipy + 2) { c->got = 1; COINS++; SCORE += 100; sfx_coin(); } }
    for (int i = 0; i < nloose; i++) { loose_t *c = &loose[i]; int d = c->x - ipx;
        if (!c->got && d < 10 && d > -10 && c->y > ipy - rh - 4 && c->y < ipy + 4) { c->got = 1; give(c->item, 0, 0, 0);
            char b[24]; char *p = str_cat(b, "+1 "); str_cat(p, EUP[c->item]); say(b, AN_FIX, 0, c->x, c->y - 10, 60, BC_ME); } }
   
    PLX256 = (s32)(pl.x * 256);
    upd_npcs();
   
    if (S->type == T_EPIS) upd_proj();
    follow_cam(pl.x - 100);
    if (!B.on && pl.x >= stopX() - 1) start_boss();
}
static void move_dog(void) {
    if (!CREW[CI].dog) return;
    float tgt = pl.x + (pl.flip ? 44 : -44), ox = dog.x; dog.x += (tgt - dog.x) * .08f; float dv = dog.x - ox;
    if (dv > .15f || dv < -.15f) { dog.flip = dv < 0; dog.step++; }
}

/* ---------------------------------------------------------------- jefes */
static void box_world(float *x, float *y) {
    const rider_t *R = compose_rider(CI, VEHI, 0); int d = R->ax - R->bxc;
    *x = pl.flip ? pl.x + d : pl.x - d; *y = pl.y - R->H + 1 + R->byt;
}
static void start_boss(void) {
    memset(&B, 0, sizeof B); B.on = 1; B.epiTip = 1;
    if (S->type == T_EPIS) { int x0 = LEN - 230; for (int i = 0; i < 3; i++) { B.rec[i].x = x0 + 80 + i * 27; B.rec[i].look = (i + 1) % 6; B.rec[i].gear = 0; } }
    pl.vx = 0; pl.flip = 0;
}
static int boss_step(void) {
    int t = ++B.t, bx = bxW(); char b[64];
    pl.vy += .32f; pl.y = pl.y + pl.vy; if (pl.y > GY) pl.y = GY; if (pl.y >= GY) pl.vy = 0;
    PLX256 = (s32)(pl.x * 256);
    upd_npcs(); follow_cam(S->type == T_EPIS ? LEN - VW : pl.x - 100);
    if (S->type == T_HIRE) {
        if (t == 1) { say("¡¡TU FORECAST NO CUADRA!!", AN_FIX, 0, bx, GY - 62, 80, BC_BAD); sfx_hit(); }
        if (t == 50) SAY_PL("¡Mira mis contrataciones!", 0, 60, BC_ME);
        if (t == 80) { B.good = 1; poof(bx, GY - 30, 26); sfx_power(); }
        if (t == 95) { int left = 0; for (int i = 0; i < nnpc; i++) if (npcs[i].state == NS_WALK || npcs[i].missed) left++;
            say(left ? "¡Forecast casi perfecto!" : "¡¡FORECAST PERFECTO!!", AN_FIX, 0, bx, GY - 34, 150, BC_OK); sfx_win(); }
        if (t == 125) { B.hascoin = 1; B.cx = bx; B.cy = GY - 50; B.ct = 0; }
    } else if (S->type == T_DOCS) {
        if (t == 1) { char *p = str_cat(b, "¡¡SUPER ENVÍO DE "); p = str_num(p, imax(docs, 1)); str_cat(p, " DOCUMENTOS!!"); SAY_PL(b, 0, 130, BC_BAD); }
        B.charge = t > 15 && t < 60;
        if (t == 60) { slide(180, 1000, 900); bagDocs = 0; }
        if (t >= 60 && t < 140 && t % 3 == 0) { float wx, wy; box_world(&wx, &wy); pfx_t *p = fx_add(FX_DOC);
            if (p) { p->x = wx; p->y = wy + (rndf() * 6 - 3); p->vx = 3.5f + rndf() * 2; p->vy = (rndf() - .5f) * 1.2f; p->life = 70; } }
        if (t == 140) { B.ko = 1; sfx_hit(); }
        if (t == 175) { B.stamped = 1; tone(70, 350, 0, W_SQUARE, 140); }
        if (t == 200) { B.good = 1; for (int i = 0; i < 3; i++) poof(bx - 40 + i * 38, GY - 34, 14); sfx_power(); say("¡Gracias!", AN_FIX, 0, bx - 2, GY - 40, 80, BC_OK); }
        if (t == 235) { B.hascoin = 1; B.cx = bx; B.cy = GY - 50; B.ct = 0; }
    } else {
        int x0 = bx - 120, MX = x0 + 158, FX = x0 + 186;
        if (t == 1) { say("¡¡AQUÍ NO SE ENTREGA NADA!!", AN_FIX, 0, MX + 10, GY - 62, 90, BC_BAD); sfx_hit(); }
        if (t == 45) SAY_PL("¡Azlan, a por ellos!", 0, 60, BC_ME);
        if (t >= 55 && t < 91 && t % 6 == 0) { static const u8 IT6[6] = {0, 1, 2, 3, 0, 3}; pfx_t *p = fx_add(FX_EPI);
            if (p) { p->item = IT6[((t - 60) / 6 + 6) % 6]; p->x = pl.x + 12; p->y = pl.y - 20; p->tx = (t / 6) % 2 ? MX : FX; p->ty = GY - 40; p->life = 60; p->rec = -1; }
            slide(500, 900, 120); }
        if (t == 105) { B.good = 1; poof(MX, GY - 30, 26); poof(FX, GY - 26, 26); sfx_power(); }
        if (t == 120) say("¡Bienvenido al almacén!", AN_FIX, 0, FX, GY - 30, 80, BC_OK);
        if (t >= 140 && !B.del_on) { B.del_on = 1; B.di = 0; B.dt = 0; }
        if (B.del_on && B.di < 6) { B.dt++;
            static const u8 DI[6] = {0, 0, 0, 1, 3, 3}, DR[6] = {0, 1, 2, 0, 1, 2};
            if (B.dt == 1) { B.dwho = B.di % 2 ? FX : MX; B.ditem = DI[B.di]; B.dr = DR[B.di]; }
            if (B.dt == 8) tone(1800, 60, 0, W_SQUARE, 50);
            if (B.dt == 14) { pfx_t *p = fx_add(FX_EPI); if (p) { p->item = B.ditem; p->x = B.dwho - 6; p->y = GY - 18; p->tx = B.rec[B.dr].x; p->ty = GY - 18; p->life = 40; p->rec = B.dr; } }
            if (B.dt >= 22) { B.di++; B.dt = 0; } }
        if (t == 290) { say("¡EPIs 100% entregados en Catalunya!", AN_FIX, 0, x0 + 170, GY - 34, 120, BC_BAD); sfx_win(); B.cheer = 1; }
        if (t >= 290 && t < 420 && t % 2 == 0) {
            u8 cc[5] = {C(0xe4002b), C(0xffd23f), C(0x62d26f), C(0x58c4ff), C(0xffffff)};
            for (int i = 0; i < 3; i++) { pfx_t *p = fx_add(FX_CONF); if (p) { p->x = x0 + rndf() * 250; p->y = 24; p->vx = (rndf() - .5f) * .8f; p->vy = .6f + rndf() * .8f; p->c = cc[rndi(5)]; p->life = 150; } } }
        if (t == 320) { B.hascoin = 1; B.cx = MX; B.cy = GY - 44; B.ct = 0; }
    }
    if (B.hascoin) { B.ct++;
        if (B.ct < 40) B.cy -= .6f; else { B.cx += (pl.x - B.cx) * .09f; B.cy += ((pl.y - RHh - 16) - B.cy) * .09f; }
        if (B.ct == 95) { sfx_coin(); sfx_power(); B.hascoin = 0; B.got = 1; SAY_PL("¡MONEDA DE ORO!", 0, 90, BC_OK); B.end = t + 80; } }
    if (B.end && t == B.end) return 1;
    return 0;
}

/* ---------------------------------------------------------------- fx */
static void fx_step(void) {
    int j = 0;
    for (int i = 0; i < nfx; i++) { pfx_t *p = &fxs[i]; int keep = 0; p->life--;
        switch (p->type) {
        case FX_DOC: p->x += p->vx; p->y += p->vy; keep = p->life > 0 && p->x < LEN; break;
        case FX_DOCFLY: { p->t++; int hand = p->t < 34; float tx, ty; if (hand) { tx = pl.x + (pl.flip ? -3 : 3); ty = pl.y - 30; } else { box_world(&tx, &ty); ty -= 2; }
            float k = hand ? .22f : .25f; p->x += (tx - p->x) * k; p->y += (ty - p->y) * k;
            if (p->t == 34) tone(520, 50, 0, W_SQUARE, 40);
            if (p->t >= 48) { bagDocs++; tone(300, 80, 0, W_SQUARE, 60); keep = 0; } else keep = 1; } break;
        case FX_POOF: p->x += p->vx; p->y += p->vy; p->vx *= .93f; p->vy *= .93f; keep = p->life > 0; break;
        case FX_EPI: p->x += (p->tx - p->x) * .16f; p->y += (p->ty - p->y) * .16f - (p->life > 30 ? 1 : 0);
            if (p->tx - p->x < 3 && p->tx - p->x > -3) { if (p->rec >= 0) { B.rec[(int)p->rec].gear |= 1 << p->item; sfx_coin(); } else tone(200, 60, 0, W_SQUARE, 70); keep = 0; }
            else keep = p->life > 0; break;
        case FX_CONF: p->x += p->vx + fsin((pfr + p->y) / 9.0f) * .3f; p->y += p->vy; keep = p->life > 0 && p->y < GY; break;
        case FX_ITEM: p->y -= .7f; keep = p->life > 0; break;
        }
        if (keep) fxs[j++] = *p;
    }
    nfx = j;
}

/* ---------------------------------------------------------------- dibujo */
static void draw_pickup(int it, int x, int y) {
#define R_(a, b, w, h, c) rect(x + (a), y + (b), w, h, c)
    if (it == IT_MONSTER) { R_(-3, -6, 7, 12, KK); R_(-2, -5, 5, 10, C(0x1b1b1b)); R_(-2, -5, 5, 1, C(0x9aa3b5)); R_(-1, -3, 1, 6, C(0x62ff6f)); R_(1, -3, 1, 6, C(0x62ff6f)); R_(0, -2, 1, 4, C(0x62ff6f)); }
    else if (it == IT_CONTRATO) { R_(-4, -5, 8, 10, KK); R_(-3, -4, 6, 8, C(0xffffff)); R_(-2, -3, 4, 1, C(0x9aa3b5)); R_(-2, -1, 4, 1, C(0x9aa3b5)); R_(0, 1, 3, 3, C(0xd62839)); }
    else if (it == IT_TICKET) { R_(-5, -3, 11, 7, KK); R_(-4, -2, 9, 5, C(0xffffff)); R_(-4, -2, 2, 5, C(0x1b1b1b)); R_(0, -1, 1, 3, KK); R_(2, -1, 1, 3, KK); R_(0, 1, 3, 1, KK); }
    else if (it == IT_FAVOR) { R_(-4, -4, 9, 9, KK); R_(-3, -3, 7, 7, C(0xffd23f)); R_(-2, -2, 1, 5, KK); R_(-1, -1, 1, 1, KK); R_(0, 0, 1, 1, KK); R_(1, -2, 1, 5, KK); }
    else draw_item(it, x - 3, y - 3, 1);
#undef R_
}
static void draw_box(box_t *b, int x) {
    if (x < -20 || x > VW + 20) return;
    int y = b->y - (b->bump > 5 ? 3 : b->bump > 0 ? 1 : 0);
    rect(x - 8, y, 16, 14, KK);
    if (b->used) { rect(x - 7, y + 1, 14, 12, C(0x8a5a35)); rect(x - 7, y + 11, 14, 2, C(0x6e4527)); return; }
    rect(x - 7, y + 1, 14, 12, C(0xffb000)); rect(x - 7, y + 1, 14, 1, C(0xffe08a)); rect(x - 7, y + 1, 1, 12, C(0xffe08a)); rect(x - 7, y + 12, 14, 1, C(0xc97a00));
    u8 d = C(0x7a4a00); pset(x - 6, y + 2, d); pset(x + 5, y + 2, d); pset(x - 6, y + 11, d); pset(x + 5, y + 11, d);
    u8 q = (pfr >> 4) & 1 ? C(0xd62839) : C(0xa51d2b);
    rect(x - 2, y + 3, 4, 1, q); rect(x + 1, y + 4, 2, 2, q); rect(x - 1, y + 6, 2, 1, q); rect(x - 1, y + 7, 2, 2, q); rect(x - 1, y + 10, 2, 2, q);
}
static void draw_obstacle(obst_t *o, int x) {
    if (x < -20 || x > VW + 20) return; int y = GY;
    if (o->type == 0) { rect(x - 6, y - 2, 12, 2, KK);
        for (int k = 0; k < 11; k++) { int h = (5 * (2 * k + 1) + 11) / 22; rect(x - h, y - 13 + k, 2 * h, 1, KK); }   /* = poly del cono */
        for (int k = 0; k < 10; k++) { int h = (4 * (2 * k + 1) + 10) / 20; rect(x - h, y - 12 + k, 2 * h, 1, C(0xff7a1a)); } rect(x - 2, y - 8, 4, 2, C(0xffffff)); }
    else if (o->type == 1) { rect(x - 6, y - 12, 12, 12, KK); rect(x - 5, y - 11, 10, 10, C(0x2f7d3a)); rect(x - 6, y - 13, 12, 2, C(0x1f5a28)); rect(x - 3, y - 9, 1, 7, C(0x62b85c)); rect(x + 1, y - 9, 1, 7, C(0x62b85c)); }
    else { rect(x - 8, y - 11, 16, 6, KK); rect(x - 7, y - 5, 2, 5, KK); rect(x + 5, y - 5, 2, 5, KK); for (int i = 0; i < 4; i++) rect(x - 7 + i * 4, y - 10, 4, 4, i % 2 ? C(0xffffff) : C(0xd62839)); }
}
/* drawEvil / drawWorker(evil) pre-dibujados (2 colores de aura) */
static bmp_t *EV[4][2]; /* 0 Martí (hire) / Martí B (epis), 1 Franco, y trabajadores 1..3 en EVW */
static bmp_t *EVW[3][2];
static bmp_t *evil_bmp(const char *const *spr, int n, const u8 *pal, int flip, int sc, int ai) {
    int w = strlen(spr[0]); bmp_t *b = bmp_new(w * sc + 4, n * sc + 4); if (!b) return 0;
    tgt_bmp(b); origin(2, 2);
    u8 aura = ai ? C(0x9b30ff) : C(0x5a1a8a);
    for (int r = 0; r < n; r++) for (int q = 0; q < w; q++) if (spr[r][q] != '.') rect((flip ? w - 1 - q : q) * sc - 2, r * sc - 2, sc + 4, sc + 4, aura);
    u8 p[128]; memcpy(p, pal, 128); for (int i = 0; i < 128; i++) if (P_EVIL[i]) p[i] = P_EVIL[i];
    draw_pal_s(spr, n, 0, 0, p, flip, sc);
    for (int r = 0; r < n; r++) for (int q = 0; q < w; q++) if (spr[r][q] == 'E') {
        int qq = flip ? w - 1 - q : q, left = qq < w / 2;
        for (int k = 0; k < 3; k++) rect((qq - 1 + k) * sc, (r - 1) * sc + (left ? k : 2 - k) * sc / 3 - sc / 3, sc, imax(1, sc / 2), C(0x120b0c)); }
    origin(0, 0); tgt_back(); return b;
}
static bmp_t *worker_bmp(int li, int ai) {
    bmp_t *b = bmp_new(16 * 2 + 4, 15 * 2 + 4); if (!b) return 0;
    tgt_bmp(b); origin(2, 2);
    const look_t *lk = &LOOKS[li % 6]; const int sc = 2;
    u8 pal[128]; memset(pal, 0, 128);
    pal['K'] = C(0x1b1212); pal['H'] = lk->H; pal['S'] = lk->S; pal['s'] = lk->s; pal['B'] = lk->H; pal['C'] = C(0x3b1f4f); pal['c'] = C(0x2a1538); pal['P'] = lk->P; pal['G'] = C(0x3a2a1e);
    char rws[15][17]; const char *rp[15];
    for (int r = 0; r < 15; r++) { for (int q = 0; q < 16; q++) { char ch = CAND_T[r][q]; rws[r][q] = ch == 'b' ? (lk->beard ? 'B' : 'S') : ch; } rws[r][16] = 0; rp[r] = rws[r]; }
    u8 aura = ai ? C(0x9b30ff) : C(0x5a1a8a);
    for (int r = 0; r < 15; r++) for (int q = 0; q < 16; q++) if (rp[r][q] != '.') rect(q * sc - 2, r * sc - 2, sc + 4, sc + 4, aura);
    draw_pal_s(rp, 15, 0, 0, pal, 0, sc);
    rect(6 * sc, 5 * sc, sc, sc, C(0xff2a2a)); rect(9 * sc, 5 * sc, sc, sc, C(0xff2a2a));
    rect(5 * sc, 4 * sc, 2 * sc, 1, C(0x120b0c)); rect(9 * sc, 4 * sc, 2 * sc, 1, C(0x120b0c));
    origin(0, 0); tgt_back(); return b;
}
static void build_evil(void) {
    memset(EV, 0, sizeof EV); memset(EVW, 0, sizeof EVW);
    for (int a = 0; a < 2; a++) {
        if (S->type == T_HIRE) EV[0][a] = evil_bmp(MARTI, 28, P_MPAL_I, 1, 2, a);
        else if (S->type == T_EPIS) { EV[0][a] = evil_bmp(MARTI_B, 28, P_MPAL_A, 1, 2, a); EV[1][a] = evil_bmp(FRANCO, 24, P_FPAL, 1, 2, a); }
        else for (int i = 0; i < 3; i++) EVW[i][a] = worker_bmp((i + 2) % 6, a);
    }
}
static void evil_at(bmp_t *b, const char *const *spr, int n, int x, int y, const u8 *pal) {
    if (b) blit_t(b, x - 2, y - 2, 0); else draw_evil(spr, n, x, y, pal, 1, 2);
}
static void draw_marti(int bx, int c) {
    int good = B.good, ex = bx + (good ? 14 : 24) - c;
    rect(ex, GY - 42, 26, 20, KK); rect(ex + 1, GY - 41, 24, 18, C(0xffffff));
    static const u8 HG[6] = {3, 5, 4, 8, 10, 13}, HB[6] = {13, 11, 9, 6, 4, 2};
    for (int i = 0; i < 6; i++) { int h = good ? HG[i] : HB[i]; rect(ex + 3 + i * 4, GY - 24 - h, 3, h, good ? (i == 5 ? C(0x62d26f) : C(0x2f8f3a)) : C(0xe4002b)); }
    rect(ex + 4, GY - 22, 2, 22, KK); rect(ex + 20, GY - 22, 2, 22, KK);
    if (good) draw_pal(MARTI, 28, bx - 9 - c, GY - 28, P_MPAL_I, 1);
    else evil_at(EV[0][(pfr >> 2) & 1], MARTI, 28, bx - 18 - c, GY - 56, P_MPAL_I);
}
static void draw_office(int bx, int c) {
    for (int i = 0; i < 3; i++) { int dx = bx - 40 + i * 38 - c;
        if (B.good) draw_worker((i + 2) % 6, dx - 14, GY - 35, 0, 1);
        else { bmp_t *wb = EVW[i][(pfr >> 2) & 1]; if (wb) blit_t(wb, dx - 24, GY - 49 + (B.ko ? 4 : 0), 0); else draw_worker((i + 2) % 6, dx - 22, GY - 47 + (B.ko ? 4 : 0), 1, 2); }
        if (B.ko && !B.good) for (int j = 0; j < 2; j++) { float a = pfr / 8.0f + j * 3.1f; rect(iround(dx - 6 + fcos(a) * 9), iround(GY - 50 + fsin(a) * 2), 3, 3, C(0xffd23f)); }
        rect(dx + 2, GY - 31, 15, 12, KK); rect(dx + 3, GY - 30, 13, 9, B.stamped ? C(0x62d26f) : (((pfr >> 4) + i) & 1 ? C(0xe4002b) : C(0x8c0a1e)));
        u8 W = C(0xffffff);
        if (!B.stamped) { rect(dx + 5, GY - 28, 9, 1, W); rect(dx + 5, GY - 25, 6, 1, W); } else { rect(dx + 6, GY - 26, 2, 2, W); rect(dx + 8, GY - 25, 2, 2, W); rect(dx + 10, GY - 28, 2, 3, W); }
        rect(dx + 8, GY - 19, 3, 3, KK); rect(dx - 16, GY - 17, 36, 5, KK); rect(dx - 15, GY - 16, 34, 3, C(0x8a5a35));
        rect(dx - 14, GY - 12, 3, 12, KK); rect(dx + 15, GY - 12, 3, 12, KK);
        rect(dx - 12, GY - 19, 8, 2, C(0xf2f2f2)); rect(dx - 11, GY - 21, 7, 2, C(0xf2f2f2));
    }
}
static void build_warehouse(void) {
    wh_bmp = bmp_new(252, GY - 20); if (!wh_bmp) return;
    tgt_bmp(wh_bmp);
    rect(0, 0, 252, GY - 20, KK); rect(2, 2, 248, GY - 22, C(0x8a94aa));
    for (int x = 4; x < 250; x += 6) rect(x, 2, 2, GY - 22, C(0x7a8399));
    rect(2, 2, 248, 4, C(0xd0102c));
    for (int u = 0; u < 4; u++) { int sx = 16 + u * 58;
        rect(sx, 14, 3, GY - 34, C(0x3d4658)); rect(sx + 49, 14, 3, GY - 34, C(0x3d4658));
        for (int l = 0; l < 4; l++) { int sy = 26 + l * 20; rect(sx, sy, 52, 3, C(0xe09a2f)); rect(sx, sy + 3, 52, 1, KK);
            for (int k = 0; k < 5; k++) { int kind = (u * 7 + l * 3 + k) % 6, ix = sx + 4 + k * 10, iy = sy - 8;
                if (kind < 4) draw_item(kind, ix, iy + (kind == 0 ? 2 : 1), 1);
                else { rect(ix, iy, 9, 8, KK); rect(ix + 1, iy + 1, 7, 6, C(0xc9a46a)); rect(ix + 1, iy + 3, 7, 1, C(0xa88550)); } } } }
    tgt_back();
}
static void draw_warehouse(int bx, int c) {
    int x0 = bx - 120 - c;
    if (x0 >= VW) return;
    if (wh_bmp) blit_opq(wh_bmp, 0, 0, 252, GY - 20, x0, 20, 0);
    if (B.on) for (int i = 0; i < 3; i++) pers_draw(B.rec[i].look, iround(B.rec[i].x - 8 - c), GY - 24, 0, 0, B.rec[i].gear, 24);
    int MX = x0 + 158, FX = x0 + 186;
    if (B.good) { int bob = B.cheer ? ((pfr >> 4) & 1) : 0;
        draw_pal(MARTI_B, 28, MX - 9, GY - 28 - bob, P_MPAL_A, 1); draw_pal(FRANCO, 24, FX - 8, GY - 24 - (B.cheer ? 1 - bob : 0), P_FPAL, 1);
        if (B.del_on && B.di < 6 && B.dt >= 1 && B.dt < 14) { int hx = (iround(B.dwho) == bx - 120 + 158 ? MX : FX) - 12;
            draw_item(B.ditem, hx - 4, GY - 20, 1); rect(hx + 6, GY - 19, 3, 5, KK); rect(hx + 7, GY - 18, 1, 2, C(0x58c4ff));
            if (B.dt >= 6 && B.dt < 12) rect(hx - 3, GY - 17, 9, 1, C(0xff2a2a)); } }
    else { evil_at(EV[0][(pfr >> 2) & 1], MARTI_B, 28, MX - 18, GY - 56, P_MPAL_A); evil_at(EV[1][(pfr >> 2) & 1], FRANCO, 24, FX - 12, GY - 48, P_FPAL); }
}
static void draw_wave(npc_t *k, int x) {
    int y = GY - 24, f = k->flip, w = ((pfr >> 3) + k->home) & 1, sh = f ? 3 : 12, hx = f ? 0 : 15;
    const look_t *lk = &LOOKS[k->look];
    line(x + sh, y + 11, x + hx, y + 5 - w, KK, 3); line(x + sh, y + 11, x + hx, y + 5 - w, lk->C, 1);
    int px = x + hx + (f ? -5 : 0) + (w ? 1 : -1), py = y - 3 - w;
    rect(px, py, 7, 9, KK); rect(px + 1, py + 1, 5, 7, C(0xffffff)); rect(px + 2, py + 3, 3, 1, C(0x9aa3b5)); rect(px + 2, py + 5, 3, 1, C(0x9aa3b5));
    rect(x + hx - 1, y + 5 - w, 2, 2, lk->S);
}
static void draw_status_raw(void) {
    for (int i = 0; i < 3; i++) { int x = 4 + i * 10, y = 4;
        rect(x, y + 1, 9, 5, KK); rect(x + 1, y, 3, 7, KK); rect(x + 5, y, 3, 7, KK); rect(x + 2, y + 6, 5, 2, KK); rect(x + 3, y + 8, 3, 1, KK);
        u8 c = i < life ? C(0xff3a4e) : C(0x5a4a4a);
        rect(x + 1, y + 1, 3, 4, c); rect(x + 5, y + 1, 3, 4, c); rect(x + 2, y + 5, 5, 2, c); rect(x + 3, y + 7, 3, 1, c);
        if (i < life) pset(x + 2, y + 2, C(0xffb3bd)); }
    int its[4], ns[4], n;
    if (S->type == T_HIRE) { its[0] = IT_CONTRATO; ns[0] = it_contracts; n = 1; }
    else if (S->type == T_DOCS) { its[0] = IT_FAVOR; ns[0] = it_favor; its[1] = IT_TICKET; ns[1] = it_tickets; n = 2; }
    else { for (int i = 0; i < 4; i++) { its[i] = i; ns[i] = it_epi[i]; } n = 4; }
    int x = 40; char b[8];
    for (int i = 0; i < n; i++) { draw_pickup(its[i], x + 4, 9); str_num(b, ns[i]); text(x + 10, 6, b, KK, 0); text(x + 9, 5, b, C(0xffffff), 0); x += ns[i] > 9 ? 28 : 21; }
}
static bmp_t *STB; static rle_t STR; static s32 st_key[6];
static void draw_status(void) {
    s32 key[6] = {life, it_contracts, it_favor * 100 + it_tickets, it_epi[0] * 100 + it_epi[1], it_epi[2] * 100 + it_epi[3], S->type};
    if (!STB) { STB = bmp_new(150, 18); memset(&STR, 0, sizeof STR); st_key[0] = -1; }
    if (!STB) { draw_status_raw(); return; }
    if (memcmp(key, st_key, sizeof key)) { memcpy(st_key, key, sizeof key); memset(STB->px, 0, 150 * 18); tgt_bmp(STB); draw_status_raw(); tgt_back(); rle_build(&STR, STB, 18 * 24); }
    rle_draw(&STR, 0, 0);
}
static void draw_play(void) {
    int c = iround(pcam), bx = bxW();

    bg_draw();
    for (int i = 0; i < nhole; i++) { int x = holes[i].x0 - c, w = holes[i].x1 - holes[i].x0; if (x > VW || x + w < 0) continue;
        rect(x, GY, w, 8, C(0x241610)); rect(x, GY + 8, w, 8, C(0x180e0b)); rect(x, GY + 16, w, 8, C(0x0b0706));
        rect(x - 1, GY, 2, 4, C(0x3e7a35)); rect(x + w - 1, GY, 2, 4, C(0x3e7a35)); rect(x, GY + 4, 2, VH - GY, C(0x6e4527)); rect(x + w - 2, GY + 4, 2, VH - GY, C(0x6e4527)); }
    if (S->type == T_EPIS) draw_warehouse(bx, c);

    for (int i = 0; i < nobst; i++) draw_obstacle(&obst[i], obst[i].x - c);
    for (int i = 0; i < nplat; i++) for (int j = 0; j < plats[i][2]; j++) { int px = plats[i][0] + j * 16 - c, y = plats[i][1]; if (px < -16 || px > VW) continue;
        rect(px, y, 16, 8, KK); rect(px + 1, y + 1, 14, 6, C(0xd0703f)); rect(px + 1, y + 1, 14, 1, C(0xe89363)); rect(px + 7, y + 1, 1, 6, C(0x9c4d2a)); }
    for (int i = 0; i < ncoin; i++) { coin_t *k = &coinsA[i]; if (k->got) continue; int x = k->x - c; if (x < -8 || x > VW + 8) continue;
        static const u8 WS[4] = {6, 4, 2, 4}; int w = WS[((pfr >> 3) + k->x) % 4];
        rect(x - w / 2 - 1, k->y - 5, w + 2, 10, KK); rect(x - w / 2, k->y - 4, w, 8, C(0xffd23f)); if (w > 2) rect(x + w / 2 - 1, k->y - 3, 1, 6, C(0xc9971a)); }
    for (int i = 0; i < nbox; i++) draw_box(&boxes[i], boxes[i].x - c);
    for (int i = 0; i < nloose; i++) if (!loose[i].got) { int x = loose[i].x - c; if (x < -10 || x > VW + 10) continue;
        draw_item(loose[i].item, x - 3, iround(loose[i].y - 3 + fsin((pfr + loose[i].x) / 10.0f) * 2), 1); }
   
    if (bx - c < VW + 60) { if (S->type == T_HIRE) draw_marti(bx, c); if (S->type == T_DOCS) draw_office(bx, c); }
   
    for (int i = 0; i < nnpc; i++) { npc_t *k = &npcs[i]; if (k->state == NS_GONE) continue; int x = ((k->x + 128) >> 8) - 8 - c; if (x < -40 || x > VW + 40) continue;
        if (k->flash > 0) { k->flash--; if ((k->flash >> 1) & 1) continue; }

        pers_draw(k->look, x, GY - 24 - (k->state == NS_WAIT && ((pfr >> 4) + k->home) % 2 ? 1 : 0), k->flip, k->moving ? (((k->step >> 3) & 1) ? 1 : -1) : 0, k->gear, 24);

        if (S->type == T_DOCS && (k->state == NS_WALK || (k->state == NS_ACT && k->t < 24))) draw_wave(k, x);
        }
   
    for (int i = 0; i < nproj; i++) draw_item(proj[i].item, iround(proj[i].x - 3 - c), iround(proj[i].y - 3), 1);
    for (int i = 0; i < nfx; i++) { pfx_t *p = &fxs[i]; int x = iround(p->x - c), y = iround(p->y);
        switch (p->type) {
        case FX_DOC: rect(x - 1, y - 1, 7, 8, KK); rect(x, y, 5, 6, C(0xffffff)); rect(x + 1, y + 2, 3, 1, C(0x9aa3b5)); break;
        case FX_DOCFLY: rect(x - 3, y - 4, 7, 8, KK); rect(x - 2, y - 3, 5, 6, C(0xffffff)); rect(x - 1, y - 1, 3, 1, C(0x9aa3b5)); break;
        case FX_POOF: case FX_CONF: rect(x, y, 2, 2, p->c); break;
        case FX_EPI: draw_item(p->item, x - 3, y - 3, 1); break;
        case FX_ITEM: draw_pickup(p->item, x, y); break;
        } }
    if (CREW[CI].dog) { static bmp_t *DB[4]; static u32 dmark = 0xffffffff; if (dmark != mark_g) { for (int f = 0; f < 4; f++) { DB[f] = rows_bmp(f & 1 ? DOG1 : DOG0, 18, P_DOG, f >> 1); memset(&DR[f], 0, sizeof DR[f]); rle_build(&DR[f], DB[f], 18 * 8); } dmark = mark_g; }
        int df = ((dog.step >> 3) & 1) | (dog.flip ? 2 : 0); if (DR[df].b) rle_draw(&DR[df], iround(dog.x - 14 - c), GY - 18); else draw_dog(df & 1, iround(dog.x - 14 - c), GY - 18, dog.flip); }
    if (B.charge) { int r = 6 + ((pfr >> 1) % 4) * 2; rect(iround(pl.x + 10 - c - r / 2), iround(pl.y - 22 - r / 2), r, r, (pfr >> 1) & 1 ? C(0xffffff) : C(0xffd23f)); }
    const rider_t *R = compose_rider(CI, VEHI, (pl.step >> 2) & 3);
    int ry = iround(pl.y - R->H + 1 + ((VEHI == 0 && pl.on && (pl.vx > .3f || pl.vx < -.3f) && (pfr >> 1) & 1) ? 1 : 0));
    if (!(inv > 0 && (pfr >> 2) & 1)) { if (pl.flip) blit_t(R->b, iround(pl.x + R->ax - c) - R->W, ry, 1); else if (RR[(pl.step >> 2) & 3].b) rle_draw(&RR[(pl.step >> 2) & 3], iround(pl.x - R->ax - c), ry); else blit_t(R->b, iround(pl.x - R->ax - c), ry, 0); }
    if (S->type == T_DOCS) { float wx, wy; box_world(&wx, &wy); int n = imin(bagDocs, 6);
        for (int i = 0; i < n; i++) { int x = iround(wx - c) - 8 + i * 3, y = iround(wy) - 4 - (i % 2); rect(x, y, 4, 5, KK); rect(x + 1, y + 1, 2, 4, C(0xffffff)); }
        if (B.charge && ((pfr >> 1) & 1)) rect_dither(iround(wx - c) - 11, iround(wy) - 1, 22, 13, C(0xffd23f)); }
   
    if (B.hascoin) big_coin(iround(B.cx - c), iround(B.cy), pfr);
    draw_plane_at(plane.sx, plane.sy, 1, 0);
   
    /* rótulos del DOM: ciudades, jefe, sello */
    for (int i = 0; i < S->np; i++) { int sx = PLX[i] - c; if (sx > -40 && sx < VW + 40) text_sh(sx, GY + 11, S->pn[i], C(0xffffff), C(0x120b0c), 1); }
    int bsx = bx - c;
    if (bsx < VW + 30 && !B.on) { int w = imax(text_len(S->boss), 5) * 4 + 10, yb = S->type == T_EPIS ? 20 : 52, x = bsx - w / 2;
        rect(x, yb - 17, w, 17, C(0xffffff)); rect(x + 1, yb - 16, w - 2, 15, C(0x120b0c));
        stext(bsx, yb - 14, "FINAL", C(0xffd23f), 1); stext(bsx, yb - 7, S->boss, C(0xffffff), 1); }
    if (B.stamped) { int w = 12 * 8 + 12, x = bsx - w / 2, y = 66 - 9;
        rect(x, y, w, 18, C(0x1f9e4a)); rect(x + 2, y + 2, w - 4, 14, C(0xffffff)); text(bsx, y + 5, "ALL APPROVED", C(0x1f9e4a), 1); }

    draw_status();
    draw_hud_top(1, c1, c2, S->c1, S->c2);
    draw_bubbles();
   
}

/* ---------------------------------------------------------------- bucle */
static void plane_step(void) {
    float tx = B.on ? 40 : fclamp((pl.x - pcam) + 70, 46, VW - 52);
    plane.sx += (tx - plane.sx) * .04f; plane.sy = (B.on ? 52 : 58) + fsin(pfr / 25.0f) * 3; /* más bajo que en el web: sus bocadillos no tapan el marcador */
}
static int step(void) {
    pfr++; st_t++;
    if (st_t == 30) plane_say(S->hint, BC_OK, 140);
    if (st_t == 204) plane_say(S->type == T_HIRE ? "¡Rompe las cajas: sin CONTRATOS no contratas!" : S->type == T_DOCS ? "¡Busca tickets de Uber y el favor de Nerea!" : "¡Coge EPIs de las cajas y dispara con B!", BC_OK, 150);
    int done = 0;
    if (S->type == T_DOCS && !B.ko && bxW() - pcam < VW + 30 && pfr % 55 == 0) { int i = (pfr / 55) % 3; say("¡RECHAZADO!", AN_FIX, 0, bxW() - 40 + i * 38 - 6, GY - 52, 45, BC_NO); }

    if (B.on) done = boss_step(); else physics();

    move_dog(); fx_step(); plane_step();
    return done;
}
static void dbg_update(void) {
    s32 *d = plat_dbgv; d[0] = iround(pl.x); d[1] = iround(pl.y); d[2] = pl.on; d[3] = life; d[4] = B.on; d[8] = pfr; d[9] = plat_maxn; d[10] = c1; d[11] = c2; d[12] = gameOverAt; d[13] = SCORE;
    int j = 0;
    if (pl.on && !B.on) { for (int i = 0; i < nbox; i++) { float dx = boxes[i].x - pl.x; if (!boxes[i].used && dx > -2 && dx < 10) j = 1; }
        for (int i = 0; i < nhole; i++) { float dx = holes[i].x0 - pl.x; if (dx > 2 && dx < 18) j = 2; }
        for (int i = 0; i < nobst; i++) { float dx = obst[i].x - pl.x; if (dx > 8 && dx < 22) j = 3; } }
    d[6] = j; d[7] = S->type == T_EPIS && !B.on && epi_target() && pfr - lastShot > 22;
    d[14] = iround(pcam); d[15] = B.t;
    { npc_t *k = epi_target(); d[5] = k ? (k - npcs) * 1000 + k->need[0] * 100 + it_epi[(int)k->need[0]] * 10 + (k->warned + 1) : -1; }
}

int platform_play(int ci, int veh) {
    if (ci == 1) return raul_play();
    memset(plat_dbgv, 0, sizeof plat_dbgv);
    u32 mark = pool_mark(); people_cache_reset(); pers_reset(); plat_ui_reset(); plat_ui_alloc(); STB = 0; mark_g = mark + 1 + frame_no;
    si = ci == 0 ? 0 : ci == 2 ? 1 : 2; S = &SCR[si]; CI = ci; VEHI = veh; LEN = S->len;
    for (int i = 0; i < S->nland; i++) { LND[i] = S->land[i]; LND[i].x = iround(S->land[i].x * ((float)LEN / S->len0)); }
    for (int i = 0; i < S->np; i++) PLX[i] = iround(S->px[i] * ((float)LEN / S->len0));
    plat_dbg = 1; bg_build(LND, S->nland); plat_dbg = 2;
    wh_bmp = 0; if (S->type == T_EPIS) build_warehouse();
    for (int f = 0; f < 4; f++) { memset(&RR[f], 0, sizeof RR[f]); rle_build(&RR[f], compose_rider(CI, VEHI, f)->b, 0); }
    build_evil(); plat_pool = (pool_mark() & 0xFFFFFF) - (mark & 0xFFFFFF);
    RHh = compose_rider(CI, VEHI, 0)->h; plat_dbg = 3;
    bub_anchor = play_anchor; plat_maxn = 0; STARTT = 0;
    start_screen(); plat_dbg = 4;
    RES_L1 = S->c1; RES_L2 = S->c2;
    int result = 0;
    draw_play(); frame_end();
    for (;;) {
        int n = frame_end(); plat_note_n(n);
        if (key_hit & K_START) { if (pause_menu()) { result = 0; break; } frame_end(); continue; }
        if (key_hit & (K_A | K_UP)) jump_req = 1;
        if (key_hit & K_B) shoot_req = 1;
        if (B.on || gameOverAt) jump_req = shoot_req = 0;
        int done = 0;
        for (int i = 0; i < n && !done; i++) done = step();
       
        RES_C1 = c1; RES_C2 = c2;
        if (done) { result = 1; break; }
        draw_play(); dbg_update();
    }
    clear_bubbles(); bub_anchor = 0;
    pool_reset(mark); people_cache_reset(); pers_reset(); plat_ui_reset();
    return result;
}
