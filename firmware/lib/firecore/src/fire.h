#pragma once
#include <cstdint>
#include <vector>

#include "layout.h"

namespace fw {

// Linear-light color, each channel 0..1 (values above 1 are clipped on output).
// w is the strip's dedicated warm-white die; for RGB-only strips it is folded into RGB.
struct Rgbw {
  float r = 0, g = 0, b = 0, w = 0;
};

// Defaults are fitted to the fireplace video's light statistics (steadiness, where it
// varies, zone correlation, color) by tools/video_match/fit_params.py.
struct FireParams {
  // Heat at the bottom and top of the TV; light spill is strongest low and centered,
  // like a log fire sitting at the bottom of the screen.
  float heatBottom = 1.02f;
  float heatTop = 0.72f;
  float centerBias = 0.19f;  // heat lost toward the left/right corners

  // How strongly everything below flickers, at the bottom edge vs the top edge. A log fire
  // is steady at the ember bed and lively where the flame tips reach.
  float flickerBottom = 0.13f, flickerTop = 0.73f;

  // Whole-fire flicker: three bands of smooth noise (slow sway, mid flicker, fast lick).
  float flickerSlow = 0.18f, flickerMid = 0.10f, flickerFast = 0.05f;
  float rateSlow = 0.9f, rateMid = 2.7f, rateFast = 7.0f;  // noise units per second

  // Spatial texture that drifts upward, so brightness ripples climb the sides.
  float localAmount = 1.20f;
  float riseSpeed = 0.8f;
  float evolveSpeed = 0.35f;
  float scaleX = 2.2f, scaleY = 1.6f;

  // "Breathing": the fire slowly burning down and catching again over ~minutes.
  float breathAmount = 0.06f;
  float breathRate = 0.025f;

  // Flares: a log catching. Mean interval, strength range, rise and decay.
  float flareMeanIntervalS = 7.0f;
  float flareMin = 0.10f, flareMax = 0.19f;
  float flareRiseS = 0.25f, flareDecayS = 1.2f;
};

class FireEngine {
 public:
  FireEngine(std::vector<LedPos> layout, uint32_t seed);

  // Advance to absolute time tSeconds (monotonic) and fill `out` (size = LED count).
  void render(double tSeconds, Rgbw* out);

  size_t size() const { return layout_.size(); }
  FireParams params;

  // Exposed for tests/preview.
  static float noise3(float x, float y, float z);
  static Rgbw heatToColor(float heat);

 private:
  struct Flare {
    float x0 = 0.5f, strength = 0;
    double start = -1e9;
  };
  float rand01();
  void updateFlares(double t);

  std::vector<LedPos> layout_;
  uint32_t rng_;
  Flare flares_[3];
  double nextFlare_ = 2.0;
  double lastT_ = 0;
};

}  // namespace fw
