// Renders the firmware's fire engine on the host and dumps raw frames for preview.py.
// Build (from this directory):
//   g++ -std=c++17 -O2 -I../../firmware/lib/firecore/src preview.cpp ../../firmware/lib/firecore/src/fire.cpp -o preview
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "fire.h"
#include "output.h"

int main(int argc, char** argv) {
  const int frames = argc > 1 ? std::atoi(argv[1]) : 300;
  const double fps = argc > 2 ? std::atof(argv[2]) : 25.0;
  // Same default layout as firmware/include/config.h.
  const fw::Segment segs[] = {
      {0, 0, 0, 1, 23}, {0, 1, 1, 1, 41}, {1, 1, 1, 0, 23}, {1, 0, 0, 0, 41}};
  auto layout = fw::buildLayout(segs, 4);
  fw::FireEngine fire(layout, 42);
  std::vector<fw::Rgbw> px(layout.size());

  std::printf("%zu %d\n", layout.size(), frames);
  for (auto& p : layout) std::printf("%.4f %.4f\n", p.x, p.y);
  for (int f = 0; f < frames; ++f) {
    fire.render(f / fps, px.data());
    for (auto& c : px) std::printf("%.4f %.4f %.4f %.4f\n", c.r, c.g, c.b, c.w);
  }
  return 0;
}
