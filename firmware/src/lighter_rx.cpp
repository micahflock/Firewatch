#include "lighter_rx.h"

#include <NimBLEDevice.h>
#include <Preferences.h>

#include <atomic>
#include <mutex>

namespace lighter_rx {
namespace {

skeumo::link::Key key;
uint8_t deviceId = 1;
void (*onFrame)(const skeumo::link::Message&) = nullptr;
Preferences prefs;
skeumo::link::ReplayGuard guard;
std::atomic<bool> resetPending{false};

std::mutex statsMutex;
Stats current;

class Listener : public NimBLEAdvertisedDeviceCallbacks {
  void onResult(NimBLEAdvertisedDevice* d) override {
    if (!d->haveManufacturerData()) return;
    const std::string data = d->getManufacturerData();
    skeumo::link::Message m;
    const auto r = skeumo::link::decode(key, reinterpret_cast<const uint8_t*>(data.data()), data.size(), m);
    if (r == skeumo::link::Result::NotOurs) return;  // someone else's advertisement
    if (r != skeumo::link::Result::Ok) {
      std::lock_guard<std::mutex> lock(statsMutex);
      ++current.rejected;
      return;
    }
    if (m.deviceId != deviceId) return;

    if (resetPending.exchange(false)) guard.reset();
    if (!guard.accept(m.counter)) return;  // the rest of this burst, or a replay
    prefs.putUInt("ctr", m.counter);

    {
      std::lock_guard<std::mutex> lock(statsMutex);
      current.counter = m.counter;
      current.lastSeenMs = millis();
      current.lastChannel = m.channel;
      if (m.batteryPct <= 100) current.batteryPct = m.batteryPct;
      current.rssi = static_cast<int8_t>(d->getRSSI());
    }
    Serial.printf("[lighter] channel %u counter %u battery %u%% rssi %d\n", m.channel, m.counter, m.batteryPct,
                  d->getRSSI());
    onFrame(m);
  }
};

Listener listener;

}  // namespace

void begin(const skeumo::link::Key& k, uint8_t id, void (*cb)(const skeumo::link::Message&)) {
  key = k;
  deviceId = id;
  onFrame = cb;
  prefs.begin("skeumo", false);
  guard = skeumo::link::ReplayGuard(prefs.getUInt("ctr", 0));
  current.counter = guard.last();

  NimBLEDevice::init("");
  NimBLEScan* scan = NimBLEDevice::getScan();
  scan->setAdvertisedDeviceCallbacks(&listener, true);  // every packet; dedupe is by counter
  scan->setDuplicateFilter(false);
  scan->setActiveScan(false);  // just listen; never transmit
  // 30 ms of every 100 ms: the lighter repeats its frame every ~20 ms for 400 ms, so it's
  // caught within a few tenths of a second, while Wi-Fi keeps most of the shared radio.
  scan->setInterval(100);
  scan->setWindow(30);
  scan->setMaxResults(0);  // don't accumulate results in RAM
  scan->start(0, nullptr, false);
}

void resetCounter() {
  resetPending = true;
  prefs.putUInt("ctr", 0);
  std::lock_guard<std::mutex> lock(statsMutex);
  current.counter = 0;
}

Stats stats() {
  std::lock_guard<std::mutex> lock(statsMutex);
  return current;
}

}  // namespace lighter_rx
