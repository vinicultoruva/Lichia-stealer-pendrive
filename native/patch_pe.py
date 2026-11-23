import sys

path = sys.argv[1]
b = bytearray(open(path, 'rb').read())

i = b.find(b'Rich', 0x40, 0x400)
if i != -1:
    for j in range(0x80, min(i + 8, len(b))):
        b[j] = 0

pe = int.from_bytes(b[0x3C:0x40], 'little')
b[pe + 8:pe + 12] = b'\x00' * 4
b[pe + 24 + 64:pe + 24 + 68] = b'\x00' * 4

open(path, 'wb').write(b)
print('patched:', path)
