/* RPG · CATALUNYA (portado de rpgStart/rpgTick/rpgPress/rpgDraw/rpgUI del web).
   Pantalla Game Boy de 160x144 con 4 tonos, centrada en 240x160. Los 4 tonos del lienzo y los 4 de la
   interfaz son entradas propias de la paleta del hardware: cambiar de paleta, fundir o destellar es reescribirlas. */
#include "rpg_int.h"
#include "gen.h"

extern bmp_t T;
u32 rpg_prof[8], rpg_profs[8], rpg_profn[8];
#define PROFT(i, _t) do { if (_t > rpg_prof[i]) rpg_prof[i] = _t; rpg_profs[i] += _t; rpg_profn[i]++; } while (0)
#define PROF(i, x) do { u32 _t = ticks(); x; _t = ticks() - _t; PROFT(i, _t); } while (0)
#define PALRAM ((volatile u16 *)0x05000000)

/* ---------------------------------------------------------------- datos */
static const u32 GB_PALS[3][4] = {{0xe0f8d0, 0x88c070, 0x346856, 0x081820}, {0xf2f2ea, 0xaaa99e, 0x5b5a52, 0x151511}, {0xfff1e6, 0xf2a08a, 0xb3203a, 0x2a0a12}};
static const char *const GB_PAL_N[3] = {"VERDE", "GRIS", "FLiiTS"};
u8 rpg_pal;                                  /* saved["rpg.pal"] */
typedef struct { const char *name, *role; u8 hp; const char *atk[3]; } rnpc_t;
static const rnpc_t RPG_NPCS[10] = {
    {"Aamir", "Rider urbano", 52, {"Ruta alternativa", "Pedido duplicado", "Batería al 20%"}},
    {"Bilal", "Aspirante · ciclomotor", 48, {"Cambio de dirección", "Cliente impaciente", "Lluvia sorpresa"}},
    {"Hamza", "Rider de costa", 58, {"Viento de playa", "Subida larga", "Pedido doble"}},
    {"Imran", "Repartidor nocturno", 62, {"Turno de noche", "Portal sin número", "Ascensor averiado"}},
    {"Farhan", "Mecánico de motos", 55, {"Revisión de frenos", "Batería baja", "Pinchazo"}},
    {"Saira", "Especialista en rutas", 50, {"Atajo por el barrio", "Pedido urgente", "Dirección incorrecta"}},
    {"Ayesha", "Aspirante · formación", 46, {"Prueba de ruta", "Código de acceso", "Parada extra"}},
    {"Zain", "Rider · zona comercial", 60, {"Centro peatonal", "Pedido pesado", "Cambio de turno"}},
    {"Nil", "Rider local", 49, {"Calle cortada", "Pedido doble", "Lluvia"}},
    {"Lucas", "Aspirante · estación", 53, {"Tren con retraso", "Portal cerrado", "Ruta larga"}}};
enum { K_HIRE, K_DOCS, K_EPIS, K_RAUL };
typedef struct { const char *name; u8 spr, hp, nin, nout; const char *intro[2], *outro[2], *atk[4]; } boss_t;
typedef struct { u8 kind; const char *title; u8 nz, ztpl[2]; const char *zname[2]; const char *rules[2]; const boss_t *boss; } mission_t;
static const boss_t BOSS_M = {"MARTÍ", 0, 95, 1, 2, {"MARTÍ: ¡¡TU FORECAST NO CUADRA!!"}, {"MARTÍ: ¿Y estas contrataciones...?", "MARTÍ: ¡¡FORECAST PERFECTO!!"},
    {"Forecast negativo", "Hoja de cálculo", "Reunión urgente", "Recorte de plantilla"}};
static const boss_t BOSS_V = {"VALIDACIÓN", 1, 100, 2, 2, {"VALIDACIÓN: ¡¡RECHAZADO!! ¡¡RECHAZADO!!", "Usa tus documentos: ¡SUPER ENVÍO!"}, {"¡¡ALL APPROVED!!", "VALIDACIÓN: ¡Gracias!"},
    {"¡Rechazado!", "Falta una firma", "Vuelva mañana", "Sello caducado"}};
static const boss_t BOSS_A = {"ALMACÉN", 2, 110, 1, 2, {"MARTÍ Y FRANCO: ¡¡AQUÍ NO SE ENTREGA NADA!!"}, {"FRANCO: ¡Bienvenido al almacén!", "¡EPIs 100% entregados en Catalunya!"},
    {"Inventario cerrado", "Escáner sin batería", "Caja equivocada", "Pedido bloqueado"}};
static const mission_t MISSIONS[4] = {
    {K_HIRE, "CONTRATA A QUIEN HAGA GLOVO + UBER", 2, {0, 1}, {"BARCELONA", "SABADELL"},
     {"Habla con cada rider. Si hace GLOVO + UBER, bájale los PS y ofrécele un CONTRATO.", "Si solo hace Glovo, recházalo en combate. Y al que falla... ¡despídelo!"}, &BOSS_M},
    {K_RAUL, "ORDENA LA OFICINA", 1, {3, 3}, {"OFICINA FLiiTS", ""},
     {"La oficina es un caos: sin citas y sin turnos.", "Atiende a cada persona: VALIDA su cita, UNA cita por persona, SIN cita no se entra y RELLENA los huecos."}, 0},
    {K_DOCS, "PIDE LOS PAPELES A TODOS", 2, {0, 2}, {"BARCELONA", "GAVÀ · MATARÓ"},
     {"Cada aspirante te enseña un documento.", "APRUEBA los que están en regla y RECHAZA los que no. Los aprobados van a tu mochila."}, &BOSS_V},
    {K_EPIS, "¡NADIE SE VA SIN SU EQUIPO!", 2, {1, 0}, {"LLEIDA · MARTORELL", "SANT CUGAT · PARETS"},
     {"Cada rider necesita casco, chaqueta, guantes y mochila.", "Dale el EPI que le falta. ¡Si te equivocas, pierdes PS!"}, &BOSS_A}};
static const char *const LABELS[4][2] = {{"Contratados", "Despedidos"}, {"Aprobados", "Rechazados"}, {"EPIs", "Completos"}, {"Citas", "Turnos"}};
/* SCREENS[].npcs */
enum { A_HIRE, A_REJECT, A_FIRE };
static const struct { u8 red, act; const char *say; } HIRE_D[8] = {{0, A_HIRE, "¡Hago Glovo y Uber!"}, {0, A_REJECT, "Solo hago Glovo."}, {1, A_FIRE, "Solo hago Glovo."},
    {0, A_HIRE, "¡Hago Glovo y Uber!"}, {1, A_FIRE, "Mañana no trabajo"}, {0, A_REJECT, "Solo hago Glovo."}, {0, A_HIRE, "¡Hago Glovo y Uber!"}, {1, A_FIRE, "¡Este pedido está muy lejos!"}};
static const struct { const char *doc; u8 ok; const char *ans; } DOCS_D[8] = {{"NIE", 1, "Aquí tienes mi NIE"}, {"Carnet de moto", 0, "Tengo carnet extranjero"},
    {"Pasaporte", 1, "¡Pasaporte en regla!"}, {"Permiso de trabajo", 0, "Me caducó en 2025..."}, {"Carnet de moto", 1, "Carnet español, ¡toma!"}, {"NIE", 0, "NIE caducado..."},
    {"Permiso de trabajo", 1, "Permiso vigente"}, {"Pasaporte", 0, "Pasaporte caducado"}};
static const s8 EPIS_D[7][4] = {{0, -1}, {1, 2, -1}, {3, -1}, {0, 2, -1}, {1, -1}, {3, 0, -1}, {2, 1, 3, -1}};
static const char *const EPI_UP[4] = {"CASCO", "CHAQUETA", "GUANTES", "MOCHILA"};
static const char *const INAME[4] = {"el casco", "la chaqueta", "los guantes", "la mochila"};
enum { RC_CITA, RC_OTRA, RC_SIN, RC_CANCELA };
static const struct { u8 c; const char *code, *say; } RAUL_CASES[6] = {{RC_CITA, "7KQ2", "¡Hola! Tengo cita. Mi código es 7KQ2."}, {RC_OTRA, 0, "Ya tengo cita... ¿puedo coger otra para mañana?"},
    {RC_SIN, 0, "¡Vengo sin cita! ¿Me dejas pasar?"}, {RC_CITA, "M4TR", "Buenos días. Código de cita: M4TR."}, {RC_CANCELA, 0, "No puedo venir el martes. Cancelo mi cita de las 12h."},
    {RC_CITA, "X9PL", "Me llegó el mail con el código X9PL."}};
static const char *const SHIFTS[11] = {"LUN 12–16h", "LUN 19–23h", "MAR 12–16h", "MAR 19–23h", "MIÉ 12–16h", "JUE 19–23h", "VIE 19–23h", "SÁB 12–16h", "SÁB 19–23h", "DOM 12–16h", "DOM 19–23h"};
static const char *const RPG_MOVES[4][4] = {{"Oferta Glovo", "Oferta Uber", "Moto empresa", "Sueldo fijo"}, {"Pegatina FLiiTS", "Ruta optimizada", "Turno asignado", "PODER FLiiTS"},
    {"Revisar papeles", "Validar permiso", "Pasaporte + NIE", "Aprobación RTO"}, {"Casco", "Chaqueta", "Mochila térmica", "Kit completo"}};
static const struct { const char *tier; u8 pp, power; } RPG_TIERS[4] = {{"FLOJO", 24, 10}, {"MEDIO", 16, 18}, {"MEDIO", 13, 24}, {"FUERTE", 6, 36}};
static const u8 EN_ST[4][2] = {{22, 8}, {15, 15}, {12, 21}, {6, 30}};
static const s8 DVX[4] = {0, 0, -1, 1}, DVY[4] = {-1, 1, 0, 0};
#define OPP(d) ((d) ^ 1)

