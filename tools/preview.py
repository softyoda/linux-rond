#!/usr/bin/env python3
"""Aperçu de l'écran rond : assombrit les coins invisibles et trace le cercle + le carré inscrit."""
import sys
from PIL import Image, ImageDraw
src, dst = sys.argv[1], sys.argv[2]
im = Image.open(src).convert("RGB")
w, h = im.size
d = min(w, h)
mask = Image.new("L", im.size, 0)
ImageDraw.Draw(mask).ellipse((0, 0, d - 1, d - 1), fill=255)
dark = Image.new("RGB", im.size, (0, 0, 0))
out = Image.composite(im, Image.blend(im, dark, 0.8), mask)
dr = ImageDraw.Draw(out)
dr.ellipse((0, 0, d - 1, d - 1), outline=(255, 60, 60), width=3)
s = int(d / 2 ** 0.5); o = (d - s) // 2
dr.rectangle((o, o, o + s, o + s), outline=(80, 200, 255), width=2)
out.save(dst)
