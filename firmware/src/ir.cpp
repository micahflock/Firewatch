#include "ir.h"

#include "config.h"

#if IR_LED_PIN >= 0
#include <IRremoteESP8266.h>
#include <IRsend.h>

namespace {
// Roku TV NEC codes (32-bit, IRremoteESP8266 bit order). Discrete on/off exist on
// TCL Roku TVs; confirm with an IR receiver if yours ignores them.
constexpr uint32_t kRokuPowerOn = 0x57E316E9;
constexpr uint32_t kRokuPowerOff = 0x57E318E7;
IRsend irsend(IR_LED_PIN);
}  // namespace

namespace ir {
void begin() { irsend.begin(); }
void powerOn() { irsend.sendNEC(kRokuPowerOn, 32, 1); }
void powerOff() { irsend.sendNEC(kRokuPowerOff, 32, 1); }
}  // namespace ir

#else

namespace ir {
void begin() {}
void powerOn() {}
void powerOff() {}
}  // namespace ir

#endif
