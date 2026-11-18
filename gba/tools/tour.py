# Recorre un modo/juego desde los menús pulsando botones y saca una hoja de capturas.
import sys, random; sys.path.insert(0, '/tmp/claude-0/-workspace/8dc91dcf-bfc0-5e40-b28a-bc016414b30c/scratchpad/gba/tools')
from drv import Gba
from PIL import Image
rom, mode, pick, out = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), sys.argv[4]
keys = sys.argv[5].split(',') if len(sys.argv) > 5 else ['A', 'RIGHT', 'A', 'UP', 'A', 'LEFT', 'B']
random.seed(pick * 7 + mode)
g = Gba(rom); ui = lambda: g.s32('UI_SCREEN')
g.frames(90); g.press('START', 30)
while ui() != 3: g.press('A', 40)
for _ in range(mode): g.press('DOWN')
g.press('A', 30)
while ui() != 4: g.frames(5)
for _ in range(pick): g.press('RIGHT')
g.press('A', 40)
if ui() == 5: g.press('A', 40)
shots = []
for i in range(16):
    for _ in range(6):
        k = random.choice(keys); g.hold(k, random.choice([3, 3, 10, 30]))
        g.frames(8)
    f = '%s_%02d.png' % (out, i); g.shot(f, 1); shots.append(f)
W = Image.new('RGB', (240 * 4 + 12, 160 * 4 + 12), (255, 0, 255))
for i, f in enumerate(shots): W.paste(Image.open(f), ((i % 4) * 243, (i // 4) * 163))
W = W.resize((W.width * 2, W.height * 2), Image.NEAREST); W.save(out + '_sheet.png'); g.close()