/* ---------------------------------------------------------------- estado */
typedef struct { char name[28]; const char *tier; u8 pp, maxPp, power; } move_t;
typedef struct { s8 x, y, fx, fy; u8 t, moving, dir, turn, flip; u16 step, bump; } mob_t;
typedef struct { mob_t m; u8 zone, done, boss, look, uid, state, need[4], nneed; s8 hx, hy, hd; s16 cool, alert; } npc_t;
enum { ST_IDLE, ST_ALERT, ST_APPROACH };
enum { SP_NPC, SP_PERSON, SP_BAGS, SP_BOSS0, SP_BOSS1, SP_BOSS2 };
typedef struct { int n; u8 boss, check, lv, spr, boxes, hidden, good, sig, coin, dizzy; char name[24]; s16 hp, maxHp, disp; move_t moves[4]; } enemy_t;
enum { MN_NONE, MN_ROOT, MN_FIGHT, MN_CHECK, MN_BAG, MN_PARTY, MN_START };
enum { V_MAP, V_BATTLE };
typedef struct { u8 k, on; s16 n, shown, len; void (*f)(int); int arg; char t[108]; } qi_t;
enum { QK_TEXT, QK_FN, QK_ANIM, QK_DRAIN };
enum { AN_FLASH, AN_WIPE, AN_SLIDE, AN_HEROATK, AN_ENEMYATK, AN_ENEMYDODGE, AN_HERODODGE, AN_HEROHIT, AN_ENEMYHIT, AN_FAINT, AN_THROW, AN_SIGN, AN_STAMP, AN_TEAR,
    AN_COIN, AN_COINS, AN_FADEOUT, AN_FADEIN };

static struct {
    int ci; const mission_t *M; u8 kind;
    s16 hp, maxHp, disp; u8 monster; u8 team[8], nteam; s16 c1, c2, docs, hires;
    u8 zi, tpl, view, menu, mcur, lock, fin, armor, ended;
    mob_t p, dog; u8 has_dog;
    npc_t npcs[10]; u8 nnpc;
    enemy_t E; u8 has_e;
    move_t moves[4];
    s16 banner; u32 t;
    struct { u8 on, a; s16 t, n; } anim;
    int win_at;
} mg;
static qi_t QB[96] EWRAM_BSS; static int qh, qn;
int rpg_maxn, rpg_nhist[4];                   /* rendimiento: n máximo e histograma de n */
u8 UN[4];                                     /* tonos de la interfaz (sin fundidos) */

/* ---------------------------------------------------------------- utilidades de texto */
static int u8dec(const char **ps) {
    const u8 *s = (const u8 *)*ps; int c = *s++;
    if (c >= 0xE0) { c = ((c & 15) << 12) | ((s[0] & 63) << 6) | (s[1] & 63); s += 2; }
    else if (c >= 0xC0) { c = ((c & 31) << 6) | (s[0] & 63); s += 1; }
    *ps = (const char *)s; return c;
}
static char *u8enc(char *d, int c) {
    if (c < 0x80) *d++ = c;
    else if (c < 0x800) { *d++ = 0xC0 | (c >> 6); *d++ = 0x80 | (c & 63); }
    else { *d++ = 0xE0 | (c >> 12); *d++ = 0x80 | ((c >> 6) & 63); *d++ = 0x80 | (c & 63); }
    return d;
}
/* toUpperCase (ASCII y Latin-1) */
static char *upcat(char *d, const char *s) {
    while (*s) { int c = u8dec(&s); if (c >= 'a' && c <= 'z') c -= 32; else if (c >= 0xE0 && c <= 0xFE && c != 0xF7) c -= 32; d = u8enc(d, c); }
    *d = 0; return d;
}
static char *num(char *d, int v) { return str_num(d, v); }
/* sb: concatena varias cadenas en un búfer rotativo */
static char SBUF[4][200]; static int sbi;
static char *cat(const char *a, ...) {
    char *buf = SBUF[sbi = (sbi + 1) & 3], *d = buf; *d = 0;
    __builtin_va_list ap; __builtin_va_start(ap, a);
    for (const char *s = a; s; s = __builtin_va_arg(ap, const char *)) d = str_cat(d, s);
    __builtin_va_end(ap); *d = 0; return buf;
}
static const char *itos(int v) { static char b[4][12]; static int k; char *s = b[k = (k + 1) & 3]; *num(s, v) = 0; return s; }
static const char *HERO(void) { static char b[16]; upcat(b, CREW[mg.ci].name); return b; }

/* ---------------------------------------------------------------- cola Q */
static qi_t *qpush(int k) { qi_t *q = &QB[(qh + qn) % 96]; if (qn < 96) qn++; q->k = k; q->on = 0; q->n = q->shown = q->len = 0; q->f = 0; q->arg = 0; q->t[0] = 0; return q; }
static void qpop(void) { qh = (qh + 1) % 96; qn--; }
static void Qrun(void (*f)(int), int arg) { qi_t *q = qpush(QK_FN); q->f = f; q->arg = arg; }
static void Qanim(int a, int n) { qi_t *q = qpush(QK_ANIM); q->arg = a; q->n = n; }
static void Qdrain(void) { qpush(QK_DRAIN); }
#define CH 17
static void Qsay(const char *s) {   /* rpgPages */
    static u16 L[32][CH + 1]; static u8 LL[32]; int nl = 0;
    u16 cp[300]; int n = 0; while (*s && n < 299) cp[n++] = u8dec(&s);
    int i = 0;
    for (;;) {
        u16 line[CH + 1]; int ln = 0;
        for (;;) {
            int ws = i; while (i < n && cp[i] != ' ' && cp[i] != '\n') i++; int wl = i - ws;
            while (wl > CH) { if (ln) { memcpy(L[nl], line, ln * 2); LL[nl++] = ln; ln = 0; } memcpy(L[nl], cp + ws, CH * 2); LL[nl++] = CH; ws += CH; wl -= CH; if (nl >= 30) break; }
            if (!ln) { memcpy(line, cp + ws, wl * 2); ln = wl; }
            else if (ln + 1 + wl <= CH) { line[ln++] = ' '; memcpy(line + ln, cp + ws, wl * 2); ln += wl; }
            else { memcpy(L[nl], line, ln * 2); LL[nl++] = ln; memcpy(line, cp + ws, wl * 2); ln = wl; }
            if (nl >= 30) break;
            if (i < n && cp[i] == ' ') { i++; continue; }
            break;
        }
        memcpy(L[nl], line, ln * 2); LL[nl++] = ln;
        if (i < n && cp[i] == '\n' && nl < 30) { i++; continue; }
        break;
    }
    for (int k = 0; k < nl; k += 2) {
        qi_t *q = qpush(QK_TEXT); char *d = q->t; int c = 0;
        for (int j = 0; j < LL[k]; j++) { d = u8enc(d, L[k][j]); c++; }
        if (k + 1 < nl) { *d++ = '\n'; c++; for (int j = 0; j < LL[k + 1]; j++) { d = u8enc(d, L[k + 1][j]); c++; } }
        *d = 0; q->len = c; q->shown = 0;
    }
}

/* ---------------------------------------------------------------- declaraciones */
static void rpg_open(int id) { mg.menu = id; mg.mcur = 0; }
static void rpg_close(void) { mg.menu = MN_NONE; }
static void f_open(int id) { rpg_open(id); }
static void rpg_battle(int ni); static void rpg_check(int ni); static void rpg_enemy_turn(int unused); static void rpg_enemy_down(void);
static void rpg_lose(void); static void rpg_resolve(int ni); static void rpg_back_to_map(int after);
static void rpg_enter_zone(int zi, int x, int y, int dir);
static void f_battle(int ni) { rpg_battle(ni); }
static void f_check(int ni) { rpg_check(ni); }
static void f_sfx(int k) { if (k == 0) sfx_power(); else if (k == 1) sfx_hit(); else if (k == 2) sfx_coin(); else if (k == 3) sfx_clear(); else if (k == 4) sfx_win(); }
static int remaining(void) { int r = 0; for (int i = 0; i < mg.nnpc; i++) if (!mg.npcs[i].boss && !mg.npcs[i].done) r++; return r; }
static const char *npc_name(npc_t *n) { static char b[4][24]; static int k; char *s = b[k = (k + 1) & 3];
    if (n->boss) return mg.M->boss->name; upcat(s, RPG_NPCS[n->look].name); return s; }
static int active(npc_t *n) { return n->zone == mg.zi && !n->done; }
static void moves_for(int ci) { for (int i = 0; i < 4; i++) { strcpy(mg.moves[i].name, RPG_MOVES[ci][i]);
    mg.moves[i].tier = RPG_TIERS[i].tier; mg.moves[i].pp = mg.moves[i].maxPp = RPG_TIERS[i].pp; mg.moves[i].power = RPG_TIERS[i].power; } }
static void enemy_moves(const char *const *names) { for (int i = 0; i < 4; i++) { strcpy(mg.E.moves[i].name, names[i]); mg.E.moves[i].pp = mg.E.moves[i].maxPp = EN_ST[i][0]; mg.E.moves[i].power = EN_ST[i][1]; mg.E.moves[i].tier = ""; } }

/* ---------------------------------------------------------------- inicio */
static void new_state(int ci) {
    memset(&mg, 0, sizeof mg); qh = qn = 0;
    mg.ci = ci; mg.M = &MISSIONS[ci]; mg.kind = mg.M->kind; mg.hp = mg.maxHp = mg.disp = 100; mg.monster = 2; mg.win_at = -1;
    moves_for(ci);
}
static void rpg_start(int ci) {
    new_state(ci);
    const mission_t *M = mg.M; int nd = M->kind == K_HIRE ? 8 : M->kind == K_DOCS ? 8 : M->kind == K_EPIS ? 7 : 6, nz = M->nz, per = (nd + nz - 1) / nz, cnt[2] = {0, 0};
    for (int k = 0; k < nd; k++) {
        int zi = imin(nz - 1, k / per); const tpl_t *Tp = &TPL[M->ztpl[zi]]; const slot_t *s = &Tp->slots[cnt[zi]++ % Tp->nslots];
        npc_t *n = &mg.npcs[mg.nnpc++]; memset(n, 0, sizeof *n);
        n->uid = k; n->look = (k * 3 + ci * 2) % 10; n->zone = zi; n->hx = s->x; n->hy = s->y; n->hd = s->d;
        if (M->kind == K_EPIS) { for (int j = 0; j < 4 && EPIS_D[k][j] >= 0; j++) n->need[n->nneed++] = EPIS_D[k][j]; }
    }
    if (M->boss) { npc_t *n = &mg.npcs[mg.nnpc++]; memset(n, 0, sizeof *n); const slot_t *s = &TPL[M->ztpl[nz - 1]].boss;
        n->boss = 1; n->uid = 99; n->zone = nz - 1; n->hx = s->x; n->hy = s->y; n->hd = s->d; }
    const slot_t *st = &TPL[M->ztpl[0]].start; rpg_enter_zone(0, st->x, st->y, st->d);
    Qsay(cat("¡", HERO(), "! MISIÓN: ", M->title, 0)); Qsay(M->rules[0]); Qsay(M->rules[1]);
}
static void stepTo(mob_t *o, int x, int y) { o->fx = o->x; o->fy = o->y; o->x = x; o->y = y; o->t = 0; o->moving = 1; o->step++; }
static void mob_at(mob_t *o, int x, int y, int dir) { memset(o, 0, sizeof *o); o->x = o->fx = x; o->y = o->fy = y; o->t = 16; o->dir = dir; }
static u8 map_dirty; static u32 scene_key; static void map_restore(void);
static void rpg_enter_zone(int zi, int x, int y, int dir) {
    if (map_dirty) { MAPB->w = MW * 16; map_dirty = 0; scene_key = 0; }
    mg.zi = zi; mg.tpl = mg.M->ztpl[zi]; mg.banner = 120;
    mob_at(&mg.p, x, y, dir);
    rpg_map_build(mg.tpl); rpg_cache_reset();
    if (mg.ci == 0) { int od = OPP(dir), bx = x + DVX[od], by = y + DVY[od], ok = bx >= 0 && bx < MW && by >= 0 && by < MH && !rpg_solid_ch(GRID[mg.tpl][by][bx]);
        mob_at(&mg.dog, ok ? bx : x, ok ? by : y, 0); mg.dog.flip = dir == D_LEFT; mg.has_dog = 1; }
    else mg.has_dog = 0;
    for (int i = 0; i < mg.nnpc; i++) { npc_t *n = &mg.npcs[i]; if (!active(n)) continue; mob_at(&n->m, n->hx, n->hy, n->hd); n->state = ST_IDLE; n->cool = 60; }
}
static void f_enter(int pk) { rpg_enter_zone(pk >> 24, (pk >> 16) & 255, (pk >> 8) & 255, pk & 255); mg.lock = 0; }
static int rpg_free(void) { return !qn && !mg.menu && !mg.lock && mg.view == V_MAP; }

