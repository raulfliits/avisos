import sys; sys.path.insert(0, '/tmp/claude-0/-workspace/8dc91dcf-bfc0-5e40-b28a-bc016414b30c/scratchpad/gba/tools')
from drv import Gba
from PIL import Image
rom = sys.argv[1]; out = sys.argv[2]
shots = []
def run(idx, name, keys):
    g = Gba(rom); g.frames(90); g.press('START', 60); g.press('A', 120); g.press('A', 80)
    g.press('DOWN'); g.press('DOWN'); g.press('A', 60)
    for _ in range(idx): g.press('RIGHT')
    g.press('A', 60); g.shot(f'{out}/{name}0.png'); g.press('A', 120)
    for k, n in keys: g.hold(k, n) if k else g.frames(n)
    g.shot(f'{out}/{name}1.png'); shots.extend([f'{out}/{name}0.png', f'{out}/{name}1.png'])
    print(name, 'MG.state', g.s32('MG', 4 * 8), 'vbl', g.s32('vbl_count')); g.close()
run(0, 'ivan', [('A', 40), (None, 200), ('LEFT', 30), ('RIGHT', 30)])
run(1, 'gerard', [(None, 60), ('RIGHT', 30), ('A', 5), (None, 200)])
run(2, 'azlan', [('UP', 60), ('LEFT', 20), ('A', 5), (None, 60)])
W = Image.new('RGB', (480 * 3, 320 * 2))
for i, f in enumerate(shots): W.paste(Image.open(f), ((i % 3) * 480, (i // 3) * 320))
W.save(f'{out}/smoke.png')
