#!/usr/bin/env python3
# Compila la ROM de GBA.
#   python3 -I build.py <raíz_proyecto> [salida.gba]
# La raíz contiene eng/ (motor), game/ (juego, *.c y *.h). Se busca la fuente en <raíz>/../gb/dl/ps2p.ttf o <raíz>/dl/ps2p.ttf.
import sys, os, re, glob, math, subprocess, shutil
from PIL import Image, ImageFont, ImageDraw

ROOT = os.path.abspath(sys.argv[1])
OUT = os.path.abspath(sys.argv[2]) if len(sys.argv) > 2 else os.path.join(ROOT, 'build', 'fliits.gba')
TOOLS = os.path.dirname(os.path.abspath(__file__))
B = os.path.join(ROOT, 'build'); SRC = os.path.join(B, 'src'); OBJ = os.path.join(B, 'obj')
for d in (SRC, OBJ): os.makedirs(d, exist_ok=True)
FONTP = next(p for p in [os.path.join(ROOT, 'dl/ps2p.ttf'), os.path.join(TOOLS, '../dl/ps2p.ttf'), os.path.join(TOOLS, '../../gb/dl/ps2p.ttf')] if os.path.exists(p))

srcs = sorted(glob.glob(os.path.join(ROOT, 'eng', '*.c')) + glob.glob(os.path.join(ROOT, 'game', '*.c')))
hdrs = sorted(glob.glob(os.path.join(ROOT, 'eng', '*.h')) + glob.glob(os.path.join(ROOT, 'game', '*.h')))
asms = sorted(glob.glob(os.path.join(ROOT, 'eng', '*.s')))
COLRE = re.compile(r'\bC\(\s*0x([0-9a-fA-F]{6})\s*\)')

# ---------------------------------------------------------------- paleta
cols = {}
for f in srcs + hdrs:
    for m in COLRE.finditer(open(f, encoding='utf-8').read()):
        v = int(m.group(1), 16); cols[v] = cols.get(v, 0) + 1
base = [0x000000, 0xFFFFFF]
uniq = sorted(set(cols) | set(base))
def rgb(v): return (v >> 16, (v >> 8) & 255, v & 255)
def dist(a, b): return 3*(a[0]-b[0])**2 + 4*(a[1]-b[1])**2 + 2*(a[2]-b[2])**2
# agrupación: cada color es un grupo; se fusionan los dos más cercanos hasta tener <= 255
groups = [[v] for v in uniq]
cent = [rgb(v) for v in uniq]
weight = [max(cols.get(v, 1), 1) for v in uniq]
while len(groups) > 255:
    best = None
    for i in range(len(cent)):
        for j in range(i+1, len(cent)):
            d = dist(cent[i], cent[j])
            if best is None or d < best[0]: best = (d, i, j)
    _, i, j = best
    wi, wj = weight[i], weight[j]
    cent[i] = tuple(round((cent[i][k]*wi + cent[j][k]*wj)/(wi+wj)) for k in range(3))
    groups[i] += groups[j]; weight[i] = wi + wj
    del groups[j], cent[j], weight[j]
index = {}
for gi, g in enumerate(groups):
    for v in g: index[v] = gi + 1          # índice 0 = transparente / negro de fondo
pal24 = [0] + [ (c[0] << 16) | (c[1] << 8) | c[2] for c in cent ]
pal24 += [0] * (256 - len(pal24))
def to15(v): r, g, b = rgb(v); return (r >> 3) | ((g >> 3) << 5) | ((b >> 3) << 10)