/* ---------------------------------------------------------------- mapa */
static int rpg_solid(int x, int y, int who) { /* who: -1 jugador, si no índice de PNJ */
    if (x < 0 || y < 0 || x >= MW || y >= MH) return 1;
    if (rpg_solid_ch(GRID[mg.tpl][y][x])) return 1;
    const tpl_t *Tp = &TPL[mg.tpl];
    for (int i = 0; i < Tp->nbld; i++) { const bld_t *b = &Tp->bld[i]; if (x >= b->x && x < b->x + b->w && y >= b->y && y < b->y + b->h) return 1; }
    if (who != -1 && mg.p.x == x && mg.p.y == y) return 1;
    for (int i = 0; i < mg.nnpc; i++) if (i != who && active(&mg.npcs[i]) && mg.npcs[i].m.x == x && mg.npcs[i].m.y == y) return 1;
    return 0;
}
static void rpg_challenge(int ni);
static void rpg_travel(int z, int x, int y, int dir) { mg.lock = 1; Qanim(AN_FADEOUT, 16); Qrun(f_enter, (z << 24) | (x << 16) | (y << 8) | dir); Qanim(AN_FADEIN, 16); }
static int rpg_sight(void) {
    if (mg.kind == K_RAUL) return 0;
    for (int i = 0; i < mg.nnpc; i++) { npc_t *n = &mg.npcs[i]; if (!active(n) || n->boss || n->state != ST_IDLE || n->m.moving || n->cool > 0) continue;
        for (int k = 1; k <= 4; k++) { int x = n->m.x + DVX[n->m.dir] * k, y = n->m.y + DVY[n->m.dir] * k;
            if (x == mg.p.x && y == mg.p.y) { mg.lock = 1; n->state = ST_ALERT; n->alert = 40; tone(880, 60, 0, W_SQUARE, 60); tone(1320, 100, 70, W_SQUARE, 60); return 1; }
            if (rpg_solid(x, y, i)) break; } }
    return 0;
}
static void rpg_map_update(void) {
    mob_t *p = &mg.p;
    if (p->moving && ++p->t >= 16) p->moving = 0;
    if (mg.has_dog && mg.dog.moving && ++mg.dog.t >= 16) mg.dog.moving = 0;
    for (int i = 0; i < mg.nnpc; i++) { npc_t *n = &mg.npcs[i]; if (active(n) && n->m.moving && ++n->m.t >= 16) n->m.moving = 0; }
    if (mg.banner > 0) mg.banner--;
    for (int i = 0; i < mg.nnpc; i++) { npc_t *n = &mg.npcs[i]; if (!active(n)) continue;
        if (n->cool > 0) n->cool--;
        if (n->state == ST_ALERT) { if (--n->alert <= 0) n->state = ST_APPROACH; continue; }
        if (n->state == ST_APPROACH && !n->m.moving) { int dist = iabs(p->x - n->m.x) + iabs(p->y - n->m.y), dx = DVX[n->m.dir], dy = DVY[n->m.dir];
            if (dist <= 1 || rpg_solid(n->m.x + dx, n->m.y + dy, i)) { n->state = ST_IDLE; n->cool = 300; p->dir = OPP(n->m.dir); mg.lock = 0; rpg_challenge(i); }
            else stepTo(&n->m, n->m.x + dx, n->m.y + dy);
            continue; }
        if (!n->boss && mg.kind != K_RAUL && n->state == ST_IDLE && !n->m.moving && rpg_free() && (mg.t + n->uid * 37) % 110 == 0) n->m.dir = rndi(4);
    }
    if (p->moving || !rpg_free()) return;
    if (rpg_sight()) return;
    int want = (key_held & K_UP) ? D_UP : (key_held & K_DOWN) ? D_DOWN : (key_held & K_LEFT) ? D_LEFT : (key_held & K_RIGHT) ? D_RIGHT : -1;
    if (want < 0) { p->turn = 0; return; }
    if (want != p->dir && p->turn == 0) { p->dir = want; p->turn = 5; return; }
    if (p->turn > 1) { p->turn--; return; }
    p->dir = want; int x = p->x + DVX[want], y = p->y + DVY[want];
    if (x < 0 || x >= MW) { int to = mg.zi + (x < 0 ? -1 : 1);
        if (to >= 0 && to < mg.M->nz) { rpg_travel(to, x < 0 ? MW - 1 : 0, y, want); return; }
        if (p->bump++ % 40 == 0) { sfx_bump(); Qsay(x < 0 ? "Tu misión sigue en la otra dirección." : "¡Aún no! Termina la misión antes de irte."); } return; }
    if (rpg_solid(x, y, -1)) { if (p->bump++ % 18 == 0) sfx_bump(); return; }
    p->bump = 0;
    if (mg.has_dog) { mob_t *d = &mg.dog; stepTo(d, p->x, p->y); d->flip = d->x < d->fx ? 1 : d->x > d->fx ? 0 : d->flip; }
    stepTo(p, x, y);
}
static void rpg_challenge(int ni) {
    npc_t *n = &mg.npcs[ni]; const char *who = npc_name(n); const mission_t *M = mg.M;
    if (n->boss) { int rem = remaining();
        if (rem > 0) { Qsay(cat(who, ": ¡Aún te quedan ", itos(rem), " por atender! Vuelve cuando acabes.", 0)); return; }
        for (int i = 0; i < M->boss->nin; i++) Qsay(M->boss->intro[i]); Qrun(f_battle, ni); return; }
    int k = n->uid;
    if (M->kind == K_HIRE) { Qsay(cat(who, ": ", HIRE_D[k].red ? "Ya reparto con FLiiTS, pero..." : "Busco trabajo de rider.", " ", HIRE_D[k].say, 0)); Qrun(f_battle, ni); }
    else if (M->kind == K_DOCS) { Qsay(cat(HERO(), ": ¿Me enseñas tu ", DOCS_D[k].doc, "?", 0)); Qsay(cat(who, ": ", DOCS_D[k].ans, 0)); Qrun(f_check, ni); }
    else if (M->kind == K_EPIS) { Qsay(cat(who, ": ¡Me falta ", INAME[n->need[0]], "!", 0)); Qrun(f_check, ni); }
    else { Qsay(cat(who, ": ", RAUL_CASES[k].say, 0)); Qrun(f_check, ni); }
}
static void f_hub(int u) { (void)u; mg.hp = mg.maxHp; mg.disp = mg.hp; sfx_power(); }
static void rpg_interact(void) {
    mob_t *p = &mg.p; int x = p->x + DVX[p->dir], y = p->y + DVY[p->dir]; const tpl_t *Tp = &TPL[mg.tpl];
    for (int i = 0; i < mg.nnpc; i++) { npc_t *n = &mg.npcs[i]; if (active(n) && n->m.x == x && n->m.y == y) { if (!n->boss) n->m.dir = OPP(p->dir); rpg_challenge(i); return; } }
    if (Tp->sx == x && Tp->sy == y) {
        if (mg.kind == K_RAUL) Qsay("OFICINA FLiiTS\nCitas · formación · turnos");
        else { const mission_t *M = mg.M; Qsay(cat(M->zname[mg.zi], mg.zi > 0 ? "\n« " : "", mg.zi > 0 ? M->zname[mg.zi - 1] : "",
            mg.zi + 1 < M->nz ? "\n" : "", mg.zi + 1 < M->nz ? M->zname[mg.zi + 1] : "", mg.zi + 1 < M->nz ? " »" : "", 0)); }
        return; }
    for (int i = 0; i < Tp->nbld; i++) { const bld_t *b = &Tp->bld[i];
        if (x == b->x + b->w / 2 && y == b->y + b->h - 1) {
            if (b->hub) { Qsay("FLiiTS HUB. Te tomas un descanso..."); Qrun(f_hub, 0); Qsay("¡PS al máximo!"); } else Qsay(b->msg);
            return; } }
    if (y >= 0 && y < MH && x >= 0 && x < MW && GRID[mg.tpl][y][x] == '~') Qsay("El Mediterráneo brilla al sol.");
}

