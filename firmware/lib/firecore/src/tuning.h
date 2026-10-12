#pragma once
#include <cstddef>

#include "fire.h"

namespace fw {

// The FireParams a person can adjust live from the web page. The table is the single
// source for the page's sliders, the /tune endpoint and what gets saved across reboots;
// defaults come from FireParams itself.
struct Tunable {
  const char* key;  // URL argument and saved-settings key (at most 13 characters)
  const char* group;
  const char* label;
  const char* hint;
  float min, max, step;
  float FireParams::*field;
};

extern const Tunable kTunables[];
extern const size_t kTunableCount;

const Tunable* findTunable(const char* key);

// Clamps v to the tunable's range and stores it. Returns false for an unknown key.
bool setTunable(FireParams& p, const char* key, float v);

}  // namespace fw
