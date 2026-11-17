# Genera game/people_data.c y game/people_data.h desde tools/people.json (datos del juego web).
import json, sys, os
D = json.load(open(sys.argv[1])); OUT = sys.argv[2]
c = ['#include "people.h"\n']; h = ['#ifndef PEOPLE_DATA_H\n#define PEOPLE_DATA_H\n#include "eng.h"\n']
def hx(s):
    s = s.lstrip('#')
    if len(s) == 3: s = ''.join(ch*2 for ch in s)
    return 'C(0x%s)' % s.lower()
def spr(name, rows, pad=None):
    if pad: rows = [r.ljust(pad, '.')[:pad] for r in rows]
    c.append('const char *const %s[%d] = {%s};\n' % (name, len(rows), ','.join('"%s"' % r for r in rows)))
    h.append('extern const char *const %s[%d];\n#define %s_H %d\n#define %s_W %d\n' % (name, len(rows), name, len(rows), name, max(len(r) for r in rows)))
def pal(name, d):
    ks = [k for k, v in d.items() if isinstance(v, str) and v.startswith('#')]
    c.append('const char %s_CH[] = "%s";\nconst u8 %s_COL[] = {%s};\n' % (name, ''.join(ks), name, ','.join(hx(d[k]) for k in ks)))
    h.append('extern const char %s_CH[];\nextern const u8 %s_COL[];\n' % (name, name))
pal('PAL', D['PAL'])
for i, cr in enumerate(D['CREW']): spr('CREW%d' % i, cr['spr'], 16)
for i, f in enumerate(D['DOG']): spr('DOG%d' % i, f, 28)
pal('DOGPAL', D['DOGPAL']); spr('MOTO', D['MOTO']); spr('BOSS', D['BOSS']); spr('CAND_T', D['CAND_T'])
spr('MARTI', D['MARTI']); spr('MARTI_B', D['MARTI_B']); spr('FRANCO', D['FRANCO'])
for n in ['FPAL', 'MPAL', 'MPAL_I', 'MPAL_A', 'EVIL', 'ARMOR', 'IPAL']: pal(n, D[n])
for k, v in D['ITEM'].items(): spr('ITEM_' + k.upper(), v)
c.append('const look_t LOOKS[6] = {%s};\n' % ','.join('{%s,%s,%s,%d,%s,%s,%s}' % (hx(l['H']), hx(l['S']), hx(l['s']), l['beard'], hx(l['C']), hx(l['c']), hx(l['P'])) for l in D['LOOKS']))
c.append('const crew_t CREW[4] = {%s};\n' % ','.join('{CREW%d,\'%s\',\'%s\',\'%s\',%d,%d,"%s"}' % (i, cr['pants'], cr['sleeve'], cr['skin'], cr['helm'], 1 if cr.get('dog') else 0, cr['name']) for i, cr in enumerate(D['CREW'])))
h.append('#endif\n')
open(os.path.join(OUT, 'people_data.c'), 'w').write(''.join(c)); open(os.path.join(OUT, 'people_data.h'), 'w').write(''.join(h))
