#!/usr/bin/env python3
"""Fit FireParams so the LED glow's light statistics match a video's.

    python3 fit_params.py video_stats.json [--preview ../preview/preview] [--iters 300]

Scores the engine (via the preview binary, sampled every 10 s over 3 simulated hours)
against the target stats and random-searches the parameters that shape *where* and *how
much* the light varies. Flicker speeds can't be learned from 10-second storyboard
samples, so rates are left at their defaults. Color is set by the palette in fire.cpp
and only lightly weighted here: a video's average light is often near-white because the
camera clips the flames, and matching it exactly makes the glow look peach, not amber.
"""
import argparse, json, subprocess, sys
import numpy as np
from stats import led_zones, read_preview_dump, summarize

WEIGHTS = {
    "total_cv": 3, "low_cv": 3, "top_cv": 2, "left_cv": 1, "right_cv": 1,
    "top_over_low": 2, "left_over_low": 1, "right_over_low": 1,
    "corr_left_right": 1, "corr_top_low": 1, "color_g_over_r": 0.5, "color_b_over_r": 0,
}
SPACE = {  # name: (low, high)
    "heatTop": (0.5, 1.0), "heatBottom": (0.75, 1.05), "centerBias": (0.0, 0.3),
    "flickerBottom": (0.0, 0.6), "flickerTop": (0.6, 2.0),
    "localAmount": (0.2, 1.6), "breathAmount": (0.0, 0.1), "flareMax": (0.05, 0.6),
}


def run(binary, params, samples=1080):
    args = [binary, str(samples), "0.1"]
    for k, v in params.items():
        args.append(f"{k}={v:.4f}")
        if k == "flareMax":
            args.append(f"flareMin={v / 2:.4f}")
    out = subprocess.run(args, capture_output=True, text=True, check=True).stdout
    pos, frames = read_preview_dump(out)
    return summarize(*led_zones(pos, frames))


def score(stats, target):
    return sum(w * ((stats[k] - target[k]) / max(abs(target[k]), 0.05)) ** 2 for k, w in WEIGHTS.items())


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("target")
    ap.add_argument("--preview", default="../preview/preview")
    ap.add_argument("--iters", type=int, default=300)
    ap.add_argument("--seed", type=int, default=1)
    a = ap.parse_args()
    target = json.load(open(a.target))["stats"]
    rng = np.random.default_rng(a.seed)
    # Start from the firmware's current defaults (fire.h).
    best_p = {"heatTop": 0.83, "heatBottom": 0.85, "centerBias": 0.05, "flickerBottom": 0.05,
              "flickerTop": 0.84, "localAmount": 0.99, "breathAmount": 0.036, "flareMax": 0.24}
    best_s = run(a.preview, best_p)
    best = score(best_s, target)
    print(f"defaults: score {best:.2f}", file=sys.stderr)
    for i in range(a.iters):
        # Wide random draws first, then shrink toward the best point found.
        spread = max(0.05, 1.0 - i / (0.7 * a.iters))
        p = {}
        for k, (lo, hi) in SPACE.items():
            v = best_p[k] + rng.normal(0, spread * (hi - lo) / 2)
            p[k] = float(np.clip(v, lo, hi))
        s = run(a.preview, p)
        sc = score(s, target)
        if sc < best:
            best, best_p, best_s = sc, p, s
            print(f"iter {i}: score {best:.2f}", file=sys.stderr)
    print(json.dumps({"score": best, "params": best_p, "stats": best_s}, indent=1))
    print("\n%-20s %8s %8s" % ("metric", "target", "fitted"), file=sys.stderr)
    for k in WEIGHTS:
        print("%-20s %8.3f %8.3f" % (k, target[k], best_s[k]), file=sys.stderr)


if __name__ == "__main__":
    main()
