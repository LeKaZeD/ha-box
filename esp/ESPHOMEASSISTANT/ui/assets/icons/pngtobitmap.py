from PIL import Image
import sys

# Usage:
#   python png_to_1bpp.py input.png out.h ICON_NAME [threshold] [invert] [mirror]
# Example:
#   python png_to_1bpp.py check.png icons_check.h ICON_CHECK_16x16 180 0 1  # miroir ON
#   python png_to_1bpp.py check.png icons_check.h ICON_CHECK_16x16 180 0 0  # miroir OFF

inp = sys.argv[1]
out = sys.argv[2]
name = sys.argv[3]

THRESHOLD = int(sys.argv[4]) if len(sys.argv) > 4 else 180
INVERT_BITS = bool(int(sys.argv[5])) if len(sys.argv) > 5 else False  # 1 => invert bits
MIRROR_X = bool(int(sys.argv[6])) if len(sys.argv) > 6 else True      # 1 => mirror left-right (default ON)

img = Image.open(inp).convert("RGBA")

# Remplit la transparence par du blanc (important pour Figma)
bg = Image.new("RGBA", img.size, (255, 255, 255, 255))
bg.alpha_composite(img)

# Mirror droite-gauche (flip horizontal)
if MIRROR_X:
    bg = bg.transpose(Image.FLIP_LEFT_RIGHT)

gray = bg.convert("L")

# Binarisation: 1=white, 0=black (mode "1" avec seuil)
bw = gray.point(lambda p: 255 if p > THRESHOLD else 0, mode="1")

w, h = bw.size
px = bw.load()

srcBpr = (w + 7) // 8
data = [0] * (srcBpr * h)

# Pack MSB-first, row-major (compatible avec ton drawBitmap)
for y in range(h):
    for x in range(w):
        byte_i = y * srcBpr + (x >> 3)
        bit = 7 - (x & 7)

        # Mode "1": px[x,y] est 0 (black) ou 255 (white)
        is_white = 1 if px[x, y] else 0  # 1 => white

        if is_white:
            data[byte_i] |= (1 << bit)   # bit=1 => white
        # else: bit reste à 0 => black

# Inversion optionnelle (si tu veux 1=black au lieu de 1=white)
if INVERT_BITS:
    data = [b ^ 0xFF for b in data]

with open(out, "w", encoding="utf-8") as f:
    f.write("#pragma once\n#include <Arduino.h>\n\n")
    f.write(f"// {w}x{h} 1bpp MSB-first row-major\n")
    f.write(f"// bit=1 => white, bit=0 => black\n")
    f.write(f"// mirror_x={'1' if MIRROR_X else '0'}\n")
    f.write(f"static const uint8_t {name}[] PROGMEM = {{\n  ")
    for i, b in enumerate(data):
        f.write(f"0x{b:02X}")
        if i != len(data) - 1:
            f.write(", ")
        if (i + 1) % 12 == 0 and i != len(data) - 1:
            f.write("\n  ")
    f.write("\n};\n")