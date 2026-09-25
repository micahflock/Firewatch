#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

namespace fw {

// Coordinates are normalized to the TV as seen FROM THE FRONT:
// x = 0 at the viewer's left edge, 1 at the right; y = 0 at the bottom, 1 at the top.
// Standing behind the TV, left and right are swapped — wire accordingly.
struct Segment {
  float x0, y0;    // position of the first LED in this run
  float x1, y1;    // position of the last LED in this run
  uint16_t count;  // LEDs in this run, in data-chain order
};

struct LedPos {
  float x, y;
};

inline std::vector<LedPos> buildLayout(const Segment* segs, size_t n) {
  std::vector<LedPos> out;
  for (size_t s = 0; s < n; ++s) {
    const Segment& g = segs[s];
    for (uint16_t i = 0; i < g.count; ++i) {
      float t = (i + 0.5f) / g.count;
      out.push_back({g.x0 + (g.x1 - g.x0) * t, g.y0 + (g.y1 - g.y0) * t});
    }
  }
  return out;
}

}  // namespace fw
