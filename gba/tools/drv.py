"""Conductor de pruebas para la ROM en mGBA (sin ventana).
    from drv import Gba
    g = Gba('build/fliits.gba')          # arranca
    g.frames(60); g.press('A'); g.hold('RIGHT', 30); g.shot('x.png')
    g.u32('MG') / g.read('nombre_variable', 4) lee memoria por símbolo (build/fliits.elf)
"""
import subprocess, os, struct
from PIL import Image
HERE = os.path.dirname(os.path.abspath(__file__))
KEYS = {'A': 1, 'B': 2, 'SELECT': 4, 'START': 8, 'RIGHT': 16, 'LEFT': 32, 'UP': 64, 'DOWN': 128, 'R': 256, 'L': 512}
class Gba:
    def __init__(self, rom):
        self.rom = os.path.abspath(rom); self.elf = os.path.join(os.path.dirname(self.rom), 'fliits.elf')
        self.p = subprocess.Popen([os.path.join(HERE, 'hrn'), self.rom], stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True)
        self.held = 0; self.syms = None; self.n = 0
    def cmd(self, c):
        self.p.stdin.write(c + '\n'); self.p.stdin.flush()
        out = []
        while True:
            l = self.p.stdout.readline()
            if not l: raise RuntimeError('el emulador se cerró')
            if l.startswith('ok'): return out
            out.append(l.strip())
    def _k(self): self.cmd('k %d' % self.held)
    def frames(self, n=1): self.cmd('f %d' % n); self.n += n
    def down(self, *ks):
        for k in ks: self.held |= KEYS[k]
        self._k()
    def up(self, *ks):
        for k in ks: self.held &= ~KEYS[k]
        self._k()
    def hold(self, k, n): self.down(k); self.frames(n); self.up(k)
    def press(self, k, after=6): self.hold(k, 3); self.frames(after)
    def shot(self, path, scale=2):
        tmp = path + '.ppm'; self.cmd('s ' + tmp)
        im = Image.open(tmp); im.load(); os.remove(tmp)
        if scale != 1: im = im.resize((im.width * scale, im.height * scale), Image.NEAREST)
        im.save(path); return im
    def sym(self, name):
        if self.syms is None:
            self.syms = {}
            for l in subprocess.run(['arm-none-eabi-nm', self.elf], capture_output=True, text=True).stdout.splitlines():
                a = l.split()
                if len(a) == 3: self.syms[a[2]] = int(a[0], 16)
        return self.syms[name]
    def read(self, addr, n):
        if isinstance(addr, str): addr = self.sym(addr)
        return bytes.fromhex(self.cmd('r %x %d' % (addr, n))[0])
    def u8(self, a, off=0): return self.read(self.sym(a) + off if isinstance(a, str) else a + off, 1)[0]
    def s32(self, a, off=0): return struct.unpack('<i', self.read(self.sym(a) + off if isinstance(a, str) else a + off, 4))[0]
    def u16(self, a, off=0): return struct.unpack('<H', self.read(self.sym(a) + off if isinstance(a, str) else a + off, 2))[0]
    def close(self):
        try: self.cmd('q')
        except Exception: pass
