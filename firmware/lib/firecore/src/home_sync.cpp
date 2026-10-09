#include "home_sync.h"

namespace fw {

bool HomeSync::homeWrote(HomeState wanted, HomeState actual, uint32_t now) {
  // While an earlier Home write is still being carried out, the device hasn't caught up yet:
  // compare against what Home last asked for, so a quick reversing tap isn't dropped.
  const bool pending = holding_ && now - holdStartMs_ < holdMs_;
  const bool baseline = pending ? shown_.on : actual.on;
  shown_ = wanted;
  holding_ = true;
  holdStartMs_ = now;
  return wanted.on != baseline;
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
