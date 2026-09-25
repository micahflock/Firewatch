#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

#include "fire.h"

namespace fw {

struct OutputConfig {
  bool rgbw = true;
  // How the warm-white die is approximated on RGB-only strips (linear PWM ratios).
  float wToR = 1.00f, wToG = 0.55f, wToB = 0.18f;
  // Power model, per LED channel at 100% duty, used to cap supply current.
  float maPerChannel = 15.0f;
  float maIdlePerLed = 1.0f;
  float maController = 250.0f;
  float maBudget = 3000.0f;
};

// Brightness scaling, supply-current limiting and temporal dithering: 16-bit intent in,
// 8-bit wire values out. Dithering carries each channel's rounding error into the next
// frame, which keeps dim embers smooth instead of stepping between 8-bit levels.
class OutputStage {
 public:
  OutputStage(size_t leds, OutputConfig cfg) : cfg_(cfg), n_(leds), err_(leds * 4, 0) {}

  size_t channels() const { return cfg_.rgbw ? 4 : 3; }

  // out receives R,G,B[,W] per LED (logical order; the LED driver maps to wire order).
  void process(const Rgbw* in, float brightness, uint8_t* out) {
    float sum = 0;
    tmp_.resize(n_ * 4);
    for (size_t i = 0; i < n_; ++i) {
      Rgbw c = in[i];
      if (!cfg_.rgbw) {
        c.r += c.w * cfg_.wToR;
        c.g += c.w * cfg_.wToG;
        c.b += c.w * cfg_.wToB;
        c.w = 0;
      }
      float* t = &tmp_[i * 4];
      t[0] = clamp01(c.r * brightness);
      t[1] = clamp01(c.g * brightness);
      t[2] = clamp01(c.b * brightness);
      t[3] = clamp01(c.w * brightness);
      sum += t[0] + t[1] + t[2] + t[3];
    }
    float fixed = cfg_.maController + cfg_.maIdlePerLed * n_;
    float variable = sum * cfg_.maPerChannel;
    scale_ = 1.0f;
    if (fixed + variable > cfg_.maBudget && variable > 0) {
      scale_ = (cfg_.maBudget - fixed) / variable;
      if (scale_ < 0) scale_ = 0;
    }
    currentMa_ = fixed + variable * scale_;

    const size_t ch = channels();
    for (size_t i = 0; i < n_; ++i) {
      for (size_t k = 0; k < ch; ++k) {
        // 8.8 fixed point: full scale 255.0 == 65280.
        uint32_t v16 = static_cast<uint32_t>(tmp_[i * 4 + k] * scale_ * 65280.0f + 0.5f);
        uint8_t& e = err_[i * 4 + k];
        uint32_t acc = v16 + e;
        uint32_t hi = acc >> 8;
        e = static_cast<uint8_t>(acc & 0xFF);
        out[i * ch + k] = static_cast<uint8_t>(hi > 255 ? 255 : hi);
      }
    }
  }

  float currentMa() const { return currentMa_; }
  float limitScale() const { return scale_; }

 private:
  static float clamp01(float v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }
  OutputConfig cfg_;
  size_t n_;
  std::vector<uint8_t> err_;
  std::vector<float> tmp_;
  float currentMa_ = 0, scale_ = 1;
};

// Eased on/off envelope so the glow swells up and dies down instead of snapping.
class Fader {
 public:
  Fader(float fadeInS, float fadeOutS) : in_(fadeInS), out_(fadeOutS) {}
  void setTarget(bool on) { target_ = on; }
  bool target() const { return target_; }
  float step(float dtS) {
    if (target_) {
      level_ += dtS / in_;
      if (level_ > 1) level_ = 1;
    } else {
      level_ -= dtS / out_;
      if (level_ < 0) level_ = 0;
    }
    return value();
  }
  float value() const { return level_ * level_ * (3 - 2 * level_); }
  bool idle() const { return !target_ && level_ <= 0; }

 private:
  float in_, out_;
  float level_ = 0;
  bool target_ = false;
};

}  // namespace fw
