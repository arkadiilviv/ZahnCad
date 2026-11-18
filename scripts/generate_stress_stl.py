"""Create a synthetic curved plate for throughput checks; no patient or tooth anatomy."""
import argparse
import math
import pathlib
import struct

parser = argparse.ArgumentParser()
parser.add_argument('output', type=pathlib.Path)
parser.add_argument('--columns', type=int, default=2500)
parser.add_argument('--rows', type=int, default=1000)
args = parser.parse_args()
if args.columns <= 0 or args.rows <= 0 or args.columns * args.rows * 2 > 10_000_000:
    parser.error('Require positive dimensions and at most 10 million triangles')
face = struct.Struct('<12fH')
def vertex(x, y):
    return x * 0.02, y * 0.02, math.sin(x * 0.003) * math.cos(y * 0.005)

with args.output.open('wb') as output:
    output.write(b'ZahnCAD synthetic curved plate'.ljust(80, b'\0'))
    output.write(struct.pack('<I', args.columns * args.rows * 2))
    previous = [vertex(x, 0) for x in range(args.columns + 1)]
    for y in range(args.rows):
        following = [vertex(x, y + 1) for x in range(args.columns + 1)]
        row = bytearray()
        for x in range(args.columns):
            a, b, c, d = previous[x], previous[x+1], following[x], following[x+1]
            row += face.pack(0, 0, 1, *a, *b, *c, 0)
            row += face.pack(0, 0, 1, *b, *d, *c, 0)
        output.write(row)
        previous = following
print(f'Wrote {args.columns * args.rows * 2} synthetic triangles to {args.output}')
