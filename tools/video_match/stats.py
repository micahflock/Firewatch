"""Shared statistics for comparing the video's light with the LED fire engine.

Both sides are reduced to four edge zones (left, right, top, bottom) plus a total, in
linear light, sampled at independent moments. The numbers describe *how the light
behaves* (steadiness, where it varies, how zones co-vary, color), not specific frames.
"""
import numpy as np

WARM_WHITE = np.array([1.0, 0.72, 0.42])  # 3000K die as linear RGB (display approximation)
LUMA = np.array([0.2126, 0.7152, 0.0722])


def summarize(zones, rgb_weighted):
    """zones: dict name -> 1-D array of linear luminance per sample. rgb_weighted: 3-vector."""
    t = zones["total"]
    out = {
        "total_cv": float(t.std() / t.mean()),
        "total_p95_over_p5": float(np.percentile(t, 95) / np.percentile(t, 5)),
    }
    for z in ("left", "right", "top", "bottom"):
        v = zones[z]
        out[f"{z}_cv"] = float(v.std() / v.mean())
    b = zones["bottom"].mean()
    for z in ("left", "right", "top"):
        out[f"{z}_over_bottom"] = float(zones[z].mean() / b)
    out["corr_left_right"] = float(np.corrcoef(zones["left"], zones["right"])[0, 1])
    out["corr_top_bottom"] = float(np.corrcoef(zones["top"], zones["bottom"])[0, 1])
    c = rgb_weighted / rgb_weighted[0]
    out["color_g_over_r"] = float(c[1])
    out["color_b_over_r"] = float(c[2])
    return out


def video_zones(frames_srgb):
    """frames_srgb: (N, H, W, 3) floats 0..1. Zones are the outer thirds of the picture."""
    lin = np.clip(frames_srgb, 0, 1) ** 2.2
    Y = lin @ LUMA
    N, H, W = Y.shape
    zones = {
        "total": Y.mean(axis=(1, 2)),
        "left": Y[:, :, : W // 3].mean(axis=(1, 2)),
        "right": Y[:, :, 2 * W // 3 :].mean(axis=(1, 2)),
        "top": Y[:, : H // 3].mean(axis=(1, 2)),
        "bottom": Y[:, 2 * H // 3 :].mean(axis=(1, 2)),
    }
    rgb = (lin * Y[..., None]).sum(axis=(0, 1, 2)) / Y.sum()
    return zones, rgb


def led_zones(pos, frames_rgbw):
    """pos: (n, 2) LED x,y (front view). frames_rgbw: (N, n, 4) linear LED duty."""
    rgb = frames_rgbw[..., :3] + frames_rgbw[..., 3:4] * WARM_WHITE
    Y = rgb @ LUMA
    x, y = pos[:, 0], pos[:, 1]
    sel = {
        "left": x < 0.02, "right": x > 0.98, "top": y > 0.98, "bottom": y < 0.02,
    }
    zones = {"total": Y.mean(axis=1)}
    for k, m in sel.items():
        zones[k] = Y[:, m].mean(axis=1)
    w = (rgb * Y[..., None]).sum(axis=(0, 1)) / Y.sum()
    return zones, w


def read_preview_dump(text):
    lines = text.strip().split("\n")
    n, frames = map(int, lines[0].split())
    pos = np.array([list(map(float, l.split())) for l in lines[1 : 1 + n]])
    data = np.array([list(map(float, l.split())) for l in lines[1 + n : 1 + n + n * frames]])
    return pos, data.reshape(frames, n, 4)
