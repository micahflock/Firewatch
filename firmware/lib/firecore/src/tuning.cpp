#include "tuning.h"

#include <cmath>
#include <cstring>

namespace fw {

const Tunable kTunables[] = {
    {"red", "Color", "Red", "How much red", 0.0f, 1.5f, 0.05f, &FireParams::red},
    {"orange", "Color", "Orange", "Adds green to the red: more is more orange, less is deeper red", 0.0f,
     3.0f, 0.05f, &FireParams::orange},
    {"white", "Color", "Warm white", "The strip's white die: softens the color toward peach", 0.0f, 4.0f,
     0.05f, &FireParams::white},

    {"heatLow", "Highs and lows", "Glow, lower sides",
     "The ember bed's level: brighter and more orange as it goes up", 0.3f, 1.3f, 0.01f,
     &FireParams::heatBottom},
    {"heatTop", "Highs and lows", "Glow, top", "The flame tips' level, across the top", 0.3f, 1.3f, 0.01f,
     &FireParams::heatTop},
    {"flickLow", "Highs and lows", "Flicker, lower sides",
     "How far the lower sides swing between dim and bright", 0.0f, 1.0f, 0.01f, &FireParams::flickerBottom},
    {"flickTop", "Highs and lows", "Flicker, top", "How far the top swings between dim and bright", 0.0f,
     1.5f, 0.01f, &FireParams::flickerTop},

    {"speed", "Motion", "Speed", "How fast everything moves", 0.25f, 3.0f, 0.05f, &FireParams::speed},
    {"flareGap", "Motion", "Seconds between flares", "Average wait between flares (a log catching)", 1.0f,
     60.0f, 0.5f, &FireParams::flareMeanIntervalS},
    {"flareMax", "Motion", "Flare strength", "How bright the strongest flares get", 0.0f, 1.0f, 0.01f,
     &FireParams::flareMax},
};
const size_t kTunableCount = sizeof(kTunables) / sizeof(kTunables[0]);

const Tunable* findTunable(const char* key) {
  for (const Tunable& t : kTunables)
    if (std::strcmp(t.key, key) == 0) return &t;
  return nullptr;
}

bool setTunable(FireParams& p, const char* key, float v) {
  const Tunable* t = findTunable(key);
  if (!t) return false;
  if (!std::isfinite(v)) v = FireParams{}.*(t->field);
  p.*(t->field) = std::fmin(t->max, std::fmax(t->min, v));
  return true;
}

}  // namespace fw
