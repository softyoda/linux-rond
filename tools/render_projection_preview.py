#!/usr/bin/env python3
"""Render the approved mesh with synthetic content. Requires NumPy and Pillow."""
import json
from pathlib import Path
import numpy as np
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
SIZE = 720


def font(size, mono=False):
    name = 'DejaVuSansMono' if mono else 'DejaVuSans'
    try:
        return ImageFont.truetype(f'/usr/share/fonts/TTF/{name}.ttf', size)
    except OSError:
        return ImageFont.load_default(size=size)


def demo():
    image = Image.new('RGB', (SIZE, SIZE), '#151e29')
    draw = ImageDraw.Draw(image)
    draw.rectangle((0, 0, SIZE, 43), fill='#263345')
    draw.text((20, 10), 'Terminal · demo session', font=font(19), fill='#edf3fb')
    for x, text in [(605, '−'), (645, '□'), (685, '×')]:
        draw.text((x, 7), text, font=font(23), fill='#c7d4e4')
    draw.rectangle((0, 44, SIZE, 87), fill='#1c2939')
    draw.text((20, 56), 'File    Edit    View    Search    Help', font=font(18), fill='#afc2d8')
    rows = [
        ('$ ls -lh ~/Projects', '#6bd5c3'),
        ('', '#dbe5f2'),
        ('drwxr-xr-x  docs       4.0K  Oct 31  09:10', '#dbe5f2'),
        ('drwxr-xr-x  examples   4.0K  Oct 31  09:12', '#dbe5f2'),
        ('drwxr-xr-x  media      4.0K  Oct 31  09:15', '#dbe5f2'),
        ('drwxr-xr-x  projects   4.0K  Oct 31  09:18', '#dbe5f2'),
        ('-rw-r--r--  notes.txt  1.2K  Oct 31  09:20', '#dbe5f2'),
        ('', '#dbe5f2'),
        ('$ printf "Hello, round world!\\n"', '#6bd5c3'),
        ('Hello, round world!', '#f1c77d'),
        ('', '#dbe5f2'),
        ('This is a synthetic terminal sample.', '#a9bdd5'),
        ('The same content is shown in both views.', '#a9bdd5'),
        ('', '#dbe5f2'),
        ('$ _', '#6bd5c3'),
    ]
    for i, (text, color) in enumerate(rows):
        draw.text((20, 110 + i * 35), text, font=font(25, True), fill=color)
    draw.rectangle((0, SIZE-37, SIZE, SIZE), fill='#263345')
    draw.text((18, SIZE-28), 'demo                     UTF-8     100%', font=font(19, True), fill='#a9bdd5')
    return image


def project(image, mesh):
    count = len(mesh)
    points = (mesh + 1) * (SIZE - 1) / 2
    uv = np.full((SIZE, SIZE, 2), np.nan)
    for j in range(count - 1):
        for i in range(count - 1):
            a, b, c, d = points[j, i], points[j, i+1], points[j+1, i], points[j+1, i+1]
            corners = np.array([a, b, c, d])
            lo = np.maximum(np.floor(corners.min(0)).astype(int), 0)
            hi = np.minimum(np.ceil(corners.max(0)).astype(int) + 1, SIZE)
            xx, yy = np.meshgrid(np.arange(lo[0], hi[0]), np.arange(lo[1], hi[1]))
            target = np.stack([xx, yy], -1)
            u = np.full(xx.shape, .5)
            v = u.copy()
            e, f, g = b-a, c-a, d-b-c+a
            for _ in range(8):
                error = a + u[..., None]*e + v[..., None]*f + (u*v)[..., None]*g - target
                du, dv = e + v[..., None]*g, f + u[..., None]*g
                det = du[..., 0]*dv[..., 1] - du[..., 1]*dv[..., 0]
                det = np.where(abs(det) > 1e-10, det, 1e-10)
                u -= (error[..., 0]*dv[..., 1] - error[..., 1]*dv[..., 0]) / det
                v -= (du[..., 0]*error[..., 1] - du[..., 1]*error[..., 0]) / det
            residual = np.linalg.norm(a + u[..., None]*e + v[..., None]*f + (u*v)[..., None]*g - target, axis=-1)
            valid = (u >= -1e-7) & (u <= 1+1e-7) & (v >= -1e-7) & (v <= 1+1e-7) & (residual < 1e-4)
            block = uv[lo[1]:hi[1], lo[0]:hi[0]]
            block[valid] = np.stack([i+u[valid], j+v[valid]], -1) / (count-1)
    valid = np.isfinite(uv).all(-1)
    xy = np.clip(np.nan_to_num(uv) * (SIZE-1), 0, SIZE-1.001)
    x, y = np.floor(xy).astype(int).transpose(2, 0, 1)
    ax, ay = (xy - np.floor(xy)).transpose(2, 0, 1)
    source = np.asarray(image, dtype=float)
    sampled = sum(source[sy, sx] * weight[..., None] for sx, sy, weight in [
        (x, y, (1-ax)*(1-ay)), (x+1, y, ax*(1-ay)),
        (x, y+1, (1-ax)*ay), (x+1, y+1, ax*ay),
    ])
    out = np.full((SIZE, SIZE, 3), [11, 17, 25], dtype=np.uint8)
    out[valid] = sampled[valid].clip(0, 255).astype(np.uint8)
    return Image.fromarray(out)


def main():
    mesh = np.array(json.loads((ROOT/'projection/approved-mesh.json').read_text())['vertices'])
    square = demo()
    circle = project(square, mesh)
    board = Image.new('RGB', (1560, 880), '#0b1119')
    draw = ImageDraw.Draw(board)
    for x, title, image in [(40, 'Square source', square), (800, 'Approved circular projection', circle)]:
        draw.text((x, 29), title, font=font(25), fill='#dbe5f2')
        board.paste(image, (x, 84))
    draw.text((40, 831), 'Projection study · synthetic content · live mode in development', font=font(20), fill='#90a6bf')
    destination = ROOT/'docs/images/projection-preview.png'
    destination.parent.mkdir(parents=True, exist_ok=True)
    board.save(destination, optimize=True)
    print(destination)


if __name__ == '__main__':
    main()
