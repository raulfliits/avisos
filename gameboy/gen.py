# Genera assets.c / assets.h para la ROM de Game Boy de FLiiTS Catalunya World.
# Uso: python3 -I gen.py <dir_gb>
import sys, os
from PIL import Image, ImageFont, ImageDraw

GB = sys.argv[1]
OUT = os.path.join(GB, 'build'); os.makedirs(OUT, exist_ok=True)
FONT = ImageFont.truetype(os.path.join(GB, 'dl/ps2p.ttf'), 8)
FONT16 = ImageFont.truetype(os.path.join(GB, 'dl/ps2p.ttf'), 16)

def tile_bytes(px):  # px: 64 valores 0..3
    out = []
    for y in range(8):
        lo = hi = 0
        for x in range(8):
            v = px[y*8+x]
            lo |= (v & 1) << (7-x); hi |= ((v >> 1) & 1) << (7-x)
        out += [lo, hi]
    return out

def img_tiles(im, x0, y0, w, h):  # im: 'L' con valores 0..3
    res = []
    for ty in range(h):
        for tx in range(w):
            res.append(tuple(im.getpixel((x0+tx*8+x, y0+ty*8+y)) for y in range(8) for x in range(8)))
    return res

# ---------------------------------------------------------------- fuente y tiles comunes
# Códigos propios para caracteres no ASCII (el preprocesador de main.c los traduce).
EXTRA = {'Á':0x80,'É':0x81,'Í':0x82,'Ó':0x83,'Ú':0x84,'Ñ':0x85,'Ü':0x86,'¡':0x87,'¿':0x88,'·':0x89,'À':0x8A,'È':0x8B,'Ò':0x8C,'Ç':0x8D,'Ï':0x8E,
         '♥':0x90,'♡':0x91,'●':0x92,'▶':0x93,'▼':0x94,'✓':0x95,'×':0x96,'«':0x97,'»':0x98,'ı':0x99}
LOWER = {'á':'Á','é':'É','í':'Í','ó':'Ó','ú':'Ú','ñ':'Ñ','ü':'Ü','à':'À','è':'È','ò':'Ò','ç':'Ç','ï':'Ï'}
glyphs = [' '] + [chr(c) for c in range(ord('A'), ord('Z')+1)] + [chr(c) for c in range(ord('0'), ord('9')+1)] + \
         list(".,!?:-'/%+()\"#=") + list('ÁÉÍÓÚÑÜ¡¿·ÀÈÒÇÏ')
tiles = []; lut = [0]*256

def glyph_px(ch):
    im = Image.new('L', (8, 8), 0); ImageDraw.Draw(im).text((0, 0), ch, font=FONT, fill=255)
    return tuple(3 if im.getpixel((x, y)) > 128 else 0 for y in range(8) for x in range(8))

ACC = {'Á': ('A', '....##..'), 'É': ('E', '....##..'), 'Í': ('I', '....##..'), 'Ó': ('O', '....##..'), 'Ú': ('U', '....##..'),
       'À': ('A', '..##....'), 'È': ('E', '..##....'), 'Ò': ('O', '..##....'), 'Ñ': ('N', '.##.##..'), 'Ü': ('U', '.##.##..'), 'Ï': ('I', '.##.##..')}
