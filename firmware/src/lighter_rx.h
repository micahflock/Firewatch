#pragma once
// Listens for the Skeumo fireplace lighter: a passive BLE scan for its signed frames
// (skeumo_link). Each verified, fresh frame is handed to the callback once, on the BLE
// host task. Copies from the same broadcast burst, replays and forgeries are dropped.

#include <Arduino.h>

#include "link.h"

namespace lighter_rx {

struct Stats {
  uint32_t counter = 0;      // last accepted frame counter
  uint32_t lastSeenMs = 0;   // millis() of the last accepted frame, 0 = never
  uint8_t lastChannel = 0;
  uint8_t batteryPct = skeumo::link::kBatteryUnknown;
  int8_t rssi = 0;
  uint32_t rejected = 0;     // frames with a bad tag or version (wrong key?)
};

void begin(const skeumo::link::Key& key, uint8_t deviceId, void (*onFrame)(const skeumo::link::Message&));

// Forget the last counter, e.g. after erasing and re-flashing the lighter.
void resetCounter();

Stats stats();

}  // namespace lighter_rx
