#include <unity.h>

#include <cmath>

#include "fire.h"
#include "output.h"

using namespace fw;

void setUp() {}
void tearDown() {}

// The shipped layout: left side up, across the top, right side down; no bottom edge.
static std::vector<LedPos> sides87() {
  const Segment segs[] = {{0, 0, 0, 1, 23}, {0, 1, 1, 1, 41}, {1, 1, 1, 0, 23}};
  return buildLayout(segs, 3);
}

void test_layout_positions() {
  auto l = sides87();
  TEST_ASSERT_EQUAL(87, l.size());
  TEST_ASSERT_FLOAT_WITHIN(1e-4, 0.0f, l[0].x);
  TEST_ASSERT_FLOAT_WITHIN(1e-4, 0.5f / 23, l[0].y);
  TEST_ASSERT_FLOAT_WITHIN(1e-4, 1.0f, l[23].y);
  TEST_ASSERT_FLOAT_WITHIN(1e-4, 1.0f, l[86].x);
}

void test_noise_is_bounded_and_periodic() {
  for (float x = -3; x < 300; x += 0.37f) {
    float n = FireEngine::noise3(x, 1.3f, 2.7f);
    TEST_ASSERT_TRUE(n > -1.1f && n < 1.1f);
    TEST_ASSERT_FLOAT_WITHIN(1e-3, n, FireEngine::noise3(x + 256, 1.3f, 2.7f));
  }
}

void test_palette_monotonic_brightness() {
  float prev = -1;
  for (float h = 0; h <= 1.4f; h += 0.05f) {
    Rgbw c = FireEngine::heatToColor(h);
    float lum = c.r * 0.3f + c.g * 0.6f + c.w * 0.5f;
    TEST_ASSERT_TRUE(lum >= prev - 1e-6f);
    TEST_ASSERT_TRUE(c.r >= c.g && c.g >= c.b);  // always warm: never green/blue-tinted
    prev = lum;
  }
}

static float lum(const Rgbw& c) { return 0.21f * c.r + 0.72f * c.g + 0.76f * c.w; }

static double cv(const std::vector<double>& v) {
  double m = 0, s = 0;
  for (double x : v) m += x;
  m /= v.size();
  for (double x : v) s += (x - m) * (x - m);
  return std::sqrt(s / v.size()) / m;
}

void test_fire_is_steady_low_and_lively_on_top() {
  auto layout = sides87();
  FireEngine fire(layout, 1234);
  std::vector<Rgbw> px(fire.size());
  double change = 0;
  std::vector<double> top, low;
  std::vector<float> last(fire.size(), 0);
  for (int f = 0; f < 3000; ++f) {
    fire.render(f / 100.0, px.data());
    double t = 0, b = 0;
    for (size_t i = 0; i < px.size(); ++i) {
      float l = lum(px[i]);
      if (layout[i].y > 0.99f) t += l;
      if (layout[i].y < 0.33f) b += l;
      if (f) change += std::fabs(l - last[i]);
      last[i] = l;
    }
    top.push_back(t);
    low.push_back(b);
  }
  TEST_ASSERT_TRUE(cv(top) > 2.0 * cv(low));  // steady low on the sides, lively flame tips
  TEST_ASSERT_TRUE(change / (3000.0 * 87) > 0.001);  // flickers
  TEST_ASSERT_TRUE(change / (3000.0 * 87) < 0.05);   // but not strobing at 100 fps
}

void test_fire_survives_long_uptime() {
  FireEngine fire(sides87(), 99);
  std::vector<Rgbw> px(fire.size());
  double t0 = 45.0 * 24 * 3600;  // 45 days
  for (int f = 0; f < 500; ++f) {
    fire.render(t0 + f / 100.0, px.data());
    for (auto& c : px) TEST_ASSERT_TRUE(std::isfinite(c.r) && c.r >= 0);
  }
}

void test_dither_average_matches_intent() {
  OutputConfig cfg;
  OutputStage out(1, cfg);
  Rgbw c{0.3f / 255.0f, 0, 0, 7.5f / 255.0f};  // well below 1 LSB on red
  uint8_t px[4];
  double sumR = 0, sumW = 0;
  for (int i = 0; i < 1024; ++i) {
    out.process(&c, 1.0f, px);
    sumR += px[0];
    sumW += px[3];
  }
  TEST_ASSERT_FLOAT_WITHIN(0.01, 0.3, sumR / 1024);
  TEST_ASSERT_FLOAT_WITHIN(0.01, 7.5, sumW / 1024);
  Rgbw zero{};
  for (int i = 0; i < 300; ++i) {
    out.process(&zero, 1.0f, px);
    TEST_ASSERT_EQUAL(0, px[0] + px[1] + px[2] + px[3]);
  }
}

void test_power_limit() {
  OutputConfig cfg;
  cfg.maBudget = 2000;
  std::vector<Rgbw> white(128, Rgbw{1, 1, 1, 1});
  OutputStage out(128, cfg);
  std::vector<uint8_t> px(128 * 4);
  out.process(white.data(), 1.0f, px.data());
  TEST_ASSERT_FLOAT_WITHIN(1.0, 2000.0, out.currentMa());
  TEST_ASSERT_FLOAT_WITHIN(0.01, (2000.0 - 250 - 128) / (128 * 4 * 15.0), out.limitScale());
}

void test_rgb_strip_folds_white() {
  OutputConfig cfg;
  cfg.rgbw = false;
  OutputStage out(1, cfg);
  Rgbw c{0, 0, 0, 1};
  uint8_t px[3];
  out.process(&c, 1.0f, px);
  TEST_ASSERT_EQUAL(255, px[0]);
  TEST_ASSERT_TRUE(px[1] > 100 && px[1] < 180);
}

void test_fader() {
  Fader f(2.0f, 1.0f);
  TEST_ASSERT_TRUE(f.idle());
  f.setTarget(true);
  f.step(1.0f);
  TEST_ASSERT_FLOAT_WITHIN(1e-4, 0.5f, f.value());
  f.step(5.0f);
  TEST_ASSERT_FLOAT_WITHIN(1e-4, 1.0f, f.value());
  f.setTarget(false);
  f.step(1.0f);
  TEST_ASSERT_TRUE(f.idle());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_layout_positions);
  RUN_TEST(test_noise_is_bounded_and_periodic);
  RUN_TEST(test_palette_monotonic_brightness);
  RUN_TEST(test_fire_is_steady_low_and_lively_on_top);
  RUN_TEST(test_fire_survives_long_uptime);
  RUN_TEST(test_dither_average_matches_intent);
  RUN_TEST(test_power_limit);
  RUN_TEST(test_rgb_strip_folds_white);
  RUN_TEST(test_fader);
  return UNITY_END();
}
