#pragma once
// Apple Home, via HomeSpan. Firewatch appears as a bridge with up to two accessories:
//
//  - "Fireplace": a dimmable light that mirrors the scene. It's on whenever the glow is on,
//    however the fire was started (Roku remote, web page, lighter). Turning it on or off
//    from the Home app, Siri, a scene or an automation runs the same scene as /on and /off,
//    and its brightness is the web slider's. HomeSpan owns Wi-Fi (connect and reconnect)
//    and mDNS.
//  - "Lighter" (if LIGHTER_ENABLED): the BLE lighter's Strike and Snuff as stateless buttons,
//    plus its battery, so a strike can also run a Home scene (e.g. dim the room lights).

#include "link.h"

namespace homekit {

struct Hooks {
  void (*setFire)(bool on);        // Fireplace turned on or off from Apple Home
  bool (*fireOn)();                // is the scene on right now?
  void (*setBrightness)(int pct);  // Fireplace brightness changed from Apple Home
  int (*brightness)();             // the current brightness, 0-100
  void (*onWifiConnected)();       // first connection only, after mDNS is up
};

// Call once from setup(). HomeSpan joins Wi-Fi with these credentials. setupCode may be
// null or empty (HomeSpan's default code, 466-37-726, is used then).
void begin(const Hooks& hooks, const char* ssid, const char* password, const char* setupCode);

// Whether a Home app has paired with Firewatch.
bool paired();

// Relay a verified lighter frame into Apple Home. Safe to call from any task.
void lighterEvent(const skeumo::link::Message& m);

}  // namespace homekit