/* ---------------------------------------------------------------- combates */
static void f_intro_tones(int u) { static const u16 f[7] = {523, 494, 523, 494, 523, 659, 784}; (void)u; for (int i = 0; i < 7; i++) tone(f[i], 70, i * 60, W_SQUARE, 60); }
static void f_view_battle(int u) { (void)u; mg.view = V_BATTLE; }
static void f_boxes(int u) { (void)u; mg.E.boxes = 1; }
static void rpg_intro_battle(void) { Qrun(f_intro_tones, 0); Qanim(AN_FLASH, 28); Qanim(AN_WIPE, 24); Qrun(f_view_battle, 0); Qanim(AN_SLIDE, 36); Qrun(f_boxes, 0); }
static void rpg_battle(int ni) {
    npc_t *n = &mg.npcs[ni]; const boss_t *B = mg.M->boss; const rnpc_t *d = n->boss ? 0 : &RPG_NPCS[n->look];
    memset(&mg.E, 0, sizeof mg.E); mg.has_e = 1; enemy_t *e = &mg.E;
    e->n = ni; e->boss = n->boss; strcpy(e->name, npc_name(n)); e->hp = e->maxHp = e->disp = n->boss ? B->hp : d->hp; e->lv = n->boss ? 9 : d->hp > 55 ? 5 : 4;
    if (n->boss) enemy_moves(B->atk); else { const char *nm[4] = {d->atk[0], d->atk[1], d->atk[2], "Contraataque"}; enemy_moves(nm); }
    e->spr = n->boss ? SP_BOSS0 + B->spr : SP_NPC;
    moves_for(mg.ci); mg.disp = mg.hp;
    if (n->boss && mg.kind == K_DOCS && mg.docs) { move_t *m = &mg.moves[3]; char *s = str_cat(m->name, "SUPER ENVÍO ×"); *num(s, mg.docs) = 0;
        m->tier = "ESPECIAL"; m->pp = m->maxPp = 2; m->power = 14 + 6 * mg.docs; }
    rpg_intro_battle();
    Qsay(n->boss ? cat("¡", e->name, " te desafía!", 0) : HIRE_D[n->uid].red ? cat("¡", e->name, " no se quiere ir!", 0) : cat("¡", e->name, " quiere demostrar que vale!", 0));
    Qrun(f_open, MN_ROOT);
}
static void rpg_check(int ni) {
    npc_t *n = &mg.npcs[ni]; memset(&mg.E, 0, sizeof mg.E); mg.has_e = 1; enemy_t *e = &mg.E;
    e->n = ni; e->check = 1; strcpy(e->name, npc_name(n)); e->hp = e->maxHp = e->disp = 1; e->spr = mg.kind == K_EPIS ? SP_PERSON : SP_NPC;
    mg.disp = mg.hp; Qanim(AN_FLASH, 12); Qanim(AN_WIPE, 18); Qrun(f_view_battle, 0); Qanim(AN_SLIDE, 30); Qrun(f_boxes, 0); Qrun(f_open, MN_CHECK);
}
static void f_hero_dmg(int d) { mg.hp = imax(0, mg.hp - d); sfx_hit(); }
static void f_after_penalty(int u) { (void)u; if (mg.hp <= 0) rpg_lose(); else rpg_open(MN_CHECK); }
static void rpg_penalty(int dmg, const char *why) { rpg_close(); sfx_back(); Qsay(why); Qrun(f_hero_dmg, dmg); Qanim(AN_HEROHIT, 20); Qdrain(); Qrun(f_after_penalty, 0); }
static void f_resolve(int ni) { rpg_resolve(ni); }
static void rpg_decide(int i) {
    enemy_t *e = &mg.E; npc_t *n = &mg.npcs[e->n]; const char *who = e->name; int k = n->uid;
    if (mg.kind == K_DOCS) {
        int ok = DOCS_D[k].ok;
        if ((i == 0) != ok) { rpg_penalty(15, ok ? cat("¡Ojo! El ", DOCS_D[k].doc, " de ", who, " estaba en regla.", 0) : cat("¡Ojo! Ese ", DOCS_D[k].doc, " no es válido.", 0)); return; }
        rpg_close();
        if (ok) { mg.c1++; mg.docs++; Qrun(f_sfx, 0); Qsay("¡APROBADO!"); Qsay(cat("El ", DOCS_D[k].doc, " va a la mochila. Llevas ", itos(mg.docs), ".", 0)); }
        else { mg.c2++; Qrun(f_sfx, 1); Qsay("¡RECHAZADO!"); Qsay(cat(who, ": Volveré con los papeles en regla.", 0)); }
        Qrun(f_resolve, e->n); return;
    }
    if (mg.kind == K_EPIS) {
        int has = -1; for (int j = 0; j < n->nneed; j++) if (n->need[j] == i) has = j;
        if (has < 0) { rpg_penalty(10, cat("¡", who, " ya tiene ", INAME[i], "!", 0)); return; }
        rpg_close(); for (int j = has; j + 1 < n->nneed; j++) n->need[j] = n->need[j + 1]; n->nneed--; mg.c1++;
        Qrun(f_sfx, 2); Qsay(cat("¡", EPI_UP[i], " entregado!", 0));
        if (n->nneed) { Qsay(cat(who, ": ¡Gracias! Pero me falta ", INAME[n->need[0]], ".", 0)); Qrun(f_open, MN_CHECK); }
        else { mg.c2++; Qrun(f_sfx, 0); Qsay(cat(who, ": ¡Completo! ¡Gracias!", 0)); Qrun(f_resolve, e->n); }
        return;
    }
    int c = RAUL_CASES[k].c;
    if (i != c) { rpg_penalty(10, "Así no... ¡piensa en el nuevo sistema de citas!"); return; }
    rpg_close(); Qrun(f_sfx, 2);
    if (c == RC_CITA) { mg.c1++; mg.c2++; Qsay(cat("✓ Código ", RAUL_CASES[k].code, " validado.", 0)); Qsay(cat("Turno: ", SHIFTS[(mg.c1 * 3) % 11], ". ¡Pasa a formación!", 0)); }
    else if (c == RC_OTRA) Qsay("✓ Solo una cita por persona. ¡Nadie acapara huecos!");
    else if (c == RC_SIN) Qsay("✓ Sin cita no se entra. Pide tu cita con el QR.");
    else { mg.c2++; Qsay("✓ Hueco rellenado: adelantamos a quien tenía la cita lejos."); }
    Qrun(f_resolve, e->n);
}
/* after: 0 nada, 1<<8|ni hecho, 2<<8|ni enfriar 360, 3<<8|ni enfriar 300 */
static void f_back(int after) {
    mg.view = V_MAP; mg.has_e = 0; mg.menu = MN_NONE;
    int a = after >> 8, ni = after & 255;
    if (a == 1) mg.npcs[ni].done = 1; else if (a == 2) mg.npcs[ni].cool = 360; else if (a == 3) mg.npcs[ni].cool = 300;
}
static void rpg_back_to_map(int after) { Qanim(AN_FADEOUT, 16); Qrun(f_back, after); Qanim(AN_FADEIN, 16); }
static void f_mission_done(int u) { (void)u; mg.ended = 1; }
static void f_after_resolve(int u) {
    (void)u; if (remaining() > 0) return;
    if (mg.M->boss) Qsay(cat("¡Todos atendidos! Ahora, a por ", mg.M->boss->name, ".", 0));
    else { Qrun(f_sfx, 3); Qsay("¡Oficina en orden ✓!"); Qsay("Todos tienen su cita y su turno."); Qanim(AN_FADEOUT, 20); Qrun(f_mission_done, 0); }
}
static void rpg_resolve(int ni) { rpg_back_to_map((1 << 8) | ni); Qrun(f_after_resolve, 0); }
static void f_enemy_dmg(int d) { mg.E.hp = imax(0, mg.E.hp - d); sfx_hit(); }
static void f_after_hit(int u) { (void)u; if (mg.E.hp <= 0) rpg_enemy_down(); else rpg_enemy_turn(0); }
static void f_enemy_turn(int u) { rpg_enemy_turn(u); }
static void rpg_use_move(int i) {
    move_t *m = &mg.moves[i]; if (m->pp <= 0) return; rpg_close(); m->pp--;
    enemy_t *e = &mg.E; int ci = mg.ci, dmg = m->power, crit = 0; const char *note = 0;
    if (!strncmp(m->name, "SUPER ENVÍO", 12)) { crit = 1; note = cat("¡¡", itos(mg.docs), " DOCUMENTOS EN REGLA!!", 0); }
    else if (ci == 0 && i == 3) { dmg += 5; note = "¡La oferta FLiiTS convence!"; }
    else if (ci == 2 && i >= 2) { dmg = (m->power * 135 + 50) / 100; crit = 1; note = "¡Documentación validada!"; }
    else if (ci == 2 && i == 0) { dmg = imax(6, (m->power * 7 + 5) / 10); note = "Tendrá que corregir sus papeles."; }
    else if (ci == 3) note = "¡EPI entregado!";
    if (!crit && rndi(100) < 8) { crit = 1; dmg = (dmg * 3 + 1) / 2; }
    dmg += rndi(4);
    char up[32]; upcat(up, m->name);
    Qsay(cat("¡", HERO(), " usa ", up, "!", 0)); Qanim(AN_HEROATK, 14);
    if (!e->boss && rndi(100) < 10) { Qanim(AN_ENEMYDODGE, 20); Qsay(cat("¡", e->name, " lo esquiva!", 0)); Qrun(f_enemy_turn, 0); return; }
    Qrun(f_enemy_dmg, dmg); Qanim(AN_ENEMYHIT, 24); Qdrain();
    if (crit) Qsay("¡Golpe crítico!"); if (note) Qsay(note);
    Qrun(f_after_hit, 0);
}
static void f_after_hero_hit(int u) { (void)u; if (mg.hp <= 0) rpg_lose(); else rpg_open(MN_ROOT); }
static void rpg_enemy_turn(int u) {
    (void)u; enemy_t *e = &mg.E; int av[4], na = 0; for (int i = 0; i < 4; i++) if (e->moves[i].pp > 0) av[na++] = i;
    if (!na) { Qsay(cat("¡", e->name, " no tiene fuerzas para atacar!", 0)); Qrun(f_open, MN_ROOT); return; }
    move_t *m = &e->moves[av[rndi(na)]]; m->pp--;
    char up[32]; upcat(up, m->name);
    Qsay(cat("¡", e->name, " usa ", up, "!", 0)); Qanim(AN_ENEMYATK, 14);
    if (rndi(100) < 12) { Qanim(AN_HERODODGE, 20); Qsay(cat("¡", HERO(), " lo esquiva!", 0)); Qrun(f_open, MN_ROOT); return; }
    int dmg = imax(4, m->power + rndi(5));
    Qrun(f_hero_dmg, dmg); Qanim(AN_HEROHIT, 24); Qdrain(); Qrun(f_after_hero_hit, 0);
}
static void f_slide(int k) { if (k == 0) slide(600, 120, 400); else slide(400, 900, 300); }
static void f_hide(int w) { mg.E.hidden = 1; if (w) sfx_win(); }
static void f_c2(int u) { (void)u; mg.c2++; }
static void f_good(int u) { (void)u; mg.E.good = 1; if (mg.kind == K_DOCS) mg.E.sig = 1; }
static void f_coin(int on) { mg.E.coin = on; if (!on) { sfx_coin(); sfx_win(); } }
static void f_dizzy(int u) { (void)u; mg.E.dizzy = 1; sfx_win(); }
static void rpg_boss_win(void) {
    const boss_t *B = mg.M->boss;
    Qrun(f_sfx, 0); Qanim(AN_FLASH, 24); Qrun(f_good, 0);
    for (int i = 0; i < B->nout; i++) Qsay(B->outro[i]);
    Qrun(f_coin, 1); Qanim(AN_COIN, 70); Qrun(f_coin, 0); Qsay("¡MONEDA DE ORO!");
    Qanim(AN_FADEOUT, 20); Qrun(f_mission_done, 0);
}
static void rpg_enemy_down(void) {
    enemy_t *e = &mg.E;
    if (mg.fin) { Qrun(f_dizzy, 0); Qsay("MOCHILAS: ¡¡ESTÁ BIEN!! ¡¡ESTÁ BIEN!!"); Qsay("¡¡NUEVAS ZONAS PARA REPARTIR!!"); Qanim(AN_FADEOUT, 24); Qrun(f_mission_done, 0); return; }
    if (e->boss) { rpg_boss_win(); return; }
    Qrun(f_slide, 0); Qanim(AN_FAINT, 22); Qrun(f_hide, 0);
    int act = HIRE_D[mg.npcs[e->n].uid].act;
    if (act == A_HIRE) { Qsay(cat(e->name, " se va a pensarlo...", 0)); Qsay("¡Era buen candidato! Vuelve a hablarle y ofrécele un CONTRATO."); rpg_back_to_map((2 << 8) | e->n); }
    else if (act == A_REJECT) { Qsay(cat("¡", e->name, " RECHAZADO!", 0)); Qsay("Solo Glovo no puede ser."); Qrun(f_resolve, e->n); }
    else { Qrun(f_c2, 0); Qsay(cat("¡", e->name, " DESPEDIDO!", 0)); Qrun(f_resolve, e->n); }
}
static void f_heal_final(int u) { (void)u; mg.hp = mg.maxHp; sfx_power(); }
static void f_lose_reset(int u) {
    (void)u; mg.hp = mg.maxHp; mg.disp = mg.hp; mg.view = V_MAP; mg.has_e = 0; mg.menu = MN_NONE;
    const slot_t *st = &TPL[mg.M->ztpl[0]].start; rpg_enter_zone(0, st->x, st->y, st->d);
}
static void rpg_lose(void) {
    Qsay(cat("¡", HERO(), " está agotado!", 0));
    if (mg.fin) { Qrun(f_heal_final, 0); Qdrain(); Qsay("¡Pero el equipo no se rinde! ¡Otra vez!"); Qrun(f_open, MN_ROOT); return; }
    Qsay("Vuelves al inicio de la misión a descansar...");
    Qanim(AN_FADEOUT, 16); Qrun(f_lose_reset, 0); Qanim(AN_FADEIN, 16);
    Qsay("¡PS recuperados! Inténtalo otra vez.");
}
static void rpg_flee(void) {
    rpg_close(); enemy_t *e = &mg.E;
    if (e->boss || mg.fin) { Qsay("¡No puedes huir de un jefe!"); Qrun(f_open, MN_ROOT); return; }
    Qsay("¡Escapas sin problemas!"); rpg_back_to_map((3 << 8) | e->n);
}
static void f_heal(int h) { mg.hp += h; sfx_power(); }
static void rpg_monster(void) {
    int battle = mg.view == V_BATTLE, inFight = battle && !(mg.has_e && mg.E.check); rpg_close();
    if (mg.monster <= 0) { Qsay("No te quedan latas de MONSTER."); Qrun(f_open, MN_BAG); return; }
    if (mg.hp >= mg.maxHp) { Qsay("Ya tienes los PS al máximo."); Qrun(f_open, MN_BAG); return; }
    mg.monster--; int heal = imin(35, mg.maxHp - mg.hp);
    Qsay(cat("¡", HERO(), " se bebe una MONSTER!", 0)); Qrun(f_heal, heal); Qdrain(); Qsay(cat("Recupera ", itos(heal), " PS.", 0));
    if (inFight) Qrun(f_enemy_turn, 0); else if (battle) Qrun(f_open, MN_CHECK);
}
static void f_sign(int u) { (void)u; mg.E.sig = 1; tone(70, 300, 0, W_SQUARE, 140); }
static void f_hired(int u) { (void)u; mg.c1++; mg.hires++; if (mg.nteam < 8) mg.team[mg.nteam++] = mg.npcs[mg.E.n].look; }
static void rpg_contract(void) {
    rpg_close(); enemy_t *e = &mg.E;
    if (e->boss) { Qsay("¡A un jefe no se le ofrece contrato!"); Qrun(f_open, MN_ROOT); return; }
    int act = HIRE_D[mg.npcs[e->n].uid].act;
    if (act == A_REJECT) { Qsay(cat(e->name, " solo hace Glovo...", 0)); Qsay("¡Solo Glovo no puede ser! Recházalo."); Qrun(f_open, MN_ROOT); return; }
    if (act == A_FIRE) { Qsay(cat(e->name, " ya trabaja con FLiiTS... ¡y falla!", 0)); Qsay("Aquí toca DESPEDIR."); Qrun(f_open, MN_ROOT); return; }
    if (e->hp * 100 > e->maxHp * 45) { Qsay(cat(e->name, " aún no está convencido.", 0)); Qsay("Baja sus PS por debajo de la mitad."); Qrun(f_open, MN_ROOT); return; }
    Qsay(cat("¡", HERO(), " ofrece un CONTRATO!", 0)); Qrun(f_slide, 1); Qanim(AN_THROW, 30); Qanim(AN_SIGN, 46);
    if (rndi(100) < 82) { Qrun(f_sign, 0); Qanim(AN_STAMP, 24); Qrun(f_hide, 1); Qrun(f_hired, 0);
        Qsay("¡CONTRATO FIRMADO!"); Qsay(cat("¡", e->name, " CONTRATADO! ¡Tú a ", mg.zi ? "Sabadell" : "Barcelona", "!", 0)); Qrun(f_resolve, e->n); }
    else { Qanim(AN_TEAR, 30); Qsay(cat("¡", e->name, " rompe el contrato!", 0)); Qrun(f_enemy_turn, 0); }
}
/* final: las mochilas gigantes */
static void f_armor(int u) { (void)u; mg.armor = 1; sfx_power(); mg.win_at = mg.t + 18; }
static void rpg_start_final(void) {
    new_state(1); mg.fin = 1; mg.view = V_BATTLE; mg.hp = mg.maxHp = mg.disp = 140;
    enemy_t *e = &mg.E; mg.has_e = 1; e->n = -1; strcpy(e->name, "MOCHILAS"); e->hp = e->maxHp = e->disp = 160; e->lv = 99; e->boss = 1; e->spr = SP_BAGS; e->boxes = 1;
    static const char *const AT[4] = {"No hay más zonas", "Pedido gigante", "Lluvia de pedidos", "Aplastón"}; enemy_moves(AT);
    mg.tpl = 3; mg.p.x = 9; mg.p.y = 7;
    Qsay("¡Las MOCHILAS GIGANTES no dejan repartir en Catalunya!"); Qsay("MOCHILAS: ¡¡NO HAY MÁS ZONAS!!");
    Qsay("¡Ivan, Gerard y Azlan te dan sus monedas de oro!"); Qanim(AN_COINS, 80);
    Qrun(f_armor, 0); Qanim(AN_FLASH, 24); Qsay("¡¡PODER FLiiTS!!"); Qrun(f_open, MN_ROOT);
}

