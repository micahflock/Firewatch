#!/usr/bin/env python3
"""Measure a YouTube fireplace video's light from its public storyboard images.

    python3 storyboard.py CF00JqnCcsU --cache /tmp/sb --out video_stats.json [--frames-out frames/]

YouTube publishes low-res "storyboard" frames (the scrub-bar previews): for long videos,
160x90 px about every 10 s across the whole video. That's too coarse to see flicker
speed, but enough to measure how the light behaves at random moments (see stats.py), and
to find repeats: many long fireplace videos are a short clip on a loop.

Needs yt-dlp (and a JavaScript runtime such as node for current YouTube extraction).
Nothing is stored in the repo; images go to --cache.
"""
import argparse
import json
import os
import subprocess
import sys

import numpy as np
from PIL import Image

from stats import summarize, video_zones


def fetch(video_id, cache):
    os.makedirs(cache, exist_ok=True)
    info_path = os.path.join(cache, "info.json")
    if not os.path.exists(info_path):
        out = subprocess.run(
            ["yt-dlp", "--no-warnings", "--js-runtimes", "node", "-J", f"https://www.youtube.com/watch?v={video_id}"],
            capture_output=True, text=True, check=True).stdout
        open(info_path, "w").write(out)
    info = json.load(open(info_path))
    sb = max((f for f in info["formats"] if f["format_id"].startswith("sb")), key=lambda f: f.get("width") or 0)
    for i, fr in enumerate(sb["fragments"]):
        p = os.path.join(cache, f"sheet_{i:03d}.jpg")
        if not os.path.exists(p):
            subprocess.run(["curl", "-sS", "-f", "-o", p, fr["url"]], check=True)
    return info, sb


def slice_frames(info, sb, cache):
    w, h, rows, cols = sb["width"], sb["height"], sb["rows"], sb["columns"]
    frames = []
    for i in range(len(sb["fragments"])):
        im = np.asarray(Image.open(os.path.join(cache, f"sheet_{i:03d}.jpg")).convert("RGB"), dtype=np.float32) / 255
        for r in range(rows):
            for c in range(cols):
                tile = im[r * h:(r + 1) * h, c * w:(c + 1) * w]
                if tile.shape[:2] == (h, w):
                    frames.append(tile)
    dt = 1 / sb["fps"]
    n = min(len(frames), int(np.ceil(info["duration"] / dt)))
    return np.array(frames[:n]), dt


def repeats(frames, dt, top=5):
    """Lags at which the picture (blurred, so logs/flame layout dominate) repeats."""
    small = np.array([np.asarray(Image.fromarray((f * 255).astype(np.uint8)).resize((32, 18), Image.BOX),
                                 dtype=np.float32).mean(-1).ravel() for f in frames])
    g = (small - small.mean(1, keepdims=True)) / (small.std(1, keepdims=True) + 1e-6)
    C = g @ g.T / g.shape[1]
    lags = np.arange(2, len(C) // 2)
    prof = np.array([np.median(np.diag(C, k)) for k in lags])
    order = np.argsort(-prof)[:top]
    base = float(np.median(prof))
    return [{"lag_s": round(float(lags[i] * dt), 1), "median_similarity": round(float(prof[i]), 3),
             "share_near_identical": round(float((np.diag(C, lags[i]) > 0.95).mean()), 2)} for i in order], base


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("video_id")
    ap.add_argument("--cache", default="storyboard_cache")
    ap.add_argument("--out", default="video_stats.json")
    ap.add_argument("--frames-out", help="also write individual frames here (for preview.py --tv-frames)")
    a = ap.parse_args()

    info, sb = fetch(a.video_id, a.cache)
    frames, dt = slice_frames(info, sb, a.cache)
    print(f"{info['title']}: {info['duration'] / 3600:.2f} h, {len(frames)} storyboard frames every {dt:.2f} s",
          file=sys.stderr)
    stats = summarize(*video_zones(frames))
    reps, base = repeats(frames, dt)
    json.dump({"video_id": a.video_id, "title": info["title"], "duration_s": info["duration"],
               "sample_interval_s": dt, "source": "storyboard", "stats": stats,
               "repeats": reps, "typical_similarity": round(base, 3)}, open(a.out, "w"), indent=1)
    for k, v in stats.items():
        print(f"  {k:20s} {v:.3f}", file=sys.stderr)
    print(f"  typical similarity between frames: {base:.3f}; strongest repeats:", file=sys.stderr)
    for r in reps:
        print(f"    every {r['lag_s']:8.1f} s  similarity {r['median_similarity']:.3f}  "
              f"near-identical {r['share_near_identical']:.0%}", file=sys.stderr)
    if a.frames_out:
        os.makedirs(a.frames_out, exist_ok=True)
        for i, f in enumerate(frames):
            Image.fromarray((f * 255).astype(np.uint8)).save(os.path.join(a.frames_out, f"{i:05d}.png"))


if __name__ == "__main__":
    main()
