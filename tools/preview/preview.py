#!/usr/bin/env python3
"""Turn preview.cpp output into an animated GIF of the glow around the TV.

    ./preview 250 25 | python3 preview.py out.gif
    ./preview 250 25 | python3 preview.py out.gif --tv-frames 'frames/*.png'

The LED values are linear PWM duty; they're mixed as light (additive, blurred) onto a
dark firebox and gamma-encoded for display. It approximates the look, not photometry.
--tv-frames shows images on the TV (e.g. frames from tools/video_match/storyboard.py),
cycled a few per second; they are not time-synced with the glow.
"""
import argparse
import glob
import sys

import numpy as np
from PIL import Image, ImageDraw, ImageFilter

W, H = 480, 300
TV = (90, 60, 390, 236)  # left, top, right, bottom in pixels
WARM_WHITE = np.array([1.0, 0.72, 0.42])  # 3000K die as linear RGB
GAIN = 0.9  # exposure of the simulated camera; higher values clip the glow toward white


def main(out_path, tv_frames=None):
    tv = [Image.open(p).convert("RGB") for p in sorted(glob.glob(tv_frames))] if tv_frames else []
    lines = sys.stdin.read().split("\n")
    n, frames = map(int, lines[0].split())
    pos = [tuple(map(float, l.split())) for l in lines[1 : 1 + n]]
    data = np.array([list(map(float, l.split())) for l in lines[1 + n : 1 + n + n * frames]])
    data = data.reshape(frames, n, 4)

    tl, tt, tr, tb = TV
    pts = [(tl + x * (tr - tl), tb - y * (tb - tt)) for x, y in pos]
    # Push each LED's light outward from the TV center, as the angled strip does.
    cx, cy = (tl + tr) / 2, (tt + tb) / 2
    spill = []
    for px, py in pts:
        dx, dy = px - cx, py - cy
        d = max(abs(dx) / (tr - tl), abs(dy) / (tb - tt)) * 2
        spill.append((cx + dx / d * 1.18, cy + dy / d * 1.25))

    base = np.zeros((H, W, 3))
    base[:, :] = [0.006, 0.004, 0.004]  # sooty firebox
    out = []
    for f in range(frames):
        light = Image.new("RGB", (W, H))
        draw = ImageDraw.Draw(light)
        for (sx, sy), c in zip(spill, data[f]):
            rgb = np.clip(c[:3] + c[3] * WARM_WHITE, 0, 1)
            col = tuple(int(v * 255) for v in rgb)
            draw.ellipse([sx - 16, sy - 16, sx + 16, sy + 16], fill=col)
        lit = np.asarray(light.filter(ImageFilter.GaussianBlur(22)), dtype=float) / 255 * GAIN
        img = np.clip(base + lit, 0, 1) ** (1 / 2.2)
        frame = Image.fromarray((img * 255).astype(np.uint8))
        d = ImageDraw.Draw(frame)
        if tv:
            pic = tv[(f // 4) % len(tv)].resize((tr - tl, tb - tt), Image.BICUBIC)
            frame.paste(pic, (tl, tt))
            d.rectangle(TV, outline=(30, 30, 34))
        else:
            d.rectangle(TV, fill=(8, 8, 10), outline=(30, 30, 34))
            d.text((tl + 8, tt + 6), "TV (YouTube fire)", fill=(70, 70, 76))
        for x, y in pts:
            d.point((x, y), fill=(60, 50, 40))
        frame = frame.resize((W * 2 // 3, H * 2 // 3), Image.LANCZOS)
        out.append(frame.convert("P", palette=Image.ADAPTIVE, colors=64))
    out[0].save(out_path, save_all=True, append_images=out[1:], duration=40, loop=0)


if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("out")
    ap.add_argument("--tv-frames", help="glob of images to show on the TV")
    a = ap.parse_args()
    main(a.out, a.tv_frames)