/* ---------------------------------------------------------------- menús (rpgMenuDef) */
enum { CL_ROOT, CL_LOW, CL_SIDE, CL_FULL };
typedef struct { char t[44]; u8 dis, act; } mi_t;
typedef struct { u8 cls, grid, n, has_back; const char *prompt; char info[2][24]; mi_t it[8]; } menu_t;
enum { MA_NONE, MA_OPEN_FIGHT, MA_OPEN_PARTY, MA_OPEN_BAG, MA_FLEE, MA_MOVE0, MA_MOVE1, MA_MOVE2, MA_MOVE3, MA_DEC0, MA_DEC1, MA_DEC2, MA_DEC3,
    MA_MONSTER, MA_CONTRACT, MA_BACK, MA_MISSION, MA_PAL, MA_QUIT, MA_CLOSE };
static void add(menu_t *m, const char *t, int act, int dis) { mi_t *it = &m->it[m->n++]; strncpy(it->t, t, 43); it->t[43] = 0; it->act = act; it->dis = dis; }
static void menu_def(menu_t *m) {
    memset(m, 0, sizeof *m); int battle = mg.view == V_BATTLE;
    switch (mg.menu) {
    case MN_ROOT: m->cls = CL_ROOT; m->grid = 2; m->prompt = "¿Qué\nhará?"; add(m, "LUCHAR", MA_OPEN_FIGHT, 0); add(m, "EQUIPO", MA_OPEN_PARTY, 0); add(m, "MOCHILA", MA_OPEN_BAG, 0); add(m, "HUIR", MA_FLEE, 0); break;
    case MN_FIGHT: { m->cls = CL_LOW; m->has_back = 1;
        for (int i = 0; i < 4; i++) { char up[32]; upcat(up, mg.moves[i].name); add(m, up, MA_MOVE0 + i, mg.moves[i].pp <= 0); }
        move_t *mv = &mg.moves[mg.mcur < 4 ? mg.mcur : 0]; char *d = str_cat(m->info[0], "PP "); d = num(d, mv->pp); d = str_cat(d, "/"); *num(d, mv->maxPp) = 0;
        d = str_cat(m->info[1], mv->tier); d = str_cat(d, " · "); *num(d, mv->power) = 0; } break;
    case MN_CHECK: m->cls = CL_LOW;
        if (mg.kind == K_DOCS) { add(m, "APROBAR", MA_DEC0, 0); add(m, "RECHAZAR", MA_DEC1, 0); add(m, "MOCHILA", MA_OPEN_BAG, 0); }
        else if (mg.kind == K_EPIS) { for (int i = 0; i < 4; i++) add(m, cat("DAR ", EPI_UP[i], 0), MA_DEC0 + i, 0); }
        else { add(m, "VALIDAR CITA", MA_DEC0, 0); add(m, "UNA POR PERSONA", MA_DEC1, 0); add(m, "SIN CITA, FUERA", MA_DEC2, 0); add(m, "RELLENAR HUECO", MA_DEC3, 0); }
        break;
    case MN_BAG: m->cls = battle ? CL_LOW : CL_SIDE; m->has_back = 1;
        add(m, cat("MONSTER ×", itos(mg.monster), 0), MA_MONSTER, 0);
        if (mg.kind == K_HIRE) add(m, "CONTRATO ×∞", MA_CONTRACT, 0);
        add(m, battle ? "VOLVER" : "CERRAR", MA_BACK, 0); break;
    case MN_PARTY: m->cls = CL_FULL; m->has_back = 1;
        add(m, cat(HERO(), " · PS ", itos(mg.hp), "/", itos(mg.maxHp), 0), MA_NONE, 0);
        for (int i = 0; i < mg.nteam; i++) { char up[16]; upcat(up, RPG_NPCS[mg.team[i]].name); add(m, cat(up, " · ", RPG_NPCS[mg.team[i]].role, 0), MA_NONE, 0); }
        if (!mg.nteam) add(m, "(aún sin riders)", MA_NONE, 1);
        add(m, "VOLVER", MA_BACK, 0); break;
    case MN_START: m->cls = CL_SIDE; m->has_back = 1;
        add(m, "MISIÓN", MA_MISSION, 0); add(m, "EQUIPO", MA_OPEN_PARTY, 0); add(m, "MOCHILA", MA_OPEN_BAG, 0);
        add(m, cat("PALETA ", GB_PAL_N[rpg_pal], 0), MA_PAL, 0); add(m, "SALIR", MA_QUIT, 0); add(m, "CERRAR", MA_CLOSE, 0); break;
    }
}
static int quit_req; static void dark_lut(void);
static void menu_back(void) {
    int battle = mg.view == V_BATTLE;
    if (mg.menu == MN_START) { rpg_close(); return; }
    if (mg.menu == MN_FIGHT) { rpg_open(MN_ROOT); return; }
    if (battle) rpg_open(mg.has_e && mg.E.check ? MN_CHECK : MN_ROOT); else rpg_open(MN_START);
}
static void menu_act(int a) {
    int battle = mg.view == V_BATTLE;
    switch (a) {
    case MA_OPEN_FIGHT: rpg_open(MN_FIGHT); break;
    case MA_OPEN_PARTY: rpg_open(MN_PARTY); break;
    case MA_OPEN_BAG: rpg_open(MN_BAG); break;
    case MA_FLEE: rpg_flee(); break;
    case MA_MOVE0: case MA_MOVE1: case MA_MOVE2: case MA_MOVE3: rpg_use_move(a - MA_MOVE0); break;
    case MA_DEC0: case MA_DEC1: case MA_DEC2: case MA_DEC3: rpg_decide(a - MA_DEC0); break;
    case MA_MONSTER: rpg_monster(); break;
    case MA_CONTRACT:
        if (battle && !(mg.has_e && mg.E.check)) rpg_contract();
        else { rpg_close(); Qsay("Úsalo en un combate cuando el rider esté convencido."); Qrun(f_open, battle ? MN_CHECK : MN_BAG); }
        break;
    case MA_BACK: menu_back(); break;
    case MA_MISSION: { rpg_close(); const char *l1 = LABELS[mg.kind][0], *l2 = LABELS[mg.kind][1];
        Qsay(mg.M->title); Qsay(mg.M->rules[0]); Qsay(mg.M->rules[1]);
        Qsay(cat(l1, ": ", itos(mg.c1), " · ", l2, ": ", itos(mg.c2), "\nQuedan: ", itos(remaining()), 0)); } break;
    case MA_PAL: rpg_pal = (rpg_pal + 1) % 3; dark_lut(); break;
    case MA_QUIT: quit_req = 1; break;
    case MA_CLOSE: rpg_close(); break;
    }
}
enum { BT_A, BT_B, BT_START, BT_UP, BT_DOWN, BT_LEFT, BT_RIGHT };
static int rpg_press(int b) { /* devuelve 1 si el web no hace nada con START (→ menú de pausa) */
    if (qn) { qi_t *s = &QB[qh]; if (s->k == QK_TEXT && (b == BT_A || b == BT_B)) { if (s->shown < s->len) s->shown = s->len; else qpop(); } return b == BT_START; }
    if (mg.lock) return b == BT_START;
    if (mg.menu) { menu_t M; menu_def(&M); int n = M.n, prev = mg.mcur, ign = 0;
        if (b == BT_UP) mg.mcur = M.grid == 2 ? mg.mcur ^ 2 : (mg.mcur + n - 1) % n;
        else if (b == BT_DOWN) mg.mcur = M.grid == 2 ? mg.mcur ^ 2 : (mg.mcur + 1) % n;
        else if ((b == BT_LEFT || b == BT_RIGHT) && M.grid == 2) mg.mcur ^= 1;
        else if (b == BT_A) { mi_t *it = &M.it[mg.mcur]; if (it->act != MA_NONE && !it->dis) { sfx_select(); menu_act(it->act); } else sfx_bump(); }
        else if (b == BT_B || b == BT_START) { if (M.has_back) { sfx_back(); menu_back(); } else ign = b == BT_START; }
        if (prev != mg.mcur && mg.menu) sfx_tick();
        return ign; }
    if (mg.view != V_MAP) return b == BT_START;
    if (b == BT_A) rpg_interact(); else if (b == BT_START) { sfx_open(); rpg_open(MN_START); }
    return 0;
}