def accent_px(ch):
    base, acc = ACC[ch]; px = glyph_px(base); rows = [px[r*8:(r+1)*8] for r in range(8)]
    letter = rows[:7]
    for _ in range(2):
        n = len(letter)
        drop = next((i for i in (3, 2, 4, 1, 5, 6) if i < n - 1 and (letter[i] == letter[i-1] or letter[i] == letter[i+1])), n // 2)
        letter = letter[:drop] + letter[drop+1:]
    acc_row = tuple(3 if c == '#' else 0 for c in acc)
    return tuple(v for r in [acc_row, rows[7]] + letter + [rows[7]] for v in r)
for ch in glyphs:
    idx = len(tiles); tiles.append(accent_px(ch) if ch in ACC else glyph_px(ch))
    code = EXTRA.get(ch, ord(ch)); lut[code] = idx
    if 'A' <= ch <= 'Z': lut[ord(ch.lower())] = idx
for lo, up in LOWER.items(): pass  # las minúsculas acentuadas se convierten en el preprocesador

def art(rows):
    return tuple(int(c) for r in rows for c in r)
SYM = {
 '♥': ["01101100", "13313310", "13333310", "13333310", "01333100", "00131000", "00010000", "00000000"],
 '♡': ["01101100", "10010010", "10000010", "10000010", "01000100", "00101000", "00010000", "00000000"],
 '●': ["00333300", "03111130", "31122113", "31211213", "31211213", "31122113", "03111130", "00333300"],
 '▶': ["03000000", "03300000", "03330000", "03333000", "03330000", "03300000", "03000000", "00000000"],
 '▼': ["00000000", "33333330", "03333300", "00333000", "00030000", "00000000", "00000000", "00000000"],
 '✓': ["00000003", "00000033", "00000330", "33003300", "03333000", "00330000", "00000000", "00000000"],
 '×': ["00000000", "33000330", "03303300", "00333000", "03303300", "33000330", "00000000", "00000000"],
 '«': ["00000000", "00330330", "03303300", "33033000", "03303300", "00330330", "00000000", "00000000"],
 '»': ["00000000", "33033000", "03303300", "00330330", "03303300", "33033000", "00000000", "00000000"],
}
lut[0x99] = len(tiles); tiles.append(glyph_px('i'))
for ch, rows in SYM.items():
    lut[EXTRA[ch]] = len(tiles); tiles.append(art(rows))

# Caja de diálogo: TL T TR L R BL B BR
BOX0 = len(tiles)
box = {
 'TL': ["00000000", "00333333", "03111111", "03100000", "03100000", "03100000", "03100000", "03100000"],
 'T':  ["00000000", "33333333", "11111111", "00000000", "00000000", "00000000", "00000000", "00000000"],
 'TR': ["00000000", "33333300", "11111130", "00000130", "00000130", "00000130", "00000130", "00000130"],
 'L':  ["03100000"]*8,
 'R':  ["00000130"]*8,
 'BL': ["03100000", "03100000", "03100000", "03100000", "03100000", "03111111", "00333333", "00000000"],
 'B':  ["00000000", "00000000", "00000000", "00000000", "00000000", "11111111", "33333333", "00000000"],
 'BR': ["00000130", "00000130", "00000130", "00000130", "00000130", "11111130", "33333300", "00000000"],
}
for k in ['TL', 'T', 'TR', 'L', 'R', 'BL', 'B', 'BR']: tiles.append(art(box[k]))
# Barras de PS: 9 niveles (0..8 px llenos) + extremos
BAR0 = len(tiles)
for n in range(9):
    rows = ["33333333"] + ["".join('3' if x < n else '0' for x in range(8))]*4 + ["33333333", "00000000", "00000000"]
    rows = ["00000000"] + rows[:7]
    tiles.append(art(rows))
tiles.append(art(["00000000", "00000003", "00000003", "00000003", "00000003", "00000003", "00000003", "00000000"]))  # BAR_L
tiles.append(art(["00000000", "30000000", "30000000", "30000000", "30000000", "30000000", "30000000", "00000000"]))  # BAR_R
FILL1 = len(tiles); tiles.append(tuple([1]*64))
FILL3 = len(tiles); tiles.append(tuple([3]*64))
COMMON_N = len(tiles)
assert COMMON_N < 100, COMMON_N
SCENE0 = 100  # los tiles de escena empiezan aquí (BG en modo 0x8800: 256 tiles libres)

# ---------------------------------------------------------------- metatiles 16x16 para mapas
def canvas(): return Image.new('L', (16, 16), 0)
def put(im, rows, ox=0, oy=0):
    for y, r in enumerate(rows):
        for x, c in enumerate(r):
            if c != ' ': im.putpixel((ox+x, oy+y), int(c))
M = {}
im = canvas()
for y in range(16):
    for x in range(16):
        if (x*7+y*5) % 23 == 0 or (x+y*3) % 29 == 0: im.putpixel((x, y), 1)
M['.'] = im
im = canvas(); im.paste(1, (0, 0, 16, 16))
for (x, y) in [(3, 2), (11, 5), (6, 10), (13, 13), (1, 13), (9, 1)]: im.putpixel((x, y), 2)
M['='] = im
im = M['.'].copy()
put(im, ["     3333     ", "   33222233   ", "  3221122223  ", " 322111222223 ", " 321122222223 ", "32222222222223",
         "32222222222223", "32222222222223", " 322222222223 ", " 332222222233 ", "   33322333   ", "     3223     ",
         "     3223     ", "    332233    "], 1, 1)
M['T'] = im
im = M['.'].copy()
for (x, y) in [(3, 3), (11, 4), (7, 9), (2, 12), (12, 12)]:
    put(im, [" 1 ", "131", " 1 ", " 2 "], x-1, y-1)
M['F'] = im
im = canvas(); im.paste(2, (0, 0, 16, 16))
put(im, ["", "", "  111     ", " 1   1   ", "", "", "", "", "        111  ", "       1   1 "])
M['~'] = im
im = canvas()
for y in range(16):
    for x in range(16):
        if (x*3+y*7) % 11 == 0: im.putpixel((x, y), 1)
M['s'] = im
im = M['.'].copy()
put(im, ["", "", "", " 3          3   ", "3333333333333333", "3111111111111113", " 3          3   ", " 3          3   ",
         "3333333333333333", "3111111111111113", " 3          3   ", " 3          3   "])
M['f'] = im
im = canvas(); im.paste(2, (0, 0, 16, 16))
for y in range(0, 16, 4): put(im, ["3333333333333333"], 0, y)
for y in range(2, 16, 4): put(im, ["1   1   1   1   "], 0, y)
M['R'] = im
im = canvas(); im.paste(1, (0, 0, 16, 16))
put(im, ["3333333333333333", "", "", "  33333333333   ", "  30001300013   ", "  30001300013   ", "  33333333333   ", "  30001300013   ",
         "  30001300013   ", "  33333333333   ", "", "", "", "", "", "2222222222222222"])
M['H'] = im
im = canvas(); im.paste(1, (0, 0, 16, 16))
put(im, ["3333333333333333", "", "  3333333333    ", "  3222222223    ", "  3222222223    ", "  3222222223    ", "  3222222223    ",
         "  3222222223    ", "  3222222323    ", "  3222222223    ", "  3222222223    ", "  3222222223    ", "  3222222223    ",
         "  3222222223    ", "  3222222223    ", "2232222222232222"])
M['D'] = im
im = M['.'].copy()
put(im, ["", "", " 333333333333 ", " 310000000013 ", " 303330333013 ", " 310000000013 ", " 303303330013 ", " 310000000013 ",
         " 333333333333 ", "      33      ", "      33      ", "      33      ", "     3333     "], 1, 0)
M['S'] = im
im = canvas(); im.paste(0, (0, 0, 16, 16))
for y in range(16):
    for x in range(16):
        if x % 16 == 15 or y % 16 == 15: im.putpixel((x, y), 1)
M['_'] = im
im = canvas(); im.paste(2, (0, 0, 16, 16))
put(im, ["", "", "", "", "", "", "", "", "", "", "", "", "3333333333333333", "1111111111111111", "1111111111111111", "3333333333333333"])
put(im, ["  33333   ", "  31113   ", "  31113   ", "  33333   "], 4, 3)
M['W'] = im
im = M['_'].copy()
put(im, ["", "", "  3333333333  ", "  3000000003  ", "  3011111103  ", "  3011111103  ", "  3333333333  ", " 33333333333333",
         "3111111111111113", "3333333333333333", "3 3          3 3", "3 3          3 3", "3 3          3 3", "333          333"])
M['K'] = im
im = M['_'].copy()
put(im, ["    3  3  3    ", "   323 323 3   ", "  32223 32223  ", " 3222232222223 ", " 3222222222223 ", "  32222222223  ",
         "   332222233   ", "     3223      ", "  33333333333  ", "  31111111113  ", "  31111111113  ", "   311111113   ",
         "   311111113   ", "   333333333   "], 0, 1)
M['P'] = im
M[' '] = canvas()

SOLID = set('TfRHDS~WKP ')
SCENES_MAP = {
 'ivan': ["TTTTTTTTTT",
          "T....=..FT",
          "TF...=...T",
          "==========",
          "T....=...T",
          "T.S..=..FT",
          "RRRR.=RRRR",
          "HDHH.=HHDH",
          "TTTTTTTTTT"],
 'gerard': ["TTTTTTTTTT",
            "T...=....T",
            "Tff.=..ffT",
            "T...=====T",
            "T...=....T",
            "TF..=..S.T",
            "RRR.=.RRRR",
            "HDH.=.HDHH",
            "ffffffffff"],
 'azlan': ["~~~~~~~~~~",
           "ssssssssss",
           "T...=....T",
           "==========",
           "T...=....T",
           "TF..=..S.T",
           "RRRR=.RRRR",
           "HHDH=.HDHH",
           "ssssssssss"],
 'raul': ["WWWWWWWWWW",
          "P__KKKK__P",
          "__________",
          "__________",
          "__________",
          "P________P",
          "KK______KK",
          "__________",
          "WWWWWWWWWW"],
}

def scene_from_image(im):
    """im: 160x144 'L' 0..3 → (tiles únicos, mapa 20x18)."""
    uniq = {}; tl = []; mp = []
    for t in img_tiles(im, 0, 0, 20, 18):
        if t not in uniq: uniq[t] = len(tl); tl.append(t)
        mp.append(SCENE0 + uniq[t])
    assert len(tl) <= 256 - SCENE0, len(tl)
    return tl, mp

scenes = {}; solids = {}
for name, rows in SCENES_MAP.items():
    im = Image.new('L', (160, 144), 0)
    for my, r in enumerate(rows):
        for mx, c in enumerate(r): im.paste(M[c], (mx*16, my*16))
    scenes[name] = scene_from_image(im)
    solids[name] = [2 if c == 'S' else 1 if c in SOLID else 0 for r in rows for c in r]

# ---------------------------------------------------------------- título
im = Image.new('L', (160, 144), 0); d = ImageDraw.Draw(im)
# marco
d.rectangle((2, 2, 157, 141), outline=3); d.rectangle((4, 4, 155, 139), outline=1)
# logo FLiiTS grande (16 px) con sombra
for dx, dy, col in [(2, 2, 1), (0, 0, 3)]:
    d.text((32+dx, 16+dy), "FLiiTS", font=FONT16, fill=col)
d.rectangle((24, 40, 135, 41), fill=2)
scenes['title'] = scene_from_image(im)

# ---------------------------------------------------------------- combate
def battle(variant):
    im = Image.new('L', (160, 144), 0); d = ImageDraw.Draw(im)
    d.ellipse((92, 56, 156, 72), fill=1, outline=2)   # plataforma enemigo
    d.ellipse((4, 84, 68, 98), fill=1, outline=2)     # plataforma héroe
    if variant == 'doc':
        x, y = 108, 18
        d.rectangle((x, y, x+31, y+39), fill=0, outline=3); d.rectangle((x+1, y+1, x+30, y+38), outline=1)
        for i in range(5): d.line((x+5, y+7+i*5, x+26-(i % 2)*6, y+7+i*5), fill=2)
        d.ellipse((x+14, y+26, x+28, y+38), outline=3); d.text((x+17, y+29), "X", font=FONT, fill=3)
        d.polygon([(x+4, y+42), (x+10, y+42), (x+7, y+46)], fill=3)
    if variant == 'bags':
        for i, (x, y) in enumerate([(84, 26), (106, 14), (128, 26)]):
            d.rectangle((x, y+6, x+25, y+33), fill=2, outline=3)
            d.rectangle((x+2, y, x+23, y+8), fill=1, outline=3)
            d.rectangle((x+3, y+12, x+22, y+15), fill=1)
            d.rectangle((x+6, y+18, x+9, y+21), fill=0); d.rectangle((x+16, y+18, x+19, y+21), fill=0)
            d.point((x+8, y+20), fill=3); d.point((x+17, y+20), fill=3)
            d.line((x+5, y+16, x+10, y+18), fill=3); d.line((x+20, y+16, x+15, y+18), fill=3)
            d.line((x+9, y+27, x+16, y+27), fill=3); d.line((x+8, y+28, x+9, y+27), fill=3); d.line((x+17, y+28, x+16, y+27), fill=3)
    return scene_from_image(im)
scenes['battle'] = battle('plain'); scenes['battle_doc'] = battle('doc'); scenes['battle_bags'] = battle('bags')

# Moneda grande 16x16 para la tarjeta de victoria (en escena 'coin')
im = Image.new('L', (160, 144), 0); d = ImageDraw.Draw(im)
d.ellipse((64, 32, 95, 63), fill=1, outline=3); d.ellipse((67, 35, 92, 60), outline=2)
d.text((72, 44), "F", font=FONT16, fill=3) if False else d.text((76, 44), "F", font=FONT, fill=3)
scenes['coin'] = scene_from_image(im)

# Blanco (créditos, textos)
scenes['blank'] = scene_from_image(Image.new('L', (160, 144), 0))

# ---------------------------------------------------------------- sprites (16x32, 8 tiles en modo 8x16)
def lum(c): return 0.299*c[0]+0.587*c[1]+0.114*c[2]
def quant(path, crop=None, frame=(16, 32)):
    im = Image.open(os.path.join(GB, 'spr', path)).convert('RGBA')
    if crop: im = im.crop(crop)
    W, H = frame; out = Image.new('L', (W, H), 0)
    ox = (W - im.width)//2; oy = H - im.height
    for y in range(im.height):
        for x in range(im.width):
            r, g, b, a = im.getpixel((x, y))
            if a < 128: continue
            L = lum((r, g, b))
            v = 1 if L > 165 else 2 if L > 75 else 3
            out.putpixel((ox+x, oy+y), v)
    return out
def spr_tiles(im):  # orden 8x16: columna izq (arriba,abajo de cada fila de 16), luego derecha
    res = []
    W, H = im.size
    for ry in range(0, H, 16):
        for cx in range(0, W, 8):
            for sy in (0, 8):
                res.append(tuple(im.getpixel((cx+x, ry+sy+y)) for y in range(8) for x in range(8)))
    return res
CHARS = ['crew0', 'crew1', 'crew2', 'crew3'] + ['npc%d' % i for i in range(6)] + ['npcno%d' % i for i in range(6)] + ['marti', 'martib', 'franco']
sprites = []; prev = Image.new('L', (16*21, 32), 0)
for i, n in enumerate(CHARS):
    im = quant(n+'.png', crop=(1, 0, 17, 28) if n.startswith('marti') else None)
    prev.paste(im, (i*16, 0)); sprites += spr_tiles(im)
dog = quant('dog.png', frame=(32, 16)); prev.paste(dog, (19*16, 16)); sprites += spr_tiles(dog)
assert len(sprites) == 20*8
# Moneda sprite 16x16 (2 tiles x 2 columnas) para animaciones
coin = Image.new('L', (16, 16), 0); d = ImageDraw.Draw(coin)
d.ellipse((1, 1, 14, 14), fill=1, outline=3); d.ellipse((3, 3, 12, 12), outline=2); d.text((4, 4), "F", font=FONT, fill=3)
coin2 = Image.new('L', (16, 16), 0); d = ImageDraw.Draw(coin2)
coin2.paste(coin.crop((0, 0, 16, 16)).resize((8, 16)), (4, 0))
for c in (coin, coin2):
    for cx in (0, 8):
        for sy in (0, 8): sprites.append(tuple(c.getpixel((cx+x, sy+y)) for y in range(8) for x in range(8)))
# '!' sobre NPC y flecha (8x16)
bang = Image.new('L', (8, 16), 0); put(bang, ["", "", "  333   ", " 31113  ", " 31113  ", " 31113  ", "  313   ", "  313   ", "  333   ", "  313   ", "  333   "])
sprites += [tuple(bang.getpixel((x, sy+y)) for y in range(8) for x in range(8)) for sy in (0, 8)]
# vista previa
pal = {0: (240, 248, 220), 1: (170, 190, 130), 2: (80, 110, 70), 3: (20, 30, 20)}
pv = Image.new('RGB', prev.size); pv.putdata([pal[v] for v in prev.getdata()]); pv.resize((prev.width*4, prev.height*4), Image.NEAREST).save(os.path.join(OUT, 'spr-prev.png'))
for nm in scenes:
    tl, mp = scenes[nm]
    im = Image.new('RGB', (160, 144))
    alltiles = {SCENE0+i: t for i, t in enumerate(tl)}
    for i, t in enumerate(mp):
        tx, ty = i % 20, i // 20
        for k, v in enumerate(alltiles[t]): im.putpixel((tx*8+k % 8, ty*8+k//8), pal[v])
    im.resize((320, 288), Image.NEAREST).save(os.path.join(OUT, 'scene-%s.png' % nm))

# ---------------------------------------------------------------- salida C
def carr(name, data, typ='uint8_t'):
    s = 'const %s %s[%d] = {\n' % (typ, name, len(data))
    for i in range(0, len(data), 16): s += '  ' + ','.join('0x%02X' % v for v in data[i:i+16]) + ',\n'
    return s + '};\n'
c = ['#include <stdint.h>\n#include "assets.h"\n']
h = ['#ifndef ASSETS_H\n#define ASSETS_H\n#include <stdint.h>\n',
     '#define COMMON_N %d\n#define BOX0 %d\n#define BAR0 %d\n#define FILL1 %d\n#define FILL3 %d\n#define SCENE0 %d\n' % (COMMON_N, BOX0, BAR0, FILL1, FILL3, SCENE0),
     '#define T_HEART %d\n#define T_HEART0 %d\n#define T_COIN %d\n#define T_ARROW %d\n#define T_MORE %d\n' % (lut[0x90], lut[0x91], lut[0x92], lut[0x93], lut[0x94]),
     'extern const uint8_t font_lut[256];\nextern const uint8_t common_tiles[%d];\n' % (COMMON_N*16),
     'extern const uint8_t spr_tiles[%d];\n#define SPR_COIN 160\n#define SPR_BANG 168\n' % (len(sprites)*16)]
c.append(carr('font_lut', lut))
c.append(carr('common_tiles', [b for t in tiles for b in tile_bytes(t)]))
c.append(carr('spr_tiles', [b for t in sprites for b in tile_bytes(t)]))
h.append('typedef struct { uint8_t n; const uint8_t *tiles; const uint8_t *map; } scene_t;\n')
for i, nm in enumerate(scenes):
    tl, mp = scenes[nm]
    c.append(carr('sc_%s_t' % nm, [b for t in tl for b in tile_bytes(t)]))
    c.append(carr('sc_%s_m' % nm, mp))
    h.append('#define SC_%s %d\n' % (nm.upper(), i))
c.append('const scene_t scenes[%d] = {%s};\n' % (len(scenes), ','.join('{%d,sc_%s_t,sc_%s_m}' % (len(scenes[n][0]), n, n) for n in scenes)))
h.append('extern const scene_t scenes[%d];\n' % len(scenes))
for nm in solids:
    c.append(carr('solid_%s' % nm, solids[nm])); h.append('extern const uint8_t solid_%s[90];\n' % nm)
h.append('#endif\n')
open(os.path.join(OUT, 'assets.c'), 'w').write(''.join(c))
open(os.path.join(OUT, 'assets.h'), 'w').write(''.join(h))
print('comunes', COMMON_N, {n: len(scenes[n][0]) for n in scenes}, 'sprites', len(sprites))

# ---------------------------------------------------------------- preprocesa main.c (UTF-8 → códigos propios)
src = open(os.path.join(GB, 'src/main.c'), encoding='utf-8').read()
src = src.replace('FLiiTS', 'FLııTS')
out = []
for ch in src:
    ch2 = LOWER.get(ch, ch)
    if ch2 in EXTRA: out.append('\\%03o' % EXTRA[ch2])
    elif ord(ch) > 127: raise SystemExit('carácter no soportado: %r' % ch)
    else: out.append(ch)
open(os.path.join(OUT, 'main.c'), 'w').write(''.join(out))
