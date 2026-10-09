#include "home_sync.h"

namespace fw {

bool HomeSync::homeWrote(HomeState wanted, HomeState actual, uint32_t now) {
  shown_ = wanted;
  holding_ = true;
  holdStartMs_ = now;
  return wanted.on != actual.on;
}

bool HomeSync::poll(HomeState actual, uint32_t now, HomeState& out) {
  if (holding_) {
    if (actual != shown_ && now - holdStartMs_ < holdMs_) return false;
    holding_ = false;
  }
  if (actual == shown_) return false;
  shown_ = actual;
  out = actual;
  return true;
}

}  // namespace fw