/* ---------------------------------------------------------------- bucle de la cola */
static void step_queue(void) {
    for (int guard = 0; qn && guard < 60 && !mg.ended; guard++) {
        qi_t *s = &QB[qh];
        if (s->k == QK_FN) { void (*f)(int) = s->f; int a = s->arg; qpop(); f(a); continue; }
        if (s->k == QK_TEXT) { if (!s->on) s->on = 1; else if (s->shown < s->len) s->shown++; return; }
        if (s->k == QK_ANIM) { if (!s->on) { s->on = 1; mg.anim.on = 1; mg.anim.a = s->arg; mg.anim.t = 0; mg.anim.n = s->n; return; }
            if (++mg.anim.t >= s->n) { mg.anim.on = 0; qpop(); continue; } return; }
        if (s->k == QK_DRAIN) { int busy = 0;
            if (mg.has_e && mg.E.disp != mg.E.hp) { mg.E.disp += mg.E.hp > mg.E.disp ? 1 : -1; busy = 1; }
            if (mg.disp != mg.hp) { mg.disp += mg.hp > mg.disp ? 1 : -1; busy = 1; }
            if (busy) return; qpop(); continue; }
    }
}
static void rpg_tick(void) {
    mg.t++; step_queue(); if (mg.ended) return;
    if (mg.win_at >= 0 && (int)mg.t >= mg.win_at) { mg.win_at = -1; sfx_win(); }
    if (mg.view == V_MAP) rpg_map_update();
}

/* ---------------------------------------------------------------- paleta actual (rpgPalNow) */
static void apply_pal(void) {
    const u32 *G = GB_PALS[rpg_pal]; u32 P[4];
    for (int i = 0; i < 4; i++) P[i] = G[i];
    if (mg.anim.on) { int a = mg.anim.a, t = mg.anim.t, n = mg.anim.n;
        if (a == AN_FLASH) { if ((t >> 2) & 1) for (int i = 0; i < 4; i++) P[i] = G[3 - i]; }
        else { int lv = a == AN_FADEOUT ? imin(3, t * 4 / n) : a == AN_FADEIN ? imin(3, (n - t) * 4 / n) : 0;
            for (int i = 0; i < 4; i++) P[i] = G[imax(0, i - lv)]; } }
    rpg_tones_set(P);
    volatile u16 *pr = PALRAM; for (int i = 0; i < 4; i++) { u32 v = G[i]; pr[UN[i]] = ((v >> 19) & 31) | (((v >> 11) & 31) << 5) | (((v >> 3) & 31) << 10); }
}

/* ---------------------------------------------------------------- dibujo */
static void lerp(mob_t *o, int *x, int *y) { int k = o->moving ? o->t : 16;
    *x = o->fx * 16 + (o->x - o->fx) * k; *y = o->fy * 16 + (o->y - o->fy) * k; }
