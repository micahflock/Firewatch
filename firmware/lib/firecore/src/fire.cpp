#include "fire.h"

#include <cmath>

namespace fw {
namespace {

// Ken Perlin's reference permutation; the 256-periodicity lets time wrap seamlessly.
const uint8_t kPerm[256] = {
    151, 160, 137, 91,  90,  15,  131, 13,  201, 95,  96,  53,  194, 233, 7,   225, 140, 36,  103, 30,  69,
    142, 8,   99,  37,  240, 21,  10,  23,  190, 6,   148, 247, 120, 234, 75,  0,   26,  197, 62,  94,  252,
    219, 203, 117, 35,  11,  32,  57,  177, 33,  88,  237, 149, 56,  87,  174, 20,  125, 136, 171, 168, 68,
    175, 74,  165, 71,  134, 139, 48,  27,  166, 77,  146, 158, 231, 83,  111, 229, 122, 60,  211, 133, 230,
    220, 105, 92,  41,  55,  46,  245, 40,  244, 102, 143, 54,  65,  25,  63,  161, 1,   216, 80,  73,  209,
    76,  132, 187, 208, 89,  18,  169, 200, 196, 135, 130, 116, 188, 159, 86,  164, 100, 109, 198, 173, 186,
    3,   64,  52,  217, 226, 250, 124, 123, 5,   202, 38,  147, 118, 126, 255, 82,  85,  212, 207, 206, 59,
    227, 47,  16,  58,  17,  182, 189, 28,  42,  223, 183, 170, 213, 119, 248, 152, 2,   44,  154, 163, 70,
    221, 153, 101, 155, 167, 43,  172, 9,   129, 22,  39,  253, 19,  98,  108, 110, 79,  113, 224, 232, 178,
    185, 112, 104, 218, 246, 97,  228, 251, 34,  242, 193, 238, 210, 144, 12,  191, 179, 162, 241, 81,  51,
    145, 235, 249, 14,  239, 107, 49,  192, 214, 31,  181, 199, 106, 157, 184, 84,  204, 176, 115, 121, 50,
    45,  127, 4,   150, 254, 138, 236, 205, 93,  222, 114, 67,  29,  24,  72,  243, 141, 128, 195, 78,  66,
    215, 61,  156, 180};

inline int P(int i) { return kPerm[i & 255]; }
inline float fade(float t) { return t * t * t * (t * (t * 6 - 15) + 10); }
inline float lerp(float a, float b, float t) { return a + (b - a) * t; }
inline float grad(int h, float x, float y, float z) {
  h &= 15;
  float u = h < 8 ? x : y;
  float v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
  return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
}
inline float wrap256(double v) { return static_cast<float>(v - 256.0 * std::floor(v / 256.0)); }
inline float clamp01(float v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }
inline float smoothstep(float t) {
  t = clamp01(t);
  return t * t * (3 - 2 * t);
}

struct Stop {
  float heat;
  Rgbw c;
};
// Linear-light values (LED PWM duty), not sRGB. Red carries the glow and green alone sets
// how orange it is. Warm white only enters on flares: on the strip the white die outshines
// the red one, so even a little of it washes the glow out to peach-white on dark brick.
const Stop kPalette[] = {
    {0.00f, {0.000f, 0.000f, 0.000f, 0.000f}},
    {0.20f, {0.140f, 0.006f, 0.000f, 0.000f}},  // deep ember
    {0.45f, {0.420f, 0.030f, 0.000f, 0.000f}},  // ember red
    {0.70f, {0.700f, 0.090f, 0.000f, 0.010f}},  // red-orange
    {0.90f, {0.900f, 0.170f, 0.000f, 0.030f}},  // orange
    {1.10f, {1.000f, 0.260f, 0.000f, 0.070f}},  // deep amber
    {1.40f, {1.000f, 0.380f, 0.000f, 0.250f}},  // flare: hot amber
};
constexpr int kStops = sizeof(kPalette) / sizeof(kPalette[0]);

}  // namespace

float FireEngine::noise3(float x, float y, float z) {
  float fx = std::floor(x), fy = std::floor(y), fz = std::floor(z);
  int X = static_cast<int>(fx) & 255, Y = static_cast<int>(fy) & 255, Z = static_cast<int>(fz) & 255;
  x -= fx;
  y -= fy;
  z -= fz;
  float u = fade(x), v = fade(y), w = fade(z);
  int A = P(X) + Y, AA = P(A) + Z, AB = P(A + 1) + Z;
  int B = P(X + 1) + Y, BA = P(B) + Z, BB = P(B + 1) + Z;
  return lerp(lerp(lerp(grad(P(AA), x, y, z), grad(P(BA), x - 1, y, z), u),
                   lerp(grad(P(AB), x, y - 1, z), grad(P(BB), x - 1, y - 1, z), u), v),
              lerp(lerp(grad(P(AA + 1), x, y, z - 1), grad(P(BA + 1), x - 1, y, z - 1), u),
                   lerp(grad(P(AB + 1), x, y - 1, z - 1), grad(P(BB + 1), x - 1, y - 1, z - 1), u), v),
              w);
}

Rgbw FireEngine::heatToColor(float heat) {
  if (heat <= kPalette[0].heat) return kPalette[0].c;
  for (int i = 1; i < kStops; ++i) {
    if (heat <= kPalette[i].heat) {
      const Stop& a = kPalette[i - 1];
      const Stop& b = kPalette[i];
      float t = (heat - a.heat) / (b.heat - a.heat);
      return {lerp(a.c.r, b.c.r, t), lerp(a.c.g, b.c.g, t), lerp(a.c.b, b.c.b, t), lerp(a.c.w, b.c.w, t)};
    }
  }
  return kPalette[kStops - 1].c;
}

FireEngine::FireEngine(std::vector<LedPos> layout, uint32_t seed)
    : layout_(std::move(layout)), rng_(seed ? seed : 0x9E3779B9u) {}

float FireEngine::rand01() {
  rng_ ^= rng_ << 13;
  rng_ ^= rng_ >> 17;
  rng_ ^= rng_ << 5;
  return (rng_ >> 8) * (1.0f / 16777216.0f);
}

void FireEngine::updateFlares(double t) {
  if (t < lastT_) nextFlare_ = t;  // time went backwards (tests/preview restart)
  lastT_ = t;
  if (t < nextFlare_) return;
  // Exponentially distributed gaps read as random, never as a rhythm.
  nextFlare_ = t - std::log(1.0f - rand01() * 0.999f) * params.flareMeanIntervalS;
  Flare* slot = &flares_[0];
  for (Flare& f : flares_)
    if (f.start < slot->start) slot = &f;
  slot->start = t;
  slot->x0 = 0.2f + 0.6f * rand01();
  float lo = std::fmin(params.flareMin, params.flareMax);
  slot->strength = lo + (params.flareMax - lo) * rand01();
}

void FireEngine::render(double realT, Rgbw* out) {
  const FireParams& p = params;
  if (!started_ || realT < lastRealT_) {
    simT_ = realT;  // first frame, or time went backwards (tests/preview restart)
  } else {
    simT_ += (realT - lastRealT_) * p.speed;
  }
  started_ = true;
  lastRealT_ = realT;
  const double t = simT_;
  updateFlares(t);

  float global = p.flickerSlow * noise3(wrap256(t * p.rateSlow), 0.5f, 0.5f) +
                 p.flickerMid * noise3(wrap256(t * p.rateMid), 10.5f, 0.5f) +
                 p.flickerFast * noise3(wrap256(t * p.rateFast), 20.5f, 0.5f);
  float breath = 1.0f + p.breathAmount * noise3(wrap256(t * p.breathRate), 30.5f, 0.5f);

  float rise = wrap256(t * p.riseSpeed);
  float evolve = wrap256(t * p.evolveSpeed);

  float flareEnv[3];
  for (int i = 0; i < 3; ++i) {
    float dt = static_cast<float>(t - flares_[i].start);
    flareEnv[i] = (dt < 0 || dt > 8 * p.flareDecayS)
                      ? 0.0f
                      : flares_[i].strength * smoothstep(dt / p.flareRiseS) * std::exp(-dt / p.flareDecayS);
  }

  for (size_t i = 0; i < layout_.size(); ++i) {
    float x = layout_[i].x, y = layout_[i].y;
    float cx = 2 * x - 1;
    float base = p.heatBottom + (p.heatTop - p.heatBottom) * y - p.centerBias * cx * cx;
    float amp = p.flickerBottom + (p.flickerTop - p.flickerBottom) * y;
    float local = noise3(x * p.scaleX, y * p.scaleY - rise, evolve);
    float heat = base * breath * (1.0f + amp * (global + p.localAmount * local));
    for (int f = 0; f < 3; ++f) {
      if (flareEnv[f] <= 0) continue;
      float d = (x - flares_[f].x0) / 0.25f;
      heat += amp * flareEnv[f] * std::exp(-d * d);
    }
    Rgbw c = heatToColor(heat);
    out[i] = {c.r * p.red, c.g * p.orange, c.b, c.w * p.white};
  }
}

}  // namespace fw
