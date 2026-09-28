#pragma once
// Skeumo BLE link: a battery "talisman" wakes, broadcasts one signed frame in a short
// burst of non-connectable BLE advertisements, and goes back to deep sleep. A powered
// "host" controller (e.g. Firewatch) scans for these frames, acts on them directly, and
// relays them into Apple Home as button presses.
//
// The frame travels as BLE manufacturer-specific data:
//
//   company ID  0xFFFF (little endian; the Bluetooth SIG ID reserved for internal/test use)
//   [0..1]  magic 'S' 'K'
//   [2]     version (1)
//   [3]     device ID      which talisman this is (a host may listen to several)
//   [4]     channel        which input: 1..N = intents (buttons), 0 = heartbeat (no intent)
//   [5]     press          0 = single, 1 = double, 2 = long (HomeKit's ProgrammableSwitchEvent)
//   [6]     battery %      0..100, 255 = unknown
//   [7..10] counter        little endian; strictly increasing per device (replay protection)
//   [11..18] tag           first 8 bytes of HMAC-SHA256(key, bytes 0..10)
//
// A talisman repeats the same frame for a few hundred milliseconds; the host accepts the
// first copy (counter > last accepted) and ignores the rest.

#include <array>
#include <cstddef>
#include <cstdint>

namespace skeumo {
namespace link {

constexpr uint16_t kCompanyId = 0xFFFF;
constexpr uint8_t kVersion = 1;
constexpr size_t kKeyLen = 16;
constexpr size_t kTagLen = 8;
constexpr size_t kBodyLen = 11;                     // bytes covered by the tag
constexpr size_t kFrameLen = kBodyLen + kTagLen;    // 19
constexpr size_t kMfgDataLen = 2 + kFrameLen;       // 21, including the company ID
constexpr uint8_t kBatteryUnknown = 255;

enum class Press : uint8_t { Single = 0, Double = 1, Long = 2 };

struct Message {
  uint8_t deviceId = 1;
  uint8_t channel = 0;  // 0 = heartbeat
  Press press = Press::Single;
  uint8_t batteryPct = kBatteryUnknown;
  uint32_t counter = 0;
};

using Key = std::array<uint8_t, kKeyLen>;
using MfgData = std::array<uint8_t, kMfgDataLen>;

enum class Result : uint8_t { Ok, NotOurs, BadVersion, BadTag };

// Builds the manufacturer data (company ID first) for one message.
MfgData encode(const Key& key, const Message& m);

// Parses manufacturer data as delivered by a BLE stack (company ID first). NotOurs means
// "some other device's advertisement": ignore it silently.
Result decode(const Key& key, const uint8_t* data, size_t len, Message& out);

// Parses a 32-hex-digit key (as kept in secrets.h). Returns false if malformed.
bool parseKey(const char* hex, Key& out);

// Replay protection for one device: accepts only counters above the last accepted one.
// The host persists last() across reboots; reset() is for re-flashing a talisman.
class ReplayGuard {
 public:
  explicit ReplayGuard(uint32_t last = 0) : last_(last) {}
  bool accept(uint32_t counter) {
    if (counter <= last_) return false;
    last_ = counter;
    return true;
  }
  uint32_t last() const { return last_; }
  void reset() { last_ = 0; }

 private:
  uint32_t last_;
};

}  // namespace link
}  // namespace skeumo
