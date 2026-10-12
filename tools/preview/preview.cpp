// Renders the firmware's fire engine on the host and dumps raw frames for preview.py.
// Build (from this directory):
//   g++ -std=c++17 -O2 -I../../firmware/lib/firecore/src preview.cpp ../../firmware/lib/firecore/src/fire.cpp ../../firmware/lib/firecore/src/tuning.cpp -o preview
// Usage: ./preview FRAMES FPS [param=value ...]   e.g. ./preview 250 25 flickerTop=1.2
// Settings copied from the web page's "Show settings" work too, e.g. orange=1.60 flickTop=0.50.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "fire.h"
#include "output.h"
#include "tuning.h"

static bool setParam(fw::FireParams& p, const char* arg) {
  struct Field {
    const char* name;
    float* v;
  } fields[] = {
      {"heatBottom", &p.heatBottom},   {"heatTop", &p.heatTop},
      {"centerBias", &p.centerBias},   {"flickerBottom", &p.flickerBottom},
      {"flickerTop", &p.flickerTop},   {"flickerSlow", &p.flickerSlow},
      {"flickerMid", &p.flickerMid},   {"flickerFast", &p.flickerFast},
      {"rateSlow", &p.rateSlow},       {"rateMid", &p.rateMid},
      {"rateFast", &p.rateFast},       {"localAmount", &p.localAmount},
      {"riseSpeed", &p.riseSpeed},     {"evolveSpeed", &p.evolveSpeed},
      {"scaleX", &p.scaleX},           {"scaleY", &p.scaleY},
      {"breathAmount", &p.breathAmount}, {"breathRate", &p.breathRate},
      {"flareMeanIntervalS", &p.flareMeanIntervalS}, {"flareMin", &p.flareMin},
      {"flareMax", &p.flareMax},       {"flareRiseS", &p.flareRiseS},
      {"flareDecayS", &p.flareDecayS},
  };
  const char* eq = std::strchr(arg, '=');
  if (!eq) return false;
  for (auto& f : fields) {
    if (std::strlen(f.name) == static_cast<size_t>(eq - arg) && std::strncmp(arg, f.name, eq - arg) == 0) {
      *f.v = static_cast<float>(std::atof(eq + 1));
      return true;
    }
  }
  return false;
}

int main(int argc, char** argv) {
  const int frames = argc > 1 ? std::atoi(argv[1]) : 300;
  const double fps = argc > 2 ? std::atof(argv[2]) : 25.0;
  // Same default layout as firmware/include/config.h.
  const fw::Segment segs[] = {{0, 0, 0, 1, 17}, {0, 1, 1, 1, 38}, {1, 1, 1, 0, 17}};
  auto layout = fw::buildLayout(segs, 3);
  fw::FireEngine fire(layout, 42);
  for (int i = 3; i < argc; ++i) {
    const char* eq = std::strchr(argv[i], '=');
    bool fromPage = eq && fw::setTunable(fire.params, std::string(argv[i], eq - argv[i]).c_str(), std::atof(eq + 1));
    if (!fromPage && !setParam(fire.params, argv[i])) {
      std::fprintf(stderr, "unknown parameter: %s\n", argv[i]);
      return 2;
    }
  }
  std::vector<fw::Rgbw> px(layout.size());

  std::printf("%zu %d\n", layout.size(), frames);
  for (auto& p : layout) std::printf("%.4f %.4f\n", p.x, p.y);
  for (int f = 0; f < frames; ++f) {
    fire.render(f / fps, px.data());
    for (auto& c : px) std::printf("%.4f %.4f %.4f %.4f\n", c.r, c.g, c.b, c.w);
  }
  return 0;
}
