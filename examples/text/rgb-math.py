#!/usr/bin/env python3

def rgb5to8(c5):
    r5 = c5 >> 11 & 0x1F
    g6 = c5 >>  5 & 0x3F
    b5 = c5 >>  0 & 0x1F
    r8 = r5 << 3 | r5 >> 2
    g8 = g6 << 2 | g6 >> 4
    b8 = b5 << 3 | b5 >> 2
    return r8 << 16 | g8 << 8 | b8 << 0
   

def rgb8to5(c8):
    r = c8 >> 16 & 0xFF
    g = c8 >>  8 & 0xFF
    b = c8 >>  0 & 0xFF
    r >>= 3
    g >>= 2
    b >>= 3
    print('r g b', hex(r), hex(g), hex(b))
    return r << 11 | g << 5 | b << 0

amber5 = 0xfd85
print('amber: {:x} {:x}'.format(amber5, rgb5to8(amber5)))
