#pragma once
#include <cstdint>

namespace fw {

// What Apple Home shows for the Fireplace light.
struct HomeState {
  bool on = false;
  int brightness = 0;  // 0-100

  bool operator==(const HomeState& o) const { return on == o.on && brightness == o.brightness; }
  bool operator!=(const HomeState& o) const { return !(*this == o); }
};

// Keeps Apple Home's view of the scene honest without fighting the person using it. Free of
// I/O so it can be unit-tested:
//  - Anything else that changes the scene (remote, web page, button, the TV's auto-off) is
//    reported to Home.
//  - After Home writes, reports are held back until the device matches or holdMs passes, so
//    the tile doesn't flip back while the network task is busy waking the TV.
//  - A Home write that matches the scene's on/off is not a command: sending On again would
//    relaunch the fire over whatever the TV is showing.
class HomeSync {
 public:
  explicit HomeSync(uint32_t holdMs = 5000) : holdMs_(holdMs) {}

  // Home's view when the accessory is created.
  void seed(HomeState s) {
    shown_ = s;
    holding_ = false;
  }

  // Home now shows `wanted`; the device is at `actual`. Returns true when the scene's on/off
  // must change to wanted.on: against the device's state, or against Home's previous write if
  // that one is still being carried out.
  bool homeWrote(HomeState wanted, HomeState actual, uint32_t nowMs);

  // Returns true when Home must be told `out` (and from then on shows it).
  bool poll(HomeState actual, uint32_t nowMs, HomeState& out);

 private:
  uint32_t holdMs_;
  HomeState shown_;
  bool holding_ = false;
  uint32_t holdStartMs_ = 0;
};

// HomeSpan halts the board at boot on a setup code it rejects, so check it at compile time:
// exactly 8 digits, not all the same digit, and not 12345678 or 87654321.
constexpr bool validSetupCode(const char* s) {
  for (int i = 0; i < 8; ++i)
    if (s[i] < '0' || s[i] > '9') return false;
  if (s[8] != '\0') return false;
  bool allSame = true;
  for (int i = 1; i < 8; ++i) allSame = allSame && s[i] == s[0];
  auto is = [](const char* a, const char* b) {
    for (int i = 0; i < 8; ++i)
      if (a[i] != b[i]) return false;
    return true;
  };
  return !allSame && !is(s, "12345678") && !is(s, "87654321");
}

}  // namespace fw