# ---------------------------------------------------------------- fuente 8x8
F8 = ImageFont.truetype(FONTP, 8)
EXTRA = "¡¿·×«»°ªºÁÉÍÓÚÀÈÒÑÜÇÏáéíóúàèòñüçï€–—’‘“”…✓✗→←↑↓★☆▶◀▲▼♥●○∞"
cps = list(range(32, 127)) + [ord(c) for c in EXTRA]
HAND = {
 '✓': ["00000001", "00000011", "00000110", "11001100", "01111000", "00110000", "00000000", "00000000"],
 '✗': ["11000011", "01100110", "00111100", "00011000", "00111100", "01100110", "11000011", "00000000"],
 '→': ["00001000", "00001100", "11111110", "11111111", "11111110", "00001100", "00001000", "00000000"],
 '←': ["00010000", "00110000", "01111111", "11111111", "01111111", "00110000", "00010000", "00000000"],
 '↑': ["00011000", "00111100", "01111110", "11111111", "00111100", "00111100", "00111100", "00000000"],
 '↓': ["00111100", "00111100", "00111100", "11111111", "01111110", "00111100", "00011000", "00000000"],
 '★': ["00011000", "00011000", "11111111", "01111110", "00111100", "01100110", "11000011", "00000000"],
 '☆': ["00011000", "00100100", "11000011", "01000010", "00100100", "01011010", "11000011", "00000000"],
 '▶': ["01000000", "01110000", "01111100", "01111111", "01111100", "01110000", "01000000", "00000000"],
 '◀': ["00000010", "00001110", "00111110", "11111110", "00111110", "00001110", "00000010", "00000000"],
 '▲': ["00000000", "00011000", "00111100", "01111110", "11111111", "00000000", "00000000", "00000000"],
 '▼': ["00000000", "11111111", "01111110", "00111100", "00011000", "00000000", "00000000", "00000000"],
 '♥': ["01101100", "11111110", "11111110", "11111110", "01111100", "00111000", "00010000", "00000000"],
 '●': ["00111100", "01111110", "11111111", "11111111", "11111111", "01111110", "00111100", "00000000"],
 '○': ["00111100", "01000010", "10000001", "10000001", "10000001", "01000010", "00111100", "00000000"],
 '∞': ["00000000", "01100110", "10011001", "10011001", "10011001", "01100110", "00000000", "00000000"],
 '…': ["00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "10010010", "00000000"],
}
ACC = {'Á': ('A', '....##..'), 'É': ('E', '....##..'), 'Í': ('I', '....##..'), 'Ó': ('O', '....##..'), 'Ú': ('U', '....##..'),
       'À': ('A', '..##....'), 'È': ('E', '..##....'), 'Ò': ('O', '..##....'), 'Ñ': ('N', '.##.##..'), 'Ü': ('U', '.##.##..'), 'Ï': ('I', '.##.##..')}
def glyph_rows(ch):
    if ch in HAND: return [int(r, 2) for r in HAND[ch]]
    im = Image.new('L', (8, 8), 0); ImageDraw.Draw(im).text((0, 0), ch, font=F8, fill=255)
    rows = []
    for y in range(8):
        v = 0
        for x in range(8):
            if im.getpixel((x, y)) > 128: v |= 0x80 >> x
        rows.append(v)
    if ch in ACC:  # mayúsculas acentuadas legibles en 8 px: acento + letra de 5 filas
        base = glyph_rows(ACC[ch][0])[:7]
        for _ in range(2):
            n = len(base)
            d = next((i for i in (3, 2, 4, 1, 5) if i < n-1 and (base[i] == base[i-1] or base[i] == base[i+1])), n//2)
            base = base[:d] + base[d+1:]
        acc = int(ACC[ch][1].replace('#', '1').replace('.', '0'), 2)
        rows = [acc, 0] + base + [0]
    return rows
font = [(cp, glyph_rows(chr(cp))) for cp in sorted(set(cps))]

# ---------------------------------------------------------------- fuente 3x5
SMALL = {
 'A': "010101111101101", 'B': "110101110101110", 'C': "011100100100011", 'D': "110101101101110", 'E': "111100110100111",
 'F': "111100110100100", 'G': "011100101101011", 'H': "101101111101101", 'I': "111010010010111", 'J': "001001001101010",
 'K': "101101110101101", 'L': "100100100100111", 'M': "101111111101101", 'N': "110101101101101", 'O': "010101101101010",
 'P': "110101110100100", 'Q': "010101101110011", 'R': "110101110101101", 'S': "011100010001110", 'T': "111010010010010",
 'U': "101101101101111", 'V': "101101101101010", 'W': "101101111111101", 'X': "101101010101101", 'Y': "101101010010010",
 'Z': "111001010100111", '0': "111101101101111", '1': "010110010010111", '2': "110001010100111", '3': "110001010001110",
 '4': "101101111001001", '5': "111100110001110", '6': "011100110101010", '7': "111001010010010", '8': "010101010101010",
 '9': "010101011001110", '.': "000000000000010", ',': "000000000010100", ':': "000010000010000", '!': "010010010000010",
 '?': "110001010000010", '-': "000000111000000", '+': "000010111010000", '/': "001001010100100", '%': "101001010100101",
 '(': "010100100100010", ')': "010001001001010", "'": "010010000000000", '·': "000000010000000", '¡': "010000010010010",
 '¿': "010000010100011", '×': "000101010101000", '✓': "000001001110010", '★': "010111010101000", '€': "011110100110011",
 '=': "000111000111000", '←': "010100111100010", '→': "010001111001010", '↑': "010111010010010", '↓': "010010010111010",
 '▶': "100110111110100", '◀': "001011111011001", '☆': "010101010101000", '♥': "000101111111010", '●': "000010111010000", '"': "101101000000000", '#': "101111101111101", '<': "001010100010001", '>': "100010001010100",
}
def small_base(ch):
    m = {'Á': 'A', 'À': 'A', 'É': 'E', 'È': 'E', 'Í': 'I', 'Ï': 'I', 'Ó': 'O', 'Ò': 'O', 'Ú': 'U', 'Ü': 'U', 'Ñ': 'N', 'Ç': 'C'}
    u = ch.upper(); return m.get(u, u)
small = []
for cp in sorted(set(cps)):
    b = small_base(chr(cp))
    if b in SMALL: small.append((cp, [int(SMALL[b][r*3:r*3+3], 2) for r in range(5)]))

# ---------------------------------------------------------------- gen.h / gen.c
nq = [i for i, (cp, _) in enumerate(font) if cp == ord('?')][0]
gh = ['#ifndef GEN_H\n#define GEN_H\n#include "eng.h"\n',
      '#define PAL_N %d\n#define FONT_N %d\n#define FONT_Q %d\n#define SMALL_N %d\n' % (len(cent) + 1, len(font), nq, len(small)),
      'extern const u16 PAL15[256];\nextern const u32 PAL24[256];\nextern const u16 FONT_CP[];\nextern const u8 FONT_BM[][8];\n',
      'extern const u16 SMALL_CP[];\nextern const u8 SMALL_BM[][5];\nextern const s16 SIN_T[1024];\n#endif\n']
gc = ['#include "gen.h"\n',
      'const u16 PAL15[256] = {%s};\n' % ','.join(str(to15(v)) for v in pal24),
      'const u32 PAL24[256] = {%s};\n' % ','.join('0x%06X' % v for v in pal24),
      'const u16 FONT_CP[] = {%s};\n' % ','.join(str(cp) for cp, _ in font),
      'const u8 FONT_BM[][8] = {%s};\n' % ','.join('{' + ','.join(str(r) for r in rows) + '}' for _, rows in font),
      'const u16 SMALL_CP[] = {%s};\n' % ','.join(str(cp) for cp, _ in small),
      'const u8 SMALL_BM[][5] = {%s};\n' % ','.join('{' + ','.join(str(r) for r in rows) + '}' for _, rows in small),
      'const s16 SIN_T[1024] = {%s};\n' % ','.join(str(round(math.sin(i * 2 * math.pi / 1024) * 4096)) for i in range(1024))]
def wr(p, s):
    if not os.path.exists(p) or open(p, encoding='utf-8').read() != s: open(p, 'w', encoding='utf-8').write(s)
wr(os.path.join(SRC, 'gen.h'), ''.join(gh)); wr(os.path.join(SRC, 'gen.c'), ''.join(gc))

# ---------------------------------------------------------------- fuentes con colores sustituidos
def subst(t): return COLRE.sub(lambda m: '((u8)%d)' % index[int(m.group(1), 16)], t)
for f in srcs + hdrs:
    wr(os.path.join(SRC, os.path.basename(f)), subst(open(f, encoding='utf-8').read()))
for f in asms: shutil.copy(f, SRC)

CC = 'arm-none-eabi-gcc'
CFL = ['-mcpu=arm7tdmi', '-mthumb', '-mthumb-interwork', '-O2', '-ffunction-sections', '-fdata-sections', '-Wall', '-Wno-unused-function',
       '-Wno-misleading-indentation', '-fno-strict-aliasing', '-std=gnu11', '-I', SRC]
objs = []
for f in [os.path.join(SRC, 'gen.c')] + [os.path.join(SRC, os.path.basename(s)) for s in srcs] + [os.path.join(SRC, os.path.basename(a)) for a in asms]:
    o = os.path.join(OBJ, os.path.basename(f) + '.o'); objs.append(o)
    deps = [f] + glob.glob(os.path.join(SRC, '*.h'))
    if os.path.exists(o) and os.path.getmtime(o) >= max(os.path.getmtime(d) for d in deps): continue
    r = subprocess.run([CC] + CFL + ['-c', f, '-o', o], capture_output=True, text=True)
    if r.stderr: sys.stderr.write(r.stderr)
    if r.returncode: sys.exit('error compilando ' + f)
elf = os.path.join(B, 'fliits.elf')
r = subprocess.run([CC, '-mcpu=arm7tdmi', '-mthumb', '-mthumb-interwork', '-nostartfiles', '-specs=nosys.specs', '-T', os.path.join(ROOT, 'eng', 'gba.ld'),
                    '-Wl,--gc-sections', '-Wl,-Map,' + os.path.join(B, 'fliits.map'), '-o', elf] + objs + ['-lm'], capture_output=True, text=True)
if r.stderr: sys.stderr.write(r.stderr)
if r.returncode: sys.exit('error enlazando')
subprocess.run(['arm-none-eabi-objcopy', '-O', 'binary', elf, OUT], check=True)
subprocess.run([os.path.join(TOOLS, 'gbafix'), OUT, '-tFLIITS WORLD', '-cFLCW', '-mFL', '-r1'], check=True, capture_output=True)
sz = subprocess.run(['arm-none-eabi-size', elf], capture_output=True, text=True).stdout.splitlines()[-1].split()
print('colores %d (paleta %d)  rom %d KB  iwram data+bss %s+%s' % (len(uniq), len(cent) + 1, os.path.getsize(OUT) // 1024, sz[1], sz[2]))
