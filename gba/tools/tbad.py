import struct
def dump(g, tag=''):
    n = g.s32('tbad_n'); out = []
    raw = g.read('TBAD', 64 * n) if n else b''
    for i in range(n):
        b = raw[i*64:(i+1)*64]
        a = b[:28].split(b'\0')[0].decode('utf-8', 'replace'); bb = b[28:56].split(b'\0')[0].decode('utf-8', 'replace')
        x, y, w, k = struct.unpack('<4h', b[56:64])
        out.append(('SALE ' if k == 1 else 'PISA ') + '%-28s %-28s x=%d y=%d w=%d' % (a, bb, x, y, w))
    return out
