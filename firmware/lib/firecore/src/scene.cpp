#include "scene.h"

namespace fw {

void Scene::startLaunch(bool force) {
  launchPending_ = true;
  launchForce_ = force;
  launchAttempts_ = 0;
}

void Scene::serviceLaunch(const TvStatus& s, uint32_t now, Actions& a) {
  if (!launchPending_ || state_ != State::On || s.power != TvPower::On) return;
  if (!launchForce_ && s.fireAppActive) {
    launchPending_ = false;
    event_ = "fire playing";
    return;
  }
  if (launchAttempts_ > 0 && now - lastLaunchMs_ < t_.launchRetryMs) return;
  if (launchAttempts_ >= t_.maxLaunchAttempts) {
    launchPending_ = false;
    event_ = "launch failed";
    return;
  }
  a.launch = true;
  ++launchAttempts_;
  lastLaunchMs_ = now;
  launchForce_ = false;
  event_ = "launching fire";
}

Actions Scene::command(Command c, uint32_t now) {
  Actions a;
  if (c == Command::Toggle) c = state_ == State::Off ? Command::On : Command::Off;

  switch (c) {
    case Command::On:
      if (state_ == State::Off) {
        if (tvOn_) {
          state_ = State::On;
          tvSeenOnInScene_ = true;
          startLaunch(false);
          event_ = "scene on";
        } else {
          state_ = State::Waking;
          tvSeenOnInScene_ = false;
          wakeStartMs_ = lastWakeMs_ = now;
          a.wake = true;
          event_ = "waking TV";
        }
      } else if (tvOn_) {
        startLaunch(false);
      }
      break;

    case Command::Off:
      if (tvOn_ || state_ == State::Waking) a.powerOff = true;
      state_ = State::Off;
      launchPending_ = false;
      tvSeenOnInScene_ = false;
      event_ = "scene off";
      break;

    case Command::Relaunch:
      if (!tvOn_) return command(Command::On, now);
      if (state_ != State::On) {
        state_ = State::On;
        tvSeenOnInScene_ = true;
      }
      startLaunch(true);
      break;

    case Command::Toggle:
      break;
  }
  return a;
}

Actions Scene::observe(const TvStatus& s, uint32_t now) {
  Actions a;
  const bool first = !observed_;
  const bool wasOn = tvOn_;
  observed_ = true;

  switch (s.power) {
    case TvPower::On:
      unreachable_ = 0;
      tvOn_ = true;
      break;
    case TvPower::Standby:
      unreachable_ = 0;
      tvOn_ = false;
      break;
    case TvPower::Unreachable:
      // A single dropped poll must not kill the fire; without fast start the TV's
      // network is down in standby, so persistent silence means "off".
      if (unreachable_ < 255) ++unreachable_;
      if (unreachable_ >= t_.unreachableToOff) tvOn_ = false;
      break;
  }

  const bool rising = tvOn_ && (first || !wasOn);
  const bool falling = !tvOn_ && wasOn;
  // A rising edge seen at boot means the ESP32 restarted under a running TV: only launch
  // if the fire isn't already playing. A real power-on always relaunches, because YouTube
  // may still be resident but parked on its home screen.
  const bool forceLaunch = rising && !first;

  switch (state_) {
    case State::Off:
      if (rising && t_.followTv) {
        state_ = State::On;
        tvSeenOnInScene_ = true;
        startLaunch(forceLaunch);
        event_ = "TV turned on";
      }
      break;

    case State::Waking:
      if (tvOn_) {
        state_ = State::On;
        tvSeenOnInScene_ = true;
        startLaunch(forceLaunch);
        event_ = "TV awake";
      } else if (now - wakeStartMs_ >= t_.wakeTimeoutMs) {
        state_ = State::On;
        event_ = "TV did not wake; lights only";
      } else if (now - lastWakeMs_ >= t_.wakeRetryMs) {
        a.wake = true;
        lastWakeMs_ = now;
      }
      break;

    case State::On:
      if (rising) {
        tvSeenOnInScene_ = true;
        startLaunch(forceLaunch);
      }
      if (falling && tvSeenOnInScene_ && t_.followTv) {
        state_ = State::Off;
        launchPending_ = false;
        tvSeenOnInScene_ = false;
        event_ = "TV turned off";
      }
      break;
  }

  serviceLaunch(s, now, a);
  return a;
}

}  // namespace fw
