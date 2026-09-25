#pragma once
#include <cstddef>
#include <cstdint>

// Pushes one frame of logical R,G,B[,W] bytes (see OutputStage) to the strip.
namespace leds {
void begin(size_t count);
bool ready();  // previous DMA transfer finished
void show(const uint8_t* rgbw);
}  // namespace leds
