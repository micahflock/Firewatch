#pragma once
#include <cstdint>

namespace fw {

enum class Command : uint8_t { On, Off, Toggle, Relaunch };

enum class TvPower : uint8_t { Unreachable, Standby, On };

struct TvStatus {
  TvPower power = TvPower::Unreachable;
  bool fireAppActive = false;  // active app is the configured fireplace app (YouTube)
};

// What the network layer should do after a command or observation.
struct Actions {
  bool wake = false;      // Wake-on-LAN + ECP PowerOn (+ IR power-on if fitted)
  bool powerOff = false;  // ECP PowerOff (+ IR power-off if fitted)
  bool launch = false;    // ECP launch of the fireplace video
};

struct SceneTiming {
  uint32_t wakeRetryMs = 6000;     // resend PowerOn while waiting for the TV
  uint32_t wakeTimeoutMs = 60000;  // give up waiting; lights stay on by themselves
  uint32_t launchRetryMs = 15000;  // YouTube cold start can take ~10 s
  uint8_t maxLaunchAttempts = 3;
  uint8_t unreachableToOff = 3;    // consecutive failed polls before "TV is off"
  bool followTv = true;            // TV power (e.g. the Roku remote) drives the scene
};

// The whole scene's policy, free of I/O so it can be unit-tested:
//  - Turning the scene on wakes the TV and launches the fire; the lights start at once.
//  - Turning the TV on by any means (remote, voice, app) starts the scene too.
//  - Turning the TV off ends the scene; turning the scene off turns the TV off.
class Scene {
 public:
  enum class State : uint8_t { Off, Waking, On };

  explicit Scene(SceneTiming t = {}) : t_(t) {}

  Actions command(Command c, uint32_t nowMs);
  Actions observe(const TvStatus& s, uint32_t nowMs);

  State state() const { return state_; }
  bool lightsOn() const { return state_ != State::Off; }
  bool tvOn() const { return tvOn_; }
  const char* lastEvent() const { return event_; }

 private:
  void startLaunch(bool force);
  void serviceLaunch(const TvStatus& s, uint32_t nowMs, Actions& a);

  SceneTiming t_;
  State state_ = State::Off;
  bool observed_ = false;
  bool tvOn_ = false;
  uint8_t unreachable_ = 0;
  bool tvSeenOnInScene_ = false;
  uint32_t wakeStartMs_ = 0, lastWakeMs_ = 0;
  bool launchPending_ = false, launchForce_ = false;
  uint8_t launchAttempts_ = 0;
  uint32_t lastLaunchMs_ = 0;
  const char* event_ = "boot";
};

}  // namespace fw