static int stride_of(mob_t *o) { return o->moving && o->t < 8 ? ((o->step & 1) ? 1 : -1) : 0; }
static int view_of(int d) { return d == D_RIGHT ? D_DOWN : d; }
typedef struct { int y, kind, idx; } ent_t;
static void draw_map(void) {
    map_restore(); u32 tm = ticks();
    int ppx, ppy; lerp(&mg.p, &ppx, &ppy);
    int cx = iclamp(ppx + 8 - GBW / 2, 0, MW * 16 - GBW), cy = iclamp(ppy + 8 - GBH / 2, 0, MH * 16 - GBH);
    static int last_wv = -1, last_tpl = -1; int wv = (frame_no >> 4) & 3;
    if (wv != last_wv || last_tpl != mg.tpl) { rpg_map_anim(mg.tpl, wv); last_wv = wv; last_tpl = mg.tpl; }
    PROF(3, rpg_blit_map(cx, cy));
    ent_t E[16]; int ne = 0;
    for (int i = 0; i < mg.nnpc; i++) { npc_t *n = &mg.npcs[i]; if (!active(n)) continue; int x, y; lerp(&n->m, &x, &y); E[ne].y = y; E[ne].kind = 2; E[ne].idx = i; ne++; }
    E[ne].y = ppy; E[ne].kind = 0; E[ne].idx = 0; ne++;
    if (mg.has_dog) { int x, y; lerp(&mg.dog, &x, &y); E[ne].y = y - 1; E[ne].kind = 1; E[ne].idx = 0; ne++; }
    for (int a = 1; a < ne; a++) { ent_t v = E[a]; int b = a - 1; while (b >= 0 && E[b].y > v.y) { E[b + 1] = E[b]; b--; } E[b + 1] = v; }
    for (int k = 0; k < ne; k++) {
        mob_t *o; int x, y;
        if (E[k].kind == 0) { o = &mg.p; x = ppx; y = ppy; rpg_blit(spr_crew(mg.ci, view_of(o->dir), stride_of(o)), x - cx, y - 8 - cy); }
        else if (E[k].kind == 1) { o = &mg.dog; lerp(o, &x, &y); rpg_blit(spr_dog((o->moving && o->t < 8) ? 1 : 0, o->flip), x - 6 - cx, y - 2 - cy); }
        else { npc_t *n = &mg.npcs[E[k].idx]; o = &n->m; lerp(o, &x, &y);
            if (n->boss) { bmp_t *s = spr_boss(mg.M->boss->spr, 0); if (s) rpg_blit(s, x - cx - (s->w - 16) / 2, y - 12 - cy); }
            else rpg_blit(spr_npc(n->look, view_of(o->dir), stride_of(o), mg.kind == K_HIRE && HIRE_D[n->uid].red), x - cx, y - 8 - cy); }
        if (GRID[mg.tpl][o->y][o->x] == ',' && !o->moving) blit_part(MAPB, o->x * 16, o->y * 16 + 10, 16, 6, o->x * 16 - cx, o->y * 16 - cy + 10, 0);
    }
    for (int i = 0; i < mg.nnpc; i++) { npc_t *n = &mg.npcs[i]; if (!active(n) || n->state != ST_ALERT) continue; int x, y; lerp(&n->m, &x, &y); int bx = x - cx + 3, by = y - cy - 22;
        rect(bx, by, 10, 12, G3); rect(bx + 1, by + 1, 8, 10, G0); rect(bx + 4, by + 2, 2, 5, G3); rect(bx + 4, by + 8, 2, 2, G3); }
    tm = ticks() - tm; PROFT(6, tm);
}
static int rnd_i(float v) { return (int)(v < 0 ? v - .5f : v + .5f); }
static void draw_enemy(int ex, int ey, int dy) {
    enemy_t *E = &mg.E; npc_t *n = E->n >= 0 ? &mg.npcs[E->n] : 0;
    switch (E->spr) {
    case SP_NPC: rpg_blit(spr_scaled(spr_npc(n->look, D_DOWN, 0, mg.kind == K_HIRE && HIRE_D[n->uid].red), 0x100000 | (n->look << 4) | (mg.kind == K_HIRE && HIRE_D[n->uid].red), 32, 48), ex, ey + dy); break;
    case SP_PERSON: { int g = 15; for (int j = 0; j < n->nneed; j++) g &= ~(1 << n->need[j]);
        rpg_blit(spr_scaled(spr_person(n->look, g), 0x200000 | (n->look << 4) | g, 80, 56), ex - 40, ey - 6 + dy); } break;
    case SP_BAGS: draw_bags(ex - 42, ey + 22 + dy, E->dizzy, imin(14, (E->maxHp - E->hp) * 14 / E->maxHp)); break;
    default: { int k = E->spr - SP_BOSS0; bmp_t *b = spr_boss(k, E->good);
        if (k == 2) rpg_blit(spr_scaled(b, 0x300000 | (k << 4) | E->good, 54, 42), ex - 20, ey + 6 + dy);
        else rpg_blit(spr_scaled(b, 0x300000 | (k << 4) | E->good, 36, 56), ex - 2, ey - 8 + dy); } }
}
static void draw_battle(void) {
    enemy_t *E = &mg.E; if (!mg.has_e) return;
    int on = mg.anim.on, a = on ? mg.anim.a : -1, t = mg.anim.t; float k = on ? (float)t / mg.anim.n : 1.f, sk = fsin(k * 3.14159265f);
    rect(0, 0, GBW, GBH, G0);
    int ex = 108, ey = 6, hx = 8, hy = 42;
    if (a == AN_SLIDE) { ex -= rnd_i((1 - k) * 130); hx += rnd_i((1 - k) * 130); }
    if (a == AN_HEROATK) hx += rnd_i(sk * 8);
    if (a == AN_ENEMYATK) ex -= rnd_i(sk * 8);
    if (a == AN_ENEMYDODGE) ex += rnd_i(sk * 18);
    if (a == AN_HERODODGE) hx -= rnd_i(sk * 14);
    if (a == AN_HEROHIT) hx += (t >> 1) & 1 ? 2 : -2;
    px_ellipse(ex + 16, ey + 48, 30, 5, G1); px_ellipse(hx + 26, 100, 40, 8, G1);
    u32 tp = ticks();
    if (!E->hidden && !(a == AN_ENEMYHIT && (t >> 2) & 1)) {
        if (a == AN_FAINT) { rpg_clip(GBX, GBY, GBW, ey + 50); draw_enemy(ex, ey, rnd_i(k * 50)); rpg_clip(GBX, GBY, GBW, GBH); }
        else draw_enemy(ex, ey, 0); }
    if (E->check && mg.kind == K_DOCS && !E->hidden) draw_doccard(ex - 26, ey + 26);
    if (!(a == AN_HEROHIT && (t >> 2) & 1)) {
        if (mg.armor) rpg_blit(spr_scaled(spr_armor(), 0x400000, 48, 72), hx, hy);
        else rpg_blit(spr_scaled(spr_crew(mg.ci, D_UP, 0), 0x500000 | mg.ci, 48, 72), hx, hy); }
    if (mg.ci == 0) rpg_blit(spr_dog(0, 0), hx + 46, 78);
    tp = ticks() - tp; PROFT(4, tp);
    if (a == AN_THROW) { float x = hx + 40 + (ex + 12 - hx - 40) * k, y = 70 + (ey + 16 - 70) * k - sk * 34; draw_contract_icon(rnd_i(x), rnd_i(y)); }
    if (a == AN_SIGN) draw_contract(ex + 2, ey + 4, rnd_i(k * 18), 0, 0);
    if (a == AN_STAMP || (E->sig && a != AN_TEAR)) draw_contract(ex + 2, ey + 4, 18, 1, a == AN_STAMP ? imax(5, rnd_i(5 + (1 - k) * 8)) : 5);
    if (a == AN_TEAR) { int off = rnd_i(k * 20), dy = rnd_i(k * k * 30);
        rpg_clip(GBX, GBY, ex + 16, GBH); draw_contract(ex + 2 - off, ey + 4 + dy, 18, 0, 0);
        rpg_clip(GBX + ex + 16, GBY, GBW - ex - 16, GBH); draw_contract(ex + 2 + off, ey + 4 + dy, 18, 0, 0); rpg_clip(GBX, GBY, GBW, GBH); }
    if (a == AN_COIN) { float q = k * 1.4f - .4f; if (q < 0) q = 0; float r = k * 2.5f; if (r > 1) r = 1;
        draw_coin(rnd_i(ex + 16 + (hx + 24 - ex - 16) * q), rnd_i(ey + 20 - r * 16 + q * 40), t); }
    if (a == AN_COINS) { int ccx = hx + 24, ccy = 66; for (int i = 0; i < 3; i++) { float ang = k * 10 + i * 2.09f, r = (1 - k) * 34 + 4;
        draw_coin(rnd_i(ccx + fcos(ang) * r), rnd_i(ccy + fsin(ang) * r * .6f), t + i * 4); } }
}

/* En combate, sin animación, la escena no cambia: se dibuja una vez en MAPB (el mapa se rehace al volver) y se copia. */
static void draw_battle_cached(void) {
    enemy_t *E = &mg.E; if (!mg.has_e) return;
    map_dirty = 1;
    if (mg.anim.on) { scene_key = 0; draw_battle(); return; }
    int need = 0; if (E->n >= 0) for (int j = 0; j < mg.npcs[E->n].nneed; j++) need |= 1 << mg.npcs[E->n].need[j];
    u32 key = 0x80000000u | (E->spr << 0) | (E->hidden << 3) | (E->good << 4) | (E->sig << 5) | (E->dizzy << 6) | (mg.armor << 7) | ((E->n & 15) << 8) | (need << 12) | (mg.ci << 16)
        | ((E->spr == SP_BAGS ? imin(14, (E->maxHp - E->hp) * 14 / E->maxHp) : 0) << 18) | (E->check << 23);
    if (key != scene_key) { MAPB->w = GBW; tgt_bmp(MAPB); rpg_clip(0, 0, GBW, GBH); draw_battle(); tgt_back(); RC[0] = GBX; RC[1] = GBY; RC[2] = GBX + GBW; RC[3] = GBY + GBH; scene_key = key; }
    rpg_blit_win(MAPB, 0, 0);
}
static void map_restore(void) { if (map_dirty) { MAPB->w = MW * 16; rpg_map_build(mg.tpl); map_dirty = 0; scene_key = 0; } }

