#include <unity.h>

#include <cmath>
#include <cstring>
#include <vector>

#include "fire.h"
#include "tuning.h"

using namespace fw;

void setUp() {}
void tearDown() {}

static std::vector<LedPos> sides72() {
  const Segment segs[] = {{0, 0, 0, 1, 17}, {0, 1, 1, 1, 38}, {1, 1, 1, 0, 17}};
  return buildLayout(segs, 3);
}

void test_table_is_sane() {
  FireParams d;
  for (size_t i = 0; i < kTunableCount; ++i) {
    const Tunable& t = kTunables[i];
    TEST_ASSERT_TRUE(std::strlen(t.key) <= 13);  // leaves room for a 2-char prefix in NVS
    TEST_ASSERT_TRUE(t.min < t.max && t.step > 0);
    float v = d.*(t.field);
    TEST_ASSERT_TRUE(v >= t.min && v <= t.max);  // the shipped look is reachable
    TEST_ASSERT_EQUAL_PTR(&t, findTunable(t.key));
    for (size_t j = 0; j < i; ++j) TEST_ASSERT_TRUE(std::strcmp(t.key, kTunables[j].key) != 0);
  }
}

void test_set_clamps_and_rejects_unknown() {
  FireParams p;
  TEST_ASSERT_TRUE(setTunable(p, "orange", 1.7f));
  TEST_ASSERT_EQUAL_FLOAT(1.7f, p.orange);
  TEST_ASSERT_TRUE(setTunable(p, "orange", 99.0f));
  TEST_ASSERT_EQUAL_FLOAT(3.0f, p.orange);
  TEST_ASSERT_TRUE(setTunable(p, "speed", -5.0f));
  TEST_ASSERT_EQUAL_FLOAT(0.25f, p.speed);
  TEST_ASSERT_TRUE(setTunable(p, "flickTop", NAN));
  TEST_ASSERT_EQUAL_FLOAT(FireParams{}.flickerTop, p.flickerTop);
  TEST_ASSERT_FALSE(setTunable(p, "nope", 1.0f));
}

void test_color_gains_scale_channels() {
  FireEngine a(sides72(), 5), b(sides72(), 5);
  b.params.red = 0.5f;
  b.params.orange = 2.0f;
  b.params.white = 0.0f;
  std::vector<Rgbw> pa(a.size()), pb(b.size());
  a.render(3.0, pa.data());
  b.render(3.0, pb.data());
  for (size_t i = 0; i < pa.size(); ++i) {
    TEST_ASSERT_FLOAT_WITHIN(1e-6, pa[i].r * 0.5f, pb[i].r);
    TEST_ASSERT_FLOAT_WITHIN(1e-6, pa[i].g * 2.0f, pb[i].g);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, pb[i].w);
  }
}

// Double speed over 10 s shows the same fire as normal speed over 20 s, flares included.
void test_speed_scales_time() {
  FireEngine a(sides72(), 11), b(sides72(), 11);
  b.params.speed = 2.0f;
  std::vector<Rgbw> pa(a.size()), pb(b.size());
  for (int f = 0; f <= 2000; ++f) {
    a.render(f / 100.0, pa.data());
    b.render(f / 200.0, pb.data());
  }
  for (size_t i = 0; i < pa.size(); ++i) TEST_ASSERT_FLOAT_WITHIN(1e-3, pa[i].r, pb[i].r);
}

// Changing speed mid-run continues from where the fire is instead of jumping ahead.
void test_speed_change_is_continuous() {
  FireEngine fire(sides72(), 3);
  std::vector<Rgbw> before(fire.size()), after(fire.size());
  for (int f = 0; f <= 1000; ++f) fire.render(f / 100.0, before.data());
  fire.params.speed = 3.0f;
  fire.render(10.001, after.data());
  for (size_t i = 0; i < before.size(); ++i) TEST_ASSERT_FLOAT_WITHIN(0.05f, before[i].r, after[i].r);
}

void test_flare_max_below_min_still_works() {
  FireEngine fire(sides72(), 9);
  fire.params.flareMax = 0.0f;
  fire.params.flareMeanIntervalS = 1.0f;
  std::vector<Rgbw> px(fire.size());
  for (int f = 0; f < 1000; ++f) {
    fire.render(f / 50.0, px.data());
    for (auto& c : px) TEST_ASSERT_TRUE(std::isfinite(c.r) && c.r >= 0);
  }
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_table_is_sane);
  RUN_TEST(test_set_clamps_and_rejects_unknown);
  RUN_TEST(test_color_gains_scale_channels);
  RUN_TEST(test_speed_scales_time);
  RUN_TEST(test_speed_change_is_continuous);
  RUN_TEST(test_flare_max_below_min_still_works);
  return UNITY_END();
}
