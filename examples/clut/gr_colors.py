#! /usr/bin/env python3

# Golden Ratio Colors
# as suggested here.
# http://martin.ankerl.com/2009/12/09/how-to-create-random-colors-programmatically

from math import sqrt
from colorsys import hsv_to_rgb

n = 15
Φ = (sqrt(5) - 1) / 2

def hex_fmt(rgb):
    return '0x' + ''.join('%02x' % c for c in rgb)

hue = 0
for i in range(n):
    hue = (hue + Φ) % 1
    hsv = (hue, 1.0, 1.0)
    rgb = hsv_to_rgb(*hsv)
    rgb_int = [round(c * 0xFF) for c in rgb]
    print('rgb = {} = {}'.format(rgb_int, hex_fmt(rgb_int)))