/* ---------------------------------------------------------------- interfaz (rpgUI / rpgLive) */
static void gbx(int x, int y, int w, int h) { rect(x, y, w, h, UN[3]); rect(x + 1, y + 1, w - 2, h - 2, UN[0]); frame_rect(x + 2, y + 2, w - 4, h - 4, UN[3]); }
static void cursor(int x, int y) { static const u8 W[7] = {1, 2, 3, 4, 3, 2, 1}; for (int i = 0; i < 7; i++) rect(x, y + i, W[i], 1, UN[3]); }
static void text_n(int x, int y, const char *s, int n, u8 col) { /* primeras n letras, con saltos de línea */
    char buf[110]; int line = 0;
    while (*s && n > 0) { char *d = buf; while (*s && *s != '\n' && n > 0) { const char *p = s; u8dec(&s); memcpy(d, p, s - p); d += s - p; n--; } *d = 0;
        text(x, y + line * 13, buf, col, 0); if (*s == '\n' && n > 0) { s++; n--; line++; } else break; }
}
static int fit(char *d, const char *s, int max) { /* corta con … (text-overflow: ellipsis) */
    int n = text_len(s); if (n <= max) { strcpy(d, s); return n; }
    char *o = d; for (int i = 0; i < max - 1; i++) { const char *p = s; u8dec(&s); memcpy(o, p, s - p); o += s - p; } o = u8enc(o, 0x2026); *o = 0; return max;
}
static void hpbar(int x, int y, int w, int disp, int maxHp) {
    text_small(x, y, "PS", UN[3], 0);
    int bx = x + 10, bw = w - 10; rect(bx, y, bw, 5, UN[3]); rect(bx + 1, y + 1, bw - 2, 3, UN[0]);
    int fw = (bw - 2) * disp / maxHp; int low = disp * 4 < maxHp;
    if (!low || (frame_no % 30) < 15) rect(bx + 1, y + 1, fw, 3, low ? UN[3] : UN[2]);
}
static void draw_ui(void) {
    enemy_t *E = &mg.E; int battle = mg.view == V_BATTLE && mg.has_e && E->boxes;
    if (battle) {
        char info[64] = "";
        if (E->check) { npc_t *n = &mg.npcs[E->n];
            if (mg.kind == K_DOCS) { char *d = str_cat(info, "DOC: "); upcat(d, DOCS_D[n->uid].doc); }
            else if (mg.kind == K_EPIS) { char *d = str_cat(info, "FALTA:"); for (int j = 0; j < n->nneed; j++) { d = str_cat(d, " "); d = str_cat(d, EPI_UP[n->need[j]]); } }
            else upcat(info, RPG_NPCS[n->look].role); }
        int lines = 0; static char wl[6][16];
        if (E->check) { /* ajuste a 11 columnas */
            const char *s = info; while (*s && lines < 6) { while (*s == ' ') s++; const char *st = s, *brk = 0; int c = 0;
                while (*s && c < 11) { if (*s == ' ') brk = s; u8dec(&s); c++; }
                if (*s && *s != ' ' && brk) s = brk;
                int len = s - st; if (len > 15) len = 15; memcpy(wl[lines], st, len); wl[lines][len] = 0; lines++; } }
        int eh = E->check ? 18 + lines * 9 : 29;
        gbx(3, 3, 100, eh);
        char nm[20]; fit(nm, E->name, (E->check || E->boss) ? 11 : 8); text(8, 8, nm, UN[3], 0);
        if (!E->check && !E->boss) { char lv[8] = "Nv"; *num(lv + 2, E->lv) = 0; text(98, 8, lv, UN[3], 2); }
        if (E->check) for (int i = 0; i < lines; i++) text(8, 18 + i * 9, wl[i], UN[3], 0);
        else hpbar(8, 19, 90, E->disp, E->maxHp);
        gbx(70, 56, 88, 38);
        text(75, 61, HERO(), UN[3], 0); text(153, 61, mg.fin ? "Nv99" : "Nv1", UN[3], 2);
        hpbar(75, 72, 78, mg.disp, mg.maxHp);
        char hp[12]; char *d = hp; if (mg.disp < 100) *d++ = ' '; if (mg.disp < 10) *d++ = ' '; d = num(d, mg.disp); *d++ = '/'; *num(d, mg.maxHp) = 0;
        text(153, 82, hp, UN[3], 2);
    }
    menu_t M; int hasM = mg.menu != MN_NONE; if (hasM) menu_def(&M);
    if (mg.view == V_MAP && mg.banner > 0 && !mg.anim.on && !hasM) {
        const char *zn = mg.M->zname[mg.zi]; int w = text_len(zn) * 8 + 10, x = iclamp(160 - w, 0, 3);
        gbx(x, 3, w, 15); text(x + 5, 7, zn, UN[3], 0); }
    qi_t *s = qn ? &QB[qh] : 0; int textNow = s && s->k == QK_TEXT;
    if (textNow || (hasM && M.prompt) || battle) {
        gbx(0, 96, 160, 48);
        if (textNow) { if (s->on) text_n(8, 105, s->t, s->shown, UN[3]);
            if (s->shown >= s->len && (frame_no % 33) < 17) { for (int i = 0; i < 4; i++) rect(149 + i, 135 + i, 7 - 2 * i, 1, UN[3]); } }
        else if (hasM && M.prompt) text_n(6, 105, M.prompt, 99, UN[3]);
    }
    if (hasM && !textNow) {
        if (mg.menu == MN_FIGHT) { gbx(0, 52, 112, 32); text(7, 58, M.info[0], UN[3], 0); text(7, 70, M.info[1], UN[3], 0); }
        if (M.cls == CL_ROOT) { gbx(46, 96, 114, 48);
            for (int i = 0; i < M.n; i++) { int cx = (i & 1) ? 105 : 50, cy = (i & 2) ? 124 : 107; if (i == mg.mcur) cursor(cx, cy);
                const char *p = M.it[i].t; int x = cx + 5; while (*p) { char ch[2] = {*p++, 0}; text(x, cy, ch, M.it[i].dis ? UN[1] : UN[3], 0); x += 7; } } }
        else if (M.cls == CL_LOW) { gbx(0, 84, 160, 60); int y0 = 84 + (60 - M.n * 13) / 2 + 3;
            for (int i = 0; i < M.n; i++) { int y = y0 + i * 13; if (i == mg.mcur) cursor(6, y); char b[44]; fit(b, M.it[i].t, 18); text(12, y, b, M.it[i].dis ? UN[1] : UN[3], 0); } }
        else if (M.cls == CL_SIDE) { int h = imax(43, M.n * 12 + 10); gbx(40, 0, 120, h); int y0 = (h - M.n * 12) / 2 + 2;
            for (int i = 0; i < M.n; i++) { int y = y0 + i * 12; if (i == mg.mcur) cursor(45, y); char b[44]; fit(b, M.it[i].t, 13); text(51, y, b, M.it[i].dis ? UN[1] : UN[3], 0); } }
        else { gbx(0, 0, 160, 96);
            for (int i = 0; i < M.n; i++) { int y = 7 + i * 12; if (i == mg.mcur) cursor(6, y); char b[44]; fit(b, M.it[i].t, 18); text(12, y, b, M.it[i].dis ? UN[1] : UN[3], 0); } }
    }
}
static int bezel_n;     /* el marco solo se dibuja en las dos páginas tras empezar o tras la pausa */
static void draw_all(void) {
    u32 t0 = ticks();
    rpg_cache_frame();
    origin(0, 0); clip_reset();
    u32 tb = ticks();
    if (bezel_n > 0) { bezel_n--;
    rect(0, 0, GBX, SH, C(0x57566a)); rect(GBX + GBW, 0, SW - GBX - GBW, SH, C(0x57566a)); rect(GBX, 0, GBW, GBY, C(0x57566a)); rect(GBX, GBY + GBH, GBW, SH - GBY - GBH, C(0x57566a));
    frame_rect(GBX - 1, GBY - 1, GBW + 2, GBH + 2, C(0x2a2a33)); frame_rect(GBX - 2, GBY - 2, GBW + 4, GBH + 4, C(0x2a2a33)); }
    tb = ticks() - tb; PROFT(7, tb);
    origin(GBX, GBY); rpg_clip(GBX, GBY, GBW, GBH);
    PROF(1, if (mg.view == V_BATTLE) draw_battle_cached(); else draw_map());
    if (mg.anim.on && mg.anim.a == AN_WIPE) { int h = mg.anim.t * 72 / mg.anim.n; rect(0, 0, GBW, h, G3); rect(0, GBH - h, GBW, h, G3); }
    PROF(2, draw_ui());
    origin(0, 0); clip_reset();
    t0 = ticks() - t0; PROFT(0, t0);
}

/* ---------------------------------------------------------------- entrada principal */
s32 RPGD[80];
static void rpg_dbg(void) {
    s32 *d = RPGD; qi_t *s = qn ? &QB[qh] : 0;
    d[0] = mg.view; d[1] = mg.menu; d[2] = mg.mcur; d[3] = qn; d[4] = s ? s->k : -1; d[5] = mg.zi; d[6] = mg.p.x; d[7] = mg.p.y; d[8] = mg.p.dir; d[9] = mg.lock;
    d[10] = mg.hp; d[11] = mg.has_e ? mg.E.hp : -1; d[12] = mg.E.maxHp; d[13] = mg.E.check; d[14] = mg.E.boss; d[15] = mg.kind; d[16] = remaining(); d[17] = mg.nnpc;
    d[18] = mg.tpl; d[19] = mg.ended; d[20] = mg.fin; d[21] = mg.c1; d[22] = mg.c2; d[23] = mg.has_e && mg.E.n >= 0 ? mg.npcs[mg.E.n].uid : -1;
    d[24] = mg.has_e && mg.E.n >= 0 && mg.npcs[mg.E.n].nneed ? mg.npcs[mg.E.n].need[0] : -1; d[25] = mg.p.moving; d[26] = mg.docs; d[27] = mg.M->nz; d[28] = mg.E.boxes; d[29] = mg.anim.on;
    for (int i = 0; i < 10; i++) { npc_t *n = &mg.npcs[i]; d[30 + i * 4] = n->m.x; d[31 + i * 4] = n->m.y; d[32 + i * 4] = (i < mg.nnpc) | (n->boss << 1) | (n->done << 2) | (n->zone << 3) | (n->m.moving << 4) | (n->state << 5); d[33 + i * 4] = n->uid; }
    for (int i = 0; i < 4; i++) d[70 + i] = mg.moves[i].pp;
    d[74] = mg.monster; d[75] = mg.hp; d[76] = mg.maxHp;
}
static void finish(void) {
    rpg_tones_restore(); for (int i = 0; i < 4; i++) PALRAM[UN[i]] = PAL15[UN[i]];
}

static int run_loop(void) { /* 1 = misión superada, 0 = salir */
    bezel_n = 2;
    for (;;) {
        draw_all();
        int n = frame_end(); if (n > rpg_maxn) rpg_maxn = n; rpg_nhist[n & 3]++;
        apply_pal();
        rpg_dbg();
        if (key_hit & K_SEL) { if (pause_menu()) return 0; bezel_n = 2; continue; }
        static const u16 KB[7] = {K_A, K_B, K_START, K_UP, K_DOWN, K_LEFT, K_RIGHT};
        for (int i = 0; i < 7; i++) if (key_hit & KB[i]) { if (rpg_press(i)) { if (pause_menu()) return 0; bezel_n = 2; } }
        if (quit_req) return 0;
        PROF(5, for (int i = 0; i < n && !mg.ended; i++) rpg_tick());
        if (mg.ended) return 1;
    }
}
static void dark_lut(void) { /* el menú de pausa oscurece con LUT_DARK: los tonos se oscurecen al color de la paleta más parecido */
    for (int i = 0; i < 4; i++) { u32 v = GB_PALS[rpg_pal][i]; u8 d = col_rgb(((v >> 16) & 255) / 2, ((v >> 8) & 255) / 2, (v & 255) / 2); LUT_DARK[TN[i]] = d; LUT_DARK[UN[i]] = d; }
}
int rpg_play(int ci) {
    u32 mark = pool_mark();
    MAPB = bmp_new(MW * 16, MH * 16); map_dirty = 0; scene_key = 0;
    if (!MAPB) return 0;   /* sin memoria (no debería pasar: el modo usa ~150 KB del pool) */
    rpg_tones_init(); rpg_tpl_init();
    if (PAL_N <= 248) for (int i = 0; i < 4; i++) UN[i] = 248 + i; else for (int i = 0; i < 4; i++) UN[i] = TN[i];
    u8 sv[8]; for (int i = 0; i < 4; i++) { sv[i] = LUT_DARK[TN[i]]; sv[4 + i] = LUT_DARK[UN[i]]; }
    dark_lut();
    rpg_atlas_init();
    quit_req = 0;
    rpg_cache_reset();
    rpg_start(ci); apply_pal();
    int r = run_loop();
    if (r) { RES_C1 = mg.c1; RES_C2 = mg.c2; RES_L1 = LABELS[mg.kind][0]; RES_L2 = LABELS[mg.kind][1]; }
    if (r && ci == 1) { mg.ended = 0; rpg_start_final(); r = run_loop(); }
    finish();
    for (int i = 0; i < 4; i++) { LUT_DARK[TN[i]] = sv[i]; LUT_DARK[UN[i]] = sv[4 + i]; }
    pool_reset(mark); people_cache_reset();
    extern void rpg_cache_forget(void); rpg_cache_forget();
    return r;
}
