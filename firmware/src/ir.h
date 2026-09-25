#pragma once

// Optional IR blaster (IR_LED_PIN in config.h). Uses the Roku TV discrete power codes,
// so repeating them is harmless. No-ops when IR_LED_PIN < 0.
namespace ir {
void begin();
void powerOn();
void powerOff();
}  // namespace ir
