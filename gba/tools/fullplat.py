# Partida completa de PLATAFORMAS desde los menús: 3 pantallas, oficina de Raül, final y créditos.
import sys, struct; sys.path.insert(0, '/tmp/claude-0/-workspace/8dc91dcf-bfc0-5e40-b28a-bc016414b30c/scratchpad/gba/tools')
from drv import Gba
rom, out = sys.argv[1], sys.argv[2]
g = Gba(rom); shots = []
def ui(): return g.s32('UI_SCREEN')
def shot(n): f = '%s/%s.png' % (out, n); g.shot(f); shots.append(f)
def wait_ui(v, maxf=3000):
    f = 0
    while ui() != v and f < maxf: g.frames(5); f += 5
    return ui() == v
g.frames(90); g.press('START', 30); wait_ui(2)
while ui() == 2: g.press('A', 40)
assert ui() == 3, ui()
g.press('A', 30); wait_ui(4)
def play_screen(idx, veh, name):
    pass  # el selector ya apunta al siguiente sin superar
    g.press('A', 20); wait_ui(5)
    for _ in range(veh): g.press('RIGHT')
    g.press('A', 10)
    k = 0
    while ui() != 0 and k < 600: g.frames(5); k += 5
    g.down('RIGHT'); f = 0
    while ui() == 0 and f < 9000:
        g.frames(2); f += 2
        d = struct.unpack('<16i', g.read('plat_dbgv', 64))
        if d[6] and d[2]: g.down('A'); g.frames(4); g.up('A'); g.frames(3); f += 7
        if d[7]: g.down('B'); g.frames(4); g.up('B'); g.frames(3); f += 7
        if f in (600, 601): shot(name + '_juego')
    g.up('RIGHT')
    print(name, 'frames', f, 'ui', ui(), 'maxn', g.s32('plat_maxn'))
    g.frames(40); shot(name + '_moneda'); g.press('A', 30); wait_ui(4); g.frames(20)
play_screen(0, 0, 'ivan'); shot('selector1')
play_screen(1, 1, 'gerard')
play_screen(2, 2, 'azlan'); shot('selector_raul')
# Raül (el cursor ya está en él tras desbloquear)
g.press('A', 10)
k = 0
while ui() != 0 and k < 600: g.frames(5); k += 5
f = 0
while ui() != 7 and f < 8000:
    g.frames(10); f += 10
    if f in (300, 1500, 2600): shot('raul_%d' % f)
print('raul+final frames', f, 'ui', ui())
g.frames(200); shot('creditos'); g.press('B', 30); wait_ui(4); g.frames(20); shot('selector_fin')
from PIL import Image
W = Image.new('RGB', (480 * 4, 320 * ((len(shots) + 3) // 4)))
for i, f in enumerate(shots): W.paste(Image.open(f), ((i % 4) * 480, (i // 4) * 320))
W.save(out + '/fullplat.png'); g.close()
